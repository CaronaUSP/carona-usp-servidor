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
pthread_cond_t comunica_thread;
pthread_key_t dados_thread;

inline int ja_conectado(const struct in_addr *ip) {
	int i;
	for (i = 0; i < MAX_CLIENTES; i++) {
		if (conectados[i] == *(uint32_t *)ip) {
			return 1;
		}
	}
	return 0;
}

inline void aceita_conexao(tsd_t *tsd, const struct in_addr *ip) {
	pthread_t thread;
	pthread_create(&thread, NULL, th_conecao_cliente, tsd);	// free() de argumentos será pela thread
	pthread_detach(thread);			// não receber retorno e liberar recursos ao final da execução
	
	pthread_mutex_lock(&mutex_modifica_thread);
	conectados[clientes_agora] = *(uint32_t *)ip;
	clientes_agora++;
	pthread_mutex_unlock(&mutex_modifica_thread);
	clientes_total++;
}

// Limpeza de recursos ao terminar thread
void* th_limpeza(void *tmp) {
	tsd_t *tsd = tmp;
	pthread_mutex_lock(&mutex_modifica_thread);
	clientes_agora--;
	conectados[tsd->n_thread] = 0;
	pthread_mutex_unlock(&mutex_modifica_thread);
	printf("Thread %d: disconexão\n", tsd->n_thread);
	close(tsd->fd_con);
	free(tsd);
	return NULL;
}

// Algumas constantes para facilitar o código
// Envia pela thread atual:
#define envia(msg, len)		th_try(write(tsd->fd_con, msg, len), "write")
// Envia string pela thread atual:
#define envia_str(str)		envia(str, strlen(str) + 1)
// Envia variável com tamanho fixo pela thread atual:
#define envia_fixo(objeto)	envia(objeto, sizeof(objeto))
#define finaliza(msg)		do{tsd->errmsg = msg; pthread_exit(NULL);}while(0)

/*******************************************************************************
 * char *leitura(leitura_t *leitura);
 * Retorna ponteiro para próximo pacote JSON (busca após byte nulo, que
 * representa o fim do atual
 ******************************************************************************/
 ///@TODO: Preciso testar isso direito
char *leitura(leitura_t *l) {
	char *prox_msg;
	
	tsd_t *tsd = pthread_getspecific(dados_thread);
	
	if (tsd == NULL) {
		// Erro bem improvável (a chave foi inicializada em init.c), mas é bom tratá-lo aqui
	}
	
	// Se já temos a próxima mensagem JSON completa nesse pacote
	if ((prox_msg = memchr(&l->area[l->fim_json_atual], 0, l->fim_pacote - l->fim_json_atual)) != NULL) {
		// Atualizamos o fim do pacote
		l->fim_json_atual = prox_msg - l->area + 1;
		// Retornamos a próxima msg
		return prox_msg;
	}
	
	// Senão, copiamos o início do pacote desejado para o início do buffer e lemos até completar l->tamanho_max bytes
	l->fim_pacote -= l->fim_json_atual;
	memmove(l->area, &l->area[l->fim_json_atual], l->fim_pacote);
	// fim_json_atual = 0
	
	
	int bytes_lidos;
	for (;;) {
		bytes_lidos = read(tsd->fd_con, l->area + l->fim_pacote - l->fim_json_atual, l->tamanho_max - (l->fim_pacote - l->fim_json_atual));
		
		if (bytes_lidos <= 0)
			finaliza("{\"msg\":\"Leitura vazia\",\"fim\"}");
		
		l->fim_pacote += bytes_lidos;
		
		if ((prox_msg = memchr(&l->area[l->fim_json_atual], 0, l->fim_pacote - l->fim_json_atual)) != NULL) {
			// Atualizamos o fim do pacote
			l->fim_json_atual = prox_msg - l->area;
			// Retornamos a próxima msg
			return prox_msg;
		}
	}
}

void* th_conecao_cliente(void *tmp) {
	const tsd_t *tsd = tmp;	// o argumento é um ponteiro para qqr área definida pelo programa,
								// então precisamos marcar o tipo de ponteiro recebido ou usar casts
	leitura_t l;
	pthread_setspecific(dados_thread, tmp);
	pthread_cleanup_push((void *)th_limpeza, tmp);
	printf("Thread criada, fd = %d\n", tsd->fd_con);
	char mensagem[200];	///@FIXME: assume que dados iniciais cabem em 200 bytes
	sprintf(mensagem, "{\"login\":\"Carona Comunitária USP\n%s\n%d clientes já conectados, %d atualmente, %d caronas dadas\"}""",
				MSG_NOVIDADES, clientes_total, clientes_agora, caronas_total);
	envia(mensagem, strlen(mensagem) + 1);
	
	char resposta[1024], *hash, *usuario;
	
	///@FIXME: aceita mensagens até 1024 bytes, senão as corta
	l.area = resposta;
	l.fim_json_atual = 0;
	l.fim_pacote = 0;
	l.tamanho_max = sizeof(resposta);
	
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
	if (senha_correta(usuario, mensagem, hash))
		printf("Autenticado!\n");
	else
		printf("Falha de autenticação\n");
	
	pthread_cleanup_pop(1);
	return NULL;
}
