all:
	cc -std=c99 -Wall parsing.c libs/mpc.c -ledit -lm -o parsing
	./parsing

clean:
	rm parsing