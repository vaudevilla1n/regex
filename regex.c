#define ARENA_IMPL
#include "arena.h"
#include <assert.h>
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
	REGEX_ZERO_ONCE = 004, // '?'

	REGEX_INTERNAL_SUCCESS = 001,
};

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

int regex_valid(const char *beg, const char *end, int type) {
	const char *e;
	for (e = beg; e < end; e++) {
		switch (*e) {
		case '(': {
			const char *l = strchr(e, ')');
			if (!l || l >= end)
				return 0;

			if (!regex_valid(e + 1, l, REGEX_CONCAT))
				return 0;

			e = l;
		} break;

		case '[': {
			const char *l = strchr(e, ']');
			if (!l || l >= end)
				return 0;

			if (!regex_valid(e + 1, l, REGEX_CONCAT_OR))
				return 0;

			e = l;
		} break;

		case '\\': {
			/* binds to nothing if at l */
			if (e + 1 == end)
				return 0;
			e++;
		} break;

		case '?':
		case '+':
		case '*': {
			/* not allowed in [ ] group */
			if (type == REGEX_CONCAT_OR)
				return 0;

			/* binds to nothing if at beginning */
			if (e == beg)
				return 0;

			/* cant bind to another multiplier */
			if ((e[-1] == '+' || e[-1] == '*' || e[-1] == '?')
					&& ((beg + 2 > e) || e[-2] != '\\'))
				return 0;
		} break;
	
		case '-': {
			/* binds to nothing if at beginning or l */
			if (e == beg || e + 1 == end)
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
			/* binds to nothing if at beginning or l */
			if (e == beg || e + 1 == end)
				return 0;
		} break;

		}
	}

	return 1;
}

struct regex_token *regex_parse(const char *beg, const char *end, struct arena *a) {
	struct regex_token *head = new(a, 1, struct regex_token, 0);
	head->type = REGEX_CONCAT;

	struct regex_token *tail = head->val.concat;

	const char *e;
	for (e = beg; e < end; e++) {
		struct regex_token *t;

		switch (*e) {
		case '(': {
			const char *l = strchr(e, ')');

			t = regex_parse(e + 1, l, a);

			e = l;
		} break;

		case '[': {
			const char *l = strchr(e, ']');

			t = regex_parse(e + 1, l, a);
			t->type = REGEX_CONCAT_OR;

			e = l;
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
		case '?':
			t->mul = REGEX_ZERO_ONCE;
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

		case REGEX_ZERO_ONCE:
			puts(" zero or once");
			break;

		default:
			putchar('\n');
		}

		t = t->next;
	}
}

const char *regex_match_token(struct regex_token *regex, const char *beg, const char *end);

/*
	ISSUE!
	must end on REGEX_OR
	bad check at end of regex_match_token
		if (ret == end && regex->next)
	
	RESOLVED!
*/

static inline const char *regex_match_byte(struct regex_token *t, const char *beg, const char *end) {
	return (beg < end && *beg == t->val.byte) ? beg + 1 : NULL;
}

static inline const char *regex_match_any(struct regex_token *t, const char *beg, const char *end) {
	return (beg < end) ? beg + 1 : NULL;
}

static inline const char *regex_match_range(struct regex_token *t, const char *beg, const char *end) {
	return (beg < end && t->val.range.start <= *beg && *beg <= t->val.range.end) ? beg + 1 : NULL;
}

static inline const char *regex_match_concat(struct regex_token *t, const char *beg, const char *end) {
	const char *e = beg;
	for (t = t->val.concat; t; t = t->next) {
		const char *n = regex_match_token(t, e, end);

		if (!n) {
			/* check for REGEX_OR ('|') to see if we can continue if nothing has been matched yet */
			struct regex_token *tor = t->next;
			for (; tor; tor = tor->next)
				if (tor->type == REGEX_OR)
					break;
			if (tor) {
				t = tor;
				continue;
			} else  {
				return NULL;
			}
		} else if (t->next && t->next->type == REGEX_OR) {
			/* one expression that is binded to a REGEX_OR matched, so just return */
			return n;
		}

		e = n;	
	}

	return e;
}

static inline const char *regex_match_concat_or(struct regex_token *t, const char *beg, const char *end) {
	for (t = t->val.concat; t; t = t->next) {
		const char *e = regex_match_token(t, beg, end);

		if (e)
			return e;
	}

	return NULL;
}

/* im not typing out this function pointer */
typedef typeof(regex_match_byte) regex_matcher_func;

const char *regex_match_token(struct regex_token *regex, const char *beg, const char *end) {
	assert(regex && beg && end && "invalid regex_match_token args");

	regex_matcher_func *matcher = NULL;

	switch (regex->type) {
	case REGEX_BYTE: {
		matcher = regex_match_byte;
	} break;

	case REGEX_MATCH_ANY: {
		matcher = regex_match_any;
	} break;

	case REGEX_RANGE: {
		matcher = regex_match_range;
	} break;

	case REGEX_CONCAT: {
		matcher = regex_match_concat;
	} break;

	case REGEX_CONCAT_OR: {
		matcher = regex_match_concat_or;
	} break;
	
	default: {
		fputs("unknown regex_token type\n", stderr);
		abort();
	} break;

	}

	switch (regex->mul) {
	case REGEX_ZERO_ONCE: {
		const char *e = matcher(regex, beg, end);

		return (e) ? e : beg;
	} break;

	case REGEX_ZERO_MORE: {
		/*
		   make sure all expressions are evaluated

		   tbh not sure why this helps but it does and i dont feel
		   like thinking abt it
		*/
		struct regex_token g = {
			.type = REGEX_CONCAT,
			.val = {
				.concat = regex->next,
			},
		};
		while (beg < end) {
			if (regex->next && (regex_match_token(&g, beg, end) == end)) {
				// nothing more to do if the path was already found
				regex->next = NULL;
				return end;
			}
			const char *e;
			if ((e = matcher(regex, beg, end)))
				beg = e;
			else
				break;
		}

		return beg;
	} break;

	case REGEX_ONCE_MORE: {
		const char *e = beg;
		struct regex_token g = {
			.type = REGEX_CONCAT,
			.val = {
				.concat = regex->next,
			},
		};
		while (e < end) {
			const char *n = matcher(regex, e, end);
			if (n)
				e = n;
			else
				break;

			if (regex->next && (regex_match_token(&g, e, end) == end)) {
				regex->next = NULL;
				return end;
			}
		}

		return (e == beg) ? NULL : e;
	} break;

	default: {
		return matcher(regex, beg, end);
	} break;

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
	
	const char *s = argv[1];
	const size_t slen = strlen(s);

	const char *ex = argv[2];
	const size_t exlen = strlen(ex);

	if (!regex_valid(ex, ex + exlen, 0)) {
		err("invalid regex");
		usage();
	}

	struct regex_token *regex = regex_parse(ex, ex + exlen, &a);

	const char *ret = regex_match_token(regex, s, s + slen);
	puts((ret == (s + slen)) ? "true" : "false");

	printf("\n%ld/%ld bytes used\n", a.head - a.init, a.tail - a.init);
}
