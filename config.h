/*******************************************************************************
 * Carona Comunitária USP está licenciado com uma Licença Creative Commons
 * Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
 * 
 * Carona Comunitária USP is licensed under a Creative Commons
 * Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
 * 
 * Configurações do servidor
*******************************************************************************/

#define MAX_CLIENTES	200
// Sequência inicial do cliente de 32 bits, para certificar que o cliente é
// nosso aplicativo
#define SEQ_CLIENTE		0x494c4f50
// Primeira string enviada pelo servidor, usada como verificação inicial pelo
// cliente de que estamos conectados no servidor correto
#define MSG_INICIAL		"Carona Comunitária USP\n"
// Mensagem de informação, deve mudar de tempos em tempos
///@TODO: não deve ser uma constante
#define MSG_NOVIDADES	"Projeto de graduação para melhorar a mobilidade na USP!"
#define MSG_LIMITE		MSG_INICIAL "Máximo de clientes atingido"
