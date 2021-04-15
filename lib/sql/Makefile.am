lib_LTLIBRARIES = libsql.la

if MYSQL
INCLUDE_MYSQL = -I$(top_srcdir)/lib/mysql/include
SOURCE_MYSQL = mysql.c
else
INCLUDE_MYSQL =
SOURCE_MYSQL =
endif

libsql_la_LIBADD =
libsql_la_SOURCES = \
	sql_parse.h log.c config.c driver.c tds.c \
	nodes.c parse.tab.c lex.scan.c sql_priv.h \
	${SOURCE_MYSQL}
INCLUDES = \
	-I$(top_srcdir)/include -I. \
	-I$(OPENLDAP)/include \
	-I$(OPENSSL)/include \
	${INCLUDE_MYSQL}
EXTRA_DIST = \
	sql.dsp

%.tab.c: %.y
	@rm -f $@
	$(YACC) -t -d -v -b $* -p sql_ $<

sql_parse.h: parse.tab.c
	@cp -f parse.tab.h $@

lex.%.c: %.l sql_parse.h
	@rm -f $@
	$(LEX) -Psql_ -o$@ $<
