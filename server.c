/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
*******************************************************************************/

#include "server.h"

int main (int argc, char **argv) {
	
	inicializa(argc, argv);	// Inicialização e abertura de portas
	
	tsd_t *argumentos;
	for (;;) {
		///@TODO: checar timeouts/trajetos cíclicos/outros modos de quebrar o servidor
		struct sockaddr_in endereco_cliente;
		// Aloca espaço para novos argumentos
		try0(argumentos = malloc(sizeof(tsd_t)), "malloc");
		argumentos->errmsg = NULL;		// mensagem de erro inicial = nenhuma
		for (;;) {
			socklen_t clilen = sizeof(struct sockaddr_in);
			// Aceita conexão e salva fd nos argumentos para a thread
			try(argumentos->fd_con = accept(s, (struct sockaddr *) &endereco_cliente, &clilen), "accept");
			if (clientes_agora < MAX_CLIENTES)
				break;
			// Se atingiu limite:
			write(argumentos->fd_con, "{\"msg\":\"Número máximo de clientes atingido\",\"fim\"}", sizeof("{\"msg\":\"Número máximo de clientes atingido\",\"fim\"}"));
			close(argumentos->fd_con);
		}
		char ip[64];
		try0(inet_ntop(AF_INET, &endereco_cliente.sin_addr, ip, sizeof(ip) - 1), "inet_ntop");
		printf("%s conectado (fd = %d), total %d\n", ip, argumentos->fd_con, clientes_agora + 1);
		#ifndef NAO_CHECA_JA_CONECTADO
		if (ja_conectado(&endereco_cliente.sin_addr)) {
			printf("Já estava conectado, recusando\n");
			close(argumentos->fd_con);
			continue;
		}
		#endif
		// Cria thread:
		aceita_conexao(argumentos, &endereco_cliente.sin_addr);
	}
}