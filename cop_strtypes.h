#ifndef COP_STRTYPES
#define COP_STRTYPES

#include "cop_attributes.h"
#include <stdint.h>

/* A cop_strh is used to hold a pointer to a known-length string of hashed
 * bytes. */
struct cop_strh {
	/* Number of bytes in the data blob. */
	uint_fast32_t        len;

	/* FNV-1A hash of len bytes of ptr. */
	uint_fast32_t        hash;

	/* The data blob. */
	const unsigned char *ptr;

};

/* Initialise a cop_strh object which represents the data of the given null-
 * terminated string. The null-terminator is not included in the length or the
 * hash that will be stored. */
static void cop_strh_init_shallow(struct cop_strh *p_ret, const char *p_str);

static COP_ATTR_UNUSED void cop_strh_init_shallow(struct cop_strh *p_ret, const char *p_str) {
	uint_fast32_t hash = 2166136261u;
	uint_fast32_t c;
	uint_fast32_t length = 0;
	while ((c = p_str[length++]) != 0)
		hash = ((hash ^ c) * 16777619u) & 0xFFFFFFFFu;
	p_ret->len  = length;
	p_ret->hash = hash;
	p_ret->ptr  = (const unsigned char *)p_str;
}

#endif /* COP_STRTYPES */
