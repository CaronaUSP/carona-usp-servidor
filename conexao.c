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
uint32_t conectados[MAX_CLIENTES] = {0};	// lista de IPs já conectados
pthread_mutex_t mutex_modifica_thread = PTHREAD_MUTEX_INITIALIZER, mutex_comunicacao = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t comunica_thread;


inline int ja_conectado(const struct in_addr *ip) {
	///@TODO: pode ser otimizado para buscar apenas conexões existentes na array?
	int i;
	for (i = 0; i < MAX_CLIENTES; i++) {
		if (conectados[i] == *(uint32_t *)ip) {
			return 1;
		}
	}
	return 0;
}

inline void aceita_conexao(args_thread *args, const struct in_addr *ip) {
	pthread_t thread;
	pthread_create(&thread, NULL, th_conecao_cliente, args);	// free() de argumentos será pela thread
	pthread_detach(thread);			// não receber retorno e liberar recursos ao final da execução
	
	pthread_mutex_lock(&mutex_modifica_thread);
	conectados[clientes_agora] = *(uint32_t *)ip;
	clientes_agora++;
	pthread_mutex_unlock(&mutex_modifica_thread);
	clientes_total++;
}

// Limpeza de recursos ao terminar thread
void* th_limpeza(void *tmp) {
	args_thread *args = tmp;
	pthread_mutex_lock(&mutex_modifica_thread);
	clientes_agora--;
	conectados[args->n_thread] = 0;
	pthread_mutex_unlock(&mutex_modifica_thread);
	printf("Thread %d: disconexão\n", args->n_thread);
	close(args->fd_con);
	free(args);
	return NULL;
}

// Algumas constantes para facilitar o código
// Envia pela thread atual:
#define envia(msg, len)		envia_dados(args, msg, len)
// Envia string pela thread atual:
#define envia_str(str)		envia(str, strlen(str) + 1)
// Envia variável com tamanho fixo pela thread atual:
#define envia_fixo(objeto)	envia(objeto, sizeof(objeto))

static inline void envia_dados(const args_thread *args, const void *buf, size_t count) {
	th_try(write(args->fd_con, buf, count), "write");
}

void* th_conecao_cliente(void *tmp) {
	const args_thread *args = tmp;	// o argumento é um ponteiro para qqr área definida pelo programa,
								// então precisamos marcar o tipo de ponteiro recebido ou usar casts
	int i;
	pthread_cleanup_push((void *)th_limpeza, tmp);
	printf("Thread criada, fd = %d\n", args->fd_con);
	char mensagem[200];	///@FIXME: assume que dados iniciais cabem em 200 bytes
	sprintf(mensagem, "{\"login\":\"Carona Comunitária USP\n%s\n%d clientes já conectados, %d atualmente, %d caronas dadas\"}""",
				MSG_NOVIDADES, clientes_total, clientes_agora, caronas_total);
	envia(mensagem, strlen(mensagem) + 1);
	
	char resposta[256];
	///@FIXME: aceita mensagens até 256 bytes, senão as corta
	int tamanho_leitura, n_tokens;
	try(tamanho_leitura = read(args->fd_con, resposta, sizeof(resposta)), "read");
	json_parser json;
	json_value hash;
	json.start = resposta;
	json.size = sizeof(resposta);
	
	json_init(&json);
	if (json_parse(&json) < 0) {
		printf("Falha JSON parse\n");
		pthread_exit(NULL);
	}
	if (json_get_str(&json, &hash, "hash") < 0) {
		printf("Chave \"hash\" não encontrada\n");
		pthread_exit(NULL);
	}
	if (hash.size != 64) {
		printf("Hash != 64 bytes!\n");
		pthread_exit(NULL);
	}
	
	
	// Dados para login:
	int nusp;
	
	// Calcula hash para usuário 0:
	senha_correta(0, mensagem, hash.value);
	
	pthread_cleanup_pop(1);
	return NULL;
}
