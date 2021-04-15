

#ifndef __PROTO_H_INCLUDE__
#define __PROTO_H_INCLUDE__

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#ifdef WIN32
/* unreferenced formal parameter */
//#pragma warning (3: 4100)
/* conditional expression is constant */
//#pragma warning (3: 4127)
/* local variable is initialized but not referenced */
#pragma warning (3: 4189)
/* local variable %s may be used without having been initialized */
//#pragma warning (3: 4701)
/* assignment within conditional expression */
//#pragma warning (3: 4706)
#include <winsock.h>
#include <process.h>
#include <io.h>
#define PACKAGE_VERSION		"1.0.0"
#define vsnprintf	_vsnprintf
#define snprintf	_snprintf
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#define S_ISREG(x)	(x & _S_IFREG)
#define S_ISDIR(x)	(x & _S_IFDIR)
#define caddr_t char *

#define CHAR_BIT	8

#undef timezone
struct timezone {
	int tv_minuteswest;
	int tv_dsttime;
};
#define ftruncate(f,s)	chsize(f,s)
#define msleep(x)	Sleep(x);
int gettimeofday(struct timeval *tp, struct timezone *tz);
int settimeofday(const struct timeval *tp, const struct timezone *tz);
#ifndef HAVE_LOCALTIME_R
struct tm *localtime_r(const time_t *l_clock, struct tm *result);
#endif
#ifndef HAVE_INET_ATON
int inet_aton(const char *cp, struct in_addr *inp);
#endif
typedef int pid_t;
typedef char int8_t;
typedef short int16_t;
typedef long int32_t;
typedef unsigned __int64 uint64_t;
#else
typedef unsigned long long uint64_t;
#define msleep(x)	usleep(x*1000);
#include <sys/time.h>
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned char boolean;
#define u_char uint8_t
#define u_short uint16_t
#define u_long uint32_t
#define u_int uint32_t
#define mode_t int

/* Standard file descriptors.  */
#define	STDIN_FILENO	0	/* Standard input.  */
#define	STDOUT_FILENO	1	/* Standard output.  */
#define	STDERR_FILENO	2	/* Standard error output.  */

#define HAVE_REGEX_H	1

#ifdef WIN32
typedef int32_t __fsblkcnt_t;		/* Type to count file system blocks. */
typedef struct {
	int __val[2];
} __fsid_t;				/* Type of file system IDs.  */

struct statfs {
	long int f_type;
#define f_fstyp f_type
	long int f_bsize;
	long int f_frsize;	/* Fragment size - unsupported */
	__fsblkcnt_t f_blocks;
	__fsblkcnt_t f_bfree;
	__fsblkcnt_t f_files;
	__fsblkcnt_t f_ffree;
	__fsblkcnt_t f_bavail;
	
	/* Linux specials */
	__fsid_t f_fsid;
	long int f_namelen;
	long int f_spare[6];
};
int statfs(const char *path, struct statfs *buf);
#endif

#endif /* __PROTO_H_INCLUDE__ */
