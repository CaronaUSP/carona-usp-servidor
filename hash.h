/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Funções para controle de hashes
*******************************************************************************/

#define SHA_256_DIGEST_LENGTH	32

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
