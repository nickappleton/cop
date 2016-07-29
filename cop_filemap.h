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

/* C Compiler, OS and Platform Abstractions - Simple file mapping.
 *
 * This is completely system dependent. Using this header may not work.
 *
 * Don't rely on this API staying the same... it doesn't do everything I want
 * it to do yet and it doesn't even work on Windows. */

#ifndef COP_FILEMAP
#define COP_FILEMAP

#include "cop_attributes.h"
#include <assert.h>
#include <stddef.h>

#if defined(_MSC_VER)
#include <windows.h>
#endif

struct cop_filemap {
	void   *ptr;
	size_t  size;
#if defined(_MSC_VER)
	HANDLE  filehandle;
	HANDLE  maphandle;
#endif
};

/* This flag will permit reads to the mapped memory. */
#define COP_FILEMAP_FLAG_R      (0x1)

/* This flag will permit writes to the mapped memory. This does not mean that
 * writes will be carried through to the underlying file; if that behaviour is
 * desired, COP_FILEMAP_SHARED must also be set. */
#define COP_FILEMAP_FLAG_W      (0x2)

/* This flag indicates that writes into the mapped memory should be carried
 * through to the underlying file. */
#define COP_FILEMAP_SHARED      (0x4)

/* The mapping could not be constructed due to a file access error. */
#define COP_FILEMAP_ERR_FILE    (1)

/* The mapping could not be created because of a system mapping problem. */
#define COP_FILEMAP_ERR_MAPPING (2)

int  cop_filemap_open(struct cop_filemap *map, const char *filename, unsigned flags);
void cop_filemap_close(struct cop_filemap *map);

#endif /* COP_FILEMAP */

