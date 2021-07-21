from Node import Node, NodeKind

def generate(node: Node):
	if node.kind == NodeKind.ND_NUM:
		print(f'\tpush {node.val}')
		return
	
	generate(node.lhs)
	generate(node.rhs)
	print('\tpop rdi')
	print('\tpop rax')

	kind = node.kind
	if kind == NodeKind.ND_ADD:
		print('\tadd rax, rdi')
	elif kind == NodeKind.ND_SUB:
		print('\tsub rax, rdi')
	elif kind == NodeKind.ND_MUL:
		print('\timul rax, rdi')
	elif kind == NodeKind.ND_DIV:
		print('\tcqo')
		print('\tidiv rdi')

	print('\tpush rax')
