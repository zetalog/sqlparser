#include "sql_priv.h"

extern sql_driver_t sql_mysql_driver;
extern sql_driver_t sql_tds_driver;

static int sql_pool_init(sql_inst_t * inst);
static void sql_pool_free(sql_inst_t * inst);

static sql_parser_t sql_driver_config[] = {
	{"driver",      SQL_CTYPE_STRING_PTR, offsetof(sql_config_t, sql_driver),      NULL, "mysql"},
	{"server",      SQL_CTYPE_STRING_PTR, offsetof(sql_config_t, sql_server),      NULL, "localhost"},
	{"port",        SQL_CTYPE_STRING_PTR, offsetof(sql_config_t, sql_port),        NULL, "3306"},
	{"login",       SQL_CTYPE_STRING_PTR, offsetof(sql_config_t, sql_login),       NULL, ""},
	{"password",    SQL_CTYPE_STRING_PTR, offsetof(sql_config_t, sql_password),    NULL, ""},
	{"database",    SQL_CTYPE_STRING_PTR, offsetof(sql_config_t, sql_db),          NULL, "epslic"},
	{"sqltrace",    SQL_CTYPE_BOOLEAN,    offsetof(sql_config_t, sql_trace),       NULL, "no"},
	{"tracefile",   SQL_CTYPE_STRING_PTR, offsetof(sql_config_t, sql_tracefile),   NULL, SQL_TRACE_FILE},
	{"num_socks",   SQL_CTYPE_INTEGER,    offsetof(sql_config_t, sql_num_socks),   NULL, "1"},
	{"retry_delay", SQL_CTYPE_INTEGER,    offsetof(sql_config_t, sql_retry_delay), NULL, "60"},
	{ NULL,         -1,                   0,                                       NULL, NULL },
};

sql_driver_t *sql_driver_init(const char *driver)
{
	if (!driver) return NULL;
#ifdef CONFIG_SQL_MYSQL
	if (strcmp(driver, "mysql") == 0)
		return &sql_mysql_driver;
#endif
#ifdef CONFIG_SQL_TDS
	if (strcmp(driver, "tds") == 0)
		return &sql_tds_driver;
#endif
	return NULL;
}

int sql_socket_close(sql_inst_t *inst, sql_socket_t * sqlsocket)
{
	sql_log(SQL_LOG_DEBUG(2),
		"sql_socket_close: Closing sqlsocket %d",
		sqlsocket->id);
	if (sqlsocket->state == sockconnected) {
		(inst->driver->sql_close)(sqlsocket, inst->config);
	}
	if (inst->driver->sql_destroy_socket) {
		(inst->driver->sql_destroy_socket)(sqlsocket, inst->config);
	}
	free(sqlsocket);
	return 1;
}

static int connect_single_socket(sql_socket_t *sqlsocket, sql_inst_t *inst)
{
	int rcode;

	sql_log(SQL_LOG_DEBUG(2), "connect_single_socket: Attempting to connect %s #%d",
		inst->driver->name, sqlsocket->id);

	rcode = (inst->driver->sql_init_socket)(sqlsocket, inst->config);
	if (rcode == 0) {
		sql_log(SQL_LOG_DEBUG(2), "connect_single_socket: Connected new DB handle, #%d",
			sqlsocket->id);
		sqlsocket->state = sockconnected;
		return(0);
	}

	/* error, or SQL_DOWN */
	sql_log(SQL_LOG_ERR,
		"connect_single_socket: Failed to connect DB handle #%d",
		sqlsocket->id);
	inst->connect_after = time(NULL) + inst->config->sql_retry_delay;
	sqlsocket->state = sockunconnected;
	return (-1);
}

int sql_pool_init(sql_inst_t * inst)
{
	int i;
	int success = 0;
	sql_socket_t *sqlsocket;

	inst->connect_after = 0;
	inst->sqlpool = NULL;

	for (i = 0; i < inst->config->sql_num_socks; i++) {
		sql_log(SQL_LOG_DEBUG(2), "sql_pool_init: starting %d", i);

		sqlsocket = malloc(sizeof(*sqlsocket));
		if (sqlsocket == NULL) {
			return -1;
		}
		memset(sqlsocket, 0, sizeof(*sqlsocket));
		sqlsocket->conn = NULL;
		sqlsocket->id = i;
		sqlsocket->state = sockunconnected;

		if (time(NULL) > inst->connect_after) {
			/*
			 * this sets the sqlsocket->state, and
			 * possibly also inst->connect_after
			 */
			if (connect_single_socket(sqlsocket, inst) == 0) {
				success = 1;
			}
		}

		/* add this socket to the list of sockets */
		sqlsocket->next = inst->sqlpool;
		inst->sqlpool = sqlsocket;
	}
	inst->last_used = NULL;

	if (!success) {
		sql_log(SQL_LOG_DEBUG(2),
			"sql_pool_init: Failed to connect to any SQL server.");
	}

	return 1;
}

void sql_pool_free(sql_inst_t * inst)
{
	sql_socket_t *cur;
	sql_socket_t *next;

	for (cur = inst->sqlpool; cur; cur = next) {
		next = cur->next;
		sql_socket_close(inst, cur);
	}
	inst->sqlpool = NULL;
}

void sql_client_free(sql_inst_t *inst)
{
	if (inst) {
		sql_pool_free(inst);
		if (inst->config) {
			free(inst->config);
		}
		if (inst->main_config) {
			sql_conf_free(&inst->main_config);
		}
		free(inst);
	}
}

sql_inst_t *sql_client_init(const char *file, const char *section)
{
	sql_inst_t *inst;
	sql_section_t *conf;

	inst = malloc(sizeof(sql_inst_t));
	memset(inst, 0, sizeof(sql_inst_t));

	inst->config = malloc(sizeof(sql_config_t));
	memset(inst->config, 0, sizeof(sql_config_t));

	if (!file) file = SQL_CONF_FILE;
	inst->main_config = sql_conf_init(file);
	if (!inst->main_config) {
		sql_log(SQL_LOG_ERR,
			"sql_client_init: config file can not be parsed, %s",
			file);
	}

	conf = sql_section_find(section);
	if (!conf) {
		sql_log(SQL_LOG_ERR,
			"sql_client_init: config section can not be found, %s",
			section);
	}

	if (sql_section_parse(conf, inst->config, sql_driver_config) < 0) {
		sql_client_free(inst);
		return NULL;
	}

	if (inst->config->sql_num_socks > SQL_MAX_SOCKS) {
		sql_log(SQL_LOG_ERR,
			"sql_client_init: number of sqlsockets cannot exceed MAX_SQL_SOCKS, %d",
			SQL_MAX_SOCKS);
		sql_client_free(inst);
		return NULL;
	}

	inst->driver = sql_driver_init(inst->config->sql_driver);
	if (!inst->driver) {
		sql_log(SQL_LOG_ERR, "sql_client_init: Could not driver %s",
			inst->config->sql_driver);
		sql_client_free(inst);
		return NULL;
	}
	inst->config->sql_driver;

	sql_log(SQL_LOG_DEBUG(1), "sql_client_init: Driver %s (module %s) loaded.",
		inst->config->sql_driver, inst->driver->name);
	sql_log(SQL_LOG_DEBUG(1), "sql_client_init: Connecting to %s@%s:%s/%s",
		inst->config->sql_login, inst->config->sql_server,
		inst->config->sql_port, inst->config->sql_db);

	if (sql_pool_init(inst) < 0) {
		sql_client_free(inst);
		return NULL;
	}
	return inst;
}

sql_socket_t * sql_socket_get(sql_inst_t * inst)
{
	sql_socket_t *cur, *start;
	int tried_to_connect = 0;
	int unconnected = 0;

	/* start at the last place we left off */
	start = inst->last_used;
	if (!start) start = inst->sqlpool;

	cur = start;

	while (cur) {
		/*
		 * if we happen upon an unconnected socket,
		 * and this instance's grace period on
		 * (re)connecting has expired, then try to
		 * connect it.  This should be really rare
		 */
		if ((cur->state == sockunconnected) && (time(NULL) > inst->connect_after)) {
			sql_log(SQL_LOG_INFO,
				"sql_socket_get: Trying to (re)connect unconnected handle %d..",
				cur->id);
			tried_to_connect++;
			connect_single_socket(cur, inst);
		}

		/* if we still aren't connected, ignore this handle */
		if (cur->state == sockunconnected) {
			sql_log(SQL_LOG_DEBUG(2),
				"sql_socket_get: Ignoring unconnected handle %d..",
				cur->id);
		        unconnected++;
			goto next;
		}

		/* should be connected, grab it */
		sql_log(SQL_LOG_DEBUG(2),
			"sql_socket_get: Reserving sql socket id: %d",
			cur->id);

		if (unconnected != 0 || tried_to_connect != 0) {
			sql_log(SQL_LOG_INFO,
				"sql_socket_get: got socket %d after skipping %d unconnected handles, "
				"tried to reconnect %d though",
				cur->id, unconnected, tried_to_connect);
		}

		/*
		 * The socket is returned in the locked
		 * state.
		 *
		 * We also remember where we left off,
		 * so that the next search can start from
		 * here.
		 *
		 * Note that multiple threads MAY over-write
		 * the 'inst->last_used' variable.  This is OK,
		 * as it's a pointer only used for reading.
		 */
		inst->last_used = cur->next;
		return cur;

		/* move along the list */
next:
		cur = cur->next;

		/*
		 * Because we didnt start at the start, once we
		 * hit the end of the linklist, we should go
		 * back to the beginning and work toward the
		 * middle!
		 */
		if (!cur)
			cur = inst->sqlpool;

		/* if we're at the socket we started */
		if (cur == start)
			break;
	}

	/* We get here if every DB handle is unconnected and unconnectABLE */
	sql_log(SQL_LOG_INFO,
		"sql_socket_get: There are no DB handles to use! skipped %d, tried to connect %d",
		unconnected, tried_to_connect);
	return NULL;
}

int sql_socket_put(sql_inst_t * inst, sql_socket_t * sqlsocket)
{
	sql_log(SQL_LOG_DEBUG(2),
		"sql_socket_put: Released sql socket id: %d",
		sqlsocket->id);
	return 0;
}

int sql_client_fetch(sql_inst_t *inst, sql_socket_t *sqlsocket)
{
	int ret;

	if (sqlsocket->conn) {
		ret = (inst->driver->sql_fetch_row)(sqlsocket, inst->config);
	} else {
		ret = SQL_DOWN;
	}

	if (ret == SQL_DOWN) {
	        /* close the socket that failed, but only if it was open */
		if (sqlsocket->conn) {
			(inst->driver->sql_close)(sqlsocket, inst->config);
		}

		/* reconnect the socket */
		if (connect_single_socket(sqlsocket, inst) < 0) {
			sql_log(SQL_LOG_ERR,
				"sql_fetch_row: reconnect failed, database down?");
			return -1;
		}

		/* retry the query on the newly connected socket */
		ret = (inst->driver->sql_fetch_row)(sqlsocket, inst->config);

		if (ret) {
			sql_log(SQL_LOG_ERR,
				"rlm_sql (%s): failed after re-connect");
			return -1;
		}
	}
	return ret;
}

int sql_client_query(sql_inst_t *inst, sql_socket_t *sqlsocket, char *query)
{
	int ret;

	/* if there's no query, return an error */
	if (!query || !*query) {
		return -1;
	}

	sql_query_log(inst, query);

	ret = (inst->driver->sql_query)(sqlsocket, inst->config, query);
	if (ret == SQL_DOWN) {
	        /* close the socket that failed */
	        (inst->driver->sql_close)(sqlsocket, inst->config);

		/* reconnect the socket */
		if (connect_single_socket(sqlsocket, inst) < 0) {
			sql_log(SQL_LOG_ERR,
				"rlm_sql (%s): reconnect failed, database down?");
			return -1;
		}

		/* retry the query on the newly connected socket */
		ret = (inst->driver->sql_query)(sqlsocket, inst->config, query);
		if (ret) {
			sql_log(SQL_LOG_ERR,
				"sql_perform_query: failed after re-connect");
			return -1;
		}
	}
	return ret;
}

int sql_client_select(sql_inst_t *inst, sql_socket_t *sqlsocket, char *query)
{
	int ret;

	/* if there's no query, return an error */
	if (!query || !*query) {
		return -1;
	}

	ret = (inst->driver->sql_select_query)(sqlsocket, inst->config, query);

	if (ret == SQL_DOWN) {
	        /* close the socket that failed */
	        (inst->driver->sql_close)(sqlsocket, inst->config);

		/* reconnect the socket */
		if (connect_single_socket(sqlsocket, inst) < 0) {
			sql_log(SQL_LOG_ERR,
				"sql_select_query: reconnect failed, database down?");
			return -1;
		}

		/* retry the query on the newly connected socket */
		ret = (inst->driver->sql_select_query)(sqlsocket, inst->config, query);

		if (ret) {
			sql_log(SQL_LOG_ERR,
				"sql_select_query: failed after re-connect");
			return -1;
		}
	}
	return ret;
}

const char *sql_dattetime_string(int utc, time_t time)
{
	struct tm *ptm;
	static char tmstr[32] = "";

	if (utc)
		ptm = gmtime(&time);
	else
		ptm = localtime(&time);
	
	if (ptm)
		strftime(tmstr, sizeof(tmstr), "%Y%m%d %H:%M:%S", ptm);
	else
		sprintf(tmstr, "Invalid Time %l", time);
	return tmstr;
}

int sql_lockfd(int fd, int lock_len)
{
#ifdef WIN32
	return _locking(fd, _LK_LOCK, lock_len);
#else
#if defined(F_LOCK) && !defined(BSD)
	return lockf(fd, F_LOCK, lock_len);
#elif defined(LOCK_EX)
	return flock(fd, LOCK_EX);
#else
	struct flock fl;
	fl.l_start = 0;
	fl.l_len = lock_len;
	fl.l_pid = getpid();
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_CUR;
	return fcntl(fd, F_SETLKW, (void *)&fl);
#endif
#endif
}

#define MAX_QUERY_LEN			4096

void sql_query_log(sql_inst_t *inst, char *querystr)
{
	FILE   *sqlfile = NULL;

	if (inst->config->sql_trace) {
		if ((sqlfile = fopen(inst->config->sql_tracefile, "a")) == (FILE *) NULL) {
			sql_log(SQL_LOG_ERR,
				"sql_query_log:  Couldn't open file %s",
				inst->config->sql_tracefile);
		} else {
			int fd = fileno(sqlfile);

			sql_lockfd(fd, MAX_QUERY_LEN);
			fputs(querystr, sqlfile);
			fputs(";\n", sqlfile);
			fclose(sqlfile); /* and release the lock */
		}
	}
}
