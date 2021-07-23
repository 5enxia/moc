from enum import Enum

import Lexer

class NodeKind(Enum):
	ADD = 0  # +
	SUB = 1  # -
	MUL = 2  # *
	DIV = 3  # /
	NUM = 4  # integer


class Node(object):
	def __init__(self, kind, lhs, rhs, val):
		self.kind = kind
		self.lhs = lhs
		self.rhs = rhs
		self.val = val

	@classmethod
	def new_node(cls, kind, lhs, rhs):
		return Node(kind, lhs, rhs, None)

	@classmethod
	def new_node_num(cls, val):
		return Node(None, None, None, val)


def expr():
	node = mul()
	while True:
		if Lexer.consume('+'):
			node = Node.new_node(NodeKind.ADD, node, mul())
		elif Lexer.consume('-'):
			node = Node.new_node(NodeKind.SUB, node, mul())
		else:
			return node

	@classmethod
	def mul(cls):
		node = Parser.primary()
		while True:
			if Lexer.consume('*'):
				node = Node.new_node(NodeKind.MUL, node, Perer.primary())
			elif Lexer.consume('/'):
				node = Node.new_node(NodeKind.DIV, node, Parser.primary())
			else:
				return node

	@classmethod
	def primary(cls):
		if Lexer.consume('('):
			node = Perser.expr()
			Lexer.expect(')')
			return node
		return Node.new_node_num(Lexer.expect_number())
	
