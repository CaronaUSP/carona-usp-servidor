/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * Funções paraa tratamento de fila de clientes esperando carona
*******************************************************************************/

#include "config.h"

#ifndef __FILA_CLIENTES_H__
#define __FILA_CLIENTES_H__

void adiciona_fila(int n);
void remove_fila(int n);

typedef struct lista_ligada_fila {
	int numero, anterior, prox;
} lista_ligada_fila_t;

extern lista_ligada_fila_t fila[MAX_CLIENTES];

#endif
