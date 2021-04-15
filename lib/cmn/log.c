/*
 * ZETALOG's Personal COPYRIGHT
 *
 * Copyright (c) 2003
 *    ZETALOG - "Lv ZHENG".  All rights reserved.
 *    Author: Lv "Zetalog" Zheng
 *    Internet: zetalog@hzcnc.com
 *
 * This COPYRIGHT used to protect Personal Intelligence Rights.
 * Redistribution and use in source and binary forms with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the Lv "Zetalog" ZHENG.
 * 3. Neither the name of this software nor the names of its developers may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 4. Permission of redistribution and/or reuse of souce code partially only
 *    granted to the developer(s) in the companies ZETALOG worked.
 * 5. Any modification of this software should be published to ZETALOG unless
 *    the above copyright notice is no longer declaimed.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ZETALOG AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE ZETALOG OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)log.c: logging routines
 * $Id: log.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef WIN32
#include <process.h>
#include <direct.h>
#include <windows.h>
/* for message compiler */
#include "event.h"
#endif

#include <syslog.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if !defined(HAVE_PID_T)
typedef int pid_t;
#endif

#if defined(HAVE_WINSOCK_H)
#include "sockerr.h"
#endif
#include <log.h>        /* Prototypes & defines. */

#define MAXNAMELEN  256

void    *log_default_handle = NULL;

#ifdef WIN32
static HINSTANCE log_event_library = NULL;
#endif

#ifdef WIN32
char    *log_user_sid = NULL;
#endif

#define VA_START(a, b) va_start((a), (b))
#define va_alist ...
#define va_dcl

#ifndef MIN
#define MIN(a, b) (((a) < (b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b))?(a):(b))
#endif

typedef struct _log_t {
    char domain[MAXNAMELEN];
#ifdef HAVE_DGETTEXT
#define NL_START(x, y) \
    strncpy((x)->domain, name, MAX(MAXNAMELEN, strlen(name))); \
    (x)->domain[MAX(MAXNAMELEN, strlen(name))] = '\0'
#define NL_UPDATE(x, e, f) \
    (f) = dgettext((x)->domain, (f))
#define NL_CLOSE(x)
#elif defined(WIN32)
#define NL_START(x, y) \
    strncpy((x)->domain, \
            last_component(__argv[0]), \
            MIN(MAXNAMELEN-1, strlen(last_component(__argv[0])))); \
    (x)->domain[MIN(MAXNAMELEN, strlen(last_component(__argv[0])))] = '\0'
#define NL_UPDATE(x, e, f) 
#define NL_CLOSE(x)
#else
#define NL_START(x, y) 
#define NL_UPDATE(x, e, f) 
#define NL_CLOSE(x)
#endif
    int level;
    int how;
} log_t;

static void replace_percentm(const char *inbuffer, char *outbuffer, int olen)
{
	register const char *t2;
	register char *t1, ch;
#ifdef WIN32
	LPVOID msgbuf;
#endif
	
	if (!outbuffer || !inbuffer)
		return;
#ifdef WIN32
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		      NULL, GetLastError(),
		      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		      (LPTSTR) &msgbuf, 0, NULL);
#endif
	
	olen--;
	for (t1 = outbuffer; (ch = *inbuffer) && t1-outbuffer < olen; ++inbuffer) {
		if (inbuffer[0] == '%' && inbuffer[1] == 'm') {
			for (++inbuffer, t2 = 
#ifdef WIN32
				msgbuf;
#else
				strerror(errno);
#endif
				(t2 && t1-outbuffer < olen) && (*t1 = *t2++);
				t1++);
		} else *t1++ = ch;
	}
	*t1 = '\0';
#ifdef WIN32
	LocalFree(msgbuf);
#endif
}

/* Return a pointer into PATH's last component, discard ext if Win32. */
static char *last_component(char *path)
{
#ifdef WIN32
#define PATH_DELIM      '\\'
#else
#define PATH_DELIM      '/'
#endif
	static char *cache = NULL;
	char *last;
	char *temp;
	if (cache)
		free(cache);    /* free last allocated */
	cache = strdup(path);
	last = strrchr(cache, PATH_DELIM);
#ifdef WIN32
	temp = strrchr(cache, '.');
	if (temp) *temp = '\0';
#endif
	if (last && (last != cache))
		return last + 1;
	else
		return cache;
}

/* Update logging
 * Accepts: *pointer to logging handle
 *          logging level
 *          message ID
 *          varible format
 *          varible list
 */
void logv_update(const void *handle, int level, int msgID, const char *oformat, va_list pvar)
{
#define FMT_BUFLEN 2*1024 + 2*10
	char fmt_cpy[FMT_BUFLEN], format[FMT_BUFLEN];
	log_t *h = (log_t *)handle;
	int serrno = errno;
	int slfac, offset = 6;
	
	*fmt_cpy = '\0';
	/* If log_t has not been initialized, do it before we start;
	 * Saves the mess of making sure it gets called ahead of time.
	 */
	if (h == NULL) {
		log_start(&log_default_handle, -1, -1,
#ifdef WIN32
			  last_component(__argv[0]));
#else
			  "Unknown");
#endif
		h = (log_t *)log_default_handle;
	}
	/* If the handle is invalid, don't log.
	 * If the maximum log level is too low for this message, don't log.
	 * If something that we call forces us to log a message, don't log.
	 */
	if (!h || !(h->how) || h->level == -1 || level > h->level)
		return;
	/* Change the format if the message is in the catalog */
	NL_UPDATE(h, msgID, oformat);
	if (!oformat)
		return;
	/* Print the pid & maybe the thread id in format here.  Skip forward if */
	/* you don't want it later (e.g. if syslogging).                        */
	sprintf(format, "%05d:", (pid_t)getpid());
	
	strcat(format, " ");
	replace_percentm(oformat, format + strlen(format), sizeof(format) - strlen(format));
#ifdef WIN32
	if (_vsnprintf(fmt_cpy, FMT_BUFLEN-1, format, pvar) < 0) {
#else
	if (vsnprintf(fmt_cpy, FMT_BUFLEN-1, format, pvar) < 0) {
#endif
		fmt_cpy[FMT_BUFLEN-1] = '\0';
	}
	/* Log to the Local log facility, e.g. Stderr on Unix and maybe a window
	 * or something on NT.  Neither system can deal with a NULL format so
	 * check that here too.
	 */
	if (h->how & LOG_LOCAL && format) {
		fprintf(stderr, "%s\n", fmt_cpy);
		fflush(stderr);
	}
	/* Log to the system logging facility -- e.g. Syslog on Unix & the
	 * EventLog on Windows NT.
	 */
	if (h->how & LOG_SYSLOG) {
		if (level == LOG_ERROR_LEVEL)
			slfac = LOG_ERR;
		else if (level == LOG_INFO_LEVEL)
			slfac = LOG_NOTICE;
		else if (level > LOG_INFO_LEVEL && level < LOG_DEBUG_LEVEL(5))
			slfac = LOG_WARNING;
		else if (level > LOG_DEBUG_LEVEL(0) && level < LOG_DEBUG_LEVEL(10))
			slfac = LOG_NOTICE;
		else if (level > LOG_DEBUG_LEVEL(5) && level < LOG_DEBUG_LEVEL(15))
			slfac = LOG_INFO;
		else if (level >= LOG_DEBUG_LEVEL(15))
			slfac = LOG_DEBUG;
		/* skip "%05d:", and maybe another " " */
		offset++;
		syslog(slfac, fmt_cpy + offset);
	}
	if (h->how & LOG_EVENT) {
#if defined(WIN32)
		HANDLE handle_event;
		TCHAR msg[256];
		LPTSTR strings[2];
		HKEY hkey;
		DWORD action;
		TCHAR regkey[1024];
		char path[1024];
		BOOL res;
		
		if (level == LOG_ERROR_LEVEL)
			slfac = EVENTLOG_ERROR_TYPE;
		else if (level == LOG_INFO_LEVEL)
			slfac = EVENTLOG_INFORMATION_TYPE;
		else if (level == LOG_WARN_LEVEL)
			slfac = EVENTLOG_WARNING_TYPE;
		else if (level > LOG_DEBUG_LEVEL(0) && level < LOG_DEBUG_LEVEL(5))
			slfac = EVENTLOG_WARNING_TYPE;
		else if (level > LOG_DEBUG_LEVEL(5) && level < LOG_DEBUG_LEVEL(15))
			slfac = EVENTLOG_INFORMATION_TYPE;
		else if (level >= LOG_DEBUG_LEVEL(15))
			goto nosyslog;  /* skip debug informations */
		/* skip "%05d:", and maybe another " " */
		offset++;
		sprintf(msg, TEXT("%s error(%d): %s"), TEXT(((log_t *)handle)->domain), serrno, fmt_cpy);
		
		res = GetModuleFileName(log_event_library, path, 1024);
		sprintf(regkey, TEXT("System\\CurrentControlSet\\Services\\EventLog\\Application\\%s"), TEXT(((log_t *)handle)->domain));
		/* create or open the key */
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			regkey,
			0L, NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, NULL,
			&hkey, &action) == ERROR_SUCCESS) {
			res = RegSetValueEx(hkey, "EventMessageFile", 0L, REG_SZ, (LPBYTE)path, strlen(path)+1);
			res = RegSetValueEx(hkey, "CategoryMessageFile", 0L, REG_SZ, (LPBYTE)path, strlen(path)+1);
			RegCloseKey(hkey);
		}
		res = GetLastError();
		/* Use event logging to log the error.
		 */
		handle_event = RegisterEventSource(NULL, TEXT(((log_t *)handle)->domain));
		strings[0] = msg;
		if (handle_event != NULL) {
			ReportEvent(handle_event,   /* handle of event source */
				(WORD)slfac,    /* event type */
				0,              /* event category */
				LOG_EVENT_MSG,  /* event ID */
				log_user_sid,   /* current user's SID */
				1,              /* strings in lpszStrings */
				0,              /* no bytes of raw data */
				strings,        /* array of error strings */
				NULL);          /* no raw data */
			(void)DeregisterEventSource(handle_event);
		}
nosyslog:
		;
#endif
	}
	errno = serrno; /* restore errno, just in case...? */
	return;
}

/* Update logging
 * Accepts: *pointer to logging handle
 *          logging level
 *          message ID
 *          varible format
 *          varible list
 */
void log_update(const void *handle, int level, int msgID, const char *format, va_alist) va_dcl
{
	va_list pvar;
	va_start(pvar, format);
	logv_update(handle, level, msgID, format, pvar);
	va_end(pvar);
}

/* Start logging operations
 * Accepts: **pointer to logging handle
 *          how to log
 *          logging level
 *          string of logging prefix name
 */
void log_start(void **vhp, int how, int level, const char *name)
{
	log_t **hp = (log_t **)vhp;
	char buf[1024], *tmp;
	/*
	char tbuf[1024];
	time_t now = time(NULL);
	*/
#ifdef WIN32
	OSVERSIONINFO vi;
	int is_winnt;
	BYTE sid_buf[4096];
	DWORD sid_size = sizeof (sid_buf);
	PSID user_sid = NULL;
	TCHAR user_name[256];
	DWORD user_size  =  255;
	TCHAR domain_name[256];
	DWORD domain_size = 255;
	SID_NAME_USE sid_type;
	DWORD sid_len;
	vi.dwOSVersionInfoSize = sizeof (vi);   /* init this. */
	GetVersionEx(&vi);                      /* lint !e534 */
	is_winnt = (vi.dwPlatformId == VER_PLATFORM_WIN32_NT);
	
	if (is_winnt) {
		if (!log_event_library)
			log_event_library = LoadLibraryEx("event.dll", NULL, DONT_RESOLVE_DLL_REFERENCES);
		/* Providing a SID (security identifier) was contributed by Victor
		 * Vogelpoel (VictorV@Telic.nl).
		 * The code from Victor was slightly modified.
		 */
		/* Get security information of current user */
		memset(user_name, 0, sizeof (user_name));
		memset(domain_name, 0, sizeof (domain_name));
		memset(sid_buf, 0, sid_size);
		GetUserName(user_name, &user_size);
		if (LookupAccountName(0,
			user_name,
			&sid_buf,
			&sid_size,
			domain_name,
			&domain_size,
			&sid_type)) {
			if (IsValidSid((PSID)(sid_buf))) {
				sid_len = GetLengthSid((PSID)(sid_buf));
				log_user_sid = (PSID)(malloc(sizeof (BYTE) * sid_len));
				CopySid(sid_len, log_user_sid, sid_buf);
				EqualSid(log_user_sid, sid_buf);
			}
		}
	}
#endif
	sprintf(buf, "%s", name);
	if (!(*hp) && ((*hp) = (log_t *)malloc(sizeof (log_t))) == NULL) {
		/* no where to store things, so just skip it. */
		return;
	} else {
		if (how == -1) {
			(*hp)->how = 0;
			if (getenv("LOG_SYSLOG"))
				(*hp)->how |= LOG_SYSLOG;
			if (getenv("LOG_STDERR"))
				(*hp)->how |= LOG_LOCAL;
		} else
			(*hp)->how = how;
		if (level == -1) {
			if ((tmp = getenv("DEBUG"))) {
				if (isdigit((unsigned char)*tmp))
					(*hp)->level = LOG_DEBUG_LEVEL(atoi(tmp));
				else
					(*hp)->level = LOG_DEBUG_LEVEL(25);
			} else
				(*hp)->level = -1;
		} else
			(*hp)->level = level;
		NL_START(*hp, buf);
	}
	/* Following code is commented out so that we can remove DontLoop */
	/*
	if (restart && (*hp)->how != 0) {
		MUTEX_LOCK(lt_mutex);
		strftime(tbuf, sizeof(tbuf), "%c", localtime(&now));
		log_update((*hp), LOG_DEBUG(0), 0, "%s Logging (re)started at %s", name, tbuf);
		MUTEX_UNLOCK(lt_mutex);
	}
	*/
}

/* End logging operations
 * Accepts: **pointer to logging handle
 */
void log_end(void *vh)
{
	log_t *h = (log_t *)vh;
	
#ifdef WIN32
	if (log_event_library) {
		FreeLibrary(log_event_library);
		log_event_library = NULL;
	}
	if (!log_user_sid) {
		free(log_user_sid);
		log_user_sid = NULL;
	}
#endif
	if (!h) return;
	NL_CLOSE(h);
	free(h);
}
