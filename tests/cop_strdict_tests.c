#include "cop/cop_strdict.h"
#include "cop/cop_alloc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


struct cop_strdict_node *make_node(struct cop_salloc_iface *iface, const char *p_id) {
	size_t                   sl    = strlen(p_id) + 1;
	struct cop_strdict_node *p_ret = cop_salloc(iface, sizeof(struct cop_strdict_node) + sl, 0);
	if (p_ret == NULL) {
		fprintf(stderr, "out of memory\n");
		abort();
	}
	memcpy(p_ret + 1, p_id, sl);
	cop_strdict_setup_by_cstr(p_ret, (const char *)(p_ret + 1), (void *)(p_ret + 1));
	return p_ret;
}

int expect_insert_ok(struct cop_strdict *p_dict, struct cop_strdict_node *p_node) {
	if (cop_strdict_insert(p_dict, p_node)) {
		fprintf(stderr, "expected cop_strdict_insert to succeed\n");
		return -1;
	}
	if (!cop_strdict_insert(p_dict, p_node)) {
		fprintf(stderr, "expected cop_strdict_insert to fail after calling again\n");
		return -1;
	}
	return 0;
}

int expect_delete_ok(struct cop_strdict *p_dict, const char *p_key) {
	if (cop_strdict_delete_by_cstr(p_dict, p_key) == NULL) {
		fprintf(stderr, "expected cop_strdict_delete_by_cstr to succeed\n");
		return -1;
	}
	if (cop_strdict_delete_by_cstr(p_dict, p_key) != NULL) {
		fprintf(stderr, "expected cop_strdict_delete_by_cstr to fail after succeeding\n");
		return -1;
	}
	return 0;
}

int expect_exists(struct cop_strdict *p_dict, const char *p_key) {
	if (cop_strdict_get_by_cstr(p_dict, p_key, NULL)) {
		fprintf(stderr, "expected to find key %s using cop_strdict_get_by_cstr\n", p_key);
		return -1;
	}
	return 0;
}

int expect_removed(struct cop_strdict *p_dict, const char *p_key) {
	if (!cop_strdict_get_by_cstr(p_dict, p_key, NULL)) {
		fprintf(stderr, "expected to not find key %s using cop_strdict_get_by_cstr\n", p_key);
		return -1;
	}
	return 0;
}

int expect_update(struct cop_strdict *p_dict, const char *p_key) {
	void *existing_val;
	void *new_val;
	if (cop_strdict_get_by_cstr(p_dict, p_key, &existing_val)) {
		fprintf(stderr, "expected to not find key %s using cop_strdict_get_by_cstr\n", p_key);
		return -1;
	}
	if (strcmp(p_key, existing_val)) {
		fprintf(stderr, "expected %s to have data that is its key string (was %s)\n", p_key, existing_val);
		return -1;
	}
	if (cop_strdict_update_by_cstr(p_dict, p_key, NULL)) {
		fprintf(stderr, "expected to be able to update key value for %s\n", p_key);
		return -1;
	}
	if (cop_strdict_get_by_cstr(p_dict, p_key, &new_val)) {
		fprintf(stderr, "expected to not be able to get value for key %s\n", p_key);
		return -1;
	}
	if (new_val != NULL) {
		fprintf(stderr, "key value should have been NULL after update\n");
		return -1;
	}
	if (cop_strdict_update_by_cstr(p_dict, p_key, existing_val)) {
		fprintf(stderr, "expected to be able to update key value for %s again\n", p_key);
		return -1;
	}
	if (cop_strdict_get_by_cstr(p_dict, p_key, &new_val)) {
		fprintf(stderr, "expected to be able to get value for key %s\n", p_key);
		return -1;
	}
	if (new_val != existing_val) {
		fprintf(stderr, "key value should have been the existing value after update\n");
		return -1;
	}
	return 0;
}

static int enumfn(void *p_context, struct cop_strdict_node *p_node, int depth) {
	printf("%*s%s\n", depth*2, "", p_node->key_data);
	return 0;
}


/* Ensure some collisions */
uint_fast32_t get_id(uint_fast32_t x) {
	if (x < 64)
		return 526746 + x;
	else
		return (1456900 + (x - 64)) & 0xFFFFFFFF;
}

static void makekey(char *p_buf, uint_fast64_t key) {
	key      = get_id(key);
	p_buf[0] = '0' + ((key / 10000000) % 10);
	p_buf[1] = '0' + ((key / 1000000) % 10);
	p_buf[2] = '0' + ((key / 100000) % 10);
	p_buf[3] = '0' + ((key / 10000) % 10);
	p_buf[4] = '0' + ((key / 1000) % 10);
	p_buf[5] = '0' + ((key / 100) % 10);
	p_buf[6] = '0' + ((key / 10) % 10);
	p_buf[7] = '0' + (key % 10);
	p_buf[8] = '\0';
}

#define ALLOCATIONS (128)

int runtests(struct cop_salloc_iface *iface) {
	struct cop_strdict dict;
	unsigned i;
	char keystr[32];

	cop_strdict_init(&dict);

	for (i = 0; i < ALLOCATIONS; i++) {
		makekey(keystr, i);
		if (expect_insert_ok(&dict, make_node(iface, keystr)))
			return -1;

	}

	for (i = 0; i < ALLOCATIONS; i++) {
		makekey(keystr, i);
		if (expect_exists(&dict, keystr))
			return -1;
	}

	for (i = 3; i <= 13; i++) {
		makekey(keystr, i);
		if (expect_delete_ok(&dict, keystr))
			return -1;
	}

	for (i = 0; i < 2; i++) {
		makekey(keystr, i);
		if (expect_exists(&dict, keystr))
			return -1;
	}
	for (i = 3; i <= 13; i++) {
		makekey(keystr, i);
		if (expect_removed(&dict, keystr))
			return -1;
	}
	for (i = 14; i < ALLOCATIONS; i++) {
		makekey(keystr, i);
		if (expect_exists(&dict, keystr))
			return -1;
	}

	for (i = 5; i <= 10; i++) {
		makekey(keystr, i);
		if (expect_insert_ok(&dict, make_node(iface, keystr)))
			return -1;
	}

	for (i = 0; i < 2; i++) {
		makekey(keystr, i);
		if (expect_exists(&dict, keystr))
			return -1;
	}
	for (i = 3; i <= 4; i++) {
		makekey(keystr, i);
		if (expect_removed(&dict, keystr))
			return -1;
	}
	for (i = 5; i <= 10; i++) {
		makekey(keystr, i);
		if (expect_exists(&dict, keystr))
			return -1;
	}
	for (i = 11; i <= 13; i++) {
		makekey(keystr, i);
		if (expect_removed(&dict, keystr))
			return -1;
	}
	for (i = 14; i < ALLOCATIONS; i++) {
		makekey(keystr, i);
		if (expect_exists(&dict, keystr))
			return -1;
	}

	for (i = 20; i < 54; i++) {
		makekey(keystr, i);
		if (expect_update(&dict, keystr))
			return -1;
	}



	//cop_strdict_enumerate(&dict, enumfn, NULL);

	return 0;
}

int main(int argc, char *argv[]) {
	struct cop_alloc_virtual mem;
	struct cop_salloc_iface  iface;
	int                      rflag;

	if (cop_alloc_virtual_init(&mem, &iface, 1024*1024*512, 16, 1024*1024))
		abort();

	rflag = runtests(&iface);

	cop_alloc_virtual_free(&mem);

	return (rflag) ? EXIT_FAILURE : EXIT_SUCCESS;
}
