#!/bin/bash

mkdir tests-tmp
cd tests-tmp
mkfifo pipe-msg-to-nc

../server $PORTA &
../file-to-out.sh pipe-msg-to-nc | nc localhost $PORTA

cd -
rm -r tests-tmp
