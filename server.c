/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
*******************************************************************************/

#include "server.h"

int main (int argc, char **argv) {
	
	int fd_con;
	
	inicializa(argc, argv);	// Inicialização e abertura de portas
	
	for (;;) {
		///@TODO: checar timeouts/trajetos cíclicos/outros modos de quebrar o servidor
		struct sockaddr_in endereco_cliente;
		for (;;) {
			socklen_t clilen = sizeof(struct sockaddr_in);
			
			fd_con = accept(s, (struct sockaddr *) &endereco_cliente, &clilen);
			if (fd_con == -1) {
				perror("accept");
				continue;
			}
			if (clientes_agora < MAX_CLIENTES)
				break;
			// Se atingiu limite:
			write(fd_con, "{\"msg\":\"Número máximo de clientes atingido\",\"fim\":null}", sizeof("{\"msg\":\"Número máximo de clientes atingido\",\"fim\":null}"));
			close(fd_con);
		}
		char ip[64];
		if (inet_ntop(AF_INET, &endereco_cliente.sin_addr, ip, sizeof(ip) - 1) == NULL) {
			perror("inet_ntop");
			close(fd_con);
		} else {
			printf("%s conectado (fd = %d), total %d\n", ip, fd_con, clientes_agora + 1);
			#ifndef NAO_CHECA_JA_CONECTADO
			if (ja_conectado(&endereco_cliente.sin_addr)) {
				printf("Já estava conectado, recusando\n");
				close(fd_con);
				continue;
			}
			aceita_conexao(fd_con, &endereco_cliente.sin_addr);
			#else
			aceita_conexao(fd_con);
			#endif
		}
	}
}
