#!/bin/bash

# Se não estiverem definidas, PORTA = 80 e IP = localhost
if [ -z "$PORTA" ]; then
	PORTA=80
fi

if [ -z "$IP" ]; then
	IP="localhost"
fi

mkdir -p local
gcc -o local/package_sender tests/package_sender.c

for file in tests/test*.sh
do
	cat $file | local/package_sender $IP $PORTA
done
