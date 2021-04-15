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
 * @(#)sql_parser.h: SQL syntax parser interface
 * $Id: sql_parser.h,v 1.3 2007-04-16 13:17:19 zhenglv Exp $
 */

#ifndef __SQL_PARSER_H_INCLUDE__
#define __SQL_PARSER_H_INCLUDE__

#define __inherit__

typedef unsigned long sql_param_t;
typedef struct _sql_expr_t sql_expr_t;
typedef struct _sql_type_t sql_type_t;
typedef struct _sql_atom_t sql_atom_t;
typedef struct _sql_table_t sql_table_t;
typedef struct _sql_column_t sql_column_t;
typedef struct _sql_assign_t sql_assign_t;
typedef struct _sql_refer_t sql_refer_t;
typedef struct _sql_order_t sql_order_t;
typedef struct _sql_option_t sql_option_t;
typedef struct _sql_constr_t sql_constr_t;
typedef struct _sql_func_t sql_func_t;

#define SQL_TYPE_CHARACTER	1
#define SQL_TYPE_NUMERIC	2
#define SQL_TYPE_DECIMAL	3
#define SQL_TYPE_INTEGER	4
#define SQL_TYPE_SMALLINT	5
#define SQL_TYPE_BIGINT		6
#define SQL_TYPE_FLOAT		7
#define SQL_TYPE_REAL		8
#define SQL_TYPE_DOUBLE		9
#define SQL_TYPE_VARCHAR	10
#define SQL_TYPE_DATETIME	11
#define SQL_TYPE_TIMESTAMP	12

struct _sql_type_t {
	__inherit__ sql_node_t parent;
	int datatype;
	int length;
	int decimal;
};

#define SQL_ATOM_INTEGER	1
#define SQL_ATOM_FLOAT		2
#define SQL_ATOM_STRING		3
/* current user */
#define SQL_ATOM_USER		4
#define SQL_ATOM_NULL		5

typedef union _sql_atomdata_t {
	int intval;
	double floatval;
	char *strval;
} sql_atomdata_t;

struct _sql_atom_t {
	__inherit__ sql_node_t parent;
	int atomtype;
	sql_atomdata_t value;
};

#define SQL_OPT_AUTO_INCREMENT	4
#define SQL_OPT_DEFAULT		2
#define SQL_OPT_KEY		3
#define SQL_OPT_CHECK		5
#define SQL_OPT_REFERENCE	6

#define SQL_DROP_CASCADE	1
#define SQL_DROP_RESTRICT	2

#define SQL_KEY_NOTNULL		1
#define SQL_KEY_UNIQUE		2
#define SQL_KEY_PRIMARY		3
#define SQL_KEY_FOREIGN		4
#define SQL_KEY_INDEX		5

#define SQL_ORDER_NONE		0
#define SQL_ORDER_ASC		1
#define SQL_ORDER_DESC		2

struct _sql_option_t {
	__inherit__ sql_node_t parent;
	int opttype;
	union {
		sql_atom_t *defval;
		int key_opt;
		int auto_increment;
		sql_expr_t *check;
		sql_refer_t *reference;
	} value;
};

#define SQL_ORDER_NUMBER	1
#define SQL_ORDER_COLUMN	2

struct _sql_order_t {
	__inherit__ sql_node_t parent;
	int ordertype;
	union {
		int number;
		sql_column_t *column;
	} orderby;
	int ordering;
};

struct _sql_table_t {
	__inherit__ sql_node_t parent;
	const char *database;
	const char *table;
	char *rangevar;
};

struct _sql_column_t {
	__inherit__ sql_node_t parent;
	const char *database;
	const char *column;
	const char *table;
	int length;		/* for index_column_ref */
	sql_type_t *datatype;	/* for column_def */
	vector_t *options;	/* for column_def */
};

#define SQL_FUNC_COUNT	1
#define SQL_FUNC_SUM	2
#define SQL_FUNC_AVG	3
#define SQL_FUNC_MIN	4
#define SQL_FUNC_MAX	5

struct _sql_func_t {
	__inherit__ sql_node_t parent;
	int functype;
	int distinct;
	int star;
	vector_t *args;
};

#define SQL_SYMBOL_EXPR		1
#define SQL_SYMBOL_ATOM		2
#define SQL_SYMBOL_COLUMN	3
#define SQL_SYMBOL_FUNC		4
#define SQL_SYMBOL_LIST		5

typedef struct _sql_symbol_t {
	__inherit__ sql_node_t parent;
	int symtype;
	union {
		sql_expr_t *expr;
		sql_atom_t *atom;
		sql_func_t *func;
		sql_column_t *column;
		vector_t *atoms;
	} symval;
} sql_symbol_t;

#define SQL_EXPR_SINGLE		0
#define SQL_EXPR_PLUS		1
#define SQL_EXPR_MINUS		2
#define SQL_EXPR_MULTIPLY	3
#define SQL_EXPR_DIVIDE		4
#define SQL_EXPR_AND		5
#define SQL_EXPR_OR		6
#define SQL_EXPR_NOT		7

#define SQL_EXPR_LT		9
#define SQL_EXPR_GT		10
#define SQL_EXPR_EQ		11
#define SQL_EXPR_NEQ		12
#define SQL_EXPR_LTEQ		13
#define SQL_EXPR_GTEQ		14

#define SQL_EXPR_LIKE		15
#define SQL_EXPR_UNLIKE		16
#define SQL_EXPR_IN		17
#define SQL_EXPR_NOTIN		18

struct _sql_expr_t {
	__inherit__ sql_node_t parent;
	int exprtype;
	sql_symbol_t *left;
	sql_symbol_t *right;
	int notnull;
};

struct _sql_assign_t {
	__inherit__ sql_node_t parent;
	sql_column_t *column;
	sql_expr_t *value;
};

struct _sql_refer_t {
	__inherit__ sql_node_t parent;
	sql_table_t *table;
	vector_t *columns;
};

struct _sql_constr_t {
	__inherit__ sql_node_t parent;
	int keytype;
	vector_t *columns;
	sql_refer_t *refer;
};

#define SQL_STATEMENT_INSERT	1
#define SQL_STATEMENT_UPDATE	2
#define SQL_STATEMENT_DELETE	3
#define SQL_STATEMENT_SELECT	4
#define SQL_STATEMENT_COMMIT	5
#define SQL_STATEMENT_ROLLBACK	6
#define SQL_STATEMENT_CREATE	7
#define SQL_STATEMENT_DROP	8

#define SQL_OPT_ALL		0
#define SQL_OPT_DISTINCT	1

typedef struct _sql_normal_stmt_t {
	__inherit__ sql_node_t parent;
	int stmttype;
} sql_normal_stmt_t;

typedef struct _sql_select_stmt_t {
	__inherit__ sql_normal_stmt_t parent;
	int opttype;
	int option;
	vector_t *selection;
	vector_t *from;
	vector_t *group_by;
	sql_expr_t *where;
	sql_expr_t *having;
	vector_t *order_by;
} sql_select_stmt_t;

typedef struct _sql_insert_stmt_t {
	__inherit__ sql_normal_stmt_t parent;
	sql_table_t *table;
	vector_t *columns;
	vector_t *values;
	sql_select_stmt_t *query;
} sql_insert_stmt_t;

typedef struct _sql_update_stmt_t {
	__inherit__ sql_normal_stmt_t parent;
	sql_table_t *table;
	vector_t *assigns;
	sql_expr_t *where;
} sql_update_stmt_t;

typedef struct _sql_delete_stmt_t {
	__inherit__ sql_normal_stmt_t parent;
	sql_table_t *table;
	sql_expr_t *where;
} sql_delete_stmt_t;

typedef struct _sql_create_stmt_t {
	__inherit__ sql_normal_stmt_t parent;
	sql_table_t *table;
	vector_t *columns;
	/*
	 * constraints, parser will collect all constraint
	 * definitions into columns, this need to be fixed up
	 */
	vector_t *constrs;
} sql_create_stmt_t;

typedef struct _sql_drop_stmt_t {
	__inherit__ sql_normal_stmt_t parent;
	sql_table_t *table;
	int behavior;
} sql_drop_stmt_t;

sql_atom_t *sql_atom_new(int type, sql_param_t val);
void sql_atom_free(sql_atom_t *atom);

sql_table_t *sql_table_new(const char *database, const char *name);
void sql_table_free(sql_table_t *table);

sql_column_t *sql_column_new(const char *database, const char *table,
			     const char *name, int length);
void sql_column_free(sql_column_t *column);

sql_option_t *sql_option_new(int type, sql_param_t val);
void sql_option_free(sql_option_t *option);

sql_expr_t *sql_expr_new(int type, sql_expr_t *left, sql_expr_t *right);
sql_expr_t *sql_expr_expr(sql_expr_t *expr);
sql_expr_t *sql_expr_atom(sql_atom_t *atom);
sql_expr_t *sql_expr_func(sql_func_t *func);
sql_expr_t *sql_expr_column(sql_column_t *column);
sql_expr_t *sql_expr_list(vector_t *list);
void sql_expr_free(sql_expr_t *expr);

sql_assign_t *sql_assign_new(sql_column_t *column, sql_expr_t *value);
void sql_assign_free(sql_assign_t *assign);

sql_refer_t *sql_refer_new(sql_table_t *table, vector_t *columns);
void sql_refer_free(sql_refer_t *refer);

sql_order_t *sql_order_new(int type, int ordering, sql_param_t value);
void sql_order_free(sql_order_t *order);

sql_func_t *sql_func_new(int type, vector_t *args);
void sql_func_free(sql_func_t *func);

sql_constr_t *sql_constr_new(int key, vector_t *columns, sql_refer_t *refer);

sql_node_t *sql_statement_new(int type);
void sql_statement_free(sql_normal_stmt_t *stmt);
char *sql_statement_dump(sql_normal_stmt_t *stmt);

int sql_parse_file(char *file);

char *sql_string_escape(const char *str);
#define sql_string_free(str)			(str ? free(str), str = NULL : NULL)
char *sql_string_append(char *old_str, const char *str);

#endif /* __SQL_PARSER_H_INCLUDE__ */
