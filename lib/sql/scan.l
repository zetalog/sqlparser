%x SQL
%{
#include "sql_priv.h"
#include "sql_parse.h"
#include <stdlib.h>
#include <string.h>

#ifdef YYDEBUG
extern int sql_debug;
#endif

extern int sql_parse(void);

int lineno = 1;
void sql_error(char *s);

/* macro to save the text and return a token */
#define TOK(name) { sql_parse_save(yytext, yyleng); return name; }
%}

space		[ \n\t]
alpha		[A-Za-z]
num			[0-9]
alphanum	[A-Za-z0-9_]

%%

<INITIAL>{
	/* skip comments */
	[ \t]*#.*\n	lineno++;
	[ \t]*#.*

	/* skip empty lines */
	[ \t]*\n	lineno++;/* return EOL; */
	[ \t]+	{
		BEGIN(SQL);
		sql_parse_start();
	}
	.	{
		unput(yytext[0]);
		BEGIN(SQL);
		sql_parse_start();
	}
}

<SQL>{
	/* literal keyword tokens */
	ADMIN		TOK(ADMIN)
	ALL		TOK(ALL)
	AND		TOK(AND)
	[Aa][Uu][Tt][Oo]_[Ii][Nn][Cc][Rr][Ee][Mm][Ee][Nn][Tt]	TOK(AUTO_INCREMENT)
	AS		TOK(AS)
	ASC		TOK(ASC)
	AUTHORIZATION	TOK(AUTHORIZATION)
	BETWEEN		TOK(BETWEEN)
	[Bb][Ii][Gg][Ii][Nn][Tt]				TOK(BIGINT)
	[Bb][Ii][Nn][Aa][Rr][Yy]				TOK(BINARY)
	BY		TOK(BY)
	CASCADE	TOK(CASCADE)
	[Cc][Hh][Aa][Rr]([Aa][Cc][Tt][Ee][Rr])?			TOK(CHARACTER)
	CHECK		TOK(CHECK)
	COMMIT		TOK(COMMIT)
	CREATE		TOK(CREATE)
	[Dd][Aa][Tt][Ee][Tt][Ii][Mm][Ee]			TOK(DATETIME)
	DECIMAL		TOK(DECIMAL)
	[Dd][Ee][Ff][Aa][Uu][Ll][Tt]				TOK(DEFAULT)
	DELETE		TOK(DELETE)
	DESC		TOK(DESC)
	DISTINCT	TOK(DISTINCT)
	DOUBLE		TOK(DOUBLE)
	DROP		TOK(DROP)
	ESCAPE		TOK(ESCAPE)
	FLOAT		TOK(FLOAT)
	FOREIGN		TOK(FOREIGN)
	FROM		TOK(FROM)
	GRANT		TOK(GRANT)
	GROUP		TOK(GROUP)
	HAVING		TOK(HAVING)
	IN		TOK(IN)
	INDEX		TOK(INDEX)
	INSERT		TOK(INSERT)
	INT(EGER)?	TOK(INTEGER)
	int(eger)?	TOK(INTEGER)
	INTO		TOK(INTO)
	into		TOK(INTO)
	IS		TOK(IS)
	KEY		TOK(KEY)
	LIKE		TOK(LIKE)
	NOT		TOK(NOT)
	NULL		TOK(NULLX)
	NUMERIC		TOK(NUMERIC)
	ON		TOK(ON)
	OPTION		TOK(OPTION)
	OR		TOK(OR)
	ORDER		TOK(ORDER)
	RESTRICT	TOK(RESTRICT)
	PRECISION	TOK(PRECISION)
	PRIMARY		TOK(PRIMARY)
	PRIVILEGES	TOK(PRIVILEGES)
	PUBLIC		TOK(PUBLIC)
	REAL		TOK(REAL)
	REFERENCES	TOK(REFERENCES)
	ROLLBACK	TOK(ROLLBACK)
	SCHEMA		TOK(SCHEMA)
	SELECT		TOK(SELECT)
	SET		TOK(SET)
	SMALLINT	TOK(SMALLINT)
	TABLE		TOK(TABLE)
	[Tt][Ii][Mm][Ee][Ss][Tt][Aa][Mm][Pp]	TOK(TIMESTAMP)
	TO		TOK(TO)
	UNIQUE		TOK(UNIQUE)
	[Uu][Nn][Ss][Ii][Gg][Nn][Ee][Dd]	TOK(UNSIGNED)
	UPDATE		TOK(UPDATE)
	USER		TOK(USER)
	VALUES		TOK(VALUES)
	values		TOK(VALUES)
	[Vv][Aa][Rr][Cc][Hh][Aa][Rr]		TOK(VARCHAR)
	VIEW		TOK(VIEW)
	WHERE		TOK(WHERE)
	WITH		TOK(WITH)
	WORK		TOK(WORK)

	AVG		{ TOK(AMMSC); sql_lval.intval = SQL_FUNC_AVG; }
	MIN		TOK(AMMSC)
	MAX		TOK(AMMSC)
	SUM		TOK(AMMSC)
	COUNT		TOK(AMMSC)

	/* punctuation */
	"="		TOK(EQ)
	"<>"		TOK(NEQ)
	"<"		TOK(LT)
	">"		TOK(GT)
	"<="		TOK(LTEQ)
	">="		TOK(GTEQ)
	[-+*/(),.;]	TOK(yytext[0])

	/* names */
	{alpha}{alphanum}*	{
		sql_parse_save(yytext, yyleng);
		sql_lval.strval = sql_parse_token;
		TOK(NAME)
	}

	/* numbers */
	{num}+	|
	{num}+"."{num}* |
	"."{num}*	{
		sql_parse_save(yytext, yyleng);
		sql_lval.intval = atoi(sql_parse_token);
		TOK(INTNUM)
	}

	{num}+[eE][+-]?{num}+	|
	{num}+"."{num}*[eE][+-]?{num}+ |
	"."{num}*[eE][+-]?{num}+	{
		sql_parse_save(yytext, yyleng);
		sql_lval.floatval = atof(sql_parse_token);
		TOK(APPROXNUM)
	}

	/* strings */
	'[^'\n]*'	{
		int c = input();
		unput(c);	/* just peeking */
		if (c != '\'') {
			sql_parse_save(yytext, yyleng);
			sql_lval.strval = sql_parse_token;
			TOK(STRING)
		} else
			yymore();
	}
	'[^'\n]*$	{
		sql_error("Unterminated string");
	}
	\n		{ lineno++; }
	{space}+	|
	"--".*		|
	#.*		{ }
	.
	<<EOF>>		{
		sql_parse_done();
	}
}

%%

void sql_error(char *s)
{
	printf("%d: %s at %s\n", lineno, s, yytext);
}

int sql_wrap()
{
	return 1;
}

int sql_parse_file(char *file)
{
#ifdef YYDEBUG
	sql_debug = 1;
#endif
	if (!file || !file[0]) return -1;
	yyin = fopen(file, "r");
	if (yyin == NULL) {
		perror(file);
		return -1;
	}

	sql_parse_done();
	if (!sql_parse())
		fprintf(stderr, "\n\nSQL parser worked\n");
	else {
		fprintf(stderr, "\n\nSQL parser failed\n");
		return -1;
	}
	return 0;
}

/* leave SQL lexing mode */
void sql_parse_done()
{
	BEGIN(INITIAL);
}
