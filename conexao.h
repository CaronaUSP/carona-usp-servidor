/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Tratamento de conexões e threads
*******************************************************************************/

int s, clientes_agora = 0, clientes_total = 0, caronas_total = 0;
uint32_t conectados[MAX_CLIENTES] = {0};	// lista de IPs já conectados
											///@WARN: 32 bits para IPv4 apenas
pthread_mutex_t mutex_modifica_thread = PTHREAD_MUTEX_INITIALIZER;
// argumentos que serão enviados às threads:
typedef struct {
	int fd_con;		// file descriptor da conexão
	int n_thread;	// número da thread
} args_thread;


int ja_conectado(const struct in_addr *ip) {
	///@TODO: pode ser otimizado para buscar apenas conexões existentes na array?
	int i;
	for (i = 0; i < MAX_CLIENTES; i++) {
		if (conectados[i] == *(uint32_t *)ip) {
			return 1;
		}
	}
	return 0;
}

void adiciona_conexao(const struct in_addr *ip) {
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

void* th_conecao_cliente(void *tmp) {
	args_thread *args = tmp;	// o argumento é um ponteiro para qqr área definida pelo programa,
								// então precisamos marcar o tipo de ponteiro recebido ou usar casts
	pthread_cleanup_push((void *)th_limpeza, tmp);
	printf("Thread criada, fd = %d\n", args->fd_con);
	char mensagem[200] = MSG_INICIAL;	///@FIXME: assume que dados iniciais cabem em 200 bytes
	sprintf(mensagem + sizeof(MSG_INICIAL) - 1, "%s\n%d clientes já conectados, %d atualmente, %d caronas dadas",
				MSG_NOVIDADES, clientes_total, clientes_agora, caronas_total);
	write(args->fd_con, mensagem, strlen(mensagem) + 1);
	/*
	 * O servidor recebe uma assinatura de 4 bytes (que é sempre a mesma) dos
	 * clientes para provar que é nosso aplicativo que está conectado, versão do cliente,
	 * o número USP e o hash
	 */
	char credenciais[4 + 4 + 4 + SHA_256_DIGEST_LENGTH];
	read(args->fd_con, credenciais, sizeof(credenciais));
	uint32_t assinatura = *((uint32_t *) credenciais), numero_usp = *((uint32_t *) credenciais + 1), versao = *((uint32_t *) credenciais + 2);
	char *hash_recebido = &credenciais[12];
	if (assinatura == SEQ_CLIENTE) {
		if (numero_usp < NUMERO_TOTAL_USUARIOS) {
			if (senha_correta(numero_usp, mensagem, hash_recebido)) {
				write(args->fd_con, "Aceito", 6);
				printf("%d: autenticado\n", args->n_thread);
			}
			else printf("%d: senha errada\n", args->n_thread);
		} else
			printf("%d: usuário %d inexistente\n", args->n_thread, numero_usp);
	} else
		printf("%d: assinatura errada\n", args->n_thread);
	pthread_cleanup_pop(1);
	return NULL;
}

void cria_thread(args_thread *args) {
	pthread_t thread;
	pthread_create(&thread, NULL, th_conecao_cliente, args);	// free() de argumentos será pela thread
	pthread_detach(thread);			// não receber retorno e liberar recursos ao final da execução
}
