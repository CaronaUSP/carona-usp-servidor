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

int ultimo_fila = -1;
lista_ligada_fila_t fila[MAX_CLIENTES];

void inicializa_fila() {
	int i;
	for (i = 0; i < MAX_CLIENTES; i++) {	// desnecessário, mas ajuda a depurar em caso de erro
		fila[i].tipo = -1;
		fila[i].anterior = -1;
		fila[i].prox = -1;
	}
}

///@TODO: testar isso direito
void adiciona_fila(int n, int tipo) {
	// adiciona_fila é chamada com n = número da thread, que é único em cada momento.
	// Então, salvar em fila[n] é seguro
	fila[n].tipo = tipo;
	fila[n].anterior = ultimo_fila;
	fila[ultimo_fila].prox = n;	// guarda na entrada anterior um link para a atual
	ultimo_fila = n;
}

void remove_fila(int n) {
	if (fila[n].tipo == -1)
		fprintf(stderr, "remove_fila() - %d não inicializado\n", n);
	else {
		if (n == ultimo_fila) {
			ultimo_fila = fila[n].anterior;
		} else {
			fila[fila[n].anterior].prox = fila[n].prox;
			fila[fila[n].prox].anterior = fila[n].anterior;
		}
		fila[n].tipo = -1;
		fila[n].anterior = -1;
		fila[n].prox = -1;
	}
}

void muda_tipo(int n, int tipo) {
	if (fila[n].tipo == -1)
		fprintf(stderr, "muda_tipo() - %d não inicializado\n", n);
	fila[n].tipo = tipo;
}

int inicio_fila() {
	int n = ultimo_fila;
	while (fila[n].anterior != -1) {
		n = fila[n].anterior;
	}
	if (fila[n].tipo == -1)		// primeira entrada é vazia = pilha vazia
		return -1;
	return n;
}

int prox_fila(int n) {
	if (n == -1) {	// n = -1 significa pegar primeiro da fila
		n = inicio_fila();
		if (n == -1)	// fila vazia
			return -1;
		return n;
	}
	
	n = fila[n].prox;
	if (fila[n].tipo == -1)	// fim da fila
		return -1;
	return n;
}
