CFLAGS = -Wall -Wextra -Wpedantic -ggdb -std=gnu23 -DARENA_IMPL

regex: regex.c arena.h

.PHONY: test clean
test: regex
	time ./regex "(dead)*" "plead"
	@echo
	time ./regex "los" "lo"
	@echo
	time ./regex "(lo)*l+" "loll"
	@echo
	time ./regex "hi|lo" "hi"
	@echo
	time ./regex "a+b" "aaab"
	@echo
	time ./regex ".+s" "abasdas"
	@echo

clean:
	rm -f ./regex
