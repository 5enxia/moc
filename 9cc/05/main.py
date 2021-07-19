from sys import argv
import re
from enum import Enum

_p_number = r'\d+'
_p_operator = r'[\+\-\*\/]'
_p_parentheses = r'[\(\)]'

_re_number = re.compile(_p_number)
_re_operator = re.compile(_p_operator)
_re_parentheses = re.compile(_p_parentheses)
_re_all = re.compile(_p_number + '|' + _p_operator + '|' + _p_parentheses)

tokens = []

class NodeKind(Enum):
	ND_ADD = 0 # +
	ND_SUB = 1 # -
	ND_MUL = 2 # *
	ND_DIV = 3 # /
	ND_NUM = 4 # 整数

class Node(object):
	@classmethod
	def new_node(kind: NodeKind, lhs: Node, rhs: Node):
		return Node(kind, lhs, rhs, None)

	@classmethod
	def new_node_num(val):
		return Node(NodeKind.ND_NUM, None, None, val)

	def __init__(self, kind: NodeKind, lhs: Node, rhs: Node, val):
		self.kind = kind # ノードの型
		self.lhs = lhs # 左辺
		self.rhs = rhs # 右辺
		self.val = val # kindがND_NUMの場合のみ使う

def expr():
	token = tokens.pop(0)
	if token == '+':
		pass
	elif token == '-':
		pass
	else:
		pass

def mul():
	token = tokens.pop(0)
	if token == '*':
		pass
	elif token == '/':
		pass
	else:
		pass

def primary():
	token = tokens.pop(0)
	if token == '(': 
		pass
	token = tokens.pop(0)
	if token == ')': 
		pass

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
