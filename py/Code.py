from Lexer import Lexer
from Parser import Parser, NodeKind, Node

def generate(node: Node):
	if node.kind == NodeKind.NUM:
		print(f'\tpush {node.val}')
		return
	
	generate(node.lhs)
	generate(node.rhs)

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

def write():
	node = Parser.expr()

	print('.intel_syntax noprefix')
	print('.globl main')
	print('main:')

	generate(node)

	print('\tpop rax')
	print('\tret')
