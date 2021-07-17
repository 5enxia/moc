from sys import argv
import re

_p_number = r'\d+'
_p_operator = r'[\+\-\*\/]'

_re_number = re.compile(_p_number)
_re_operator = re.compile(_p_operator)
_re_all = re.compile(_p_number + '|' + _p_operator)


def lexer(line):
	tokens = _re_all.findall(line)
	print("\tmov rax, {}".format(tokens[0]))
	operator = None
	operand = None
	template = ''
	for token in tokens[1:]:
		if _re_operator.match(token):
			if token == '+':
				template = '\tadd rax, {}'
			elif token == '-':
				template = '\tsub rax, {}'
		elif _re_number.match(token):
			print(template.format(token))
		

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
