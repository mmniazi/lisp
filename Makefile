all: clean
	cc -std=c99 -Wall repl.c -ledit -o repl
	./repl standard_lib.lisp

clean:
	rm -f repl