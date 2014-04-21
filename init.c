/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Funções de inicialização/finalização
*******************************************************************************/

#include "init.h"

// Se recebermos SIGINT, paramos o programa fechando as conexões.
static void sig_handler(int __attribute__((unused)) signo) {
		printf("I: Recebido SIGINT\n");
		pthread_mutex_destroy(&mutex_modifica_thread);
		///@TODO: atualizar .dados antes de fechar
		close(s);
		exit(0);
}

/*
// A limpeza é feita em th_limpeza (conexao.c)
static void thread_key_destroy(void *buf) {
	free(buf);
}
*/

void inicializa(int argc, char **argv) {
	
	if (argc != 2) {	// o programa espera um argumento (porta TCP para abrir)
		fprintf(stderr, "Uso: %s porta\n", argv[0]);
		exit(1);
	}
	
	FILE *dados;	// leitura de dados de inicialização do servidor
	const char erro_leitura[] = "Falha na leitura de .dados\n";
	if ((dados = fopen(".dados", "r")) != NULL) {
		if (fscanf(dados, "%d%d", &clientes_total, &caronas_total) != 2) {
			fprintf(stderr, erro_leitura);
		}
		tryEOF(fclose(dados), "fclose");
	} else {
		fprintf(stderr, erro_leitura);
	}
	
	uint16_t porta;
	if (sscanf(argv[1], "%hu", &porta) != 1) {		///@FIXME: retorna caracteres válidos e ignora restante (ex: 77z => 77)
		fprintf(stderr, "Porta mal formatada\n");
		exit(1);
	}
	
	try(s = socket(AF_INET, SOCK_STREAM, 0), "socket");	// tenta criar socket
	
	struct sockaddr_in endereco_serv;
	memset((char *) &endereco_serv, 0, sizeof(endereco_serv));
	endereco_serv.sin_family = AF_INET;	//IPv4
	endereco_serv.sin_addr.s_addr = INADDR_ANY;
	endereco_serv.sin_port = htons(porta);
	
	try(bind(s, (struct sockaddr *) &endereco_serv, sizeof(endereco_serv)), "bind");	// abre porta
	try(listen(s, 1), "listen");		// ouve coneções
	
	// se recebermos SIGINT, fecharemos o servidor
	struct sigaction sinal;
	memset((char *) &sinal, 0, sizeof(sinal));
	sinal.sa_handler = sig_handler;
	try(sigaction(SIGINT, &sinal, NULL), "sigaction");
	
	// mutex para atualização de dados comuns às threads
	pthread_mutex_init(&mutex_modifica_thread, NULL);
	
	pthread_cond_init(&comunica_thread, NULL);
	
	// Cria TSD (thread specific area), regiões alocadas para cada thread para
	// guardar variáveis da thread (se usássemos globais, haveria conflito entre
	// as threads)
	pthread_key_create(&dados_thread, NULL);
	
}
