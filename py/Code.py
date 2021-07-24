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
		elif node.kind == NodeKind.EQ:
			print("\tcmp rax, rdi")
			print("\tsete al")
			print("\tmovzb rax, al")
		elif node.kind == NodeKind.NE:
			print("\tcmp rax, rdi")
			print("\tsetne al")
			print("\tmovzb rax, al")
		elif node.kind == NodeKind.LT:
			print("\tcmp rax, rdi")
			print("\tsetl al")
			print("\tmovzb rax, al")
		elif node.kind == NodeKind.LE:
			print("\tcmp rax, rdi")
			print("\tsetle al")
			print("\tmovzb rax, al")
		
		print('\tpush rax')

