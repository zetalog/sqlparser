#ifndef __SQL_PRIV_H_INCLUDE__
#define __SQL_PRIV_H_INCLUDE__

#ifdef WIN32
#include <io.h>
#include <sys/locking.h>
#define strcasecmp	stricmp
#define vsnprintf	_vsnprintf
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <sql.h>

extern char *sql_parse_token;

typedef struct _sql_map_t {
	const char *name;
	int number;
} sql_map_t;

int sql_str2int(const sql_map_t *table, const char *name, int def);
const char *sql_int2str(const sql_map_t *table, int number, const char *def);

void sql_parse_start(void);
void sql_parse_save(char *s, int l);
void sql_parse_end(void);
void sql_parse_done(void);

#define sql_type_new(_type_)				\
	( sql_new_type = sql_node_make(type),		\
	  sql_new_type->datatype = SQL_TYPE_##_type_,	\
	  sql_new_type)

extern sql_type_t *sql_new_type;
extern sql_atom_t *sql_user;

char *sql_symbol_dump(sql_symbol_t *symbol);
char *sql_expr_dump(sql_expr_t *expr);
char *sql_atom_dump(sql_atom_t *atom);
char *sql_column_dump(sql_column_t *column);
char *sql_order_dump(sql_order_t *order);
char *sql_func_dump(sql_func_t *func);
char *sql_assign_dump(sql_assign_t *assign);
char *sql_table_dump(sql_table_t *table);

char *sql_column_list_dump(vector_t *columns);
char *sql_column_def_dump(vector_t *columns);
char *sql_options_dump(vector_t *options);
char *sql_datatype_dump(sql_type_t *type);
char *sql_order_list_dump(vector_t *orders);
char *sql_atom_list_dump(vector_t *atoms, int brack);
char *sql_assign_list_dump(vector_t *assigns, int brack);
char *sql_expr_list_dump(vector_t *exprs, int brack);
char *sql_table_list_dump(vector_t *tables);

char *sql_select_stmt_dump(sql_select_stmt_t *stmt);
char *sql_update_stmt_dump(sql_update_stmt_t *stmt);
char *sql_insert_stmt_dump(sql_insert_stmt_t *stmt);
char *sql_delete_stmt_dump(sql_delete_stmt_t *stmt);
char *sql_create_stmt_dump(sql_create_stmt_t *stmt);
char *sql_drop_stmt_dump(sql_drop_stmt_t *stmt);

void sql_create_stmt_fixup(sql_create_stmt_t *stmt);

#endif /* __SQL_PRIV_H_INCLUDE__ */
