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

#ifndef VEC_H
#define VEC_H

#include "cop_attributes.h"

/* Reminder for Nick:
 * This is how you build something on OS X to get an armv7 executable purely
 * for examining the disassembly:
 *   xcrun --sdk iphoneos clang -arch armv7s vecfft.c -O3 -ffast-math
 * Delete this comment at some point when I know a better way to document this
 * so I'll remember it... */

/* Another reminder:
 * Clean this garbage up... you are not proud of this. */

/* Some overall notes:
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
 * just exist? Because you probably don't want them. If the system has no
 * vector support, emulating vectors will just eat all your registers and you
 * will probably end up with code barely performing better than scalar... if
 * that!
 *
 * The header file only creates macros in a VEC_, V4F_, V8F_, V2D_, V4D_, V2X_
 * V4X_ and V8X_ namespaces. */

/* If this macro is set, compiler built-in vector support will be disabled and
 * only intrinsic header files will be used (if found). If the macro is set
 * and there are no intrinsic implementations for this system, no vectors
 * will exist. If the compiler does not support a built-in vector type, this
 * macro has no effect. */
#ifndef VEC_DISABLE_COMPILER_BUILTINS
#define VEC_DISABLE_COMPILER_BUILTINS (0)
#endif

#define VEC_FUNCTION_ATTRIBUTES COP_ATTR_UNUSED COP_ATTR_INLINE

#if defined(__clang__) || defined(__GNUC__)
#define VEC_ALIGN_BEST          __attribute__((aligned(64)))
/* #elif other compilers... */
#else
#define VEC_ALIGN_BEST
#endif

/* This macro is used to construct most of the vector functions for compilers
 * which have builtin vector support. i.e. most compiler builtin vectors
 * support using regular operators to operate on the vector - so this macro
 * will generate those operations. This macro is always used for the v1d and
 * v1f types which only really exist as a convenience. */
#define VEC_BASIC_OPERATIONS(type_, data_, initsplat_) \
static VEC_FUNCTION_ATTRIBUTES type_ type_ ## _add(type_ a, type_ b)    { return a + b; } \
static VEC_FUNCTION_ATTRIBUTES type_ type_ ## _sub(type_ a, type_ b)    { return a - b; } \
static VEC_FUNCTION_ATTRIBUTES type_ type_ ## _mul(type_ a, type_ b)    { return a * b; } \
static VEC_FUNCTION_ATTRIBUTES type_ type_ ## _ld(const void *ptr)      { return *((type_ *)ptr); } \
static VEC_FUNCTION_ATTRIBUTES type_ type_ ## _ld0(const void *ptr)     { return (type_)(*(const float *)(ptr)); } \
static VEC_FUNCTION_ATTRIBUTES type_ type_ ## _broadcast(data_ a)       { return (type_)initsplat_(a); } \
static VEC_FUNCTION_ATTRIBUTES void  type_ ## _st(void *ptr, type_ val) { *((type_ *)ptr) = val; } \
static VEC_FUNCTION_ATTRIBUTES type_ type_ ## _neg(type_ a)             { return -a; }

#define VEC_INITSPLAT1(a) (a)
#define VEC_INITSPLAT2(a) {(a), (a)}
#define VEC_INITSPLAT4(a) {(a), (a), (a), (a)}
#define VEC_INITSPLAT8(a) {(a), (a), (a), (a), (a), (a), (a), (a)}

/* Build the "scalar" v1d and v1f vectors. These really only exist to make
 * pre-processor code generation work nicely. Bury your feelings somewhere
 * other than my email address. */
typedef float  v1f;
typedef double v1d;
VEC_BASIC_OPERATIONS(v1f, float,  VEC_INITSPLAT1)
VEC_BASIC_OPERATIONS(v1d, double, VEC_INITSPLAT1)

VEC_FUNCTION_ATTRIBUTES static v1f v1f_rotl(v1f a, v1f b) { (void)a; return b; }
VEC_FUNCTION_ATTRIBUTES static v1d v1d_rotl(v1d a, v1d b) { (void)a; return b; }

/* From my experience, the compiler builtins for clang are better than trying
 * to use *mmintrin headers directly*. I'm going to assume this is true for
 * gcc also. If you would rather rely on other sources of vectorisation, you
 * can define VEC_DISABLE_COMPILER_BUILTINS which will prevent builtins ever
 * being used and if another usable vector implementation is found, it will
 * be used. For compilers which don't have builtin vector support (cough...
 * MSVC cough...), this will not be where our vectors get defined anyways.
 *
 * My "experience" was a piece of code which needed to load a float into the
 * low word of a vector and do some low-word operations. Using the SSE
 * intrinsics, this resulted in all the other vector words being explicitly
 * zeroed using other instructions - this did not occur when using the builtin
 * vector operations. When I've done profiling, the builtins have normally
 * performed better providing the generated code is sensible. */
#if !VEC_DISABLE_COMPILER_BUILTINS
#if (defined(__clang__) || defined(__GNUC__)) && (defined(__SSE__) || defined(__ARM_NEON__))

//#include "arm_neon.h"

#define V4F_EXISTS (1)
#define V2D_EXISTS (1)

#if defined(__clang__)
typedef float  v4f __attribute__((ext_vector_type(4), aligned(32)));
typedef double v2d __attribute__((ext_vector_type(2), aligned(32)));
static VEC_FUNCTION_ATTRIBUTES v4f v4f_shuf0246(v4f a, v4f b) { return __builtin_shufflevector(a, b, 0, 2, 4, 6); }
static VEC_FUNCTION_ATTRIBUTES v4f v4f_shuf1357(v4f a, v4f b) { return __builtin_shufflevector(a, b, 1, 3, 5, 7); }
static VEC_FUNCTION_ATTRIBUTES v4f v4f_shuf0415(v4f a, v4f b) { return __builtin_shufflevector(a, b, 0, 4, 1, 5); }
static VEC_FUNCTION_ATTRIBUTES v4f v4f_shuf2637(v4f a, v4f b) { return __builtin_shufflevector(a, b, 2, 6, 3, 7); }
static VEC_FUNCTION_ATTRIBUTES v2d v2d_shuf02(v2d a, v2d b)   { return __builtin_shufflevector(a, b, 0, 2); }
static VEC_FUNCTION_ATTRIBUTES v2d v2d_shuf13(v2d a, v2d b)   { return __builtin_shufflevector(a, b, 1, 3); }

static VEC_FUNCTION_ATTRIBUTES v4f v4f_rotl(v4f a, v4f b) { return __builtin_shufflevector(a, b, 1, 2, 3, 4); }
static VEC_FUNCTION_ATTRIBUTES v2d v2d_rotl(v2d a, v2d b) { return __builtin_shufflevector(a, b, 1, 2); }
static VEC_FUNCTION_ATTRIBUTES v4f v4f_reverse(v4f a)     { return __builtin_shufflevector(a, a, 3, 2, 1, 0); }

#else
#include <stdint.h>
typedef float  v4f __attribute__((vector_size(16), aligned(32)));
typedef double v2d __attribute__((vector_size(16), aligned(32)));
static VEC_FUNCTION_ATTRIBUTES v4f v4f_shuf0246(v4f a, v4f b) { static const int32_t __attribute__((vector_size(16))) shufmask = {0, 2, 4, 6}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v4f v4f_shuf1357(v4f a, v4f b) { static const int32_t __attribute__((vector_size(16))) shufmask = {1, 3, 5, 7}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v4f v4f_shuf0415(v4f a, v4f b) { static const int32_t __attribute__((vector_size(16))) shufmask = {0, 4, 1, 5}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v4f v4f_shuf2637(v4f a, v4f b) { static const int32_t __attribute__((vector_size(16))) shufmask = {2, 6, 3, 7}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v2d v2d_shuf02(v2d a, v2d b)   { static const int64_t __attribute__((vector_size(16))) shufmask = {0, 2}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v2d v2d_shuf13(v2d a, v2d b)   { static const int64_t __attribute__((vector_size(16))) shufmask = {1, 3}; return __builtin_shuffle(a, b, shufmask); }

static VEC_FUNCTION_ATTRIBUTES v4f v4f_rotl(v4f a, v4f b) { static const int32_t __attribute__((vector_size(16))) shufmask = {1, 2, 3, 4}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v2d v2d_rotl(v2d a, v2d b) { static const int64_t __attribute__((vector_size(16))) shufmask = {1, 2};       return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v4f v4f_reverse(v4f a)     { static const int32_t __attribute__((vector_size(16))) shufmask = {3, 2, 1, 0}; return __builtin_shuffle(a, a, shufmask); }

#endif

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

VEC_BASIC_OPERATIONS(v4f, float, VEC_INITSPLAT4)
VEC_BASIC_OPERATIONS(v2d, double, VEC_INITSPLAT2)

#endif /* (defined(__clang__) || defined(__GNUC__)) && defined(__SSE__) */

#if (defined(__clang__) || defined(__GNUC__)) && defined(__AVX__)

#define V8F_EXISTS (1)
#define V4D_EXISTS (1)

#if defined(__clang__)
typedef float  v8f __attribute__((ext_vector_type(8), aligned(32)));
typedef double v4d __attribute__((ext_vector_type(4), aligned(32)));
static VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf0145(v4d a, v4d b)     { return __builtin_shufflevector(a, b, 0, 1, 4, 5); }
static VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf2367(v4d a, v4d b)     { return __builtin_shufflevector(a, b, 2, 3, 6, 7); }
static VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf0426(v4d a, v4d b)     { return __builtin_shufflevector(a, b, 0, 4, 2, 6); }
static VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf1537(v4d a, v4d b)     { return __builtin_shufflevector(a, b, 1, 5, 3, 7); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf012389AB(v8f a, v8f b) { return __builtin_shufflevector(a, b, 0,  1,  2,  3,  8,  9,  10, 11); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf4567CDEF(v8f a, v8f b) { return __builtin_shufflevector(a, b, 4,  5,  6,  7,  12, 13, 14, 15); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf028A46CE(v8f a, v8f b) { return __builtin_shufflevector(a, b, 0,  2,  8,  10, 4,  6,  12, 14); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf139B57DF(v8f a, v8f b) { return __builtin_shufflevector(a, b, 1,  3,  9,  11, 5,  7,  13, 15); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf08194C5D(v8f a, v8f b) { return __builtin_shufflevector(a, b, 0,  8,  1,  9,  4,  12, 5,  13); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf2A3B6E7F(v8f a, v8f b) { return __builtin_shufflevector(a, b, 2,  10, 3,  11, 6,  14, 7,  15); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf018945CD(v8f a, v8f b) { return __builtin_shufflevector(a, b, 0,  1,  8,  9,  4,  5,  12, 13); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf23AB67EF(v8f a, v8f b) { return __builtin_shufflevector(a, b, 2,  3,  10, 11, 6,  7,  14, 15); }
#else
#include <stdint.h>
typedef float  v8f __attribute__((vector_size(32), aligned(32)));
typedef double v4d __attribute__((vector_size(32), aligned(32)));
static VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf0145(v4d a, v4d b)     { static const int64_t __attribute__((vector_size(32))) shufmask = {0, 1, 4, 5}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf2367(v4d a, v4d b)     { static const int64_t __attribute__((vector_size(32))) shufmask = {2, 3, 6, 7}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf0426(v4d a, v4d b)     { static const int64_t __attribute__((vector_size(32))) shufmask = {0, 4, 2, 6}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v4d v4d_shuf1537(v4d a, v4d b)     { static const int64_t __attribute__((vector_size(32))) shufmask = {1, 5, 3, 7}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf012389AB(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {0,  1,  2,  3,  8,  9,  10, 11}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf4567CDEF(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {4,  5,  6,  7,  12, 13, 14, 15}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf028A46CE(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {0,  2,  8,  10, 4,  6,  12, 14}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf139B57DF(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {1,  3,  9,  11, 5,  7,  13, 15}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf08194C5D(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {0,  8,  1,  9,  4,  12, 5,  13}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf2A3B6E7F(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {2,  10, 3,  11, 6,  14, 7,  15}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf018945CD(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {0,  1,  8,  9,  4,  5,  12, 13}; return __builtin_shuffle(a, b, shufmask); }
static VEC_FUNCTION_ATTRIBUTES v8f v8f_shuf23AB67EF(v8f a, v8f b) { static const int32_t __attribute__((vector_size(32))) shufmask = {2,  3,  10, 11, 6,  7,  14, 15}; return __builtin_shuffle(a, b, shufmask); }
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

VEC_BASIC_OPERATIONS(v8f, float, VEC_INITSPLAT8)
VEC_BASIC_OPERATIONS(v4d, double, VEC_INITSPLAT4)

#endif /* defined(__clang__) && defined(__AVX__) */
#endif /* !VEC_DISABLE_COMPILER_BUILTINS */

/* This macro is used by the X86/64 SSE vector implementations to construct as
 * many of the basic operations as possible. This is made possible by a
 * consistent naming of intrinsic functions. If none of the SSE headers are
 * found, don't worry, this macro will not be used - it's just easier to
 * define it here. */
#define VEC_SSE_BASIC_OPERATIONS(type_, data_, regtyp_, mmtyp_) \
VEC_FUNCTION_ATTRIBUTES static type_ type_ ## _add(type_ a, type_ b)   { return _ ## regtyp_ ## _add_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES static type_ type_ ## _sub(type_ a, type_ b)   { return _ ## regtyp_ ## _sub_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES static type_ type_ ## _mul(type_ a, type_ b)   { return _ ## regtyp_ ## _mul_p ## mmtyp_(a, b); } \
VEC_FUNCTION_ATTRIBUTES static type_ type_ ## _ld(const void *ptr)     { return _ ## regtyp_ ## _load_p ## mmtyp_(ptr); } \
VEC_FUNCTION_ATTRIBUTES static type_ type_ ## _broadcast(data_ a)      { return _ ## regtyp_ ## _set1_p ## mmtyp_(a); } \
VEC_FUNCTION_ATTRIBUTES static void  type_ ## _st(void *ptr, type_ val) { _ ## regtyp_ ## _store_p ## mmtyp_(ptr, val); }

/* Look for SSE to provide implementations of v4f and v2d. */
#if defined(__clang__) && defined(__SSE__) && (!defined(V4F_EXISTS) || !defined(V2D_EXISTS))

#include "xmmintrin.h"

#ifndef V4F_EXISTS
#define V4F_EXISTS (1)

typedef __m128 v4f;
VEC_SSE_BASIC_OPERATIONS(v4f, float, mm, s)

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
VEC_SSE_BASIC_OPERATIONS(v2d, double, mm, d)

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
VEC_SSE_BASIC_OPERATIONS(v8f, float, mm256, s)

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
VEC_SSE_BASIC_OPERATIONS(v4d, double, mm256, d)

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
#if defined(__clang__) && defined(__ARM_NEON__) && !defined(V4F_EXISTS)

#include "arm_neon.h"

#ifndef V4F_EXISTS
#define V4F_EXISTS (1)

typedef float32x4_t v4f;

static VEC_FUNCTION_ATTRIBUTES v4f  v4f_add(v4f a, v4f b)      { return vaddq_f32(a, b); }
static VEC_FUNCTION_ATTRIBUTES v4f  v4f_sub(v4f a, v4f b)      { return vsubq_f32(a, b); }
static VEC_FUNCTION_ATTRIBUTES v4f  v4f_mul(v4f a, v4f b)      { return vmulq_f32(a, b); }
static VEC_FUNCTION_ATTRIBUTES v4f  v4f_ld(const void *ptr)    { return vld1q_f32(ptr); }
static VEC_FUNCTION_ATTRIBUTES v4f  v4f_ld0(const void *ptr)   { v4f tmp; return vld1q_lane_f32(ptr, tmp, 0); }
static VEC_FUNCTION_ATTRIBUTES v4f  v4f_broadcast(float a)     { return vld1q_dup_f32(&a); }
static VEC_FUNCTION_ATTRIBUTES void v4f_st(void *ptr, v4f val) { vst1q_f32(ptr, val); }

#if 0
/* NEON should be able to do a double load with a single vld1.32 */
#define V4F_LD2(r0_, r1_, src_) do { \
	r0_ = v4f_ld(((const float *)(src_)) + 0); \
	r1_ = v4f_ld(((const float *)(src_)) + 4); \
} while (0)
#define V4F_LD2X2(r00_, r01_, r10_, r11_, src0_, src1_) do { \
	V4F_LD2(r00_, r01_, src0_); \
	V4F_LD2(r10_, r11_, src1_); \
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

/* Don't need any of the macros which we used to construct operators anymore
 * so pull them out of the global namespace. */
#undef VEC_BASIC_OPERATIONS
#undef VEC_SSE_BASIC_OPERATIONS

/* Generic V**_INTERLEAVE2/V**_DEINTERLEAVE2 implementations
 * -----------------------------------------------
 * For cases where V**_INTERLEAVE2/V**_DEINTERLEAVE2 have not been defined,
 * we implement the operations using two singular INTERLEAVE operations. */
#if defined(V4F_EXISTS) && !defined(V4F_INTERLEAVE_STORE)
#define V4F_INTERLEAVE_STORE(dest_, in1_, in2_) do { \
	v4f tmp0_, tmp1_; \
	V4F_INTERLEAVE(tmp0_, tmp1_, in1_, in2_); \
	v4f_st(dest_, tmp0_); \
	v4f_st((float *)(dest_) + 4, tmp1_); \
} while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_INTERLEAVE2)
#define V4F_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	V4F_INTERLEAVE(out1_, out2_, in1_, in2_); \
	V4F_INTERLEAVE(out3_, out4_, in3_, in4_); \
} while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_INTERLEAVE2)
#define V4D_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	V4D_INTERLEAVE(out1_, out2_, in1_, in2_); \
	V4D_INTERLEAVE(out3_, out4_, in3_, in4_); \
} while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_INTERLEAVE2)
#define V8F_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	V8F_INTERLEAVE(out1_, out2_, in1_, in2_); \
	V8F_INTERLEAVE(out3_, out4_, in3_, in4_); \
} while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_INTERLEAVE2)
#define V2D_INTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	V2D_INTERLEAVE(out1_, out2_, in1_, in2_); \
	V2D_INTERLEAVE(out3_, out4_, in3_, in4_); \
} while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_DEINTERLEAVE2)
#define V4F_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	V4F_DEINTERLEAVE(out1_, out2_, in1_, in2_); \
	V4F_DEINTERLEAVE(out3_, out4_, in3_, in4_); \
} while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_DEINTERLEAVE2)
#define V4D_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	V4D_DEINTERLEAVE(out1_, out2_, in1_, in2_); \
	V4D_DEINTERLEAVE(out3_, out4_, in3_, in4_); \
} while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_DEINTERLEAVE2)
#define V2D_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	V2D_DEINTERLEAVE(out1_, out2_, in1_, in2_); \
	V2D_DEINTERLEAVE(out3_, out4_, in3_, in4_); \
} while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_DEINTERLEAVE2)
#define V8F_DEINTERLEAVE2(out1_, out2_, out3_, out4_, in1_, in2_, in3_, in4_) do { \
	V8F_DEINTERLEAVE(out1_, out2_, in1_, in2_); \
	V8F_DEINTERLEAVE(out3_, out4_, in3_, in4_); \
} while (0)
#endif

/* Generic transpose implementations
 * -----------------------------------------------
 * For cases where V**_TRANSPOSE_INPLACE has not been defined, we implement
 * transpositions using the generic V**_INTERLEAVE/V**_INTERLEAVE2 macros. */
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
#if defined(V4D_EXISTS) && !defined(V4D_TRANSPOSE_INPLACE)
#define V4D_TRANSPOSE_INPLACE(r1_, r2_, r3_, r4_) do { \
	v4d t1_, t2_, t3_, t4_; \
	V4D_INTERLEAVE2(t1_, t2_, t3_, t4_, r1_, r3_, r2_, r4_); \
	V4D_INTERLEAVE2(r1_, r2_, r3_, r4_, t1_, t3_, t2_, t4_); \
} while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_TRANSPOSE_INPLACE)
#define V4F_TRANSPOSE_INPLACE(r1_, r2_, r3_, r4_) do { \
	v4f t1_, t2_, t3_, t4_; \
	V4F_INTERLEAVE2(t1_, t2_, t3_, t4_, r1_, r3_, r2_, r4_); \
	V4F_INTERLEAVE2(r1_, r2_, r3_, r4_, t1_, t3_, t2_, t4_); \
} while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_TRANSPOSE_INPLACE)
#define V2D_TRANSPOSE_INPLACE(r1_, r2_) do { \
	v2d t1_, t2_; \
	V2D_INTERLEAVE(t1_, t2_, r1_, r2_); \
	r1_ = t1_; \
	r2_ = t2_; \
} while (0)
#endif

#if defined(V4F_EXISTS) && !defined(V4F_LD2)
#define V4F_LD2(r0_, r1_, src_) do { \
	r0_ = v4f_ld(((const float *)(src_)) + 0); \
	r1_ = v4f_ld(((const float *)(src_)) + 4); \
} while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_LD2DINT)
#define V4F_LD2DINT(r0_, r1_, src_) do { \
	v4f tmp1_, tmp2_; \
	V4F_LD2(tmp1_, tmp2_, src_); \
	V4F_DEINTERLEAVE(r0_, r1_, tmp1_, tmp2_); \
} while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_LD2X2)
#define V4F_LD2X2(r00_, r01_, r10_, r11_, src0_, src1_) do { \
	V4F_LD2(r00_, r01_, src0_); \
	V4F_LD2(r10_, r11_, src1_); \
} while (0)
#endif
#if defined(V4F_EXISTS) && !defined(V4F_LD2X2DINT)
#define V4F_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_) do { \
	v4f tmp1_, tmp2_, tmp3_, tmp4_; \
	V4F_LD2X2(tmp1_, tmp2_, tmp3_, tmp4_, src0_, src1_); \
	V4F_DEINTERLEAVE2(r00_, r01_, r10_, r11_, tmp1_, tmp2_, tmp3_, tmp4_); \
} while (0)
#endif

#if defined(V2D_EXISTS) && !defined(V2D_LD2)
#define V2D_LD2(r0_, r1_, src_) do { \
	r0_ = v2d_ld(((const double *)(src_)) + 0); \
	r1_ = v2d_ld(((const double *)(src_)) + 2); \
} while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_LD2DINT)
#define V2D_LD2DINT(r0_, r1_, src_) do { \
	v2d tmp1_, tmp2_; \
	V2D_LD2(tmp1_, tmp2_, src_); \
	V2D_DEINTERLEAVE(r0_, r1_, tmp1_, tmp2_); \
} while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_LD2X2)
#define V2D_LD2X2(r00_, r01_, r10_, r11_, src0_, src1_) do { \
	V2D_LD2(r00_, r01_, src0_); \
	V2D_LD2(r10_, r11_, src1_); \
} while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_LD2X2DINT)
#define V2D_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_) do { \
	v2d tmp1_, tmp2_, tmp3_, tmp4_; \
	V2D_LD2X2(tmp1_, tmp2_, tmp3_, tmp4_, src0_, src1_); \
	V2D_DEINTERLEAVE2(r00_, r01_, r10_, r11_, tmp1_, tmp2_, tmp3_, tmp4_); \
} while (0)
#endif

#if defined(V8F_EXISTS) && !defined(V8F_LD2)
#define V8F_LD2(r0_, r1_, src_) do { \
	r0_ = v8f_ld(((const float *)(src_)) + 0); \
	r1_ = v8f_ld(((const float *)(src_)) + 8); \
} while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_LD2DINT)
#define V8F_LD2DINT(r0_, r1_, src_) do { \
	v8f tmp1_, tmp2_; \
	V8F_LD2(tmp1_, tmp2_, src_); \
	V8F_DEINTERLEAVE(r0_, r1_, tmp1_, tmp2_); \
} while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_LD2X2)
#define V8F_LD2X2(r00_, r01_, r10_, r11_, src0_, src1_) do { \
	V8F_LD2(r00_, r01_, src0_); \
	V8F_LD2(r10_, r11_, src1_); \
} while (0)
#endif
#if defined(V8F_EXISTS) && !defined(V8F_LD2X2DINT)
#define V8F_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_) do { \
	v8f tmp1_, tmp2_, tmp3_, tmp4_; \
	V8F_LD2X2(tmp1_, tmp2_, tmp3_, tmp4_, src0_, src1_); \
	V8F_DEINTERLEAVE2(r00_, r01_, r10_, r11_, tmp1_, tmp2_, tmp3_, tmp4_); \
} while (0)
#endif

#if defined(V4D_EXISTS) && !defined(V4D_LD2)
#define V4D_LD2(r0_, r1_, src_) do { \
	r0_ = v4d_ld(((const double *)(src_)) + 0); \
	r1_ = v4d_ld(((const double *)(src_)) + 4); \
} while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_LD2DINT)
#define V4D_LD2DINT(r0_, r1_, src_) do { \
	v4d tmp1_, tmp2_; \
	V4D_LD2(tmp1_, tmp2_, src_); \
	V4D_DEINTERLEAVE(r0_, r1_, tmp1_, tmp2_); \
} while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_LD2X2)
#define V4D_LD2X2(r00_, r01_, r10_, r11_, src0_, src1_) do { \
	V4D_LD2(r00_, r01_, src0_); \
	V4D_LD2(r10_, r11_, src1_); \
} while (0)
#endif
#if defined(V4D_EXISTS) && !defined(V4D_LD2X2DINT)
#define V4D_LD2X2DINT(r00_, r01_, r10_, r11_, src0_, src1_) do { \
	v4d tmp1_, tmp2_, tmp3_, tmp4_; \
	V4D_LD2X2(tmp1_, tmp2_, tmp3_, tmp4_, src0_, src1_); \
	V4D_DEINTERLEAVE2(r00_, r01_, r10_, r11_, tmp1_, tmp2_, tmp3_, tmp4_); \
} while (0)
#endif

#if defined(V2D_EXISTS) && !defined(V2D_ST2)
#define V2D_ST2(dest_, r0_, r1_) do { \
	v2d_st(((double *)(dest_)) + 0, r0_); \
	v2d_st(((double *)(dest_)) + 2, r1_); \
} while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_ST2INT)
#define V2D_ST2INT(dest_, r0_, r1_) do { \
	v2d tmp1_, tmp2_; \
	V2D_INTERLEAVE(tmp1_, tmp2_, r0_, r1_); \
	V2D_ST2(dest_, tmp1_, tmp2_); \
} while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_ST2X2)
#define V2D_ST2X2(dest0_, dest1_, r00_, r01_, r10_, r11_) do { \
	V2D_ST2(dest0_, r00_, r01_); \
	V2D_ST2(dest1_, r10_, r11_); \
} while (0)
#endif
#if defined(V2D_EXISTS) && !defined(V2D_ST2X2INT)
#define V2D_ST2X2INT(dest0_, dest1_, r00_, r01_, r10_, r11_) do { \
	v2d tmp1_, tmp2_, tmp3_, tmp4_; \
	V2D_INTERLEAVE2(tmp1_, tmp2_, tmp3_, tmp4_, r00_, r01_, r10_, r11_); \
	V2D_ST2X2(dest0_, dest1_, tmp1_, tmp2_, tmp3_, tmp4_); \
} while (0)
#endif

#endif /* VEC_H */

