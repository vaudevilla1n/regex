CFLAGS = -Wall -Wextra -Wpedantic -ggdb -std=gnu23 -DARENA_IMPL

regex: regex.c arena.h

.PHONY: test clean
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
	rm -f ./regex
