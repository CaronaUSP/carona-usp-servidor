/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Funções para envio de e-mails de confirmação
*******************************************************************************/

#ifndef __MAIL_H__
#define __MAIL_H__

#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#ifdef CURL_MUTEX
#include <pthread.h>
#endif

extern CURL *curl;	// Todos os envios vão usar a mesma conexão com o servidor.
					// Cada thread que requisitar um envio vai adquirir um mutex
					// e outras vão esperar a liberação. Há outras opções (criar
					// uma pilha no programa ou usar a interface 
					// http://curl.haxx.se/libcurl/c/libcurl-multi.html ou criar
					// várias conexões e isolar a função),
					// mas é desnecessário otimizar aqui (provavelmete não
					// pasaremos de alguns cadastros por minuto)

extern int inicializa_email();
extern int envia_email(const char *usuario, unsigned int codigo);

#endif
