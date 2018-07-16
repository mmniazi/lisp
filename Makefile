all:
	cc -std=c99 -Wall lispc.c libs/mpc.c -ledit -lm -o lispc
	./lispc standard_lib.lisp

clean:
	rm lispc