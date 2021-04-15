
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/timeb.h>
#ifdef WIN32
#include <winsock.h>
#endif

#include <stdio.h>
#include <time.h>

#undef timezone
struct timezone {
	int tv_minuteswest;
	int tv_dsttime;
};

#define TICKSPERSEC                 10000000
#define TICKSPERMSEC                10000
#define SECSPERDAY                  86400
#define SECSPERHOUR                 3600
#define SECSPERMIN                  60
#define MINSPERHOUR                 60
#define HOURSPERDAY                 24
#define EPOCHWEEKDAY                1 /* Jan 1, 1601 was Monday */
#define DAYSPERWEEK                 7
#define EPOCHYEAR                   1601
#define DAYSPERNORMALYEAR           365
#define DAYSPERLEAPYEAR             366
#define MONSPERYEAR                 12
#define DAYSPERQUADRICENTENNIUM     (365 * 400 + 97)
#define DAYSPERNORMALCENTURY        (365 * 100 + 24)
#define DAYSPERNORMALQUADRENNIUM    (365 * 4 + 1)

/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)
/* 1601 to 1980 is 379 years plus 91 leap days */
#define SECS_1601_TO_1980  ((379 * 365 + 91) * (ULONGLONG)SECSPERDAY)
#define TICKS_1601_TO_1980 (SECS_1601_TO_1980 * TICKSPERSEC)

int gettimeofday(struct timeval *tp, struct timezone *tz)
{
	SYSTEMTIME utc;
	FILETIME ft;
	LARGE_INTEGER li;

	GetSystemTime(&utc);
	SystemTimeToFileTime(&utc, &ft);

	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;

	tp->tv_sec = (ULONG)((li.QuadPart - TICKS_1601_TO_1970) / TICKSPERSEC);
	tp->tv_usec = (ULONG)(li.QuadPart % 10);

	return 0;
}

int settimeofday(const struct timeval *tp, const struct timezone *tz)
{
	FILETIME ft;
	LARGE_INTEGER li;
	SYSTEMTIME utc;
	
	li.QuadPart = tp->tv_sec * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
	li.QuadPart += tp->tv_usec * 10;

	ft.dwLowDateTime = li.LowPart;
	ft.dwHighDateTime = li.HighPart;

	FileTimeToSystemTime(&ft, &utc);
	SetSystemTime(&utc);

	return 0;
}

#ifndef HAVE_INET_ATON
int inet_aton(const char *cp, struct in_addr *inp)
{
	int a1, a2, a3, a4;

	if (sscanf(cp, "%d.%d.%d.%d", &a1, &a2, &a3, &a4) != 4)
		return 0;

	inp->s_addr = htonl((a1<<24)+(a2<<16)+(a3<<8)+a4);
	return 1;
}
#endif

#ifdef WIN32
int statfs(const char *path, struct statfs *buf)
{
	memset(buf, 0, sizeof (buf));
	return 0;
}
#endif

#ifndef HAVE_LOCALTIME_R
/*
 *	We use localtime_r() by default in the server.
 *
 *	For systems which do NOT have localtime_r(), we make the
 *	assumption that localtime() is re-entrant, and returns a
 *	per-thread data structure.
 *
 *	Even if localtime is NOT re-entrant, this function will
 *	lower the possibility of race conditions.
 */
struct tm *localtime_r(const time_t *l_clock, struct tm *result)
{
	memcpy(result, localtime(l_clock), sizeof(*result));
	return result;
}
#endif
