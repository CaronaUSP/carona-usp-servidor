/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Tratamento de conexões e threads
*******************************************************************************/

#include "conexao.h"
#include "json.h"

int s, clientes_agora = 0, clientes_total = 0, caronas_total = 0;
int caminhos[MAX_CLIENTES][3];
uint32_t conectados[MAX_CLIENTES] = {0};	// lista de IPs já conectados
pthread_mutex_t mutex_modifica_thread = PTHREAD_MUTEX_INITIALIZER, mutex_comunicacao = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t comunica_thread;//[MAX_CLIENTES];
pthread_key_t dados_thread;
int pilha_threads_livres[MAX_CLIENTES];
pthread_t threads[MAX_CLIENTES];
char *usuario_da_carona;

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

// Algumas constantes para facilitar o código
// Envia pela thread atual:
#define envia(msg, len)		th_try(write(tsd->fd_con, msg, len), "write")
// Envia string pela thread atual:
#define envia_str(str)		envia(str, strlen(str) + 1)
// Envia variável com tamanho fixo pela thread atual:
#define envia_fixo(objeto)	envia(objeto, sizeof(objeto))
#define finaliza(msg)		do{envia_str(msg); pthread_exit(NULL);}while(0)

// Limpeza de recursos ao terminar thread
void* th_limpeza(void *tmp) {
	tsd_t *tsd = tmp;
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
 * representa o fim do atual
 ******************************************************************************/
 ///@TODO: Preciso testar isso direito
char *leitura(leitura_t *l) {
	char *busca_nulo, *ret;
	
	tsd_t *tsd = pthread_getspecific(dados_thread);
	
	if (tsd == NULL) {
		// Erro bem improvável (a chave foi inicializada em init.c), mas é bom tratá-lo aqui
		printf("leitura: tsd não encontrada\n");
		finaliza("{\"msg\":\"leitura(): TSD não encontrada\nIsso é um bug, reporte-o!!!\",\"fim\"}");
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
	
	
	int bytes_lidos;
	for (;;) {
		bytes_lidos = read(tsd->fd_con, l->fim_pacote + 1, l->tamanho_area - (l->fim_pacote + 1 - l->area));
		
		if (bytes_lidos <= 0) {
			if (bytes_lidos == -1)
				perror("leitura: read");
			finaliza("{\"msg\":\"Leitura vazia\",\"fim\"}");
		}
		
		l->fim_pacote += bytes_lidos;
		
		if ((busca_nulo = memchr(l->fim_msg	 + 1, 0, l->fim_pacote - l->fim_msg)) != NULL) {
			// Atualizamos o fim da mensagem JSON
			l->fim_msg = busca_nulo;
			// Retornamos a próxima msg
			return l->area;
		}
	}
}

void* th_conecao_cliente(void *tmp) {
	tsd_t *tsd = tmp;	// o argumento é um ponteiro para qqr área definida pelo programa,
								// então precisamos marcar o tipo de ponteiro recebido ou usar casts
	leitura_t l;
	pthread_setspecific(dados_thread, tmp);
	pthread_cleanup_push((void *)th_limpeza, tmp);
	printf("Thread criada, fd = %d\n", tsd->fd_con);
	char mensagem[1000];	///@FIXME: assume que dados cabem em 1000 bytes
	pthread_mutex_lock(&mutex_modifica_thread);
	sprintf(mensagem, "{\"login\":\"Carona Comunitária USP\n%s\n%d clientes já conectados, %d atualmente, %d caronas dadas\"}""",
				MSG_NOVIDADES, clientes_total, clientes_agora, caronas_total);
	pthread_mutex_unlock(&mutex_modifica_thread);
	envia(mensagem, strlen(mensagem) + 1);
	
	char resposta[1024], *hash, *usuario;
	
	///@FIXME: aceita mensagens até 1024 bytes, senão as corta
	l.area = resposta;
	l.fim_pacote = resposta - 1;
	l.fim_msg = resposta - 1;
	l.tamanho_area = sizeof(resposta);
	leitura(&l);
	
	json_parser json;
	json_pair pairs[200];
	json.start = resposta;
	json.pairs = pairs;
	json.n_pairs = 200;
	
	if (json_all_parse(&json) < 0) {
		printf("Falha JSON parse\n");
		pthread_exit(NULL);
	}
	if ((hash = json_get_str(&json, "hash")) == NULL) {
		printf("Chave \"hash\" não encontrada\n");
		pthread_exit(NULL);
	}
	if (strlen(hash) != 64) {
		printf("Hash != 64 bytes!\n");
		pthread_exit(NULL);
	}
	
	usuario = json_get_str(&json, "usuario");
	
	if (usuario == NULL) {
		printf("Chave \"usuario\" não encontrada\n");
		pthread_exit(NULL);
	}
	
	// Usuário é, por enquanto, ignorado no cálculo do hash (ver hash.c).
	// Hash da senha é "1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF"
	// para qualquer usuário enviado.
	
	sprintf(mensagem, "Carona Comunitária USP\n%s\n%d clientes já conectados, %d atualmente, %d caronas dadas",
				MSG_NOVIDADES, clientes_total, clientes_agora, caronas_total);
	
	if (senha_correta(usuario, mensagem, hash) == 0) {
		printf("Falha de autenticação\n");
		finaliza("{\"msg\":\"Falha de autenticação\",\"fim\"}");
	}
	
	int da_carona = json_get_bool(&json, "da_carona");
	
	if (da_carona == JSON_INVALID) {
		printf("Valor \"da_carona\" não encontrado\n");
		finaliza("{\"msg\":\"Valor \\\"da_carona\\\" não encontrado\",\"fim\"}");
	}
	
	if (da_carona) {
		// Bem básico e para dois clientes (um dando e um recebendo carona)
		// Será substituido por arrays mais tarde
		int ponto1, ponto2, ponto3;
		if ((ponto1 = json_get_int(&json, "ponto1")) == JSON_INVALID)
			finaliza("{\"msg\":\"Mensagem faltando pontos para percorrer\",\"fim\"}");
		if ((ponto2 = json_get_int(&json, "ponto2")) == JSON_INVALID)
			finaliza("{\"msg\":\"Mensagem faltando pontos para percorrer\",\"fim\"}");
		caminhos[0][0] = ponto1;
		caminhos[0][1] = ponto2;
		if ((ponto3 = json_get_int(&json, "ponto3")) != JSON_INVALID)
			caminhos[0][2] = ponto3;
		usuario_da_carona = usuario;
		pthread_cond_signal(&comunica_thread);
	} else {
		int inicio, fim;
		if ((inicio = json_get_int(&json, "inicio")) == JSON_INVALID)
			finaliza("{\"msg\":\"Mensagem faltando ponto inicial\",\"fim\"}");
		if ((fim = json_get_int(&json, "fim")) == JSON_INVALID)
			finaliza("{\"msg\":\"Mensagem faltando ponto final\",\"fim\"}");
		pthread_mutex_lock(&mutex_comunicacao);
		pthread_cond_wait(&comunica_thread, &mutex_comunicacao);
		printf("Cruzando usuários...\n");
		int i, j;
		for (i = 0; i < 3; i++) {
			if (caminhos[0][i] == inicio)
				for (j = i + 1; j < 3; j++) {
					if (caminhos[0][j] == fim) {
						sprintf(mensagem, "{\"msg\":\"Usuário %s dará carona! Trajeto %d, %d, %d\"}",
							usuario_da_carona, caminhos[0][0], caminhos[0][1], caminhos[0][2]);
						finaliza(mensagem);
					}
				}
		}
	}
	
	pthread_cleanup_pop(1);
	return NULL;
}
