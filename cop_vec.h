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

/* C Compiler, OS and Platform Abstractions - Floating-Point SIMD Vectors.
 *
 * This does not contain everything that can be done. I just add/modify things
 * in it when I see some code that could be improved.
 *
 * Some overall notes:
 *
 * 1) You'll notice there is no multiply-accumulate (mac) instruction. This is
 * intentional. If you write lots of code using macs, unless the compiler
 * re-orders the multiply and addition operations around other instructions
 * when the system has no mac instruction, there are likely to be stalls
 * happening. I find that it is better to do the additions and multiplies
 * separately and use a "-fassociative-math" flag or similar to permit the
 * compiler to generate mac instructions when it thinks they will actually
 * give a performance benefit. Nope, I'm not going to add them to this
 * library.
 *
 * 2) What's with V4F_EXISTS and V2D_EXISTS etc... why can't they always
 * just exist? Because you probably don't want them if the system has no
 * vector support. Emulating vectors will just eat all your registers and you
 * will probably end up with code barely performing better than scalar... if
 * that!
 *
 * The header file only creates macros in VEC_, V1F_, V1D, V4F_, V2D_, V8F_,
 * V4D_ namespaces. */

/* Reminder for Nick:
 * This is how you build something on OS X to get an armv7 executable purely
 * for examining the disassembly:
 *   xcrun --sdk iphoneos clang -arch armv7s vecfft.c -O3 -ffast-math
 * Delete this comment at some point when I know a better way to document this
 * so I'll remember it... */

#ifndef COP_VEC_H
#define COP_VEC_H

#include "cop_attributes.h"

/* If this macro is set, compiler built-in vector support will be disabled and
 * only intrinsic header files will be used (if found). If the macro is set
 * and there are no intrinsic implementations for this system, no vectors
 * will exist. If the compiler does not support a built-in vector type, this
 * macro has no effect. */
#ifndef VEC_DISABLE_COMPILER_BUILTINS
#define VEC_DISABLE_COMPILER_BUILTINS (1)
#endif

/* The library implementation is divided up into two parts. Parts one and two
 * deal with defining a minimal set of vector operators and support macros.
 *
 *   1) Compiler specific builtin vector implementations
 *        Probe the compiler to see if it has support for builtin vectors. We
 *      also probe to figure out what the platform is so we don't create
 *      vectors which will not map to the hardware. Make vectors.
 *   2) Platform specific vector implementations
 *        It the compiler does not support builtins (or the user has disabled
 *      them using VEC_DISABLE_COMPILER_BUILTINS), this is where we probe for
 *      platform-specific intrinsic header files. If we find some and they
 *      correspond to vector types that have not already been created by the
 *      compiler builtins, this is where we implement them.
 *   3) Generic macro implementations
 *        All other macros required to complete the library API which have not
 *      already been defined are created using the minimal required platform
 *      APIs. This permits the platform specific code to provide optimized
 *      operations where they provide performance benefits or do nothing and
 *      get a working generic implementation.
 *
 * The reason compiler builtins have first preference over the platform
 * intrinsics is related to some real experiences where the builtins generated
 * better code than the platform specific intrinsic operators. This may not be
 * the truth all the time (hence the VEC_DISABLE_COMPILER_BUILTINS) option.
 * For the skeptics, the "experience" was a piece of code which needed to load
 * a float into the low word of a vector and do some low-word operations
 * (rotating it into another vector). Using the SSE intrinsics, this resulted
 * in all the other vector words being explicitly zeroed using an extra
 * instruction. This was due to the provided intrinsic not perfectly mapping
 * to an instruction, but having some extra meaning. This extra operation did
 * not occur when using the builtin vector operations. When I've done
 * profiling, the builtins have normally performed better.
 *
 * Writing a new platform-specific vector implementation
 * -----------------------------------------------------
 * First, you must check that an implementation has not already been
 * provided (i.e. test that Vxx_EXISTS is not defined) by a compiler builtin
 * vector implementation. The test for this obviously must in section 2 of
 * this header (i.e. after the tests and potential implementations of compiler
 * built-in vectors.
 *
 * You must provide implementations for the following functions:
 *   ld, st, lde0, broadcast, add, sub, neg, mul, rotl, reverse
 *
 * You must provide at least macro implementations of
 *   INTERLEAVE, DEINTERLEAVE
 *
 * You define EXISTS to 1.
 *
 * You may define any of the extended macro implementations if they will map
 * closer to available instructions. */

#define VEC_FUNCTION_ATTRIBUTES static COP_ATTR_UNUSED COP_ATTR_ALWAYSINLINE

#if defined(__clang__) || defined(__GNUC__)
#define VEC_ALIGN_BEST __attribute__((aligned(64)))
#elif defined(_MSC_VER)
#define VEC_ALIGN_BEST __declspec(align(64))
#else
#define VEC_ALIGN_BEST
#endif

/* This macro is used to construct most of the vector functions for compilers
 * which have builtin vector support. i.e. most compiler builtin vectors
 * support using regular operators to operate on the vector - so this macro
 * will generate those operations. This macro is always used for the v1d and
 * v1f types which only really exist as a convenience. */
#define VEC_BASIC_OPERATIONS(type_, data_, initsplat_, acc0_) \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _add(type_ a, type_ b)          { return a + b; } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _sub(type_ a, type_ b)          { return a - b; } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _mul(type_ a, type_ b)          { return a * b; } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _ld(const void *ptr)            { return *((type_ *)ptr); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _lde0(type_ a, const void *ptr) { a acc0_ = *((data_ *)ptr); return a; } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _broadcast(data_ a)             { return (type_)initsplat_(a); } \
VEC_FUNCTION_ATTRIBUTES void  type_ ## _st(void *ptr, type_ val)       { *((type_ *)ptr) = val; } \
VEC_FUNCTION_ATTRIBUTES void  type_ ## _ste0(void *ptr, type_ val)     { *((data_ *)ptr) = val acc0_; } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _neg(type_ a)                   { return -a; }

#define VEC_INITSPLAT1(a) (a)
#define VEC_INITSPLAT2(a) {(a), (a)}
#define VEC_INITSPLAT4(a) {(a), (a), (a), (a)}
#define VEC_INITSPLAT8(a) {(a), (a), (a), (a), (a), (a), (a), (a)}

#if !VEC_DISABLE_COMPILER_BUILTINS
#if (defined(__clang__) || defined(__GNUC__)) && (defined(__SSE__) || defined(__ARM_NEON__))

#define V4F_EXISTS (1)
#define V2D_EXISTS (1)

#if defined(__clang__)
typedef float  v4f __attribute__((ext_vector_type(4), aligned(32)));
typedef double v2d __attribute__((ext_vector_type(2), aligned(32)));
#define SHUFDEF_V4F2(name_, i0_, i1_, i2_, i3_) VEC_FUNCTION_ATTRIBUTES v4f v4f_ ## name_(v4f a, v4f b) { return __builtin_shufflevector(a, b, i0_, i1_, i2_, i3_); }
#define SHUFDEF_V4F1(name_, i0_, i1_, i2_, i3_) VEC_FUNCTION_ATTRIBUTES v4f v4f_ ## name_(v4f a)        { return __builtin_shufflevector(a, a, i0_, i1_, i2_, i3_); }
#define SHUFDEF_V2D2(name_, i0_, i1_)           VEC_FUNCTION_ATTRIBUTES v2d v2d_ ## name_(v2d a, v2d b) { return __builtin_shufflevector(a, b, i0_, i1_); }
#define SHUFDEF_V2D1(name_, i0_, i1_)           VEC_FUNCTION_ATTRIBUTES v2d v2d_ ## name_(v2d a)        { return __builtin_shufflevector(a, a, i0_, i1_); }
#else
#include <stdint.h>
typedef float  v4f __attribute__((vector_size(16), aligned(32)));
typedef double v2d __attribute__((vector_size(16), aligned(32)));
#define SHUFDEF_V4F2(name_, i0_, i1_, i2_, i3_) VEC_FUNCTION_ATTRIBUTES v4f v4f_ ## name_(v4f a, v4f b) { static const int32_t __attribute__((vector_size(16))) shufmask = {i0_, i1_, i2_, i3_}; return __builtin_shuffle(a, b, shufmask); }
#define SHUFDEF_V4F1(name_, i0_, i1_, i2_, i3_) VEC_FUNCTION_ATTRIBUTES v4f v4f_ ## name_(v4f a)        { static const int32_t __attribute__((vector_size(16))) shufmask = {i0_, i1_, i2_, i3_}; return __builtin_shuffle(a, b, shufmask); }
#define SHUFDEF_V2D2(name_, i0_, i1_, i2_, i3_) VEC_FUNCTION_ATTRIBUTES v2d v2d_ ## name_(v2d a, v2d b) { static const int64_t __attribute__((vector_size(16))) shufmask = {i0_, i1_};           return __builtin_shuffle(a, b, shufmask); }
#define SHUFDEF_V2D1(name_, i0_, i1_, i2_, i3_) VEC_FUNCTION_ATTRIBUTES v2d v2d_ ## name_(v2d a)        { static const int64_t __attribute__((vector_size(16))) shufmask = {i0_, i1_};           return __builtin_shuffle(a, a, shufmask); }
#endif

SHUFDEF_V4F2(shuf0246, 0, 2, 4, 6)
SHUFDEF_V4F2(shuf1357, 1, 3, 5, 7)
SHUFDEF_V4F2(shuf0415, 0, 4, 1, 5)
SHUFDEF_V4F2(shuf2637, 2, 6, 3, 7)
SHUFDEF_V4F2(rotl,     1, 2, 3, 4)
SHUFDEF_V4F1(reverse,  3, 2, 1, 0)
SHUFDEF_V2D2(shuf02,   0, 2)
SHUFDEF_V2D2(shuf13,   1, 3)
SHUFDEF_V2D2(rotl,     1, 2)
SHUFDEF_V2D1(reverse,  1, 0)

/* The following are experimental and may be removed*/
SHUFDEF_V4F1(sel0,     0, 0, 0, 0)
SHUFDEF_V4F1(sel1,     1, 1, 1, 1)
SHUFDEF_V4F1(sel2,     2, 2, 2, 2)
SHUFDEF_V4F1(sel3,     3, 3, 3, 3)

#undef SHUFDEF_V2D2
#undef SHUFDEF_V4F2
#undef SHUFDEF_V2D1
#undef SHUFDEF_V4F1

#define V4F_DEINTERLEAVE(out1_, out2_, in1_, in2_) do { \
	out1_   = v4f_shuf0246(in1_, in2_); \
	out2_   = v4f_shuf1357(in1_, in2_); \
} while (0)
#define V4F_INTERLEAVE(out1_, out2_, in1_, in2_) do { \
	out1_   = v4f_shuf0415(in1_, in2_); \
	out2_   = v4f_shuf2637(in1_, in2_); \
} while (0)
#define V2D_DEINTERLEAVE(out1_, out2_, in1_, in2_) do { \
	out1_   = v2d_shuf02(in1_, in2_); \
	out2_   = v2d_shuf13(in1_, in2_); \
} while (0)
#define V2D_INTERLEAVE(out1_, out2_, in1_, in2_) V2D_DEINTERLEAVE(out1_, out2_, in1_, in2_)

VEC_BASIC_OPERATIONS(v4f, float, VEC_INITSPLAT4, [0])
VEC_BASIC_OPERATIONS(v2d, double, VEC_INITSPLAT2, [0])

VEC_FUNCTION_ATTRIBUTES v4f   v4f_max(v4f a, v4f b) { return (v4f){a[0] > b[0] ? a[0] : b[0], a[1] > b[1] ? a[1] : b[1], a[2] > b[2] ? a[2] : b[2], a[3] > b[3] ? a[3] : b[3]}; }
VEC_FUNCTION_ATTRIBUTES v4f   v4f_min(v4f a, v4f b) { return (v4f){a[0] < b[0] ? a[0] : b[0], a[1] < b[1] ? a[1] : b[1], a[2] < b[2] ? a[2] : b[2], a[3] < b[3] ? a[3] : b[3]}; }
VEC_FUNCTION_ATTRIBUTES v2d   v2d_max(v2d a, v2d b) { return (v2d){a[0] > b[0] ? a[0] : b[0], a[1] > b[1] ? a[1] : b[1]}; }
VEC_FUNCTION_ATTRIBUTES v2d   v2d_min(v2d a, v2d b) { return (v2d){a[0] < b[0] ? a[0] : b[0], a[1] < b[1] ? a[1] : b[1]}; }

#endif /* (defined(__clang__) || defined(__GNUC__)) && defined(__SSE__) */

#if (defined(__clang__) || defined(__GNUC__)) && defined(__AVX__)

#define V8F_EXISTS (1)
#define V4D_EXISTS (1)

#if defined(__clang__)
typedef float  v8f __attribute__((ext_vector_type(8), aligned(32)));
typedef double v4d __attribute__((ext_vector_type(4), aligned(32)));
VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf0145(v4d a, v4d b)     { return __builtin_shufflevector(a, b, 0, 1, 4, 5); }
VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf2367(v4d a, v4d b)     { return __builtin_shufflevector(a, b, 2, 3, 6, 7); }
VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf0426(v4d a, v4d b)     { return __builtin_shufflevector(a, b, 0, 4, 2, 6); }
VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf1537(v4d a, v4d b)     { return __builtin_shufflevector(a, b, 1, 5, 3, 7); }
VEC_FUNCTION_ATTRIBUTES v4d v4d_rotl(v4d a, v4d b)         { return __builtin_shufflevector(a, b, 1, 2, 3, 4); }
VEC_FUNCTION_ATTRIBUTES v4d v4d_reverse(v4d a)             { return __builtin_shufflevector(a, a, 3, 2, 1, 0); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf012389AB(v8f a, v8f b) { return __builtin_shufflevector(a, b, 0,  1,  2,  3,  8,  9,  10, 11); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf4567CDEF(v8f a, v8f b) { return __builtin_shufflevector(a, b, 4,  5,  6,  7,  12, 13, 14, 15); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf028A46CE(v8f a, v8f b) { return __builtin_shufflevector(a, b, 0,  2,  8,  10, 4,  6,  12, 14); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf139B57DF(v8f a, v8f b) { return __builtin_shufflevector(a, b, 1,  3,  9,  11, 5,  7,  13, 15); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf08194C5D(v8f a, v8f b) { return __builtin_shufflevector(a, b, 0,  8,  1,  9,  4,  12, 5,  13); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf2A3B6E7F(v8f a, v8f b) { return __builtin_shufflevector(a, b, 2,  10, 3,  11, 6,  14, 7,  15); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf018945CD(v8f a, v8f b) { return __builtin_shufflevector(a, b, 0,  1,  8,  9,  4,  5,  12, 13); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf23AB67EF(v8f a, v8f b) { return __builtin_shufflevector(a, b, 2,  3,  10, 11, 6,  7,  14, 15); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_rotl(v8f a, v8f b)         { return __builtin_shufflevector(a, b, 1,  2,  3,  4,  5,  6,  7,  8); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_reverse(v8f a)             { return __builtin_shufflevector(a, a, 7,  6,  5,  4,  3,  2,  1,  0); }
#else
#include <stdint.h>
typedef float  v8f __attribute__((vector_size(32), aligned(32)));
typedef double v4d __attribute__((vector_size(32), aligned(32)));
VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf0145(v4d a, v4d b)     { static const int64_t __attribute__((vector_size(32))) shufmask = {0, 1, 4, 5}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf2367(v4d a, v4d b)     { static const int64_t __attribute__((vector_size(32))) shufmask = {2, 3, 6, 7}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf0426(v4d a, v4d b)     { static const int64_t __attribute__((vector_size(32))) shufmask = {0, 4, 2, 6}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf1537(v4d a, v4d b)     { static const int64_t __attribute__((vector_size(32))) shufmask = {1, 5, 3, 7}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v4d v4d_rotl(v4d a, v4d b)         { static const int64_t __attribute__((vector_size(32))) shufmask = {1, 2, 3, 4}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v4d v4d_reverse(v4d a)             { static const int64_t __attribute__((vector_size(32))) shufmask = {3, 2, 1, 0}; return __builtin_shuffle(a, a, shufmask); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf012389AB(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {0,  1,  2,  3,  8,  9,  10, 11}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf4567CDEF(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {4,  5,  6,  7,  12, 13, 14, 15}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf028A46CE(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {0,  2,  8,  10, 4,  6,  12, 14}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf139B57DF(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {1,  3,  9,  11, 5,  7,  13, 15}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf08194C5D(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {0,  8,  1,  9,  4,  12, 5,  13}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf2A3B6E7F(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {2,  10, 3,  11, 6,  14, 7,  15}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf018945CD(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {0,  1,  8,  9,  4,  5,  12, 13}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf23AB67EF(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {2,  3,  10, 11, 6,  7,  14, 15}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_rotl(v8f a, v8f b)         { static const int32_t __attribute__((vector_size(32))) shufmask = {1,  2,  3,  4,  5,  6,  7,  8}; return __builtin_shuffle(a, b, shufmask); }
VEC_FUNCTION_ATTRIBUTES v8f v8f_reverse(v8f a)             { static const int32_t __attribute__((vector_size(32))) shufmask = {7,  6,  5,  4,  3,  2,  1,  0}; return __builtin_shuffle(a, a, shufmask); }
#endif

/* This is awful but generates much better code than the "simple"
 * implementation. */
#define V4D_DEINTERLEAVE(out1_, out2_, in1_, in2_) do { \
	v4d t1_, t2_; \
	t1_   = v4d_shuf0145(in1_, in2_); /* a0, a1, b0, b1 */ \
	t2_   = v4d_shuf2367(in1_, in2_); /* a2, a3, b2, b3 */ \
	out1_ = v4d_shuf0426(t1_,  t2_);  /* a0, a2, b0, b2 */ \
	out2_ = v4d_shuf1537(t1_,  t2_);  /* a1, a3, b1, b3 */ \
} while (0)
#define V4D_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	v4d t1_, t2_, t3_, t4_; \
	t1_   = v4d_shuf0145(in1_, in2_); /* a0, a1, b0, b1 */ \
	t2_   = v4d_shuf2367(in1_, in2_); /* a2, a3, b2, b3 */ \
	t3_   = v4d_shuf0145(in3_, in4_); /* a0, a1, b0, b1 */ \
	t4_   = v4d_shuf2367(in3_, in4_); /* a2, a3, b2, b3 */ \
	out1_ = v4d_shuf0426(t1_,  t2_);  /* a0, a2, b0, b2 */ \
	out2_ = v4d_shuf1537(t1_,  t2_);  /* a1, a3, b1, b3 */ \
	out3_ = v4d_shuf0426(t3_,  t4_);  /* a0, a2, b0, b2 */ \
	out4_ = v4d_shuf1537(t3_,  t4_);  /* a1, a3, b1, b3 */ \
} while (0)
#define V4D_INTERLEAVE(out1_, out2_, in1_, in2_) do { \
	v4d t1_, t2_; \
	t1_   = v4d_shuf0426(in1_, in2_); /* a0, b0, a2, b2 */ \
	t2_   = v4d_shuf1537(in1_, in2_); /* a1, b1, a3, b3 */ \
	out1_ = v4d_shuf0145(t1_,  t2_);  /* a0, b0, a1, b1 */ \
	out2_ = v4d_shuf2367(t1_,  t2_);  /* a2, b2, a3, b3 */ \
} while (0)
#define V4D_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	v4d t1_, t2_, t3_, t4_; \
	t1_   = v4d_shuf0426(in1_, in2_); /* a0, b0, a2, b2 */ \
	t2_   = v4d_shuf1537(in1_, in2_); /* a1, b1, a3, b3 */ \
	t3_   = v4d_shuf0426(in3_, in4_); /* a0, b0, a2, b2 */ \
	t4_   = v4d_shuf1537(in3_, in4_); /* a1, b1, a3, b3 */ \
	out1_ = v4d_shuf0145(t1_,  t2_);  /* a0, b0, a1, b1 */ \
	out2_ = v4d_shuf2367(t1_,  t2_);  /* a2, b2, a3, b3 */ \
	out3_ = v4d_shuf0145(t3_,  t4_);  /* a0, b0, a1, b1 */ \
	out4_ = v4d_shuf2367(t3_,  t4_);  /* a2, b2, a3, b3 */ \
} while (0)
#define V4D_TRANSPOSE_INPLACE(r1_, r2_, r3_, r4_) do { \
	v4d t1_, t2_, t3_, t4_; \
	t1_ = v4d_shuf0145(r1_, r3_); /* r00, r01, r20, r21 */ \
	t3_ = v4d_shuf0145(r2_, r4_); /* r10, r11, r30, r31 */ \
	t2_ = v4d_shuf2367(r1_, r3_); /* r02, r03, r22, r23 */ \
	t4_ = v4d_shuf2367(r2_, r4_); /* r12, r13, r32, r33 */ \
	r1_ = v4d_shuf0426(t1_, t3_); /* r00, r10, r20, r30 */ \
	r2_ = v4d_shuf1537(t1_, t3_); /* r01, r11, r21, r31 */ \
	r3_ = v4d_shuf0426(t2_, t4_); /* r02, r12, r22, r32 */ \
	r4_ = v4d_shuf1537(t2_, t4_); /* r03, r13, r23, r33 */ \
} while (0)

/* This is awful but generates much better code than the "simple"
 * implementation. */
#define V8F_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	v8f Z1_, Z2_, Z3_, Z4_; \
	Z1_   = v8f_shuf012389AB(in1_, in2_); /* a0, a1, a2, a3, b0, b1, b2, b3 */ \
	Z2_   = v8f_shuf4567CDEF(in1_, in2_); /* a4, a5, a6, a7, b4, b5, b6, b7 */ \
	Z3_   = v8f_shuf012389AB(in3_, in4_); /* a0, a1, a2, a3, b0, b1, b2, b3 */ \
	Z4_   = v8f_shuf4567CDEF(in3_, in4_); /* a4, a5, a6, a7, b4, b5, b6, b7 */ \
	out1_ = v8f_shuf028A46CE(Z1_,  Z2_);  /* a0, a2, a4, a6, b0, b2, b4, b6 */ \
	out2_ = v8f_shuf139B57DF(Z1_,  Z2_);  /* a1, b3, a5, a7, b1, b3, b5, b7 */ \
	out3_ = v8f_shuf028A46CE(Z3_,  Z4_);  /* a0, a2, a4, a6, b0, b2, b4, b6 */ \
	out4_ = v8f_shuf139B57DF(Z3_,  Z4_);  /* a1, b3, a5, a7, b1, b3, b5, b7 */ \
} while (0)
#define V8F_DEINTERLEAVE(out1_, out2_, in1_, in2_) do { \
	v8f Z1_, Z2_; \
	Z1_   = v8f_shuf012389AB(in1_, in2_); /* a0, a1, a2, a3, b0, b1, b2, b3 */ \
	Z2_   = v8f_shuf4567CDEF(in1_, in2_); /* a4, a5, a6, a7, b4, b5, b6, b7 */ \
	out1_ = v8f_shuf028A46CE(Z1_,  Z2_);  /* a0, a2, a4, a6, b0, b2, b4, b6 */ \
	out2_ = v8f_shuf139B57DF(Z1_,  Z2_);  /* a1, b3, a5, a7, b1, b3, b5, b7 */ \
} while (0)
#define V8F_INTERLEAVE(out1_, out2_, in1_, in2_) do { \
	v8f Z1_, Z2_; \
	Z1_   = v8f_shuf08194C5D(in1_, in2_); /* a0, b0, a1, b1, a4, b4, a5, b5 */ \
	Z2_   = v8f_shuf2A3B6E7F(in1_, in2_); /* a2, b2, a3, b3, a6, b6, a7, b7 */ \
	out1_ = v8f_shuf012389AB(Z1_,  Z2_);  /* a0, b0, a1, b1, a2, b2, a3, b3 */ \
	out2_ = v8f_shuf4567CDEF(Z1_,  Z2_);  /* a4, b4, a5, b5, a6, b6, a7, b7 */ \
} while (0)
#define V8F_TRANSPOSE_INPLACE(r1_, r2_, r3_, r4_, r5_, r6_, r7_, r8_) do { \
	v8f t1_, t2_, t3_, t4_, t5_, t6_, t7_, t8_; \
	v8f z1_, z2_, z3_, z4_, z5_, z6_, z7_, z8_; \
	t1_ = v8f_shuf08194C5D(r1_, r2_); /* r00 r10 r01 r11 r04 r14 r05 r15 */ \
	t2_ = v8f_shuf2A3B6E7F(r1_, r2_); /* r02 r12 r03 r13 r06 r16 r07 r17 */ \
	t3_ = v8f_shuf08194C5D(r3_, r4_); /* r20 r30 r21 r31 r24 r34 r25 r35 */ \
	t4_ = v8f_shuf2A3B6E7F(r3_, r4_); /* r22 r32 r23 r33 r26 r36 r27 r37 */ \
	t5_ = v8f_shuf08194C5D(r5_, r6_); /* r40 r50 r41 r51 r44 r54 r45 r55 */ \
	t6_ = v8f_shuf2A3B6E7F(r5_, r6_); /* r42 r52 r43 r53 r46 r56 r47 r57 */ \
	t7_ = v8f_shuf08194C5D(r7_, r8_); /* r60 r70 r61 r71 r64 r74 r65 r75 */ \
	t8_ = v8f_shuf2A3B6E7F(r7_, r8_); /* r62 r72 r63 r73 r66 r76 r67 r77 */ \
	z1_ = v8f_shuf018945CD(t1_, t3_); /* r00 r10 r20 r30 r04 r14 r24 r34 */ \
	z2_ = v8f_shuf23AB67EF(t1_, t3_); /* r01 r11 r21 r31 r05 r15 r25 r35 */ \
	z3_ = v8f_shuf018945CD(t2_, t4_); /* r02 r12 r22 r32 r06 r16 r26 r36 */ \
	z4_ = v8f_shuf23AB67EF(t2_, t4_); /* r03 r13 r23 r33 r07 r17 r27 r37 */ \
	z5_ = v8f_shuf018945CD(t5_, t7_); /* r40 r50 r60 r70 r44 r54 r64 r74 */ \
	z6_ = v8f_shuf23AB67EF(t5_, t7_); /* r41 r51 r61 r71 r45 r55 r65 r75 */ \
	z7_ = v8f_shuf018945CD(t6_, t8_); /* r42 r52 r62 r72 r46 r56 r66 r76 */ \
	z8_ = v8f_shuf23AB67EF(t6_, t8_); /* r43 r53 r63 r73 r47 r57 r67 r77 */ \
	r1_ = v8f_shuf012389AB(z1_, z5_); /* r00 r10 r20 r30 r40 r50 r60 r70 */ \
	r5_ = v8f_shuf4567CDEF(z1_, z5_); /* r04 r14 r24 r34 r44 r54 r64 r74 */ \
	r2_ = v8f_shuf012389AB(z2_, z6_); /* r01 r11 r21 r31 r41 r51 r61 r71 */ \
	r6_ = v8f_shuf4567CDEF(z2_, z6_); /* r05 r15 r25 r35 r45 r55 r65 r75 */ \
	r3_ = v8f_shuf012389AB(z3_, z7_); /* r02 r12 r22 r32 r42 r52 r62 r72 */ \
	r7_ = v8f_shuf4567CDEF(z3_, z7_); /* r06 r16 r26 r36 r46 r56 r66 r76 */ \
	r4_ = v8f_shuf012389AB(z4_, z8_); /* r03 r13 r23 r33 r43 r53 r63 r73 */ \
	r8_ = v8f_shuf4567CDEF(z4_, z8_); /* r07 r17 r27 r37 r47 r57 r67 r77 */ \
} while (0)

VEC_BASIC_OPERATIONS(v8f, float, VEC_INITSPLAT8, [0])
VEC_BASIC_OPERATIONS(v4d, double, VEC_INITSPLAT4, [0])

VEC_FUNCTION_ATTRIBUTES v4d   v4d_max(v4d a, v4d b) { return (v4d){a[0] > b[0] ? a[0] : b[0], a[1] > b[1] ? a[1] : b[1], a[2] > b[2] ? a[2] : b[2], a[3] > b[3] ? a[3] : b[3]}; }
VEC_FUNCTION_ATTRIBUTES v4d   v4d_min(v4d a, v4d b) { return (v4d){a[0] < b[0] ? a[0] : b[0], a[1] < b[1] ? a[1] : b[1], a[2] < b[2] ? a[2] : b[2], a[3] < b[3] ? a[3] : b[3]}; }
VEC_FUNCTION_ATTRIBUTES v8f   v8f_max(v8f a, v8f b) { return (v8f){a[0] > b[0] ? a[0] : b[0], a[1] > b[1] ? a[1] : b[1], a[2] > b[2] ? a[2] : b[2], a[3] > b[3] ? a[3] : b[3], a[4] > b[4] ? a[4] : b[4], a[5] > b[5] ? a[5] : b[5], a[6] > b[6] ? a[6] : b[6], a[7] > b[7] ? a[7] : b[7]}; }
VEC_FUNCTION_ATTRIBUTES v8f   v8f_min(v8f a, v8f b) { return (v8f){a[0] < b[0] ? a[0] : b[0], a[1] < b[1] ? a[1] : b[1], a[2] < b[2] ? a[2] : b[2], a[3] < b[3] ? a[3] : b[3], a[4] < b[4] ? a[4] : b[4], a[5] < b[5] ? a[5] : b[5], a[6] < b[6] ? a[6] : b[6], a[7] < b[7] ? a[7] : b[7]}; }

#endif /* defined(__clang__) && defined(__AVX__) */
#endif /* !VEC_DISABLE_COMPILER_BUILTINS */

/* Section 2) Platform specific vector implementations */

/* This macro is used by the X86/64 SSE vector implementations to construct as
 * many of the basic operations as possible. This is made possible by a
 * consistent naming of intrinsic functions. If none of the SSE headers are
 * found, don't worry, this macro will not be used - it's just easier to
 * define it here. */
#define VEC_SSE_BASIC_OPERATIONS(type_, data_, mmtyp_) \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _add(type_ a, type_ b)          { return _mm_add_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _sub(type_ a, type_ b)          { return _mm_sub_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _mul(type_ a, type_ b)          { return _mm_mul_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _ld(const void *ptr)            { return _mm_load_p ## mmtyp_((const data_ *)ptr); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _lde0(type_ a, const void *ptr) { return _mm_move_s ## mmtyp_(a, _mm_load_s ## mmtyp_((const data_ *)ptr)); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _broadcast(data_ a)             { return _mm_set1_p ## mmtyp_(a); } \
VEC_FUNCTION_ATTRIBUTES void  type_ ## _st(void *ptr, type_ val)       { _mm_store_p ## mmtyp_((data_ *)ptr, val); } \
VEC_FUNCTION_ATTRIBUTES void  type_ ## _ste0(void *ptr, type_ val)     { _mm_store_s ## mmtyp_((data_ *)ptr, val); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _neg(type_ a)                   { return _mm_xor_p ## mmtyp_(a, _mm_set1_p ## mmtyp_((data_)(-0.0))); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _min(type_ a, type_ b)          { return _mm_min_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _max(type_ a, type_ b)          { return _mm_max_p ## mmtyp_(a, b); }

#define VEC_AVX_BASIC_OPERATIONS(type_, data_, mmtyp_) \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _add(type_ a, type_ b)          { return _mm256_add_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _sub(type_ a, type_ b)          { return _mm256_sub_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _mul(type_ a, type_ b)          { return _mm256_mul_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _ld(const void *ptr)            { return _mm256_load_p ## mmtyp_(ptr); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _lde0(type_ a, const void *ptr) { return _mm256_blend_p ## mmtyp_(a, _mm256_broadcast_s ## mmtyp_(ptr), 1); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _broadcast(data_ a)             { return _mm256_broadcast_s ## mmtyp_(&a); } \
VEC_FUNCTION_ATTRIBUTES void  type_ ## _st(void *ptr, type_ val)       { _mm256_store_p ## mmtyp_(ptr, val); } \
VEC_FUNCTION_ATTRIBUTES void  type_ ## _ste0(void *ptr, type_ val)     { _mm_store_s ## mmtyp_(ptr, _mm256_extractf128_p ## mmtyp_(val, 0)); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _neg(type_ a)                   { static const data_ nz = -((data_)0.0); return _mm256_xor_p ## mmtyp_(a, _mm256_broadcast_s ## mmtyp_(&nz)); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _min(type_ a, type_ b)          { return _mm256_min_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES type_ type_ ## _max(type_ a, type_ b)          { return _mm256_max_p ## mmtyp_(a, b); }

/* Look for SSE to provide implementations of v4f and v2d. */
#if ((defined(_MSC_VER) && defined(_M_X64)) || ((defined(__clang__) || defined(__GNUC__)) && defined(__SSE__))) && (!defined(V4F_EXISTS) || !defined(V2D_EXISTS))

#if defined(_MSC_VER)
#include "intrin.h"
#else
#include "xmmintrin.h"
#endif

#ifndef V4F_EXISTS
#define V4F_EXISTS (1)

typedef __m128 v4f;
VEC_SSE_BASIC_OPERATIONS(v4f, float, s)
VEC_FUNCTION_ATTRIBUTES v4f v4f_reverse(v4f a)     { return _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 1, 2, 3)); }
#if 0
#include "tmmintrin.h"
VEC_FUNCTION_ATTRIBUTES v4f v4f_rotl(v4f a, v4f b) { return _mm_castsi128_ps(_mm_alignr_epi8(_mm_castps_si128(b), _mm_castps_si128(a), 4)); }
#else
VEC_FUNCTION_ATTRIBUTES v4f v4f_rotl(v4f a, v4f b) { a = _mm_move_ss(a, b); return _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 3, 2, 1)); }
#endif

#define V4F_INTERLEAVE(out1_, out2_, in1_, in2_) \
do { \
	(out1_) = _mm_unpacklo_ps((in1_), (in2_)); \
	(out2_) = _mm_unpackhi_ps((in1_), (in2_)); \
} while (0)

#define V4F_DEINTERLEAVE(out1_, out2_, in1_, in2_) \
do { \
	(out1_) = _mm_shuffle_ps((in1_), (in2_), _MM_SHUFFLE(2, 0, 2, 0)); \
	(out2_) = _mm_shuffle_ps((in1_), (in2_), _MM_SHUFFLE(3, 1, 3, 1)); \
} while (0)

#endif /* V4F_EXISTS */

#ifndef V2D_EXISTS
#define V2D_EXISTS (1)

typedef __m128d v2d;
VEC_SSE_BASIC_OPERATIONS(v2d, double, d)
VEC_FUNCTION_ATTRIBUTES v2d v2d_reverse(v2d a)     { return _mm_shuffle_pd(a, a, _MM_SHUFFLE2(0, 1)); }
VEC_FUNCTION_ATTRIBUTES v2d v2d_rotl(v2d a, v2d b) { return _mm_shuffle_pd(a, b, _MM_SHUFFLE2(0, 1)); }

#define V2D_INTERLEAVE(out1_, out2_, in1_, in2_) \
do { \
	(out1_) = _mm_unpacklo_pd((in1_), (in2_)); \
	(out2_) = _mm_unpackhi_pd((in1_), (in2_)); \
} while (0)
#define V2D_DEINTERLEAVE V2D_INTERLEAVE

#endif /* V2D_EXISTS */

#endif

/* Look for AVX to provide implementations of v8f and v4d. */
#if defined(__clang__) && defined(__AVX__) && (!defined(V8F_EXISTS) || !defined(V4D_EXISTS))

#include "immintrin.h"

#ifndef V8F_EXISTS
#define V8F_EXISTS (1)

typedef __m256 v8f;
VEC_AVX_BASIC_OPERATIONS(v8f, float, s)

VEC_FUNCTION_ATTRIBUTES v8f v8f_reverse(v8f a)
{
	a = _mm256_permute2f128_ps(a, a, 0x01);            /* a4 a5 a6 a7 a0 a1 a2 a3 */
	a = _mm256_permute_ps(a, _MM_SHUFFLE(0, 1, 2, 3)); /* a7 a6 a5 a4 a3 a2 a1 a0 */
	return a;
}
VEC_FUNCTION_ATTRIBUTES v8f v8f_rotl(v8f a, v8f b)
{
	b = _mm256_blend_ps(a, b, 0x01);                      /* b = b0 a1 a2 a3 a4 a5 a6 a7 */
	b = _mm256_permute2f128_ps(b, b, 0x01);               /* b = a4 a5 a6 a7 b0 a1 a2 a3 */
	b = _mm256_blend_ps(a, b, 0x11);                      /* b = a4 a1 a2 a3 b0 a5 a6 a7 */
	b = _mm256_shuffle_ps(b, b, _MM_SHUFFLE(0, 3, 2, 1)); /* b = a1 a2 a3 a4 a5 a6 a7 b0 */
	return b;
}

#define V8F_DEINTERLEAVE(out1_, out2_, in1_, in2_) do { \
	v8f Z1_, Z2_; \
	Z1_   = _mm256_permute2f128_ps((in1_), (in2_), 0x20);             /* a0, a1, a2, a3, b0, b1, b2, b3 */ \
	Z2_   = _mm256_permute2f128_ps((in1_), (in2_), 0x31);             /* a4, a5, a6, a7, b4, b5, b6, b7 */ \
	out1_ = _mm256_shuffle_ps((Z1_), (Z2_), _MM_SHUFFLE(2, 0, 2, 0)); /* a0, a2, a4, a6, b0, b2, b4, b6 */ \
	out2_ = _mm256_shuffle_ps((Z1_), (Z2_), _MM_SHUFFLE(3, 1, 3, 1)); /* a1, a3, a5, a7, b1, b3, b5, b7 */ \
} while (0)

#define V8F_INTERLEAVE(out1_, out2_, in1_, in2_) do { \
	v8f Z1_, Z2_; \
	Z1_   = _mm256_unpacklo_ps((in1_), (in2_));         /* a0, b0, a1, b1, a4, b4, a5, b5 */ \
	Z2_   = _mm256_unpackhi_ps((in1_), (in2_));         /* a2, b2, a3, b3, a6, b6, a7, b7 */ \
	out1_ = _mm256_permute2f128_ps((Z1_), (Z2_), 0x20); /* a0, b0, a1, b1, a2, b2, a3, b3 */ \
	out2_ = _mm256_permute2f128_ps((Z1_), (Z2_), 0x31); /* a4, b4, a5, b5, a6, b6, a7, b7 */ \
} while (0)

#define V8F_TRANSPOSE_INPLACE(r0_, r1_, r2_, r3_, r4_, r5_, r6_, r7_) do { \
	v8f t0_, t1_, t2_, t3_, t4_, t5_, t6_, t7_; \
	v8f z0_, z1_, z2_, z3_, z4_, z5_, z6_, z7_; \
	t0_ = _mm256_unpacklo_ps(r0_, r1_); /* r00 r10 r01 r11 r04 r14 r05 r15 */ \
	t1_ = _mm256_unpackhi_ps(r0_, r1_); /* r02 r12 r03 r13 r06 r16 r07 r17 */ \
	t2_ = _mm256_unpacklo_ps(r2_, r3_); /* r20 r30 r21 r31 r24 r34 r25 r35 */ \
	t3_ = _mm256_unpackhi_ps(r2_, r3_); /* r22 r32 r23 r33 r26 r36 r27 r37 */ \
	t4_ = _mm256_unpacklo_ps(r4_, r5_); /* r40 r50 r41 r51 r44 r54 r45 r55 */ \
	t5_ = _mm256_unpackhi_ps(r4_, r5_); /* r42 r52 r43 r53 r46 r56 r47 r57 */ \
	t6_ = _mm256_unpacklo_ps(r6_, r7_); /* r60 r70 r61 r71 r64 r74 r65 r75 */ \
	t7_ = _mm256_unpackhi_ps(r6_, r7_); /* r62 r72 r63 r73 r66 r76 r67 r77 */ \
	z0_ = _mm256_shuffle_ps(t0_, t2_, _MM_SHUFFLE(1, 0, 1, 0)); /* r00 r10 r20 r30 r04 r14 r24 r34 */ \
	z1_ = _mm256_shuffle_ps(t0_, t2_, _MM_SHUFFLE(3, 2, 3, 2)); /* r01 r11 r21 r31 r05 r15 r25 r35 */ \
	z2_ = _mm256_shuffle_ps(t1_, t3_, _MM_SHUFFLE(1, 0, 1, 0)); /* r02 r12 r22 r32 r06 r16 r26 r36 */ \
	z3_ = _mm256_shuffle_ps(t1_, t3_, _MM_SHUFFLE(3, 2, 3, 2)); /* r03 r13 r23 r33 r07 r17 r27 r37 */ \
	z4_ = _mm256_shuffle_ps(t4_, t6_, _MM_SHUFFLE(1, 0, 1, 0)); /* r40 r50 r60 r70 r44 r54 r64 r74 */ \
	z5_ = _mm256_shuffle_ps(t4_, t6_, _MM_SHUFFLE(3, 2, 3, 2)); /* r41 r51 r61 r71 r45 r55 r65 r75 */ \
	z6_ = _mm256_shuffle_ps(t5_, t7_, _MM_SHUFFLE(1, 0, 1, 0)); /* r42 r52 r62 r72 r46 r56 r66 r76 */ \
	z7_ = _mm256_shuffle_ps(t5_, t7_, _MM_SHUFFLE(3, 2, 3, 2)); /* r43 r53 r63 r73 r47 r57 r67 r77 */ \
	r0_ = _mm256_permute2f128_ps(z0_, z4_, 0x20); /* r00 r10 r20 r30 r40 r50 r60 r70 */ \
	r1_ = _mm256_permute2f128_ps(z1_, z5_, 0x20); /* r01 r11 r21 r31 r41 r51 r61 r71 */ \
	r2_ = _mm256_permute2f128_ps(z2_, z6_, 0x20); /* r02 r12 r22 r32 r42 r52 r62 r72 */ \
	r3_ = _mm256_permute2f128_ps(z3_, z7_, 0x20); /* r03 r13 r23 r33 r43 r53 r63 r73 */ \
	r4_ = _mm256_permute2f128_ps(z0_, z4_, 0x31); /* r04 r14 r24 r34 r44 r54 r64 r74 */ \
	r5_ = _mm256_permute2f128_ps(z1_, z5_, 0x31); /* r05 r15 r25 r35 r45 r55 r65 r75 */ \
	r6_ = _mm256_permute2f128_ps(z2_, z6_, 0x31); /* r06 r16 r26 r36 r46 r56 r66 r76 */ \
	r7_ = _mm256_permute2f128_ps(z3_, z7_, 0x31); /* r07 r17 r27 r37 r47 r57 r67 r77 */ \
} while (0)

#endif /* V8F_EXISTS */

#ifndef V4D_EXISTS
#define V4D_EXISTS (1)

typedef __m256d v4d;
VEC_AVX_BASIC_OPERATIONS(v4d, double, d)

VEC_FUNCTION_ATTRIBUTES v4d v4d_reverse(v4d a)
{
	a = _mm256_permute2f128_pd(a, a, 0x01); /* a2 a3 a0 a1 */
	a = _mm256_permute_pd(a, 0x05);         /* a3 a2 a1 a0 */
	return a;
}
VEC_FUNCTION_ATTRIBUTES v4d v4d_rotl(v4d a, v4d b)
{
	b = _mm256_blend_pd(a, b, 0x01);         /* b = b0 a1 a2 a3 */
	b = _mm256_permute2f128_pd(b, b, 0x01);  /* b = a2 a3 b0 a1 */
	a = _mm256_shuffle_pd(a, b, 0x05);       /* a = a1 a2 a3 b0 */
	return a;
}

#define V4D_DEINTERLEAVE(out1_, out2_, in1_, in2_) do { \
	v4d Z1_, Z2_; \
	Z1_   = _mm256_permute2f128_pd((in1_), (in2_), 0x20); /* a0, a1, b0, b1 */ \
	Z2_   = _mm256_permute2f128_pd((in1_), (in2_), 0x31); /* a2, a3, b2, b3 */ \
	out1_ = _mm256_unpacklo_pd((Z1_), (Z2_));             /* a0, a2, b0, b2 */ \
	out2_ = _mm256_unpackhi_pd((Z1_), (Z2_));             /* a1, a3, b1, b3 */ \
} while (0)

#define V4D_INTERLEAVE(out1_, out2_, in1_, in2_) do { \
	v4d Z1_, Z2_; \
	Z1_   = _mm256_unpacklo_pd((in1_), (in2_));         /* a0, b0, a2, b2 */ \
	Z2_   = _mm256_unpackhi_pd((in1_), (in2_));         /* a1, b1, a3, b3 */ \
	out1_ = _mm256_permute2f128_pd((Z1_), (Z2_), 0x20); /* a0, a1, b0, b1 */ \
	out2_ = _mm256_permute2f128_pd((Z1_), (Z2_), 0x31); /* a2, a3, b2, b3 */ \
} while (0)

#endif /* V4D_EXISTS */

#endif

/* Look for NEON to provide implementation of v4f. */
#if (defined(__clang__) || defined(__GNUC__)) && defined(__ARM_NEON__) && !defined(V4F_EXISTS)

#include "arm_neon.h"

#ifndef V4F_EXISTS
#define V4F_EXISTS (1)

typedef float32x4_t v4f;

VEC_FUNCTION_ATTRIBUTES void v4f_ste0(void *ptr, v4f val) { vst1q_f32(ptr, val); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_add(v4f a, v4f b)      { return vaddq_f32(a, b); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_sub(v4f a, v4f b)      { return vsubq_f32(a, b); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_mul(v4f a, v4f b)      { return vmulq_f32(a, b); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_ld(const void *ptr)    { return vld1q_f32(ptr); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_lde0(v4f a, const void *ptr) { return vld1q_lane_f32(ptr, a, 0); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_broadcast(float a)     { return vld1q_dup_f32(&a); }
VEC_FUNCTION_ATTRIBUTES void v4f_st(void *ptr, v4f val) { vst1q_f32(ptr, val); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_neg(v4f a)             { return vnegq_f32(a); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_reverse(v4f a)         { v4f b = vrev64q_f32(a); return vcombine_f32(vget_high_f32(b), vget_low_f32(b)); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_rotl(v4f a, v4f b)     { return vreinterpretq_f32_u32(vextq_u32(vreinterpretq_u32_f32(a), vreinterpretq_u32_f32(b), 1)); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_min(v4f a, v4f b)      { return vminq_f32(a, b); }
VEC_FUNCTION_ATTRIBUTES v4f  v4f_max(v4f a, v4f b)      { return vmaxq_f32(a, b); }

#if 0
/* NEON should be able to do a double load with a single vld1.32 */
#define V4F_LD2(r0_, r1_, src_) do { \
	r0_ = v4f_ld(((const float *)(src_)) + 0); \
	r1_ = v4f_ld(((const float *)(src_)) + 4); \
} while (0)
#endif

#define V4F_LD2DINT(r0_, r1_, src_) do { \
	float32x4x2_t tmp_ = vld2q_f32(src_); \
	r0_ = tmp_.val[0]; \
	r1_ = tmp_.val[1]; \
} while (0)

#define V4F_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_) do { \
	V4F_LD2DINT(r00_, r01_, src0_); \
	V4F_LD2DINT(r10_, r11_, src1_); \
} while (0)

#define V4F_INTERLEAVE(out1_, out2_, in1_, in2_) do { \
	float32x4x2_t tmp_ = vzipq_f32(in1_, in2_); \
	out1_ = tmp_.val[0]; \
	out2_ = tmp_.val[1]; \
} while (0)

#define V4F_DEINTERLEAVE(out1_, out2_, in1_, in2_) do { \
	float32x4x2_t tmp_ = vuzpq_f32(in1_, in2_); \
	out1_ = tmp_.val[0]; \
	out2_ = tmp_.val[1]; \
} while (0)

#endif /* V4F_EXISTS */

#endif

/* Section 3) Generic macro implementations
 * -----------------------------------------------
 * The only required macros to exist at this point are *_INTERLEAVE and
 * *_DEINTERLEAVE. All other macros can be defined based on these. We do not
 * want to force backends to define custom implementations for all of the
 * various transposes/interleaves/double-loads etc, when the generic case
 * will perform just as well. This minimises the overall code required for
 * each vector implementation. The following macros will be defined if they
 * have not already been for each vector type:
 *
 *   - *_INTERLEAVE2       - Interleave two vector pairs
 *   - *_DEINTERLEAVE2     - Deinterleave two vector pairs
 *   - *_LD2               - Double vector load
 *   - *_ST2               - Double vector store
 *   - *_LD2DINT           - Deinterleaving double vector load
 *   - *_ST2INT            - Interleaving double vector store
 *   - *_LD2X2DINT         - 2 independent deinterleaving double vector loads
 *   - *_ST2X2INT          - 2 independent interleaving double vector stores
 *   - *_TRANSPOSE_INPLACE - Vector transposition */

#if defined(V4F_EXISTS) && !defined(V4F_INTERLEAVE2)
#define V4F_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_)   do { V4F_INTERLEAVE(out1_, out2_, in1_, in2_); V4F_INTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_DEINTERLEAVE2)
#define V4F_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { V4F_DEINTERLEAVE(out1_, out2_, in1_, in2_); V4F_DEINTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_LD2)
#define V4F_LD2(r0_, r1_, src_)                              do { r0_ = v4f_ld(((const float *)(src_)) + 0); r1_ = v4f_ld(((const float *)(src_)) + 4); } while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_ST2)
#define V4F_ST2(dest_, r0_, r1_)                             do { v4f_st(((float *)(dest_)) + 0, r0_); v4f_st(((float *)(dest_)) + 4, r1_); } while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_LD2DINT)
#define V4F_LD2DINT(r0_, r1_, src_)                          do { v4f tmp1_, tmp2_; V4F_LD2(tmp1_, tmp2_, src_); V4F_DEINTERLEAVE(r0_, r1_, tmp1_, tmp2_); } while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_ST2INT)
#define V4F_ST2INT(dest_, in1_, in2_)                        do { v4f tmp0_, tmp1_; V4F_INTERLEAVE(tmp0_, tmp1_, in1_, in2_); V4F_ST2(dest_, tmp0_, tmp1_); } while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_LD2X2DINT)
#define V4F_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_)  do { v4f tmp1_, tmp2_, tmp3_, tmp4_; V4F_LD2(tmp1_, tmp2_, src0_); V4F_LD2(tmp3_, tmp4_, src1_); V4F_DEINTERLEAVE2(r00_, r01_, r10_, r11_, tmp1_, tmp2_, tmp3_, tmp4_); } while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_ST2X2INT)
#define V4F_ST2X2INT(dest0_, dest1_, r00_, r01_, r10_, r11_) do { v4f tmp1_, tmp2_, tmp3_, tmp4_; V4F_INTERLEAVE2(tmp1_, tmp2_, tmp3_, tmp4_, r00_, r01_, r10_, r11_); V4F_ST2(dest0_, tmp1_, tmp2_); V4F_ST2(dest1_, tmp3_, tmp4_); } while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_TRANSPOSE_INPLACE)
#define V4F_TRANSPOSE_INPLACE(r1_, r2_, r3_, r4_) do { \
	v4f t1_, t2_, t3_, t4_; \
	V4F_INTERLEAVE2(t1_, t2_, t3_, t4_, r1_, r3_, r2_, r4_); \
	V4F_INTERLEAVE2(r1_, r2_, r3_, r4_, t1_, t3_, t2_, t4_); \
} while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_HMAX_DEFINED)
VEC_FUNCTION_ATTRIBUTES float v4f_hmax(v4f a)
{
	float VEC_ALIGN_BEST st[4];
	v4f x, y;
	V4F_INTERLEAVE(x, y, a, a);
	x = v4f_max(x, y);
	V4F_INTERLEAVE(a, y, x, x);
	a = v4f_max(a, y);
	v4f_st(st, a);
	return st[0];
}
#else
#undef V4F_HMAX_DEFINED
#endif
#if defined(V4F_EXISTS) && !defined(V4F_HMIN_DEFINED)
VEC_FUNCTION_ATTRIBUTES float v4f_hmin(v4f a)
{
	float VEC_ALIGN_BEST st[4];
	v4f x, y;
	V4F_INTERLEAVE(x, y, a, a);
	x = v4f_min(x, y);
	V4F_INTERLEAVE(a, y, x, x);
	a = v4f_min(a, y);
	v4f_st(st, a);
	return st[0];
}
#else
#undef V4F_HMIN_DEFINED
#endif

#if defined(V2D_EXISTS) && !defined(V2D_INTERLEAVE2)
#define V2D_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_)   do { V2D_INTERLEAVE(out1_, out2_, in1_, in2_); V2D_INTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_DEINTERLEAVE2)
#define V2D_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { V2D_DEINTERLEAVE(out1_, out2_, in1_, in2_); V2D_DEINTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_LD2)
#define V2D_LD2(r0_, r1_, src_)                              do { r0_ = v2d_ld(((const double *)(src_)) + 0); r1_ = v2d_ld(((const double *)(src_)) + 2); } while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_ST2)
#define V2D_ST2(dest_, r0_, r1_)                             do { v2d_st(((double *)(dest_)) + 0, r0_); v2d_st(((double *)(dest_)) + 2, r1_); } while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_LD2DINT)
#define V2D_LD2DINT(r0_, r1_, src_)                          do { v2d tmp1_, tmp2_; V2D_LD2(tmp1_, tmp2_, src_); V2D_DEINTERLEAVE(r0_, r1_, tmp1_, tmp2_); } while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_ST2INT)
#define V2D_ST2INT(dest_, r0_, r1_)                          do { v2d tmp1_, tmp2_; V2D_INTERLEAVE(tmp1_, tmp2_, r0_, r1_); V2D_ST2(dest_, tmp1_, tmp2_); } while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_LD2X2DINT)
#define V2D_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_)  do { v2d tmp1_, tmp2_, tmp3_, tmp4_; V2D_LD2(tmp1_, tmp2_, src0_); V2D_LD2(tmp3_, tmp4_, src1_); V2D_DEINTERLEAVE2(r00_, r01_, r10_, r11_, tmp1_, tmp2_, tmp3_, tmp4_); } while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_ST2X2INT)
#define V2D_ST2X2INT(dest0_, dest1_, r00_, r01_, r10_, r11_) do { v2d tmp1_, tmp2_, tmp3_, tmp4_; V2D_INTERLEAVE2(tmp1_, tmp2_, tmp3_, tmp4_, r00_, r01_, r10_, r11_); V2D_ST2(dest0_, tmp1_, tmp2_); V2D_ST2(dest1_, tmp3_, tmp4_); } while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_TRANSPOSE_INPLACE)
#define V2D_TRANSPOSE_INPLACE(r1_, r2_) do { \
	v2d t1_, t2_; \
	V2D_INTERLEAVE(t1_, t2_, r1_, r2_); \
	r1_ = t1_; \
	r2_ = t2_; \
} while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_HMAX_DEFINED)
VEC_FUNCTION_ATTRIBUTES double v2d_hmax(v2d a)
{
	double VEC_ALIGN_BEST st[2];
	v2d x, y;
	V2D_INTERLEAVE(x, y, a, a);
	a = v2d_max(x, y);
	v2d_st(st, a);
	return st[0];
}
#else
#undef V2D_HMAX_DEFINED
#endif
#if defined(V2D_EXISTS) && !defined(V2D_HMIN_DEFINED)
VEC_FUNCTION_ATTRIBUTES double v2d_hmin(v2d a)
{
	double VEC_ALIGN_BEST st[2];
	v2d x, y;
	V2D_INTERLEAVE(x, y, a, a);
	a = v2d_min(x, y);
	v2d_st(st, a);
	return st[0];
}
#else
#undef V2D_HMIN_DEFINED
#endif

#if defined(V8F_EXISTS) && !defined(V8F_INTERLEAVE2)
#define V8F_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { V8F_INTERLEAVE(out1_, out2_, in1_, in2_); V8F_INTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_DEINTERLEAVE2)
#define V8F_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { V8F_DEINTERLEAVE(out1_, out2_, in1_, in2_); V8F_DEINTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_LD2)
#define V8F_LD2(r0_, r1_, src_)                              do { r0_ = v8f_ld(((const float *)(src_)) + 0); r1_ = v8f_ld(((const float *)(src_)) + 8); } while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_ST2)
#define V8F_ST2(dest_, r0_, r1_)                             do { v8f_st(((float *)(dest_)) + 0, r0_); v8f_st(((float *)(dest_)) + 8, r1_); } while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_LD2DINT)
#define V8F_LD2DINT(r0_, r1_, src_)                          do { v8f tmp1_, tmp2_; V8F_LD2(tmp1_, tmp2_, src_); V8F_DEINTERLEAVE(r0_, r1_, tmp1_, tmp2_); } while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_ST2INT)
#define V8F_ST2INT(dest_, in1_, in2_)                        do { v8f tmp0_, tmp1_; V8F_INTERLEAVE(tmp0_, tmp1_, in1_, in2_); V8F_ST2(dest_, tmp0_, tmp1_); } while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_LD2X2DINT)
#define V8F_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_)  do { v8f tmp1_, tmp2_, tmp3_, tmp4_; V8F_LD2(tmp1_, tmp2_, src0_); V8F_LD2(tmp3_, tmp4_, src1_); V8F_DEINTERLEAVE2(r00_, r01_, r10_, r11_, tmp1_, tmp2_, tmp3_, tmp4_); } while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_ST2X2INT)
#define V8F_ST2X2INT(dest0_, dest1_, r00_, r01_, r10_, r11_) do { v8f tmp1_, tmp2_, tmp3_, tmp4_; V8F_INTERLEAVE2(tmp1_, tmp2_, tmp3_, tmp4_, r00_, r01_, r10_, r11_); V8F_ST2(dest0_, tmp1_, tmp2_); V8F_ST2(dest1_, tmp3_, tmp4_); } while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_TRANSPOSE_INPLACE)
#define V8F_TRANSPOSE_INPLACE(r1_, r2_, r3_, r4_, r5_, r6_, r7_, r8_) do { \
	v8f t1_, t2_, t3_, t4_, t5_, t6_, t7_, t8_; \
	v8f z1_, z2_, z3_, z4_, z5_, z6_, z7_, z8_; \
	V8F_INTERLEAVE2(t1_, t2_, t3_, t4_, r1_, r5_, r2_, r6_); \
	V8F_INTERLEAVE2(t5_, t6_, t7_, t8_, r3_, r7_, r4_, r8_); \
	V8F_INTERLEAVE2(z1_, z2_, z3_, z4_, t1_, t5_, t2_, t6_); \
	V8F_INTERLEAVE2(z5_, z6_, z7_, z8_, t3_, t7_, t4_, t8_); \
	V8F_INTERLEAVE2(r1_, r2_, r3_, r4_, z1_, z5_, z2_, z6_); \
	V8F_INTERLEAVE2(r5_, r6_, r7_, r8_, z3_, z7_, z4_, z8_); \
} while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_HMAX_DEFINED)
VEC_FUNCTION_ATTRIBUTES float v8f_hmax(v8f a)
{
	float VEC_ALIGN_BEST st[8];
	v8f x, y;
	V8F_INTERLEAVE(x, y, a, a);
	a = v8f_max(x, y);
	V8F_INTERLEAVE(x, y, a, a);
	a = v8f_max(x, y);
	V8F_INTERLEAVE(x, y, a, a);
	a = v8f_max(x, y);
	v8f_st(st, a);
	return st[0];
}
#else
#undef V8F_HMAX_DEFINED
#endif
#if defined(V8F_EXISTS) && !defined(V8F_HMIN_DEFINED)
VEC_FUNCTION_ATTRIBUTES float v8f_hmin(v8f a)
{
	float VEC_ALIGN_BEST st[8];
	v8f x, y;
	V8F_INTERLEAVE(x, y, a, a);
	a = v8f_min(x, y);
	V8F_INTERLEAVE(x, y, a, a);
	a = v8f_min(x, y);
	V8F_INTERLEAVE(x, y, a, a);
	a = v8f_min(x, y);
	v8f_st(st, a);
	return st[0];
}
#else
#undef V8F_HMIN_DEFINED
#endif


#if defined(V4D_EXISTS) && !defined(V4D_INTERLEAVE2)
#define V4D_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_)   do { V4D_INTERLEAVE(out1_, out2_, in1_, in2_); V4D_INTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_DEINTERLEAVE2)
#define V4D_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { V4D_DEINTERLEAVE(out1_, out2_, in1_, in2_); V4D_DEINTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_LD2)
#define V4D_LD2(r0_, r1_, src_)                              do { r0_ = v4d_ld(((const double *)(src_)) + 0); r1_ = v4d_ld(((const double *)(src_)) + 4); } while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_ST2)
#define V4D_ST2(dest_, r0_, r1_)                             do { v4d_st(((double *)(dest_)) + 0, r0_); v4d_st(((double *)(dest_)) + 4, r1_); } while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_LD2DINT)
#define V4D_LD2DINT(r0_, r1_, src_)                          do { v4d tmp1_, tmp2_; V4D_LD2(tmp1_, tmp2_, src_); V4D_DEINTERLEAVE(r0_, r1_, tmp1_, tmp2_); } while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_ST2INT)
#define V4D_ST2INT(dest_, r0_, r1_)                          do { v4d tmp1_, tmp2_; V4D_INTERLEAVE(tmp1_, tmp2_, r0_, r1_); V4D_ST2(dest_, tmp1_, tmp2_); } while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_LD2X2DINT)
#define V4D_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_)  do { v4d tmp1_, tmp2_, tmp3_, tmp4_; V4D_LD2(tmp1_, tmp2_, src0_); V4D_LD2(tmp3_, tmp4_, src1_); V4D_DEINTERLEAVE2(r00_, r01_, r10_, r11_, tmp1_, tmp2_, tmp3_, tmp4_); } while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_ST2X2INT)
#define V4D_ST2X2INT(dest0_, dest1_, r00_, r01_, r10_, r11_) do { v4d tmp1_, tmp2_, tmp3_, tmp4_; V4D_INTERLEAVE2(tmp1_, tmp2_, tmp3_, tmp4_, r00_, r01_, r10_, r11_); V4D_ST2(dest0_, tmp1_, tmp2_); V4D_ST2(dest1_, tmp3_, tmp4_); } while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_TRANSPOSE_INPLACE)
#define V4D_TRANSPOSE_INPLACE(r1_, r2_, r3_, r4_) do { \
	v4d t1_, t2_, t3_, t4_; \
	V4D_INTERLEAVE2(t1_, t2_, t3_, t4_, r1_, r3_, r2_, r4_); \
	V4D_INTERLEAVE2(r1_, r2_, r3_, r4_, t1_, t3_, t2_, t4_); \
} while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_HMAX_DEFINED)
VEC_FUNCTION_ATTRIBUTES double v4d_hmax(v4d a)
{
	double VEC_ALIGN_BEST st[4];
	v4d x, y;
	V4D_INTERLEAVE(x, y, a, a);
	x = v4d_max(x, y);
	V4D_INTERLEAVE(a, y, x, x);
	a = v4d_max(a, y);
	v4d_st(st, a);
	return st[0];
}
#else
#undef V4D_HMAX_DEFINED
#endif
#if defined(V4D_EXISTS) && !defined(V4D_HMIN_DEFINED)
VEC_FUNCTION_ATTRIBUTES double v4d_hmin(v4d a)
{
	double VEC_ALIGN_BEST st[4];
	v4d x, y;
	V4D_INTERLEAVE(x, y, a, a);
	x = v4d_min(x, y);
	V4D_INTERLEAVE(a, y, x, x);
	a = v4d_min(a, y);
	v4d_st(st, a);
	return st[0];
}
#else
#undef V4D_HMIN_DEFINED
#endif

/* Build the "scalar" v1d and v1f vectors. These really only exist to make
 * pre-processor code generation work nicely. Bury your feelings somewhere
 * other than my email address. */
typedef float  v1f;
#define VEC_NOTHING
VEC_BASIC_OPERATIONS(v1f, float,  VEC_INITSPLAT1, VEC_NOTHING)
VEC_FUNCTION_ATTRIBUTES v1f v1f_min(v1f a, v1f b)            { return a < b ? a : b; }
VEC_FUNCTION_ATTRIBUTES v1f v1f_max(v1f a, v1f b)            { return a > b ? a : b; }
VEC_FUNCTION_ATTRIBUTES v1f v1f_rotl(v1f a, v1f b)           { (void)a; return b; }
VEC_FUNCTION_ATTRIBUTES v1f v1f_reverse(v1f a)               { return a; }
VEC_FUNCTION_ATTRIBUTES v1f v1f_hmin(v1f a)                  { return a; }
VEC_FUNCTION_ATTRIBUTES v1f v1f_hmax(v1f a)                  { return a; }
#define V1F_INTERLEAVE(out1_, out2_, in1_, in2_)             do { out1_ = in1_; out2_ = in2_; } while (0)
#define V1F_DEINTERLEAVE(out1_, out2_, in1_, in2_)           do { out1_ = in1_; out2_ = in2_; } while (0)
#define V1F_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_)   do { V1F_INTERLEAVE(out1_, out2_, in1_, in2_); V1F_INTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#define V1F_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { V1F_DEINTERLEAVE(out1_, out2_, in1_, in2_); V1F_DEINTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#define V1F_LD2(r0_, r1_, src_)                              do { r0_ = v1f_ld(((const float *)(src_)) + 0); r1_ = v1f_ld(((const float *)(src_)) + 1); } while (0)
#define V1F_LD2DINT(r0_, r1_, src_)                          V1F_LD2(r0_, r1_, src_)
#define V1F_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_)  do { V1F_LD2DINT(r00_, r01_, src0_); V1F_LD2DINT(r10_, r11_, src1_); } while (0)
#define V1F_ST2(dest_, r0_, r1_)                             do { v1f_st(((float *)(dest_)) + 0, r0_); v1f_st(((float *)(dest_)) + 1, r1_); } while (0)
#define V1F_ST2INT(dest_, r0_, r1_)                          V1F_ST2(dest_, r0_, r1_)
#define V1F_ST2X2INT(dest0_, dest1_, r00_, r01_, r10_, r11_) do { V1F_ST2INT(dest0_, r00_, r01_); V1F_ST2INT(dest1_, r10_, r11_); } while (0)

typedef double v1d;
VEC_BASIC_OPERATIONS(v1d, double, VEC_INITSPLAT1, VEC_NOTHING)
VEC_FUNCTION_ATTRIBUTES v1d v1d_min(v1d a, v1d b)            { return a < b ? a : b; }
VEC_FUNCTION_ATTRIBUTES v1d v1d_max(v1d a, v1d b)            { return a > b ? a : b; }
VEC_FUNCTION_ATTRIBUTES v1d v1d_rotl(v1d a, v1d b)           { (void)a; return b; }
VEC_FUNCTION_ATTRIBUTES v1d v1d_reverse(v1d a)               { return a; }
VEC_FUNCTION_ATTRIBUTES v1d v1d_hmin(v1d a)                  { return a; }
VEC_FUNCTION_ATTRIBUTES v1d v1d_hmax(v1d a)                  { return a; }
#define V1D_INTERLEAVE(out1_, out2_, in1_, in2_)             do { out1_ = in1_; out2_ = in2_; } while (0)
#define V1D_DEINTERLEAVE(out1_, out2_, in1_, in2_)           do { out1_ = in1_; out2_ = in2_; } while (0)
#define V1D_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_)   do { V1D_INTERLEAVE(out1_, out2_, in1_, in2_); V1D_INTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#define V1D_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { V1D_DEINTERLEAVE(out1_, out2_, in1_, in2_); V1D_DEINTERLEAVE(out3_, out4_, in3_, in4_); } while (0)
#define V1D_LD2(r0_, r1_, src_)                              do { r0_ = v1d_ld(((const double *)(src_)) + 0); r1_ = v1d_ld(((const double *)(src_)) + 1); } while (0)
#define V1D_LD2DINT(r0_, r1_, src_)                          V1D_LD2(r0_, r1_, src_)
#define V1D_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_)  do { V1D_LD2DINT(r00_, r01_, src0_); V1D_LD2DINT(r10_, r11_, src1_); } while (0)
#define V1D_ST2(dest_, r0_, r1_)                             do { v1d_st(((double *)(dest_)) + 0, r0_); v1d_st(((double *)(dest_)) + 1, r1_); } while (0)
#define V1D_ST2INT(dest_, r0_, r1_)                          V1D_ST2(dest_, r0_, r1_)
#define V1D_ST2X2INT(dest0_, dest1_, r00_, r01_, r10_, r11_) do { V1D_ST2INT(dest0_, r00_, r01_); V1D_ST2INT(dest1_, r10_, r11_); } while (0)

/* Don't need any of the macros which we used to construct operators anymore
 * so pull them out of the global namespace. */
#undef VEC_NOTHING
#undef VEC_BASIC_OPERATIONS
#undef VEC_SSE_BASIC_OPERATIONS
#undef VEC_AVX_BASIC_OPERATIONS
#undef VEC_FUNCTION_ATTRIBUTES

#if defined(V8F_EXISTS)
typedef v8f vlf;
#define VLF_WIDTH      (8)
#define VLF_HI_OP(op_) V8F_ ## op_
#define VLF_LO_OP(op_) v8f_ ## op_
#define VLF_PAD_LENGTH(len) ((len) + ((8u - ((len) & 7u)) & 7u))
#elif defined(V4F_EXISTS)
typedef v4f vlf;
#define VLF_WIDTH      (4)
#define VLF_HI_OP(op_) V4F_ ## op_
#define VLF_LO_OP(op_) v4f_ ## op_
#define VLF_PAD_LENGTH(len) ((len) + ((4u - ((len) & 3u)) & 3u))
#else
typedef v1f vlf;
#define VLF_WIDTH      (1)
#define VLF_HI_OP(op_) V1F_ ## op_
#define VLF_LO_OP(op_) v1f_ ## op_
#define VLF_PAD_LENGTH(len) (len)
#endif

#define vlf_st           VLF_LO_OP(st)
#define vlf_ste0         VLF_LO_OP(ste0)
#define vlf_ld           VLF_LO_OP(ld)
#define vlf_lde0         VLF_LO_OP(lde0)
#define vlf_broadcast    VLF_LO_OP(broadcast)
#define vlf_neg          VLF_LO_OP(neg)
#define vlf_rotl         VLF_LO_OP(rotl)
#define vlf_mul          VLF_LO_OP(mul)
#define vlf_add          VLF_LO_OP(add)
#define vlf_reverse      VLF_LO_OP(reverse)
#define vlf_sub          VLF_LO_OP(sub)
#define vlf_max          VLF_LO_OP(max)
#define vlf_min          VLF_LO_OP(min)
#define vlf_hmax         VLF_LO_OP(hmax)
#define vlf_hmin         VLF_LO_OP(hmin)
#define VLF_DEINTERLEAVE VLF_HI_OP(DEINTERLEAVE)
#define VLF_INTERLEAVE   VLF_HI_OP(INTERLEAVE)
#define VLF_LD2          VLF_HI_OP(LD2)
#define VLF_ST2          VLF_HI_OP(ST2)
#define VLF_LD2DINT      VLF_HI_OP(LD2DINT)
#define VLF_ST2INT       VLF_HI_OP(ST2INT)
#define VLF_LD2X2DINT    VLF_HI_OP(LD2X2DINT)
#define VLF_ST2X2INT     VLF_HI_OP(ST2X2INT)

#if defined(V4D_EXISTS)
typedef v4d vld;
#define VLD_WIDTH      (4)
#define VLD_HI_OP(op_) V4D_ ## op_
#define VLD_LO_OP(op_) v4d_ ## op_
#define VLD_PAD_LENGTH(len) ((len) + ((4u - ((len) & 3u)) & 3u))
#elif defined(V2D_EXISTS)
typedef v2d vld;
#define VLD_WIDTH      (2)
#define VLD_HI_OP(op_) V2D_ ## op_
#define VLD_LO_OP(op_) v2d_ ## op_
#define VLD_PAD_LENGTH(len) ((len) + ((2u - ((len) & 1u)) & 1u))
#else
typedef v1d vld;
#define VLD_WIDTH      (1)
#define VLD_HI_OP(op_) V1D_ ## op_
#define VLD_LO_OP(op_) v1d_ ## op_
#define VLD_PAD_LENGTH(len) (len)
#endif

#define vld_st           VLD_LO_OP(st)
#define vld_ste0         VLD_LO_OP(ste0)
#define vld_ld           VLD_LO_OP(ld)
#define vld_lde0         VLD_LO_OP(lde0)
#define vld_broadcast    VLD_LO_OP(broadcast)
#define vld_neg          VLD_LO_OP(neg)
#define vld_rotl         VLD_LO_OP(rotl)
#define vld_mul          VLD_LO_OP(mul)
#define vld_add          VLD_LO_OP(add)
#define vld_reverse      VLD_LO_OP(reverse)
#define vld_sub          VLD_LO_OP(sub)
#define vld_max          VLD_LO_OP(max)
#define vld_min          VLD_LO_OP(min)
#define vld_hmax         VLD_LO_OP(hmax)
#define vld_hmin         VLD_LO_OP(hmin)
#define VLD_DEINTERLEAVE VLD_HI_OP(DEINTERLEAVE)
#define VLD_INTERLEAVE   VLD_HI_OP(INTERLEAVE)
#define VLD_LD2          VLD_HI_OP(LD2)
#define VLD_ST2          VLD_HI_OP(ST2)
#define VLD_LD2DINT      VLD_HI_OP(LD2DINT)
#define VLD_ST2INT       VLD_HI_OP(ST2INT)
#define VLD_LD2X2DINT    VLD_HI_OP(LD2X2DINT)
#define VLD_ST2X2INT     VLD_HI_OP(ST2X2INT)

#endif /* COP_VEC_H */

