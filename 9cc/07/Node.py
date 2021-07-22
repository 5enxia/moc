from enum import Enum

class NodeKind(Enum):
	ND_ADD = 0 # +
	ND_SUB = 1 # -
	ND_MUL = 2 # *
	ND_DIV = 3 # /
	ND_NUM = 4 # 整数

class Node(object):
	@classmethod
	def new_node(cls, kind: NodeKind, lhs, rhs ):
		return Node(kind, lhs, rhs, None)

	@classmethod
	def new_node_num(cls, val):
		return Node(NodeKind.ND_NUM, None, None, val)

	def __init__(self, kind: NodeKind, lhs, rhs , val):
		self.kind = kind # ノードの型
		self.lhs = lhs # 左辺
		self.rhs = rhs # 右辺
		self.val = val # kindがND_NUMの場合のみ使う