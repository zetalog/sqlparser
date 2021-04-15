/*
 * ZETALOG's Personal COPYRIGHT
 *
 * Copyright (c) 2001
 *    ZETALOG - "Lv ZHENG".  All rights reserved.
 *    Author: Lv "Zetalog" Zheng
 *    Internet: zetalog@gmail.com
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
 * @(#)ntserv.h: windows nt service interfaces
 * $Id: ntserv.h,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#ifndef __NT_SERVICE_H_INCLUDE__
#define __NT_SERVICE_H_INCLUDE__

#include <windows.h>

#define NTSERV_UNKNOWN		0
#define NTSERV_UNINSTALLED	1
#define NTSERV_STOPPED		2
#define NTSERV_STARTING		3
#define NTSERV_RUNNING		4
#define NTSERV_STOPPING		5
#define NTSERV_MAXSTATUS	5

/* reporters */
#define NTSERV_NOTICE		0
#define NTSERV_FAILURE		1
#define NTSERV_SUCCESS		2
typedef void (*ntserv_cb)(void *handle, int level, char *message);
void ntserv_message_cb(ntserv_cb func, void *data);
void ntserv_messagev(int level, const char *oformat, va_list pvar);
void ntserv_message(int level, char *format, ...);

/* functions */
void ntserv_deregister(char *service);
void ntserv_register(char *file, DWORD types, char *service);
int ntserv_install(char *path, char *service, char *display,
		   char *dependencies, char *description);
int ntserv_remove(char *service, char *display);
int ntserv_start(int argc, char **argv, char *service, char *display);
int ntserv_stop(char *service, char *display);
const char *ntserv_etc_path();

/* overloads */
void ntserv_create(int argc, char **argv);
int ntserv_init();
int ntserv_work();
void ntserv_fin();
void ntserv_destroy();
void ntserv_pause();
void ntserv_continue();
void ntserv_shutdown();
extern char *SERVICE_NAME;
extern char *APP_NAME;
extern char *DISPLAY_NAME;
extern char *DEPENDENCIES;
extern char *SERVICE_DESC;

#endif /* __NT_SERVICE_H_INCLUDE__ */
