#ifndef ARENA_H
#define ARENA_H

#include <stdint.h>
#include <stddef.h>

enum {
	SOFTFAIL = 1,
	NOZERO = 2,
};

typedef struct {
	uint8_t *init;	
	uint8_t *head;	
	uint8_t *tail;	
} arena_t ;

arena_t arena_init(ptrdiff_t cap);
void arena_deinit(arena_t *);

__attribute((malloc, alloc_size(2, 3), alloc_align(4)))
void *aalloc(arena_t *, ptrdiff_t count, ptrdiff_t size,
		ptrdiff_t align, int flags);
	
#define new(a, n, t, f)	(t *)aalloc(a, n, sizeof(t), _Alignof(t), f)

#endif /* ARENA_H */

#ifdef ARENA_IMPL
#include <stdlib.h>
#include <string.h>

arena_t arena_init(ptrdiff_t cap) {
	arena_t a = { 0 };

	a.init = calloc(cap, sizeof(a.head));
	a.head = a.init;
	a.tail = a.head ? a.head + cap : 0;

	return a;
}

void arena_deinit(arena_t *a) {
	free(a->init);

	*a = (arena_t) { 0 };
}

void *aalloc(arena_t *a, ptrdiff_t count, ptrdiff_t size,
		ptrdiff_t align, int flags) {
	const ptrdiff_t padding = -(uintptr_t)a->head & (align - 1);
	const ptrdiff_t available = a->tail - a->head - padding;

	if (available <= 0 || count > available/size) {
		if (flags & SOFTFAIL)
			return NULL;

		abort();
	}

	uint8_t *mem = a->head + padding;
	a->head += padding + count * size;
	return (flags & NOZERO) ? mem : memset(mem, 0, count * size);
}
#endif /* ARENA_IMPL */
