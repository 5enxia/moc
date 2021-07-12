FLEX=flex
CC=gcc
BISON=bison -d -y 

all: m05

l00: lexer00.c
	$(CC) lexer00.c -o lexer00.o

l01: lexer01.c
	$(CC) lexer01.c -o lexer01.o

l02: lexer02.c
	$(CC) lexer02.c -o lexer02.o

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

m05: mini05.c
	$(CC) mini05.c -o mini05.o
