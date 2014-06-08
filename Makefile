################################################################################
# Carona Comunitária USP está licenciado com uma Licença Creative Commons
# Atribuição-NãoComercial-CompartilhaIgual 4.0 Internacional (CC BY-NC-SA 4.0).
# 
# Carona Comunitária USP is licensed under a Creative Commons
# Attribution-NonCommercial-ShareAlike 4.0 International License (CC BY-NC-SA 4.0).
# 
# Makefile para compilar o projeto
################################################################################

CC=gcc
#-Og
DBG=-g
OPTIMIZE=-O3
LIB_PTHREADS=-lpthread
LIB_CRYPTO=-lcrypto
LIB_CURL=-lcurl

# Variáveis para envio de e-mails
ifndef SMTP_USER
$(error SMTP_USER não está configurado)
endif
ifndef SMTP_PASSWORD
$(error SMTP_PASSWORD não está configurado)
endif
ifndef SMTP_EMAIL
$(error SMTP_EMAIL não está configurado)
endif
ifndef SMTP_ADDRESS
$(error SMTP_ADDRESS não está configurado - use o formato smtp://servidor:porta)
endif

# Esses devem ser mudados para configurar a compilação como desejado
# Defines que retiram algumas partes de código, úteis para teste:
# NAO_CHECA_JA_CONECTADO: Permite várias conexões do mesmo IP
# NAO_CHECA_SENHA: Não checa se o hash está correto
DEFINES=-DNAO_CHECA_JA_CONECTADO -DSMTP_USER='"$(SMTP_USER)"' -DSMTP_PASSWORD='"$(SMTP_PASSWORD)"' -DSMTP_EMAIL='"$(SMTP_EMAIL)"' -DSMTP_ADDRESS='"$(SMTP_ADDRESS)"' -DCURL_MUTEX -D_GNU_SOURCE -DNAO_CHECA_SENHA
# Flags para gerar objetos (usar OPTIMIZE no lugar de DBG ativa várias otimizações
# e não inclui dados de depuração no executável):
CFLAGS=-c -Wall -Wextra $(DBG) $(DEFINES)
# Flags para o linker (adicione outras bibiliotecas, caso necessário):
LDFLAGS=$(LIB_PTHREADS) $(LIB_CRYPTO) $(LIB_CURL)
# Nome do executável a ser gerado:
EXECUTABLE=server
# Arquivos de configurações globais (edições fazem o projeto todo ser reconstruído):
GLOBALDEPS=global.h config.h
# Diretório de saída:
OUTPUT=bin/

TARGETS:=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,$(TARGETS))
ALL_OBJECTS:=$(wildcard *.o)
# $< = prerequisito, $@ = alvo:
COMPILE=$(CC) $(CFLAGS) $< -o $@
LINK=$(CC) $(OBJECTS) -o $(OUTPUT)$@ $(LDFLAGS)
RM=-rm -rf
MKDIR=mkdir -p
MAKEFILE_PATH:=$(abspath $(lastword $(MAKEFILE_LIST)))
SOURCE_DIR:=$(dir $(MAKEFILE_PATH))

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(MKDIR) $(OUTPUT)
	$(LINK)

# Declarações de dependência entre arquivos.
# Use:
# [saída].o: [requisitos]
# Para definir os requisitos do objeto.
# Definir os requisitos incorretamente gerará erros na linkagem (arquivos que
# dependem de outros não serão reconstruídos)
# Caso o make não funcione, é possível que as dependências estejam erradas. Tente:
# make clean && make all
# Para apagar os objetos já gerados e iniciar a compilação do zero e poste o bug no github.
# Caso você se esqueça de adicionar as dependências de um arquivo, há uma regra padrão
# definida que gerará o arquivo usando como dependências seu .c, .h, e $(GLOBALDEPS) e
# mostrando uma mensagem de aviso na compilação

json.o: json.c json.h
	$(COMPILE)

mail.o: mail.c mail.h
	$(COMPILE)

hash.o: hash.c hash.h $(GLOBALDEPS)
	$(COMPILE)

conexao.o: conexao.c conexao.h hash.o mail.o $(GLOBALDEPS)
	$(COMPILE)

init.o: init.c init.h conexao.o mail.o database.o $(GLOBALDEPS)
	$(COMPILE)

server.o: server.c server.h conexao.o init.o $(GLOBALDEPS)
	$(COMPILE)

database.o: database.c database.h
	$(COMPILE)

fila_clientes.o: fila_clientes.c fila_clientes.h config.h
	$(COMPILE)

envia_email: tests/envia_email.c mail.o
	$(MKDIR) $(OUTPUT)
	$(CC) $(CFLAGS) tests/envia_email.c -o $(SOURCE_DIR)/envia_email.o -I$(SOURCE_DIR)
	$(CC) mail.o envia_email.o -o $(OUTPUT)/envia_email $(LIB_CURL)

database_test: tests/database_test.c database.o
	$(MKDIR) $(OUTPUT)
	$(CC) $(CFLAGS) tests/database_test.c -o $(SOURCE_DIR)/database_test.o -I$(SOURCE_DIR)
	$(CC) database_test.o database.o -o $(OUTPUT)/database_test

package_sender: tests/package_sender.c
	$(MKDIR) $(OUTPUT)
	$(CC) $(CFLAGS) tests/package_sender.c -o $(SOURCE_DIR)/package_sender.o
	$(CC) package_sender.o -o $(OUTPUT)/package_sender

%.o: %.c %.h $(GLOBALDEPS)
	@echo "********************************************************************"
	@echo "ATENÇÃO: dependências não definidas para $@"
	@echo "Usando regras padrões"
	@echo "********************************************************************"
	$(COMPILE)

clean:
	$(RM) $(ALL_OBJECTS) $(EXECUTABLE)
