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

/***************************************************************************
 * UTILITY FUNCTIONS
 ***************************************************************************/

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

/***************************************************************************
 * ALLOCATOR INTERFACES
 ***************************************************************************/

struct cop_alloc_iface {
	/* Context created by implementation. */
	void *ctx;

	/* Allocate some memory. The implementation of this API is never thread-
	 * safe unless explicitly defined by the allocator. If the return value is
	 * NULL, memory was unable to be allocated but the state of the allocator
	 * will remain unchanged. If align is zero, a default alignment will be
	 * used. If align is non-zero, it must be a power of two and specifies an
	 * address boundary to place the allocation on. */
	void *(*alloc)(struct cop_alloc_iface *a, size_t size, size_t align);
};

struct cop_salloc_iface {
	struct cop_alloc_iface iface;

	/* "save" retrieves the state of the stack-like allocator. The return
	 * value of save should not be treated as being meaningful by the caller.
	 * The implementation can never fail. The return value may be passed to
	 * the restore API to return to the state at which "save" as called. */
	size_t (*save)(struct cop_salloc_iface *a);
	void   (*restore)(struct cop_salloc_iface *a, size_t s);
};

/* The following functions are provided for convenience in using the allocator
 * interfaces. */
static COP_ATTR_ALWAYSINLINE void *cop_alloc(struct cop_alloc_iface *iface, size_t size, size_t align)
{
	assert(iface != NULL);
	assert(iface->alloc != NULL);
	return iface->alloc(iface, size, align);
}
static COP_ATTR_ALWAYSINLINE void *cop_salloc(struct cop_salloc_iface *iface, size_t size, size_t align)
{
	assert(iface != NULL);
	return cop_alloc(&(iface->iface), size, align);
}
static COP_ATTR_ALWAYSINLINE size_t cop_salloc_save(struct cop_salloc_iface *iface)
{
	assert(iface != NULL);
	assert(iface->save != NULL);
	return iface->save(iface);
}
static COP_ATTR_ALWAYSINLINE void cop_salloc_restore(struct cop_salloc_iface *iface, size_t sz)
{
	assert(iface != NULL);
	assert(iface->restore != NULL);
	iface->restore(iface, sz);
}

/***************************************************************************
 * ALLOCATOR IMPLEMENTATIONS
 ***************************************************************************/

/* A temporaries group allocator is useful for when a bunch of things need to
 * be allocated then freed repeatedly. */
struct cop_alloc_grp_temps;

/* Initialise a group temporaries allocator. The interface which should be
 * used to make allocations will be placed in iface. The function returns zero
 * on success or non-zero if memory was exhausted. */
int cop_alloc_grp_temps_init(struct cop_alloc_grp_temps *gat, struct cop_salloc_iface *iface, size_t initial_sz, size_t max_grow, size_t default_align);

/* Free all memory associated with a group allocator. */
void cop_alloc_grp_temps_free(struct cop_alloc_grp_temps *gat);

struct cop_alloc_virtual;

int cop_alloc_virtual_init(struct cop_alloc_virtual *s, struct cop_salloc_iface *iface, size_t reserve_sz, size_t default_align, size_t grow_sz);
void cop_alloc_virtual_free(struct cop_alloc_virtual *s);

/* ---------------------------------------------------------------------------
 * Private parts - defined so you can put them on the stack, not so you can
 * touch their bits. */

struct cop_alloc_virtual {
	size_t         reserve_sz;
	size_t         grow_sz;
	size_t         protect_sz;
	size_t         used_sz;
	size_t         default_align;
	unsigned char *base;
};

struct cop_alloc_grp_temps_buf {
	size_t                          size;
	size_t                          alloc_sz;
	struct cop_alloc_grp_temps_buf *prev;
};
struct cop_alloc_grp_temps {
	size_t                          default_align;
	size_t                          max_grow;
	size_t                          pre_head_size; /* sum of size members of all buffers before head */
	struct cop_alloc_grp_temps_buf *head;
};

#endif /* COP_ALLOC_H */
