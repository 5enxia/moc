import re
from sys import stderr
from enum import Enum

_p_number = r'\d+'
_p_operator = r'[\+\-]'
_re_number = re.compile(_p_number)
_re_operator = re.compile(_p_operator)
_re_all = re.compile(_p_number + '|' + _p_operator)

class TokenKind(Enum):
	RESERVED = 0
	NUM = 1
	EOF = 2

class Token(object):
	def __init__(self, kind, next_token, string):
		self.kind = kind
		self.next = next_token
		self.val = None
		self.str = string

	def __str__(self):
		return self.str
	
	@classmethod
	def new_token(cls, kind, cur, string):
		tok = Token(kind, None, string)
		cur.next = tok
		return tok

class Lexer(object):
	def __init__(self, line):
		self.token = Lexer.tokenize(line)

	def __str__(self):
		return self.token.str 

	@classmethod
	def tokenize(cls, line):
		line = _re_all.findall(line)

		head = Token(None, None, None)
		cur = head

		while len(line):
			if _re_number.match(line[0]):
				t = line.pop(0)
				cur = Token.new_token(TokenKind.NUM, cur, t)
				cur.val = int(t)
			elif _re_operator.match(line[0]):
				cur = Token.new_token(TokenKind.RESERVED, cur, line.pop(0))
			else:	
				Lexer._error('Can not tokenize.')
		
		Token.new_token(TokenKind.EOF, cur, None)
		return head.next

	def consume(self, op):
		if self.token.kind != TokenKind.RESERVED or self.token.str != op:
			return False
		self.token = self.token.next
		return True
	
	def expect(self, op):
		if self.token.kind != TokenKind.RESERVED or self.token.str != op:
			Lexer._error(f'Token is not {op}.')
		self.token = self.token.next
	
	def expect_number(self):
		if self.token.kind != TokenKind.NUM:
			Lexer._error(f'Token is not number.')
		val = self.token.val
		self.token = self.token.next
		return val
	
	def at_eof(self):
		return self.token.kind == TokenKind.EOF
		
