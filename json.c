/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Biblioteca para análise de pacotes JSON
*******************************************************************************/


// O código está bem básico e não suporta objetos dentro de objetos nem arrays.
// Suporte completo a JSON será adicionado mais tarde.

// Performance não está sendo levada em conta; a versão final do código deverá
// ordenar os objetos por nome na inicialização (qsort é bom pra isso) e usar
// busca binária em json_get.

#include "json.h"

void json_init(json_parser *json) {
	json->cur = json->start;
	json->cur_end = json->start + json->size - 1;
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

// json_get busca sempre na string inteira pela chave e não suporta arrays/objetos
// em objetos.
int json_get_str(json_parser *json, json_value *ret, const char *key) {
	char *inicio = json->cur, *fim, *valor_fim;
	size_t tamanho;
	for (;;) {
		for (; inicio <= json->cur_end; inicio++) {
			switch (*inicio) {
				case '\"':
					goto inicio_chave;
				case '\t':
				case '\r':
				case '\n':
				case ' ':
					break;
				default:
					return JSON_INVALID;
			}
		}
		
		inicio_chave:
		inicio++;
		if (inicio >= json->cur_end)
			return JSON_INVALID;
		
		fim = inicio;
		
		for (; fim <= json->cur_end; fim++)
			if (*fim == '\"')
				break;
		
		if (fim == json->cur_end || fim == inicio)
			return JSON_INVALID;
		
		tamanho = fim - inicio;
		if (tamanho == strlen(key)) {
			if (!strncmp(inicio, key, tamanho)) {	// chave encontrada
				
				ret->value = fim + 1;
				for (; ret->value <= json->cur_end; ret->value++)
					if (*(ret->value) == ':')
						break;
				
				if (ret->value == json->cur_end)
					return JSON_INVALID;
				
				for (; ret->value <= json->cur_end; ret->value++)
					if (*(ret->value) == '\"')
						break;
				
				ret->value++;
				
				if (ret->value >= json->cur_end)
					return JSON_INVALID;
				
				valor_fim = ret->value;
				
				for (; valor_fim <= json->cur_end; valor_fim++)
					if (*valor_fim == '\"')
						break;
				
				if (valor_fim > json->cur_end)
					return JSON_INVALID;
				ret->size = valor_fim - ret->value;
				return JSON_OK;
			}
		}
		
		for (; inicio <= json->cur_end; inicio++)
			if (*inicio == ',')
				break;
		
		if (inicio == json->cur_end)
			return JSON_INVALID;
		
	}
}
