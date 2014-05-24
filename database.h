/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Funções para o banco de dados de usuários
*******************************************************************************/

#ifndef __PASSWORDS_H__
#define __PASSWORDS_H__

#define error(msg)		do {perror(msg); exit(1);} while(0)
#define tryEOF(cmd,msg)	do {if ((cmd) == EOF) {error(msg);}} while(0)

typedef struct usuario_struct {
	const char *email;
	const char *hash;
	struct usuario_struct *next;
} usuario_t;

extern usuario_t *primeiro_usuario;

extern int init_db(const char *arquivo);
extern int add_user(const char *email, const char *hash);
extern void save_db(const char *arquivo);
extern const char *get_user(const char *email);

#endif
