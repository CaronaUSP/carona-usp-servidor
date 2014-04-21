#!/bin/bash
# Script para teste do servidor.
# Rode o servidor na porta 80 e depois o script no mesmo computador ou rode o servidor em outro computador,
# exporte as variáveis PORTA e IP (export PORTA=XXXX IP="XXXXX") e execute o script
# Olhe a saída do servidor. O esperado é:

# XXXX conectado (fd = X), total 1
# Thread criada, fd = X
# JSON start = 1
# Sorted:
# "hash" = 	"0cf47e7b3c036501c540140f100ca2f9f56ab858689b2d7f3f07e325c8e4d6ef"
# "usuario" = 	"teste"
# Hash calculado: 0CF47E7B3C036501C540140F100CA2F9F56AB858689B2D7F3F07E325C8E4D6EF
# Hash recebido: 0cf47e7b3c036501c540140f100ca2f9f56ab858689b2d7f3f07e325c8e4d6ef
# Autenticado!
# Thread 0: disconexão
# XXXX conectado (fd = 5), total 1
# Thread criada, fd = 5
# JSON start = 1
# Sorted:
# "a" = 	"sadfjsdahfsdafjgsdk"
# "abobora" = 	32645
# "hash" = 	"158256c9ac946fba45ca4868bc5b501deaf234ff6fcee73402222e503d4585a7"
# "inteiro" = 	0
# "nusp" = 	32645
# "str" = 	"a"
# "usuario" = 	"teste"
# Hash calculado: 8F636FCDC86908BBE1160A1A84318D4E22B5B6B2D8EE8FA0C89DE494F46BC149
# Hash recebido: 158256c9ac946fba45ca4868bc5b501deaf234ff6fcee73402222e503d4585a7
# Falha de autenticação
# Thread 0: disconexão


# Se não estiverem definidas, PORTA = 80 e IP = localhost
if [ -z "$PORTA" ]; then
	PORTA=80
fi

if [ -z "$IP" ]; then
	IP="localhost"
fi

# Primeiro pacote: gera autenticação normal com hash 1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF
hash=$(echo -n "Carona Comunitária USP
Projeto de graduação para melhorar a mobilidade na USP!
1 clientes já conectados, 1 atualmente, 0 caronas dadas1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF" |
sha256sum)
hash=${hash// */}

echo "Hash calculado: ${hash}"

echo -en "{ \"hash\" : \"$hash\", \"usuario\" : \"teste\" }\0" | nc $IP $PORTA

# Segundo pacote: objeto com inteiros e strings para testar interpretação JSON do servidor
echo -en '{ "hash" : "158256c9ac946fba45ca4868bc5b501deaf234ff6fcee73402222e503d4585a7", "a" : "sadfjsdahfsdafjgsdk", "str":"a", "inteiro" : 12345, "nusp" : 1, "abobora" : 2, "usuario" : "teste" } \0' | nc $IP $PORTA
