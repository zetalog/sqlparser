#ifndef __RFC4122_UUID_H_INCLUDE__
#define __RFC4122_UUID_H_INCLUDE__

#include <proto.h>

#undef uuid_t

typedef uint64_t uuid_time_t;

typedef struct {
	char nodeID[6];
} uuid_node_t;

typedef struct {
	uint32_t  time_low;
	uint16_t  time_mid;
	uint16_t  time_hi_and_version;
	uint8_t   clock_seq_hi_and_reserved;
	uint8_t   clock_seq_low;
	uint8_t   node[6];
} uuid_t;

/* uuid_create -- generate a UUID */
int uuid_create(uuid_t * uuid);

/* uuid_create_md5_from_name -- create a version 3 (MD5) UUID using a
   "name" from a "name space" */
void uuid_create_md5_from_name(uuid_t *uuid,	/* resulting UUID */
			       uuid_t nsid,	/* UUID of the namespace */
			       void *name,	/* the name from which to generate a UUID */
			       int namelen);	/* the length of the name */


/* uuid_create_sha1_from_name -- create a version 5 (SHA-1) UUID
   using a "name" from a "name space" */
void uuid_create_sha1_from_name(uuid_t *uuid,	/* resulting UUID */
				uuid_t nsid,	/* UUID of the namespace */
				void *name,	/* the name from which to generate a UUID */
				int namelen);	/* the length of the name */

/* uuid_compare --  Compare two UUID's "lexically" and return
        -1   u1 is lexically before u2
         0   u1 is equal to u2
         1   u1 is lexically after u2
   Note that lexical ordering is not temporal ordering!
*/
int uuid_compare(uuid_t *u1, uuid_t *u2);

#define UUID_LEN_STR	36
const char *uuid_export(uuid_t *u);
uuid_t *uuid_import(const char *s);

void testuuid(int argc, char **argv);

#endif /* __RFC4122_UUID_H_INCLUDE__ */
