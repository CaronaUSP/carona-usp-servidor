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
	
	// Argumentos que serão enviados às threads
	args_thread *argumentos = malloc(sizeof(args_thread));;
	for (;;) {
		///@TODO: limitar tentativas de login,
		///checar timeouts/trajetos cíclicos/outros modos de quebrar o servidor
		struct sockaddr_in endereco_cliente;
		for (;;) {
			socklen_t clilen = sizeof(struct sockaddr_in);
			// Aceita conexão e salva fd nos argumentos para a thread
			try(argumentos->fd_con = accept(s, (struct sockaddr *) &endereco_cliente, &clilen), "accept");
			if (clientes_agora < MAX_CLIENTES)
				break;
			// Se atingiu limite:
			write(argumentos->fd_con, MSG_LIMITE, sizeof(MSG_LIMITE));
			close(argumentos->fd_con);
		}
		///@TODO: salvar endereço do cliente para cada thread (pelo menos para ter um log)
		char ip[16];	///@WARN: IPv6 usa mais que 16 caracteres! Editar se for utilizá-lo!
		try0(inet_ntop(AF_INET, &endereco_cliente.sin_addr, ip, sizeof(ip) - 1), "inet_ntop");
		printf("%s conectado (fd = %d), total %d\n", ip, argumentos->fd_con, clientes_agora + 1);
		if (ja_conectado(&endereco_cliente.sin_addr)) {
			printf("Já estava conectado, recusando\n");
			close(argumentos->fd_con);
			continue;
		}
		// Cria thread:
		argumentos->n_thread = clientes_agora;
		aceita_conexao(argumentos, &endereco_cliente.sin_addr);
		// Aloca espaço para novos argumentos
		argumentos = malloc(sizeof(args_thread));
	}
}
