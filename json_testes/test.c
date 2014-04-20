/* Use gcc -Wall -Wextra json2.h json2.c testes_json2.c -D_GNU_SOURCE -g para compilar.
 * Para testar: digite um objeto JSON válido (apenas Strings são suportadas por enquanto), como:
 { "nome" : "Carona Comunitária USP", "categoria" : "Projeto de Graduação", "disciplina" : "PCS-3100"}
 * e depois digite o nome de um par para procurar seu valor
 */

#include <stdio.h>
#include "json.h"

int main () {
	char str[200];
	json_parser json;
	json_pair pairs[200];
	json.start = str;
	json.pairs = pairs;
	
	fgets(str, 200, stdin);
	printf("Scanning %s\n", str);
	json_init(&json);

	if (json_all_parse(&json) < 0)
		return 1;
	
	
	for (;;) {
		scanf("%s", str);
		char *match;
		if ((match = json_get_str(&json, str)) == NULL)
			printf("Não encontrado\n");
		else
			printf("%s\n", match);
	}
}
