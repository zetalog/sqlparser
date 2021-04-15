/*
 * hash.c	Non-thread-safe split-ordered hash table.
 *
 *  The weird "reverse" function is based on an idea from
 *  "Split-Ordered Lists - Lock-free Resizable Hash Tables", with
 *  modifications so that they're not lock-free. :(
 *
 *  However, the split-order idea allows a fast & easy splitting of the
 *  hash bucket chain when the hash table is resized.
 *
 *  This implementation can do ~10^6 lookups/s, which should be fast
 *  enough for most people.
 *
 * Version:	$Id: hash.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 *  Copyright 2005  The FreeRADIUS server project
 */

#include <proto.h>
#include <hash.h>

#ifdef TESTING
static int grow = 0;
#endif

/*
 * perl -e 'foreach $i (0..255) {$r = 0; foreach $j (0 .. 7 ) { if (($i & ( 1<< $j)) != 0) { $r |= (1 << (7 - $j));}} print $r, ", ";if (($i & 7) == 7) {print "\n";}}'
 */
static const uint8_t reversed_byte[256] = {
	0,  128, 64, 192, 32, 160, 96,  224,
	16, 144, 80, 208, 48, 176, 112, 240,
	8,  136, 72, 200, 40, 168, 104, 232,
	24, 152, 88, 216, 56, 184, 120, 248,
	4,  132, 68, 196, 36, 164, 100, 228,
	20, 148, 84, 212, 52, 180, 116, 244,
	12, 140, 76, 204, 44, 172, 108, 236,
	28, 156, 92, 220, 60, 188, 124, 252,
	2,  130, 66, 194, 34, 162, 98,  226,
	18, 146, 82, 210, 50, 178, 114, 242,
	10, 138, 74, 202, 42, 170, 106, 234,
	26, 154, 90, 218, 58, 186, 122, 250,
	6,  134, 70, 198, 38, 166, 102, 230,
	22, 150, 86, 214, 54, 182, 118, 246,
	14, 142, 78, 206, 46, 174, 110, 238,
	30, 158, 94, 222, 62, 190, 126, 254,
	1,  129, 65, 193, 33, 161, 97,  225,
	17, 145, 81, 209, 49, 177, 113, 241,
	9,  137, 73, 201, 41, 169, 105, 233,
	25, 153, 89, 217, 57, 185, 121, 249,
	5,  133, 69, 197, 37, 165, 101, 229,
	21, 149, 85, 213, 53, 181, 117, 245,
	13, 141, 77, 205, 45, 173, 109, 237,
	29, 157, 93, 221, 61, 189, 125, 253,
	3,  131, 67, 195, 35, 163, 99,  227,
	19, 147, 83, 211, 51, 179, 115, 243,
	11, 139, 75, 203, 43, 171, 107, 235,
	27, 155, 91, 219, 59, 187, 123, 251,
	7,  135, 71, 199, 39, 167, 103, 231,
	23, 151, 87, 215, 55, 183, 119, 247,
	15, 143, 79, 207, 47, 175, 111, 239,
	31, 159, 95, 223, 63, 191, 127, 255
};

/*
 * perl -e 'foreach $i (0..255) {$r = 0;foreach $j (0 .. 7) { $r = $i & (1 << (7 - $j)); last if ($r)} print $i & ~($r), ", ";if (($i & 7) == 7) {print "\n";}}'
 */
static uint8_t parent_byte[256] = {
	0, 0, 0, 1, 0, 1, 2, 3,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63,
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71,
	72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87,
	88, 89, 90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103,
	104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126, 127
};

/* reverse a key */
static uint32_t reverse(uint32_t key)
{
	return ((reversed_byte[key & 0xff] << 24) |
		(reversed_byte[(key >> 8) & 0xff] << 16) |
		(reversed_byte[(key >> 16) & 0xff] << 8) |
		(reversed_byte[(key >> 24) & 0xff]));
}

/* take the parent by discarding the highest bit that is set */
static uint32_t parent_of(uint32_t key)
{
	if (key > 0x00ffffff)
		return (key & 0x00ffffff) | (parent_byte[key >> 24] << 24);
	if (key > 0x0000ffff)
		return (key & 0x0000ffff) | (parent_byte[key >> 16] << 16);
	if (key > 0x000000ff)
		return (key & 0x000000ff) | (parent_byte[key >> 8] << 8);
	return parent_byte[key];
}

static hash_entry_t *hash_list_find(hash_table_t *ht,
				    hash_entry_t *head,
				    uint32_t reversed)
{
	hash_entry_t *cur;
	for (cur = head; cur != &ht->null; cur = cur->next) {
		if (cur->reversed == reversed) return cur;
		if (cur->reversed > reversed) return NULL;
	}
	return NULL;
}

/* inserts a new entry into the list, in order */
static int hash_list_insert(hash_table_t *ht,
			    hash_entry_t **head,
			    hash_entry_t *node)
{
	hash_entry_t **last, *cur;
	last = head;
	for (cur = *head; cur != &ht->null; cur = cur->next) {
		if (cur->reversed > node->reversed)
			break;
		last = &(cur->next);
		if (cur->reversed == node->reversed)
			return 0;
	}
	node->next = *last;
	*last = node;
	return 1;
}

/* delete an entry from the list */
static int hash_list_delete(hash_table_t *ht,
			    hash_entry_t **head,
			    hash_entry_t *node)
{
	hash_entry_t **last, *cur;

	last = head;
	for (cur = *head; cur != &ht->null; cur = cur->next) {
		if (cur == node)
			break;
		last = &(cur->next);
	}
	*last = node->next;
	return 1;
}

/* Create the table.
 * Memory usage in bytes is (20/3) * number of entries.
 */
hash_table_t *hash_table_create(hash_free_func freeNode)
{
	hash_table_t *ht;

	ht = malloc(sizeof(*ht));
	if (!ht) return NULL;

	memset(ht, 0, sizeof(*ht));
	ht->free = freeNode;
	ht->num_buckets = HASH_NUM_BUCKETS;
	ht->mask = ht->num_buckets - 1;

	/**
	 * Have a default load factor of 2.5.  In practice this
	 * means that the average load will hit 3 before the
	 * table grows.
	 */
	ht->next_grow = (ht->num_buckets << 1) + (ht->num_buckets >> 1);

	ht->buckets = malloc(sizeof (*ht->buckets) * ht->num_buckets);
	if (!ht->buckets) {
		free(ht);
		return NULL;
	}
	memset(ht->buckets, 0, sizeof (*ht->buckets) * ht->num_buckets);

	ht->null.reversed = ~0;
	ht->null.key = ~0;
	ht->null.next = &ht->null;

	ht->buckets[0] = &ht->null;
	return ht;
}

/**
 * If the current bucket is uninitialized, initialize it
 * by recursively copying information from the parent.
 *
 * We may have a situation where entry E is a parent to 2 other
 * entries E' and E".  If we split E into E and E', then the
 * nodes meant for E" end up in E or E', both of which are wrong.
 * To solve that problem, we walk down the whole chain, trying to
 * insert the elements into the correct place.
 *
 * This slows down the growth a bit, but it works.
 */
static void hash_table_fixup(hash_table_t *ht, uint32_t entry)
{
	uint32_t parent_entry = parent_of(entry);
	hash_entry_t **last, *cur;
	uint32_t this;

	parent_entry = parent_of(entry);

	/* parent_entry == entry if and only if entry == 0 */

	if (!ht->buckets[parent_entry]) {
		hash_table_fixup(ht, parent_entry);
	}	

	/* Keep walking down cur, trying to find entries that
	 * don't belong here any more.  There may be multiple
	 * ones, so we can't have a naive algorithm...
	 */
	last = &ht->buckets[parent_entry];
	this = parent_entry;
	
	for (cur = *last; cur != &ht->null; cur = cur->next) {
		uint32_t real_entry;

		real_entry = cur->key & ht->mask;
		if (real_entry != this) {
			/* ht->buckets[real_entry] == NULL */
			*last = &ht->null;
			ht->buckets[real_entry] = cur;
			this = real_entry;
		}

		last = &(cur->next);
	}

	/* we may NOT have initialized this bucket, so do it now */
	if (!ht->buckets[entry]) ht->buckets[entry] = &ht->null;
}

/* This should be a power of two.  Changing it to 4 doesn't seem
 * to make any difference.
 */
#define GROW_FACTOR	2

/* grow the hash table */
static void hash_table_grow(hash_table_t *ht)
{
	hash_entry_t **buckets;
	
	buckets = malloc(sizeof(*buckets) * GROW_FACTOR * ht->num_buckets);
	if (!buckets) return;

	memcpy(buckets, ht->buckets,
	       sizeof (*buckets) * ht->num_buckets);
	memset(&buckets[ht->num_buckets], 0,
	       sizeof (*buckets) * ht->num_buckets);
	
	free(ht->buckets);
	ht->buckets = buckets;
	ht->num_buckets *= GROW_FACTOR; 
	ht->next_grow *= GROW_FACTOR;
	ht->mask = ht->num_buckets - 1;
#ifdef TESTING
	grow = 1;
	fprintf(stderr, "GROW TO %d\n", ht->num_buckets);
#endif
}

/* insert data */
int hash_table_insert(hash_table_t *ht, uint32_t key, void *data)
{
	uint32_t entry;
	uint32_t reversed;
	hash_entry_t *node;

	if (!ht || !data) return 0;

	entry = key & ht->mask;
	reversed = reverse(key);

	if (!ht->buckets[entry]) hash_table_fixup(ht, entry);

	/* If we try to do our own memory allocation here, the
	 * speedup is only ~15% or so, which isn't worth it.
	 */
	node = malloc(sizeof(*node));
	if (!node) return 0;
	memset(node, 0, sizeof(*node));

	node->next = &ht->null;
	node->reversed = reversed;
	node->key = key;
	node->data = data;

	/* already in the table, can't insert it */
	if (!hash_list_insert(ht, &ht->buckets[entry], node)) {
		free(node);
		return 0;
	}

	/* Check the load factor, and grow the table if
	 * necessary.
	 */
	ht->num_elements++;
	if (ht->num_elements >= ht->next_grow) {
		hash_table_grow(ht);
	}
	return 1;
}

/* internal find a node routine */
static hash_entry_t *hash_table_find(hash_table_t *ht,
				     uint32_t key)
{
	uint32_t entry;
	uint32_t reversed;

	if (!ht) return NULL;
	entry = key & ht->mask;
	reversed = reverse(key);
	if (!ht->buckets[entry]) hash_table_fixup(ht, entry);
	return hash_list_find(ht, ht->buckets[entry], reversed);
}

/* replace old data with new data */
int hash_table_replace(hash_table_t *ht, uint32_t key, void *data)
{
	hash_entry_t *node;

	if (!data) return 0;
	node = hash_table_find(ht, key);
	if (!node) return hash_table_insert(ht, key, data);
	if (ht->free) ht->free(node->data);
	node->data = data;
	return 1;
}

/* find data from a template */
void *hash_table_find_data(hash_table_t *ht, uint32_t key)
{
	hash_entry_t *node;
	node = hash_table_find(ht, key);
	if (!node) return NULL;
	return node->data;
}

/* Yank an entry from the hash table, without freeing the data */
void *hash_table_yank(hash_table_t *ht, uint32_t key)
{
	uint32_t entry;
	uint32_t reversed;
	void *data;
	hash_entry_t *node;

	if (!ht) return 0;
	entry = key & ht->mask;
	reversed = reverse(key);

	if (!ht->buckets[entry]) hash_table_fixup(ht, entry);
	node = hash_list_find(ht, ht->buckets[entry], reversed);
	if (!node) return NULL;

	hash_list_delete(ht, &ht->buckets[entry], node);
	ht->num_elements--;

	data = node->data;
	free(node);
	return data;
}

/* delete a piece of data from the hash table */
int hash_table_delete(hash_table_t *ht, uint32_t key)
{
	void *data;

	data = hash_table_yank(ht, key);
	if (!data) return 0;

	if (ht->free) ht->free(data);

	return 1;
}

/* free a hash table */
void hash_table_free(hash_table_t *ht)
{
	hash_entry_t *node, *next;

	if (!ht) return;

	/* the entries MUST be all in one linked list */
	for (node = ht->buckets[0]; node != &ht->null; node = next) {
		next = node->next;
		if (!node->data) continue; /* dummy entry */
		if (ht->free) ht->free(node->data);
		free(node);
	}

	free(ht->buckets);
	free(ht);
}

/* count number of elements */
int lrad_hash_table_num_elements(hash_table_t *ht)
{
	if (!ht) return 0;
	return ht->num_elements;
}


/* walk over the nodes, allowing deletes & inserts to happen */
int hash_table_walk(hash_table_t *ht, hash_walk_func callback, void *context)
{
	int i, rcode;;

	if (!ht || !callback) return 0;

	for (i = 0; i < ht->num_buckets; i++) {
		hash_entry_t *node, *next;
		if (!ht->buckets[i]) continue;
		for (node = ht->buckets[i]; node != &ht->null; node = next) {
			next = node->next;
			rcode = callback(context, node->data);
			if (rcode != 0) return rcode;
		}
	}
	return 0;
}


#ifdef TESTING
/* show what the hash table is doing */
int hash_table_info(hash_table_t *ht)
{
	int i, a, collisions, uninitialized;
	int array[256];

	if (!ht) return 0;

	uninitialized = collisions = 0;
	memset(array, 0, sizeof(array));

	for (i = 0; i < ht->num_buckets; i++) {
		uint32_t key;
		int load;
		hash_entry_t *node, *next;

		/**
		 * If we haven't inserted or looked up an entry
		 * in a bucket, it's uninitialized.
		 */
		if (!ht->buckets[i]) {
			uninitialized++;
			continue;
		}

		load = 0;
		key = ~0;
		for (node = ht->buckets[i]; node != &ht->null; node = next) {
			if (node->reversed == key) {
				collisions++;
			} else {
				key = node->reversed;
			}
			next = node->next;
			load++;
		}

		if (load > 255) load = 255;
		array[load]++;
	}

	printf("HASH TABLE %p\tbuckets: %d\t(%d uninitialized)\n", ht,
		ht->num_buckets, uninitialized);
	printf("\tnum entries %d\thash collisions %d\n",
		ht->num_elements, collisions);

	a = 0;
	for (i = 1; i < 256; i++) {
		if (!array[i]) continue;
		printf("%d\t%d\n", i, array[i]);

		/**
		 * Since the entries are ordered, the lookup cost
		 * for any one element in a chain is (on average)
		 * the cost of walking half of the chain.
		 */
		if (i > 1) {
			a += array[i] * i;
		}
	}
	a /= 2;
	a += array[1];

	printf("\texpected lookup cost = %d/%d or %f\n\n",
	       ht->num_elements, a,
	       (float) ht->num_elements / (float) a);
	return 0;
}
#endif


#define FNV_MAGIC_INIT (0x811c9dc5)
#define FNV_MAGIC_PRIME (0x01000193)

/*
 * A fast hash function.  For details, see:
 *
 * http://www.isthe.com/chongo/tech/comp/fnv/
 *
 * Which also includes public domain source.  We've re-written
 * it here for our purposes.
 */
uint32_t fnv_hash_buffer(const void *data, size_t size)
{
	const uint8_t *p = data;
	const uint8_t *q = p + size;
	uint32_t      hash = FNV_MAGIC_INIT;

	/* FNV-1 hash each octet in the buffer */
	while (p != q) {
		/* Multiple by 32-bit magic FNV prime, mod 2^32 */
		hash *= FNV_MAGIC_PRIME;
#if 0
		/* Potential optimization */
		hash += (hash<<1) + (hash<<4) + (hash<<7) + (hash<<8) + (hash<<24);
#endif
		/**
		 * XOR the 8-bit quantity into the bottom of
		 * the hash.
		 */
		hash ^= (uint32_t) (*p++);
    }

    return hash;
}

/* Continue hashing data */
uint32_t fnv_hash_update(const void *data, size_t size, uint32_t hash)
{
	const uint8_t *p = data;
	const uint8_t *q = p + size;
	
	while (p != q) {
		hash *= FNV_MAGIC_PRIME;
		hash ^= (uint32_t) (*p++);
	}
	return hash;
}

/**
 * Return a "folded" hash, where the lower "bits" are the
 * hash, and the upper bits are zero.
 * If you need a non-power-of-two hash, cope.
 */
uint32_t fnv_hash_fold(uint32_t hash, int bits)
{
	int count;
	uint32_t result;

	if ((bits <= 0) || (bits >= 32)) return hash;
	result = hash;

	/* Never use the same bits twice in an xor */
	for (count = 0; count < 32; count += bits) {
		hash >>= bits;
		result ^= hash;
	}
	return result & (((uint32_t) (1 << bits)) - 1);
}

/* Hash a C string, so we loop over it once */
uint32_t fnv_hash_string(const char *p)
{
	uint32_t hash = FNV_MAGIC_INIT;

	while (*p) {
		hash *= FNV_MAGIC_PRIME;
		hash ^= (uint32_t) (*p++);
	}
	return hash;
}


#ifdef TESTING
/*
 *  cc -DTESTING -I ../include/ hash.c -o hash
 *
 *  ./hash
 */
#include <stdio.h>
#include <stdlib.h>

#define MAX 1024*64
int main(int argc, char **argv)
{
	uint32_t hash;
	int i, *p, *q;
	hash_table_t *ht;
	int *array;

	ht = hash_table_create(NULL);
	if (!ht) {
		fprintf(stderr, "Hash create failed\n");
		exit(1);
	}

	array = malloc(sizeof(int) * MAX);

	for (i = 0; i < MAX; i++) {
		p = array + i;
		*p = i;
		hash = fnv_hash_buffer(&i, sizeof(i));

		if (!hash_table_insert(ht, hash, p)) {
			fprintf(stderr, "Failed insert %08x\n", i);
			exit(1);
		}
#ifdef TEST_INSERT
		q = hash_table_find_data(ht, hash);
		if (q != p) {
			fprintf(stderr, "Bad data %d\n", i);
			exit(1);
		}
#endif
	}

	hash_table_info(ht);

	/* build this to see how lookups result in shortening
	 * of the hash chains
	 */
	if (0) {
		for (i = 0; i < MAX ; i++) {
			hash = fnv_hash_buffer(&i, sizeof(i));
			q = hash_table_find_data(ht, hash);
			if (!q || *q != i) {
				fprintf(stderr, "Failed finding %d\n", i);
				exit(1);
			}
#if 0
			if (!hash_table_delete(ht, hash)) {
				fprintf(stderr, "Failed deleting %d\n", i);
				exit(1);
			}
			q = hash_table_find_data(ht, hash);
			if (q) {
				fprintf(stderr, "Failed to delete %08x\n", i);
				exit(1);
			}
#endif
		}

		hash_table_info(ht);
	}

	hash_table_free(ht);
	free(array);

	exit(0);
}
#endif
