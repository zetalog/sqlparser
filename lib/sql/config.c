#include "sql_priv.h"

static const sql_map_t tokens[] = {
	{ "{",	SQL_TOKEN_LCBRACE,	},
	{ "}",	SQL_TOKEN_RCBRACE,	},
	{ "=",	SQL_TOKEN_OP_EQ,	},
	{ "#",	SQL_TOKEN_HASH,		},
	{ NULL, 0,		},
};

/*
 * this works only as long as special tokens are max.
 * 2 characters, but it's fast.
 */
#define SQL_TOKEN_MATCH(bptr, tptr)			\
	((tptr)[0] == (bptr)[0] &&			\
	((tptr)[1] == (bptr)[1] || (tptr)[1] == 0))

/*
 * Read a word from a buffer and advance pointer.
 * This function knows about escapes and quotes.
 *
 * At end-of-line, buf[0] is set to '\0'.
 * Returns 0 or special token value.
 */
static sql_token_t getthing(char **ptr, char *buf, int buflen, int tok,
			    const sql_map_t *tokenlist)
{
	char	*s, *p;
	int	quote;
	int	escape;
	int	x;
	const sql_map_t *t;
	sql_token_t rcode;

	buf[0] = 0;

	/* Skip whitespace */
	p = *ptr;
	while (*p && isspace((int) *p))
		p++;

	if (*p == 0) {
		*ptr = p;
		return SQL_TOKEN_EOL;
	}

	/* might be a 1 or 2 character token */
	if (tok) for (t = tokenlist; t->name; t++) {
		if (SQL_TOKEN_MATCH(p, t->name)) {
			strcpy(buf, t->name);
			p += strlen(t->name);
			while (isspace((int) *p))
				p++;
			*ptr = p;
			return (sql_token_t) t->number;
		}
	}

	/* Read word. */
	quote = 0;
	if ((*p == '"') ||
	    (*p == '\'') ||
	    (*p == '`')) {
		quote = *p;
		p++;
	}
	s = buf;
	escape = 0;

	while (*p && buflen-- > 1) {
		if (escape) {
			escape = 0;
			switch (*p) {
			case 'r':
				*s++ = '\r';
				break;
			case 'n':
				*s++ = '\n';
				break;
			case 't':
				*s++ = '\t';
				break;
			case '"':
				*s++ = '"';
				break;
			case '\'':
				*s++ = '\'';
				break;
			case '`':
				*s++ = '`';
				break;
			default:
				if (*p >= '0' && *p <= '9' &&
				    sscanf(p, "%3o", &x) == 1) {
					*s++ = x;
					p += 2;
				} else
					*s++ = *p;
				break;
			}
			p++;
			continue;
		}
		if (*p == '\\') {
			p++;
			escape = 1;
			continue;
		}
		if (quote && (*p == quote)) {
			p++;
			break;
		}
		if (!quote) {
			if (isspace((int) *p))
				break;
			if (tok) {
				for (t = tokenlist; t->name; t++)
					if (SQL_TOKEN_MATCH(p, t->name))
						break;
				if (t->name != NULL)
					break;
			}
		}
		*s = *p, p++, s++;
	}
	*s++ = 0;

	/* Skip whitespace again. */
	while (*p && isspace((int) *p))
		p++;
	*ptr = p;

	/* we got SOME form of output string, even if it is empty */
	switch (quote) {
	default:
		rcode = SQL_TOKEN_BARE_WORD;
		break;
		
	case '\'':
		rcode = SQL_TOKEN_SINGLE_QUOTED_STRING;
		break;
		
	case '"':
		rcode = SQL_TOKEN_DOUBLE_QUOTED_STRING;
		break;
		
	case '`':
		rcode = SQL_TOKEN_BACK_QUOTED_STRING;
		break;
	}

	return rcode;
}

/*
 *	Read a "word" - this means we don't honor
 *	tokens as delimiters.
 */
int sql_conf_getword(char **ptr, char *buf, int buflen)
{
	return getthing(ptr, buf, buflen, 0, tokens) == SQL_TOKEN_EOL ? 0 : 1;
}

/*
 *	Read a bare "word" - this means we don't honor
 *	tokens as delimiters.
 */
int sql_conf_getbareword(char **ptr, char *buf, int buflen)
{
	sql_token_t token;

	token = getthing(ptr, buf, buflen, 0, NULL);
	if (token != SQL_TOKEN_BARE_WORD) {
		return 0;
	}

	return 1;
}

/* read the next word, use tokens as delimiters */
sql_token_t sql_conf_gettoken(char **ptr, char *buf, int buflen)
{
	return getthing(ptr, buf, buflen, 1, tokens);
}

int sql_name2token(const char *name)
{
	return sql_str2int(tokens, name, SQL_TOKEN_OP_EQ);
}

const char *sql_token2name(int number)
{
	return sql_int2str(tokens, number, "=");
}

/* copy shamelessly from FreeRADIUS project */

sql_section_t *sql_mainconfig = NULL;

/* isolate the scary casts in these tiny provably-safe functions */
sql_pair_t *sql_itemtopair(sql_item_t *ci)
{
	if (ci == NULL)
		return NULL;
	assert(ci->type == SQL_ITEM_PAIR);
	return (sql_pair_t *)ci;
}
sql_section_t *sql_itemtosection(sql_item_t *ci)
{
	if (ci == NULL)
		return NULL;
	assert(ci->type == SQL_ITEM_SECTION);
	return (sql_section_t *)ci;
}
sql_item_t *sql_pairtoitem(sql_pair_t *cp)
{
	if (cp == NULL)
		return NULL;
	return (sql_item_t *)cp;
}
sql_item_t *sql_sectiontoitem(sql_section_t *cs)
{
	if (cs == NULL)
		return NULL;
	return (sql_item_t *)cs;
}

/* create a new sql_pair_t */
sql_pair_t *sql_pair_alloc(const char *attr, const char *value,
			   sql_token_t operator, sql_section_t *parent)
{
	sql_pair_t *cp;

	cp = (sql_pair_t *)malloc(sizeof(sql_pair_t));
	memset(cp, 0, sizeof(sql_pair_t));
	cp->item.type = SQL_ITEM_PAIR;
	cp->item.parent = parent;
	cp->attr = strdup(attr);
	cp->value = strdup(value);
	cp->operator = operator;

	return cp;
}

/* free a sql_pair_t */
void sql_pair_free(sql_pair_t **cp)
{
	if (!cp || !*cp) return;

	if ((*cp)->attr)
		free((*cp)->attr);
	if ((*cp)->value)
		free((*cp)->value);

#ifndef NDEBUG
	memset(*cp, 0, sizeof(*cp));
#endif
	free(*cp);
	*cp = NULL;
}

/* allocate a sql_section_t */
sql_section_t *sql_section_alloc(const char *name1, const char *name2,
				 sql_section_t *parent)
{
	sql_section_t	*cs;

	if (name1 == NULL || !name1[0])
		name1 = "main";

	cs = (sql_section_t *)malloc(sizeof(sql_section_t));
	memset(cs, 0, sizeof(sql_section_t));
	cs->item.type = SQL_ITEM_SECTION;
	cs->item.parent = parent;
	cs->name1 = strdup(name1);
	cs->name2 = (name2 && *name2) ? strdup(name2) : NULL;

	return cs;
}

/* free a sql_section_t */
void sql_section_free(sql_section_t **cs)
{
	sql_item_t	*ci, *next;

	if (!cs || !*cs) return;

	for (ci = (*cs)->children; ci; ci = next) {
		next = ci->next;
		if (ci->type==SQL_ITEM_PAIR) {
			sql_pair_t *pair = sql_itemtopair(ci);
			sql_pair_free(&pair);
		} else {
			sql_section_t *section = sql_itemtosection(ci);
			sql_section_free(&section);
		}
	}

	if ((*cs)->name1)
		free((*cs)->name1);
	if ((*cs)->name2)
		free((*cs)->name2);

	/*
	 * And free the section
	 */
#ifndef NDEBUG
	memset(*cs, 0, sizeof(*cs));
#endif
	free(*cs);

	*cs = NULL;
}

/* add an item to a configuration section */
void sql_item_add(sql_section_t *cs, sql_item_t *ci_new)
{
	sql_item_t *ci;

	for (ci = cs->children; ci && ci->next; ci = ci->next)
		;

	if (ci == NULL)
		cs->children = ci_new;
	else
		ci->next = ci_new;
}

/* expand the variables in an input string */
const char *sql_expand_variables(const char *cf, int *lineno,
				sql_section_t *outercs,
				char *output, const char *input)
{
	char *p;
	const char *end, *ptr;
	char name[8192];
	sql_section_t *parentcs;

	/*
	 * find the master parent conf section.
	 * we can't use sql_mainconfig.config, because we're in the
	 * process of re-building it, and it isn't set up yet...
	 */
	for (parentcs = outercs;
	     parentcs->item.parent != NULL;
	     parentcs = parentcs->item.parent) {
		/* do nothing */
	}

	p = output;
	ptr = input;
	while (*ptr) {
		/* ignore anything other than "${" */
		if ((*ptr == '$') && (ptr[1] == '{')) {
			int up;
			sql_pair_t *cp;
			sql_section_t *cs;

			/*
			 * look for trailing '}', and log a
			 * warning for anything that doesn't match,
			 * and exit with a fatal error.
			 */
			end = strchr(ptr, '}');
			if (end == NULL) {
				*p = '\0';
				sql_log(SQL_LOG_INFO, "%s[%d]: Variable expansion missing }",
					cf, *lineno);
				return NULL;
			}

			ptr += 2;

			cp = NULL;
			up = 0;

			/* ${.foo} means "foo from the current section" */
			if (*ptr == '.') {
				up = 1;
				cs = outercs;
				ptr++;

				/*
				 * ${..foo} means "foo from the section
				 * enclosing this section" (etc.)
				 */
				while (*ptr == '.') {
					if (cs->item.parent)
						cs = cs->item.parent;
					ptr++;
				}

			} else {
				const char *q;
				/*
				 * ${foo} is local, with
				 * main as lower priority
				 */
				cs = outercs;

				/*
				 * ${foo.bar.baz} is always rooted
				 * from the top.
				 */
				for (q = ptr; *q && q != end; q++) {
					if (*q == '.') {
						cs = parentcs;
						up = 1;
						break;
					}
				}
			}

			while (cp == NULL) {
				char *q;

				/* find the next section */
				for (q = name;
				     (*ptr != 0) && (*ptr != '.') &&
					     (ptr != end);
				     q++, ptr++) {
					*q = *ptr;
				}
				*q = '\0';

				/*
				 * the character is a '.', find a
				 * section (as the user has given
				 * us a subsection to find)
				 */
				if (*ptr == '.') {
					sql_section_t *next;

					ptr++;	/* skip the period */

					/* find the sub-section */
					next = sql_section_sub_find(cs, name);
					if (next == NULL) {
						sql_log(SQL_LOG_ERR, "config: No such section %s in variable %s", name, input);
						return NULL;
					}
					cs = next;

				} else {
					/* no period, must be a conf-part */
					/*
					 * find in the current referenced
					 * section
					 */
					cp = sql_pair_find(cs, name);
					if (cp == NULL) {
						/*
						 * it it was NOT ${..foo}
						 * then look in the
						 * top-level config items.
						 */
						if (!up) cp = sql_pair_find(parentcs, name);
					}
					if (cp == NULL) {
						sql_log(SQL_LOG_ERR, "config: No such entry %s for string %s", name, input);
						return NULL;
					}
				}
			}

			/* substitute the value of the variable */
			strcpy(p, cp->value);
			p += strlen(p);
			ptr = end + 1;

		} else if (memcmp(ptr, "$ENV{", 5) == 0) {
			char *env;

			ptr += 5;

			/*
			 * look for trailing '}', and log a
			 * warning for anything that doesn't match,
			 * and exit with a fatal error
			 */
			end = strchr(ptr, '}');
			if (end == NULL) {
				*p = '\0';
				sql_log(SQL_LOG_INFO, "%s[%d]: Environment variable expansion missing }",
					cf, *lineno);
				return NULL;
			}

			memcpy(name, ptr, end - ptr);
			name[end - ptr] = '\0';

			/*
			 * get the environment variable
			 * if none exists, then make it an empty string
			 */
			env = getenv(name);
			if (env == NULL) {
				*name = '\0';
				env = name;
			}

			strcpy(p, env);
			p += strlen(p);
			ptr = end + 1;

		} else {
			/* copy it over verbatim */
			*(p++) = *(ptr++);
		}
	}
	*p = '\0';
	return output;
}

/* read a part of the config file */
static sql_section_t *sql_section_read(const char *cf, int *lineno, FILE *fp,
				     const char *name1, const char *name2,
				     sql_section_t *parent)
{
	sql_section_t *cs, *css;
	sql_pair_t *cpn;
	char *ptr;
	const char *value;
	char buf[8192];
	char buf1[8192];
	char buf2[8192];
	char buf3[8192];
	int t1, t2, t3;
	char *cbuf = buf;
	int len;

	/* ensure that the user can't add SQL_SECTIONs with 'internal' names */
	if ((name1 != NULL) && (name1[0] == '_')) {
		sql_log(SQL_LOG_ERR, "%s[%d]: Illegal configuration section name",
			cf, *lineno);
		return NULL;
	}

	/* allocate new section */
	cs = sql_section_alloc(name1, name2, parent);
	cs->item.lineno = *lineno;

	/* read, checking for line continuations ('\\' at EOL) */
	for (;;) {
		int eof;

		/* get data, and remember if we are at EOF */
		eof = (fgets(cbuf, sizeof(buf) - (cbuf - buf), fp) == NULL);
		(*lineno)++;

		len = strlen(cbuf);

		/*
		 * we've filled the buffer, and there isn't
		 * a CR in it.  Die!
		 */
		if ((len == sizeof(buf)) &&
		    (cbuf[len - 1] != '\n')) {
			sql_log(SQL_LOG_ERR, "%s[%d]: Line too long",
				cf, *lineno);
			sql_section_free(&cs);
			return NULL;
		}

		/* check for continuations */
		if (cbuf[len - 1] == '\n') len--;

		/* last character is '\\'.  Over-write it, and read another line */
		if ((len > 0) && (cbuf[len - 1] == '\\')) {
			cbuf[len - 1] = '\0';
			cbuf += len - 1;
			continue;
		}

		/* we're at EOF, and haven't read anything.  Stop */
		if (eof && (cbuf == buf)) {
			break;
		}

		ptr = cbuf = buf;
		t1 = sql_conf_gettoken(&ptr, buf1, sizeof(buf1));

		/* skip comments and blank lines immediately */
		if ((*buf1 == '#') || (*buf1 == '\0')) {
			continue;
		}

		/*
		 * Allow for $INCLUDE files
		 *
		 * this *SHOULD* work for any level include.
		 * I really really really hate this file.  -cparker
		 */
		if (strcasecmp(buf1, "$INCLUDE") == 0) {

			sql_section_t      *is;

			t2 = sql_conf_getword(&ptr, buf2, sizeof(buf2));

			value = sql_expand_variables(cf, lineno, cs, buf, buf2);
			if (value == NULL) {
				sql_section_free(&cs);
				return NULL;
			}

			sql_log(SQL_LOG_DEBUG(2), "Config:   including file: %s", value);

			if ((is = sql_conf_read(cf, *lineno, value, cs)) == NULL) {
				sql_section_free(&cs);
				return NULL;
			}

			/* add the included conf to our sql_section_t */
			if (is != NULL) {
				if (is->children != NULL) {
					sql_item_t *ci;

					/*
					 * re-write the parent of the
					 * moved children to be the
					 * upper-layer section.
					 */
					for (ci = is->children; ci; ci = ci->next) {
						ci->parent = cs;
					}

					/*
					 * if there are children, then
					 * move them up a layer.
					 */
					if (is->children) {
						sql_item_add(cs, is->children);
					}
					is->children = NULL;
				}
				/*
				 * Always free the section for the
				 * $INCLUDEd file.
				 */
				sql_section_free(&is);
			}

			continue;
		}

		/* No '=': must be a section or sub-section */
		if (strchr(ptr, '=') == NULL) {
			t2 = sql_conf_gettoken(&ptr, buf2, sizeof(buf2));
			t3 = sql_conf_gettoken(&ptr, buf3, sizeof(buf3));
		} else {
			t2 = sql_conf_gettoken(&ptr, buf2, sizeof(buf2));
			t3 = sql_conf_getword(&ptr, buf3, sizeof(buf3));
		}

		/* see if it's the end of a section */
		if (t1 == SQL_TOKEN_RCBRACE) {
			if (name1 == NULL || buf2[0]) {
				sql_log(SQL_LOG_ERR, "%s[%d]: Unexpected end of section",
					cf, *lineno);
				sql_section_free(&cs);
				return NULL;
			}
			return cs;
		}

		/* perhaps a subsection */
		if (t2 == SQL_TOKEN_LCBRACE || t3 == SQL_TOKEN_LCBRACE) {
			css = sql_section_read(cf, lineno, fp, buf1,
					      t2==SQL_TOKEN_LCBRACE ? NULL : buf2, cs);
			if (css == NULL) {
				sql_section_free(&cs);
				return NULL;
			}
			sql_item_add(cs, sql_sectiontoitem(css));

			continue;
		}

		/* ignore semi-colons */
		if (*buf2 == ';')
			*buf2 = '\0';

		/* must be a normal attr = value line */
		if (buf1[0] != 0 && buf2[0] == 0 && buf3[0] == 0) {
			t2 = SQL_TOKEN_OP_EQ;
		} else if (buf1[0] == 0 || buf2[0] == 0 ||
			   (t2 < SQL_TOKEN_EQSTART || t2 > SQL_TOKEN_EQEND)) {
			/* TODO: do not simply fail here!!! */
			sql_log(SQL_LOG_ERR, "%s[%d]: Line is not in 'attribute = value' format",
				cf, *lineno);
			sql_section_free(&cs);
			return NULL;
		}

		/*
		 * ensure that the user can't add SQL_PAIRs
		 * with 'internal' names;
		 */
		if (buf1[0] == '_') {
			sql_log(SQL_LOG_ERR, "%s[%d]: Illegal configuration pair name \"%s\"",
				cf, *lineno, buf1);
			sql_section_free(&cs);
			return NULL;
		}

		/* handle variable substitution via ${foo} */
		value = sql_expand_variables(cf, lineno, cs, buf, buf3);
		if (!value) {
			sql_section_free(&cs);
			return NULL;
		}


		/* add this sql_pair_t to our sql_section_t */
		cpn = sql_pair_alloc(buf1, value, t2, parent);
		cpn->item.lineno = *lineno;
		sql_item_add(cs, sql_pairtoitem(cpn));
	}

	/* see if EOF was unexpected .. */
	if (name1 != NULL) {
		sql_log(SQL_LOG_ERR, "%s[%d]: Unexpected end of file", cf, *lineno);
		sql_section_free(&cs);
		return NULL;
	}

	return cs;
}

void sql_conf_free(sql_section_t **cp)
{
	if (cp && *cp == sql_mainconfig)
		sql_mainconfig = NULL;
	sql_section_free(cp);
}

sql_section_t *sql_conf_init(const char *file)
{
	if (sql_mainconfig)
		return sql_mainconfig;
	sql_mainconfig = sql_conf_read(NULL, 0, file, NULL);
	return sql_mainconfig;
}

/* read the config file */
sql_section_t *sql_conf_read(const char *fromfile, int fromline,
			     const char *conffile, sql_section_t *parent)
{
	FILE		*fp;
	int		lineno = 0;
	sql_section_t	*cs;

	if ((fp = fopen(conffile, "r")) == NULL) {
		if (fromfile) {
			sql_log(SQL_LOG_ERR, "%s[%d]: Unable to open file \"%s\": %s",
				fromfile, fromline, conffile, strerror(errno));
		} else {
			sql_log(SQL_LOG_ERR, "Unable to open file \"%s\": %s",
				conffile, strerror(errno));
		}
		return NULL;
	}

	if(parent) {
	    cs = sql_section_read(conffile, &lineno, fp, NULL, NULL, parent);
	} else {
	    cs = sql_section_read(conffile, &lineno, fp, NULL, NULL, NULL);
	}

	fclose(fp);
	return cs;
}

/* return a sql_pair_t within a sql_section_t */
sql_pair_t *sql_pair_find(sql_section_t *section, const char *name)
{
	sql_item_t	*ci;

	if (section == NULL) {
		section = sql_mainconfig;
	}

	for (ci = section->children; ci; ci = ci->next) {
		if (ci->type != SQL_ITEM_PAIR)
			continue;
		if (name == NULL || strcmp(sql_itemtopair(ci)->attr, name) == 0)
			break;
	}

	return sql_itemtopair(ci);
}

/* return the attr of a sql_pair_t */
char *sql_pair_attr(sql_pair_t *pair)
{
	return (pair ? pair->attr : NULL);
}

/* return the value of a sql_pair_t */
char *sql_pair_value(sql_pair_t *pair)
{
	return (pair ? pair->value : NULL);
}

/* return the first label of a sql_section_t */
char *sql_section_name1(sql_section_t *section)
{
	return (section ? section->name1 : NULL);
}

/* return the second label of a sql_section_t */
char *sql_section_name2(sql_section_t *section)
{
	return (section ? section->name2 : NULL);
}

/* find a value in a sql_section_t */
char *sql_section_value_find(sql_section_t *section, const char *attr)
{
	sql_pair_t	*cp;

	cp = sql_pair_find(section, attr);

	return (cp ? cp->value : NULL);
}

/*
 * Return the next pair after a sql_pair_t
 * with a certain name (char *attr) If the requested
 * attr is NULL, any attr matches.
 */
sql_pair_t *sql_pair_find_next(sql_section_t *section, sql_pair_t *pair, const char *attr)
{
	sql_item_t	*ci;

	/*
	 * If pair is NULL this must be a first time run
	 * Find the pair with correct name
	 */
	if (pair == NULL){
		return sql_pair_find(section, attr);
	}

	ci = sql_pairtoitem(pair)->next;

	for (; ci; ci = ci->next) {
		if (ci->type != SQL_ITEM_PAIR)
			continue;
		if (attr == NULL || strcmp(sql_itemtopair(ci)->attr, attr) == 0)
			break;
	}

	return sql_itemtopair(ci);
}

/* find a sql_section_t, or return the root if name is NULL */
sql_section_t *sql_section_find(const char *name)
{
	if (name)
		return sql_section_sub_find(sql_mainconfig, name);
	else
		return sql_mainconfig;
}

/* find a sub-section in a section */
sql_section_t *sql_section_sub_find(sql_section_t *section, const char *name)
{
	sql_item_t *ci;

	for (ci = section->children; ci; ci = ci->next) {
		if (ci->type != SQL_ITEM_SECTION)
			continue;
		if (strcmp(sql_itemtosection(ci)->name1, name) == 0)
			break;
	}

	return sql_itemtosection(ci);
}

/*
 * Return the next subsection after a sql_section_t
 * with a certain name1 (char *name1). If the requested
 * name1 is NULL, any name1 matches.
 */
sql_section_t *sql_subsection_find_next(sql_section_t *section,
					sql_section_t *subsection,
					const char *name1)
{
	sql_item_t	*ci;

	/*
	 * If subsection is NULL this must be a first time run
	 * Find the subsection with correct name
	 */
	if (subsection == NULL){
		ci = section->children;
	} else {
		ci = sql_sectiontoitem(subsection)->next;
	}

	for (; ci; ci = ci->next) {
		if (ci->type != SQL_ITEM_SECTION)
			continue;
		if ((name1 == NULL) ||
		    (strcmp(sql_itemtosection(ci)->name1, name1) == 0))
			break;
	}

	return sql_itemtosection(ci);
}

/* return the next item after a sql_item_t */
sql_item_t *sql_item_find_next(sql_section_t *section, sql_item_t *item)
{
	/*
	 * If item is NULL this must be a first time run
	 * Return the first item
	 */
	if (item == NULL) {
		return section->children;
	} else {
		return item->next;
	}
}

int sql_section_lineno(sql_section_t *section)
{
	return sql_sectiontoitem(section)->lineno;
}

int sql_pair_lineno(sql_pair_t *pair)
{
	return sql_pairtoitem(pair)->lineno;
}

int sql_item_is_section(sql_item_t *item)
{
	return item->type == SQL_ITEM_SECTION;
}
int sql_item_is_pair(sql_item_t *item)
{
	return item->type == SQL_ITEM_PAIR;
}

/* parse a configuration section into user-supplied variables */
int sql_section_parse(sql_section_t *cs, void *base,
		     const sql_parser_t *variables)
{
	int i;
	int rcode;
	char **q;
	sql_pair_t *cp;
	sql_section_t *subsection;
	char buffer[8192];
	const char *value;
	void *data;

	/* handle the user-supplied variables */
	for (i = 0; variables[i].name != NULL; i++) {
		value = variables[i].dflt;
		if (variables[i].data) {
			data = variables[i].data; /* prefer this */
		} else if (base) {
			data = ((char *)base) + variables[i].offset;
		} else {
			data = variables[i].data;
		}

		cp = sql_pair_find(cs, variables[i].name);
		if (cp) {
			value = cp->value;
		}

		switch (variables[i].type) {
		case SQL_CTYPE_SUBSECTION:
			subsection = sql_section_sub_find(cs,variables[i].name);

			/*
			 * if the configuration section is NOT there,
			 * then ignore it.
			 *
			 * FIXME! This is probably wrong... we should
			 * probably set the items to their default values.
			 */
			if (subsection == NULL) {
				break;
			}

			rcode = sql_section_parse(subsection, base,
					(sql_parser_t *) data);
			if (rcode < 0) {
				return -1;
			}
			break;

		case SQL_CTYPE_BOOLEAN:
			/* allow yes/no and on/off */
			if ((strcasecmp(value, "yes") == 0) ||
					(strcasecmp(value, "on") == 0)) {
				*(int *)data = 1;
			} else if ((strcasecmp(value, "no") == 0) ||
						(strcasecmp(value, "off") == 0)) {
				*(int *)data = 0;
			} else {
				*(int *)data = 0;
				sql_log(SQL_LOG_ERR,
					"Bad value \"%s\" for boolean variable %s",
					value, variables[i].name);
				return -1;
			}
			sql_log(SQL_LOG_DEBUG(2), " %s: %s = %s", cs->name1,
				variables[i].name, value);
			break;

		case SQL_CTYPE_INTEGER:
			*(int *)data = strtol(value, 0, 0);
			sql_log(SQL_LOG_DEBUG(2), " %s: %s = %d",
				cs->name1, variables[i].name, *(int *)data);
			break;

		case SQL_CTYPE_STRING_PTR:
			q = (char **) data;
			if (*q != NULL) {
				free(*q);
			}

			/*
			 * expand variables while parsing, but ONLY expand
			 * ones which haven't already been expanded.
			 */
			if (value && (value == variables[i].dflt)) {
				value = sql_expand_variables("?",
							    &cs->item.lineno,
							    cs, buffer, value);
				if (!value) {
					return -1;
				}
			}

			sql_log(SQL_LOG_DEBUG(2), " %s: %s = \"%s\"",
				cs->name1, variables[i].name, value ? value : "(null)");
			*q = value ? strdup(value) : NULL;
			break;

		default:
			sql_log(SQL_LOG_ERR, "type %d not supported yet", variables[i].type);
			return -1;
			break;
		}
	}
	return 0;
}

int sql_nottoken(const char *buf, int buflen)
{
	const char *p = buf;
	const sql_map_t *t;

	while (*p && buflen-- > 1) {
		if (*p == '\\')
			return 1;
		if (isspace((int) *p))
			return 1;
		for (t = tokens; t->name; t++) {
			if (SQL_TOKEN_MATCH(p, t->name))
				return 1;
		}
		p++;
	}
	return 0;
}

static char *sql_conf_escape(const char *value)
{
	static char *res = NULL;
	char *out;
	const char *in;
	int inlen = 0, done = 0, outlen;

	if (!value)
		return NULL;
	inlen = strlen(value);
	if (!sql_nottoken(value, inlen+1))
		return (char *)value;
	if (res) {
		free(res);
	}
	res = malloc(2 * inlen + 3);
	if (!res) return (char *)value;

	out = res;
	outlen = 2 * inlen + 3;
	in = value;

	*out++ = '"';
	while (inlen-- > 0 && (done + 3) < outlen) {
		if (*in == '\\') {
			*out++ = '\\';
			*out++ = '\\';
			outlen -= 2;
			done += 2;
		} else if (*in == '"') {
			*out++ = '\\';
			*out++ = '"';
			outlen -= 2;
			done += 2;
		} else if (*in == '\r') {
			*out++ = '\\';
			*out++ = 'r';
			outlen -= 2;
			done += 2;
		} else if (*in == '\n') {
			*out++ = '\\';
			*out++ = 'n';
			outlen -= 2;
			done += 2;
		} else if (*in == '\t') {
			*out++ = '\\';
			*out++ = 't';
			outlen -= 2;
			done += 2;
		} else {
			*out++ = *in;
			outlen--;
			done++;
		}
		in++;
	}
	*out++ = '"';
	*out = 0;
	return res;
}

/*
 * conf_dump: tries to dump the config structure in a readable format
 *
 * indent: 0 means the top most sections, we will do special dump operations.
 */
int sql_conf_dump(sql_section_t *cs, int indent, FILE *stream)
{
	sql_section_t *scs;
	sql_pair_t *cp;
	sql_item_t *ci;

	for (ci = cs->children; ci; ci = ci->next) {
		if (ci->type == SQL_ITEM_PAIR) {
			cp = sql_itemtopair(ci);
			if (!cp || !cp->attr)
				continue;
			if (cp->value && strlen(cp->value)) {
				fprintf(stream, "%.*s%s = %s\n",
					indent, "\t\t\t\t\t\t\t\t\t\t\t",
					cp->attr, sql_conf_escape(cp->value));
			} else {
				fprintf(stream, "%.*s%s\n",
					indent, "\t\t\t\t\t\t\t\t\t\t\t",
					cp->attr);
			}
		} else {
			scs = sql_itemtosection(ci);
			if (!scs || !scs->name1)
				continue;
			fprintf(stream, "\n%.*s%s %s%s{\n",
				indent, "\t\t\t\t\t\t\t\t\t\t\t",
				scs->name1,
				scs->name2 ? scs->name2 : "",
				scs->name2 ?  " " : "");
			sql_conf_dump(scs, indent+1, stream);
			fprintf(stream, "%.*s}\n",
				indent, "\t\t\t\t\t\t\t\t\t\t\t");
		}
	}
	return 0;
}

void sql_pair_remove(sql_section_t *cs, sql_pair_t *cp)
{
	sql_item_t *ci, *next, *last;

	if (!cs) return;

	last = NULL;
	for (ci = cs->children; ci; ci = next) {
		next = ci->next;
		if (ci->type == SQL_ITEM_PAIR) {
			sql_pair_t *pair = sql_itemtopair(ci);
			if (pair == cp) {
				if (last)
					last->next = next;
				else
					cs->children = next;
				sql_pair_free(&pair);
			}
		}
		last = ci;
	}
}

void sql_section_remove(sql_section_t *cs, sql_section_t *subcs)
{
	sql_item_t *ci, *next, *last;

	if (!cs) return;

	last = NULL;
	for (ci = cs->children; ci; ci = next) {
		next = ci->next;
		if (ci->type == SQL_ITEM_SECTION) {
			sql_section_t *section = sql_itemtosection(ci);
			if (subcs == section) {
				if (last)
					last->next = next;
				else
					cs->children = next;
				sql_section_free(&section);
			}
		}
		last = ci;
	}
}
