from sys import argv

from Lexer import Lexer
from Parser import Parser
from Code import Code

def main():
	argc = len(argv)
	if argc != 2:
		exit(1)
	
	lexer = Lexer(argv[1])
	parser = Parser(lexer)
	code = Code(parser)
	code.write()

main()