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

#include "cop_attributes.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct aalloc;

static COP_ATTR_UNUSED void aalloc_push(struct aalloc *s);
static COP_ATTR_UNUSED void aalloc_merge_pop(struct aalloc *s);
static COP_ATTR_UNUSED void aalloc_pop(struct aalloc *s);
static COP_ATTR_UNUSED void aalloc_init(struct aalloc *s, size_t default_align, size_t min_alloc);
static COP_ATTR_UNUSED void aalloc_free(struct aalloc *s);
static COP_ATTR_UNUSED void *aalloc_alloc(struct aalloc *s, size_t size);
static COP_ATTR_UNUSED void *aalloc_align_alloc(struct aalloc *s, size_t size, size_t align);

struct aalloc_rec {
	size_t             sz;
	unsigned char     *buf;
	struct aalloc_rec *next;
};

struct aalloc {
	struct aalloc_rec     *top;
	size_t                 top_pos;
	size_t                 default_align;
	size_t                 min_alloc;
	unsigned               sp;
	struct aalloc_rec     *bottom;
	struct {
		struct aalloc_rec *top;
		size_t             top_pos;
	} stack[32];
};

static struct aalloc_rec *aalloc_rec_init(size_t initial_size)
{
	struct aalloc_rec *rec = malloc(sizeof(*rec) + initial_size);
	if (rec != NULL) {
		rec->sz   = initial_size;
		rec->buf  = (unsigned char *)(rec + 1);
		rec->next = NULL;
	}
	return rec;
}

static size_t aalloc_alignoffset(size_t val, size_t align_mask)
{
	return (align_mask + 1 - (val & align_mask)) & align_mask;
}

static void *aalloc_align_alloc(struct aalloc *s, size_t size, size_t align)
{
	size_t req_alloc;

	assert(align && (((align - 1) & align) == 0) && "align must be positive and a power of two");

	/* If there are already allocations and we can fit in this guy... */
	while (s->top != NULL) {
		size_t offset = aalloc_alignoffset((size_t)(s->top->buf + s->top_pos), align - 1);
		if (s->top_pos + offset + size <= s->top->sz) {
			void *r;
			s->top_pos += offset;
			r = s->top->buf + s->top_pos;
			s->top_pos += size;
			return r;
		}
		if (s->top->next == NULL)
			break;
		s->top     = s->top->next;
		s->top_pos = 0;
	}

	/* There are either no allocations or there is not enough space in the top
	 * allocation. */
	req_alloc = 4 * size + align - 1;
	if (s->min_alloc > req_alloc) {
		req_alloc = s->min_alloc;
	}
	if (s->top != NULL) {
		s->top->next = aalloc_rec_init(req_alloc);
		s->top = s->top->next;
	} else {
		s->top = aalloc_rec_init(req_alloc);
	}

	s->top_pos = 0;

	if (s->top != NULL) {
		size_t offset = aalloc_alignoffset((size_t)(s->top->buf + s->top_pos), align - 1);
		void *r;
		assert(s->top_pos + offset + size <= s->top->sz);
		s->top_pos += offset;
		r = s->top->buf + s->top_pos;
		s->top_pos += size;
		return r;
	}

	return NULL;
}

static void *aalloc_alloc(struct aalloc *s, size_t size)
{
	return aalloc_align_alloc(s, size, s->default_align);
}

static void aalloc_init(struct aalloc *s, size_t default_align, size_t min_alloc)
{
	assert(default_align && (((default_align - 1) & default_align) == 0) && "default_align must be positive and a power of two");
	s->top           = NULL;
	s->top_pos       = 0;
	s->default_align = default_align;
	s->min_alloc     = min_alloc;
	s->sp            = 0;
	s->bottom        = NULL;
}

static void aalloc_free(struct aalloc *s)
{
	while (s->bottom != NULL) {
		s->top = s->bottom;
		s->bottom = s->bottom->next;
		free(s->top);
	}
}

static void aalloc_push(struct aalloc *s)
{
	s->stack[s->sp].top     = s->top;
	s->stack[s->sp].top_pos = s->top_pos;
	s->sp++;
}

static void aalloc_merge_pop(struct aalloc *s)
{
	s->sp--;
}

static void aalloc_pop(struct aalloc *s)
{
	s->sp--;
	s->top     = s->stack[s->sp].top;
	s->top_pos = s->stack[s->sp].top_pos;
}

#endif /* COP_ALLOC_H */
