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

#ifndef COP_SORT_H
#define COP_SORT_H

/* C Compiler, OS and Platform Abstractions - Sorting.
 *
 * Sorting doesn't really fit the COP description, but is seems useful enough
 * that it's getting included. This header provides some macros for generating
 * sort functions suitable for arrays of plain data types or structures. You
 * may add a "static" declarator before the macro to create a static sort
 * function.
 *
 * - COP_SORT_HEAP(fn_name_, data_type_, comparator_macro_)
 *   Prototype: void fn_name_(data_type_ *data, size_t length)
 *   In-place heap sort.
 *
 * - COP_SORT_INSERTION(fn_name_, data_type_, comparator_macro_)
 *   Prototype: void fn_name_(data_type_ *data, size_t length)
 *   In-place insertion sort.
 *
 * - COP_SORT_MERGE(fn_name_, data_type_, comparator_macro_)
 *   Prototype: void fn_name_(data_type_ *data, data_type *temp, size_t length)
 *   Out-of-place cache-friendly merge sort.
 *
 * - COP_SORT_QUICK(fn_name_, data_type_, comparator_macro_)
 *   Prototype: void fn_name_(data_type_ *data, size_t length)
 *   In-place quick sort. I don't think this is performing as well as it
 *   should... which probably means my implementation is junk.
 *
 * The comparator_macro_ should be supplied as a function-like macro which
 * evaluates to a non-zero value if the values are in the correct order. For
 * example
 *
 *   #define CM(a, b) (a < b)
 *   COP_SORT_INSERTION(my_sort_function, int, CM)
 *
 * Will create an insertion sort on integers and will place them in ascending
 * order. */

#include <assert.h>
#include <stddef.h>

#define COP_SORT_INSERTION(fn_name_, data_type_, comparator_macro_) \
void fn_name_(data_type_ *inout, size_t nb_elements) \
{ \
	size_t i, j; \
	assert(nb_elements >= 2); \
	for (i = 1; i < nb_elements; i++) { \
		data_type_ tmp; \
		tmp = inout[i]; \
		for (j = i; j && (comparator_macro_(tmp, inout[j-1])); j--) \
			inout[j] = inout[j-1]; \
		if (j != i) \
			inout[j] = tmp; \
	} \
}

#define COP_SORT_QUICK(fn_name_, data_type_, comparator_macro_) \
void fn_name_(data_type_ *inout, size_t nb_elements) \
{ \
	size_t start = 0; \
	size_t end   = nb_elements-1; \
	data_type_ pval; \
	if (nb_elements < 4) { \
		size_t i, j; \
		assert(nb_elements >= 2); \
		for (i = 1; i < nb_elements; i++) { \
			data_type_ tmp; \
			tmp = inout[i]; \
			for (j = i; j && (comparator_macro_(tmp, inout[j-1])); j--) \
				inout[j] = inout[j-1]; \
			if (j != i) \
				inout[j] = tmp; \
		} \
		return; \
	} \
	if (nb_elements >= 16) { \
		size_t p1 = start; \
		size_t p3 = start + (end - start) / 2; \
		size_t p2 = end; \
		if (comparator_macro_(inout[p3], inout[p2])) { size_t tp = p3; p3 = p2; p2 = tp; } \
		if (comparator_macro_(inout[p2], inout[p1])) { size_t tp = p2; p2 = p1; p1 = tp; } \
		if (comparator_macro_(inout[p3], inout[p2])) { size_t tp = p3; p3 = p2; p2 = tp; } \
		pval = inout[p2]; \
		if (p2 != end) \
			inout[p2] = inout[end]; \
	} else { \
		pval = inout[end]; \
	} \
	end--; assert(end > start); \
	while (start < end) { \
		if (comparator_macro_(inout[start], pval)) { \
			start++; \
		} else if (comparator_macro_(inout[end], pval)) { \
			data_type_ tmp = inout[end]; inout[end] = inout[start]; inout[start] = tmp; \
			start++; \
			end--; \
		} else { \
			end--; \
		} \
	} \
	while (start < nb_elements-1 && (comparator_macro_(inout[start], pval))) \
		start++; \
	if (start != nb_elements-1) \
		inout[nb_elements-1] = inout[start]; \
	inout[start] = pval; \
	if (start > 1) \
		fn_name_(inout, start); \
	nb_elements = nb_elements - ++start; \
	if (nb_elements > 1) \
		fn_name_(inout + start, nb_elements); \
}

#define COP_SORT_MERGE(fn_name_, data_type_, comparator_macro_) \
void fn_name_(data_type_ *inout, data_type_ *scratch, size_t nb_elements) \
{ \
	if (nb_elements <= 8) { \
		size_t i, j; \
		assert(nb_elements >= 2); \
		for (i = 1; i < nb_elements; i++) { \
			data_type_ tmp; \
			tmp = inout[i]; \
			for (j = i; j && (comparator_macro_(tmp, inout[j-1])); j--) \
				inout[j] = inout[j-1]; \
			if (j != i) \
				inout[j] = tmp; \
		} \
	} else { \
		size_t l1 = nb_elements / 2; \
		size_t l2 = nb_elements - l1; \
		data_type_ *p1 = scratch; \
		data_type_ *p2 = scratch + l1; \
		memcpy(scratch, inout, nb_elements * sizeof(data_type_)); \
		fn_name_(p1, inout, l1); \
		fn_name_(p2, inout, l2); \
		while (l1 && l2) { \
			if (comparator_macro_((*p1), (*p2))) { \
				*inout++ = *p1++; \
				l1--; \
			} else { \
				*inout++ = *p2++; \
				l2--; \
			} \
		} \
		while (l1--) \
			*inout++ = *p1++; \
		while (l2--) \
			*inout++ = *p2++; \
	} \
}

#define COP_SORT_HEAP(fn_name_, data_type_, comparator_macro_) \
void fn_name_(data_type_ *data, size_t nb_data) \
{ \
	size_t i; \
	for (i = 2; i <= nb_data; i++) { \
		size_t     j  = i - 1; \
		size_t     p  = (i >> 1) - 1; \
		data_type_ jv = data[j]; \
		data_type_ pv = data[p]; \
		while (!(comparator_macro_(jv, pv))) { \
			data[j] = pv; \
			data[p] = jv; \
			j       = p; \
			p       = (p + 1) >> 1; \
			if (!p--) \
				break; \
			pv = data[p]; \
		} \
	} \
	for (i = 1; i < nb_data; i++) { \
		size_t p, c, hs = nb_data - i; \
		data_type_ pv   = data[hs]; \
		data[hs]        = data[0]; \
		data[0]         = pv; \
		for (p = 0, c = 1; c < hs; p = c, c = ((c + 1) << 1) - 1) { \
			data_type_    cv2, cv = data[c]; \
			size_t cn             = c + 1; \
			if (cn < hs && (cv2 = data[cn], !(comparator_macro_(cv2, cv)))) { \
				c  = cn; \
				cv = cv2; \
			} \
			if (comparator_macro_(cv, pv)) \
				break; \
			data[c] = pv; \
			data[p] = cv; \
		} \
	} \
}

#endif /* COP_SORT_H */
