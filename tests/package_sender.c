/*
 * Em desenvolvimento!
 */
#include <stdio.h>

// Se recebermos SIGINT, enviamos um novo pacote:
static void sig_handler(int __attribute__((unused)) signo) {
		printf("I: Recebido SIGINT\n");
		
}


int main(int argc, char **argv) {
	
	if (argc != 2) {	// o programa espera um argumento (porta TCP para abrir)
		fprintf(stderr, "Uso: %s porta\n", argv[0]);
		exit(1);
	}
	
	uint16_t porta;
	if (sscanf(argv[1], "%hu", &porta) != 1) {		///@FIXME: retorna caracteres válidos e ignora restante (ex: 77z => 77)
		fprintf(stderr, "Porta mal formatada\n");
		exit(1);
	}
	
	try(s = socket(AF_INET, SOCK_STREAM, 0), "socket");	// tenta criar socket
	
	struct addrinfo hints, *res, *try;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	int ret;
	if ((ret = getaddrinfo(argv[1], argv[2], &hints, &res))) gai_err("getaddrinfo", ret);
	for (try = res; try != NULL; try = try->ai_next) {
		if ((connection = socket(try->ai_family, try->ai_socktype, try->ai_protocol)) != -1) {
			printf("Connecting...\n");
			if (connect(connection, try->ai_addr, try->ai_addrlen) == 0) break;
			perror("connect");
			close(connection);	//can't connect, close socket
		}
	}
	if (try == NULL) {
		printf("Can't connect!\n");
		exit(1);
	}
	freeaddrinfo(res);
	
	
	// se recebermos SIGINT, enviamos novo pacote
	struct sigaction sinal;
	memset((char *) &sinal, 0, sizeof(sinal));
	sinal.sa_handler = sig_handler;
	try(sigaction(SIGINT, &sinal, NULL), "sigaction");
	
	
	
	
	
}
