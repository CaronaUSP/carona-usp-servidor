#!/bin/bash

# Se não estiverem definidas, PORTA = 80 e IP = localhost
if [ -z "$PORTA" ]; then
	PORTA=80
fi

if [ -z "$IP" ]; then
	IP="localhost"
fi

make package_sender

for file in tests/test*
do
	cat $file | local/package_sender $IP $PORTA
done
