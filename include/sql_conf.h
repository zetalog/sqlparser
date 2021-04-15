#ifndef __SQL_SQL_H_INCLUDE__
#define __SQL_SQL_H_INCLUDE__

#define SQL_CTYPE_STRING		0
#define SQL_CTYPE_INTEGER		1
#define SQL_CTYPE_STRING_PTR		100
#define SQL_CTYPE_BOOLEAN		101
#define SQL_CTYPE_SUBSECTION		102

typedef enum _sql_token_t {
	SQL_TOKEN_INVALID = 0,                  /* invalid token */
	SQL_TOKEN_EOL,                          /* end of line */
	SQL_TOKEN_LCBRACE,			/* { */
	SQL_TOKEN_RCBRACE,			/* } */

	SQL_TOKEN_OP_EQ,			/* = */
	SQL_TOKEN_HASH,                         /* # */
	SQL_TOKEN_BARE_WORD,			/* bare word */
	SQL_TOKEN_DOUBLE_QUOTED_STRING,         /* "foo" */
	SQL_TOKEN_SINGLE_QUOTED_STRING,         /* 'foo' */
	SQL_TOKEN_BACK_QUOTED_STRING,		/* `foo` */
	SQL_TOKEN_TOKEN_LAST
} sql_token_t;

#define SQL_TOKEN_EQSTART	SQL_TOKEN_OP_EQ
#define	SQL_TOKEN_EQEND		SQL_TOKEN_OP_EQ

/* And this pointer trick too */
#ifndef offsetof
# define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

/* export the minimum amount of information about these structs */
typedef struct _sql_item_t sql_item_t;
typedef struct _sql_pair_t sql_pair_t;
typedef struct _sql_part_t sql_section_t;

typedef enum _sql_ctype_t {
	SQL_ITEM_PAIR,
	SQL_ITEM_SECTION
} sql_ctype_t;

struct _sql_item_t {
	sql_item_t *next;
	sql_section_t *parent;
	int lineno;
	sql_ctype_t type;
};
struct _sql_pair_t {
	sql_item_t item;
	char *attr;
	char *value;
	sql_token_t operator;
};
struct _sql_part_t {
	sql_item_t item;
	char *name1;
	char *name2;
	sql_item_t *children;
};

typedef struct _sql_parser_t {
	const char *name;
	int type;		/* PW_TYPE_STRING, etc. */
	size_t offset;		/* relative pointer within "base" */
	void *data;		/* absolute pointer if base is NULL */
	const char *dflt;	/* default as it would appear in radiusd.conf */
} sql_parser_t;

sql_pair_t *sql_pair_find(sql_section_t *section, const char *name);
sql_pair_t *sql_pair_find_next(sql_section_t *section, sql_pair_t *pair,
			       const char *name);
char *sql_pair_attr(sql_pair_t *pair);
char *sql_pair_value(sql_pair_t *pair);

int sql_section_parse(sql_section_t *cs, void *base,
		      const sql_parser_t *variables);
sql_section_t *sql_section_find(const char *name);
char *sql_section_value_find(sql_section_t *section, const char *attr);
char *sql_section_name1(sql_section_t *section);
char *sql_section_name2(sql_section_t *section);
sql_section_t *sql_section_sub_find(sql_section_t *section, const char *name);
sql_section_t *sql_subsection_find_next(sql_section_t *section,
					sql_section_t *subsection,
					const char *name1);

sql_item_t *sql_item_find_next(sql_section_t *section, sql_item_t *item);
int sql_item_is_section(sql_item_t *item);
int sql_item_is_pair(sql_item_t *item);
sql_pair_t *sql_itemtopair(sql_item_t *item);
sql_section_t *sql_itemtosection(sql_item_t *item);
sql_item_t *sql_pairtoitem(sql_pair_t *cp);
sql_item_t *sql_sectiontoitem(sql_section_t *cs);
void sql_item_add(sql_section_t *cs, sql_item_t *ci_new);

sql_section_t *sql_section_alloc(const char *name1, const char *name2,
				 sql_section_t *parent);
sql_pair_t *sql_pair_alloc(const char *attr, const char *value,
			   sql_token_t operator, sql_section_t *parent);
void sql_pair_free(sql_pair_t **cp);
void sql_section_free(sql_section_t **cp);

void sql_pair_remove(sql_section_t *cs, sql_pair_t *cp);
void sql_section_remove(sql_section_t *cs, sql_section_t *subcs);

sql_section_t *sql_conf_read(const char *fromfile, int fromline,
			     const char *conffile, sql_section_t *parent);
sql_section_t *sql_conf_init(const char *file);
void sql_conf_free(sql_section_t **cp);
int sql_conf_dump(sql_section_t *cs, int indent, FILE *stream);

#endif /* __SQL_SQL_H_INCLUDE__ */
