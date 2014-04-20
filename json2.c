/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Biblioteca para análise de pacotes JSON
*******************************************************************************/

#include "json2.h"

static int cmp_json_pair(const void *p1, const void *p2) {	// ponteiros para json_pair
	return strcmp( ( (json_pair const *) p1)->name, ( (json_pair const *) p2)->name);
}

void json_init(json_parser *json) {
	json->cur = json->start;
	json->cur_end = json->start + json->size - 1;
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

static char *json_getstr(json_parser *json) {
	char *end, *p;
	for (; *(json->cur); json->cur++) {
		switch (*json->cur) {
			case '\"':
				end = rawmemchr(json->cur + 1, '\"');
				p = malloc(end - json->cur);
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

static char *json_get_value_str(json_parser *json) {
	for (; *json->cur; json->cur++) {
		switch (*json->cur) {
			case ':':
				json->cur++;
				return json_getstr(json);
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


static void bp() {}	//para depuração
#define try(cmd)	do {int __ret = (cmd); if (__ret < 0) return __ret;} while(0)
#define try0(cmd)	do {if ((cmd) == NULL) return JSON_INVALID;} while(0)
int json_all_parse(json_parser *json) {
	try(search_object(json));
	printf("JSON start = %d\n", json->cur - json->start);
	///@TODO: checar objetos vazios
	int obj_n = 0;
	do {
		char *str;
		try0(str = json_getstr(json));
		
		char *value;
		try0(value = json_get_value_str(json));
		printf("String %s = \"%s\"\n", str, value);
		
		json->pairs[obj_n].name = str;
		json->pairs[obj_n].type = JSON_STRING;
		json->pairs[obj_n].value = value;
		
		obj_n++;
	} while (json_next(json) == 1);
	bp();
	qsort(json->pairs, obj_n, sizeof(json_pair), cmp_json_pair);
	
	printf("Sorted:\n");
	int i;
	for (i=0; i < obj_n; i++) {
		json_str *str = (json_str *) &json->pairs[i];
		printf("%s = %s\n", str->name, str->value);
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

int json_parse(json_parser *json) {
	
	// Linha suspeita:
	memmove(json->start, json->cur_end + 1, json->size - (json->cur_end - json->start + 1));
	for (; json->cur < json->start + json->size && *(json->cur) != '{'; json->cur++) {
		switch (*(json->cur)) {
			case '\t':
			case '\r':
			case '\n':
			case ' ':
				break;
			default:
				return JSON_INVALID;
		}
	}
	
	if (json->cur >= json->start + json->size) {
		*(json->cur) = *(json->start);
		return JSON_INCOMPLETE;
	}
	
	int i_stack = 0;
	json->cur++;
	
	for (json->cur_end = json->cur; json->cur_end < json->start + json->size; json->cur_end++) {
		switch (*(json->cur_end)) {
			case '{':
				i_stack++;
				break;
			case '}':
				if (i_stack == 0) {
					json->cur_end--;
					return JSON_OK;
				}
				i_stack--;
				break;
		}
	}
	return JSON_INCOMPLETE;
}
