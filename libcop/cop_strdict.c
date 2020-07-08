#include "cop/cop_strdict.h"
#include <string.h>

static COP_ATTR_ALWAYSINLINE uint_fast64_t getikey(const struct cop_strh *p_str) {
	return (((uint_fast64_t)p_str->len) << 32) | p_str->hash;
}

static COP_ATTR_ALWAYSINLINE uint_fast32_t keytolen(uint_fast64_t key) {
	return (uint_least32_t)(key >> 32);
}

void cop_strdict_node_init(struct cop_strdict_node *p_node, const struct cop_strh *p_strh, void *p_data) {
	unsigned i;
	p_node->key      = getikey(p_strh);
	p_node->key_data = p_strh->ptr;
	p_node->data     = p_data;
	for (i = 0; i < COP_STRDICT_CHID_NB; i++)
		p_node->kids[i] = NULL;
}

void cop_strdict_node_init_by_cstr(struct cop_strdict_node *p_node, const char *p_persistent_cstr, void *p_data) {
	struct cop_strh s;
	cop_strh_init_shallow(&s, p_persistent_cstr);
	cop_strdict_node_init(p_node, &s, p_data);
}

int
cop_strdict_get_by_cstr
	(const struct cop_strdict_node  *p_root
	,const char                     *p_key
	,void                          **pp_value
	) {
	struct cop_strh s;
	cop_strh_init_shallow(&s, p_key);
	return cop_strdict_get(p_root, &s, pp_value);
}

int
cop_strdict_update_by_cstr
	(struct cop_strdict_node *p_root
	,const char              *p_key
	,void                    *p_value
	) {
	struct cop_strh s;
	cop_strh_init_shallow(&s, p_key);
	return cop_strdict_update(p_root, &s, p_value);
}

struct cop_strdict_node *
cop_strdict_delete_by_cstr
	(struct cop_strdict_node **pp_root
	,const char               *p_key
	) {
	struct cop_strh s;
	cop_strh_init_shallow(&s, p_key);
	return cop_strdict_delete(pp_root, &s);
}

int
cop_strdict_get
	(const struct cop_strdict_node  *p_root
	,const struct cop_strh          *p_key
	,void                          **pp_value /* ignored unless COP_STRDICT_FIND_STORE in flags */
	) {
	uint_fast64_t ikey = getikey(p_key);
	uint_fast64_t ukey = ikey;
	while (p_root != NULL) {
		if (p_root->key == ikey && !memcmp(p_root->key_data, p_key->ptr, p_key->len)) {
			if (pp_value != NULL)
				*pp_value = p_root->data;
			return 0;
		}
		p_root   = p_root->kids[ukey & COP_STRDICT_CHID_MASK];
		ukey   >>= COP_STRDICT_CHID_BITS;
	}
	return -1;
}

int
cop_strdict_update
	(struct cop_strdict_node *p_root
	,const struct cop_strh   *p_key
	,void                    *p_value /* ignored unless COP_STRDICT_FIND_STORE in flags */
	) {
	uint_fast64_t ikey = getikey(p_key);
	uint_fast64_t ukey = ikey;
	while (p_root != NULL) {
		if (p_root->key == ikey && !memcmp(p_root->key_data, p_key->ptr, p_key->len)) {
			p_root->data = p_value;
			return 0;
		}
		p_root   = p_root->kids[ukey & COP_STRDICT_CHID_MASK];
		ukey   >>= COP_STRDICT_CHID_BITS;
	}
	return -1;
}

int
cop_strdict_insert
	(struct cop_strdict_node **pp_root
	,struct cop_strdict_node  *p_item
	) {
	uint_fast32_t             len    = keytolen(p_item->key);
	uint_fast64_t             ukey   = p_item->key;
	struct cop_strdict_node  *p_node = *pp_root;
	while (p_node != NULL) {
		if (p_node->key == p_item->key && !memcmp(p_node->key_data, p_item->key_data, len)) 
			return -1;
		pp_root  = &(p_node->kids[ukey & COP_STRDICT_CHID_MASK]);
		p_node   = *pp_root;
		ukey   >>= COP_STRDICT_CHID_BITS;
	}
	*pp_root = p_item;
	return 0;
}

static int findkid(const struct cop_strdict_node *p_node, uint_fast64_t offset) {
	unsigned i;
	for (i = 0; i < COP_STRDICT_CHID_NB; i++)
		if (p_node->kids[(i + offset) & COP_STRDICT_CHID_MASK] != NULL)
			return (i + offset) & COP_STRDICT_CHID_MASK;
	return -1;
}

/* Delete a key. Returns non-zero if the key did not exist. Otherwise, the key
 * will always be deleted successfully. */
struct cop_strdict_node *
cop_strdict_delete
	(struct cop_strdict_node **pp_root
	,const struct cop_strh    *p_key
	) {

	uint_fast64_t             ikey  = getikey(p_key);
	uint_fast64_t             ukey  = ikey;
	struct cop_strdict_node  *p_ret = *pp_root;
	while (p_ret != NULL) {
		if (p_ret->key == ikey && !memcmp(p_ret->key_data, p_key->ptr, p_key->len))
			break;
		pp_root  = &(p_ret->kids[ukey & COP_STRDICT_CHID_MASK]);
		p_ret   = *pp_root;
		ukey   >>= COP_STRDICT_CHID_BITS;
	}
	if (p_ret != NULL) {
		int kid_idx;
		while ((kid_idx = findkid(p_ret, ukey)) >= 0) {
			struct cop_strdict_node *p_kid = p_ret->kids[kid_idx];
			unsigned i;
			for (i = 0; i < COP_STRDICT_CHID_NB; i++) {
				struct cop_strdict_node *p_tmp = p_ret->kids[i];
				p_ret->kids[i] = p_kid->kids[i];
				p_kid->kids[i] = p_tmp;
			}
			*pp_root               = p_kid;
			pp_root                = &(p_kid->kids[kid_idx]);
			p_kid->kids[kid_idx]   = p_ret;
			ukey                 >>= COP_STRDICT_CHID_BITS;
		}
		*pp_root = NULL;
	}
	return p_ret;
}

static
int
cop_strdict_enumerate_rec
	(struct cop_strdict_node  *p_node
	,cop_strdict_enumerate_fn *p_fn
	,void                     *p_context
	,int                       depth
	) {
	unsigned i;
	for (i = 0; i < COP_STRDICT_CHID_NB; i++)
		if (p_node->kids[i] != NULL) {
			int r = cop_strdict_enumerate_rec(p_node->kids[i], p_fn, p_context, depth + 1);
			if (r)
				return r;
		}
	return p_fn(p_context, p_node, depth);
}

int
cop_strdict_enumerate
	(struct cop_strdict_node  *p_root
	,cop_strdict_enumerate_fn *p_fn
	,void                     *p_context
	) {
	if (p_root == NULL)
		return 0;
	return cop_strdict_enumerate_rec(p_root, p_fn, p_context, 0);
}

void cop_strdict_node_to_key(const struct cop_strdict_node *p_node, struct cop_strh *p_key) {
	p_key->hash = p_node->key & 0xFFFFFFFFu;
	p_key->len  = p_node->key >> 32;
	p_key->ptr  = p_node->key_data;
}

void *cop_strdict_node_to_data(const struct cop_strdict_node *p_node) {
	return p_node->data;
}

