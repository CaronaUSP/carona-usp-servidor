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
int caminhos[MAX_CLIENTES][50], posicoes_livres[MAX_CLIENTES][50];
#ifndef NAO_CHECA_JA_CONECTADO
__int128_t conectados[MAX_CLIENTES] = {0};	// lista de IPs já conectados
#endif
int comm[MAX_CLIENTES];

pthread_key_t dados_thread;
int pilha_threads_livres[MAX_CLIENTES];
char *usuario_da_carona;
tsd_t tsd_array[MAX_CLIENTES];

// Algumas constantes para facilitar o código
// Envia pela thread atual:
#define envia(msg, len)		th_try(write(tsd->fd_con, msg, len), "write")
// Envia string pela thread atual:
#define envia_str(str)		envia(str, strlen(str) + 1)
// Envia variável com tamanho fixo pela thread atual:
#define envia_fixo(objeto)	envia(objeto, sizeof(objeto))
#define finaliza(msg)		do{envia_str(msg); pthread_exit(NULL);}while(0)

#define th_error(msg)		do {fprintf(stderr, "Thread %d: %s: %s\n", tsd->n_thread, msg, strerror(errno)); pthread_exit(NULL);} while(0)
#define th_try(cmd,msg)		do {if ((cmd) == -1) {th_error(msg);}} while(0)
// Envia string por outra thread:
#define envia_str_outro(thread, str)		envia_outro(thread, str, strlen(str) + 1)
// Envia variável com tamanho fixo ppor outra thread:
#define envia_fixo_outro(thread, objeto)	envia_outro(thread, objeto, sizeof(objeto))

int envia_outro(int thread, char *msg, size_t len) {
	int status = write(tsd_array[thread].fd_con, msg, len);
	tsd_t *tsd = pthread_getspecific(dados_thread);
	
	if (status == -1) {
		fprintf(stderr, "Thread %d: %s: %s em mensagem para thread %d\n", tsd->n_thread, "write", strerror(errno), thread);
		return -1;
	}
	return 0;
}

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
	
	tsd_array[n_thread].n_thread = n_thread;	///@TODO: ao invés de salvar n_thread na tsd, dá pra recuperar calculando o índice da entrada
	tsd_array[n_thread].fd_con = fd_con;
	
	pthread_mutex_unlock(&processando);
	
	pthread_create(&threads[n_thread], NULL, th_conecao_cliente, &tsd_array[n_thread]);	// free() de argumentos será pela thread
	pthread_detach(threads[n_thread]);			// não receber retorno e liberar recursos ao final da execução
}

int par_vazio(int n) {
	int i;
	for (i = 0; i < MAX_PARES; i++)
		if (tsd_array[n].pares[i] == -1)
			return i;
	return 0;
}

// Limpeza de recursos ao terminar thread
void* th_limpeza(void *tmp) {
	tsd_t *tsd = tmp;
	int i, j;
	
	adquire_mutex();
	
	for (i = 0; i < MAX_PARES; i++) {	// se pareado, liberar os pares
		if (tsd->pares[i] == -1)
			break;
		for (j = 0; j < MAX_PARES; j++)
			if (tsd_array[tsd->pares[i]].pares[j] == tsd->n_thread) {
				tsd_array[tsd->pares[i]].pares[j] = -1;
				if (fila[tsd->pares[i]].tipo == FILA_RECEBE_CARONA_PAREADO) {	// se outro iria receber carona
					fila[tsd->pares[i]].tipo = FILA_RECEBE_CARONA;			// não mais
					write(tsd_array[tsd->pares[i]].fd_con, "{\"msg\":\"Sem conexão com motorista, esperando outra carona\"}", sizeof("{\"msg\":\"Sem conexão com motorista, esperando outra carona\"}" ));
				} else if (fila[tsd->pares[i]].tipo == FILA_DA_CARONA) {	// se outro iria dar carona
					// libera lugares:
					int k = 0, l = 0;
					while (caminhos[tsd->pares[i]][k] != tsd->inicio)
						k++;
					l = k;
					while (caminhos[tsd->pares[i]][l] != tsd->fim) {
						posicoes_livres[tsd->pares[i]][l]++;
						l++;
					}
					write(tsd_array[tsd->pares[i]].fd_con, "{\"msg\":\"Conexão perdida com um carona\"}", sizeof("{\"msg\":\"Conexão perdida com um carona\"}"));
				}
				break;
			}
	}
	
	remove_fila(tsd->n_thread);
	solta_mutex();
	
	write(tsd->fd_con, "{\"fim\":null}", sizeof("{\"fim\":null}"));
	///@TODO: esperar cliente fechar a conexão ao invés de sleep()
	sleep(5);
	
	adquire_mutex();
	clientes_agora--;
	#ifndef NAO_CHECA_JA_CONECTADO
	conectados[tsd->n_thread] = 0;
	#endif
	pilha_threads_livres[clientes_agora] = tsd->n_thread;
	printf("Thread %d: disconexão\n", tsd->n_thread);
	close(tsd->fd_con);
	tsd->fd_con = tsd->n_thread = -1;	// desnecessário, mas facilita depuração (entradas -1 = entradas alocadas e liberadas)
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
		finaliza("{\"msg\":\"leitura(): TSD não encontrada\nIsso é um bug, reporte-o!!!\"}");
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
			finaliza("{\"msg\":\"Leitura vazia\"}");
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
		finaliza("{\"msg\":\"Falha JSON parse\"}");
	}
	adquire_mutex();
}

int distancia(int da_carona, int recebe_carona) {
	int i, j;
	if (fila[da_carona].tipo != FILA_DA_CARONA || fila[recebe_carona].tipo != FILA_RECEBE_CARONA ||
		par_vazio(da_carona) == -1)
		return -1;
	
	for (i = 0; i < (int) sizeof(caminhos[0]) && caminhos[da_carona][i] != -1; i++) {
		if (caminhos[da_carona][i] == tsd_array[recebe_carona].inicio) {
			for (j = i; j < (int) sizeof(caminhos[0]) && caminhos[da_carona][j] != -1; j++) {
				if (caminhos[da_carona][j] == tsd_array[recebe_carona].fim) {
					printf("Compatível: %d, distância %d\n", da_carona, j - i);
					return j - i;
				}				
				if (posicoes_livres[da_carona][j] == 0)
					return -1;	// sem lugar livre
			}
			break;
		}
	}
	return -1;
}

int parada(int carro, int passageiro) {
	int i = 0, j;
	while (caminhos[carro][i] != tsd_array[passageiro].inicio)
		i++;
	j = i;
	while (caminhos[carro][j] != tsd_array[passageiro].fim) {
		posicoes_livres[carro][j]--;
		j++;
	}
	return i;
}

typedef struct {
	int id, melhor, parar;
} comparador_t;

void compara_recebe_carona(comparador_t *compara) {
	int comparar_com = -1, d_menor = 2000, d_atual;
	compara->melhor = -1;
	while ((comparar_com = prox_fila(comparar_com)) != -1) {
		printf("Comparando com %d\n", comparar_com);
		d_atual = distancia(comparar_com, compara->id);
		if (d_atual != -1) {  // caminho compatível
			if (d_atual < d_menor) {
				d_menor = d_atual;
				compara->melhor = comparar_com;
			}
		}
	}
	// seta ponto de parada:
	if (compara->melhor != -1)
		compara->parar = parada(compara->melhor, compara->id);
}

void compara_da_carona(comparador_t *compara) {
	int comparar_com = -1, d_maior = 0, d_atual;
	compara->melhor = -1;
	while ((comparar_com = prox_fila(comparar_com)) != -1) {
		printf("Comparando com %d\n", comparar_com);
		d_atual = distancia(compara->id, comparar_com);
		if (d_atual != -1) {  // caminho compatível
			if (d_atual > d_maior) {
				d_maior = d_atual;
				compara->melhor = comparar_com;
			}
		}
	}
	// seta ponto de parada:
	if (compara->melhor != -1)
		compara->parar = parada(compara->melhor, compara->id);
}

#endif
