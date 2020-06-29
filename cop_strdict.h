#ifndef COP_STRDICT_H
#define COP_STRDICT_H

#include "cop_strtypes.h"
#include <stddef.h>

struct cop_strdict_node;

/* Initialise an empty dictionary. */
static COP_ATTR_UNUSED struct cop_strdict_node *cop_strdict_init(void) {
	return NULL;
}

/* Setup a new node with a key and data pointer using a cop_strh structure
 * for the key.
 *
 * The function takes a cop_strdict_node structure and fills in the required
 * parameters. The cop_strh structure will be used for the key and must have a
 * persistent data pointer. p_data is the initial data pointer value. No
 * allocations take place. */
void
cop_strdict_setup
	(struct cop_strdict_node *p_node
	,const struct cop_strh   *p_strh
	,void                    *p_data
	);

/* Setup a new node with a key and data pointer using a null-terminated stirng
 * for the key.
 *
 * The function takes a cop_strdict_node structure and fills in the required
 * parameters. p_persistent_cstr must be a persistent null-terminated string
 * which will be used for the key. p_data is the initial data pointer value.
 * No allocations take place. */
void
cop_strdict_setup_by_cstr
	(struct cop_strdict_node *p_node
	,const char              *p_persistent_cstr
	,void                    *p_data
	);

void cop_strdict_node_to_key(const struct cop_strdict_node *p_node, struct cop_strh *p_key);
void *cop_strdict_node_to_data(const struct cop_strdict_node *p_node);

/* Insert a node into the dictionary.
 *
 * The key pointer and it's internal key pointer must point to persistent
 * memory until such time as it is deleted from the dictionary. The function
 * returns zero if the key was inserted successfully. It returns non-zero if
 * a node with the given key already existed in the dictionary - if this
 * occurs, no change is made to the dictionary. */
int
cop_strdict_insert
	(struct cop_strdict_node **pp_root
	,struct cop_strdict_node  *p_key
	);

/* Find an existing value in the dictionary by cop_strh structure.
 *
 * Search for a key using the data in the given cop_strh structure. If
 * pp_value is not NULL and the key exists, the pointer will be set to
 * the data pointer of the item with the given key. The function returns
 * zero if the key exists. It returns non-zero if the key does not exist. */
int /* zero on success, non-zero when key does not exist */
cop_strdict_get
	(struct cop_strdict_node **pp_root
	,const struct cop_strh    *p_key
	,void                    **pp_value /* ignored unless COP_STRDICT_FIND_STORE in flags */
	);

/* Find an existing value in the dictionary by null-terminated string.
 *
 * Search for a key using the data in the given null-terminated string (
 * which will be converted to a cop_strh structure using the
 * cop_strh_init_shallow() function). If pp_value is not NULL and the key
 * exists, the pointer will be set to the data pointer of the item with the
 * given key. The function returns zero if the key exists. It returns non-
 * zero if the key does not exist. */
int  /* zero on success, non-zero when key does not exist */
cop_strdict_get_by_cstr
	(struct cop_strdict_node **pp_root
	,const char               *p_key
	,void                    **pp_value
	);

int /* zero on success, non-zero when key does not exist */
cop_strdict_update
	(struct cop_strdict_node **pp_root
	,const struct cop_strh    *p_key
	,void                     *p_value
	);
int /* zero on success, non-zero when key does not exist */
cop_strdict_update_by_cstr
	(struct cop_strdict_node **pp_root
	,const char               *p_key
	,void                     *p_value
	);

/* Remove an item using a key defined by a cop_strh structure.
 *
 * If the return value is NULL, the key was not found in the dictionary.
 * Otherwise, it will be the same pointer that was passed to the
 * cop_strdict_insert() function to insert the node. */
struct cop_strdict_node *
cop_strdict_delete
	(struct cop_strdict_node **pp_root
	,const struct cop_strh    *p_key
	);

/* Remove an item using a key defined by a null-terminated string.
 *
 * Finds a node and returns it. The key will be converted to a cop_strh using
 * the cop_strh_init_shallow() function to search. If the return value is
 * NULL, the key was not found in the dictionary. Otherwise, it will be the
 * same pointer that was passed to the cop_strdict_insert() function to insert
 * the node. */
struct cop_strdict_node *
cop_strdict_delete_by_cstr
	(struct cop_strdict_node **pp_root
	,const char               *p_key
	);

typedef int (cop_strdict_enumerate_fn)(void *p_context, struct cop_strdict_node *p_node, int depth);
int
cop_strdict_enumerate
	(struct cop_strdict_node  **pp_root
	,cop_strdict_enumerate_fn  *p_fn
	,void                      *p_context
	);

/* ---------------------------------------------------------------------------
 * Internal bits
 *
 * These are defined so that you can know their memory requirements and
 * allocate them potentially on the stack. You should not depend on their
 * members being stable and should interact with them using only the above
 * APIs.
 * ------------------------------------------------------------------------ */

/* These macros define the number of children per node. 4 has been found to
 * give excellent performance for arbitrary sized dictionaries. */
#define COP_STRDICT_CHID_BITS (2)
#define COP_STRDICT_CHID_NB   (1u<<COP_STRDICT_CHID_BITS)
#define COP_STRDICT_CHID_MASK (COP_STRDICT_CHID_NB-1u)

/* Internal node structure */
struct cop_strdict_node {
	/* Lower 32 bits is the hash. Upper 32 bits is the key data length. */
	uint_fast64_t            key;

	/* Pointer to the key data that was in the cop_strh object used to build
	 * the node. */
	const unsigned char     *key_data;

	/* Current value of the node data pointer. */
	void                    *data;

	/* Pointers to child nodes. Initialised to NULL. */
	struct cop_strdict_node *kids[COP_STRDICT_CHID_NB];

};

#endif /* COP_STRDICT_H */

