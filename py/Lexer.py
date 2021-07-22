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
	token = None

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
		
		Token.new_token(TokenKind.EOF, cur, 'EOF')
		return head.next

	@classmethod
	def _error(cls, msg):
		print(msg, file=stderr)
		exit(1)

	@classmethod
	def consume(cls, op):
		if Lexer.token.kind != TokenKind.RESERVED or Lexer.token.str != op:
			return False
		Lexer.token = Lexer.token.next
		return True
	
	@classmethod
	def expect(cls, op):
		if Lexer.token.kind != TokenKind.RESERVED or Lexer.token.str != op:
			Lexer._error(f'Token is not {op}.')
		Lexer.token = Lexer.token.next
	
	@classmethod
	def expect_number(cls):
		if Lexer.token.kind != TokenKind.NUM:
			Lexer._error(f'Token is not number.')
		val = Lexer.token.val
		Lexer.token = Lexer.token.next
		return val
	
	@classmethod
	def at_eof(cls):
		return Lexer.token.kind == TokenKind.EOF
		
