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

/* C Compiler, OS and Platform Abstractions - Common Compiler Function and
 *                                            Variable Attributes.
 *
 * This header provides the following function-attribute-like macros:
 *
 *   COP_ATTR_UNUSED   Declare that a function might not be used (as is a
 *                     common case in header-only libraries). Example use:
 *                     to suppress unused function compiler warnings.
 *   COP_ATTR_NOINLINE Declare that the function should never be inlined.
 *                     Example use: an optimizing compiler inlines a large
 *                     function which we are trying to get sampled profiling
 *                     information for - use this to prevent that.
 *   COP_ATTR_ALWAYSINLINE Declare that we always want this function to be
 *                     inlined. There is still no guarantee that inlining will
 *                     occur.
 *
 * The following variable-attribute-like macros:
 *
 *   COP_ATTR_RESTRICT Declare that a pointer variable does not alias other
 *                     pointers. By default, all pointers of compatible types
 *                     may alias.
 *
 * The following compiler-hint macros:
 *
 *   COP_HINT_FALSE(condition) This function-like macro should be used with a
 *                     conditional argument. The macro will always return the
 *                     result of the condition. It will indicate to supporting
 *                     compilers that the condition is likely to evaluate to
 *                     zero. */

#ifndef COP_ATTRIBUES_H
#define COP_ATTRIBUES_H

#ifdef __STDC_VERSION__
#if (__STDC_VERSION__ >= 199901L)
#define COP_ATTR_RESTRICT restrict
#endif
#endif

#if defined(__clang__) || defined(__GNUC__)
#define COP_ATTR_UNUSED         __attribute__((unused))
#define COP_ATTR_ALWAYSINLINE   __inline__ __attribute__((always_inline))
#define COP_ATTR_NOINLINE       __attribute__((noinline))
#define COP_HINT_FALSE(cond)    __builtin_expect(cond, 0)

#ifndef COP_ATTR_RESTRICT
#define COP_ATTR_RESTRICT       __restrict__
#endif
#elif defined(_MSC_VER)
#define COP_ATTR_UNUSED
#define COP_ATTR_ALWAYSINLINE
#define COP_ATTR_NOINLINE       __declspec(noinline)
#define COP_HINT_FALSE(cond)    (cond)

#ifndef COP_ATTR_RESTRICT
#define COP_ATTR_RESTRICT       __restrict
#endif
#else
#define COP_ATTR_UNUSED
#define COP_ATTR_ALWAYSINLINE
#define COP_ATTR_NOINLINE
#define COP_HINT_FALSE(cond)    (cond)
#endif

#ifndef COP_ATTR_RESTRICT
#define COP_ATTR_RESTRICT
#endif

#endif /* COP_ATTRIBUES_H */
