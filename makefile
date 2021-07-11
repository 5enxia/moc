FLEX=flex
CC=gcc
BISON=bison -d -y 

all: f01 f03 c02 c02l m01

f01: flex01.lex
	$(FLEX) flex01.lex
	$(CC) lex.yy.c -o flex01.o

f03: flex03.lex
	$(FLEX) flex01.lex
	$(CC) lex.yy.c -o flex03.o

c02: calc02.c
	$(CC) calc02.c -o calc02.o

c02l: calc02.lex
	$(FLEX) calc02.lex
	$(BISON) calc02.y
	$(CC) -o calc02l.o y.tab.c lex.yy.c

m01: mci01.c
	$(CC) mci01.c -o mci01.o