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

#include "cop/cop_filemap.h"
#include <stdint.h>

#if defined(_MSC_VER)

#include <stdlib.h>

int cop_filemap_open(struct cop_filemap *map, const char *filename, unsigned flags)
{
	DWORD faccess;
	DWORD mapprotect;
	DWORD mapaccess;
	LARGE_INTEGER fsz;
	LPWSTR wfn;
	int fnlen;

	if ((flags & COP_FILEMAP_FLAG_W) == 0) {
		/* read only access */
		faccess    = GENERIC_READ;
		mapprotect = PAGE_READONLY;
		mapaccess  = FILE_MAP_READ;
	} else if ((flags & COP_FILEMAP_SHARED)) {
		/* shared write access */
		faccess    = GENERIC_READ | GENERIC_WRITE;
		mapprotect = PAGE_READWRITE;
		mapaccess  = FILE_MAP_WRITE;
	} else {
		/* unshared write access */
		faccess    = GENERIC_READ;
		mapprotect = PAGE_READONLY;
		mapaccess  = FILE_MAP_COPY;
	}

	fnlen = MultiByteToWideChar(CP_UTF8, 0, filename, -1, NULL, 0);
	if (fnlen == 0)
		return -1;

	wfn = malloc(sizeof(*wfn) * fnlen);
	if (wfn == NULL)
		return -1;

	if (MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfn, fnlen) != fnlen) {
		free(wfn);
		return -1;
	}

	map->filehandle = CreateFileW(wfn, faccess, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (map->filehandle == INVALID_HANDLE_VALUE) {
		free(wfn);
		return -1;
	}

	free(wfn);

	if (!GetFileSizeEx(map->filehandle, &fsz)) {
		CloseHandle(map->filehandle);
		return -1;
	}

	if (fsz.QuadPart > SIZE_MAX) {
		CloseHandle(map->filehandle);
		return -1;
	}

	map->size = (size_t)fsz.QuadPart;

	map->maphandle = CreateFileMapping(map->filehandle, NULL, mapprotect, 0, 0, NULL);
	if (map->maphandle == INVALID_HANDLE_VALUE) {
		CloseHandle(map->filehandle);
		return -1;
	}

	map->ptr = MapViewOfFile(map->maphandle, mapaccess, 0, 0, 0);
	if (map->ptr == NULL) {
		CloseHandle(map->maphandle);
		CloseHandle(map->filehandle);
		return -1;
	}

	return 0;
}

void cop_filemap_close(struct cop_filemap *map)
{
	UnmapViewOfFile(map->ptr);
	CloseHandle(map->maphandle);
	CloseHandle(map->filehandle);
}

#else

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

int cop_filemap_open(struct cop_filemap *map, const char *filename, unsigned flags)
{
	int         oflags;
	int         fd;
	struct stat fs;
	int         prot;

	if (flags & COP_FILEMAP_FLAG_W) {
		oflags = (flags & COP_FILEMAP_SHARED) ? O_RDWR                   : O_RDONLY;
		prot   = (flags & COP_FILEMAP_FLAG_R) ? (PROT_READ | PROT_WRITE) : PROT_WRITE;
	} else {
		assert(flags & COP_FILEMAP_FLAG_R);
		oflags = O_RDONLY;
		prot   = PROT_READ;
	}

	fd = open(filename, oflags);
	if (fd == -1)
		return COP_FILEMAP_ERR_FILE;

	if (fstat(fd, &fs) == -1 || fs.st_size > SIZE_MAX) {
		close(fd);
		return COP_FILEMAP_ERR_FILE;
	}

	map->size = fs.st_size;
	map->ptr  = mmap(0, fs.st_size, prot, (flags & COP_FILEMAP_SHARED) ? MAP_SHARED : MAP_PRIVATE, fd, 0);

	close(fd);

	return (map->ptr == MAP_FAILED) ? COP_FILEMAP_ERR_MAPPING : 0;
}

void cop_filemap_close(struct cop_filemap *map)
{
	munmap(map->ptr, map->size);
}

#endif
