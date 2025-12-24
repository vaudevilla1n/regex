#include "arena.h"
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>

typedef ptrdiff_t isize_t;

#define UNUSED(x)	(void)x
#define TODO(x)		do { fprintf(stderr, "todo: %s\n", #x); abort(); } while (0)

#define err(fmt, ...) \
	do { \
		fprintf(stderr, fmt, ##__VA_ARGS__); \
		fprintf(stderr, "\n"); \
	} while (0)

#define perr(fmt, ...) \
	do { \
		fprintf(stderr, fmt, ##__VA_ARGS__); \
		fprintf(stderr, ": %s\n", strerror(errno)); \
	} while (0)

/*
	abc[d-l]+.*
	grammar

	expr		:= ('^' | '$') capture (capture)*
	capture		:= concat | alternation | term
	concat		:= term (term)*
	alternaton	:= term ('|' term)*
	term		:= group | literal quant | literal | bracket
	group		:= '(' capture ')'
	bracket		:= '[' (literal | range)+ ']' | '[' '^' (literal | range)+ ']'
	range		:= literal '-' literal
	literal		:= any ascii character
	quant		:= zero or more | once or more | zero or once | '{' n | min, | ,max | min,max '}'
	zero or more	:= '*'
	once or more	:= '+'
	zero or once	:= '?'
	wildcard	:= '.'
	epsilon		:= '' (empty string)

	ast
*/

void regex_match(const char *regex, const char *s) {
	UNUSED(regex);
	UNUSED(s);
	TODO(regex_match);
}

void regex_print(const char *regex, const size_t n) {
	UNUSED(regex);
	UNUSED(n);
	TODO(regex_print);
}

void usage(void) {
	fputs("usage: ./regex [<str> <regex> (match)]\n"
			"               [-p <num> <regex> (print <num> matching strings)]\n", stderr);
	exit(1);
}

void arena_diagnostics(arena_t *a) {
	const isize_t used_bytes = a->head - a->init;
	const isize_t total_bytes = a->tail - a->init;
	const double percentage = ((double)used_bytes / (double)total_bytes) * 100;
	printf("\n%ld/%ld bytes used (%.2f%%)\n", used_bytes, total_bytes, percentage);
}

#define MAX_ALLOC	1024

int main(int argc, char **argv) {
	arena_t a = arena_init(MAX_ALLOC);

	opterr = 0;
	switch (getopt(argc, argv, "p:")) {
	// no option specified
	case -1: {
		// matching requires two arguments
		if (argc - optind != 2)
			usage();

		const char *str = argv[optind];
		const char *regex = argv[optind + 1];

		regex_match(regex, str);
	} break;

	case 'p': {
		// printing requires one non-flag argument
		if (argc - optind != 1)
			usage();

		errno = 0;
		char *end;
		const size_t nstr = strtoull(optarg, &end, 10);

		if (errno || *end || !*optarg) {
			err("invalid integer: \"%s\"\n", optarg);
			usage();
		}

		const char *regex = argv[optind];

		regex_print(regex, nstr);
	} break;

	default: usage();
	}

	arena_diagnostics(&a);
	arena_deinit(&a);
}
