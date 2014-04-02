/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
*******************************************************************************/

#include "server.h"

int s, clientes_agora = 0, clientes_total, caronas_total;
int pilha_threads_livres[MAX_CLIENTES];
int desligando = 0;
pthread_t threads[MAX_CLIENTES];
pthread_mutex_t mutex_modifica_thread = PTHREAD_MUTEX_INITIALIZER;

// argumentos que serão enviados às threads:
typedef struct {
	int fd_con;		// file descriptor da conexão
	int n_thread;	// número da thread
} args_thread;

// Se recebermos SIGINT, paramos o programa fechando as conexões.
// O código está com um loop lento e precisa ser arrumado (usar variável "desligando"
// para evitar deadlock não é uma boa ideia)
static void sig_handler(int __attribute__((unused)) signo) {
		printf("I: Recebido SIGINT\n");
		///@TODO: otimizar loop (i * j iterações) e controlar melhor deadlocks (usar variável "desligando" pode ser inseguro)
		// fecha todas as conexões
		desligando = 1;
		pthread_mutex_lock(&mutex_modifica_thread);
		int i;
		for (i = 0; i < MAX_CLIENTES; i++) {
			int j;
			for (j = clientes_agora; j < MAX_CLIENTES; j++)
				if (pilha_threads_livres[j] == i)
					break;		// para ao encontrar que thread não estava sendo usada
			if (pilha_threads_livres[j] != i) {	// se estava sendo usada
				printf("Fechando thread %d\n", i);
				pthread_cancel(threads[i]);
				pthread_join(threads[i], NULL);
			}
		}
		pthread_mutex_unlock(&mutex_modifica_thread);
		pthread_mutex_destroy(&mutex_modifica_thread);
		///@TODO: atualizar .dados antes de fechar
		close(s);
		exit(0);
}

// Limpeza de recursos ao terminar thread
void* th_limpeza(void *tmp) {
	args_thread *args = tmp;
	if (!desligando) {		// setada se estamos fechando o programa e precisamos apenas
							// liberar os recursos, sem adicionar a thread novamente à pilha
		pthread_mutex_lock(&mutex_modifica_thread);
		clientes_agora--;
		pilha_threads_livres[clientes_agora] = args->n_thread;
		pthread_mutex_unlock(&mutex_modifica_thread);
		printf("Thread %d: disconexão\n", args->n_thread);
	}
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

int main (int argc, char **argv) {
	if (argc != 2) {	// o programa espera um argumento (porta TCP para abrir)
		fprintf(stderr, "Uso: %s porta\n", argv[0]);
		exit(1);
	}
	
	FILE *dados;	// leitura de dados de inicialização do servidor
	try0(dados = fopen(".dados", "r"), "fopen");		// leitura do início do arquivo
	if (fscanf(dados, "%d%d", &clientes_total, &caronas_total) < 2) {
		printf("Falha na leitura de .dados\n");
		exit(1);
	}
	
	uint16_t porta;
	if (sscanf(argv[1], "%hu", &porta) == 0) {		///@FIXME: retorna caracteres válidos e ignora restante (ex: 77z => 77)
		fprintf(stderr, "Porta mal formatada\n");
		exit(1);
	}
	
	// pilha com threads livres
	int i;
	for (i = 0; i < MAX_CLIENTES; i++) {
		pilha_threads_livres[i] = i;
	}
	
	try(s = socket(AF_INET, SOCK_STREAM, 0), "socket");	// tenta criar socket
	
	struct sockaddr_in endereco_serv, endereco_cliente;
	memset((char *) &endereco_serv, 0, sizeof(endereco_serv));
	endereco_serv.sin_family = AF_INET;	//IPv4
	endereco_serv.sin_addr.s_addr = INADDR_ANY;
	endereco_serv.sin_port = htons(porta);
	if (bind(s, (struct sockaddr *) &endereco_serv, sizeof(endereco_serv)) == -1)	// abre porta
		error("bind");
	
	if (listen(s, 1) == -1)		// ouve coneções
		error("listen");
	
	// se recebermos SIGINT, fecharemos o servidor
	struct sigaction sinal;
	memset((char *) &sinal, 0, sizeof(sinal));
	sinal.sa_handler = sig_handler;
	try(sigaction(SIGINT, &sinal, NULL), "sigaction");
	
	// mutex para atualização de dados comuns às threads
	pthread_mutex_init(&mutex_modifica_thread, NULL);
	
	for (;;) {
		///@TODO: aceitar uma coneção por IP (evitar DoS) e limitar tentativas de login,
		///checar timeouts/trajetos cíclicos/outros modos de quebrar o servidor
		// argumentos que serão enviados às threads
		args_thread *argumentos = malloc(sizeof(args_thread));
		socklen_t clilen = sizeof(struct sockaddr_in);	// tamanho da estrutura de dados do cliente
		for (;;) {
			// aceita conexão e salva fd nos argumentos para a thread
			try(argumentos->fd_con = accept(s, (struct sockaddr *) &endereco_cliente, &clilen), "accept");
			if (clientes_agora == MAX_CLIENTES) {
				write(argumentos->fd_con, MSG_LIMITE, sizeof(MSG_LIMITE));
				close(argumentos->fd_con);
				continue;
			}
			break;
		}
		pthread_mutex_lock(&mutex_modifica_thread);
		// retira uma thread da pilha
		argumentos->n_thread = clientes_agora;
		clientes_agora++;
		clientes_total++;
		pthread_create(&threads[pilha_threads_livres[clientes_agora]], NULL, th_conecao_cliente, argumentos);	// free() de argumentos será pela thread
		pthread_mutex_unlock(&mutex_modifica_thread);
		///@TODO: salvar endereço do cliente para cada thread (pelo menos para ter um log)
		char ip[16];	///@WARN: IPv6 usa mais que 16 caracteres! Editar se for usá-lo!
		try0(inet_ntop(AF_INET, &endereco_cliente.sin_addr, ip, sizeof(ip) - 1), "inet_ntop");
		printf("%s conectado (fd = %d) total %d\n", ip, argumentos->fd_con, clientes_agora);
	}
}
