#ifndef COP_STRTYPES
#define COP_STRTYPES

#include "cop_attributes.h"
#include <stdint.h>

struct cop_strh {
	uint_fast32_t        len;
	uint_fast32_t        hash;
	const unsigned char *ptr;

};

static COP_ATTR_UNUSED void cop_strh_init_shallow(struct cop_strh *p_ret, const char *p_str) {
	uint_fast32_t hash = 2166136261;
	uint_fast32_t c;
	uint_fast32_t length = 0;
	while ((c = p_str[length++]) != 0)
		hash = ((hash ^ c) * 16777619) & 0xFFFFFFFFu;
	p_ret->len  = length;
	p_ret->hash = hash;
	p_ret->ptr  = (const unsigned char *)p_str;
}

#endif /* COP_STRTYPES */
