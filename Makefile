all:
	cc -std=c99 -Wall lispc.c libs/mpc.c -ledit -lm -o lispc
	./lispc standard_lib.lisp

clean:
	rm -f lispc parser

.PHONY: parser
parser:
	cc -std=c11 -Wall parser.c -g parser
	./parser