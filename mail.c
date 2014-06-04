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
// http://tools.ietf.org/html/rfc5322#section-3.6
#define TEXTO_MENSAGEM	\
"Date: Sat, 17 May 2014 12:18:27 -0300\r\n"\
"To: %s\r\n"\
"From: " SMTP_EMAIL " (Carona Comunitária USP)\r\n"\
"Reply-To: <noreply@noreply.br>\r\n"\
"Subject: Confirmação de e-mail - Carona USP\r\n"\
"\r\n"\
"Obrigado pelo interesse no projeto Carona Comunitária USP!\r\n"\
"O código para a criação da sua conta é %u.\r\n"\
"\r\n"\
"Aviso: Se você não requisitou uma conta, simplesmente ignore esse e-mail.\r\n"
#define min(a, b)	a < b ? a : b


CURL *curl;
int posicao_envio;
char mensagem[sizeof(TEXTO_MENSAGEM) + 264];
#ifdef CURL_MUTEX
pthread_mutex_t mutex_curl;		// mutex para envio de e-mails
#endif


// http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTREADFUNCTION
static size_t leitura_dados_curl(void *ptr, size_t size, size_t nmemb, __attribute__((unused)) void *userdata) {
	// Retornar o menor entre o restante da mensagem e o tamanho desejado:
	int tamanho_retorno = min(strlen(&mensagem[posicao_envio]), size * nmemb);
	memcpy(ptr, &mensagem[posicao_envio], tamanho_retorno);
	posicao_envio += tamanho_retorno;
	return tamanho_retorno;
}

#define setopt(opt, arg, str1, str2) 	if ((res = curl_easy_setopt(curl, opt, arg)) != CURLE_OK) {\
			fprintf(stderr, "curl_easy_setopt(curl, %s, %s): %d - %s\n", str1, str2, res, curl_easy_strerror(res));\
			return -1 ;}

int envia_email(const char *usuario, unsigned int codigo) {
	///@TODO: validar e-mail (sem caracteres inválidos) antes de enviar
	char email[257];	// limite de endereço válido é 256
	struct curl_slist *destinos = NULL;
	#ifdef CURL_MUTEX
	pthread_mutex_lock(&mutex_curl);
	#endif
	CURLcode res;
	posicao_envio = 0;
	
	if ((curl = curl_easy_init()) == NULL) {
		fprintf(stderr, "curl_easy_init() falhou\n");
		return -1;
	}
	
	int size = snprintf(email, sizeof(email), "<%s@usp.br>", usuario);
	if (size < 0 || size >= (int) sizeof(email)) {		// falha ou endereço não cabe em 256 bytes
		fprintf(stderr, "Falha na conversão de e-mail\n");
		return -1;
	}
	size = snprintf(mensagem, sizeof(mensagem), TEXTO_MENSAGEM, email, codigo);
	if (size < 0 || size >= (int) sizeof(mensagem)) {
		fprintf(stderr, "Falha na escrita de e-mail\n");
		return -1;
	}
	printf("Enviando:\n%s\n", mensagem);
	
	// Configurações de constantes no Makefile
	setopt(CURLOPT_USERNAME, SMTP_USER, "CURLOPT_USERNAME", SMTP_USER);
	setopt(CURLOPT_PASSWORD, SMTP_PASSWORD, "CURLOPT_PASSWORD", SMTP_PASSWORD);
	setopt(CURLOPT_URL, SMTP_ADDRESS, "CURLOPT_URL", SMTP_ADDRESS);
	
	setopt(CURLOPT_READFUNCTION, leitura_dados_curl, "CURLOPT_READFUNCTION", "leitura_dados_curl");
	
	destinos = curl_slist_append(destinos, email);
	setopt(CURLOPT_MAIL_RCPT, destinos, "CURLOPT_MAIL_RCPT", "destinos");
	//setopt(CURLOPT_READDATA, NULL, "CURLOPT_READDATA", "NULL");
	setopt(CURLOPT_UPLOAD, 1L, "CURL_UPLOAD", "1L");
	
	// Opcional:
	setopt(CURLOPT_VERBOSE, 1L, "CURLOPT_VERBOSE", "1L");
	
	res = curl_easy_perform(curl);
	
	if (res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform(): %d - %s\n", res, curl_easy_strerror(res));
	
	curl_slist_free_all(destinos);
	curl_easy_cleanup(curl);
	
	#ifdef CURL_MUTEX
	pthread_mutex_unlock(&mutex_curl);
	#endif
	
	return res == CURLE_OK ? 0 : -1;	// Apenas para manter consistente com as
										// outras funções (que retornam -1 em erro)
}
