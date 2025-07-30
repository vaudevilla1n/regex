CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter -g3 -std=gnu23

regex: regex.c arena.h

test: regex
	time ./regex "plead" "(dead)*"
	time ./regex "lo" "los"
	time ./regex "loll" "(lo)*l+"
	time ./regex "hi" "hi|lo"

clean:
	rm -f regex
