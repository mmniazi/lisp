all: clean
	cc -std=c99 -Wall lisp.c -ledit -o lisp
	chmod +x lisp

clean:
	rm -f lisp
