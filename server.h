/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Arquivo de definição de funções, constantes e inclusões do servidor
*******************************************************************************/

// Inclusões:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include "passwords.h"

// Constantes úteis:
#define DBG				fprintf(stderr, "Line %d\n", __LINE__)
#define error(msg)		do {perror(msg); exit(1);} while(0)
#define try(cmd,msg)	do {if ((cmd) == -1) {error(msg);}} while(0)
#define try0(cmd,msg)	do {if ((cmd) == NULL) {error(msg);}} while(0)
#define SHA_256_DIGEST_LENGTH	32

// Configurações do servidor:
#define MAX_CLIENTES	200
// Sequência inicial do cliente de 32 bits, para certificar que o cliente é
// nosso aplicativo
#define SEQ_CLIENTE		0x494c4f50
// Primeira string enviada pelo servidor, usada como verificação inicial pelo
// cliente de que estamos conectados no servidor correto. Não deve ser mudada!
#define MSG_INICIAL		"Carona Comunitária USP\n"
// Mensagem de informação, deve mudar de tempos em tempos
///@TODO: não deve ser uma constante
#define MSG_NOVIDADES	"Projeto de graduação para melhorar a mobilidade na USP!"
#define MSG_LIMITE		MSG_INICIAL "Máximo de clientes atingido"

int senha_correta (int usuario, const char *mensagem_autenticacao, const char *hash_recebido) {
	char hash_final[SHA_256_DIGEST_LENGTH];
	SHA_CTX calcula_hash;
	SHA1_Init(&calcula_hash);
	SHA1_Update(&calcula_hash, mensagem_autenticacao, strlen(mensagem_autenticacao));
	SHA1_Update(&calcula_hash, passwords[usuario], SHA_256_DIGEST_LENGTH);
	SHA1_Final((unsigned char *)hash_final, &calcula_hash);
	return (!strncmp(hash_final, hash_recebido, SHA_256_DIGEST_LENGTH));
}

inline unsigned char *sha256(const char *frase) {
	// recebendo um ponteiro nulo como destino, o hash é salvo em um vetor ESTÁTICO
	return SHA256((const unsigned char *)frase, strlen(frase), NULL);
}
