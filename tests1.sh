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

# Primeiro pacote: gera autenticação normal com hash 1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF
# para usuário que receberá carona
hash=$(echo -n "Carona Comunitária USP
Projeto de graduação para melhorar a mobilidade na USP!
1 clientes já conectados, 1 atualmente, 0 caronas dadas1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF1234567890ABCDEF" |
sha256sum)
hash=${hash// */}

echo "Hash calculado: ${hash}"

echo -en "{ \"hash\" : \"$hash\", \"usuario\" : \"teste\", \"da_carona\" : false, \"inicio\" : 0, \"fim\" : 2 }\0" | nc $IP $PORTA
