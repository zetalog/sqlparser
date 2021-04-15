/*
 * ZETALOG's Personal COPYRIGHT
 *
 * Copyright (c) 2003
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
 * @(#)argcv.c: argc / argv handling routines
 * $Id: argcv.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <argcv.h>

#define isws(c) ((c) == ' '|| (c) == '\t')
#define isdelim(c, delim) (/*(c)=='"'||*/strchr(delim,(c)) != 0)
#define isquote(c) (strchr("\"\'",(c)) != 0)
#define isesc(c) ((c) == '\\')

static int argcv_scan(int len, const char *command, const char *delim,
                      int *start, int *end, int *save)
{
	int i = *save;
	int inquote = 0;
	static char quote = '"';
	
	/* Skip initial whitespace */
	while (i < len && isws(command[i]) || isdelim(command [i], delim))
		i++;
	*start = i;
	
	switch (command[i]) {
	case '"':
	case '\'':
		quote = command[*start];
		while (++i < len) {
			if (command[i] == quote) {
				if (command[i+1] != quote)
					break;
				else i++;
			}
			if (isesc(command[i])) i++;
		}
		if (i < len)  /* found matching quote */
			break;
		/*FALLTHRU*/
	default:
		if (isdelim(command[i], delim))
			break;
		/* Skip until next whitespace character or end of line */
		while (++i < len &&
			!(isws(command [i]) || isdelim(command [i], delim)))
			;
		i--;
		break;
	}
	
	*end = i;
	*save = i+1;
	return *save;
}

void argcv_fine(char *command, const char quote, const char *delim)
{
	char *next = command;
	int inquote = 0;

	for (; *next; ) {
		if (*next == quote) {
			inquote = !inquote;
			if (inquote) {
				memcpy(next, next + 1, strlen(next + 1) + 1);
				next--;
			}
		}
		if (isesc(*next)) {
			if (next[1]) {
				/* dont parse the escaped character */
				memcpy(next, next + 1, strlen(next + 1) + 1);
				next--;
			}
		}
		next++;
	}
}

void argcv_set(int argc, char **argv, int index, const char *value)
{
	assert(value && strlen(value));
	if (index > argc)
		return;
	if (argv[index])
		free(argv[index]);
	argv[index] = strdup(value);
}

int argcv_has_delim(char *buf, char *delim)
{
	char *ch;

	for (ch = buf; *ch != '\0'; ch++) {
		if (isdelim(*ch, delim))
			return 1;
	}
	return 0;
}

int argcv_count_escapes(char *buf)
{
	char *ch;
	int sum = 0;

	for (ch = buf; *ch != '\0'; ch++) {
		if (isquote(*ch))
			sum++;
		else if (isesc(*ch))
			sum++;
	}
	return sum;
}

int argcv_put(char *command, int size, char *delim, int argc, char **argv)
{
	int i = 0;
	int len = 0, sum = 0;
	char *ch;
	
	for (i = 0; i < argc; i++) {
		assert(argv[i]);
		len = strlen(argv[i]);
		assert(len > 0);
		sum += len;

		if (argcv_has_delim(argv[i], delim)) {
			sum += argcv_count_escapes(argv[i]);
			sum += 2;	/* quoters */
		}
		sum += 1;		/* delim */
	}
	sum += 1;			/* end of string */
	if (!command || sum > size) return sum;
	
	sum = 0;
	for (i = 0; i < argc; i++) {
		len = strlen(argv[i]);
		if (argcv_has_delim(argv[i], delim)) {
			command[sum++] = '\"';
			for (ch = argv[i]; *ch != '\0'; ch++) {
				if (isquote(*ch) || isesc(*ch))
					command[sum++] = '\\';
				command[sum++] = *ch;
			}
			command[sum++] = '\"';
		} else {
			for (ch = argv[i]; *ch != '\0'; ch++)
				command[sum++] = *ch;
		}
		if (i < (argc-1))
			command[sum++] = *delim;
	}
	command[sum] = '\0';
	assert(sum < size);
	return 0;
}

int argcv_get(const char *command, const char *delim, int *argc, char ***argv)
{
	int len = strlen(command);
	int i = 0;
	int start, end, save;
	
	*argc = 0;
	*argv = 0;
	
	while (len > 0 && isspace(command[len-1]))
		len--;
	if (len < 1)
		return -1;
	
	/* Count number of arguments */
	*argc = 1;
	save = 0;
	while (argcv_scan(len, command, delim, &start, &end, &save) < len)
		(*argc)++;
	
	*argv = (char **)calloc((*argc + 1), sizeof (char *));
	
	i = 0;
	save = 0;
	for (i = 0; i < *argc; i++) {
		int n;
		char quote = 0;
		argcv_scan(len, command, delim, &start, &end, &save);
		
		if (command[start] == '"' && command[end] == '"') {
			start++;
			end--;
			quote = '"';
		}
		else if (command[start] == '\'' && command[end] == '\'') {
			start++;
			end--;
			quote = '\'';
		}
		n = end - start + 1;
		(*argv)[i] = (char *)calloc(n+1,  sizeof (char));
		if ((*argv)[i] == 0) {
			argcv_free(i, *argv);
			return -1;
		}
		memcpy((*argv)[i], &command[start], n);
		(*argv)[i][n] = 0;
		if (quote != 0) {
			argcv_fine((*argv)[i], quote, delim);
		}
	}
	(*argv)[i] = 0;
	return 0;
}

/* accept argc, return argv */
char **argcv_new(int argc)
{
	return calloc(sizeof (char *) * argc, 1);
}

/*
 * frees all elements of an argv array
 * argc is the number of elements
 * argv is the array
 */
int argcv_free(int argc, char **argv)
{
	while (--argc >= 0) {
		if (argv[argc])
			free(argv[argc]);
	}
	free(argv);
	return 0;
}

#ifdef CONFIG_ARGCV_DEBUG
static char mem[] = "\"\"\"sdf\"\"af   s\"  \"sd\"   \"sdf\"";

void argcv_debug()
{
	int cargc, i;
	char **cargv;
	char buf[1024];

	printf("argcv_get: %s\n", mem);
	argcv_get(mem, " \t", &cargc, &cargv);
	for (i = 0; i < cargc; i++) {
		printf("argv[%d]: %s\n", i, cargv[i]);
	}
	argcv_put(buf, 1024, " \t", cargc, cargv);
	printf("argcv_put: %s\n", buf);
	argcv_free(cargc, cargv);
	argcv_get(mem, " \t", &cargc, &cargv);
	for (i = 0; i < cargc; i++) {
		printf("argv[%d]: %s\n", i, cargv[i]);
	}
	argcv_free(cargc, cargv);
}
#endif
