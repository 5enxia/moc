from sys import stderr
from Parser import NodeKind, Node

class Code(object):
	def __init__(self, parser):
		self.parser = parser

	def write(self):
		node = self.parser.program()

		print('.intel_syntax noprefix')
		print('.globl main')
		print('main:')

		# alloc 26 variable
		print('\tpush rbp')
		print('\tmov rbp, rsp')
		print('\tsub rsp, 208')

		for code in self.parser.code:
			if code:
				Code.generate(code)
				print('\tpop rax')

		print('\tmov rsp, rbp')
		print('\tpop rbp')
		print('\tret')

	@classmethod
	def generate(cls, node: Node):
		if node.kind == NodeKind.NUM:
			print(f'\tpush {node.val}')
			return
		elif node.kind == NodeKind.LVAR: 
			Code.gen_lval(node)
			print('\tpop rax')
			print(f'\tmov rax, [rax]')
			print('\tpush rax')
			return
		elif node.kind == NodeKind.ASSIGN: 
			Code.gen_lval(node.lhs)
			Code.generate(node.rhs)
			print('\tpop rdi')
			print('\tpop rax')
			print('\tmov [rax], rdi')
			print('\tpush rdi')
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

	@classmethod
	def gen_lval(cls, node: Node):
		if node.kind != NodeKind.LVAR:
			Code._error(' left value is not local variable.')
		print('\tmov rax, rbp')
		print(f'\tsub rax, {node.offset}')
		print('\tpush rax')

	@classmethod
	def _error(cls, msg):
		print(msg, file=stderr)
		exit(1)