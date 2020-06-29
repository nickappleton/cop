#ifndef COP_STRDICT_H
#define COP_STRDICT_H

#include "cop_strtypes.h"
#include <stddef.h>

struct cop_strdict;
struct cop_strdict_node;

/* Initialise an empty dictionary. */
int
cop_strdict_init
	(struct cop_strdict     *p_dict
	);

/* Find an existing value. The return value is NULL if the key is not found
 * in the dictionary. Otherwise, the return value points to the the value
 * pointer. The return value is valid until a following call to cop_strdict_set or
 * cop_strdict_delete. */

int /* zero on success, non-zero when key does not exist */
cop_strdict_get
	(struct cop_strdict     *p_dict
	,const struct cop_strh  *p_key
	,void                  **p_value /* ignored unless COP_STRDICT_FIND_STORE in flags */
	);
int  /* zero on success, non-zero when key does not exist */
cop_strdict_get_by_cstr
	(struct cop_strdict     *p_dict
	,const char             *p_key
	,void                  **p_value
	);

int /* zero on success, non-zero when key does not exist */
cop_strdict_update
	(struct cop_strdict     *p_dict
	,const struct cop_strh  *p_key
	,void                   *p_value
	);
int /* zero on success, non-zero when key does not exist */
cop_strdict_update_by_cstr
	(struct cop_strdict     *p_dict
	,const char             *p_key
	,void                   *p_value
	);

struct cop_strdict_node *
cop_strdict_setup
	(struct cop_strdict_node *p_node
	,const struct cop_strh   *p_strh
	,void                    *p_data
	);
struct cop_strdict_node *
cop_strdict_setup_by_cstr
	(struct cop_strdict_node *p_node
	,const char              *p_persistent_cstr
	,void                    *p_data
	);
int /* zero on success, non-zero when key already exists */
cop_strdict_insert
	(struct cop_strdict      *p_dict
	,struct cop_strdict_node *p_key
	);

/* Delete a key. Returns non-zero if the key did not exist. Otherwise, the key
 * will always be deleted successfully. */
struct cop_strdict_node *
cop_strdict_delete
	(struct cop_strdict    *p_dict
	,const struct cop_strh *p_key
	);
struct cop_strdict_node *
cop_strdict_delete_by_cstr
	(struct cop_strdict    *p_dict
	,const char            *p_key
	);

typedef int (cop_strdict_enumerate_fn)(void *p_context, struct cop_strdict_node *p_node, int depth);
int
cop_strdict_enumerate
	(struct cop_strdict       *p_dict
	,cop_strdict_enumerate_fn *p_fn
	,void                     *p_context
	);


#define COP_STRDICT_CHID_BITS (2)
#define COP_STRDICT_CHID_NB   (1u<<COP_STRDICT_CHID_BITS)
#define COP_STRDICT_CHID_MASK (COP_STRDICT_CHID_NB-1u)

struct cop_strdict_node {
	uint_fast64_t            key;
	const unsigned char     *key_data;
	void                    *data;
	struct cop_strdict_node *kids[COP_STRDICT_CHID_NB];

};

struct cop_strdict {
	struct cop_strdict_node *root;

};



#endif /* COP_STRDICT_H */

