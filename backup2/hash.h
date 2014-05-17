/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Funções para controle de hashes
*******************************************************************************/

#ifndef __HASH_H__
#define __HASH_H__

#include "global.h"
#include <openssl/evp.h>

#define NUMERO_TOTAL_USUARIOS	(sizeof(passwords) / sizeof(passwords[0]))

int senha_correta (const char *usuario, const char *mensagem_autenticacao, const char *hash_recebido);

#endif
