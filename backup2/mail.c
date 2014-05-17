/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Funções para envio de e-mails de confirmação
*******************************************************************************/

// http://curl.haxx.se/libcurl/c/ e http://curl.haxx.se/libcurl/c/allfuncs.html
// são úteis :)

#include "mail.h"

int send(const char *usuario, int codigo) {
	CURL *curl;
	curl = curl_easy_init();
	
}
