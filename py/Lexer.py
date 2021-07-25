import re
from sys import stderr
from enum import Enum, auto

_p_number = r'\d+'
_p_operator = r'[\+\-\*\/]'
_p_parenthesis = r'[\(\)]'
_p_equality = r'[\=\!]\='
_p_relational = r'[\<\>]\=?'
_p_ident = r'[a-z]'
_p_assign = r'\='
_p_semicolon = r'\;'

_re_number = re.compile(_p_number)
_re_operator = re.compile(_p_operator)
_re_parenthesis = re.compile(_p_parenthesis)
_re_equality = re.compile(_p_equality)
_re_relational = re.compile(_p_relational)
_re_ident = re.compile(_p_ident)
_re_assign = re.compile(_p_assign)
_re_semicolon = re.compile(_p_semicolon)

_re_all = re.compile( \
	_p_number + '|' + \
	_p_operator + '|' + \
	_p_parenthesis + '|' + \
	_p_equality + '|' + \
	_p_relational + '|' + \
	_p_ident + '|' + \
	_p_assign + '|' + \
	_p_semicolon
)

class TokenKind(Enum):
	RESERVED = 0
	IDENT = auto()
	NUM = auto()
	EOF = auto()

class Token(object):
	def __init__(self, kind, next_token, string):
		self.kind = kind
		self.next = next_token
		self.val = None
		self.str = string
		self.len =len(string) if string else 0 

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
			elif _re_operator.match(line[0]) or \
					_re_parenthesis.match(line[0]) or \
					_re_equality.match(line[0]) or \
					_re_relational.match(line[0]) or \
					_re_assign.match(line[0]) or \
					_re_semicolon.match(line[0]):
				cur = Token.new_token(TokenKind.RESERVED, cur, line.pop(0))
			elif _re_ident.match(line[0]):
				cur = Token.new_token(TokenKind.IDENT, cur, line.pop(0))
			else:	
				Lexer._error(f'Can not tokenize {line[0]}.')
		
		Token.new_token(TokenKind.EOF, cur, None)
		return head.next

	def consume(self, op):
		if self.token.kind != TokenKind.RESERVED or self.token.len != len(op) or self.token.str != op:
			return False
		self.token = self.token.next
		return True

	def consume_ident(self):
		if self.token.kind != TokenKind.IDENT:
			return False
		tok = self.token
		self.token = self.token.next
		return tok
	
	def expect(self, op):
		if self.token.kind != TokenKind.RESERVED or self.token.len != len(op) or self.token.str != op:
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

	@classmethod
	def _error(cls, msg):
		print(msg, file=stderr)
		exit(1)
