from sys import argv
import re

_p_number = r'\d+'
_p_operator = r'[\+\-\*\/]'

_re_number = re.compile(_p_number)
_re_operator = re.compile(_p_operator)
_re_all = re.compile(_p_number + '|' + _p_operator)


def lexer(line):
	tokens = _re_all.findall(line)
	print(f'\tmov rax, {tokens.pop(0)}')

	while len(tokens) > 0:
		token = tokens.pop(0)
		if _re_operator.match(token):
			if token == '+':
				token = tokens.pop(0)
				if _re_number.match(token):
					print(f'\tadd rax, {token}')
			elif token == '-':
				token = tokens.pop(0)
				if _re_number.match(token):
					print(f'\tsub rax, {token}')
		else:
			pass

if __name__ == '__main__':
	argc = len(argv)
	if argc != 2:
		print("引数の個数が正しくありません")
		exit(1)
	p = argv[1]
	print(".intel_syntax noprefix")
	print(".globl main")
	print("main:")

	lexer(p)

	print("\tret")
