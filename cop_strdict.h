#ifndef COP_STRDICT_H
#define COP_STRDICT_H

#include "cop_strtypes.h"
#include <stddef.h>

/* This structure is defined later in this header. Don't access members
 * directly. Use the accessor functions below. */
struct cop_strdict_node;

/* ---------------------------------------------------------------------------
 * Dictionary initialisation and cleanup
 * ------------------------------------------------------------------------ */

/* Initialise an empty dictionary. You do not strictly need to use this
 * function and may always assign the root-node pointer to null. There are no
 * cleanup functions as this library does not perform any allocations. If you
 * dynamically allocate nodes and wish to free them, you may use the enumerate
 * method to achieve this. */
static COP_ATTR_UNUSED struct cop_strdict_node *cop_strdict_init(void) {
	return NULL;
}

/* ---------------------------------------------------------------------------
 * Node initialisation prior to insertion
 * 
 * Use the below functions to prepare a node to be inserted into the
 * dictionary using cop_strdict_insert() or extract the key or data from an
 * a node that has been setup.
 * ------------------------------------------------------------------------ */

/* Setup a new node with a key and data pointer using a cop_strh structure
 * for the key.
 *
 * The function takes a cop_strdict_node structure and fills in the required
 * parameters. The cop_strh structure will be used for the key and must have a
 * persistent data pointer. p_data is the initial data pointer value. No
 * allocations take place. It is undefined for p_node or p_strh to be null. */
void
cop_strdict_node_init
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
 * No allocations take place. It is undefined for p_node or p_persistend_cstr
 * to be null. */
void
cop_strdict_node_init_by_cstr
	(struct cop_strdict_node *p_node
	,const char              *p_persistent_cstr
	,void                    *p_data
	);

/* Get the key from an initialised node. The function never fails. It is
 * undefined for p_node to be null. */
void cop_strdict_node_to_key(const struct cop_strdict_node *p_node, struct cop_strh *p_key);

/* Return the data pointer from an initialised node. The function never
 * fails. It is undefined for p_node to be null. */
void *cop_strdict_node_to_data(const struct cop_strdict_node *p_node);

/* ---------------------------------------------------------------------------
 * Dictionary insertion, retrieval, modification and deletion methods
 * ------------------------------------------------------------------------ */

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
	(const struct cop_strdict_node  *p_root
	,const struct cop_strh          *p_key
	,void                         **pp_value
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
	(const struct cop_strdict_node  *p_root
	,const char                     *p_key
	,void                          **pp_value
	);

int /* zero on success, non-zero when key does not exist */
cop_strdict_update
	(struct cop_strdict_node *p_root
	,const struct cop_strh   *p_key
	,void                    *p_value
	);
int /* zero on success, non-zero when key does not exist */
cop_strdict_update_by_cstr
	(struct cop_strdict_node *p_root
	,const char              *p_key
	,void                    *p_value
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

/* ---------------------------------------------------------------------------
 * Enumeration of dictionary keys and values.
 * ------------------------------------------------------------------------ */

/* Return non-zero in the enumeration function to signal an early termination
 * of the enumeration. p_context will have the value passed as p_context to
 * the call to cop_strdict_enumerate. p_node is the current node and may be
 * examined using the cop_strdict_node_to_* functions - it is never null.
 * depth is the height of the node in the tree and is purely informational. */
typedef int (cop_strdict_enumerate_fn)(void *p_context, struct cop_strdict_node *p_node, int depth);

/* Enumerate the keys in the dictionary. The return value is zero if all calls
 * to the callback return zero. If a callback function ever returns non-zero,
 * the enumeration terminates and the return value of the terminating callback
 * is returned.
 * 
 * The ordering of the enumeration is strictly leaves-first. This permits
 * using the enumerate function to clean up memory for dynamically allocated
 * node objects (i.e. you may call free(p_node) in the callback function as
 * all the child nodes will have already been enumerated). The value of
 * p_context is passed to the callback function. p_root is the root node which
 * may be NULL (which will result in no calls to the callback). */
int
cop_strdict_enumerate
	(struct cop_strdict_node  *p_root
	,cop_strdict_enumerate_fn *p_fn
	,void                     *p_context
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

