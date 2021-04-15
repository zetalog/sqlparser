
#ifndef __HASH_TABLE_H_INCLUDE__
#define __HASH_TABLE_H_INCLUDE__

typedef void (*hash_free_func)(void *);
typedef int (*hash_walk_func)(void * /* ctx */, void * /* data */);

typedef struct _hash_entry_t {
	struct _hash_entry_t *next;
	uint32_t reversed;
	uint32_t key;
 	void *data;
} hash_entry_t;

typedef struct _hash_table_t {
	int num_elements;
	int num_buckets; /* power of 2 */
	int next_grow;
	int mask;
	hash_free_func free;
	hash_entry_t null;
	hash_entry_t **buckets;
} hash_table_t;

/*
 * A reasonable number of buckets to start off with.
 * Should be a power of two.
 */
#define HASH_NUM_BUCKETS	64

/**
 * Fast hash, which isn't too bad.  Don't use for cryptography,
 * just for hashing internal data.
 */
uint32_t fnv_hash_buffer(const void *data, size_t size);
uint32_t fnv_hash_update(const void *data, size_t size, uint32_t hash);
uint32_t fnv_hash_string(const char *p);

hash_table_t *hash_table_create(hash_free_func freeNode);
void hash_table_free(hash_table_t *ht);
int hash_table_insert(hash_table_t *ht, uint32_t key, void *data);
int hash_table_delete(hash_table_t *ht, uint32_t key);
void *hash_table_yank(hash_table_t *ht, uint32_t key);
int hash_table_replace(hash_table_t *ht, uint32_t key, void *data);
void *hash_table_find_data(hash_table_t *ht, uint32_t key);
int hash_table_num_elements(hash_table_t *ht);
int hash_table_walk(hash_table_t *ht, hash_walk_func callback, void *ctx);

#endif /* __HASH_TABLE_H_INCLUDE__ */
