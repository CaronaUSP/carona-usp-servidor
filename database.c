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

usuario_t *primeiro_usuario = NULL, *ultimo_usuario;

///@TODO: usa lista linkada de usuários, que pode ser um pouco lenta
/// para bancos de dados muito grandes... Pode ser melhorado (mas não deve ser
/// problema aqui)

int init_db(const char *arquivo) {
	FILE *db = fopen(arquivo, "r");
	char email[258], hash[66];
	usuario_t *atual;
	if (db != NULL) {
		if (fgets(email, sizeof(email), db) == NULL ||
			fgets(hash, sizeof(hash), db) == NULL) {
			tryEOF(fclose(db), "fclose");
			return 0;
		}
		
		char *fim = strchr(email, '\n');
		if (fim != NULL)
			*fim = 0;
		
		fim = strchr(hash, '\n');
		if (fim != NULL)
			*fim = 0;
		
		size_t len_email = strlen(email) + 1;
		primeiro_usuario = malloc(sizeof(usuario_t) + len_email + strlen(hash) + 1);
		if (primeiro_usuario == NULL)
			return -1;
		char *buf_email = (char *) primeiro_usuario + sizeof(usuario_t),
				*buf_hash = buf_email + len_email;
		strcpy(buf_email, email);
		strcpy(buf_hash, hash);
		ultimo_usuario = primeiro_usuario;
		ultimo_usuario->email = buf_email;
		ultimo_usuario->hash = buf_hash;
		while (fgets(email, sizeof(email), db) != NULL &&
				fgets(hash, sizeof(hash), db) != NULL) {
			
			len_email = strlen(email) + 1;
			atual = malloc(sizeof(usuario_t) + len_email + strlen(hash) + 1);
			
			buf_email = (char *) atual + sizeof(usuario_t),
			buf_hash = buf_email + len_email;
			
			fim = strchr(email, '\n');
			if (fim != NULL)
				*fim = 0;
			
			fim = strchr(hash, '\n');
			if (fim != NULL)
				*fim = 0;
			strcpy(buf_email, email);
			strcpy(buf_hash, hash);
			
			ultimo_usuario->next = atual;
			atual->email = buf_email;
			atual->hash = buf_hash;
			ultimo_usuario = atual;
		}
		tryEOF(fclose(db), "fclose");
		ultimo_usuario->next = NULL;
		return 0;
	}
	fprintf(stderr, "Falha na leitura do banco de dados\n");
	return -1;
}

int add_user(const char *email, const char *hash) {
	///@TODO: se usuário já existe, substituir entrada antiga
	usuario_t *novo;
	novo = malloc(sizeof(usuario_t));
	if (novo == NULL)
		return -1;
	if (ultimo_usuario != NULL) {	// primeiro do servidor
		ultimo_usuario->next = novo;
		ultimo_usuario = novo;
	} else {
		primeiro_usuario = novo;
		ultimo_usuario = novo;
	}
	novo->email = email;
	novo->hash = hash;
	novo->next = NULL;
	return 0;
}

const char *get_user(const char *email) {
	usuario_t *prox = primeiro_usuario;
	while (prox != NULL){
		if (!strcmp(prox->email, email))
			return prox->hash;
		prox = prox->next;
	}
	return NULL;
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
