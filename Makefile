CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter -g3 -std=gnu23

regex: regex.c arena.h

test: regex
	./regex "plead" "(dead)*"

clean:
	rm -f regex
