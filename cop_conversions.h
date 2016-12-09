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

/* C Compiler, OS and Platform Abstractions - Type/memory conversions */

#ifndef COP_CONVERSIONS
#define COP_CONVERSIONS

#include "cop_attributes.h"
#include <stdint.h>
#include <string.h>

static COP_ATTR_UNUSED uint_fast16_t cop_ld_ule16(const void *buf)
{
	const unsigned char *b = buf;
	return
		((uint_fast16_t)b[0] << 0) |
		((uint_fast16_t)b[1] << 8);
}

static COP_ATTR_UNUSED uint_fast16_t cop_ld_ube16(const void *buf)
{
	const unsigned char *b = buf;
	return
		((uint_fast16_t)b[1] << 0) |
		((uint_fast16_t)b[0] << 8);
}

static COP_ATTR_UNUSED uint_fast32_t cop_ld_ule24(const void *buf)
{
	const unsigned char *b = buf;
	return
		((uint_fast32_t)b[0] << 0) |
		((uint_fast32_t)b[1] << 8) |
		((uint_fast32_t)b[2] << 16);
}

static COP_ATTR_UNUSED uint_fast32_t cop_ld_ube24(const void *buf)
{
	const unsigned char *b = buf;
	return
		((uint_fast32_t)b[2] << 0) |
		((uint_fast32_t)b[1] << 8) |
		((uint_fast32_t)b[0] << 16);
}

static COP_ATTR_UNUSED int_fast32_t cop_ld_sle24(const void *buf)
{
#if defined(__clang__) && defined(__amd64)
	int32_t val = 0;
	memcpy((char *)&val + 1, (const unsigned char *)buf, 3);
	val >>= 8;
	return val;
#else
	uint_fast32_t val;
	val =              ((const unsigned char *)buf)[2];
	val = (val << 8) | ((const unsigned char *)buf)[1];
	val = (val << 8) | ((const unsigned char *)buf)[0];
	return (val & 0x800000u) ? -(int_fast32_t)(((~val) & 0x7FFFFFu) + 1) : (int_fast32_t)val;
#endif
}

static COP_ATTR_UNUSED uint_fast32_t cop_ld_ule32(const void *buf)
{
	const unsigned char *b = buf;
	return
		((uint_fast32_t)b[0] << 0) |
		((uint_fast32_t)b[1] << 8) |
		((uint_fast32_t)b[2] << 16) |
		((uint_fast32_t)b[3] << 24);
}

static COP_ATTR_UNUSED uint_fast32_t cop_ld_ube32(const void *buf)
{
	const unsigned char *b = buf;
	return
		((uint_fast32_t)b[3] << 0) |
		((uint_fast32_t)b[2] << 8) |
		((uint_fast32_t)b[1] << 16) |
		((uint_fast32_t)b[0] << 24);
}

static COP_ATTR_UNUSED void cop_st_ule16(void *buf, uint_fast16_t val)
{
	unsigned char *b = buf;
	b[0] = val        & 0xFFu;
	b[1] = (val >> 8) & 0xFFu;
}

static COP_ATTR_UNUSED void cop_st_sle16(void *buf, int_fast16_t val)
{
	cop_st_ule16(buf, ((uint_fast16_t)val) & 0xFFFF);
}

static COP_ATTR_UNUSED void cop_st_ule24(void *buf, uint_fast32_t val)
{
	unsigned char *b = buf;
	b[0] = val         & 0xFFu;
	b[1] = (val >> 8)  & 0xFFu;
	b[2] = (val >> 16) & 0xFFu;
}

static COP_ATTR_UNUSED void cop_st_sle24(void *buf, int_fast32_t val)
{
	cop_st_ule24(buf, ((uint_fast32_t)val) & 0xFFFFFF);
}

static COP_ATTR_UNUSED void cop_st_ule32(void *buf, uint_fast32_t val)
{
	unsigned char *b = buf;
	b[0] = val         & 0xFFu;
	b[1] = (val >> 8)  & 0xFFu;
	b[2] = (val >> 16) & 0xFFu;
	b[3] = (val >> 24) & 0xFFu;
}

#endif /* COP_CONVERSIONS */
