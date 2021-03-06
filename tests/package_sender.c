#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>

#define CONNECTION_WAIT		200 * 1000		// Após fim da entrada de um arquivo, quanto tempo (ms)
																	// esperar por uma resposta do servidor?

#define error(msg)		do {perror(msg); exit(1);} while(0)
#define try(cmd, msg) 	do {if ((cmd) == -1) {close(connection); error(msg);}} while(0)
#define HELPER		\
"Uso: %s ip porta\n"\
"Programa para envio e visualização de pacotes ASCII TCP/IP\n"\
"Para enviar um pacote, digite os dados e aperte Enter\n"\
"Pacotes são enviados com caractere nulo no fim\n"\
"Pacotes enviados são mostrados na saída padrão com >>> e recebidos com <<<\n"\
"Se a entrada padrão terminar, o programa espera %d ms por uma resposta do\n"\
"servidor antes de fechar a conexão.\n", argv[0], CONNECTION_WAIT



int main(int argc, char **argv) {
	char buf[4000] = {0};
	int connection;	//socket
		
	if (argc != 3) {	// o programa espera um argumento (porta TCP para abrir)
		fprintf(stderr, HELPER);
		exit(1);
	}
	
	struct addrinfo hints, *res, *try;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	int ret;
	if ((ret = getaddrinfo(argv[1], argv[2], &hints, &res))) {
		fprintf(stderr, "%s: %s\n", "getaddrinfo", gai_strerror(ret));
		exit(1);
	}
	
	for (try = res; try != NULL; try = try->ai_next) {
		if ((connection = socket(try->ai_family, try->ai_socktype, try->ai_protocol)) != -1) {
			printf("Conectando...\n");
			if (connect(connection, try->ai_addr, try->ai_addrlen) == 0) break;
			perror("connect");
			close(connection);
		}
	}
	
	if (try == NULL) {
		fprintf(stderr, "Não foi possível conectar-se\n");
		exit(1);
	}
	
	freeaddrinfo(res);
	
	struct pollfd fds[2];
	
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = connection;
	fds[1].events = POLLIN;
	fds[1].revents = 0;
	
	for (;;) {
		
		
		try(poll(fds, 2, -1), "poll");
		
		if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			break;
		}
		
		// Linux gera POLLIN e retorna 0 no read() se a conexão for fechada,
		// mas checamos aqui também (não sei se esses códigos são gerados em alguma
		// ocasião, mas na dúvida...)
		if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
			printf("Conexão fechada\n");
			return 0;
		}
		
		if (fds[0].revents & POLLIN) {
			if (fgets(buf, sizeof(buf), stdin) == NULL)
				break;
			printf(">>>%s\n", buf);
			try(write(connection, buf, strlen(buf) + 1), "write");
			buf[0] = 0;
		}
		
		if (fds[1].revents & POLLIN) {
			int size;
			try(size = read(connection, buf, sizeof(buf) - 1), "read");
			if (size == 0) {
				printf("Conexão fechada\n");
				return 0;
			}
			buf[size] = 0;
			printf("<<<%s\n", buf);
		}
	}
	
	///Código fechava o programa ao chegar no fim da leitura da entrada
	///padrão. Podem haver mais respostas do servidor úteis ao usuário.
	///O ideal é entrar em um loop e fazer poll() da conexão com timeout
	///de um segundo e fechar apenas quando se passar um segundo sem resposta
	///ou a conexão for fechada.
	
	printf("Fim da leitura, aguardando respostas do servidor...\n(Ctrl+C para cancelar)\n");
	
	int poll_ret;
	while (((poll_ret = poll(&fds[1], 1, CONNECTION_WAIT)) == 1) && (fds[1].revents & POLLIN)) {	// poll() com connection
		int size;
		try(size = read(connection, buf, sizeof(buf) - 1), "read");
		if (size == 0) {
			printf("Conexão fechada\n");
			return 0;
		}
		buf[size] = 0;
		printf("<<<%s\n", buf);
	}
	return 0;
}
