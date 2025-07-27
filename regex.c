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
	REGEX_CONCAT = 001, // concat of multiple types, match all "()"
	REGEX_CONCAT_OR = 002, // concat of multiple types, match any "[]"
	REGEX_MATCH_ANY = 004, // '.'
	REGEX_BYTE = 006, // character
	REGEX_OR = 010, // '|'
	REGEX_RANGE = 011, // x-y

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

			if (!regex_valid(e + 1, end, REGEX_CONCAT))
				return 0;

			e = end;
		} break;

		case '[': {
			const char *end = strchr(e, ']');
			if (!end || end >= ex_end)
				return 0;

			if (!regex_valid(e + 1, end, REGEX_CONCAT_OR))
				return 0;

			e = end;
		} break;

		case '\\': {
			/* binds to nothing if at end */
			if (e + 1 == ex_end)
				return 0;
			e++;
		} break;

		case '+':
		case '*': {
			/* not allowed in [ ] group */
			if (type == REGEX_CONCAT_OR)
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

struct regex_range {
	char start;
	char end;
};

union regex_val {
	char byte;
	struct regex_token *concat;
	struct regex_range range;
};

struct regex_token {
	uint8_t type;
	uint8_t mul;
	union regex_val val;
	struct regex_token *next;
};

struct regex_token *regex_parse(const char *ex, const char *ex_end, struct arena *a) {
	struct regex_token *head = new(a, 1, struct regex_token, 0);
	head->type = REGEX_CONCAT;

	struct regex_token *tail = head->val.concat;

	const char *e;
	for (e = ex; e < ex_end; e++) {
		struct regex_token *t;

		switch (*e) {
		case '(': {
			const char *end = strchr(e, ')');

			t = regex_parse(e + 1, end, a);

			e = end;
		} break;

		case '[': {
			const char *end = strchr(e, ']');

			t = regex_parse(e + 1, end, a);
			t->type = REGEX_CONCAT_OR;

			e = end;
		} break;

		case '|': {
			t = new(a, 1, struct regex_token, 0);
			t->type = REGEX_OR;
		} break;

		case '.': {
			t = new(a, 1, struct regex_token, 0);
			t->type = REGEX_MATCH_ANY;
		} break;

			  /*
			     TODO stop being a dumbass

			     create a REGEX_RANGE type so that u dont waste memory
			     expanding this
			   */

		case '\\':
			e++;
			[[fallthrough]];
		default: {
			t = new(a, 1, struct regex_token, 0);
			if (e[1] == '-') {
				char l = e[0];
				char r = e[2];
				if (l > r) {
					const char tmp = l;
					l = r;
					r = tmp;
				}

				t->type = REGEX_RANGE;
				t->val.range.start = l;
				t->val.range.end = r;

				e += 2;
			} else {
				t->type = REGEX_BYTE;
				t->val.byte = *e;
			}
		} break;
		}

		switch (e[1]) {
		case '*':
			t->mul = REGEX_ZERO_MORE;
			e++;
			break;
		case '+':
			t->mul = REGEX_ONCE_MORE;
			e++;
			break;
		default:
			t->mul = REGEX_ONCE;
			break;
		}

		if (tail)
			tail->next = t;
		else
			head->val.concat = t;
		tail = t;
	}

	return head;
}

void regex_log(const struct regex_token *t) {
	while (t) {
		switch (t->type) {
		case REGEX_BYTE: {
			printf("'%c'", t->val.byte);
		} break;

		case REGEX_RANGE: {
			printf("range(%c -> %c)", t->val.range.start, t->val.range.end);
		} break;

		case REGEX_CONCAT: {
			puts("concat (match all) {");
			regex_log(t->val.concat);
			putchar('}');
		} break;

		case REGEX_CONCAT_OR: {
			puts("concat (match any) {");
			regex_log(t->val.concat);
			putchar('}');
		} break;

		case REGEX_OR:
			puts("or");
			break;

		case REGEX_MATCH_ANY:
			printf("match any");
			break;
		}

		switch (t->mul) {
		case REGEX_ONCE_MORE:
			puts(" once or more");
			break;

		case REGEX_ZERO_MORE:
			puts(" zero or more");
			break;

		default:
			putchar('\n');
		}

		t = t->next;
	}
}

void usage(void) {
	fputs("usage: ./regex <str> <regex>\n", stderr);
	exit(1);
}

int main(int argc, char **argv) {
	if (argc != 3)
		usage();
	
	struct arena a = arena_init(4096);
	
	const char *ex = argv[2];
	const size_t exlen = strlen(ex);

	if (!regex_valid(ex, ex + exlen, 0)) {
		err("invalid regex");
		usage();
	}

	struct regex_token *regex = regex_parse(ex, ex + exlen, &a);
	regex_log(regex);
	
	printf("\n%ld bytes used\n", a.head - a.init);

	puts("false");
}
