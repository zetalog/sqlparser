#include "sql_priv.h"
#include <stdarg.h>

static void sql_log_default(const void *data, int severity, char *msg);

sql_log_f sql_log_call = sql_log_default;
int sql_log_level = SQL_LOG_INFO;
const void *sql_log_data = NULL;

static const sql_map_t sql_log_types[] = {
	{ "crit", SQL_LOG_CRIT, },
	{ "err",  SQL_LOG_ERR, },
	{ "warn", SQL_LOG_WARN, },
	{ "info", SQL_LOG_INFO, },
	{ NULL,   0, },
};

int sql_str2log(const char *name)
{
	return sql_str2int(sql_log_types, name, SQL_LOG_ERR);
}

const char *sql_log2str(int number)
{
	return sql_int2str(sql_log_types, number, "dbg");
}

void sql_log_default(const void *data, int severity, char *msg)
{
	if (msg)
		fprintf(stderr, "<%s> %s", sql_log2str(severity), msg);
}

void sql_log_start(int level, sql_log_f func, const void *data)
{
	sql_log_level = level;
	if (func) {
		sql_log_call = func;
		sql_log_data = data;
	}
}

#define SQL_MAX_ERROR	1024
void sql_logv(int severity, char *format, va_list args)
{
	char errmsg[SQL_MAX_ERROR+1] = "";
	int size = SQL_MAX_ERROR;
	int i;
	char *ptr = errmsg;

	ptr += strlen(errmsg);
	i = vsnprintf(ptr, size, format, args);

	if (sql_log_call)
		sql_log_call(sql_log_data, severity, errmsg);
}

void sql_log(int severity, char *format, ...)
{
	va_list args;

	va_start(args, format);
	sql_logv(severity, format, args);
	va_end(args);
}
