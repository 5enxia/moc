from sys import argv
import re

from Node import NodeKind, Node
import Code

_p_number = r'\d+'
_p_operator = r'[\+\-\*\/]'
_p_parentheses = r'[\(\)]'

_re_number = re.compile(_p_number)
_re_operator = re.compile(_p_operator)
_re_parentheses = re.compile(_p_parentheses)
_re_all = re.compile(_p_number + '|' + _p_operator + '|' + _p_parentheses)

tokens = []
token = None

def consume(target):
	if tokens == []:
		return False
	elif target != tokens[0]:
		return False
	else:
		tokens.pop(0)
		return True

def expr():
	node = mul()
	while True:
		if consume('+'):
			node = Node.new_node(NodeKind.ND_ADD, node, mul())
		elif consume('-'):
			node = Node.new_node(NodeKind.ND_SUB, node, mul())
		else:
			return node

def mul():
	node = unary()
	while True:
		if consume('*'):
			node = Node.new_node(NodeKind.ND_MUL, node, unary())
		elif consume('/'):
			node = Node.new_node(NodeKind.ND_DIV, node, unary())
		else:
			return node

def unary():
	if consume('+'):
		return primary()
	if consume('-'):
		return Node.new_node(NodeKind.ND_SUB, Node.new_node_num(0), primary())
	return primary()

def primary():
	if consume('('):
		node = expr()
		if consume(')'):
			return node
		else:
			exit(0)
	elif _re_number.match(tokens[0]):
		return Node.new_node_num(tokens.pop(0))


def lexer(line):
	tokens = _re_all.findall(line)
	token = tokens.pop(0)
	print(f'\tmov rax, {token}')

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
	tokens = _re_all.findall(p)

	node = expr()

	print(".intel_syntax noprefix")
	print(".globl main")
	print("main:")

	Code.generate(node)

	print("\tpop rax")
	print("\tret")