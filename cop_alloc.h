/* Copyright (c) 2016 Nick Appleton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE. */

#ifndef COP_ALLOC_H
#define COP_ALLOC_H

#include "cop/cop_attributes.h"
#include <stddef.h>
#include <assert.h>

/* Returns the maximum amount of memory that can be locked to physical pages
 * by the process. A return value of zero means either:
 *   no pages can be locked
 *   the number of lockable pages cannot be queried or
 *   it is unknown how many pages can be locked.
 * A return value of SIZE_MAX indicates that there is no limit imposed on the
 * number of pages which may be locked. Otherwise, the return value is roughly
 * how many pages can be locked. */
size_t cop_memory_query_current_lockable();
size_t cop_memory_query_page_size();
size_t cop_memory_query_system_memory();

struct cop_alloc_iface {
	/* Allocate some memory. */
	void *(*alloc)(struct cop_alloc_iface *a, size_t size, size_t align);

	/* Context created by implementation. */
	void *ctx;
};

/* align == 0 indicates that the default alignment should be used. */
static COP_ATTR_ALWAYSINLINE void *cop_alloc(struct cop_alloc_iface *iface, size_t size, size_t align)
{
	assert(iface != NULL);
	assert(iface->alloc != NULL);
	return iface->alloc(iface, size, align);
}

/* A temporaries group allocator is useful for when a bunch of things need to
 * be allocated then freed repeatedly. */
struct cop_alloc_grp_temps;

/* Initialise a group temporaries allocator. The interface which should be
 * used to make allocations will be placed in iface. The function returns zero
 * on success or non-zero if memory was exhausted. */
int cop_alloc_grp_temps_init(struct cop_alloc_grp_temps *gat, struct cop_alloc_iface *iface, size_t initial_sz, size_t max_grow, size_t default_align);

/* Reset the tempoaray allocations object to a state where it has zero
 * allocations (it is undefined to use any allocated data that was obtained
 * using the interface). If try_realloc is non-zero and there were multiple
 * buffers allocated, the size of all of the allocations which were made will
 * be summed and we will try to create one buffer of this size. If try_realloc
 * is zero, or there is only one allocated buffer, or try_realloc was non-zero
 * and the reallocation operation failed, the largest allocation buffer will
 * be preserved and the reset will be discarded. */
void cop_alloc_grp_temps_reset(struct cop_alloc_grp_temps *gat, int try_realloc);

/* Free all memory associated with a group allocator. */
void cop_alloc_grp_temps_free(struct cop_alloc_grp_temps *gat);

struct aalloc;

void aalloc_push(struct aalloc *s);
void aalloc_merge_pop(struct aalloc *s);
void aalloc_pop(struct aalloc *s);
void aalloc_init(struct aalloc *s, size_t reserve_sz, size_t default_align, size_t grow_sz);
void aalloc_free(struct aalloc *s);
void *aalloc_alloc(struct aalloc *s, size_t size);
void *aalloc_align_alloc(struct aalloc *s, size_t size, size_t align);

/* ---------------------------------------------------------------------------
 * Private parts - defined so you can put them on the stack, not so you can
 * touch their bits. */

struct aalloc {
	size_t         reserve_sz;
	size_t         grow_sz;
	size_t         protect_sz;
	size_t         default_align;
	unsigned char *base;
	unsigned       sp;
	size_t         stack[32];
};

struct cop_alloc_grp_temps_buf {
	size_t                          size;
	size_t                          alloc_sz;

	struct cop_alloc_grp_temps_buf *next;
};
struct cop_alloc_grp_temps {
	size_t                          default_align;
	size_t                          max_grow;
	struct cop_alloc_grp_temps_buf *first;
};

#endif /* COP_ALLOC_H */
