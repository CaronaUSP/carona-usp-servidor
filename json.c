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
				*end = 0;
				p = json->cur + 1;
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
				*json->cur = 0;	// Ver json_get_int()
				return 0;
			case '\t':
			case '\r':
			case '\n':
			case ' ':
				break;
			case ',':
				*json->cur = 0;	// Ver json_get_int()
				json->cur++;
				return 1;
			default:
				return JSON_INVALID;
		}
	}
	return JSON_INVALID;
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


static int json_parse_int(json_parser *json) {
	
	for (; *json->cur; json->cur++) {
		switch (*json->cur) {
			case '\t':
			case '\r':
			case '\n':
			case ' ':
				*json->cur = 0;	// Fim do número, coloca caractere nulo
				json->cur++;	// Próxima função começará acessos do próximo byte
			// '}' e ',' serão usadas por json_next() para encontrar o fim do objeto/item,
			// então não vamos colocá-los como 0 aqui:
			///@TODO: Isso é um pouco confuso (ler aqui, colocar o caractere nulo em outra função),
			/// melhor mudar isso alguma hora (mas funciona bem, então vou deixar assim por enquanto)
			case '}':
			case ',':
				return 0;		// Sucesso!
			case '0': case '1': case '2': case '3': case '4': case '5':
			case '6': case '7': case '8': case '9':
				break;
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
				if (!strncmp(json->cur, "true", 4)) {
					json->cur += 4;
					return JSON_TRUE;
				}
				return JSON_INVALID;
			case 'f':
				if (!strncmp(json->cur, "false", 5)) {
					json->cur += 5;
					return JSON_FALSE;
				}
				return JSON_INVALID;
			case 'n':
				if (!strncmp(json->cur, "null", 4)) {
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

/*
 * Busca fim de uma array
 */
int json_array_end(json_parser *json) {
	int n_aberturas = 0;	// número de colchetes ('[') que foram abertos
	
	for (; *json->cur; json->cur++) {
		switch (*json->cur) {
			case '[':
				n_aberturas++;
				break;
			case ']':
				n_aberturas--;
				if (n_aberturas == 0) {
					*json->cur = 0;
					json->cur++;
					return 0;
				}
		}
	}
	// Atingiu final do pacote sem achar fim da array:
	return JSON_INVALID;
	
}

void bp() {}	//para depuração
#define try(cmd)	do {int __ret = (cmd); if (__ret < 0) return __ret;} while(0)
#define try0(cmd)	do {if ((cmd) == NULL) return JSON_INVALID;} while(0)
int json_all_parse(json_parser *json) {
	json->cur = json->start;
	try(search_object(json));
	
	int obj_n = 0;
	int has_ended = is_empty(json);
	if (has_ended == JSON_INCOMPLETE)
		return JSON_INCOMPLETE;
	
	if (!has_ended) {
		int has_next;
		do {
			
			if (obj_n >= json->n_pairs)
				return JSON_NOMEM;	// Faltam entradas em json->pairs
			
			char *str;
			try0(str = json_getstr(json));
			
			int type;
			
			switch (type = json_get_type(json)) {
				// Seria melhor e mais organizado ter uma array para cada tipo
				case JSON_STRING:
					try0(json->pairs[obj_n].value = json_getstr(json));
					break;
				case JSON_NUMBER:
					printf("Recebido número, assumindo como inteiro (float ainda não é suportado!)\n");
					json->pairs[obj_n].value = json->cur;
					try(json_parse_int(json));
					break;
				case JSON_TRUE:
				case JSON_FALSE:
				case JSON_NULL:
					break;
				case JSON_OBJECT:
					///@TODO: suportar objetos (não é importante agora)
					printf("JSON: Objetos não suportados!\n");
					return JSON_INVALID;
				case JSON_ARRAY:
					json->pairs[obj_n].value = json->cur + 1;
					try(json_array_end(json));
					break;
				default:
					printf("JSON: json_get_type: tipo %d desconhecido\n", type);
			}
			
			json->pairs[obj_n].name = str;
			json->pairs[obj_n].type = type;
			
			obj_n++;
			has_next = json_next(json);
			if (has_next == JSON_INVALID)
				return JSON_INVALID;
		} while (has_next == 1 && obj_n < json->n_pairs);
	}
	
	bp();
	qsort(json->pairs, obj_n, sizeof(json_pair), cmp_json_pair);
	
	printf("Sorted:\n");
	int i;
	for (i=0; i < obj_n; i++) {
		printf("\"%s\" = \t", json->pairs[i].name);
		switch (json->pairs[i].type) {
			case  JSON_STRING:
				printf("\"%s\"\n", json->pairs[i].value);
				break;
			case JSON_INT:
				printf("%s\n", json->pairs[i].value);
				break;
			case JSON_TRUE:
				printf("true\n");
				break;
			case JSON_FALSE:
				printf("false\n");
				break;
			case JSON_NULL:
				printf("null\n");
				break;
			case JSON_ARRAY:
				printf("array [%s]\n", json->pairs[i].value);
				break;
			default:
				printf("Tipo desconhecido %d\n", json->pairs[i].type);
		}
	}
	
	json->n_pairs = obj_n;
	return 0;
}

json_pair *json_get(json_parser *json, const char *search, int type) {
	json_pair search_pair, *result;
	search_pair.name = search;
	
	result = bsearch(&search_pair, json->pairs, json->n_pairs, sizeof(json_pair), cmp_json_pair);
	
	if (result == NULL)
		return NULL;
	
	if (result->type == type)
		return result;
	
	return NULL;
}

char *json_get_str(json_parser *json, const char *search) {
	json_pair *result;
	if ((result =json_get(json, search, JSON_STRING)) == NULL)
		return NULL;
	return result->value;
}


int json_get_int(json_parser *json, const char *search) {
	json_pair *result = json_get(json, search, JSON_INT);
	int n;
	if (result  == NULL)
		return JSON_INVALID;	///@TODO: Isso precisa ser arrumado (-1 não poderá ser lido pois retornará o mesmo que JSON_INVALID),
								/// mas não é importante agora
	sscanf(result->value, "%d", &n);
	return n;
}

int json_get_bool(json_parser *json, const char *search) {
	///@TODO: melhorar a interface (sem duas buscas aqui)
	/// talvez definir um tipo JSON_BOOL?
	if (json_get(json, search, JSON_TRUE)  != NULL)
		return 1;
	if (json_get(json, search, JSON_FALSE) != NULL)
		return 0;
	return JSON_INVALID;
}
