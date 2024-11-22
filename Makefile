all:
	gcc -Ofast kirin.c -o kirin

debug:
	gcc kirin.c -o kirin
