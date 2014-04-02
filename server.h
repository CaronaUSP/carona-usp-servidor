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
#include "config.h"
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
#include "hash.h"

// Constantes úteis:
#define DBG				fprintf(stderr, "Line %d\n", __LINE__)
#define error(msg)		do {perror(msg); exit(1);} while(0)
#define try(cmd,msg)	do {if ((cmd) == -1) {error(msg);}} while(0)
#define try0(cmd,msg)	do {if ((cmd) == NULL) {error(msg);}} while(0)
