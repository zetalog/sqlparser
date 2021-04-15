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
 * @(#)ntserv.c: windows nt service routines
 * $Id: ntserv_main.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#include <ntserv.h>
#include <io.h>
#include <fcntl.h>
#include <log.h>
#include <stdio.h>
#include <argcv.h>
#include <getopt.h>

/* externals */
HANDLE stopevent = 0;
BOOL debug = 0;
void *ntserv_log = NULL;
int winnt;

/* internals */
SERVICE_STATUS sstat;
SERVICE_STATUS_HANDLE sstathdl;
BOOL conready;
DWORD ctrlsacpted = SERVICE_ACCEPT_STOP;
BOOL instance = 0;

/* definations */
#ifndef RSP_SIMPLE_SERVICE
#define RSP_SIMPLE_SERVICE 1
#endif
#ifndef RSP_UNREGISTER_SERVICE
#define RSP_UNREGISTER_SERVICE 0
#endif

typedef DWORD (WINAPI *ntserv_regfunc)(DWORD pid, DWORD type);

static char *ntserv_95key = "Software\\Microsoft\\Windows\\CurrentVersion\\RunServices";

/* callbacks */
BOOL WINAPI control_handler(DWORD ctrl_type);
void WINAPI service_main(DWORD uargc, LPTSTR *uargv);
void WINAPI service_ctrl(DWORD ctrl_code);
LRESULT CALLBACK faceless_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

/* internals */
int ntserv_dispatcher();
void ntserv_set_console();
void ntserv_run(int argc, char **argv);
void ntserv_kill();
void ntserv_usage();

static int ntserv_install_main();
static int ntserv_remove_main();
static int ntserv_start_main(int argc, char **argv);
static int ntserv_stop_main();

static int ntserv_debug(int argc, char ** argv, int faceless);
static int ntserv_report(DWORD current, DWORD ecode, DWORD waithint);

void ntserv_log_main(void *log_handle, int level, char *message)
{
	int log_lvl;

	switch (level) {
	case NTSERV_NOTICE:
		log_lvl = LOG_INFO_LEVEL;
		break;
	case NTSERV_FAILURE:
		log_lvl = LOG_FAIL_LEVEL;
		break;
	case NTSERV_SUCCESS:
		log_lvl = LOG_INFO_LEVEL;
		break;
	default:
		log_lvl = LOG_DEBUG_LEVEL(0);
		break;
	}
	log_update(ntserv_log, debug?LOG_DEBUG_LEVEL(0):log_lvl, 0, message);
}

/* Detect OS type
 * Returns: 0 when winNT while 1 other
 */
int ntserv_os_win95()
{
	OSVERSIONINFO vi;
	static int osinit = FALSE;
	if (!osinit) {
		vi.dwOSVersionInfoSize = sizeof (vi);
		GetVersionEx(&vi);  /* lint !e534 */
		winnt = (vi.dwPlatformId == VER_PLATFORM_WIN32_NT);
		osinit = TRUE;
	}
	return (!winnt);
}

/* WinNT service main entry
 * Accepts: number of arguments
 *          ** pointer to command arguments
 *          ** pointer to environment parameters
 * Returns: error status
 */
int main(int argc, char* argv[], char* envp[])
{
	int nargc = 0, margc = argc;
	char **nargv = argcv_new(margc);
	int (*fnc)() = ntserv_dispatcher;
	int faceless = 0;

	debug = 0;
	conready = 0;
	instance = TRUE;

	argcv_set(margc, nargv, nargc, argv[0]);
	nargc++;

	/* SERVICE_STATUS members that rarely change */
	sstat.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	sstat.dwServiceSpecificExitCode = 0;

	while (argv++, --argc) {
		if (strcmp(argv[0], "install") == 0) {
			fnc = ntserv_install_main;
			break;
		}
		if (strcmp(argv[0], "uninstall") == 0) {
			fnc = ntserv_remove_main;
			break;
		}
		if (strcmp(argv[0], "start") == 0) {
			fnc = ntserv_start_main;
			break;
		}
		if (strcmp(argv[0], "stop") == 0) {
			fnc = ntserv_stop_main;
			break;
		}
		if (strcmp(argv[0], "account") == 0) {
			/* login-account (only useful with -i) */
			/*username = NEXT_ARG; */
			break;
		}
		if (strcmp(argv[0], "password") == 0) {
			/* password (only useful with -i) */
			/*passwd = NEXT_ARG; */
			break;
		}
		if (strcmp(argv[0], "debug") == 0) {
			debug = 1;
			continue;
		}
		if (strcmp(argv[0], "faceless") == 0) {
			debug = 1;
			faceless = 1;
			continue;
		}
		if (strcmp(argv[0], "help") == 0) {
			ntserv_usage();
			break;
		}
		argcv_set(margc, nargv, nargc, argv[0]);
		nargc++;
	}

	if (!ntserv_log) {
		if (debug || fnc != ntserv_dispatcher)
			log_start(&ntserv_log, LOG_LOCAL,
				  LOG_DEBUG_LEVEL(15), SERVICE_NAME);
		else
			log_start(&ntserv_log, LOG_EVENT | LOG_LOCAL,
				  LOG_WARN_LEVEL, SERVICE_NAME);
	}

	ntserv_message_cb(ntserv_log_main, ntserv_log);

	/* if Win95, run as faceless app. */
	if (fnc == ntserv_dispatcher && ntserv_os_win95()) {
		/* act as if -f was passed anyways. */
		debug = 1, faceless = 1;
	}
	if (debug)
		return ntserv_debug(nargc, nargv, faceless);
	else {
		return (*fnc)();
	}
}

/* Start service control dispatcher
 * Returns: 1 when success while 0 when failure
 */
int ntserv_dispatcher()
{
	/* default implementation creates a single threaded service.
	 * override this method and provide more table entries for
	 * a multithreaded service (one entry for each thread).
	 */
	SERVICE_TABLE_ENTRY dispatch_table[] = {
		{ (SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION)service_main },
		{ 0, 0 }
	};
	BOOL ret = StartServiceCtrlDispatcher(dispatch_table);
	if (!ret) {
		log_update(ntserv_log, debug?LOG_DEBUG_LEVEL(0):LOG_FAIL_LEVEL, 0,
			   "ntserv_dispatcher: StartServiceCtrlDispatcher - %m");
	}
	return ret;
}

/* WinNT service usage information
 */
void ntserv_usage()
{
	printf("%s install   to install the %s\n", APP_NAME, SERVICE_NAME);
	printf("%s uninstall to uninstall the %s\n", APP_NAME, SERVICE_NAME);
	printf("%s debug     to debug the %s\n", APP_NAME, SERVICE_NAME);
	printf("%s faceless  to run faceless %s on win95\n", APP_NAME, SERVICE_NAME);
	printf("%s start     to start installed %s\n", APP_NAME, SERVICE_NAME);
	printf("%s stop      to stop installed %s\n", APP_NAME, SERVICE_NAME);
	printf("%s help      to view this help infomation\n", APP_NAME, SERVICE_NAME);
	exit(0);
}

/* Install winNT service
 * Returns: 1 when installation succeeded while 0 failed
 */
int ntserv_install_main()
{
	TCHAR path[1024];
	BOOL ret = 0;

	/* have to show the console here for the
	 * diagnostic or error reason: orignal class assumed
	 * that we were using _main for entry (a console app).
	 * This particular usage is a Windows app (no console),
	 * so we need to create it. Using ntserv_setcon with _main
	 * is ok - does nothing, since you only get one console.
	 */
	ntserv_set_console();	
	if (GetModuleFileName(0, path, 1023) == 0) {
		log_update(ntserv_log, LOG_DEBUG_LEVEL(0), 0,
			   "ntserv_install: GetModuleFileName -%m");
		return 0;
	}
	if (ntserv_os_win95()) {
		/* code added to install as Win95 service
		 * Create a key for that application and insert values for
		 * "EventMessageFile" and "TypesSupported"
		 */
		HKEY key = 0;
		LONG res = ERROR_SUCCESS;
		if (RegCreateKey(HKEY_LOCAL_MACHINE, ntserv_95key , &key) == ERROR_SUCCESS) {
			res = RegSetValueEx(key,		/* handle of key to set value for */
					   SERVICE_NAME,	/* address of value to set (NAME OF SERVICE) */
					   0,			/* reserved */
					   REG_EXPAND_SZ,	/* flag for value type */
					   (CONST BYTE*)path,	/* address of value data */
					   strlen(path) + 1);	/* size of value data */
			RegCloseKey(key);
			ret = TRUE;
		}
	} else {
		ret = ntserv_install(path, TEXT(SERVICE_NAME), TEXT(DISPLAY_NAME),
				     TEXT(DEPENDENCIES), TEXT(SERVICE_DESC));
	}
	return ret;
}

/* Remove winNT service
 * Returns: 1 when removing succeeded while 0 failed
 */
int ntserv_remove_main()
{
	BOOL ret = 0;

	/* have to show the console here for the
	 * diagnostic or error reason: orignal class assumed
	 * that we were using _main for entry (a console app).
	 * This particular usage is a Windows app (no console),
	 * so we need to create it. Using ntserv_setcon with _main
	 * is ok - does nothing, since you only get one console.
	 */
	ntserv_set_console();	
	if (ntserv_os_win95()) {
		/* code added to install as Win95 service */
		HKEY key = 0;
		LONG res = ERROR_SUCCESS;
		if (RegCreateKey(HKEY_LOCAL_MACHINE, ntserv_95key , &key) == ERROR_SUCCESS) {
			res = RegDeleteValue(key, SERVICE_NAME);
			RegCloseKey(key);
			ret = TRUE;
		}
	} else {
		ret = ntserv_remove(TEXT(SERVICE_NAME), TEXT(DISPLAY_NAME));
	}
	return TRUE;
}

/* Debug winNT service
 * Accepts: number of aguments
 *          ** pointer to arguments
 *          if faceless for win95
 * Returns: error status
 */
int ntserv_debug(int argc, char **argv, int faceless)
{
	DWORD uargc;
	LPTSTR *uargv;
	ntserv_regfunc fncptr = 0;

	uargc   = (DWORD)argc;
	uargv = argv;
	if (!faceless) {    /* no faceless, so give it a face. */
		ntserv_set_console(); /* make the console for debugging */
		log_update(ntserv_log, LOG_DEBUG_LEVEL(0), 0,
			   "Debugging %s.", TEXT(DISPLAY_NAME));
		SetConsoleCtrlHandler(control_handler, TRUE);
	}
	/*if Win95, register server */
	if (faceless /*&& ntserv_os_win95()*/) {
		WNDCLASS wndclass;
		ATOM atom;
		HWND hwnd;
		HMODULE module;
		memset(&wndclass, 0, sizeof (WNDCLASS));
		wndclass.lpfnWndProc = faceless_wndproc;
		wndclass.hInstance = (HINSTANCE)(GetModuleHandle(0));
		wndclass.lpszClassName = TEXT("RRL_faceless_wndproc");
		atom = RegisterClass(&wndclass);
		hwnd = CreateWindow(wndclass.lpszClassName,
				    TEXT(""), 0, 0, 0, 0, 0, 0, 0,
				    wndclass. hInstance, 0);
		module = GetModuleHandle(TEXT("kernel32.dll"));
		/* punch F1 on "RegisterServiceProcess" for what it does and when to use it. */
		fncptr = (ntserv_regfunc)GetProcAddress(module, "RegisterServiceProcess");
		if (fncptr != 0)
			(*fncptr)(0, RSP_SIMPLE_SERVICE);
	}
	ntserv_run(uargc, uargv);
#ifdef UNICODE
	GlobalFree((HGLOBAL)uargv);
#endif
	if (fncptr != 0)     /* if it's there, remove it: our run is over */
		(*fncptr)(0, RSP_UNREGISTER_SERVICE);
	return TRUE;
}

/* Start up winNT service
 * Accepts: number of aguments
 *          ** pointer to arguments
 * Returns: error status
 */
int ntserv_start_main(int argc, char **argv)
{
	return ntserv_start(argc, argv, SERVICE_NAME, DISPLAY_NAME);
}

/* Stop running winNT service
 * Returns: error status
 */
int ntserv_stop_main()
{
	return ntserv_stop(SERVICE_NAME, DISPLAY_NAME);
}

void ntserv_exit(void)
{
	ntserv_report(SERVICE_STOPPED, GetLastError(), 0);
}

/* WinNT service running loop
 * Returns: number of arguments
 *          ** pointer to arguments
 */
void ntserv_run(int argc, char **argv)
{
	atexit(ntserv_exit);
	/* report to the SCM that we're about to start */
	ntserv_create(argc, argv);
	if (!ntserv_init())
		return;
	log_update(ntserv_log, LOG_INFO_LEVEL, 0,
		   "%s started.", DISPLAY_NAME);
	ntserv_report(SERVICE_START_PENDING, 0, 3000);
	stopevent = CreateEvent(0, TRUE, 0, 0);
	/* You might do some more initialization here.
	 * Parameter processing for instance ...
	 */
	/* report SERVICE_RUNNING immediately before you enter the main-loop
	 * DON'T FORGET THIS!
	 */
	ntserv_report(SERVICE_RUNNING, 0, 3000);
	/* enter main-loop
	 * If the ntserv_kill() method sets the event, then we will break out of
	 * this loop.
	 */
	while (WaitForSingleObject(stopevent, 10) != WAIT_OBJECT_0) {
		if (!ntserv_work())
			break;
	}
	if (stopevent)
		CloseHandle(stopevent);
	ntserv_destroy();
}

/* Stop winNT service
 */
void ntserv_kill()
{
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	ntserv_fin();
	if (!debug)
		log_update(ntserv_log, LOG_INFO_LEVEL, 0,
			   "%s stopped.", DISPLAY_NAME);
	ntserv_report(SERVICE_STOP_PENDING, 11000, 3000);
	if (stopevent)
		SetEvent(stopevent);
}

/* Create console for faceless apps if not already there
 */
void ntserv_set_console()
{
	if (!conready) {
		DWORD astds[3] = { STD_OUTPUT_HANDLE, STD_ERROR_HANDLE, STD_INPUT_HANDLE };
		FILE *atrgs[3] = { stdout, stderr, stdin };
		long hand;
		register int i;
		AllocConsole();
		/* you only get 1 console. */
		/* lovely hack to get the standard io (printf, getc, etc)
		 * to the new console. Pretty much does what the
		 * C lib does for us, but when we want it, and inside of a
		 * Window'd app.
		 * The ugly look of this is due to the error checking (bad
		 * return values. Remove the if xxx checks if you like it
		 * that way.
		 */
		for (i = 0; i < 3; i++) {
			hand = (long)GetStdHandle(astds[i]);
			if (hand != (long)INVALID_HANDLE_VALUE) {
				int osf = _open_osfhandle(hand, _O_TEXT);
				if (osf != -1) {
					FILE *fp = _fdopen(osf, (astds[i] == STD_INPUT_HANDLE) ? "r" : "w");
					if (fp!=0) {
						*(atrgs[i]) = *fp;
						setvbuf(fp, 0, _IONBF, 0);
					}
				}
			}
		}
		conready = TRUE;
	}
}

/* Report winNT service status to service manager
 * Accepts: current status
 *          exit code
 *          wait hint
 * Returns: result of setting service status
 */
int ntserv_report(DWORD current, DWORD ecode, DWORD waithint)
{
	static DWORD chk_point = 1;
	BOOL res = TRUE;
	if (!debug) {/* when debugging we don't report to the SCM */
		if (current == SERVICE_START_PENDING)
			sstat.dwControlsAccepted = 0;
		else
			sstat.dwControlsAccepted = /*SERVICE_ACCEPT_STOP*/ctrlsacpted;
		sstat.dwCurrentState = current;
		sstat.dwWin32ExitCode = NO_ERROR;
		sstat.dwWaitHint = waithint;
		/* added code to support error exiting */
		sstat.dwServiceSpecificExitCode = ecode;
		if (ecode != 0)
			sstat.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
		if ((current == SERVICE_RUNNING) ||
			(current == SERVICE_STOPPED))
			sstat.dwCheckPoint = 0;
		else
			sstat.dwCheckPoint = ++chk_point;
			/* Report the status of the service to the service control manager.
		*/
		if (!(res = SetServiceStatus(sstathdl, &sstat))) {
			log_update(ntserv_log, debug?LOG_DEBUG_LEVEL(0):LOG_ERROR_LEVEL, 0,
				   "SetServiceStatus Error - %m");
		}
	}
	return res;
}

/* winNT servie main
 * Accepts: number of arguments
 *          arguments
 */
void WINAPI service_main(DWORD uargc, LPTSTR *uargv)
{
	/* register our service control handler:
	 */
	sstathdl = RegisterServiceCtrlHandler(SERVICE_NAME,
					      service_ctrl);
	if (sstathdl) {
		if (ntserv_report(SERVICE_START_PENDING,/* service state */
				  NO_ERROR, 3000))	/* wait hint */
			ntserv_run(uargc, uargv);
	}
	if (sstathdl)
		(void)ntserv_report(SERVICE_STOPPED, GetLastError(), 0);
}

/* Control winNT service
 * Accepts: control code
 */
void WINAPI service_ctrl(DWORD ctrl_code)
{
	/* Handle the requested control code.
	 */
	switch (ctrl_code) {
	case SERVICE_CONTROL_STOP:
		/* Stop the service.
		 *
		 * SERVICE_STOP_PENDING should be reported before
		 * setting the Stop Event - hServerStopEvent - in
		 * ntserv_kill().  This avoids a race condition
		 * which may result in a 1053 - The Service did not respond...
		 * error.
		 */
		sstat.dwCurrentState = SERVICE_STOP_PENDING;
		ntserv_kill();
		break;
	case SERVICE_CONTROL_PAUSE:
		sstat.dwCurrentState = SERVICE_PAUSE_PENDING;
		ntserv_pause();
		break;
	case SERVICE_CONTROL_CONTINUE:
		sstat.dwCurrentState = SERVICE_CONTINUE_PENDING;
		ntserv_continue();
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		ntserv_shutdown();
		break;
	case SERVICE_CONTROL_INTERROGATE:
		/* Update the service status. */
		ntserv_report(sstat.dwCurrentState, NO_ERROR, 3000);
		break;
	default:
		/* invalid control code */
		break;
	}
}

/* Control console
 * Accepts: control type
 * Returns: whether success
 */
BOOL WINAPI control_handler(DWORD ctrl_type)
{
	switch (ctrl_type) {
	case CTRL_BREAK_EVENT:  /* use Ctrl+C or Ctrl+Break to simulate */
	case CTRL_C_EVENT:      /* SERVICE_CONTROL_STOP in debug mode */
		log_update(ntserv_log, LOG_DEBUG_LEVEL(0), 0,
			   "Stopping %s.", TEXT(DISPLAY_NAME));
		ntserv_kill();
		return TRUE;
		break;
	}
	return 0;
}

/* Faceless window procedure for usage within Win95 (mostly), but can be invoked under NT by using -f
 * Accepts: window handle
 *          windows message
 *          message's wparam
 *          message's lparam
 * Returns: return result
 */
LRESULT CALLBACK faceless_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_QUERYENDSESSION || msg == WM_ENDSESSION || msg == WM_QUIT) {
		if ((!lparam) || msg == WM_QUIT) {
			DestroyWindow(hwnd);    /* kill me */
			if (instance)
				ntserv_kill();      /* stop me. */
			return TRUE;
		}
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
