from Lexer import Lexer

def write(token):
	Lexer.token = token

	print('.intel_syntax noprefix')
	print('.globl main')
	print('main:')
	print(f'\tmov rax, {Lexer.expect_number()}')

	while not Lexer.at_eof():
		if Lexer.consume('+'):
			print(f'\tadd rax, {Lexer.expect_number()}')
			continue

		Lexer.expect('-')
		print(f'\tsub rax, {Lexer.expect_number()}')

	print('\tret')
