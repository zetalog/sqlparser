%{
#include <stdlib.h>
#include <string.h>
#include "sql_priv.h"

extern void *alloca(size_t);
extern void sql_error(char *);
extern int sql_lex(void);
%}

/* symbolic tokens */
%union {
	int intval;
	double floatval;
	char *strval;
	int subtok;
	sql_table_t *table;
	sql_type_t *type;
	sql_atom_t *atom;
	sql_expr_t *expr;
	sql_column_t *column;
	sql_assign_t *assign;
	sql_node_t *node;
	sql_refer_t *refer;
	sql_func_t *function;
	sql_option_t *option;
	sql_constr_t *constraint;
	sql_order_t *order;
	vector_t *vector;
}
	
%token <strval> NAME STRING
%token <intval> INTNUM
%token <floatval> APPROXNUM

/* operators */
%left OR
%left AND
%left NOT
%left EQ NEQ LT GT LTEQ GTEQ
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

/* literal keyword tokens */
%token ADMIN ALL AMMSC AS ASC AUTHORIZATION AUTO_INCREMENT BETWEEN
%token BIGINT BINARY BY CASCADE CHARACTER CHECK COMMIT CREATE
%token DATETIME DECIMAL DEFAULT DELETE DESC
%token DISTINCT DOUBLE DROP ESCAPE FLOAT FOREIGN
%token FROM GRANT GROUP HAVING IN INDEX INSERT
%token INTEGER INTO IS KEY LIKE NULLX NUMERIC ON
%token OPTION ORDER RESTRICT PRECISION PRIMARY PRIVILEGES
%token PUBLIC REAL REFERENCES ROLLBACK SCHEMA SELECT SET
%token SMALLINT TABLE TIMESTAMP TO
%token UNIQUE UNSIGNED UPDATE USER VALUES VARCHAR VIEW WHERE
%token WITH WORK

%type <strval> user range_variable grantee
%type <intval> opt_all_distinct opt_with_check_option opt_asc_desc drop_behavior
%type <type> data_type
%type <table> table table_ref
%type <atom> literal atom insert_atom
%type <column> column_ref column column_def
%type <assign> assignment
%type <order> ordering_spec
%type <option> column_def_opt
%type <function> function_ref
%type <constraint> table_constraint_def
%type <vector> column_list opt_column_list, atom_list
	       insert_atom_list table_ref_list from_clause
	       grantee_list scalar_exp_list column_ref_list
	       opt_group_by_clause assignment_list selection
	       table_element_list column_def_opt_list
	       opt_schema_element_list schema_element_list
	       ordering_spec_list opt_order_by_clause
%type <expr> scalar_exp search_condition
	     predicate test_for_null between_predicate
	     comparison_predicate like_predicate in_predicate
	     opt_where_clause where_clause opt_having_clause
%type <node> manipulative_stmt sql sql_list
	     select_stmt rollback_stmt commit_stmt
	     delete_stmt update_stmt insert_stmt
	     table_def view_def grant_def schema_def
	     schema_element table_element

%%

sql_list:
		sql ';'					{ sql_parse_end(); }
	|	sql_list sql ';'			{ sql_parse_end(); }
	;

sql:
		schema_def
	|	table_def				    { sql_node_insert($1); }
	|	view_def
	|	grant_def
	|	manipulative_stmt			{ sql_node_insert($1); }
	;

/* schema definition language */
schema_def:
		CREATE SCHEMA AUTHORIZATION user opt_schema_element_list
	;

opt_schema_element_list:
		/* empty */				{ $$ = NULL; }
	|	schema_element_list
	;

schema_element_list:
		schema_element				{ $$ = sql_vector_new((DESTROYER)sql_statement_free, NULL, $1); }
	|	schema_element_list schema_element	{ $$ = sql_vector_add($1, $2); }
	;

schema_element:
		table_def
	|	view_def
	|	grant_def
	;

table_def:
		CREATE TABLE table '(' table_element_list ')'
{
	sql_create_stmt_t *stmt;
	$$ = sql_statement_new(SQL_STATEMENT_CREATE);
	stmt = (sql_create_stmt_t *)$$;
	stmt->table = $3;
	stmt->columns = $5;
	sql_create_stmt_fixup((sql_create_stmt_t *)$$);
}
	|	DROP TABLE table drop_behavior
{
	sql_drop_stmt_t *stmt;
	$$ = sql_statement_new(SQL_STATEMENT_DROP);
	stmt = (sql_drop_stmt_t *)$$;
	stmt->table = $3;
	stmt->behavior = $4;
}
	|	ALTER TABLE table alter_table_action
{
	sql_alter_stmt_t *stmt;
	$$ = sql_statement_new(SQL_STATEMENT_ALTER);
	stmt = (sql_alter_stmt_t *)$$;
}

	;

table_element_list:
		table_element				{ $$ = sql_vector_new((DESTROYER)sql_column_free, NULL, $1); }
	|	table_element_list ',' table_element	{ $$ = sql_vector_add($1, $3); }
	;

table_element:
		column_def				{ $$ = (sql_node_t *)$1; }
	|	table_constraint_def			{ $$ = (sql_node_t *)$1; }
	;

column_def:
		column data_type column_def_opt_list
{
	$$ = $1;
	$$->datatype = $2;
	$$->options = $3;
}
	;

column_def_opt_list:
		/* empty */				{ $$ = sql_vector_empty((DESTROYER)sql_option_free, NULL); }
	|	column_def_opt_list column_def_opt	{ $$ = sql_vector_add($1, $2); }
	;

column_def_opt:
		AUTO_INCREMENT				{ $$ = sql_option_new(SQL_OPT_AUTO_INCREMENT, 0); }
	|	NOT NULLX				{ $$ = sql_option_new(SQL_OPT_KEY, SQL_KEY_NOTNULL); }
	|	NOT NULLX UNIQUE			{ $$ = sql_option_new(SQL_OPT_KEY, SQL_KEY_UNIQUE); }
	|	NOT NULLX PRIMARY KEY			{ $$ = sql_option_new(SQL_OPT_KEY, SQL_KEY_PRIMARY); }
	|	DEFAULT literal
{
	$$ = sql_option_new(SQL_OPT_DEFAULT, (sql_param_t)($2));
}
	|	DEFAULT NULLX
{
	$$ = sql_option_new(SQL_OPT_DEFAULT, (sql_param_t)(sql_atom_new(SQL_ATOM_NULL, 0)));
}
	|	DEFAULT USER
{
	$$ = sql_option_new(SQL_OPT_DEFAULT, (sql_param_t)(sql_atom_new(SQL_ATOM_USER, 0)));
}
	|	CHECK '(' search_condition ')'
{
	$$ = sql_option_new(SQL_OPT_CHECK, (sql_param_t)$3);
}
	|	REFERENCES table
{
	$$ = sql_option_new(SQL_OPT_REFERENCE, (sql_param_t)(sql_refer_new($2, NULL)));
}
	|	REFERENCES table '(' column_list ')'
{
	$$ = sql_option_new(SQL_OPT_REFERENCE, (sql_param_t)(sql_refer_new($2, $4)));
}
	;

table_constraint_def:
		UNIQUE '(' column_list ')'		{ $$ = sql_constr_new(SQL_KEY_UNIQUE, $3, NULL); }
	|	PRIMARY KEY '(' column_list ')'		{ $$ = sql_constr_new(SQL_KEY_PRIMARY, $4, NULL); }
	|	FOREIGN KEY '(' column_list ')'
		REFERENCES table
{
	$$ = sql_constr_new(SQL_KEY_PRIMARY, $4, sql_refer_new($7, NULL));
}
	|	FOREIGN KEY '(' column_list ')'
		REFERENCES table '(' column_list ')'
{
	$$ = sql_constr_new(SQL_KEY_PRIMARY, $4, sql_refer_new($7, $9));
}
	|	KEY column '(' column_ref_list ')'	{ /* MySQL extension, I don't know what it is */ }
	|	INDEX column '(' column_ref_list ')'	{ /* MySQL extension, I don't know what it is */ }
	;

column_list:
		column					{ $$ = sql_vector_new((DESTROYER)sql_column_free, NULL, $1); }
	|	column_list ',' column			{ $$ = sql_vector_add($1, $3); }
	;

drop_behavior:
		CASCADE				{ $$ = SQL_DROP_CASCADE; }
	|	RESTRICT			{ $$ = SQL_DROP_RESTRICT; }
	;


view_def:
		CREATE VIEW table opt_column_list
		AS select_stmt opt_with_check_option
	;

opt_with_check_option:
		/* empty */				{ $$ = 0; }
	|	WITH CHECK OPTION			{ $$ = 1; }
	;

opt_column_list:
		/* empty */				{ $$ = NULL; }
	|	'(' column_list ')'			{ $$ = $2; }
	;

grant_def:
		GRANT privileges ON table TO grantee_list
		opt_with_grant_option
	;

opt_with_grant_option:
		/* empty */
	|	WITH GRANT OPTION
	|	WITH ADMIN OPTION
	;

privileges:
		ALL PRIVILEGES
	|	ALL
	|	operation_list
	;

operation_list:
		operation
	|	operation_list ',' operation
	;

operation:
		SELECT
	|	INSERT
	|	DELETE
	|	UPDATE opt_column_list
	|	REFERENCES opt_column_list
	;

grantee_list:
		grantee					{ $$ = sql_vector_new((DESTROYER)free, (COMPARER)strcmp, $1); }
	|	grantee_list ',' grantee		{ $$ = sql_vector_add($1, $3); }
	;

grantee:
		PUBLIC					{ $$ = "PUBLIC"; }
	|	user					{ $$ = $1; }
	;

opt_order_by_clause:
		/* empty */				{ $$ = NULL; }
	|	ORDER BY ordering_spec_list		{ $$ = $3; }
	;

ordering_spec_list:
		ordering_spec				{ $$ = sql_vector_new((DESTROYER)sql_order_free, NULL, $1); }
	|	ordering_spec_list ',' ordering_spec	{ $$ = sql_vector_add($1, $3); }
	;

ordering_spec:
		INTNUM opt_asc_desc			{ $$ = sql_order_new(SQL_ORDER_NUMBER, $2, (sql_param_t)$1); }
	|	column_ref opt_asc_desc			{ $$ = sql_order_new(SQL_ORDER_COLUMN, $2, (sql_param_t)$1); }
	;

opt_asc_desc:
		/* empty */				{ $$ = SQL_ORDER_NONE; }
	|	ASC					{ $$ = SQL_ORDER_ASC; }
	|	DESC					{ $$ = SQL_ORDER_DESC; }
	;

/* manipulative statements */
manipulative_stmt:
		commit_stmt
	|	rollback_stmt
	|	insert_stmt
	|	update_stmt
	|	delete_stmt
	|	select_stmt
	;

commit_stmt:
		COMMIT WORK				{ $$ = sql_statement_new(SQL_STATEMENT_COMMIT); }
	;

delete_stmt:
		DELETE FROM table opt_where_clause
{
	sql_delete_stmt_t *stmt;
	$$ = sql_statement_new(SQL_STATEMENT_DELETE);
	stmt = (sql_delete_stmt_t *)$$;
	stmt->table = $3;
	stmt->where = $4;
}
	;

insert_stmt:
		INSERT INTO table opt_column_list VALUES '(' insert_atom_list ')'
{
	sql_insert_stmt_t *stmt;
	$$ = sql_statement_new(SQL_STATEMENT_INSERT);
	stmt = (sql_insert_stmt_t *)$$;
	stmt->table = $3;
	stmt->columns = $4;
	stmt->values = $7;
}
	|	INSERT INTO table opt_column_list select_stmt
{
	sql_insert_stmt_t *stmt;
	$$ = sql_statement_new(SQL_STATEMENT_INSERT);
	stmt = (sql_insert_stmt_t *)$$;
	stmt->table = $3;
	stmt->columns = $4;
	stmt->query = (sql_select_stmt_t *)$5;
}
	;

insert_atom_list:
		insert_atom				{ $$ = sql_vector_new((DESTROYER)sql_atom_free, NULL, $1); }
	|	insert_atom_list ',' insert_atom	{ $$ = sql_vector_add($1, $3); }
	;

insert_atom:
		atom
	|	NULLX					{ $$ = sql_atom_new(SQL_ATOM_NULL, 0); }
	;

rollback_stmt:
		ROLLBACK WORK				{ $$ = sql_statement_new(SQL_STATEMENT_ROLLBACK); }
	;

select_stmt:
		SELECT opt_all_distinct selection from_clause
		opt_where_clause opt_group_by_clause
		opt_having_clause opt_order_by_clause
{
	sql_select_stmt_t *stmt;
	$$ = sql_statement_new(SQL_STATEMENT_SELECT);
	stmt = (sql_select_stmt_t *)$$;
	stmt->option = $2;
	stmt->selection = $3;
	stmt->from = $4;
	stmt->where = $5;
	stmt->group_by = $6;
	stmt->having = $7;
	stmt->order_by = $8;
}
	;

opt_all_distinct:
		/* empty */				{ $$ = 0; }
	|	ALL					{ $$ |= SQL_OPT_ALL; }
	|	DISTINCT				{ $$ |= SQL_OPT_DISTINCT; }
	;

assignment_list:
		/* empty */				{ $$ = NULL; }
	|	assignment				{ $$ = sql_vector_new((DESTROYER)sql_assign_free, NULL, $1); }
	|	assignment_list ',' assignment		{ $$ = sql_vector_add($1, $3); }
	;

assignment:
		column EQ scalar_exp			{ $$ = sql_assign_new($1, $3); }
	|	column EQ NULLX				{ $$ = sql_assign_new($1, NULL); }
	;

update_stmt:
		UPDATE table SET assignment_list opt_where_clause
{
	sql_update_stmt_t *stmt;
	$$ = sql_statement_new(SQL_STATEMENT_UPDATE);
	stmt = (sql_update_stmt_t *)$$;
	stmt->table = $2;
	stmt->assigns = $4;
	stmt->where = $5;
}
	;

opt_where_clause:
		/* empty */				{ $$ = NULL; }
	|	where_clause
	;

/* query expressions */
selection:
		scalar_exp_list
	|	'*'					{ $$ = NULL; }
	;

from_clause:
		FROM table_ref_list			{ $$ = $2; }
	;

table_ref_list:
		table_ref				{ $$ = sql_vector_new((DESTROYER)sql_table_free, NULL, $1); }
	|	table_ref_list ',' table_ref		{ $$ = sql_vector_add($1, $3); }
	;

table_ref:
		table
	|	table range_variable			{ $1->rangevar = $2; $$ = $1; }
	;

where_clause:
		WHERE search_condition			{ $$ = $2; }
	;

opt_group_by_clause:
		/* empty */				{ $$ = NULL; }
	|	GROUP BY column_ref_list		{ $$ = $3; }
	;

column_ref_list:
		column_ref				{ $$ = sql_vector_new((DESTROYER)sql_column_free, NULL, $1); }
	|	column_ref_list ',' column_ref		{ $$ = sql_vector_add($1, $3); }
	;

opt_having_clause:
		/* empty */				{ $$ = NULL; }
	|	HAVING search_condition			{ $$ = $2; }
	;

/* search conditions */
search_condition:
		/* empty */				{ $$ = NULL; }
	|	search_condition OR search_condition	{ $$ = sql_expr_new(SQL_EXPR_OR, $1, $3); }
	|	search_condition AND search_condition	{ $$ = sql_expr_new(SQL_EXPR_AND, $1, $3); }
	|	NOT search_condition			{ $$ = sql_expr_new(SQL_EXPR_NOT, NULL, $2); }
	|	'(' search_condition ')'		{ $$ = $2; }
	|	predicate
	;

predicate:
		comparison_predicate
	|	between_predicate
	|	like_predicate
	|	test_for_null
	|	in_predicate
	;

comparison_predicate:
		scalar_exp EQ scalar_exp		{ $$ = sql_expr_new(SQL_EXPR_EQ, $1, $3); }
	|	scalar_exp NEQ scalar_exp		{ $$ = sql_expr_new(SQL_EXPR_NEQ, $1, $3); }
	|	scalar_exp LT scalar_exp		{ $$ = sql_expr_new(SQL_EXPR_LT, $1, $3); }
	|	scalar_exp GT scalar_exp		{ $$ = sql_expr_new(SQL_EXPR_GT, $1, $3); }
	|	scalar_exp LTEQ scalar_exp		{ $$ = sql_expr_new(SQL_EXPR_LTEQ, $1, $3); }
	|	scalar_exp GTEQ scalar_exp		{ $$ = sql_expr_new(SQL_EXPR_GTEQ, $1, $3); }
	;

between_predicate:
		scalar_exp NOT BETWEEN scalar_exp AND scalar_exp
{
	$$ = sql_expr_new(SQL_EXPR_AND,
			  sql_expr_new(SQL_EXPR_LT, $1, $4),
			  sql_expr_new(SQL_EXPR_GT, $1, $6));
}
	|	scalar_exp BETWEEN scalar_exp AND scalar_exp
{
	$$ = sql_expr_new(SQL_EXPR_AND,
			  sql_expr_new(SQL_EXPR_GTEQ, $1, $3),
			  sql_expr_new(SQL_EXPR_LTEQ, $1, $5));
}
	;

like_predicate:
		scalar_exp NOT LIKE atom		{ $$ = sql_expr_new(SQL_EXPR_UNLIKE, $1, sql_expr_atom($4)); }
	|	scalar_exp LIKE atom			{ $$ = sql_expr_new(SQL_EXPR_LIKE, $1, sql_expr_atom($3)); }
	|	scalar_exp NOT LIKE atom ESCAPE atom
{
	$$ = sql_expr_new(SQL_EXPR_AND,
			  sql_expr_new(SQL_EXPR_UNLIKE, $1, sql_expr_atom($4)),
			  sql_expr_new(SQL_EXPR_LIKE, NULL, sql_expr_atom($6)));
}
	|	scalar_exp LIKE atom ESCAPE atom
{
	$$ = sql_expr_new(SQL_EXPR_AND,
			  sql_expr_new(SQL_EXPR_LIKE, $1, sql_expr_atom($3)),
			  sql_expr_new(SQL_EXPR_UNLIKE, NULL, sql_expr_atom($5)));
}
	;

test_for_null:
		column_ref IS NOT NULLX			{ $$ = sql_expr_column($1); $$->notnull = 1; }
	|	column_ref IS NULLX			{ $$ = sql_expr_column($1); $$->notnull = 0; }
	;

in_predicate:
		scalar_exp NOT IN '(' atom_list ')'	{ $$ = sql_expr_new(SQL_EXPR_NOTIN, $1, sql_expr_list($5)); }
	|	scalar_exp IN '(' atom_list ')'		{ $$ = sql_expr_new(SQL_EXPR_IN, $1, sql_expr_list($4)); }
	;

atom_list:
		atom					{ $$ = sql_vector_new((DESTROYER)sql_atom_free, NULL, $1); }
	|	atom_list ',' atom			{ $$ = sql_vector_add($1, $3); }
	;

/* scalar expressions */
scalar_exp:
		scalar_exp '+' scalar_exp		{ $$ = sql_expr_new(SQL_EXPR_PLUS, $1, $3); }
	|	scalar_exp '-' scalar_exp		{ $$ = sql_expr_new(SQL_EXPR_MINUS, $1, $3); }
	|	scalar_exp '*' scalar_exp		{ $$ = sql_expr_new(SQL_EXPR_MULTIPLY, $1, $3); }
	|	scalar_exp '/' scalar_exp		{ $$ = sql_expr_new(SQL_EXPR_DIVIDE, $1, $3); }
	|	'+' scalar_exp %prec UMINUS		{ $$ = sql_expr_new(SQL_EXPR_PLUS, NULL, $2); }
	|	'-' scalar_exp %prec UMINUS		{ $$ = sql_expr_new(SQL_EXPR_MINUS, NULL, $2); }
	|	atom					{ $$ = sql_expr_atom($1); }
	|	column_ref				{ $$ = sql_expr_column($1); }
	|	function_ref				{ $$ = sql_expr_func($1); }
	|	'(' scalar_exp ')'			{ $$ = $2; }
	;

scalar_exp_list:
		scalar_exp				{ $$ = sql_vector_new((DESTROYER)sql_expr_free, NULL, $1); }
	|	scalar_exp_list ',' scalar_exp		{ $$ = sql_vector_add($1, $3); }
	;

atom:
		literal
	|	USER					{ $$ = sql_atom_new(SQL_ATOM_USER, 0); }
	;

function_ref:
		AMMSC '(' '*' ')'
{
	$$ = sql_func_new(yylval.intval, NULL);
	$$->star = 1;
	$$->distinct = 0;
}
	|	AMMSC '(' DISTINCT column_ref ')'
{
	$$ = sql_func_new(yylval.intval, sql_vector_new((DESTROYER)sql_expr_free, NULL, sql_expr_column($4)));
	$$->star = 0;
	$$->distinct = 1;
}
	|	AMMSC '(' ALL scalar_exp ')'
{
	$$ = sql_func_new(yylval.intval, sql_vector_new((DESTROYER)sql_expr_free, NULL, sql_expr_expr($4)));
	$$->star = 0;
	$$->distinct = 0;
}
	|	AMMSC '(' scalar_exp ')'
{
	$$ = sql_func_new(yylval.intval, sql_vector_new((DESTROYER)sql_expr_free, NULL, sql_expr_expr($3)));
	$$->star = 0;
	$$->distinct = 0;
}
	;

literal:
		STRING					{ $$ = sql_atom_new(SQL_ATOM_STRING, (sql_param_t)$1); }
	|	INTNUM					{ $$ = sql_atom_new(SQL_ATOM_INTEGER, (sql_param_t)$1); }
	|	APPROXNUM				{ $$ = sql_atom_new(SQL_ATOM_FLOAT, (sql_param_t)$1); }
	;

/* miscellaneous */
table:
		NAME					{ $$ = sql_table_new(NULL, $1); }
	|	NAME '.' NAME				{ $$ = sql_table_new($1, $3); }
	;

column_ref:
		NAME					{ $$ = sql_column_new(NULL, NULL, $1, 0); }
	|	NAME '(' INTNUM ')'			{ $$ = sql_column_new(NULL, NULL, $1, $3); }
	|	NAME '.' NAME				{ $$ = sql_column_new(NULL, $1, $3, 0); }
	|	NAME '.' NAME '(' INTNUM ')'		{ $$ = sql_column_new(NULL, $1, $3, $5); }
	|	NAME '.' NAME '.' NAME			{ $$ = sql_column_new($1, $3, $5, 0); }
	|	NAME '.' NAME '.' NAME '(' INTNUM ')'	{ $$ = sql_column_new($1, $3, $5, $7); }
	;

/* data types */
data_type:
		CHARACTER				{ $$ = sql_type_new(CHARACTER); }
	|	CHARACTER BINARY			{ $$ = sql_type_new(CHARACTER); }
	|	CHARACTER '(' INTNUM ')'		{ $$ = sql_type_new(CHARACTER); $$->length = $3; }
	|	CHARACTER '(' INTNUM ')' BINARY		{ $$ = sql_type_new(CHARACTER); $$->length = $3; }
	|	VARCHAR					{ $$ = sql_type_new(VARCHAR); }
	|	VARCHAR BINARY				{ $$ = sql_type_new(VARCHAR); }
	|	VARCHAR '(' INTNUM ')'			{ $$ = sql_type_new(VARCHAR); $$->length = $3; }
	|	VARCHAR '(' INTNUM ')' BINARY		{ $$ = sql_type_new(VARCHAR); $$->length = $3; }
	|	NUMERIC					{ $$ = sql_type_new(NUMERIC); }
	|	NUMERIC '(' INTNUM ')'			{ $$ = sql_type_new(NUMERIC); $$->length = $3; }
	|	NUMERIC '(' INTNUM ',' INTNUM ')'	{ $$ = sql_type_new(NUMERIC); $$->length = $3; $$->decimal = $5; }
	|	DECIMAL					{ $$ = sql_type_new(DECIMAL); }
	|	DECIMAL '(' INTNUM ')'			{ $$ = sql_type_new(DECIMAL); $$->length = $3; }
	|	DECIMAL '(' INTNUM ',' INTNUM ')'	{ $$ = sql_type_new(DECIMAL); $$->length = $3; $$->decimal = $5; }
	|	INTEGER					{ $$ = sql_type_new(INTEGER); }
	|	INTEGER UNSIGNED			{ $$ = sql_type_new(INTEGER); }
	|	INTEGER '(' INTNUM ')'			{ $$ = sql_type_new(INTEGER); $$->length = $3; }
	|	INTEGER '(' INTNUM ')' UNSIGNED		{ $$ = sql_type_new(INTEGER); $$->length = $3; }
	|	SMALLINT				{ $$ = sql_type_new(SMALLINT); }
	|	SMALLINT UNSIGNED			{ $$ = sql_type_new(SMALLINT); }
	|	SMALLINT '(' INTNUM ')'			{ $$ = sql_type_new(BIGINT); $$->length = $3; }
	|	SMALLINT '(' INTNUM ')' UNSIGNED	{ $$ = sql_type_new(BIGINT); $$->length = $3; }
	|	BIGINT					{ $$ = sql_type_new(BIGINT); }
	|	BIGINT UNSIGNED				{ $$ = sql_type_new(BIGINT); }
	|	BIGINT '(' INTNUM ')'			{ $$ = sql_type_new(BIGINT); $$->length = $3; }
	|	BIGINT '(' INTNUM ')' UNSIGNED		{ $$ = sql_type_new(BIGINT); $$->length = $3; }
	|	FLOAT					{ $$ = sql_type_new(FLOAT); }
	|	FLOAT UNSIGNED				{ $$ = sql_type_new(FLOAT); }
	|	FLOAT '(' INTNUM ')'			{ $$ = sql_type_new(FLOAT); $$->length = $3; }
	|	FLOAT '(' INTNUM ')' UNSIGNED		{ $$ = sql_type_new(FLOAT); $$->length = $3; }
	|	REAL					{ $$ = sql_type_new(REAL); }
	|	DOUBLE PRECISION			{ $$ = sql_type_new(DOUBLE); }
	|	DATETIME				{ $$ = sql_type_new(DATETIME); }
	|	TIMESTAMP '(' INTNUM ')'		{ $$ = sql_type_new(TIMESTAMP); $$->length = $3; }
	;

/* the various things you can name */
column:
		NAME					{ $$ = sql_column_new(NULL, NULL, $1, 0); }
	;

range_variable:
		NAME
	;

user:
		NAME
	;
%%
