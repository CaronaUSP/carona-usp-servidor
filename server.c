/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
*******************************************************************************/

///@WARN: Apenas um esqueleto para iniciar o projeto.
///Thread inicial abre uma porta IPv4, espera coneções e cria uma thread por cliente.
///As threads fecham as coneções e liberam sua memória.
///Foi escrito apenas de base e possui muitos @TODO/@FIXME

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <openssl/sha.h>

#define DBG				fprintf(stderr, "Line %d\n", __LINE__)
#define error(msg)		do {perror(msg); exit(1);} while(0)
#define try(cmd,msg)	do {if ((cmd) == -1) {error(msg);}} while(0)
#define try0(cmd,msg)	do {if ((cmd) == NULL) {error(msg);}} while(0)

int s, total_con = 0, total_cli, total_caronas;
pthread_t thread_abandonada;

// argumentos que serão enviados às threads:
typedef struct {
	int fd_con;	// file descriptor da coneção
} args_thread;

static void sig_handler(int __attribute__((unused)) signo) {
		printf("I: Recebido SIGINT\n");
		close(s);
		///@TODO: atualizar .dados antes de fechar
		exit(0);
}

void* th_conecao_cliente(void *tmp) {
	args_thread *args = tmp;	// o argumento é um ponteiro para qqr área definida pelo programa,
							// então precisamos marcar o tipo de ponteiro recebido ou usar casts
	printf("Thread criada, fd = %d\n", args->fd_con);
	close(args->fd_con);
	free(args);
	return NULL;
}

inline unsigned char *sha256(const void *frase) {
	 return SHA256(frase, strlen(frase), NULL);	// recebendo um ponteiro nulo como destino,
												// o hash é salvo em um vetor estático e sobrescrito
												// cada vez que a função é chamada
}

int main (int argc, char **argv) {
	if (argc != 2) {	// o programa espera um argumento (porta TCP para abrir)
		fprintf(stderr, "Uso: %s porta\n", argv[0]);
		exit(1);
	}
	
	FILE *dados;	// leitura de dados de inicialização do servidor
	try0(dados = fopen(".dados", "r"), "fopen");		// leitura + escrita do início do arquivo
	if (fscanf(dados, "%d%d", &total_cli, &total_caronas) < 2) {
		printf("Falha na leitura de .dados\n");
		exit(1);
	}
	
	uint16_t porta;
	if (sscanf(argv[1], "%hu", &porta) == 0) {		///@FIXME: retorna caracteres válidos e ignora restante (ex: 77z => 77)
		fprintf(stderr, "Porta mal formatada\n");
		exit(1);
	}
	// a mensagem pode ser usada para negociar hashes com o cliente para autenticação
	printf("Porta %hu, %d clientes já conectados, %d caronas dadas\n", porta, total_cli, total_caronas);
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
	
	struct sigaction sinal;
	memset((char *) &sinal, 0, sizeof(sinal));
	sinal.sa_handler = sig_handler;
	try(sigaction(SIGINT, &sinal, NULL), "sigaction");
	
	for (;;) {
		///@TODO: aceitar uma coneção por IP (evitar DoS) e limitar tentativas de login,
		///checar timeouts/trajetos cíclicos/outros modos de quebrar o servidor
		args_thread *argumentos = malloc(sizeof(args_thread));
		socklen_t clilen = sizeof(struct sockaddr_in);	// tamanho da estrutura de dados do cliente
		try(argumentos->fd_con = accept(s, (struct sockaddr *) &endereco_cliente, &clilen), "accept");
		///@TODO: salvar endereço do cliente para cada thread (pelo menos para ter um log)
		total_con++;
		char ip[16];	///@WARN: IPv6 usa mais que 15 caracteres! Editar se for usá-lo!
		try0(inet_ntop(AF_INET, &endereco_cliente.sin_addr, ip, sizeof(ip) - 1), "inet_ntop");
		///@FIXME: a thread é criada e abandonada (thread_abandonada é um nome sugestivo?)
		printf("%s conectado (fd = %d) total %d\n", ip, argumentos->fd_con, total_con);
		pthread_create(&thread_abandonada, NULL, th_conecao_cliente, argumentos);	// free() de argumentos será pela thread
	}
}

