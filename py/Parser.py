from enum import Enum

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
		return Node(NodeKind.NUM, None, None, val)


class Parser(object):
	def __init__(self, lexer):
		self.lexer = lexer

	def expr(self):
		node = self.mul()
		while True:
			if self.lexer.consume('+'):
				node = Node.new_node(NodeKind.ADD, node, self.mul())
			elif self.lexer.consume('-'):
				node = Node.new_node(NodeKind.SUB, node, self.mul())
			else:
				return node

	def mul(self):
		node = self.unary()
		while True:
			if self.lexer.consume('*'):
				node = Node.new_node(NodeKind.MUL, node, self.unary())
			elif self.lexer.consume('/'):
				node = Node.new_node(NodeKind.DIV, node, self.unary())
			else:
				return node
	
	def unary(self):
		if self.lexer.consume('+'):
			return self.primary()
		if self.lexer.consume('-'):
			return Node.new_node(NodeKind.SUB, Node.new_node_num(0), self.primary())
		return self.primary()

	def primary(self):
		if self.lexer.consume('('):
			node = self.expr()
			self.lexer.expect(')')
			return node
		return Node.new_node_num(self.lexer.expect_number())
	
