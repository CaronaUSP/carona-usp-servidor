/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Biblioteca para análise de pacotes JSON
*******************************************************************************/

#ifndef __JSON_H__
#define __JSON_H__

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define JSON_OK			0
#define JSON_INVALID	-1
#define JSON_INCOMPLETE	-2
#define JSON_NOMEM		-3

#define JSON_FALSE		0
#define JSON_TRUE		1
#define JSON_STRING		2
#define JSON_OBJECT		3
#define JSON_ARRAY		4
#define JSON_NULL		5

#define JSON_NUMBER		1 << 3
#define JSON_INT		JSON_NUMBER
#define JSON_FLOAT		JSON_NUMBER | 1

typedef struct {
	char *name;
	int type;
	void *value;
} json_pair;

typedef struct {
	char *start, *cur;
	json_pair *pairs;
	int n_pairs;
} json_parser;

typedef struct {
	char *name;
	int type;
} json_bool;

typedef struct {
	char *name;
	int type;
	char *value;
} json_str;

typedef struct {
	char *name;
	int type;
	int value;
} json_int;

typedef struct {
	char *name;
	int type;
	int *value;
} json_float;

typedef struct {
	char *value;
	size_t size;
} json_value;

void json_init(json_parser *json);
char *json_get_str(json_parser *json, const char *search);
int json_all_parse(json_parser *json);
int json_get_int(json_parser *json, const char *search);
#endif
