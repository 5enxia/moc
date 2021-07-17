from sys import argv

if __name__ == '__main__':
	argc = len(argv)
	if argc != 2:
		print("引数の個数が正しくありません")
		exit(1)
	
	print(".intel_syntax noprefix")
	print(".globl main")
	print("main:")
	print("\tmov rax, {}".format(argv[1]))
	print("\tret")