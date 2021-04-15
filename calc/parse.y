%token INTEGER VARIABLE
%left '+' '-'
%left '*' '/'
%{
void *alloca(size_t);
void calc_error(char *);
int calc_lex(void);
int sym[26];
%}
%%

program:
		statement '\n'
	|
	;

statement:
		expr { printf("%d\n", $1);}
	|	VARIABLE '=' expr { sym[$1] = $3; }
	;

expr:
		INTEGER
	|	VARIABLE { $$ = sym[$1]; }
	|	expr '+' expr { $$ = $1 + $3; }
	|	expr '-' expr { $$ = $1 - $3; }
	|	expr '*' expr { $$ = $1 * $3; }
	|	expr '/' expr { $$ = $1 * $3; }
	|	'(' expr ')'  { $$ = $2; }
	;

%%

void calc_error(char *s) {
	fprintf(stderr, "%s\n", s);
}

void calc_user_input()
{
	int i = 10;

	while (i--) {
		printf("User: ");
		calc_parse();
	}
}

void *alloca(size_t size)
{
	return calloc(1, size);
}

int main(void) {
	calc_user_input();
	return 0;
}
