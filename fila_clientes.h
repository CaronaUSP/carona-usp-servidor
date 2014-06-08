/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * Funções paraa tratamento de fila de clientes esperando carona
*******************************************************************************/

#include "config.h"
#include <stdio.h>

#ifndef __FILA_CLIENTES_H__
#define __FILA_CLIENTES_H__

#define FILA_DA_CARONA		0
#define FILA_RECEBE_CARONA	1
#define FILA_RECEBE_CARONA_PAREADO	2
#define FILA_DA_CARONA_PAREADO	3

void inicializa_fila();
void adiciona_fila(int n, int tipo);
void remove_fila(int n);
void muda_tipo(int n, int tipo);
int prox_fila(int n);

typedef struct lista_ligada_fila {
	int tipo, anterior, prox;
} lista_ligada_fila_t;

extern lista_ligada_fila_t fila[MAX_CLIENTES];

#endif
