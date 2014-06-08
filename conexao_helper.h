/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Tratamento de conexões, algumas funções secundárias
*******************************************************************************/

#ifndef __CONEXAO_HELPER_H__
#define __CONEXAO_HELPER_H__

pthread_t threads[MAX_CLIENTES];
// Está como ERRORCHECK pela função th_limpeza. A maior parte do código
// a chama com o mutex já adquirido, mas leitura() a chama sem adquirí-lo.
pthread_mutex_t processando = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
///@TODO: arrumar isso, tem muita coisa declarada junto
int s, clientes_agora = 0, clientes_total = 0, caronas_total = 0;
int caminhos[MAX_CLIENTES][30], pos_atual[MAX_CLIENTES] = {0};
#ifndef NAO_CHECA_JA_CONECTADO
__int128_t conectados[MAX_CLIENTES] = {0};	// lista de IPs já conectados
#endif
int comm[MAX_CLIENTES], fd[MAX_CLIENTES];

pthread_key_t dados_thread;
int pilha_threads_livres[MAX_CLIENTES];
char *usuario_da_carona, *placa[MAX_CLIENTES];
tsd_t tsd[MAX_CLIENTES];

// Algumas constantes para facilitar o código
// Envia pela thread atual:
#define envia(msg, len)		th_try(write(tsd->fd_con, msg, len), "write")
// Envia string pela thread atual:
#define envia_str(str)		envia(str, strlen(str) + 1)
// Envia variável com tamanho fixo pela thread atual:
#define envia_fixo(objeto)	envia(objeto, sizeof(objeto))
#define finaliza(msg)		do{envia_str(msg); pthread_exit(NULL);}while(0)

void adquire_mutex() {
	tsd_t *tsd = pthread_getspecific(dados_thread);
	
	printf("%d - pedindo mutex\n", tsd->n_thread);
	pthread_mutex_lock(&processando);
	printf("%d - adquiriu mutex\n", tsd->n_thread);
	
}

void solta_mutex() {
	tsd_t *tsd = pthread_getspecific(dados_thread);
	
	printf("%d - soltando mutex\n", tsd->n_thread);
	pthread_mutex_unlock(&processando);
	
}

#ifndef NAO_CHECA_JA_CONECTADO
inline int ja_conectado(const struct in_addr *ip) {
	int i;
	for (i = 0; i < MAX_CLIENTES; i++) {
		if (conectados[i] == *(__int128_t *)ip) {
			return 1;
		}
	}
	return 0;
}
#endif


#ifndef NAO_CHECA_JA_CONECTADO
inline void aceita_conexao(int fd_con, const struct in_addr *ip) {
#else
inline void aceita_conexao(int fd_con) {
#endif
	
	int n_thread;
	pthread_mutex_lock(&processando);
	n_thread = pilha_threads_livres[clientes_agora];
	#ifndef NAO_CHECA_JA_CONECTADO
	conectados[clientes_agora] = *(uint32_t *)ip;
	#endif
	clientes_agora++;
	clientes_total++;
	
	tsd[n_thread].n_thread = n_thread;	///@TODO: ao invés de salvar n_thread na tsd, dá pra recuperar calculando o índice da entrada
	tsd[n_thread].fd_con = fd_con;
	
	pthread_mutex_unlock(&processando);
	
	pthread_create(&threads[tsd->n_thread], NULL, th_conecao_cliente, &tsd[n_thread]);	// free() de argumentos será pela thread
	pthread_detach(threads[tsd->n_thread]);			// não receber retorno e liberar recursos ao final da execução
}

// Limpeza de recursos ao terminar thread
void* th_limpeza(void *tmp) {
	tsd_t *tsd = tmp;
	adquire_mutex();
	clientes_agora--;
	#ifndef NAO_CHECA_JA_CONECTADO
	conectados[tsd->n_thread] = 0;
	#endif
	fila[tsd->n_thread].tipo = -1;
	pilha_threads_livres[clientes_agora] = tsd->n_thread;
	remove_fila(tsd->n_thread);
	printf("Thread %d: disconexão\n", tsd->n_thread);
	close(tsd->fd_con);
	solta_mutex();
	return NULL;
}

/*******************************************************************************
 * char *leitura(leitura_t *leitura);
 * Retorna ponteiro para próximo pacote JSON (busca após byte nulo, que
 * representa o fim do atual)
 ******************************************************************************/
char *leitura(leitura_t *l) {
	char *busca_nulo, *ret;
	
	tsd_t *tsd = pthread_getspecific(dados_thread);
	
	if (tsd == NULL) {
		// Erro bem improvável (a chave foi inicializada em init.c), mas é bom tratá-lo aqui
		fprintf(stderr, "leitura: tsd não encontrada\n");
		finaliza("{\"msg\":\"leitura(): TSD não encontrada\nIsso é um bug, reporte-o!!!\",\"fim\":null}");
	}
	
	// Se já temos a próxima mensagem JSON completa nesse pacote
	if ((busca_nulo = memchr(l->fim_msg + 1, 0, l->fim_pacote - l->fim_msg)) != NULL) {
		ret = l->fim_msg + 1;
		// Atualizamos o fim do pacote
		l->fim_msg = busca_nulo;
		// Retornamos a próxima msg
		return ret;
	}
	
	// Senão, copiamos o início do pacote desejado para o início do buffer e lemos até completar l->tamanho_max bytes
	l->fim_pacote -= l->fim_msg - l->area + 1;
	memmove(l->area, l->fim_msg + 1, l->fim_msg - l->area + 1);
	l->fim_msg = l->area - 1;
	
	int bytes_lidos;
	for (;;) {
		bytes_lidos = read(tsd->fd_con, l->fim_pacote + 1, l->tamanho_area - (l->fim_pacote + 1 - l->area));
		
		if (bytes_lidos <= 0) {
			if (bytes_lidos == -1)
				perror("leitura: read");
			finaliza("{\"msg\":\"Leitura vazia\",\"fim\":null}");
		}
		
		l->fim_pacote += bytes_lidos;
		
		if ((busca_nulo = memchr(l->fim_msg + 1, 0, l->fim_pacote - l->fim_msg)) != NULL) {
			// Atualizamos o fim da mensagem JSON
			l->fim_msg = busca_nulo;
			// Retornamos a próxima msg
			return l->area;
		}
	}
}

void recebe_dados(leitura_t *l, json_parser *json) {
	solta_mutex();
	json->start = leitura(l);
	if (json_all_parse(json) < 0) {
		tsd_t *tsd = pthread_getspecific(dados_thread);
		fprintf(stderr, "%d: Falha JSON parse\n", tsd->n_thread);
		finaliza("{\"msg\":\"Falha JSON parse\",\"fim\":null}");
	}
	adquire_mutex();
}

int distancia(int da_carona, int inicio, int fim) {
	int i, j;
	printf("%d %d %d\n", da_carona, inicio, fim);
	if (fila[da_carona].tipo != FILA_DA_CARONA)
		return -1;
	
	for (i = 0; i < (int) sizeof(caminhos[0]); i++) {
		if (caminhos[da_carona][i] == inicio) {
			printf("inicio match\n");
			for (j = i + 1; j < (int) sizeof(caminhos[0]); j++) {
				if (caminhos[da_carona][j] == fim) {
					printf("Compatível: %d, distância %d\n", da_carona, j - i);
					return j - i;
				}
			}
			break;
		}
	}
	return -1;
}

#endif
