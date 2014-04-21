/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Funções para controle de hashes
*******************************************************************************/

#include "hash.h"

/*
 * Esta função checa se o hash retornado pelo cliente bate com o esperado.
 * 
 * Para informação sobre hashes:
 * http://en.wikipedia.org/wiki/Hashing_function
 * Algoritmo usado:
 * http://en.wikipedia.org/wiki/Secure_Hash_Algorithm
 * 
 * No servidor ficam salvos os hashes de uma mensagem contendo a senha (e nunca
 * as senhas originais).
 * Para gerar sempre um hash diferente a cada acesso (e evitar que a conta de
 * alguém possa ser sempre acessada caso o hash da senha seja descoberto), o
 * cliente recebe uma mensagem diferente para cada login (ver server.c - o código
 * envia o número de clientes já conectados ao servidor, número atual e caronas
 * dadas) e o cliente calcula o hash desta mensagem com o hash da senha (que o
 * servidor possui) (+ indica concatenação):
 * hash(mensagem + hash("Hashes - Carona Comunitária USP (CC BY-NC-SA 4.0) " + senha))
 * O servidor realiza:
 * hash(mensagem + hash que possuo da senha)
 * E compara os dois
 */

int senha_correta (const char *usuario, const char *mensagem_autenticacao, const char *hash_recebido) {
	// Ver man 3 sha
	unsigned char hash[SHA_256_DIGEST_LENGTH];
	char hash_senha[2 * SHA_256_DIGEST_LENGTH] = "1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF",
		hash_calculado[2 * SHA_256_DIGEST_LENGTH + 1];
	int i;
	
	EVP_MD_CTX calcula_hash;
	EVP_DigestInit(&calcula_hash, EVP_sha256());
	EVP_DigestUpdate(&calcula_hash, mensagem_autenticacao, strlen(mensagem_autenticacao));
	EVP_DigestUpdate(&calcula_hash, hash_senha, SHA_256_DIGEST_LENGTH * 2);
	EVP_DigestFinal(&calcula_hash, hash, NULL);
	
	for (i = 0; i < SHA_256_DIGEST_LENGTH; i++) {
		//printf("%X", (unsigned int) hash[i]);
		sprintf(hash_calculado + 2 * i, "%.2hhX", (unsigned int) (hash[i]));
	}
	
	hash_calculado[2 * SHA_256_DIGEST_LENGTH] = 0;
	
	printf("Hash calculado: %s\n", hash_calculado);
	printf("Hash recebido: %.64s\n", hash_recebido);
	return (!strncasecmp((char *) hash_calculado, hash_recebido, 2 * SHA_256_DIGEST_LENGTH));
}
