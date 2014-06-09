/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Funções para o banco de dados de usuários
*******************************************************************************/

#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define error(msg)		do {perror(msg); return -1;} while(0)
#define tryEOF(cmd,msg)	do {if ((cmd) == EOF) {error(msg);}} while(0)
#define try(cmd,msg)	do {if ((cmd) == -1) {error(msg);}} while(0)
#define try0(cmd,msg)	do {if ((cmd) == NULL) {error(msg);}} while(0)

usuario_t *primeiro_usuario = NULL, *ultimo_usuario;

///@TODO: usa lista linkada de usuários, que pode ser um pouco lenta
/// para bancos de dados muito grandes... Pode ser melhorado (mas não deve ser
/// problema aqui)

int init_db(const char *arquivo) {
	FILE *db = fopen(arquivo, "r");
	char *db_data;
	if (db != NULL) {
		
		try(fseek(db, 0, SEEK_END), "seek");
		
		long db_size = ftell(db);
		try(db_size, "ftell");
		if (db_size == 0)
			return 0;
		try0(db_data = malloc((size_t) db_size + 1), "malloc");	// colocar caractere nulo no fim
		
		db_data[db_size] = 0;
		
		rewind(db);
		if (fread(db_data, 1, db_size, db) != (size_t) db_size) {
			///@TODO: poderia ter código para tentar novamente...
			fprintf(stderr, "Falha na leitura do banco de dados\n");
			return -1;
		}
		
		int user_n;
		char *search = db_data;
		for (user_n = 1; (search = strchr(search + 1, '\n')); user_n++) *search = 0;
		
		user_n /= 2;
		printf("%d usuários cadastrados\n", user_n);
		
		primeiro_usuario = malloc(user_n * sizeof(usuario_t));
		try0(primeiro_usuario, "malloc");
		
		int i;
		search = db_data;
		for (i = 0; i < user_n - 1; i++) {
			primeiro_usuario[i].email = search;
			search = rawmemchr(search + 1, 0) + 1;
			primeiro_usuario[i].hash = search;
			search = rawmemchr(search + 1, 0) + 1;
			primeiro_usuario[i].next = &primeiro_usuario[i + 1];
		}
		primeiro_usuario[i].email = search;
		search = rawmemchr(search + 1, 0) + 1;
		primeiro_usuario[i].hash = search;
		primeiro_usuario[i].next = NULL;	// marca última entrada
		ultimo_usuario = &primeiro_usuario[i];
		
		return 0;
	}
	fprintf(stderr, "Falha na leitura do banco de dados\n");
	return -1;
}

static usuario_t *get_user_struct(const char *email) {
	usuario_t *prox = primeiro_usuario;
	while (prox != NULL){
		if (!strcmp(prox->email, email))
			return prox;
		prox = prox->next;
	}
	return NULL;
}

int add_user(const char *email, const char *hash) {
	usuario_t *novo;
	
	if ((novo = get_user_struct(email)) != NULL) {		// já existe
		printf("Usuário %s já existe, atualizando senha\n", email);
		///@TODO: free() da estrutura se possível
		strncpy((char *) novo->hash, hash, 64);
		return 0;
	}
	
	novo = malloc(sizeof(usuario_t) + strlen(email) + 1);
	if (novo == NULL)
		return -1;
	if (ultimo_usuario != NULL) {	// primeiro do servidor
		ultimo_usuario->next = novo;
		ultimo_usuario = novo;
	} else {
		primeiro_usuario = novo;
		ultimo_usuario = novo;
	}
	
	char *copia_email = (char *) (novo + 1);
	char *copia_hash = malloc(strlen(hash) + 1);	///@TODO: alocar tudo de uma vez
	strcpy(copia_email, email);
	strcpy(copia_hash, hash);
	novo->email = copia_email;
	novo->hash = copia_hash;
	novo->next = NULL;
	return 0;
}

const char *get_user(const char *email) {
	usuario_t *prox;
	prox = get_user_struct(email);
	if (prox == NULL)
		return NULL;
	return prox->hash;
}

void save_db(const char *arquivo) {
	FILE *db = fopen(arquivo, "w");
	usuario_t *prox = primeiro_usuario;
	printf("Salvando:\n");
	while (prox != NULL) {
		fprintf(db, "%s\n%s\n", prox->email, prox->hash);
		printf("%s -> %s\n", prox->email, prox->hash);
		prox = prox->next;
	}
}
