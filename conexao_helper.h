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
pthread_mutex_t processando = PTHREAD_MUTEX_INITIALIZER;  // o código não está muito confiável  

// Algumas constantes para facilitar o código
// Envia pela thread atual:
#define envia(msg, len)		th_try(write(tsd->fd_con, msg, len), "write")
// Envia string pela thread atual:
#define envia_str(str)		envia(str, strlen(str) + 1)
// Envia variável com tamanho fixo pela thread atual:
#define envia_fixo(objeto)	envia(objeto, sizeof(objeto))
#define finaliza(msg)		do{envia_str(msg); pthread_exit(NULL);}while(0)


#ifndef NAO_CHECA_JA_CONECTADO
inline int ja_conectado(const struct in_addr *ip) {
	int i;
	for (i = 0; i < MAX_CLIENTES; i++) {
		if (conectados[i] == *(uint32_t *)ip) {
			return 1;
		}
	}
	return 0;
}
#endif

inline void aceita_conexao(tsd_t *tsd, const struct in_addr *ip) {
	
	pthread_mutex_lock(&mutex_modifica_thread);
	tsd->n_thread = pilha_threads_livres[clientes_agora];
	conectados[clientes_agora] = *(uint32_t *)ip;
	clientes_agora++;
	clientes_total++;
	pthread_mutex_unlock(&mutex_modifica_thread);
	
	pthread_create(&threads[tsd->n_thread], NULL, th_conecao_cliente, tsd);	// free() de argumentos será pela thread
	pthread_detach(threads[tsd->n_thread]);			// não receber retorno e liberar recursos ao final da execução
}

// Limpeza de recursos ao terminar thread
void* th_limpeza(void *tmp) {
	tsd_t *tsd = tmp;
	pthread_mutex_unlock(&processando);
	pthread_mutex_lock(&mutex_modifica_thread);
	clientes_agora--;
	conectados[tsd->n_thread] = 0;
	pilha_threads_livres[clientes_agora] = tsd->n_thread;
	pthread_mutex_unlock(&mutex_modifica_thread);
	printf("Thread %d: disconexão\n", tsd->n_thread);
	close(tsd->fd_con);
	free(tsd);
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
  pthread_mutex_unlock(&processando);
	leitura(l);
	if (json_all_parse(json) < 0) {
    tsd_t *tsd = pthread_getspecific(dados_thread);
		fprintf(stderr, "%d: Falha JSON parse\n", tsd->n_thread);
		finaliza("{\"msg\":\"Falha JSON parse\",\"fim\":null}");
	}
  pthread_mutex_lock(&processando);
}

#endif
