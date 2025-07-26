#define ARENA_IMPL
#include "arena.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define err(msg)	fprintf(stderr, "%s\n", msg)
#define errf(fmt, ...)	fprintf(stderr, fmt "\n", __VA_ARGS__)

int alpha(const char c) {
	const int p = ((c & 0x3f) & (0x1f));
	return (c & 0x40) && 1 <= p && p <= 26;
}

int numeric(const char c) {
	return '0' <= c && c <= '9';
}

enum {
	REGEX_GROUP_ALL = 001, // ()
	REGEX_GROUP_ANY = 002, // []
	REGEX_MATCH_ANY = 004, // '.'
	REGEX_BYTE = 006, // character
	REGEX_STR = 010, // string of characters
	REGEX_OR = 020, // '|'
	REGEX_CONCAT = 040, // concat of multiple types

	REGEX_ONCE = 000, // default multiplier
	REGEX_ZERO_MORE = 001, // '*'
	REGEX_ONCE_MORE = 002, // '+'
};

int regex_valid(const char *ex, const char *ex_end, int type) {
	const char *e;
	for (e = ex; e < ex_end; e++) {
		switch (*e) {
		case '(': {
			const char *end = strchr(e, ')');
			if (!end || end >= ex_end)
				return 0;

			if (!regex_valid(e + 1, end, REGEX_GROUP_ALL))
				return 0;

			e = end;
		} break;

		case '[': {
			const char *end = strchr(e, ']');
			if (!end || end >= ex_end)
				return 0;

			if (!regex_valid(e + 1, end, REGEX_GROUP_ANY))
				return 0;

			e = end;
		} break;

		case '\\': {
			/* binds to nothing if at end */
			if (e + 1 == ex_end)
				return 0;
			ex++;
		} break;

		case '+':
		case '*': {
			/* not allowed in [ ] group */
			if (type == REGEX_GROUP_ANY)
				return 0;

			/* binds to nothing if at beginning */
			if (e == ex)
				return 0;

			/* cant bind to another multiplier */
			if ((e[-1] == '+' || e[-1] == '*')
					&& ((ex + 2 > e) || e[-2] != '\\'))
				return 0;
		} break;
	
		case '-': {
			/* binds to nothing if at beginning or end */
			if (e == ex || e + 1 == ex_end)
				return 0;

			/* must be between either two letters or numbers */
			const char f = e[-1];
			const char l = e[1];
			if (alpha(f) && alpha(l))
				break;
			else if (numeric(f) && numeric(l))
				break;
			else
				return 0;
		} break;

		case '|': {
			/* binds to nothing if at beginning or end */
			if (e == ex || e + 1 == ex_end)
				return 0;
		} break;

		}
	}

	return 1;
}

void regex_log(const char *ex, const char *ex_end) {
	const char *e;
	for (e = ex; e < ex_end; e++) {
		switch (*e) {
		case '(': {
			const char *g = e + 1;
			const char *ge = strchr(g, ')');

			puts("match all in group (");
			regex_log(g, ge);
			puts(")");

			e = ge;
		} continue;

		case '[': {
			const char *g = e + 1;
			const char *ge = strchr(g, ']');

			puts("match all in group [");
			regex_log(g, ge);
			puts("]");

			e = ge;
		} break;

		case '+': {
			puts("once or more");
		} break;

		case '*': {
			puts("zero or more");
		} break;

		case '.': {
			puts("match any character"); 
		} break;

		case '|': {
			puts("OR");
		} break;

		case '-': {
			puts("TO");
		} break;

		case '\\':
			e++;
			[[fallthrough]];
		default:
			printf("match char %c\n", *e);
			break;
		}

	}
}

/*
	TODO figure this shit out
	im thinking of having all tokens as linked lists
	i.e
		(eee+|lake)+abe =>
		(node(group_all, ONCE_MORE)->
			(node(concat)->
				node(str(ee))->
				node(byte(e, ONCE_MORE))
			)->
			node(str(lake))->
			NULL
		)->
		node(str(abe))->
		NULL
	this makes sense right????
*/

struct regex_token;

union regex_val {
	char byte;
	char *str;
	struct regex_token *group;
};

struct regex_token {
	int type;
	int mul;
	union regex_val val;
	size_t val_len;
	struct regex_token *next;
};

struct regex_token *__regex_eval(const char *ex, const char *ex_end, int type, struct arena *a) {
	puts("todo!");
	abort();
}

struct regex_token *regex_eval(const char *ex, const char *ex_end, struct arena *a) {
	struct regex_token *head = new(a, 1, struct regex_token, 0);
	*head = (struct regex_token) {
		.type = REGEX_CONCAT,
		.mul = REGEX_ONCE,
		.val = (union regex_val) { 0 },
		.val_len = 0,
		.next = NULL,
	};

	struct regex_token *t = head;

	const char *e;
	for (e = ex; e < ex_end; e++) {
		switch (*e) {
		case '(': {
			const char *g = e + 1;
			const char *ge = strchr(g, ')');

			t->next = __regex_eval(g, ge, REGEX_GROUP_ALL, a);
			t = t->next;

			e = ge;
		} break;

		case '[': {
			const char *g = e + 1;
			const char *ge = strchr(g, ']');

			t->next = __regex_eval(g, ge, REGEX_GROUP_ANY, a);
			t = t->next;

			e = ge;
		} break;

		default: {
			t->next = __regex_eval(e, ex_end, 0, a);
			t = t->next;
		} break;

		}
	}

	return head;
}

void usage(void) {
	fputs("usage: ./regex <str> <regex>\n", stderr);
	exit(1);
}

int main(int argc, char **argv) {
	if (argc != 3)
		usage();
	
	struct arena a = arena_init(1024);
	
	const char *ex = argv[2];
	const size_t exlen = strlen(ex);

	if (!regex_valid(ex, ex + exlen, 0)) {
		err("invalid regex");
		usage();
	}

	regex_log(ex, ex + exlen);
	putchar('\n');

	regex_eval(ex, ex + exlen, &a);
	
	puts("false");
}
