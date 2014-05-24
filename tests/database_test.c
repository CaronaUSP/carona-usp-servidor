/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Código para teste do banco de dados
*******************************************************************************/

#include "database.h"
#include <stdio.h>
#include <string.h>

#define HELP "Uso:\n%s arquivo add usuário hash\n%s arquivo get usuário\n%s arquivo list\n", argv[0], argv[0], argv[0]

int main(int argc, char **argv) {
	
	if (argc >= 3) {
		
		if (init_db(argv[1]) == -1) {
			printf("Falha na inicialização\n");
			return -1;
		}
		
		if (!strcmp(argv[2], "list")) {
			if (primeiro_usuario == NULL) {
				printf("Banco de dados vazio\n");
				return 0;
			}
			usuario_t *user = primeiro_usuario;
			printf("Usuários:\n");
			while (user != NULL) {
				printf("%s - %s\n", user->email, user->hash);
				user = user->next;
			}
			return 0;
		}
		
		if (argc >= 4) {
			if (!strcmp(argv[2], "get")) {
				const char *hash = get_user(argv[3]);
				if (hash == NULL) {
					printf("Usuário não encontrado\n");
					return 1;
				}
				printf("%s\n", hash);
				return 0;
			}
		}
		
		if (argc >= 5) {
			if (!strcmp(argv[2], "add")) {
				add_user(argv[3], argv[4]);
				save_db(argv[1]);
				return 0;
			}
		}
		
	}
	
	printf(HELP);
	return -1;
}
