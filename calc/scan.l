%{
#include <stdlib.h>
#include <string.h>
#include "calc_parse.h"

void calc_error(char *);

%}

%%
[a-z]		{ calc_lval = *yytext - 'a'; return VARIABLE; }
[0-9]+		{ calc_lval = atoi(yytext); return INTEGER; }
[+()=/*\n]	{ return *yytext; }
[ \t]
.			calc_error("invalid character");
%%
int calc_wrap() {
	printf("User: ");
	return 1;
}

