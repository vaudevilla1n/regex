CC = cc
CFLAGS = -Wall -Wextra -Wpedantic -Wno-unused-parameter -O3 -std=gnu23 -DARENA_IMPL

regex: regex.c arena.h

test: regex
	time ./regex "plead" "(dead)*"
	@echo
	time ./regex "lo" "los"
	@echo
	time ./regex "loll" "(lo)*l+"
	@echo
	time ./regex "hi" "hi|lo"
	@echo
	time ./regex "aaab" "a+b"
	@echo
	time ./regex "abasdas" ".+s"
	@echo

clean:
	rm -f regex
