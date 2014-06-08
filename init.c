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

const char banco_de_dados[] = "database";

// Se recebermos SIGINT, paramos o programa fechando as conexões.
static void sig_handler(int __attribute__((unused)) signo) {
		printf("I: Recebido SIGINT\n");
		close(s);
		///@TODO: limpeza decente dos recursos utilizados
		FILE *dados = fopen(".dados", "w");
		if (dados != NULL) {
			if (fprintf(dados, "%d %d", clientes_total, caronas_total) < 0)
				fprintf(stderr, "Falha na escrita de .dados\n");
		} else
			fprintf(stderr, "Falha na escrita de .dados\n");
		
		fclose(dados);
		
		save_db(banco_de_dados);
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
	
	// pilha com threads livres
	int i;
	for (i = 0; i < MAX_CLIENTES; i++) {
		pilha_threads_livres[i] = i;
	}
	
	
	// Cria TSD (thread specific area), regiões alocadas para cada thread para
	// guardar variáveis da thread (se usássemos globais, haveria conflito entre
	// as threads)
	pthread_key_create(&dados_thread, NULL);
	CURLcode ret;
	ret = curl_global_init(CURL_GLOBAL_ALL);
	if (ret != 0) {
		fprintf(stderr, "curl_global_init(): %d\n", ret);
		exit(1);
	}
	
	if (init_db(banco_de_dados) == -1) {
		fprintf(stderr, "Falha na inicialização do banco de dados\n");
		exit(1);
	}
	
	usuario_t *user = primeiro_usuario;
	printf("Usuários:\n");
	while (user != NULL) {
		printf("%s - %s\n", user->email, user->hash);
		user = user->next;
	}
	
	inicializa_fila();
	
}
