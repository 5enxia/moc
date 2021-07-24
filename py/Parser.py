from enum import Enum, auto

class NodeKind(Enum):
	ADD = 0  # +
	SUB = auto()  # -
	MUL = auto()  # *
	DIV = auto()  # /
	EQ = auto()  #  ==
	NE = auto()  #  !=
	LT = auto()  #  <
	LE = auto()  #  <=
	NUM = auto()  # integer


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
		self.code = []
		self.lexer = lexer

	# def program(self):
	# 	while self.lexer.at_eof():
	# 		self.code.append(stmt())
	# 	self.code.append(None)
	
	# def stmt(self):
	# 	node = self.expr()
	# 	self.lexer.expect(';')
	# 	return node

	# def expr(self):
	# 	node = self.mul()
	# 	while True:
	# 		if self.lexer.consume('+'):
	# 			node = Node.new_node(NodeKind.ADD, node, self.mul())
	# 		elif self.lexer.consume('-'):
	# 			node = Node.new_node(NodeKind.SUB, node, self.mul())
	# 		else:
	# 			return node

	def expr(self):
		return self.equality()
	
	def equality(self):
		node = self.relational()
		while True:
			if self.lexer.consume('=='):
				node = Node.new_node(NodeKind.EQ, node, self.relational())
			elif self.lexer.consume('!='):
				node = Node.new_node(NodeKind.NE, node, self.relational())
			else:
				return node

	def relational(self):
		node = self.add()
		while True:
			if self.lexer.consume('<'):
				node = Node.new_node(NodeKind.LT, node, self.add())
			elif self.lexer.consume('<='):
				node = Node.new_node(NodeKind.LE, node, self.add())
			elif self.lexer.consume('>'):
				node = Node.new_node(NodeKind.LT, self.add(), node)
			elif self.lexer.consume('>='):
				node = Node.new_node(NodeKind.LE, self.add(), node)
			else:
				return node
	
	def add(self):
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
			return self.unary()
		if self.lexer.consume('-'):
			return Node.new_node(NodeKind.SUB, Node.new_node_num(0), self.unary())
		return self.primary()

	def primary(self):
		if self.lexer.consume('('):
			node = self.expr()
			self.lexer.expect(')')
			return node
		return Node.new_node_num(self.lexer.expect_number())
	
