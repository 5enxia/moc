from sys import argv

import Code
import Lexer

def main():
	argc = len(argv)
	if argc != 2:
		exit(1)
	
	line = argv[1]
	token = Lexer.tokenize(line)
	Code.Lexer.token = Lexer.token
	Code.write()

main()