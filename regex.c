#include "arena.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef ptrdiff_t isize_t;

#define UNUSED(x)		(void)x
#define TODO(x)			do { fprintf(stderr, "todo: %s\n", #x); abort(); } while (0)
#define UNREACHABLE(x)		do { fprintf(stderr, "unreachable: %s\n", #x); abort(); } while (0)

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
	alternaton	:= term ('|' term)*
	concat		:= term (term)*
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

typedef enum {
	TERR,
	TEOF,
	TOPAREN, TCPAREN,
	TOBRACK, TCBRACK,
	TSTAR, TDOT, TPLUS, TQUESTION, TBAR, TDOLLAR, TCARET, TSLASH,
	TCHAR,
} token_type_t;

const char *token_type_string(const token_type_t type) {
	switch (type) {
	case TERR:	return "TERR";
	case TEOF:	return "TEOF";
	case TOPAREN:	return "TOPAREN";
	case TCPAREN:	return "TCPAREN";
	case TOBRACK:	return "TOBRACK";
	case TCBRACK:	return "TCBRACK";
	case TSTAR:	return "TSTAR";
	case TDOT:	return "TDOT";
	case TPLUS:	return "TPLUS";
	case TQUESTION:	return "TQUESTION";
	case TBAR:	return "TBAR";
	case TDOLLAR:	return "TDOLLAR";
	case TCARET:	return "TCARET";
	case TSLASH:	return "TSLASH";
	case TCHAR:	return "TCHAR";
	default: UNREACHABLE(token_type_string);
	}
}

typedef struct {
	const char *src; 
	size_t srclen;

	size_t pos;

	token_type_t token;
	const char *token_text;
	size_t token_textlen;
} lexer_t;

lexer_t lexer_new(const char *src, const size_t srclen) {
	return (lexer_t) {
		.src = src,
		.srclen = srclen,
	};
}

char lexer_peek_char(lexer_t *l) {
	return (l->pos < l->srclen) ? l->src[l->pos] : 0;
}

char lexer_next_char(lexer_t *l) {
	return (l->pos < l->srclen) ? l->src[l->pos++] : 0;
}

bool lexer_next(lexer_t *l) {
	while (isspace(lexer_peek_char(l)))
		lexer_next_char(l);

	/*
	TERR,
	TEOF,
	TOPAREN, TCPAREN,
	TOBRACK, TCBRACK,
	TSTAR, TDOT, TPLUS, TQUESTION, TBAR, TDOLLAR, TCARET, TSLASH
	TCHAR,
	*/

	if (!lexer_peek_char(l))
		return false;
	
	l->token_text = l->src + l->pos;
	l->token_textlen = 1;
	
	switch (lexer_next_char(l)) {
	case '(': l->token = TOPAREN; break;
	case ')': l->token = TCPAREN; break;
	case '[': l->token = TOBRACK; break;
	case ']': l->token = TCBRACK; break;
	case '*': l->token = TSTAR; break;
	case '.': l->token = TDOT; break;
	case '+': l->token = TPLUS; break;
	case '?': l->token = TQUESTION; break;
	case '|': l->token = TBAR; break;
	case '$': l->token = TDOLLAR; break;
	case '^': l->token = TCARET; break;
	case '\\': l->token = TSLASH; break;
	default: l->token = TCHAR; break;
	}

	return true;
}

void regex_tokenize(const char *regex) {
	lexer_t l = lexer_new(regex, strlen(regex));

	while (lexer_next(&l))
		printf("%s: %.*s\n", token_type_string(l.token), (int)l.token_textlen, l.token_text);
}

void regex_match(const char *regex, const char *s) {
	regex_tokenize(regex);

	UNUSED(regex);
	UNUSED(s);
	//TODO(regex_match);
}

void regex_print(const char *regex, const size_t n) {
	regex_tokenize(regex);

	UNUSED(regex);
	UNUSED(n);
	//TODO(regex_print);
}

void usage(void) {
	fputs("usage: ./regex [<regex> <str> (match)]\n"
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

		const char *regex = argv[optind];
		const char *str = argv[optind + 1];

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
