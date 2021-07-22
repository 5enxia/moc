from sys import argv

import Code
from Lexer import Lexer

def main():
	argc = len(argv)
	if argc != 2:
		exit(1)
	
	line = argv[1]
	token = Lexer.tokenize(line)
	Code.write(token)

main()