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
#include <openssl/sha.h>

// Senhas iniciais
// As senhas são compiladas com o programa e estáticas (por enquanto)
// Hashes sha-256 de ("Hashes - Carona Comunitária USP (CC BY-NC-SA 4.0) " + senha)

const char passwords[2][32] = {
	//Senha: 01
	{0x67, 0x47, 0xdc, 0x33, 0x37, 0xab, 0x9e, 0x02, 0x1b, 0x66, 0x8a, 0x94, 0xce, 0xf9, 0x04, 0xbb,
	0xda, 0x3e, 0x1b, 0x85, 0x15, 0xd4, 0x75, 0x05, 0x74, 0x7a, 0x27, 0x14, 0xb0, 0x0d, 0x5a, 0xef},
	
	//Senha: 02
	{0x05, 0x9f, 0xa7, 0xaa, 0x12, 0x58, 0x12, 0xfc, 0xee, 0x9a, 0x43, 0xad, 0xd4, 0x50, 0x6c, 0xc4,
	0xd7, 0x39, 0xc6, 0xb0, 0x40, 0x93, 0xb9, 0x3e, 0x56, 0x63, 0x75, 0x22, 0xb3, 0xef, 0xc4, 0x2c},
};

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

int senha_correta (int usuario, const char *mensagem_autenticacao, const char *hash_recebido) {
	// Ver man 3 sha
	unsigned char hash_final[SHA_256_DIGEST_LENGTH];
	SHA_CTX calcula_hash;
	SHA1_Init(&calcula_hash);
	SHA1_Update(&calcula_hash, mensagem_autenticacao, strlen(mensagem_autenticacao));
	SHA1_Update(&calcula_hash, passwords[usuario], SHA_256_DIGEST_LENGTH);
	SHA1_Final(hash_final, &calcula_hash);
	return (!strncmp((char *) hash_final, hash_recebido, SHA_256_DIGEST_LENGTH));
}
