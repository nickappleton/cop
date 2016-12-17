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

#include "cop/cop_alloc.h"

#include <stdint.h> /* SIZE_MAX */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/mman.h>     /* mmap */
#include <sys/time.h>     /* getrlimit */
#include <sys/resource.h> /* getrlimit */
#endif
#ifdef __APPLE__
#include <sys/sysctl.h>   /* sysctl */
#include <sys/mman.h>     /* mmap */
#include <sys/time.h>     /* getrlimit */
#include <sys/resource.h> /* getrlimit */
#endif
#if _WIN32
#include <windows.h>
#endif

size_t cop_memory_query_page_size()
{
#ifdef __linux__
	long page_size = sysconf(_SC_PAGE_SIZE);
	return (page_size < 0) ? 0 : (size_t)page_size;
#elif __APPLE__
	int    name[2]  = {CTL_HW, HW_PAGESIZE};
	u_int  namelen  = sizeof(name) / sizeof(name[0]);
	size_t value = 0;
	size_t valuelen = sizeof(value);
	return (sysctl(name, namelen, &value, &valuelen, NULL, 0) == 0) ? value : 0;
#elif _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwPageSize;
#else
#error "implement me for your platform"
#endif
}

size_t cop_memory_query_system_memory()
{
#ifdef __linux__
	long page_size = sysconf(_SC_PAGE_SIZE);
	long npage = sysconf(_SC_PHYS_PAGES);
	return (page_size < 0 || npage < 0) ? 0 : ((size_t)npage * (size_t)page_size);
#elif __APPLE__
	int    name[2]  = {CTL_HW, HW_MEMSIZE};
	u_int  namelen  = sizeof(name) / sizeof(name[0]);
	size_t value = 0;
	size_t valuelen = sizeof(value);
	return (sysctl(name, namelen, &value, &valuelen, NULL, 0) == 0) ? value : 0;
#elif _WIN32
	MEMORYSTATUSEX memstatus;
	memstatus.dwLength = sizeof(memstatus);
	return (GlobalMemoryStatusEx(&memstatus)) ? (size_t)memstatus.ullTotalPhys : 0;
#else
#error "implement me for your platform"
#endif
}

size_t cop_memory_query_current_lockable()
{
#if defined(__linux__) || defined(__APPLE__)
	struct rlimit rlim;
	if (!getrlimit(RLIMIT_MEMLOCK, &rlim))
		return (rlim.rlim_cur == RLIM_INFINITY) ? SIZE_MAX : rlim.rlim_cur;
	return 0;
#elif _WIN32
	SIZE_T smin, smax;
	return (GetProcessWorkingSetSize(GetCurrentProcess(), &smin, &smax)) ? smax : 0;
#else
#error "implement me for your platform"
#endif
}

void aalloc_init(struct aalloc *s, size_t reserve_sz, size_t default_align, size_t grow_sz)
{
	size_t ps = cop_memory_query_page_size();

	assert(default_align && (((default_align - 1) & default_align) == 0) && "default_align must be positive and a power of two");

	if (ps == 0 || reserve_sz == 0)
		abort();

	s->reserve_sz    = ps * ((reserve_sz + ps - 1) / ps);
	s->default_align = default_align;
	s->grow_sz       = ps * ((grow_sz == 0) ? 1 : ((grow_sz + ps - 1) / ps));
	s->sp            = 0;
	s->stack[s->sp]  = 0;
	s->protect_sz    = 0;
#if _WIN32
	s->base          = VirtualAlloc(NULL, s->reserve_sz, MEM_RESERVE, PAGE_NOACCESS);
	if (s->base == NULL)
		abort();
#else
	s->base          = mmap(NULL, s->reserve_sz, PROT_NONE, MAP_SHARED | MAP_ANON, -1, 0);
	if (s->base == MAP_FAILED)
		abort();
#endif
}

static size_t aalloc_alignoffset(size_t val, size_t align_mask)
{
	return (align_mask + 1 - (val & align_mask)) & align_mask;
}

void *aalloc_align_alloc(struct aalloc *s, size_t size, size_t align)
{
	size_t csz;
	size_t offset;

	assert(align && (((align - 1) & align) == 0) && "align must be positive and a power of two");

	csz    = s->stack[s->sp];
	offset = csz + aalloc_alignoffset((size_t)(s->base + csz), align - 1);

	/* Need to protect more pages. */
	if (offset + size > s->protect_sz) {
		size_t new_sz;
		new_sz = offset + size;
		new_sz = s->grow_sz * ((new_sz + s->grow_sz - 1) / s->grow_sz);
#if _WIN32
		if (VirtualAlloc(s->base + s->protect_sz, new_sz - s->protect_sz, MEM_COMMIT, PAGE_READWRITE) == NULL)
			return NULL;
#else
		if (mprotect(s->base, new_sz, PROT_READ | PROT_WRITE) == -1)
			return NULL;
#endif
		s->protect_sz = new_sz;
	}

	s->stack[s->sp] = offset + size;
	return s->base + offset;
}

void *aalloc_alloc(struct aalloc *s, size_t size)
{
	return aalloc_align_alloc(s, size, s->default_align);
}

void aalloc_free(struct aalloc *s)
{
#if _WIN32
	VirtualFree(s->base, 0, MEM_RELEASE);
#else
	munmap(s->base, s->reserve_sz);
#endif
}

void aalloc_push(struct aalloc *s)
{
	size_t csz = s->stack[s->sp];
	s->stack[++s->sp] = csz;
}

void aalloc_merge_pop(struct aalloc *s)
{
	size_t csz = s->stack[s->sp];
	assert(s->sp);
	s->stack[--s->sp] = csz;
}

void aalloc_pop(struct aalloc *s)
{
	assert(s->sp);
	/* TODO: unprotect pages. */
	s->sp--;
}


static void *alloc_grp_temps(struct cop_alloc_iface *a, size_t size, size_t align)
{
	struct cop_alloc_grp_temps *ctx = a->ctx;
	size_t start;
	if (align == 0)
		align = ctx->default_align;
	assert(align);
	start = ctx->first->size + aalloc_alignoffset((size_t)(ctx->first + 1) + ctx->first->size, align - 1);
	if (start + size > ctx->first->alloc_sz) {
		struct cop_alloc_grp_temps_buf *nb;
		size_t min_alloc    = size + align - 1;
		size_t actual_alloc = ctx->first->size * 2;
		actual_alloc = (actual_alloc > ctx->max_grow) ? ctx->max_grow : actual_alloc;
		actual_alloc = (actual_alloc < min_alloc) ? min_alloc : actual_alloc;
		nb = malloc(sizeof(*nb) + actual_alloc);
		if (nb == NULL)
			return NULL;
		nb->alloc_sz = actual_alloc;
		nb->size     = 0;
		nb->next     = ctx->first;
		ctx->first   = nb;
		start        = aalloc_alignoffset((size_t)(ctx->first + 1), align - 1);
	}
	ctx->first->size = start + size;
	return (unsigned char *)(ctx->first + 1) + start;
}

int cop_alloc_grp_temps_init(struct cop_alloc_grp_temps *gat, struct cop_alloc_iface *iface, size_t initial_sz, size_t max_grow, size_t default_align)
{
	initial_sz           = initial_sz ? initial_sz : cop_memory_query_page_size();
	initial_sz           = initial_sz ? initial_sz : 1024;
	gat->first           = malloc(sizeof(gat->first[0]) + initial_sz);
	if (gat->first == NULL)
		return -1;
	gat->first->alloc_sz = initial_sz;
	gat->first->size     = 0;
	gat->max_grow        = max_grow ? max_grow : (initial_sz * 2);
	gat->default_align   = default_align ? default_align : 16;
	iface->ctx           = gat;
	iface->alloc         = alloc_grp_temps;
	return 0;
}

void cop_alloc_grp_temps_reset(struct cop_alloc_grp_temps *gat, int try_realloc)
{
	struct cop_alloc_grp_temps_buf *first = gat->first;
	struct cop_alloc_grp_temps_buf *bigs = NULL;
	size_t total_alloc = 0;

	/* Ensure the largest allocation is at the start of the list. */
	while (first != NULL) {
		struct cop_alloc_grp_temps_buf *tmp;
		tmp          = first;
		first        = tmp->next;
		total_alloc += tmp->alloc_sz;
		if (bigs != NULL && tmp->alloc_sz < bigs->alloc_sz) {
			tmp->next  = bigs->next;
			bigs->next = tmp;
		} else {
			tmp->next = bigs;
			bigs      = tmp;
		}
	}
	bigs->next = NULL;

	/* Free every allocation after the frist. */
	if (bigs != NULL) {
		first = bigs->next;
		while (first != NULL) {
			struct cop_alloc_grp_temps_buf *tmp;
			tmp   = first->next;
			free(first);
			first = tmp;
		}
	}

	/* */
	if (try_realloc && total_alloc > bigs->alloc_sz) {
		struct cop_alloc_grp_temps_buf *tmp = realloc(bigs, sizeof(*bigs) + total_alloc);
		if (tmp != NULL) {
			bigs           = tmp;
			bigs->alloc_sz = total_alloc;
		}
	}

	bigs->size = 0;
	gat->first = bigs;
}

void cop_alloc_grp_temps_free(struct cop_alloc_grp_temps *gat)
{
	while (gat->first != NULL) {
		struct cop_alloc_grp_temps_buf *tmp = gat->first;
		gat->first = tmp->next;
		free(tmp);
	}
}




