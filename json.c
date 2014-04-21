/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Biblioteca para análise de pacotes JSON
*******************************************************************************/

#include "json.h"

static int cmp_json_pair(const void *p1, const void *p2) {	// ponteiros para json_pair
	return strcmp( ( (json_pair const *) p1)->name, ( (json_pair const *) p2)->name);
}

static int search_object(json_parser *json) {
	for (; *json->cur; json->cur++) {
		switch (*json->cur) {
			case '{':
				json->cur++;
				return 0;
			case '\t':
			case '\r':
			case '\n':
			case ' ':
				break;
			default:
				return JSON_INVALID;
		}
	}
	return JSON_INCOMPLETE;
}

static int is_empty(json_parser *json) {
	for (; *json->cur; json->cur++) {
		switch (*json->cur) {
			case '}':
				return 1;
			case '\t':
			case '\r':
			case '\n':
			case ' ':
				break;
			default:
				return 0;
		}
	}
	return JSON_INCOMPLETE;
}

static char *json_getstr(json_parser *json) {
	char *end, *p;
	for (; *(json->cur); json->cur++) {
		switch (*json->cur) {
			case '\"':
				if ((end = strchr(json->cur + 1, '\"')) == NULL)
					return NULL;
				if ((p = malloc(end - json->cur)) == NULL) {
					perror("malloc");
					return NULL;
				}
				memcpy(p, json->cur + 1, end - json->cur - 1);
				p[end - json->cur - 1] = 0;
				json->cur = end + 1;
				return p;
			case '\t':
			case '\r':
			case '\n':
			case ' ':
				break;
			default:
				return NULL;
		}
	}
	return NULL;
}


static int json_next(json_parser *json) {	//1 = há mais dados antes do fim do objeto
	for (; *json->cur; json->cur++) {
		switch (*json->cur) {
			case '}':
				return 0;
			case '\t':
			case '\r':
			case '\n':
			case ' ':
				break;
			case ',':
				json->cur++;
				return 1;
			default:
				return JSON_INVALID;
		}
	}
	return 1;
}

static int json_get_value_start(json_parser *json) {
	for (; *json->cur; json->cur++) {
		switch (*json->cur) {
			case ':':
				json->cur++;
				return JSON_OK;
			case '\t':
			case '\r':
			case '\n':
			case ' ':
				break;
			default:
				return JSON_INVALID;
		}
	}
	return JSON_INVALID;
}


static int json_get_value_int(json_parser *json) {
	int num;
	if (sscanf(json->cur, "%d", &num) == EOF) {
		perror("sscanf");
		return JSON_INVALID;	// isso precisa ser corrigido (um inteiro = -1 (mesmo valor de JSON_INVALID) não é diferenciado de erro)
	}
	
	for (; *json->cur; json->cur++) {
		switch (*json->cur) {
			case '}':
			case ',':
			case '\t':
			case '\r':
			case '\n':
			case ' ':
			case '0': case '1': case '2': case '3': case '4': case '5':
			case '6': case '7': case '8': case '9':
				return num;
			default:
				return JSON_INVALID;
		}
	}
	// Atingiu final do pacote sem achar fim do objeto:
	return JSON_INVALID;
}

static int json_get_type(json_parser *json) {
	json_get_value_start(json);
	for (; *json->cur; json->cur++) {
		switch (*json->cur) {
			case '\"':
				return JSON_STRING;
			case 't':
				if (strncmp(json->cur, "true", 4)) {
					json->cur += 4;
					return JSON_TRUE;
				}
				return JSON_INVALID;
			case 'f':
				if (strncmp(json->cur, "false", 5)) {
					json->cur += 5;
					return JSON_FALSE;
				}
				return JSON_INVALID;
			case 'n':
				if (strncmp(json->cur, "null", 4)) {
					json->cur += 4;
					return JSON_NULL;
				}
				return JSON_INVALID;
			case '{':
				return JSON_OBJECT;
			case '[':
				return JSON_ARRAY;
			case '0': case '1': case '2': case '3': case '4': case '5':
			case '6': case '7': case '8': case '9':
				return JSON_NUMBER;
			case '\t':
			case '\r':
			case '\n':
			case ' ':
				break;
			default:
				return JSON_INVALID;
		}
	}
	return JSON_INVALID;
}

static void bp() {}	//para depuração
#define try(cmd)	do {int __ret = (cmd); if (__ret < 0) return __ret;} while(0)
#define try0(cmd)	do {if ((cmd) == NULL) return JSON_INVALID;} while(0)
int json_all_parse(json_parser *json) {
	json->cur = json->start;
	try(search_object(json));
	printf("JSON start = %d\n", json->cur - json->start);
	
	int obj_n = 0;
	int has_ended = is_empty(json);
	if (has_ended == JSON_INCOMPLETE)
		return JSON_INCOMPLETE;
	
	///@TODO: checar limite de json->pairs
	
	if (!has_ended)
		do {
			char *str;
			try0(str = json_getstr(json));
			
			int type = json_get_type(json);
			if (type == JSON_INVALID)
				return JSON_INVALID;
			
			try(json_get_value_start);
			
			json->pairs[obj_n].name = str;
			json->pairs[obj_n].type = type;
			
			
			switch (type) {
				case JSON_STRING:
					try0(json->pairs[obj_n].value = json_getstr(json));
					break;
				case JSON_NUMBER:
					printf("Recebido número, assumindo como inteiro (float ainda não é suportado!)\n");
					try(((json_int *) json->pairs)[obj_n].value = json_get_value_int(json));
					break;
				default:
					// Arrays e objetos não são suportados. true, false e null
					// são, mas vou adicionar aqui mais tarde (quando formos utilizá-los)
					printf("JSON: Tipo não suportado %d encontrado\n", type);
			}
			
			obj_n++;
		} while (json_next(json) == 1);
	
	bp();
	qsort(json->pairs, obj_n, sizeof(json_pair), cmp_json_pair);
	
	printf("Sorted:\n");
	int i;
	for (i=0; i < obj_n; i++) {
		printf("\"%s\" = \t", json->pairs[i].name);
		if (json->pairs[i].type == JSON_STRING)
			printf("\"%s\"\n", ((json_str *) json->pairs)[i].value);
		else if (json->pairs[i].type == JSON_INT)
			printf("%d\n", ((json_int *) json->pairs)[i].value);
	}
	
	json->n_pairs = obj_n;
	return 0;
}

char *json_get_str(json_parser *json, const char *search) {
	json_pair search_pair, *result;
	search_pair.name = search;
	search_pair.type = JSON_STRING;
	search_pair.value = NULL;
	
	result = bsearch(&search_pair, json->pairs, json->n_pairs, sizeof(json_pair), cmp_json_pair);
	if (result == NULL)
		return NULL;
	
	return result->value;
}
