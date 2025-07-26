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

int regex_valid(const char *ex, const char *ex_end) {
	const char *e;
	for (e = ex; e < ex_end; e++) {
		switch (*e) {
		case '(': {
			const char *end = strchr(e, ')');
			if (!end || end >= ex_end)
				return 0;

			if (!regex_valid(e + 1, end))
				return 0;

			e = end;
		} break;

		case '[': {
			const char *end = strchr(e, ']');
			if (!end || end >= ex_end)
				return 0;

			if (!regex_valid(e + 1, end))
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
			/* binds to nothing if at beginning */
			if (e == ex)
				return 0;

			/* cant bind to another specifier */
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

void usage(void) {
	fputs("usage: ./regex <str> <regex>\n", stderr);
	exit(1);
}

int main(int argc, char **argv) {
	if (argc != 3)
		usage();
	
	const char *ex = argv[2];
	const size_t exlen = strlen(ex);

	if (!regex_valid(ex, ex + exlen)) {
		err("invalid regex");
		usage();
	}

	regex_log(ex, ex + exlen);
	putchar('\n');
	
	puts("false");
}
