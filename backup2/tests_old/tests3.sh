#!/bin/bash
# Script para teste do servidor.
# Rode o servidor na porta 80 e depois o script no mesmo computador ou rode o servidor em outro computador,
# exporte as variáveis PORTA e IP (export PORTA=XXXX IP="XXXXX") e execute o script


# Se não estiverem definidas, PORTA = 80 e IP = localhost
if [ -z "$PORTA" ]; then
	PORTA=80
fi

if [ -z "$IP" ]; then
	IP="localhost"
fi

# Terceiro pacote: objeto com inteiros e strings para testar interpretação JSON do servidor
echo -en '{ "hash" : "158256c9ac946fba45ca4868bc5b501deaf234ff6fcee73402222e503d4585a7", "a" : "sadfjsdahfsdafjgsdk", "str":"a", "inteiro" : 12345, "nusp" : 1, "abobora" : 2, "usuario" : "teste", "verdadeiro" : true, "falso" : false, "nulo" : null, "array" : [1, 2, 3, 4, 5, 6, 7, "oi", "foo", "bar"] } \0' | nc $IP $PORTA
