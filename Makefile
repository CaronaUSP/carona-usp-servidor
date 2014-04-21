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

# Esses devem ser mudados para configurar a compilação como desejado
# Flags para gerar objetos (usar OPTIMIZE no lugar de DBG ativa várias otimizações
# e não inclui dados de depuração no executável):
CFLAGS=-c -Wall -Wextra $(DBG)
# Flags para o linker (adicione outras bibiliotecas, caso necessário):
LDFLAGS=$(LIB_PTHREADS) $(LIB_CRYPTO)
# Nome do executável a ser gerado:
EXECUTABLE=server
# Arquivos de configurações globais (edições fazem o projeto todo ser reconstruído):
GLOBALDEPS=global.h config.h

TARGETS:=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,$(TARGETS))
ALL_OBJECTS:=$(wildcard *.o)
# $< = prerequisito, $@ = alvo:
COMPILE=$(CC) $(CFLAGS) $< -o $@
LINK=$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
RM=-rm -rf

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
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

hash.o: hash.c hash.h $(GLOBALDEPS)
	$(COMPILE)

conexao.o: conexao.c conexao.h hash.o $(GLOBALDEPS)
	$(COMPILE)

init.o: init.c init.h conexao.o $(GLOBALDEPS)
	$(COMPILE)

server.o: server.c server.h conexao.o init.o $(GLOBALDEPS)
	$(COMPILE)

%.o: %.c %.h $(GLOBALDEPS)
	@echo "********************************************************************"
	@echo "ATENÇÃO: dependências não definidas para $@"
	@echo "Usando regras padrões"
	@echo "********************************************************************"
	$(COMPILE)

clean:
	$(RM) $(ALL_OBJECTS) $(EXECUTABLE)
