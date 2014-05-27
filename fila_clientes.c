/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Funções paraa tratamento de fila de clientes esperando carona
*******************************************************************************/

#include "fila_clientes.h"

int ultimo_fila = 0;
lista_ligada_fila_t fila[MAX_CLIENTES];

void adiciona_fila(int n) {
	int i;
	for (i = 0; i < MAX_CLIENTES; i++) {
		if (fila[i].numero == -1) {		// se posição vazia
			fila[i].numero = n;			// salva n
			fila[i].anterior = ultimo_fila;
			fila[ultimo_fila].prox = i;	// guarda na entrada anterior um link para a atual
			ultimo_fila = i;
			break;
		}
	}
}

void remove_fila(int n) {
	int pos;
	for (pos = 0; pos < MAX_CLIENTES; pos++)
		if (fila[pos].numero == n)
			break;
	
	fila[pos].numero = -1;
	if (pos == ultimo_fila) {
		ultimo_fila = fila[pos].anterior;
	} else {
		fila[fila[pos].anterior].prox = fila[pos].prox;
		fila[fila[pos].prox].anterior = fila[pos].anterior;
	}
}
