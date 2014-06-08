/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Arquivo de definição de constantes e inclusões globais do servidor (para todos
 * os arquivos)
*******************************************************************************/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

// Inclusões para todos os arquivos:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "config.h"

// Constantes úteis:
#define DBG				fprintf(stderr, "Line %d\n", __LINE__)
#define error(msg)		do {perror(msg); exit(1);} while(0)
#define try(cmd,msg)	do {if ((cmd) == -1) {error(msg);}} while(0)
#define try0(cmd,msg)	do {if ((cmd) == NULL) {error(msg);}} while(0)
#define tryEOF(cmd,msg)	do {if ((cmd) == EOF) {error(msg);}} while(0)
#define SHA_256_DIGEST_LENGTH	32

#ifndef NAO_CHECA_JA_CONECTADO
#error NAO_CHECA_JA_CONECTADO não definido, mas versão atual só pode ser gerada com essa flag\
 (tivemos uns problemas com IPv6, versões mais antigas no repositório suportam apenas IPv4).\
 Rode "make clean" e tente novamente.
#endif

#endif
