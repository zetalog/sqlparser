#include <uuid.h>

#include <md5.h>
#include <sha1.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifndef WIN32
#include <sys/sysinfo.h>
#else
#include <iphlpapi.h>
#include <ipifcons.h>
#endif

/* set the following to the number of 100ns ticks of the actual
   resolution of your system's clock */
#define UUIDS_PER_TICK 1024

#define LOCK
#define UNLOCK

/* various forward declarations */
static int read_state(uint16_t *clockseq, uuid_time_t *timestamp,
		      uuid_node_t *node);
static void write_state(uint16_t clockseq, uuid_time_t timestamp,
			uuid_node_t node);
static void format_uuid_v1(uuid_t *uuid, uint16_t clockseq,
			   uuid_time_t timestamp, uuid_node_t node);

static void format_uuid_v3or5(uuid_t *uuid, unsigned char hash[16],
			      int v);
static void get_current_time(uuid_time_t *timestamp);
static uint16_t true_random(void);

void get_random_info(char seed[16]);

/* system dependent call to get IEEE node ID.
   This sample implementation generates a random node ID. */
void get_ieee_node_identifier(uuid_node_t *node)
{
	static inited = 0;
	static uuid_node_t saved_node;
	char seed[16];
	
	if (!inited) {
#ifdef WIN32
		PIP_ADAPTER_INFO pAdapterInfo;
		IP_ADAPTER_INFO AdapterInfo[16];
		DWORD dwBufLen = sizeof(AdapterInfo);
		DWORD dwStatus;
		
		dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
		if (dwStatus == ERROR_SUCCESS) {
			pAdapterInfo = AdapterInfo;
			do {
				if (pAdapterInfo->Type == IF_TYPE_ETHERNET_CSMACD) {
					memcpy(&saved_node, pAdapterInfo->Address, 6);
					inited = 1;
					break;
				}
				pAdapterInfo = pAdapterInfo->Next;
			} while (pAdapterInfo);
		}
	}
#endif
	if (!inited) {
		get_random_info(seed);
		seed[0] |= 0x01;
		memcpy(&saved_node, seed, sizeof saved_node);
	}
	*node = saved_node;
}

/* system dependent call to get the current system time. Returned as
   100ns ticks since UUID epoch, but resolution may be less than
   100ns. */
#ifdef _WINDOWS_

void get_system_time(uuid_time_t *uuid_time)
{
	ULARGE_INTEGER time;
	
	/* NT keeps time in FILETIME format which is 100ns ticks since
	   Jan 1, 1601. UUIDs use time in 100ns ticks since Oct 15, 1582.
	   The difference is 17 Days in Oct + 30 (Nov) + 31 (Dec)
	   + 18 years and 5 leap days. */
	GetSystemTimeAsFileTime((FILETIME *)&time);
	time.QuadPart +=
		(uint64_t) (1000*1000*10)       // seconds
		* (uint64_t) (60 * 60 * 24)       // days
		* (uint64_t) (17+30+31+365*18+5); // # of days
	*uuid_time = time.QuadPart;
}

/* Sample code, not for use in production; see RFC 1750 */
void get_random_info(char seed[16])
{
	md5_t c;
	struct {
		MEMORYSTATUS m;
		SYSTEM_INFO s;
		FILETIME t;
		LARGE_INTEGER pc;
		DWORD tc;
		DWORD l;
		char hostname[MAX_COMPUTERNAME_LENGTH + 1];
	} r;
	
	md5_init(&c);
	GlobalMemoryStatus(&r.m);
	GetSystemInfo(&r.s);
	GetSystemTimeAsFileTime(&r.t);
	QueryPerformanceCounter(&r.pc);
	r.tc = GetTickCount();
	
	
	r.l = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerName(r.hostname, &r.l);
	md5_update(&c, (uint8_t *)&r, sizeof r);
	md5_final((unsigned char *)seed, &c);
}

#else

void get_system_time(uuid_time_t *uuid_time)
{
	struct timeval tp;
	
	gettimeofday(&tp, (struct timezone *)0);
	
	/* Offset between UUID formatted times and Unix formatted times.
	   UUID UTC base time is October 15, 1582.
	   Unix base time is January 1, 1970.*/
	*uuid_time = ((uint64_t)tp.tv_sec * 10000000)
		+ ((uint64_t)tp.tv_usec * 10)
		+ 0x01B21DD213814000LL;
}

/* Sample code, not for use in production; see RFC 1750 */
void get_random_info(char seed[16])
{
	md5_t c;
	struct {
		struct sysinfo s;
		struct timeval t;
		char hostname[257];
	} r;
	
	md5_init(&c);
	sysinfo(&r.s);
	gettimeofday(&r.t, (struct timezone *)0);
	gethostname(r.hostname, 256);
	md5_update(&c, (uint8_t *)&r, sizeof r);
	md5_final(seed, &c);
}

#endif

/* uuid_create -- generator a UUID */
int uuid_create(uuid_t *uuid)
{
	uuid_time_t timestamp, last_time;
	uint16_t clockseq;
	uuid_node_t node;
	uuid_node_t last_node;
	int f;

	/* acquire system-wide lock so we're alone */
	LOCK;
	/* get time, node ID, saved state from non-volatile storage */
	get_current_time(&timestamp);
	get_ieee_node_identifier(&node);
	f = read_state(&clockseq, &last_time, &last_node);

	/* if no NV state, or if clock went backwards, or node ID
	changed (e.g., new network card) change clockseq */
	if (!f || memcmp(&node, &last_node, sizeof node))
		clockseq = true_random();
	else if (timestamp < last_time)
		clockseq++;

	/* save the state for next time */
	write_state(clockseq, timestamp, node);

	UNLOCK;

	/* stuff fields into the UUID */
	format_uuid_v1(uuid, clockseq, timestamp, node);
	return 1;
}

/*
 * format_uuid_v1 -- make a UUID from the timestamp, clockseq,
 *                   and node ID
 */
void format_uuid_v1(uuid_t* uuid, uint16_t clock_seq,
                    uuid_time_t timestamp, uuid_node_t node)
{
	/* Construct a version 1 uuid with the information we've gathered
	   plus a few constants. */
	uuid->time_low = (unsigned long)(timestamp & 0xFFFFFFFF);
	uuid->time_mid = (unsigned short)((timestamp >> 32) & 0xFFFF);
	uuid->time_hi_and_version =
		(unsigned short)((timestamp >> 48) & 0x0FFF);
	uuid->time_hi_and_version |= (1 << 12);
	uuid->clock_seq_low = clock_seq & 0xFF;
	uuid->clock_seq_hi_and_reserved = (clock_seq & 0x3F00) >> 8;
	uuid->clock_seq_hi_and_reserved |= 0x80;
	memcpy(&uuid->node, &node, sizeof uuid->node);
}

/* data type for UUID generator persistent state */
typedef struct {
	uuid_time_t  ts;       /* saved timestamp */
	uuid_node_t  node;     /* saved node ID */
	uint16_t   cs;       /* saved clock sequence */
} uuid_state;

static uuid_state st;

/* read_state -- read UUID generator state from non-volatile store */
int read_state(uint16_t *clockseq, uuid_time_t *timestamp,
               uuid_node_t *node)
{
	static int inited = 0;
	FILE *fp;
	
	/* only need to read state once per boot */
	if (!inited) {
		fp = fopen("state", "rb");
		if (fp == NULL)
			return 0;
		fread(&st, sizeof st, 1, fp);
		fclose(fp);
		inited = 1;
	}
	*clockseq = st.cs;
	*timestamp = st.ts;
	*node = st.node;
	return 1;
}

/*
 * write_state -- save UUID generator state back to non-volatile
 *                storage
 */
void write_state(uint16_t clockseq, uuid_time_t timestamp,
                 uuid_node_t node)
{
	static int inited = 0;
	static uuid_time_t next_save;
	FILE* fp;
	if (!inited) {
		next_save = timestamp;
		inited = 1;
	}
	
	/* always save state to volatile shared state */
	st.cs = clockseq;
	st.ts = timestamp;
	st.node = node;
	if (timestamp >= next_save) {
		fp = fopen("state", "wb");
		fwrite(&st, sizeof st, 1, fp);
		fclose(fp);
		/* schedule next save for 10 seconds from now */
		next_save = timestamp + (10 * 10 * 1000 * 1000);
	}
}

/* get-current_time -- get time as 60-bit 100ns ticks since UUID epoch.
   Compensate for the fact that real clock resolution is
   less than 100ns. */
void get_current_time(uuid_time_t *timestamp)
{
	static int inited = 0;
	static uuid_time_t time_last;
	static uint16_t uuids_this_tick;
	uuid_time_t time_now;
	
	if (!inited) {
		get_system_time(&time_now);
		uuids_this_tick = UUIDS_PER_TICK;
		inited = 1;
	}
	
	for ( ; ; ) {
		get_system_time(&time_now);
		
		/* if clock reading changed since last UUID generated, */
		if (time_last != time_now) {
			/* reset count of uuids gen'd with this clock reading */
			uuids_this_tick = 0;
			time_last = time_now;
			break;
		}
		if (uuids_this_tick < UUIDS_PER_TICK) {
			uuids_this_tick++;
			break;
		}
		/* going too fast for our clock; spin */
	}
	/* add the count of uuids to low order bits of the clock reading */
	*timestamp = time_now + uuids_this_tick;
}

/*
 * true_random -- generate a crypto-quality random number.
 * **This sample doesn't do that.**
 */
static uint16_t true_random(void)
{
	static int inited = 0;
	uuid_time_t time_now;
	
	if (!inited) {
		get_system_time(&time_now);
		time_now = time_now / UUIDS_PER_TICK;
		srand((unsigned int)
			(((time_now >> 32) ^ time_now) & 0xffffffff));
		inited = 1;
	}
	return rand();
}

/*
 * uuid_create_md5_from_name -- create a version 3 (MD5) UUID using a
 *                              "name" from a "name space"
 */
void uuid_create_md5_from_name(uuid_t *uuid, uuid_t nsid, void *name,
                               int namelen)
{
	md5_t c;
	unsigned char hash[MD5_DIGESTSIZE];
	uuid_t net_nsid;
	
	/* put name space ID in network byte order so it hashes the same
	   no matter what endian machine we're on */
	net_nsid = nsid;
	net_nsid.time_low = htonl(net_nsid.time_low);
	net_nsid.time_mid = htons(net_nsid.time_mid);
	net_nsid.time_hi_and_version = htons(net_nsid.time_hi_and_version);
	
	md5_init(&c);
	md5_update(&c, (uint8_t *)&net_nsid, sizeof net_nsid);
	md5_update(&c, name, namelen);
	md5_final(hash, &c);
	
	/* the hash is in network byte order at this point */
	format_uuid_v3or5(uuid, hash, 3);
}


void uuid_create_sha1_from_name(uuid_t *uuid, uuid_t nsid, void *name,
                                int namelen)
{
	sha1_t c;
	unsigned char hash[SHA1_DIGESTSIZE];
	uuid_t net_nsid;
	unsigned char *data = NULL;
	
	/*
	 * put name space ID in network byte order so it hashes the same
	 * no matter what endian machine we're on
	 */
	net_nsid = nsid;
	net_nsid.time_low = htonl(net_nsid.time_low);
	net_nsid.time_mid = htons(net_nsid.time_mid);
	net_nsid.time_hi_and_version = htons(net_nsid.time_hi_and_version);
	
	sha1_init(&c);
	data = malloc(sizeof (net_nsid) + namelen);
	if (data) {
		/* XXX: this is required because sizeof(net_nsid) < SHA1_BLOCKSIZE */
		memcpy(data, &net_nsid, sizeof net_nsid);
		memcpy(data + sizeof net_nsid, name, namelen);
		sha1_update(&c, (uint8_t *)data, sizeof (net_nsid) + namelen);
		free(data);
	}
	sha1_final(hash, &c);
	
	/* the hash is in network byte order at this point */
	format_uuid_v3or5(uuid, hash, 5);
}

/*
 * format_uuid_v3or5 -- make a UUID from a (pseudo)random 128-bit
 *                      number
 */
void format_uuid_v3or5(uuid_t *uuid, unsigned char hash[16], int v)
{
	/* convert UUID to local byte order */
	memcpy(uuid, hash, sizeof *uuid);
	uuid->time_low = ntohl(uuid->time_low);
	uuid->time_mid = ntohs(uuid->time_mid);
	uuid->time_hi_and_version = ntohs(uuid->time_hi_and_version);
	
	/* put in the variant and version bits */
	uuid->time_hi_and_version &= 0x0FFF;
	uuid->time_hi_and_version |= (v << 12);
	uuid->clock_seq_hi_and_reserved &= 0x3F;
	uuid->clock_seq_hi_and_reserved |= 0x80;
}

/* uuid_compare --  Compare two UUID's "lexically" and return */
#define CHECK(f1, f2) if (f1 != f2) return f1 < f2 ? -1 : 1;
int uuid_compare(uuid_t *u1, uuid_t *u2)
{
	int i;
	
	CHECK(u1->time_low, u2->time_low);
	CHECK(u1->time_mid, u2->time_mid);
	CHECK(u1->time_hi_and_version, u2->time_hi_and_version);
	CHECK(u1->clock_seq_hi_and_reserved, u2->clock_seq_hi_and_reserved);
	CHECK(u1->clock_seq_low, u2->clock_seq_low)
	for (i = 0; i < 6; i++) {
		if (u1->node[i] < u2->node[i])
			return -1;
		if (u1->node[i] > u2->node[i])
			return 1;
	}
	return 0;
}
#undef CHECK

static int uuid_isstr(const char *str, size_t str_len)
{
	int i;
	const char *cp;

	/*
	 * example reference:
	 * f81d4fae-7dec-11d0-a765-00a0c91e6bf6
	 * 012345678901234567890123456789012345
	 * 0         1         2         3     
	 */
	if (str == NULL)
		return 0;
	if (str_len == 0)
		str_len = strlen(str);
	if (str_len < UUID_LEN_STR)
		return 0;
	for (i = 0, cp = str; i < UUID_LEN_STR; i++, cp++) {
		if ((i == 8) || (i == 13) || (i == 18) || (i == 23)) {
			if (*cp == '-')
				continue;
			else
				return 0;
		}
		if (!isxdigit((int)(*cp)))
			return 0;
	}
	return 1;
}

uuid_t *uuid_import(const char *s)
{
	static uuid_t u;
	uint16_t tmp16;
	const char *cp;
	char hexbuf[3];
	const char *str;
	unsigned int i;
	
	/* check for correct UUID string representation syntax */
	str = (const char *)s;
	if (!uuid_isstr(str, 0)) return 0;
	
	/* parse hex values of "time" parts */
	u.time_low            = (uint32_t)strtoul(str,    NULL, 16);
	u.time_mid            = (uint16_t)strtoul(str+9,  NULL, 16);
	u.time_hi_and_version = (uint16_t)strtoul(str+14, NULL, 16);
	
	/* parse hex values of "clock" parts */
	tmp16 = (uint16_t)strtoul(str+19, NULL, 16);
	u.clock_seq_low             = (uint8_t)(tmp16 & 0xff); tmp16 >>= 8;
	u.clock_seq_hi_and_reserved = (uint8_t)(tmp16 & 0xff);
	
	/* parse hex values of "node" part */
	cp = str+24;
	hexbuf[2] = '\0';
	for (i = 0; i < (unsigned int)sizeof(u.node); i++) {
		hexbuf[0] = *cp++;
		hexbuf[1] = *cp++;
		u.node[i] = (uint8_t)strtoul(hexbuf, NULL, 16);
	}
	return &u;
}

const char *uuid_export(uuid_t *u)
{
	static char s[UUID_LEN_STR+1];

	if (!u) return 0;
	if (snprintf(s, UUID_LEN_STR+1,
		     "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		     (unsigned long)u->time_low,
		     (unsigned int)u->time_mid,
		     (unsigned int)u->time_hi_and_version,
		     (unsigned int)u->clock_seq_hi_and_reserved,
		     (unsigned int)u->clock_seq_low,
		     (unsigned int)u->node[0],
		     (unsigned int)u->node[1],
		     (unsigned int)u->node[2],
		     (unsigned int)u->node[3],
		     (unsigned int)u->node[4],
		     (unsigned int)u->node[5]) != UUID_LEN_STR) {
		return 0;
	}
	return s;
}
