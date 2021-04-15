#include <windows.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <ntserv.h>

static char *ntserv_regkey = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";

void *ntserv_data = NULL;
ntserv_cb ntserv_call = NULL;
SERVICE_STATUS sstat;

void ntserv_message_cb(ntserv_cb func, void *data)
{
	ntserv_call = func;
	ntserv_data = data;
}

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

/* Update logging
 * Accepts: *pointer to logging handle
 *          logging level
 *          message ID
 *          varible format
 *          varible list
 */
void ntserv_messagev(int level, const char *oformat, va_list pvar)
{
#define FMT_BUFLEN 2*1024 + 2*10
	char fmt_cpy[FMT_BUFLEN], format[FMT_BUFLEN];
	int serrno = errno;
	
	*fmt_cpy = '\0';

	/* Print the pid & maybe the thread id in format here.  Skip forward if */
	/* you don't want it later (e.g. if syslogging).                        */
	sprintf(format, "%05d:", getpid());
	
	strcat(format, " ");
	replace_percentm(oformat, format + strlen(format), sizeof(format) - strlen(format));
#ifdef WIN32
	if (_vsnprintf(fmt_cpy, FMT_BUFLEN-1, format, pvar) < 0) {
#else
	if (vsnprintf(fmt_cpy, FMT_BUFLEN-1, format, pvar) < 0) {
#endif
		fmt_cpy[FMT_BUFLEN-1] = '\0';
	}
	if (ntserv_call)
		ntserv_call(ntserv_data, level, fmt_cpy);
	errno = serrno; /* restore errno, just in case...? */
	return;
}

void ntserv_message(int level, char *format, ...)
{
	va_list pvar;
	va_start(pvar, format);
	ntserv_messagev(level, format, pvar);
	va_end(pvar);
}

/* Deregister event message environment
 */
void ntserv_deregister(char *service)
{
	TCHAR key_name[256];
	HKEY key = 0;
	LONG ret = ERROR_SUCCESS;
	strcpy(key_name, ntserv_regkey);
	strcat(key_name, service);
	ret = RegDeleteKey(HKEY_LOCAL_MACHINE, key_name);
	/* now we have to delete the application from the "Sources" value too. */
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  /* handle of open key */
		ntserv_regkey,       /* address of name of subkey to open */
		0,                   /* reserved */
		KEY_ALL_ACCESS,      /* security access mask */
		&key                 /* address of handle of open key */
		);
	if (ret == ERROR_SUCCESS) {
		DWORD size;
		/* retrieve the size of the needed value */
		ret = RegQueryValueEx(key,              /* handle of key to query */
			TEXT("Sources"),  /* address of name of value to query */
			0,                /* reserved */
			0,                /* address of buffer for value type */
			0,                /* address of data buffer */
			&size             /* address of data buffer size */
			);
		if (ret == ERROR_SUCCESS) {
			DWORD type;
			LPBYTE buffer = (LPBYTE)(GlobalAlloc(GPTR, size));
			LPBYTE buf_new = (LPBYTE)(GlobalAlloc(GPTR, size));
			register LPTSTR p;
			register LPTSTR pnew;
			BOOL need_save;
			ret = RegQueryValueEx(key,	/* handle of key to query */
					      TEXT("Sources"), /* address of name of value to query */
					      0,	/* reserved */
					      &type,	/* address of buffer for value type */
					      buffer,	/* address of data buffer */
					      &size);	/* address of data buffer size */
			if (ret == ERROR_SUCCESS) {
				if (type != REG_MULTI_SZ)
					ntserv_message(NTSERV_FAILURE, "ntserv_register: not REG_MULTI_SZ - %m");
				/* check whether this service is already a known source */
				p = (LPTSTR)(buffer);
				pnew = (LPTSTR)(buf_new);
				need_save = 0;  /* assume the value is already correct */
				for (; *p; p += strlen(p)+1, pnew += strlen(pnew)+1) {
					/* except ourself: copy the source string into the destination */
					if (strcmp(p, service) != 0)
						strcpy(pnew, p);
					else {
						need_save = TRUE;       /* *this* application found */
						size -= strlen(p)+1;	/* new size of value */
					}
				}
				if (need_save) {
					/* OK - now store the modified value back into the
					 * registry.
					 */
					ret = RegSetValueEx(key,	/* handle of key to set value for */
							    TEXT("Sources"),/* address of value to set */
							    0,		/* reserved */
							    type,	/* flag for value type */
							    buf_new,	/* address of value data */
							    size);	/* size of value data */
				}
			}
			GlobalFree((HGLOBAL)(buffer));
			GlobalFree((HGLOBAL)(buf_new));
		}
		RegCloseKey(key);
	}
}

/* Register event message environment
 * Accepts: event message file
 *          value type
 */
void ntserv_register(char *file, DWORD types, char *service)
{
	char key_name[256];
	HKEY key = 0;
	LONG ret = ERROR_SUCCESS;
	strcpy(key_name, ntserv_regkey);
	strcat(key_name, service);
	/* Create a key for that application and insert values for
	 * "EventMessageFile" and "TypesSupported"
	 */
	if (RegCreateKey(HKEY_LOCAL_MACHINE, key_name, &key) == ERROR_SUCCESS) {
		ret = RegSetValueEx(key,			/* handle of key to set value for */
				    TEXT("EventMessageFile"),	/* address of value to set */
				    0,				/* reserved */
				    REG_EXPAND_SZ,		/* flag for value type */
				    (CONST BYTE*)file,		/* address of value data */
				    strlen(file) + 1);		/* size of value data */
		/* Set the supported types flags. */
		ret = RegSetValueEx(key,			/* handle of key to set value for */
				    TEXT("TypesSupported"),	/* address of value to set */
				    0,				/* reserved */
				    REG_DWORD,			/* flag for value type */
				    (CONST BYTE*)&types,	/* address of value data */
				    sizeof (DWORD));		/* size of value data */
				    
		RegCloseKey(key);
	}
	/* Add the service to the "Sources" value */
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,  /* handle of open key */
		ntserv_regkey,       /* address of name of subkey to open */
		0,                   /* reserved */
		KEY_ALL_ACCESS,      /* security access mask */
		&key                 /* address of handle of open key */
		);
	if (ret == ERROR_SUCCESS) {
		DWORD size;
		/* retrieve the size of the needed value */
		ret = RegQueryValueEx(key,	/* handle of key to query */
				      TEXT("Sources"), /* address of name of value to query  */
				      0,	/* reserved */
				      0,	/* address of buffer for value type */
				      0,	/* address of data buffer */
				      &size);	/* address of data buffer size */
			
		if (ret == ERROR_SUCCESS) {
			DWORD type;
			LPBYTE buffer;
			register LPTSTR p;
			DWORD size_new = size + strlen(service) + 1;
			buffer = (LPBYTE)(GlobalAlloc(GPTR, size_new));
			ret = RegQueryValueEx(key,	/* handle of key to query */
					      TEXT("Sources"),	/* address of name of value to query */
					      0,		/* reserved */
					      &type,		/* address of buffer for value type */
					      buffer,		/* address of data buffer */
					      &size);		/* address of data buffer size */
			if (ret == ERROR_SUCCESS) {
				if (type != REG_MULTI_SZ)
					ntserv_message(NTSERV_FAILURE, "ntserv_register: not REG_MULTI_SZ - %m");
				/* check whether this service is already a known source */
				p = (LPTSTR)(buffer);
				for (; *p; p += strlen(p)+1) {
					if (strcmp(p, service) == 0)
						break;
				}
				if (!(*p)) {
					/* We're standing at the end of the stringarray
					 * and the service does still not exist in the "Sources".
					 * Now insert it at this point.
					 * Note that we have already enough memory allocated
					 * (see GlobalAlloc() above). We also don't need to append
					 * an additional '\0'. This is done in GlobalAlloc() above
					 * too.
					 */
					strcpy(p, service);
					/* OK - now store the modified value back into the
					 * registry.
					 */
					ret = RegSetValueEx(key,	/* handle of key to set value for */
							    TEXT("Sources"),/* address of value to set */
							    0,		/* reserved */
							    type,	/* flag for value type */
							    buffer,	/* address of value data */
							    size_new);	/* size of value data */
						
				}
			}
			GlobalFree((HGLOBAL)(buffer));
		}
		RegCloseKey(key);
	}
}

/* Install winNT service
 * Returns: 1 when installation succeeded while 0 failed
 */
int ntserv_install(char *path, char *service, char *display,
		   char *dependencies, char *description)
{
	SC_HANDLE   sch_service;
	SC_HANDLE   sch_manager;
	BOOL ret = 0;
	SERVICE_DESCRIPTION desc;

	sch_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (sch_manager) {
		sch_service = CreateService(sch_manager,	/* SCManager database */
					   service,		/* name of service */
					   display,		/* name to display */
					   SERVICE_ALL_ACCESS,	/* desired access */
					   SERVICE_WIN32_OWN_PROCESS,	/* service type */
					   SERVICE_AUTO_START,	/* start type */
					   SERVICE_ERROR_NORMAL,/* error control type */
					   path,		/* service's binary */
					   0,			/* no load ordering group */
					   0,			/* no tag identifier */
					   dependencies,	/* dependencies */
					   0,			/* LocalSystem account */
					   0);			/* no password */
		if (sch_service) {
			desc.lpDescription = description;
			ChangeServiceConfig2(sch_service, SERVICE_CONFIG_DESCRIPTION, &desc);
			CloseServiceHandle(sch_service);
			ret = TRUE;
		} else {
			ntserv_message(NTSERV_FAILURE, "ntserv_install: CreateService - %m");
		}
		CloseServiceHandle(sch_manager);
	} else {
		ntserv_message(NTSERV_FAILURE, "ntserv_install: OpenSCManager - %m");
	}
	if (ret) {
		/* installation succeeded. Now register the message file */
		ntserv_register(path,   /* the path to the application itself */
				EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE,
				service);
		/* supported types */
		ntserv_message(NTSERV_SUCCESS, "%s installed.", service);
	}
	return ret;
}

/* Remove winNT service
 * Returns: 1 when removing succeeded while 0 failed
 */
int ntserv_remove(char *service, char *display)
{
	SC_HANDLE sch_service;
	SC_HANDLE sch_manager;
	BOOL ret = 0;

	sch_manager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	if (sch_manager) {
		sch_service = OpenService(sch_manager, service, SERVICE_ALL_ACCESS);
		if (sch_service) {
			ntserv_message(NTSERV_NOTICE, "Stopping %s.", display);
			/* try to stop the service */
			if (ControlService(sch_service, SERVICE_CONTROL_STOP, &sstat)) {
				Sleep(1000);
				while (QueryServiceStatus(sch_service, &sstat)) {
					if (sstat.dwCurrentState == SERVICE_STOP_PENDING) {
						Sleep(1000);
					} else
						break;
				}
				if (sstat.dwCurrentState == SERVICE_STOPPED)
					ntserv_message(NTSERV_SUCCESS, "%s stopped.", display);
				else
					ntserv_message(NTSERV_FAILURE, "ntserv_remove: ControlService - %m");
			}
			/* now remove the service */
			if (DeleteService(sch_service)) {
				ntserv_message(NTSERV_SUCCESS, "%s removed.", display);
				ret = TRUE;
			} else {
				ntserv_message(NTSERV_FAILURE, "ntserv_remove: DeleteService - %m");
			}
			CloseServiceHandle(sch_service);
		} else {
			ntserv_message(NTSERV_FAILURE, "ntserv_remove: OpenService - %m");
		}
		CloseServiceHandle(sch_manager);
	} else {
		ntserv_message(NTSERV_FAILURE, "ntserv_remove: OpenSCManager - %m");
	}
	if (ret)
		ntserv_deregister(service);
	return TRUE;
}

/* Start up winNT service
 * Accepts: number of aguments
 *          ** pointer to arguments
 * Returns: error status
 */
int ntserv_start(int argc, char **argv, char *service, char *display)
{
	BOOL ret = 0;
	int retry = 5;

	SC_HANDLE sch_manager = OpenSCManager(0,	/* machine (0 == local) */
					      0,	/* database (0 == default) */
					      SC_MANAGER_ALL_ACCESS); /* access required */
	if (sch_manager) {
		SC_HANDLE sch_service = OpenService(sch_manager,
						    service,
						    SERVICE_ALL_ACCESS);
		if (sch_service) {
			/* try to start the service */
			ntserv_message(NTSERV_NOTICE, "Starting %s.", display);
			if (StartService(sch_service, argc, argv)) {
				Sleep(200);
				while (retry && QueryServiceStatus(sch_service, &sstat)) {
					if (sstat.dwCurrentState == SERVICE_START_PENDING) {
						Sleep(250);
						retry--;
					} else
						break;
				}
				if (sstat.dwCurrentState == SERVICE_RUNNING) {
					ret = TRUE;
					ntserv_message(NTSERV_SUCCESS, "%s started.", display);
				} else
					ntserv_message(NTSERV_FAILURE, "%s failed to start.", display);
			} else {
				/* StartService failed */
				ntserv_message(NTSERV_FAILURE, "ntserv_start: StartService - %m");
			}
			CloseServiceHandle(sch_service);
		} else {
			ntserv_message(NTSERV_FAILURE, "ntserv_start: OpenService - %m");
		}
		CloseServiceHandle(sch_manager);
	} else {
		ntserv_message(NTSERV_FAILURE, "ntserv_start: OpenSCManager - %m");
	}
	return ret;
}

/* Stop running winNT service
 * Returns: error status
 */
int ntserv_stop(char *service, char *display)
{
	BOOL ret = 0;
	int retry = 5;

	SC_HANDLE sch_manager = OpenSCManager(0,	/* machine (0 == local) */
					      0,	/* database (0 == default) */
					      SC_MANAGER_ALL_ACCESS); /* access required */
	if (sch_manager) {
		SC_HANDLE sch_service = OpenService(sch_manager,
						    service,
						    SERVICE_ALL_ACCESS);
		if (sch_service) {
			/* try to stop the service */
			if (ControlService(sch_service, SERVICE_CONTROL_STOP, &sstat)) {
				ntserv_message(NTSERV_NOTICE, "Stopping %s.", display);
				Sleep(1000);
				while (retry && QueryServiceStatus(sch_service, &sstat)) {
					if (sstat.dwCurrentState == SERVICE_STOP_PENDING) {
						retry--;
						Sleep(250);
					} else
						break;
				}
				if (sstat.dwCurrentState == SERVICE_STOPPED) {
					ret = TRUE;
					ntserv_message(NTSERV_SUCCESS, "%s stopped.", display);
				} else
					ntserv_message(NTSERV_FAILURE, "%s failed to stop.", display);
			}
			CloseServiceHandle(sch_service);
		} else {
			ntserv_message(NTSERV_FAILURE, "ntserv_end: OpenService - %m");
		}
		CloseServiceHandle(sch_manager);
	} else {
		ntserv_message(NTSERV_FAILURE, "ntserv_end: OpenSCManager - %m");
	}
	return ret;
}

const char *ntserv_etc_path()
{
	static char path[MAX_PATH+1];
	int size;
	static int syspath_init = 0;

	if (syspath_init)
		return path;
	size = GetSystemDirectory(path, MAX_PATH);
	path[size] = '\0';
	return path;
}
