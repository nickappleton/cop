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
#include <stddef.h>

/* Returns the maximum amount of memory that can be locked to physical pages
 * by the process. A return value of zero means either:
 *   no pages can be locked
 *   the number of lockable pages cannot be queried or
 *   it is unknown how many pages can be locked.
 * A return value of SIZE_MAX indicates that there is no limit imposed on the
 * number of pages which may be locked. Otherwise, the return value is roughly
 * how many pages can be locked. */
static COP_ATTR_UNUSED size_t cop_memory_query_current_lockable();

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

static COP_ATTR_UNUSED size_t cop_memory_query_page_size()
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
#error "do not know how to get page size"
#endif
}

static COP_ATTR_UNUSED size_t cop_memory_query_system_memory()
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
	return (GlobalMemoryStatusEx(&memstatus)) ? memstatus.ullTotalPhys : 0;
#else
#error "do not know how to get system memory"
#endif
}

static COP_ATTR_UNUSED size_t cop_memory_query_current_lockable()
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
#error "implement me"
#endif
}

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct aalloc;

static COP_ATTR_UNUSED void aalloc_push(struct aalloc *s);
static COP_ATTR_UNUSED void aalloc_merge_pop(struct aalloc *s);
static COP_ATTR_UNUSED void aalloc_pop(struct aalloc *s);
static COP_ATTR_UNUSED void aalloc_init(struct aalloc *s, size_t reserve_sz, size_t default_align, size_t grow_sz);
static COP_ATTR_UNUSED void aalloc_free(struct aalloc *s);
static COP_ATTR_UNUSED void *aalloc_alloc(struct aalloc *s, size_t size);
static COP_ATTR_UNUSED void *aalloc_align_alloc(struct aalloc *s, size_t size, size_t align);

struct aalloc {
	size_t         reserve_sz;
	size_t         grow_sz;
	size_t         protect_sz;
	size_t         default_align;
	unsigned char *base;
	unsigned       sp;
	size_t         stack[32];
};

static void aalloc_init(struct aalloc *s, size_t reserve_sz, size_t default_align, size_t grow_sz)
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

static void *aalloc_align_alloc(struct aalloc *s, size_t size, size_t align)
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

static void *aalloc_alloc(struct aalloc *s, size_t size)
{
	return aalloc_align_alloc(s, size, s->default_align);
}

static void aalloc_free(struct aalloc *s)
{
#if _WIN32
	VirtualFree(s->base, 0, MEM_RELEASE);
#else
	munmap(s->base, s->reserve_sz);
#endif
}

static void aalloc_push(struct aalloc *s)
{
	size_t csz = s->stack[s->sp];
	s->stack[++s->sp] = csz;
}

static void aalloc_merge_pop(struct aalloc *s)
{
	size_t csz = s->stack[s->sp];
	assert(s->sp);
	s->stack[--s->sp] = csz;
}

static void aalloc_pop(struct aalloc *s)
{
	assert(s->sp);
	/* TODO: unprotect pages. */
	s->sp--;
}

#endif /* COP_ALLOC_H */
