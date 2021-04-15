#include <kconfig.h>
#include "sql_priv.h"

#ifdef CONFIG_SQL_TDS
#include <tds.h>
#include <tdsconvert.h>

typedef struct _sql_tds_t {
	TDSLOGIN *login;
	TDSSOCKET *sock;
	TDSCONTEXT *context;
	sql_row_t row;
} sql_tds_t;

static int _sql_use_database(TDSSOCKET *tds, const char *name);
static int _sql_close(sql_socket_t * sqlsocket, sql_config_t *config);

static int _sql_destroy_socket(sql_socket_t *sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock = sqlsocket->conn;

	if (tds_sock) {
		_sql_close(sqlsocket, config);
		free(tds_sock);
		tds_sock = NULL;
	}
	return 0;
}

static int _sql_init_socket(sql_socket_t *sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock;
	TDSCONNECTION *connection;

	if (!sqlsocket->conn) {
		sqlsocket->conn = (sql_tds_t *)malloc(sizeof(sql_tds_t));
		if (!sqlsocket->conn) {
			return -1;
		}
		memset(sqlsocket->conn, 0, sizeof (sql_tds_t));
	}
	tds_sock = sqlsocket->conn;
	memset(tds_sock, 0, sizeof(*tds_sock));

	sql_log(SQL_LOG_DEBUG(1), "tds: Starting connect to TDS server for #%d",
		sqlsocket->id);

	tds_sock->login = tds_alloc_login();
	if (!tds_sock->login) {
		sql_log(SQL_LOG_ERR, "tds: tds_alloc_login() failed.");
		_sql_close(sqlsocket, config);
		return -1;
	}
	tds_set_passwd(tds_sock->login, config->sql_password);
	tds_set_user(tds_sock->login, config->sql_login);
	tds_set_server(tds_sock->login, config->sql_server);
	tds_set_port(tds_sock->login, atoi(config->sql_port));
	tds_set_app(tds_sock->login, "sql");
	tds_set_host(tds_sock->login, "myhost");
	tds_set_library(tds_sock->login, "TDS-Library");
//	tds_set_client_charset(tds_sock->login, "ISO-8859-1");
	tds_set_client_charset(tds_sock->login, "utf8");
	tds_set_language(tds_sock->login, "us_english");
	tds_set_version(tds_sock->login, 8, 0);

	tds_sock->context = tds_alloc_context(NULL);
	tds_sock->sock = tds_alloc_socket(tds_sock->context, 512);
	tds_set_parent(tds_sock->sock, NULL);
	connection = tds_read_config_info(NULL, tds_sock->login, tds_sock->context->locale);

	if (!connection || tds_connect(tds_sock->sock, connection) == TDS_FAIL) {
		if (connection) {
			tds_free_socket(tds_sock->sock);
			tds_sock->sock = NULL;
			tds_free_connection(connection);
		}
		sql_log(SQL_LOG_ERR, "tds: Couldn't connect socket to TDS server %s@%s",
			config->sql_login, config->sql_server);
		_sql_close(sqlsocket, config);
		return -1;
	}
	tds_free_connection(connection);
	if (_sql_use_database(tds_sock->sock, config->sql_db) != 0) {
		sql_log(SQL_LOG_ERR, "tds: Couldn't use TDS database %s",
			config->sql_db);
		_sql_close(sqlsocket, config);
		return -1;
	}
	return 0;
}

static int _sql_process_result(TDSSOCKET * tds)
{
	int rc;
	int result_type;

	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_RESULTS)) == TDS_SUCCEED) {
		switch (result_type) {
		case TDS_DONE_RESULT:
		case TDS_DONEPROC_RESULT:
		case TDS_DONEINPROC_RESULT:
			/* ignore possible spurious result (TDS7+ send it) */
		case TDS_STATUS_RESULT:
			break;
		default:
			sql_log(SQL_LOG_ERR, "tds: query should not return results");
			return -1;
		}
	}
	if (rc == TDS_FAIL) {
		sql_log(SQL_LOG_ERR, "tds: tds_process_tokens() returned TDS_FAIL");
		return -1;
	} else if (rc != TDS_NO_MORE_RESULTS) {
		sql_log(SQL_LOG_ERR, "tds: tds_process_tokens() unexpected return");
		return -1;
	}
	return 0;
}

static int _sql_use_database(TDSSOCKET *tds, const char *name)
{
	char query[512];

	strcpy(query, "use ");
	if (name[0] == '[' && name[strlen(name)-1] == ']')
		strcat(query, name);
	else
		tds_quote_id(tds, query + 4, name, -1);
	if (tds_submit_query(tds, query) != TDS_SUCCEED) {
		sql_log(SQL_LOG_ERR, "tds: tds_submit_query() failed");
		return -1;
	}
	if (tds_process_simple_query(tds) != TDS_SUCCEED) {
		return -1;
	}
	return 0;
}

static int _sql_query(sql_socket_t * sqlsocket, sql_config_t *config, char *querystr)
{
	sql_tds_t *tds_sock = sqlsocket->conn;

	if (config->sql_trace)
		sql_log(SQL_LOG_ERR, "query:  %s", querystr);
	if (tds_sock->sock == NULL) {
		sql_log(SQL_LOG_ERR, "tds: Socket not connected");
		return SQL_DOWN;
	}

	if (tds_submit_query(tds_sock->sock, querystr) != TDS_SUCCEED) {
		sql_log(SQL_LOG_ERR, "tds: tds_submit_query() failed");
		return -1;
	}
	if (tds_process_simple_query(tds_sock->sock) != TDS_SUCCEED) {
		return -1;
	}
	return 0;
}

static int _sql_num_fields(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock = sqlsocket->conn;
	TDSRESULTINFO *result_info;
	
	/* Get information about the resulting set */
	result_info = tds_sock->sock->res_info;
	if (result_info == NULL) {
		sql_log(SQL_LOG_ERR, "tds: Can't get information about the resulting set");
		return -1;
	}
	return result_info->num_cols;
}

static int _sql_free_result(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock = sqlsocket->conn;
	int column, numfileds = _sql_num_fields(sqlsocket, config);
	
	/* Freeing reserved memory */
	if (tds_sock->row != NULL) {
		for (column=0; column < numfileds; column++) {
			if (tds_sock->row[column] != NULL) {
				free(tds_sock->row[column]);
				tds_sock->row[column] = NULL;
			}
		}
		free(tds_sock->row);
		tds_sock->row = NULL;
	}
	return 0;
}

static int _sql_store_result(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock = sqlsocket->conn;
	TDSCOLUMN **columns;
	int numfields, column;
	
	/* Check if memory were allocated */
	if (tds_sock->row != NULL)
		_sql_free_result(sqlsocket, config);
	
	/* Getting amount of result fields */
	numfields = _sql_num_fields(sqlsocket, config);
	if (numfields < 0) return -1;
	
	/* Get information about the column set */
	columns = tds_sock->sock->res_info->columns;
	if (columns == NULL) {
		sql_log(SQL_LOG_ERR, "tds: Can't get information about the column set");
		return -1;
	}
	
	/* Reserving memory for a result set */
	tds_sock->row = (char **) malloc((numfields+1)*sizeof(char *));
	if (tds_sock->row == NULL) {
		sql_log(SQL_LOG_ERR, "tds: Can't allocate the memory");
		return -1;
	}
	
	for (column = 0; column < numfields; column++)
		tds_sock->row[column] = NULL;
	tds_sock->row[numfields] = NULL;

	return 0;
}

static int _sql_select_query(sql_socket_t *sqlsocket, sql_config_t *config,
			    char *querystr)
{
	sql_tds_t *tds_sock = sqlsocket->conn;
	TDS_INT result_type;
	int rc;

	rc = tds_submit_query(tds_sock->sock, querystr);
	if (rc != TDS_SUCCEED) {
		sql_log(SQL_LOG_ERR, "tds: tds_submit_query() failed");
		return -1;
	}

	if (tds_process_tokens(tds_sock->sock, &result_type, NULL, TDS_TOKEN_RESULTS) != TDS_SUCCEED) {
		sql_log(SQL_LOG_ERR, "tds: tds_process_tokens() failed");
		return -1;
	}
	if (result_type != TDS_ROWFMT_RESULT) {
		sql_log(SQL_LOG_ERR, "tds: expected row fmt() failed");
		return -1;
	}
	if (tds_process_tokens(tds_sock->sock, &result_type, NULL, TDS_TOKEN_RESULTS) != TDS_SUCCEED) {
		sql_log(SQL_LOG_ERR, "tds: tds_process_tokens() failed");
		return -1;
	}
	if (result_type != TDS_ROW_RESULT) {
		/* no result */
		return -1;
	}
	return 0;
}

static int _sql_fetch_row(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock = sqlsocket->conn;
	TDSRESULTINFO *result_info;    
	TDSCOLUMN **columns;
	int numfields, column, ret, rc, result_type;
	CONV_RESULT cr;
	
	sqlsocket->row = NULL;
	
	/* Alocating the memory */
	if (_sql_store_result(sqlsocket, config) < 0) return 0;

	/* Getting amount of result fields */
	numfields = _sql_num_fields(sqlsocket, config);
	if (numfields < 0)
		return 0;
	/* Get information about the resulting set */
	result_info = tds_sock->sock->res_info;
	if (result_info == NULL) {
		sql_log(SQL_LOG_ERR, "tds: Can't get information about the resulting set");
		return -1;
	}
	/* Get information about the column set */
	columns = result_info->columns;
	if (columns == NULL) {
		sql_log(SQL_LOG_ERR, "tds: Can't get information about the column set");
		return -1;
	}
	
	while ((rc = tds_process_tokens(tds_sock->sock, &result_type, NULL,
					TDS_STOPAT_ROWFMT|TDS_RETURN_DONE|
					TDS_RETURN_ROW|TDS_RETURN_COMPUTE)) == TDS_SUCCEED) {
		if (result_type == TDS_ROW_RESULT || result_type == TDS_COMPUTE_RESULT) {
			/* Converting the fields to a CHAR data type */
			for (column = 0; column < numfields; column++) {
				TDSCOLUMN *curcol = tds_sock->sock->current_results->columns[column];
				TDS_CHAR *src = (TDS_CHAR *) tds_sock->sock->current_results->current_row +
							     curcol->column_offset;
				int conv_type = tds_get_conversion_type(curcol->column_type, curcol->column_size);

				if (is_blob_type(columns[column]->column_type)) {
					TDSBLOB *blob = (TDSBLOB *) src;
					src = blob->textvalue;
				}
				if (src && curcol->column_cur_size >= 0) {
					ret = tds_convert(tds_sock->context, conv_type, src,
							  curcol->column_cur_size,
							  SYBVARCHAR, &cr);
					if (ret < 0) {
						sql_log(SQL_LOG_ERR, "tds: Error converting");
						return -1;
					} else {
						tds_sock->row[column] = cr.c;
					}
				}
			}
			sqlsocket->row = tds_sock->row;
			return 0;
		}
	}
	if (rc != TDS_NO_MORE_RESULTS && rc != TDS_SUCCEED) {
		sql_log(SQL_LOG_ERR, "tds: tds_process_tokens() unexpected return");
		return -1;
	}
	return -1;
}

static int _sql_affected_rows(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock = sqlsocket->conn;
	return tds_sock->sock->rows_affected;
}

static int _sql_num_rows(sql_socket_t * sqlsocket, sql_config_t *config)
{
	return _sql_affected_rows(sqlsocket, config);
}

static const char *_sql_error(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock = sqlsocket->conn;

	if (tds_sock == NULL || tds_sock->sock == NULL) {
		return "tds: no connection to db";
	}
	return NULL;
}

static int _sql_close(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock = sqlsocket->conn;
	if (tds_sock) {
		if (tds_sock->sock) {
			tds_free_socket(tds_sock->sock);
			tds_sock->sock = NULL;
		}
		if (tds_sock->login) {
			tds_free_login(tds_sock->login);
			tds_sock->login = NULL;
		}
		if (tds_sock->context) {
			tds_free_context(tds_sock->context);
			tds_sock->context = NULL;
		}
	}
	return 0;
}

static int _sql_finish_query(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock = sqlsocket->conn;

	return _sql_process_result(tds_sock->sock);
}

static int _sql_finish_select_query(sql_socket_t * sqlsocket, sql_config_t *config)
{
	sql_tds_t *tds_sock = sqlsocket->conn;

	_sql_free_result(sqlsocket, config);
	/* make sure the current statement is complete */
	if (tds_sock->sock->state == TDS_PENDING) {
		/* send 'cancel' packet */
		tds_send_cancel(tds_sock->sock);
		/* process 'cancel' packet */
		tds_process_cancel(tds_sock->sock);
	}
	return 0;
}

sql_driver_t sql_tds_driver = {
	"FreeTDS",
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

