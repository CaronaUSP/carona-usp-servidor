#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>

#define error(msg)		do {perror(msg); exit(1);} while(0)
#define try(cmd, msg) 	do {if ((cmd) == -1) {error(msg);}} while(0)

char buf[2000] = {0};
int s, connection;	//socket

int main(int argc, char **argv) {
	
	if (argc != 3) {	// o programa espera um argumento (porta TCP para abrir)
		fprintf(stderr, "Uso: %s ip porta\n", argv[0]);
		exit(1);
	}
	
	try(s = socket(AF_INET, SOCK_STREAM, 0), "socket");	// tenta criar socket
	
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
		printf("Falha na conexão\n");
		exit(1);
	}
	
	freeaddrinfo(res);
	
	struct pollfd fds[2];
	
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[1].fd = connection;
	fds[1].events = POLLIN;
	
	for (;;) {
		char resposta[1000];
		
		try(poll(fds, 2, -1), "poll");
		
		if (fds[0].revents & POLL_IN) {
			if (fgets(buf, sizeof(buf), stdin) == NULL)
				error("fgets");
			try(write(connection, buf, strlen(buf) + 1), "write");
		}
		
		if (fds[1].revents & POLL_IN) {
			int size;
			try(size = read(connection, resposta, sizeof(resposta) - 1), "read");
			resposta[size] = 0;
			printf("%s", resposta);
		}
	}
}
