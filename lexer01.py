from sys import stdin

i = 0
while True:
	ch = stdin.read(1)
	i += 1
	if ch == '\n':
		break
	if ch == ' ':
		continue
	if ch.isalpha():
		print("ID:", ch, end='')
		while True:
			ch = stdin.read(1)
			i += 1
			if ch.isalnum():
				print(ch, end='')
			else:
				stdin.seek(i)
				break	
		print()
	elif ch.isdigit():
		print("NM:", ch)
		while True:
			ch = stdin.read(1)
			i += 1
			if ch.isdigit():
				print(ch, end='')
			else:
				stdin.seek(i)
				break	
		print()
	else:
		print("SY:", ch)
