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

/* TODO: This test suite is not complete! */

#include "cop/cop_vec.h"
#include <stdio.h>

static const int prime_data[64] =
{ 2,   3,   5,   7,   11,  13,  17,  19
, 23,  29,  31,  37,  41,  43,  47,  53
, 59,  61,  67,  71,  73,  79,  83,  89
, 97,  101, 103, 107, 109, 113, 127, 131
, 137, 139, 149, 151, 157, 163, 167, 173
, 179, 181, 191, 193, 197, 199, 211, 223
, 227, 229, 233, 239, 241, 251, 257, 263
, 269, 271, 277, 281, 283, 293, 307, 311
};

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define MAKE_BASIC_TESTS(vectypecaps_, vectype_, basetype_, veclen_) \
static int vectype_ ## _tests(void) \
{ \
	basetype_ VEC_ALIGN_BEST in[(veclen_)*MAX((veclen_), 4)]; \
	basetype_ VEC_ALIGN_BEST out[(veclen_)*MAX((veclen_), 4)]; \
	vectype_ a, b, c, d; \
	unsigned i; \
	int failures = 0; \
	basetype_ err; \
	printf("executing tests for " #vectype_ "vector type\n"); \
	for (i = 0; i < (veclen_)*MAX((veclen_), 4); i++) { \
		in[i] = prime_data[i]; \
	} \
	/* Load element 0 test */ \
	a = vectype_ ## _ld(in); \
	b = vectype_ ## _lde0(a, in + (veclen_)); \
	vectype_ ## _st(out, b); \
	for (i = 0, err = 0.0f; i < (veclen_); i++) { \
		basetype_ e = out[i] - (prime_data[(i == 0) ? (i + (veclen_)) : i]); \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectype_ "_lde0() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectype_ "_lde0() test passed\n"); \
	} \
	/* Addition test */ \
	a = vectype_ ## _ld(in); \
	b = vectype_ ## _ld(in + (veclen_)); \
	c = vectype_ ## _add(a, b); \
	vectype_ ## _st(out, c); \
	for (i = 0, err = 0.0f; i < (veclen_); i++) { \
		basetype_ e = out[i] - (prime_data[i] + prime_data[i+(veclen_)]); \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectype_ "_add() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectype_ "_add() test passed\n"); \
	} \
	/* Subtraction test */ \
	a = vectype_ ## _ld(in); \
	b = vectype_ ## _ld(in + (veclen_)); \
	c = vectype_ ## _sub(a, b); \
	vectype_ ## _st(out, c); \
	for (i = 0, err = 0.0f; i < (veclen_); i++) { \
		basetype_ e = out[i] - (prime_data[i] - prime_data[i+(veclen_)]); \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectype_ "_sub() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectype_ "_sub() test passed\n"); \
	} \
	/* Multiplication test */ \
	a = vectype_ ## _ld(in); \
	b = vectype_ ## _ld(in + (veclen_)); \
	c = vectype_ ## _mul(a, b); \
	vectype_ ## _st(out, c); \
	for (i = 0, err = 0.0f; i < (veclen_); i++) { \
		basetype_ e = out[i] - (prime_data[i] * prime_data[i+(veclen_)]); \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectype_ "_mul() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectype_ "_mul() test passed\n"); \
	} \
	/* Negation test */ \
	a = vectype_ ## _ld(in); \
	c = vectype_ ## _neg(a); \
	vectype_ ## _st(out, c); \
	for (i = 0, err = 0.0f; i < (veclen_); i++) { \
		basetype_ e = out[i] - (-prime_data[i]); \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectype_ "_neg() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectype_ "_neg() test passed\n"); \
	} \
	/* Broadcast test */ \
	c = vectype_ ## _broadcast((basetype_)prime_data[0]); \
	vectype_ ## _st(out, c); \
	for (i = 0, err = 0.0f; i < (veclen_); i++) { \
		basetype_ e = out[i] - ((basetype_)prime_data[0]); \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectype_ "_broadcast() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectype_ "_broadcast() test passed\n"); \
	} \
	/* Rotate left test */ \
	a = vectype_ ## _ld(in); \
	b = vectype_ ## _ld(in + (veclen_)); \
	c = vectype_ ## _rotl(a, b); \
	vectype_ ## _st(out, c); \
	for (i = 0, err = 0.0f; i < (veclen_); i++) { \
		basetype_ e = out[i] - prime_data[i+1]; \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectype_ "_rotl() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectype_ "_rotl() test passed\n"); \
	} \
	/* Reverse test */ \
	a = vectype_ ## _ld(in); \
	c = vectype_ ## _reverse(a); \
	vectype_ ## _st(out, c); \
	for (i = 0, err = 0.0f; i < (veclen_); i++) { \
		basetype_ e = out[i] - prime_data[(veclen_)-1-i]; \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectype_ "_reverse() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectype_ "_reverse() test passed\n"); \
	} \
	/* Interleave test */ \
	a = vectype_ ## _ld(in); \
	b = vectype_ ## _ld(in + (veclen_)); \
	vectypecaps_ ## _INTERLEAVE(c, d, a, b); \
	vectype_ ## _st(out, c); \
	vectype_ ## _st(out + (veclen_), d); \
	for (i = 0, err = 0.0f; i < (veclen_)*2; i++) { \
		basetype_ e; \
		e = out[i] - prime_data[(i&1)*(veclen_)+i/2]; \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectypecaps_ "_INTERLEAVE() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectypecaps_ "_INTERLEAVE() test passed\n"); \
	} \
	/* Interleave-store test */ \
	a = vectype_ ## _ld(in); \
	b = vectype_ ## _ld(in + (veclen_)); \
	vectypecaps_ ## _ST2INT(out, a, b); \
	for (i = 0, err = 0.0f; i < (veclen_)*2; i++) { \
		basetype_ e; \
		e = out[i] - prime_data[(i&1)*(veclen_)+i/2]; \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectypecaps_ "_ST2INT() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectypecaps_ "_ST2INT() test passed\n"); \
	} \
	/* Double interleave-store test */ \
	a = vectype_ ## _ld(in + (veclen_)*0); \
	b = vectype_ ## _ld(in + (veclen_)*1); \
	c = vectype_ ## _ld(in + (veclen_)*2); \
	d = vectype_ ## _ld(in + (veclen_)*3); \
	vectypecaps_ ## _ST2X2INT(out, out + (veclen_)*2, a, b, c, d); \
	for (i = 0, err = 0.0f; i < (veclen_)*2; i++) { \
		basetype_ e; \
		e = out[i+(veclen_)*0] - prime_data[(i&1)*(veclen_)+i/2]; \
		err += e*e; \
		e = out[i+(veclen_)*2] - prime_data[(i&1)*(veclen_)+i/2+(veclen_)*2]; \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectypecaps_ "_ST2X2INT() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectypecaps_ "_ST2X2INT() test passed\n"); \
	} \
	/* Deinterleave test */ \
	a = vectype_ ## _ld(in); \
	b = vectype_ ## _ld(in + (veclen_)); \
	vectypecaps_ ## _DEINTERLEAVE(c, d, a, b); \
	vectype_ ## _st(out, c); \
	vectype_ ## _st(out + (veclen_), d); \
	for (i = 0, err = 0.0f; i < (veclen_)*2; i++) { \
		basetype_ e; \
		e = out[i] - prime_data[(i%(veclen_))*2+i/(veclen_)]; \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectypecaps_ "_DEINTERLEAVE() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectypecaps_ "_DEINTERLEAVE() test passed\n"); \
	} \
	/* Deinterleave-load test */ \
	vectypecaps_ ## _LD2DINT(c, d, in); \
	vectype_ ## _st(out, c); \
	vectype_ ## _st(out + (veclen_), d); \
	for (i = 0, err = 0.0f; i < (veclen_)*2; i++) { \
		basetype_ e; \
		e = out[i] - prime_data[(i%(veclen_))*2+i/(veclen_)]; \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectypecaps_ "_LD2DINT() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectypecaps_ "_LD2DINT() test passed\n"); \
	} \
	/* Double deinterleave-load test */ \
	vectypecaps_ ## _LD2X2DINT(a, b, c, d, in, in + (veclen_)*2); \
	vectype_ ## _st(out + (veclen_)*0, a); \
	vectype_ ## _st(out + (veclen_)*1, b); \
	vectype_ ## _st(out + (veclen_)*2, c); \
	vectype_ ## _st(out + (veclen_)*3, d); \
	for (i = 0, err = 0.0f; i < (veclen_)*2; i++) { \
		basetype_ e; \
		e = out[i+(veclen_)*0] - prime_data[(i%(veclen_))*2+((i/(veclen_))&1)]; \
		err += e*e; \
		e = out[i+(veclen_)*2] - prime_data[(i%(veclen_))*2+((i/(veclen_))&1)+(veclen_)*2]; \
		err += e*e; \
	} \
	if (err > 1e-10) { \
		printf("  " #vectypecaps_ "_LD2X2DINT() test failed\n"); \
		failures++; \
	} else { \
		printf("  " #vectypecaps_ "_LD2X2DINT() test passed\n"); \
	} \
	return failures; \
}

MAKE_BASIC_TESTS(V1F, v1f, float,  1)
MAKE_BASIC_TESTS(V1D, v1d, double, 1)
MAKE_BASIC_TESTS(VLF, vlf, float,  VLF_WIDTH)

#if V4F_EXISTS
MAKE_BASIC_TESTS(V4F, v4f, float,  4)
#endif
#if V2D_EXISTS
MAKE_BASIC_TESTS(V2D, v2d, double, 2)
#endif
#if V8F_EXISTS
MAKE_BASIC_TESTS(V8F, v8f, float,  8)
#endif
#if V4D_EXISTS
MAKE_BASIC_TESTS(V4D, v4d, double, 4)
#endif

int main(int argc, char *argv[])
{
	int failures = 0;
	failures += v1f_tests();
	failures += v1d_tests();
	failures += vlf_tests();

#if V2D_EXISTS
	failures += v2d_tests();
#else
	printf("no v2d type - skipping tests\n");
#endif
#if V4F_EXISTS
	failures += v4f_tests();
#else
	printf("no v4f type - skipping tests\n");
#endif
#if V8F_EXISTS
	failures += v8f_tests();
#else
	printf("no v8f type - skipping tests\n");
#endif
#if V4D_EXISTS
	failures += v4d_tests();
#else
	printf("no v4d type - skipping tests\n");
#endif

	if (failures) {
		printf("%d test failures\n", failures);
	} else {
		printf("all tests passed\n");
	}

	return failures;
}
