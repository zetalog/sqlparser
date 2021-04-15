/*
 * ZETALOG's Personal COPYRIGHT
 *
 * Copyright (c) 2007
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
 * @(#)sql_driver.h: SQL client driver interface
 * $Id: sql_driver.h,v 1.5 2007-05-20 16:32:11 zhenglv Exp $
 */

#ifndef __SQL_DRIVER_H_INCLUDE__
#define __SQL_DRIVER_H_INCLUDE__

#include <time.h>

#define SQL_DOWN			1 /* for re-connect */
#define SQL_TRACE_FILE			"/var/sqltrace.sql"
#define SQL_CONF_FILE			"/etc/sqlcli.conf"

typedef char** sql_row_t;

typedef struct _sql_socket_t {
	int id;
	struct _sql_socket_t *next;
	enum { sockconnected, sockunconnected } state;
	void *conn;
	sql_row_t row;
} sql_socket_t;

#define SQL_MAX_SOCKS	256

typedef struct _sql_config_t {
	char *sql_driver;
	char *sql_server;
	char *sql_port;
	char *sql_login;
	char *sql_password;
	char *sql_db;
	int sql_trace;
	char *sql_tracefile;
	int sql_num_socks;
	int sql_retry_delay;
	/* individual driver config */
	void *driver_config;
} sql_config_t;

typedef struct _sql_driver_t {
	const char *name;
	int (*sql_init_socket)(sql_socket_t *sqlsocket, sql_config_t *config);
	int (*sql_destroy_socket)(sql_socket_t *sqlsocket, sql_config_t *config);
	int (*sql_query)(sql_socket_t *sqlsocket, sql_config_t *config, char *query);
	int (*sql_select_query)(sql_socket_t *sqlsocket, sql_config_t *config, char *query);
	int (*sql_store_result)(sql_socket_t *sqlsocket, sql_config_t *config);
	int (*sql_num_fields)(sql_socket_t *sqlsocket, sql_config_t *config);
	int (*sql_num_rows)(sql_socket_t *sqlsocket, sql_config_t *config);
	int (*sql_fetch_row)(sql_socket_t *sqlsocket, sql_config_t *config);
	int (*sql_free_result)(sql_socket_t *sqlsocket, sql_config_t *config);
	const char *(*sql_error)(sql_socket_t *sqlsocket, sql_config_t *config);
	int (*sql_close)(sql_socket_t *sqlsocket, sql_config_t *config);
	int (*sql_finish_query)(sql_socket_t *sqlsocket, sql_config_t *config);
	int (*sql_finish_select_query)(sql_socket_t *sqlsocket, sql_config_t *config);
	int (*sql_affected_rows)(sql_socket_t *sqlsocket, sql_config_t *config);
} sql_driver_t;

typedef struct _sql_inst_t {
	sql_section_t *main_config;
	time_t connect_after;
	sql_socket_t *sqlpool;
	sql_socket_t *last_used;
	sql_config_t *config;
	sql_driver_t *driver;
} sql_inst_t;

void sql_query_log(sql_inst_t *inst, char *querystr);

sql_inst_t *sql_client_init(const char *file, const char *section);
void sql_client_free(sql_inst_t *inst);

int sql_socket_close(sql_inst_t *inst, sql_socket_t * sqlsocket);
sql_socket_t * sql_socket_get(sql_inst_t * inst);
int sql_socket_put(sql_inst_t * inst, sql_socket_t * sqlsocket);

int sql_client_query(sql_inst_t *inst, sql_socket_t *sqlsocket,
		     char *query);
int sql_client_select(sql_inst_t *inst, sql_socket_t *sqlsocket,
		      char *query);
int sql_client_fetch(sql_inst_t *inst, sql_socket_t *sqlsocket);

const char *sql_dattetime_string(int utc, time_t time);

#endif /* __SQL_DRIVER_H_INCLUDE__ */
