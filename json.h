/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Biblioteca para análise de pacotes JSON
*******************************************************************************/

#include <stddef.h>
#include <string.h>
#define JSON_OK			0
#define JSON_INVALID	-1
#define JSON_INCOMPLETE	-2

typedef struct {
	char *start, *cur, *cur_end;
	size_t size;
} json_parser;

typedef struct {
	char *value;
	size_t size;
} json_value;

void json_init(json_parser *json);
int json_parse(json_parser *json);
int json_get_str(json_parser *json, json_value *ret, const char *key);
