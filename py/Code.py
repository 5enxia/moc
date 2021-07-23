from Parser import NodeKind, Node

class Code(object):
	def __init__(self, parser):
		self.parser = parser

	def write(self):
		node = self.parser.expr()

		print('.intel_syntax noprefix')
		print('.globl main')
		print('main:')

		Code.generate(node)

		print('\tpop rax')
		print('\tret')

	@classmethod
	def generate(cls, node: Node):
		if node.kind == NodeKind.NUM:
			print(f'\tpush {node.val}')
			return
		
		Code.generate(node.lhs)
		Code.generate(node.rhs)

		print('\tpop rdi')
		print('\tpop rax')

		if node.kind == NodeKind.ADD:
			print('\tadd rax, rdi')
		elif node.kind == NodeKind.SUB:
			print('\tsub rax, rdi')
		elif node.kind == NodeKind.MUL:
			print('\timul rax, rdi')
		elif node.kind == NodeKind.DIV:
			print('\tcqo')
			print('\tidiv rdi')
		
		print('\tpush rax')

