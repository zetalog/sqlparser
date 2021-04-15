#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sql_priv.h"

extern FILE *sql_out;	/* lex output file */

#define SQL_BUFFER_BASE		2048

char *sql_parse_buf = NULL;	/* buffer for SQL command */
char *sql_parse_cbp = NULL;	/* current buffer pointer */
int sql_parse_len = 0;
char *sql_parse_token = NULL;

extern void sql_error(char *);

list_head(sql_parse_tree);
sql_node_t *sql_new_node = NULL;
vector_t *sql_new_vector = NULL;
sql_type_t *sql_new_type;
sql_atom_t *sql_user = NULL;

int sql_str2int(const sql_map_t *table, const char *name, int def)
{
	const sql_map_t *this;

	for (this = table; this->name != NULL; this++) {
		if (strcasecmp(this->name, name) == 0) {
			return this->number;
		}
	}
	return def;
}

const char *sql_int2str(const sql_map_t *table, int number, const char *def)
{
	const sql_map_t *this;
	for (this = table; this->name != NULL; this++) {
		if (this->number == number) {
			return this->name;
		}
	}
	return def;
}

static const sql_map_t sql_operators[] = {
	{ "+",      SQL_EXPR_PLUS, },
	{ "-",      SQL_EXPR_MINUS, },
	{ "*",      SQL_EXPR_MULTIPLY, },
	{ "/",      SQL_EXPR_DIVIDE, },
	{ "AND",    SQL_EXPR_AND, },
	{ "OR",     SQL_EXPR_OR, },
	{ "NOT",    SQL_EXPR_NOT, },
	{ "<",      SQL_EXPR_LT, },
	{ ">",      SQL_EXPR_GT, },
	{ "=",      SQL_EXPR_EQ, },
	{ "<>",     SQL_EXPR_NEQ, },
	{ ">=",     SQL_EXPR_GTEQ, },
	{ "<=",     SQL_EXPR_LTEQ, },
	{ "LIKE",   SQL_EXPR_LIKE, },
	{ "UNLIKE", SQL_EXPR_UNLIKE, },
	{ "IN",     SQL_EXPR_IN, },
	{ "NOT IN", SQL_EXPR_NOTIN, },
	{ NULL,     0, },
};

/* Start added by zhangry at 20071218 */
static const sql_map_t sql_datatype[] = {
	{ "CHARACTER", SQL_TYPE_CHARACTER },
	{ "NUMERIC", SQL_TYPE_NUMERIC },
	{ "DECIMAL", SQL_TYPE_DECIMAL },
	{ "INTEGER", SQL_TYPE_INTEGER },
	{ "SMALLINT", SQL_TYPE_SMALLINT },
	{ "BIGINT", SQL_TYPE_BIGINT },
	{ "FLOAT", SQL_TYPE_FLOAT },
	{ "REAL", SQL_TYPE_REAL },
	{ "DOUBLE", SQL_TYPE_DOUBLE },
	{ "VARCHAR", SQL_TYPE_VARCHAR },
	{ "DATETIME", SQL_TYPE_DATETIME },
	{ "TIMESTAMP", SQL_TYPE_TIMESTAMP },
};

const char *sql_type2str(int datatype)
{
	return sql_int2str(sql_datatype, datatype, "");
}

static const sql_map_t sql_options[] = {
	{ "NOT NULL", SQL_KEY_NOTNULL },
	{ "NOT NULL UNIQUE", SQL_KEY_UNIQUE },
	{ "NOT NULL PRIMARY KEY", SQL_KEY_PRIMARY },
	{ "FOREIGN", SQL_KEY_FOREIGN },
	{ "INDEX", SQL_KEY_INDEX },
};

const char *sql_opts2str(int optstype)
{
	return sql_int2str(sql_options, optstype, "");
}

/* End added by zhangry at 20071218 */

const char *sql_oper2str(int exprtype)
{
	return sql_int2str(sql_operators, exprtype, "");
}

sql_node_t *sql_node_new(int size, int tag)
{
	assert((size) >= sizeof(sql_node_t));
	sql_new_node = (sql_node_t *) calloc(1, size);
	list_init(&(sql_new_node->list));
	sql_new_node->type = (tag);
	return sql_new_node;
}

void sql_node_free(sql_node_t *node)
{
	if (node) {
		list_delete(&node->list);
		free(node);
	}
}

/* start an embedded command after EXEC SQL */
void sql_parse_start(void)
{
	if (!sql_parse_buf) {
		sql_parse_buf = calloc(1, SQL_BUFFER_BASE);
		sql_parse_len = SQL_BUFFER_BASE;
		sql_parse_cbp = sql_parse_buf;
	}
}

/* save a SQL token */
void sql_parse_save(char *s, int l)
{
	int offset;

	assert(l > 0);

	offset = sql_parse_cbp - sql_parse_buf;
	if ((offset+l+1) > sql_parse_len) {
		sql_parse_buf = realloc(sql_parse_buf, sql_parse_len+SQL_BUFFER_BASE);
		sql_parse_len += SQL_BUFFER_BASE;
		sql_parse_cbp = sql_parse_buf+offset;
	}
	memcpy(sql_parse_cbp, s, l);
	sql_parse_token = sql_parse_cbp;
	sql_parse_cbp += l;
	sql_parse_cbp[0] = '\0';
	sql_parse_cbp++;
}

/* end of SQL command, now write it out */
void sql_parse_end(void)
{
	char *buf = NULL;
	/* call exec_sql function */
	printf("\nDumping structure:\n");
	buf = sql_statement_dump((sql_normal_stmt_t *)sql_new_node);
	if (buf) {
		fprintf(sql_out, "%s;\n", buf);
		free(buf);
		buf = NULL;
	}
	sql_statement_free((sql_normal_stmt_t *)sql_new_node);
	sql_new_node = NULL;

	/* return scanner to regular mode */
	sql_parse_done();
}

void sql_symbol_free(sql_symbol_t *data)
{
	if (data) {
		switch (data->symtype) {
		case SQL_SYMBOL_EXPR:
			sql_expr_free(data->symval.expr);
			break;
		case SQL_SYMBOL_ATOM:
			sql_atom_free(data->symval.atom);
			break;
		case SQL_SYMBOL_FUNC:
			sql_func_free(data->symval.func);
			break;
		case SQL_SYMBOL_COLUMN:
			sql_column_free(data->symval.column);
			break;
		case SQL_SYMBOL_LIST:
			destroy_vector(data->symval.atoms);
			break;
		}
		sql_node_free((sql_node_t *)data);
	}
}

sql_symbol_t *sql_symbol_expr(sql_expr_t *expr)
{
	sql_symbol_t *sym = NULL;
	if (expr) {
		sym = sql_node_make(symbol);
		if (expr->exprtype == SQL_EXPR_SINGLE) {
			/*
			 * optimization for SINGLE expr
			 * single symbol get copied directly from the old,
			 * the old one should be released
			 */
			assert(!expr->right);
			sym->symtype = expr->left->symtype;
			sym->symval = expr->left->symval;
			/* this will save memory */
			sql_node_free((sql_node_t *)expr);
		} else {
			sym->symtype = SQL_SYMBOL_EXPR;
			sym->symval.expr = expr;
		}
	}
	return sym;
}

int sql_symbol_single(sql_symbol_t *symbol)
{
	if (!symbol) return 1;
	if (!symbol->symval.expr) return 1;
	else {
		if (symbol->symtype == SQL_SYMBOL_EXPR) {
			return sql_symbol_single(symbol->symval.expr->left) ||
			       sql_symbol_single(symbol->symval.expr->right);
		}
		if (symbol->symtype == SQL_SYMBOL_LIST)
			return element_count(symbol->symval.atoms) <= 1;
		return 1;
	}
}

char *sql_symbol_dump(sql_symbol_t *symbol)
{
	char *res = NULL;
	char *tmp = NULL;

	if (symbol) {
		switch (symbol->symtype) {
		case SQL_SYMBOL_EXPR:
			if (!sql_symbol_single(symbol))
				res = sql_string_append(res, "(");
			tmp = sql_expr_dump(symbol->symval.expr);
			res = sql_string_append(res, tmp);
			if (!sql_symbol_single(symbol))
				res = sql_string_append(res, ")");
			break;
		case SQL_SYMBOL_ATOM:
			tmp = sql_atom_dump(symbol->symval.atom);
			res = sql_string_append(res, tmp);
			break;
		case SQL_SYMBOL_COLUMN:
			tmp = sql_column_dump(symbol->symval.column);
			res = sql_string_append(res, tmp);
			break;
		case SQL_SYMBOL_FUNC:
			tmp = sql_func_dump(symbol->symval.func);
			res = sql_string_append(res, tmp);
			break;
		case SQL_SYMBOL_LIST:
			tmp = sql_atom_list_dump(symbol->symval.atoms, 1);
			res = sql_string_append(res, tmp);
			break;
		}
	}
	sql_string_free(tmp);
	return res;
}

void sql_expr_free(sql_expr_t *expr)
{
	if (expr) {
		sql_symbol_free(expr->left);
		sql_symbol_free(expr->right);
		sql_node_free((sql_node_t *)expr);
	}
}

sql_expr_t *sql_expr_new(int type, sql_expr_t *left, sql_expr_t *right)
{
	sql_expr_t *expr = sql_node_make(expr);
	if (expr) {
		expr->exprtype = type;
		expr->left = sql_symbol_expr(left);
		expr->right = sql_symbol_expr(right);
	}
	return expr;
}

sql_expr_t *sql_expr_atom(sql_atom_t *atom)
{
	sql_expr_t *expr = sql_node_make(expr);
	if (expr) {
		expr->exprtype = SQL_EXPR_SINGLE;
		expr->left = sql_node_make(symbol);
		if (expr->left) {
			expr->left->symtype = SQL_SYMBOL_ATOM;
			expr->left->symval.atom = atom;
		}
	}
	return expr;
}

sql_expr_t *sql_expr_func(sql_func_t *func)
{
	sql_expr_t *expr = sql_node_make(expr);
	if (expr) {
		expr->exprtype = SQL_EXPR_SINGLE;
		expr->left = sql_node_make(symbol);
		if (expr->left) {
			expr->left->symtype = SQL_SYMBOL_FUNC;
			expr->left->symval.func = func;
		}
	}
	return expr;
}

sql_expr_t *sql_expr_list(vector_t *left)
{
	sql_expr_t *expr = sql_node_make(expr);
	if (expr) {
		expr->exprtype = SQL_EXPR_SINGLE;
		expr->left = sql_node_make(symbol);
		if (expr->left) {
			expr->left->symtype = SQL_SYMBOL_LIST;
			expr->left->symval.atoms = left;
		}
	}
	return expr;
}

sql_expr_t *sql_expr_column(sql_column_t *column)
{
	sql_expr_t *expr = sql_node_make(expr);
	if (expr) {
		expr->exprtype = SQL_EXPR_SINGLE;
		expr->left = sql_node_make(symbol);
		if (expr->left) {
			expr->left->symtype = SQL_SYMBOL_COLUMN;
			expr->left->symval.column = column;
		}
	}
	return expr;
}

sql_expr_t *sql_expr_expr(sql_expr_t *expr)
{
	sql_expr_t *nexpr = sql_node_make(expr);
	if (nexpr) {
		expr->exprtype = SQL_EXPR_SINGLE;
		nexpr->left = sql_node_make(symbol);
		if (nexpr->left) {
			nexpr->left->symtype = SQL_SYMBOL_EXPR;
			nexpr->left->symval.expr = expr;
		}
	}
	return nexpr;
}

char *sql_expr_dump(sql_expr_t *expr)
{
	char *res = NULL;
	char *tmp = NULL;
	if (expr) {
		if (expr->left) {
			res = sql_string_append(res, tmp = sql_symbol_dump(expr->left));
			sql_string_free(tmp);
		}
		if (expr->exprtype != SQL_EXPR_SINGLE) {
			res = sql_string_append(res, " ");
			res = sql_string_append(res, sql_oper2str(expr->exprtype));
			res = sql_string_append(res, " ");
		}
		if (expr->right)
			res = sql_string_append(res, tmp = sql_symbol_dump(expr->right));
	}
	sql_string_free(tmp);
	return res;
}

char *sql_expr_list_dump(vector_t *exprs, int brack)
{
	int count, index;
	char *res = NULL;
	char *tmp = NULL;
	if (exprs) {
		count = element_count(exprs);
		if (count > 0) {
			if (brack)
				res = sql_string_append(res, "(");
			for (index = 0; index < count; index++) {
				res = sql_string_append(res, tmp = sql_expr_dump((sql_expr_t *)get_element(exprs, index)));
				if (index != (count-1))
					res = sql_string_append(res, ", ");
				sql_string_free(tmp);
			}
			if (brack)
				res = sql_string_append(res, ")");
		}
	}
	return res;
}

sql_assign_t *sql_assign_new(sql_column_t *column, sql_expr_t *value)
{
	sql_assign_t *assign = sql_node_make(assign);
	if (assign) {
		assign->column = column;
		assign->value = value;
	}
	return assign;
}

void sql_assign_free(sql_assign_t *assign)
{
	if (assign) {
		if (assign->column)
			sql_column_free(assign->column);
		if (assign->value)
			sql_expr_free(assign->value);
		sql_node_free((sql_node_t *)assign);
	}
}

char *sql_assign_dump(sql_assign_t *assign)
{
	char *res = NULL, *tmp = NULL;
	
	if (assign && assign->column) {
		res = sql_string_append(res, tmp = sql_column_dump(assign->column));
		sql_string_free(tmp);
		res = sql_string_append(res, " = ");
		res = sql_string_append(res, tmp = sql_expr_dump(assign->value));
	}
	sql_string_free(tmp);
	return res;
}

char *sql_assign_list_dump(vector_t *assigns, int brack)
{
	char *res = NULL, *tmp = NULL;
	int count, index;
	if (assigns) {
		count = element_count(assigns);
		if (count > 0) {
			if (brack)
				res = sql_string_append(res, "(");
			for (index = 0; index < count; index++) {
				res = sql_string_append(res, tmp = sql_assign_dump((sql_assign_t *)get_element(assigns, index)));
				if (index != (count-1))
					res = sql_string_append(res, ", ");
				sql_string_free(tmp);
			}
			if (brack)
				res = sql_string_append(res, ")");
		}
	}
	return res;
}

sql_refer_t *sql_refer_new(sql_table_t *table, vector_t *columns)
{
	sql_refer_t *refer = sql_node_make(refer);
	if (refer) {
		refer->columns = columns;
		refer->table = table;
	}
	return refer;
}

void sql_refer_free(sql_refer_t *refer)
{
	if (refer) {
		if (refer->columns)
			destroy_vector(refer->columns);
		if (refer->table)
			sql_table_free(refer->table);
		sql_node_free((sql_node_t *)refer);
	}
}

sql_table_t *sql_table_new(const char *database, const char *name)
{
	sql_table_t *table = sql_node_make(table);
	if (table) {
		table->database = database;
		table->table = name;
	}
	return table;
}

void sql_table_free(sql_table_t *table)
{
	if (table) {
		sql_node_free((sql_node_t *)table);
	}
}

char *sql_table_dump(sql_table_t *table)
{
	char *res = NULL;
	if (table && table->table) {
		if (table->database) {
			res = sql_string_append(res, table->database);
			res = sql_string_append(res, ".");
		}
		res = sql_string_append(res, table->table);
		if (table->rangevar) {
			res = sql_string_append(res, " ");
			res = sql_string_append(res, table->rangevar);
		}
	}
	return res;
}

char *sql_table_list_dump(vector_t *tables)
{
	char *res = NULL, *tmp = NULL;
	int count, index;
	if (tables) {
		count = element_count(tables);
		if (count > 0) {
			for (index = 0; index < count; index++) {
				res = sql_string_append(res, tmp = sql_table_dump((sql_table_t *)get_element(tables, index)));
				if (index != (count-1))
					res = sql_string_append(res, ", ");
				sql_string_free(tmp);
			}
		}
	}
	return res;
}

sql_atom_t *sql_atom_new(int type, sql_param_t val)
{
	sql_atom_t *atom = sql_node_make(atom);
	if (atom) {
		atom->atomtype = type;
		switch (type) {
		case SQL_ATOM_INTEGER:
			atom->value.intval = (int)val;
			break;
		case SQL_ATOM_FLOAT:
			atom->value.floatval = (double)val;
			break;
		case SQL_ATOM_STRING:
			if (!val)
				atom->value.strval = (char *)val;
			else
				atom->value.strval = strdup((char *)val);
			break;
		}
	}
	return atom;
}

void sql_atom_free(sql_atom_t *atom)
{
	if (atom) {
		switch (atom->atomtype) {
		case SQL_ATOM_STRING:
			if (atom->value.strval)
				free(atom->value.strval);
			break;
		}
		sql_node_free((sql_node_t *)atom);
	}
}

char *sql_atom_dump(sql_atom_t *atom)
{
	char *res = NULL;
	char buf[128];

	memset(buf, 0, sizeof(buf));
	if (atom) {
		switch (atom->atomtype) {
		case SQL_ATOM_FLOAT:
			sprintf(buf, "%f", atom->value.floatval);
			res = sql_string_append(res, buf); 
			break;
		case SQL_ATOM_INTEGER:
			sprintf(buf, "%d", atom->value.intval);
			res = sql_string_append(res, buf); 
			break;
		case SQL_ATOM_STRING:
			res = sql_string_append(res, atom->value.strval);
			break;
		case SQL_ATOM_USER:
			res = sql_string_append(res, "USER");
			break;
		case SQL_ATOM_NULL:
			res = sql_string_append(res, "NULL");
			break;
		}
	}
	return res;
}

char *sql_atom_list_dump(vector_t *atoms, int brack)
{
	char *res = NULL, *tmp = NULL;
	int count, index;

	if (atoms) {
		count = element_count(atoms);
		if (count > 0) {
			if (brack)
				res = sql_string_append(res, "(");
			for (index = 0; index < count; index++) {
				res = sql_string_append(res, tmp = sql_atom_dump((sql_atom_t *)get_element(atoms, index)));
				if (index != (count-1))
					res = sql_string_append(res, ", ");
				sql_string_free(tmp);
			}
			if (brack)
				res = sql_string_append(res, ")");
		}
	}
	return res;
}

sql_column_t *sql_column_new(const char *database, const char *table,
			     const char *name, int length)
{
	sql_column_t *column = sql_node_make(column);
	if (column) {
		column->database = database;
		column->table = table;
		column->column = name;
		column->length = length;
	}
	return column;
}

void sql_column_free(sql_column_t *column)
{
	if (column) {
		if (column->options)
			destroy_vector(column->options);
		sql_node_free((sql_node_t *)column);
	}
}

char *sql_column_dump(sql_column_t *column)
{
	char *res = NULL;
	if (column && column->column) {
		if (column->table) {
			if (column->database) {
				res = sql_string_append(res, column->database);
				res = sql_string_append(res, ".");
			}
			res = sql_string_append(res, column->table);
			res = sql_string_append(res, ".");
		}
		res = sql_string_append(res, column->column);
	}
	return res;
}

/* Start added by zhangry at 20071218 */
char *sql_datatype_dump(sql_type_t *type)
{
	char *res = NULL;
	if (type && type->datatype) {
		res = sql_string_append(res, sql_type2str(type->datatype));
		if (type->length) {
			char buf[128];
			memset(buf, 0, sizeof(buf));
			sprintf(buf, "(%d", type->length);
			res = sql_string_append(res, buf);
			if (type->decimal) {
				sprintf(buf, ",%d", type->decimal);
				res = sql_string_append(res, buf);
			}
			res = sql_string_append(res, ")");
		}
	}
	return res;
}

char *sql_constr_dump(sql_constr_t *constr)
{
	char *res = NULL, *tmp = NULL;

	if (constr) {
		switch (constr->keytype) {
		case SQL_KEY_UNIQUE:
			res = sql_string_append(res, "UNIQUE ");
			res = sql_string_append(res, tmp = sql_column_list_dump(constr->columns));
			break;
		case SQL_KEY_PRIMARY:
			res = sql_string_append(res, "PRIMARY KEY ");
			res = sql_string_append(res, tmp = sql_column_list_dump(constr->columns));
			break;
		case SQL_KEY_FOREIGN:
			res = sql_string_append(res, "FOREIGN KEY ");
			res = sql_string_append(res, tmp = sql_column_list_dump(constr->refer->columns));
			sql_string_free(tmp);
			res = sql_string_append(res, "REFERENCES ");
			res = sql_string_append(res, tmp = sql_table_dump(constr->refer->table));
			break;
		}
		sql_string_free(tmp);
	}
	return res;
}

char *sql_options_dump(vector_t *options)
{
	char *res = NULL, *tmp = NULL;
	int count, index;

	if (options) {
		count = element_count(options);
		if (count > 0) {
			for (index = 0; index < count; index++) {
				sql_option_t *opt = (sql_option_t *)get_element(options, index);
				switch (opt->opttype) {
				case SQL_OPT_AUTO_INCREMENT:
					res = sql_string_append(res, "AUTO_INCREMENT");
					break;
				case SQL_OPT_KEY:
					res = sql_string_append(res, sql_opts2str(opt->value.key_opt));
					break;
				case SQL_OPT_DEFAULT:
					res = sql_string_append(res, "DEFAULT ");
					res = sql_string_append(res, tmp = sql_atom_dump(opt->value.defval));
					break;
				case SQL_OPT_CHECK:
					res = sql_string_append(res, "CHECK (");
					res = sql_string_append(res, tmp = sql_expr_dump(opt->value.check));
					res = sql_string_append(res, ")");
					break;
				case SQL_OPT_REFERENCE:
					res = sql_string_append(res, "REFERENCES ");
					res = sql_string_append(res, tmp = sql_table_dump(opt->value.reference->table));
					sql_string_free(tmp);
					res = sql_string_append(res, tmp = sql_column_list_dump(opt->value.reference->columns));
					break;
				}
				if (index != (count -1))
					res = sql_string_append(res, " ");
				sql_string_free(tmp);
			}
		}
	}
	return res;
}
/* End added by zhangry at 20071218 */

char *sql_column_list_dump(vector_t *columns)
{
	char *res = NULL;
	int count, index;

	if (columns) {
		count = element_count(columns);
		if (count > 0) {
			res = sql_string_append(res, "(");
			for (index = 0; index < count; index++) {
				res = sql_string_append(res, 
					sql_column_dump((sql_column_t *)get_element(columns, index)));
				if (index != (count-1))
					res = sql_string_append(res, ", ");
			}
			res = sql_string_append(res, ")");
		}
	}
	return res;
}

/* Start added by zhangry at 20071218 */
char *sql_column_def_dump(vector_t *columns)
{
	char *res = NULL, *tmp = NULL;
	int count, index;

	if (columns) {
		count = element_count(columns);
		if (count > 0) {
			res = sql_string_append(res, "(");
			for (index = 0; index < count; index++) {
				sql_column_t *t = (sql_column_t *)get_element(columns, index);
				if (sql_node_is_type(t, column)) { 
					res = sql_string_append(res, tmp = sql_column_dump(t));
					sql_string_free(tmp);
					if (t->datatype) {
						res = sql_string_append(res, " ");
						res = sql_string_append(res, tmp = sql_datatype_dump(t->datatype));
						sql_string_free(tmp);
						if (t->options) {
							res = sql_string_append(res, " ");
							res = sql_string_append(res, tmp = sql_options_dump(t->options));
							sql_string_free(tmp);
						}
					}
				}
				if (sql_node_is_type(t, constr)) {
					res = sql_string_append(res, tmp = sql_constr_dump((sql_constr_t *)t));
					sql_string_free(tmp);
				}
				if (index != (count-1))
					res = sql_string_append(res, ", ");
			}
			res = sql_string_append(res, ")");
		}
	}
	return res;
}
/* End added by zhangry at 20071218 */

sql_option_t *sql_option_new(int type, sql_param_t val)
{
	sql_option_t *option = sql_node_make(option);
	if (option) {
		option->opttype = type;
		switch (type) {
		case SQL_OPT_AUTO_INCREMENT:
			option->value.auto_increment = (int)val;
			break;
		case SQL_OPT_KEY:
			option->value.key_opt = (int)val;
			break;
		case SQL_OPT_DEFAULT:
			option->value.defval = (sql_atom_t *)val;
			break;
		case SQL_OPT_CHECK:
			option->value.check = (sql_expr_t *)val;
			break;
		case SQL_OPT_REFERENCE:
			option->value.reference = (sql_refer_t *)val;
			break;
		}
	}
	return option;
}

void sql_option_free(sql_option_t *option)
{
	if (option) {
		switch (option->opttype) {
		case SQL_OPT_CHECK:
			sql_expr_free(option->value.check);
			break;
		case SQL_OPT_REFERENCE:
			sql_refer_free(option->value.reference);
			break;
		}
		sql_node_free((sql_node_t *)option);
	}
}

sql_order_t *sql_order_new(int type, int ordering, sql_param_t value)
{
	sql_order_t *order = sql_node_make(order);
	if (order) {
		order->ordertype = type;
		order->ordering = ordering;
		switch (type) {
		case SQL_ORDER_NUMBER:
			order->orderby.number = value;
			break;
		case SQL_ORDER_COLUMN:
			order->orderby.column = (sql_column_t *)value;
			break;
		}
	}
	return order;
}

void sql_order_free(sql_order_t *order)
{
	if (order) {
		switch (order->ordertype) {
		case SQL_ORDER_COLUMN:
			sql_column_free(order->orderby.column);
			break;
		}
		sql_node_free((sql_node_t *)order);
	}
}

char *sql_order_dump(sql_order_t *order)
{
	char *res = NULL, *tmp = NULL;
	char buf[20];
	
	if (order) {
		switch (order->ordertype) {
		case SQL_ORDER_COLUMN:
			res = sql_string_append(res, tmp = sql_column_dump(order->orderby.column));
			break;
		case SQL_ORDER_NUMBER:
			sprintf(buf, "%d", order->orderby.number);
			res = sql_string_append(res, buf);
			break;
		}
		if (order->ordering == SQL_ORDER_ASC)
			res = sql_string_append(res, " ASC");
		if (order->ordering == SQL_ORDER_DESC)
			res = sql_string_append(res, " DESC");
	}
	sql_string_free(tmp);
	return res;
}

char *sql_order_list_dump(vector_t *orders)
{
	char *res = NULL, *tmp = NULL;
	int count, index;
	if (orders) {
		count = element_count(orders);
		if (count > 0) {
			for (index = 0; index < count; index++) {
				res = sql_string_append(res, tmp = sql_order_dump((sql_order_t *)get_element(orders, index)));
				if (index != (count-1))
					res = sql_string_append(res, ", ");
				sql_string_free(tmp);
			}
		}
	}
	return res;
}

sql_func_t *sql_func_new(int type, vector_t *args)
{
	sql_func_t *func = sql_node_make(func);
	if (func) {
		func->functype = type;
		func->args = args;
	}
	return func;
}

void sql_func_free(sql_func_t *func)
{
	if (func) {
		if (func->args)
			destroy_vector(func->args);
		sql_node_free((sql_node_t *)func);
	}
}

char *sql_func_dump(sql_func_t *func)
{
	return NULL;
}

sql_constr_t *sql_constr_new(int key, vector_t *columns, sql_refer_t *refer)
{
	sql_constr_t *constr = sql_node_make(constr);
	if (constr) {
		constr->keytype = key;
		switch (key) {
		case SQL_KEY_UNIQUE:
		case SQL_KEY_PRIMARY:
			constr->columns = columns;
			break;
		case SQL_KEY_FOREIGN:
			constr->columns = columns;
			constr->refer = refer;
			break;
		}
	}
	return constr;
}

char *sql_select_stmt_dump(sql_select_stmt_t *stmt)
{
	char *res = NULL, *tmp = NULL;
	assert(stmt);

	if (stmt->selection && stmt->from) {
		res = sql_string_append(res, "SELECT ");
		if (stmt->option & SQL_OPT_ALL)
			res = sql_string_append(res, "ALL ");
		if (stmt->option & SQL_OPT_DISTINCT)
			res = sql_string_append(res, "DISTINCT");
		res = sql_string_append(res, tmp = sql_expr_list_dump(stmt->selection, 0));
		sql_string_free(tmp);
		res = sql_string_append(res, " FROM ");
		res = sql_string_append(res, tmp = sql_table_list_dump(stmt->from));
		sql_string_free(tmp);
		if (stmt->where) {
			res = sql_string_append(res, " WHERE ");
			res = sql_string_append(res, tmp = sql_expr_dump(stmt->where));
			sql_string_free(tmp);
		}
		if (stmt->group_by) {
			res = sql_string_append(res, " GROUP BY ");
			res = sql_string_append(res, tmp = sql_column_list_dump(stmt->group_by));
			sql_string_free(tmp);
		}
		if (stmt->having) {
			res = sql_string_append(res, " HAVING ");
			res = sql_string_append(res, tmp = sql_expr_dump(stmt->having));
			sql_string_free(tmp);
		}
		if (stmt->order_by) {
			res = sql_string_append(res, " ORDER BY ");
			res = sql_string_append(res, tmp = sql_order_list_dump(stmt->order_by));
			sql_string_free(tmp);
		}
	}
	return res;
}

void sql_select_stmt_free(sql_select_stmt_t *stmt)
{
	if (stmt) {
		if (stmt->selection)
			destroy_vector(stmt->selection);
		if (stmt->from)
			destroy_vector(stmt->from);
		if (stmt->group_by)
			destroy_vector(stmt->group_by);
		if (stmt->order_by)
			destroy_vector(stmt->order_by);
		if (stmt->where)
			sql_expr_free(stmt->where);
		if (stmt->having)
			sql_expr_free(stmt->having);
		sql_node_free((sql_node_t *)stmt);
	}
}

char *sql_create_stmt_dump(sql_create_stmt_t *stmt)
{
	char *res = NULL, *tmp = NULL;
	assert(stmt);

	res = sql_string_append(res, "CREATE TABLE ");
	res = sql_string_append(res, tmp = sql_table_dump(stmt->table));
	sql_string_free(tmp);
	res = sql_string_append(res, " ");
	res = sql_string_append(res, tmp = sql_column_def_dump(stmt->columns));
	sql_string_free(tmp);

	return res;
}

void sql_create_stmt_free(sql_create_stmt_t *stmt)
{
	if (stmt) {
		if (stmt->table)
			sql_table_free(stmt->table);
		if (stmt->columns)
			destroy_vector(stmt->columns);
		if (stmt->constrs)
			destroy_vector(stmt->constrs);
		sql_node_free((sql_node_t *)stmt);
	}
}

char *sql_drop_stmt_dump(sql_drop_stmt_t *stmt)
{
	char *res = NULL, *tmp = NULL;
	assert(stmt);

	res = sql_string_append(res, "DROP TABLE ");
	res = sql_string_append(res, tmp = sql_table_dump(stmt->table));
	sql_string_free(tmp);
	if (stmt->behavior == SQL_DROP_CASCADE)
		res = sql_string_append(res, " CASCADE");
	if (stmt->behavior == SQL_DROP_RESTRICT)
		res = sql_string_append(res, " RESTRICT");

	return res;
}

void sql_drop_stmt_free(sql_drop_stmt_t *stmt)
{
	if (stmt) {
		if (stmt->table)
			sql_table_free(stmt->table);
		sql_node_free((sql_node_t *)stmt);
	}
}

char *sql_update_stmt_dump(sql_update_stmt_t *stmt)
{
	char *res = NULL, *tmp = NULL;
	assert(stmt);

	res = sql_string_append(res, "UPDATE ");
	res = sql_string_append(res, tmp = sql_table_dump(stmt->table));
	sql_string_free(tmp);
	res = sql_string_append(res, " SET ");
	res = sql_string_append(res, tmp = sql_assign_list_dump(stmt->assigns, 0));
	sql_string_free(tmp);
	if (stmt->where) {
		res = sql_string_append(res, " WHERE ");
		res = sql_string_append(res, tmp = sql_expr_dump(stmt->where));
		sql_string_free(tmp);
	}
	return res;
}

void sql_update_stmt_free(sql_update_stmt_t *stmt)
{
	if (stmt) {
		if (stmt->assigns)
			destroy_vector(stmt->assigns);
		if (stmt->table)
			sql_table_free(stmt->table);
		if (stmt->where)
			sql_expr_free(stmt->where);
		sql_node_free((sql_node_t *)stmt);
	}
}

char *sql_insert_stmt_dump(sql_insert_stmt_t *stmt)
{
	char *res = NULL, *tmp = NULL;

	assert(stmt);

	if (stmt->table && stmt->table && stmt->values) {
		res = strdup("INSERT INTO ");
		res = sql_string_append(res, tmp = sql_table_dump(stmt->table));
		sql_string_free(tmp);
		res = sql_string_append(res, " ");
		res = sql_string_append(res, tmp = sql_column_list_dump(stmt->columns));
		sql_string_free(tmp);
		res = sql_string_append(res, " VALUES (");
		res = sql_string_append(res, tmp = sql_atom_list_dump(stmt->values, 0));
		sql_string_free(tmp);
		res = sql_string_append(res, ")");
	}
	return res;
}

void sql_insert_stmt_free(sql_insert_stmt_t *stmt)
{
	if (stmt) {
		if (stmt->columns)
			destroy_vector(stmt->columns);
		if (stmt->query)
			sql_select_stmt_free(stmt->query);
		if (stmt->table)
			sql_table_free(stmt->table);
		if (stmt->values)
			destroy_vector(stmt->values);
		sql_node_free((sql_node_t *)stmt);
	}
}

char *sql_delete_stmt_dump(sql_delete_stmt_t *stmt)
{
	char *res = NULL, *tmp = NULL;
	assert(stmt);

	res = sql_string_append(res, "DELETE FROM ");
	res = sql_string_append(res, tmp = sql_table_dump(stmt->table));
	sql_string_free(tmp);
	if (stmt->where) {
		res = sql_string_append(res, " WHERE ");
		res = sql_string_append(res, tmp = sql_expr_dump(stmt->where));
		sql_string_free(tmp);
	}
	return res;
}

void sql_delete_stmt_free(sql_delete_stmt_t *stmt)
{
	if (stmt) {
		if (stmt->table)
			sql_table_free(stmt->table);
		if (stmt->where)
			sql_expr_free(stmt->where);
		sql_node_free((sql_node_t *)stmt);
	}
}

void sql_create_stmt_fixup(sql_create_stmt_t *stmt)
{
	if (stmt) {
		assert(!stmt->constrs);
		/* find constrs in the columns */
	}
}

sql_node_t *sql_statement_new(int type)
{
	sql_normal_stmt_t *stmt;

	switch (type) {
	case SQL_STATEMENT_ROLLBACK:
		stmt = (sql_normal_stmt_t *)sql_node_make(normal_stmt);
		stmt->stmttype = SQL_STATEMENT_ROLLBACK;
		return (sql_node_t *)stmt;
	case SQL_STATEMENT_COMMIT:
		stmt = (sql_normal_stmt_t *)sql_node_make(normal_stmt);
		stmt->stmttype = SQL_STATEMENT_COMMIT;
		return (sql_node_t *)stmt;
	case SQL_STATEMENT_INSERT:
		stmt = (sql_normal_stmt_t *)sql_node_make(insert_stmt);
		stmt->stmttype = SQL_STATEMENT_INSERT;
		return (sql_node_t *)stmt;
	case SQL_STATEMENT_DELETE:
		stmt = (sql_normal_stmt_t *)sql_node_make(delete_stmt);
		stmt->stmttype = SQL_STATEMENT_DELETE;
		return (sql_node_t *)stmt;
	case SQL_STATEMENT_UPDATE:
		stmt = (sql_normal_stmt_t *)sql_node_make(update_stmt);
		stmt->stmttype = SQL_STATEMENT_UPDATE;
		return (sql_node_t *)stmt;
	case SQL_STATEMENT_SELECT:
		stmt = (sql_normal_stmt_t *)sql_node_make(select_stmt);
		stmt->stmttype = SQL_STATEMENT_SELECT;
		return (sql_node_t *)stmt;
	case SQL_STATEMENT_CREATE:
		stmt = (sql_normal_stmt_t *)sql_node_make(create_stmt);
		stmt->stmttype = SQL_STATEMENT_CREATE;
		return (sql_node_t *)stmt;
	case SQL_STATEMENT_DROP:
		stmt = (sql_normal_stmt_t *)sql_node_make(drop_stmt);
		stmt->stmttype = SQL_STATEMENT_DROP;
		return (sql_node_t *)stmt;
	}
	return NULL;
}

void sql_statement_free(sql_normal_stmt_t *stmt)
{
	if (stmt) {
		switch (stmt->stmttype) {
		case SQL_STATEMENT_INSERT:
			assert(stmt->parent.type == T_insert_stmt);
			sql_insert_stmt_free((sql_insert_stmt_t *)stmt);
			break;
		case SQL_STATEMENT_UPDATE:
			assert(stmt->parent.type == T_update_stmt);
			sql_update_stmt_free((sql_update_stmt_t *)stmt);
			break;
		case SQL_STATEMENT_DELETE:
			assert(stmt->parent.type == T_delete_stmt);
			sql_delete_stmt_free((sql_delete_stmt_t *)stmt);
			break;
		case SQL_STATEMENT_SELECT:
			assert(stmt->parent.type == T_select_stmt);
			sql_select_stmt_free((sql_select_stmt_t *)stmt);
			break;
		}
	}
}

char *sql_statement_dump(sql_normal_stmt_t *stmt)
{
	if (stmt) {
		switch (stmt->stmttype) {
		case SQL_STATEMENT_INSERT:
			return sql_insert_stmt_dump((sql_insert_stmt_t *)stmt);
		case SQL_STATEMENT_UPDATE:
			return sql_update_stmt_dump((sql_update_stmt_t *)stmt);
		case SQL_STATEMENT_DELETE:
			return sql_delete_stmt_dump((sql_delete_stmt_t *)stmt);
		case SQL_STATEMENT_SELECT:
			return sql_select_stmt_dump((sql_select_stmt_t *)stmt);
		case SQL_STATEMENT_CREATE:
			return sql_create_stmt_dump((sql_create_stmt_t *)stmt);
		case SQL_STATEMENT_DROP:
			return sql_drop_stmt_dump((sql_drop_stmt_t *)stmt);
		}
	}
	return NULL;
}

char *sql_string_append(char *old_str, const char *str)
{
	int old_size;
	int new_size;
	char *text;
	int size = strlen(str);
	
	if (old_str) {
		old_size = strlen(old_str);
		new_size = old_size + size + 1;
		text = realloc(old_str, new_size);
		memcpy(text + old_size, str, size);
		text[new_size-1] = 0;
	} else {
		text = strdup(str);
	}
	return text;
}

char *sql_string_escape(const char *str)
{
	int index = 0;
	int len = 0;
	char *res = NULL;
	int outlen = 0;
	
	len = strlen(str);
	res = (char *)malloc(sizeof(char)*len*2+3);
	if (!res) return NULL;
	
	res[outlen++] = '\'';
	while (index < len) {
		unsigned char c = *(str+index);
		index++;
		switch (c) {
		case '\'':
			res[outlen++] = '\'';
			res[outlen++] = '\'';
			break;
		default:
			res[outlen++] = c;
			break;
		}
	}
	res[outlen++] = '\'';
	res[outlen] = 0;
	
	return res;
}
