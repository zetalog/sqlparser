#ifdef WIN32
#include <winsock.h>
#endif

#include <kconfig.h>

#ifdef CONFIG_SQL_MYSQL
#include <mysql.h>
#include <errmsg.h>

#include "sql_priv.h"

typedef struct _sql_mysql_t {
	MYSQL conn;
	MYSQL *sock;
	MYSQL_RES *result;
	sql_row_t row;
} sql_mysql_t;

static int _sql_init_socket(sql_socket_t *sqlsocket, sql_config_t *config)
{
	sql_mysql_t *mysql_sock;

	if (!sqlsocket->conn) {
		sqlsocket->conn = (sql_mysql_t *)malloc(sizeof(sql_mysql_t));
		if (!sqlsocket->conn) {
			return -1;
		}
	}
	mysql_sock = sqlsocket->conn;
	memset(mysql_sock, 0, sizeof(*mysql_sock));

	sql_log(SQL_LOG_DEBUG(1), "mysql: Starting connect to MySQL server for #%d",
		sqlsocket->id);

	mysql_init(&(mysql_sock->conn));
	mysql_options(&(mysql_sock->conn), MYSQL_READ_DEFAULT_GROUP, config->sql_db);
	if (!(mysql_sock->sock = mysql_real_connect(&(mysql_sock->conn),
						    config->sql_server,
						    config->sql_login,
						    config->sql_password,
						    config->sql_db,
						    atoi(config->sql_port),
						    NULL,
						    CLIENT_FOUND_ROWS))) {
		sql_log(SQL_LOG_ERR, "mysql: Couldn't connect socket to MySQL server %s@%s:%s",
			config->sql_login, config->sql_server, config->sql_db);
		sql_log(SQL_LOG_ERR, "mysql: Mysql error '%s'",
			mysql_error(&mysql_sock->conn));
		mysql_sock->sock = NULL;
		return -1;
	}
	return 0;
}

static int _sql_destroy_socket(sql_socket_t *sqlsocket, sql_config_t *config)
{
	free(sqlsocket->conn);
	sqlsocket->conn = NULL;
	return 0;
}

static int _sql_check_error(int error)
{
	switch(error) {
	case CR_SERVER_GONE_ERROR:
	case CR_SERVER_LOST:
	case -1:
		sql_log(SQL_LOG_DEBUG(2),
			"mysql: MYSQL check_error: %d, returning SQL_DOWN", error);
		return SQL_DOWN;
		break;
	case 0:
		return 0;
		break;
	case CR_OUT_OF_MEMORY:
	case CR_COMMANDS_OUT_OF_SYNC:
	case CR_UNKNOWN_ERROR:
	default:
		sql_log(SQL_LOG_DEBUG(2),
			"mysql: MYSQL check_error: %d received", error);
		return -1;
		break;
	}
}

static int _sql_query(sql_socket_t * sqlsocket, sql_config_t *config, char *querystr)
{
	sql_mysql_t *mysql_sock = sqlsocket->conn;

	if (config->sql_trace)
		sql_log(SQL_LOG_DEBUG(2), "query:  %s", querystr);
	if (mysql_sock->sock == NULL) {
		sql_log(SQL_LOG_ERR, "mysql: Socket not connected");
		return SQL_DOWN;
	}

	mysql_query(mysql_sock->sock, querystr);
	return _sql_check_error(mysql_errno(mysql_sock->sock));
}

static int _sql_store_result(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_mysql_t *mysql_sock = sqlsocket->conn;

	if (mysql_sock->sock == NULL) {
		sql_log(SQL_LOG_ERR, "mysql: Socket not connected");
		return SQL_DOWN;
	}
	if (!(mysql_sock->result = mysql_store_result(mysql_sock->sock))) {
		sql_log(SQL_LOG_ERR, "mysql: MYSQL Error: Cannot get result");
		sql_log(SQL_LOG_ERR, "mysql: MYSQL Error: %s",
			mysql_error(mysql_sock->sock));
		return _sql_check_error(mysql_errno(mysql_sock->sock));
	}
	return 0;
}

static int _sql_num_fields(sql_socket_t * sqlsocket, sql_config_t *config)
{
	int     num = 0;
	sql_mysql_t *mysql_sock = sqlsocket->conn;

#if MYSQL_VERSION_ID >= 32224
	if (!(num = mysql_field_count(mysql_sock->sock))) {
#else
	if (!(num = mysql_num_fields(mysql_sock->sock))) {
#endif
		sql_log(SQL_LOG_ERR, "mysql: MYSQL Error: No Fields");
		sql_log(SQL_LOG_ERR, "mysql: MYSQL error: %s",
			mysql_error(mysql_sock->sock));
	}
	return num;
}

static int _sql_select_query(sql_socket_t *sqlsocket, sql_config_t *config,
			    char *querystr)
{
	int ret;

	ret = _sql_query(sqlsocket, config, querystr);
	if (ret)
		return ret;
	ret = _sql_store_result(sqlsocket, config);
	if (ret) {
		return ret;
	}

	/* Why? Per http://www.mysql.com/doc/n/o/node_591.html,
	 * this cannot return an error.  Perhaps just to complain if no
	 * fields are found?
	 */
	_sql_num_fields(sqlsocket, config);
	return ret;
}

static int _sql_num_rows(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_mysql_t *mysql_sock = sqlsocket->conn;

	if (mysql_sock->result)
		return (int)mysql_num_rows(mysql_sock->result);
	return 0;
}

static int _sql_fetch_row(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_mysql_t *mysql_sock = sqlsocket->conn;

	/* check pointer before de-referencing it */
	if (!mysql_sock->result) {
		return SQL_DOWN;
	}

	sqlsocket->row = mysql_fetch_row(mysql_sock->result);

	if (sqlsocket->row == NULL) {
		return _sql_check_error(mysql_errno(mysql_sock->sock));
	}
	return 0;
}

static int _sql_free_result(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_mysql_t *mysql_sock = sqlsocket->conn;

	if (mysql_sock->result) {
		mysql_free_result(mysql_sock->result);
		mysql_sock->result = NULL;
	}
	return 0;
}

static const char *_sql_error(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_mysql_t *mysql_sock = sqlsocket->conn;

	if (mysql_sock == NULL || mysql_sock->sock == NULL) {
		return "mysql: no connection to db";
	}
	return mysql_error(mysql_sock->sock);
}

static int _sql_close(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_mysql_t *mysql_sock = sqlsocket->conn;

	if (mysql_sock && mysql_sock->sock){
		mysql_close(mysql_sock->sock);
		mysql_sock->sock = NULL;
	}

	return 0;
}

static int _sql_finish_query(sql_socket_t * sqlsocket, sql_config_t *config)
{
	return 0;
}

static int _sql_finish_select_query(sql_socket_t * sqlsocket, sql_config_t *config)
{
	_sql_free_result(sqlsocket, config);
	return 0;
}

static int _sql_affected_rows(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_mysql_t *mysql_sock = sqlsocket->conn;
	return (int)mysql_affected_rows(mysql_sock->sock);
}

sql_driver_t sql_mysql_driver = {
	"MySQL",
	_sql_init_socket,
	_sql_destroy_socket,
	_sql_query,
	_sql_select_query,
	_sql_store_result,
	_sql_num_fields,
	_sql_num_rows,
	_sql_fetch_row,
	_sql_free_result,
	_sql_error,
	_sql_close,
	_sql_finish_query,
	_sql_finish_select_query,
	_sql_affected_rows
};
#endif

