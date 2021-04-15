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
 * @(#)log.h: logging interface
 * $Id: log.h,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#ifndef __LOG_H_INCLUDE__
#define __LOG_H_INCLUDE__

#ifdef __cplusplus
extern "C" {
#endif /* __cpluscplus */

/* Intro:
 *   1. This logging interface encapsulats Unix syslog and Win32 event log for
 *      LOG_SYSTEM and stderr displaying for LOG_LOCAL;
 *   2. Use %m displaying error message directly;
 *   3. If set errno using WSAGetLastError() on Win32 platform before calling
 *      log_update(), %m will display error message for winsock last error;
 *   4. Display thread id by setting log_show_thread_ids = TRUE, currently for
 *      pthread supports;
 *   5. Support user security identifier for WinNT event logging;
 *   6. Automatic executable name detection;
 *   7. Environments for default logging feature.
 * Usage:
 *   To use this routines on win32, you must have an .mc file describing the
 *   event log messages.
 */

#define LOG_BUF             256     /* log buffer length */

#include <stdarg.h>

extern void log_update(const void *handle, int level, int entry, const char *format, ...);
extern void log_start(void **handlep, int how, int level, const char * name);
extern void log_end(void *handle);
void logv_update(const void *handle, int level, int entry, const char *format, va_list);

/* how */
#define LOG_LOCAL           0x01
#define LOG_SYSTEM          0x02
#define LOG_EVENT           0x04

/* level */
#define LOG_FAIL_LEVEL		0x01
#define LOG_ERROR_LEVEL		0x02
#define LOG_INFO_LEVEL		0x03
#define LOG_WARN_LEVEL		0x04
#define LOG_DEBUG_MAX		0xff
#define LOG_DEBUG_LEVEL(x)	(0x04 + (x))

extern void *log_default_handle;
#ifdef WIN32
extern char *log_user_sid;
#endif

#ifdef __cplusplus
}
#endif /* __cpluscplus */

#endif /* __LOG_H_INCLUDE__ */
