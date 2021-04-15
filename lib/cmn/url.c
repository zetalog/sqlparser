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
 * @(#)url.c: HTTP form data codec functions
 * $Id: url.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */
/*
 * URL encoding C version
 * A new string is produced, URL encoded. Rules are
 * Alphanumeric chars unchanged
 * Space --> '+'
 * Special chars (the rest) --> "%hh" where hh is hexadecimal
 * while decoding do the reverse
 */
#include <proto.h>
#include <url.h>
#include <base64.h>

static int is_alpha_numeric(char ch);
static int hex2int(const char *hex);
static char bin2hex(char ch);
static char hex2char(const uint8_t *hex);
void char2hex(const char c, uint8_t *hex, size_t size);

/*
 * return ascii char for hex value of
 * rightmost 4 bits of input
 * mask off right nibble  - & bitwise AND
 * make ascii '0' - '9'
 * account for 7 chars between '9' and 'A'
 */
char bin2hex(char ch)
{
	/* mask off right nibble  - & bitwise AND*/
	ch = ch & 0x0f;
	/* make ascii '0' - '9'  */
	ch += '0';
	/* account for 7 chars between '9' and 'A' */
	if (ch > '9')
		ch += 7;
	return (ch);
}

int is_alpha_numeric(char ch)
{
	return ((ch >='a') && (ch <= 'z') ||    /* logical AND &&, OR || */
		(ch >='A') && (ch <= 'Z') ||
		(ch >='0') && (ch <= '9') );
}

int url_encode(const char *in, size_t inlen, char *out, size_t outlen)
{
	char ch;		/* each char, use $t2 */
	char *limit;	/* point to last available location in encode */
	char *start = out;	/* save start of encode for length calculation */
	int  maxlen = outlen< inlen*3 ?outlen:inlen*3;
	limit = out + maxlen;	/* need to store 3 chars and a zero */      
	
	while ((ch = *in ++)!= 0) {
		/* end of string, asciiz */
		if (ch == ' ')
			* out ++ = '+';
		else if (is_alpha_numeric(ch))
			* out ++ = ch;
		else {
			* out ++ = '%';
			* out ++ = bin2hex ((char)(ch >> 4));	/*shift right for left nibble*/
			* out ++ = bin2hex (ch);		/* right nibble */
		}
		if (out > limit) {
			*out = 0;	/* still room to terminate string */
			return (-1);	/* get out with error indication */
		}
	}
	/* store zero byte to terminate string */
	*out = 0;
	/* done, return count of characters */
	return (out-start);
}

/* unescapes escaped characters. */
static int hex2int(const char *hex)
{
	int Hi;		/* holds high byte */
	int Lo;		/* holds low byte */
	int Result;	/* holds result */
	
	/* Get the value of the first byte to Hi */
	Hi = hex[0];
	if ('0' <= Hi && Hi <= '9') {
		Hi -= '0';
	} else if ('a' <= Hi && Hi <= 'f') {
		Hi -= ('a'-10);
	} else if ('A' <= Hi && Hi <= 'F') {
		Hi -= ('A'-10);
	}
	
	/* Get the value of the second byte to Lo */
	Lo = hex[1];
	if ('0' <= Lo && Lo <= '9') {
		Lo -= '0';
	} else if ('a' <= Lo && Lo <= 'f') {
		Lo -= ('a'-10);
	} else if ('A' <= Lo && Lo <= 'F') {
		Lo -= ('A'-10);
	}
	Result = Lo + (16 * Hi);
	return (Result);
}

int url_decode(const char *in, size_t inlen, char *out, size_t outlen)
{
	char ch;		/* each char, use $t2 */
	char * limit;		/* point to last available location in encode */
	char * start = out;	/* save start of encode for length calculation */
	char s[3];
	int  maxlen = outlen<inlen?outlen:inlen;
	
	/* need to store 3 chars and a zero */ 
	limit = out + maxlen;
	/* end of string, asciiz */
	while ((ch = *in ++) != 0) {
		if (ch == '+')
			* out ++ = ' ';
		else if (is_alpha_numeric(ch))
			* out ++ = ch;
		else if (ch == '%') {
			s[0] = * in ++ ;
			s[1] = * in ++ ;
			s[2] = 0;
			if (isxdigit(s[0]) && isxdigit(s[1])) {
				/* right nibble */
				* out ++ = hex2int(s);
			}
		}
		if (out > limit) {
			*out = 0;	/* still room to terminate string */
			return (-1);	/* get out with error indication */
		}
	}
	* out = 0;		/* store zero byte to terminate string */
	return (out - start);	/* done, return count of characters */
}

char hex2char(const uint8_t *hex)
{
	int i, j;
	
	i = (int) hex[0];
	j = (int) hex[1];
	
	if (hex[0] >= 'A') 
		i -= 55; 
	else
		i -= 48;
	if (hex[1] >= 'A') 
		j -= 55; 
	else
		j -= 48; 
	
	return (char) ((16*i) + j);
}

void char2hex(const char c, uint8_t *hex, size_t size)
{
	size_t dec, i, rem, res, j, k;
	char tmp;
	
	dec = (int) c;
	
	i = 0;
	do {
		res = dec / 16;
		rem = dec % 16;
		dec = res;
		
		if (rem >= 10)
			hex[i] = rem + 'A' - 10;
		else
			hex[i] = (char ) rem + '0';	 
		i++;
	} while (dec != 0 && i < size);
	
	/* Pad the string with '0' */ 
	while (i < size-1) hex[i++] = '0';
	
	hex[i] = '\0';
	
	/* Reverse the string. */
	for (j = 0, k = i-1; j < k; j++, k--) {
		tmp    = hex[j];
		hex[j] = hex[k];
		hex[k] = tmp;
	}
}

int qp_decode(const uint8_t *in, size_t inlen, uint8_t *out, size_t outsize)
{
	size_t decoded, i;
	const uint8_t *p, *q;
	char hex[3];
	
	q = p = in;
	
	for (p = in, i = 0; p < (inlen + in) && i < outsize; p++, i++) {
		if (*p == '=') {                    
			q = p + 1;
			
			if (*q == '\n') {
				/* Soft Line Break. */
				p += 2;
				i--;
			}
			else if (*q == '\r') {
				p += 3;
				i--;
			}
			else {
				/* Encoded Character. */
				strncpy(hex, q, 2);
				hex[2] = 0;
				decoded = strtol(hex, NULL, 16);
				out[i] = (char) decoded;
				p += 2;
			}
		}
		else {
			out[i] = (char) *p;
		}
	}
	out[i] = '\0';
	
	return i+1;
}

int qp_encode(const uint8_t *in, size_t inlen, uint8_t *out, size_t outsize)
{
	size_t i, columns;
	const uint8_t *p, *q;
	char hex[3];
	
	q = in + inlen;
	assert(outsize >= (inlen*3+1));
	
	i = 0;
	
	for (p = in, columns = 0; p < q && *p != '\0'; p++, columns++) {
		if ((*p >= '!' && *p <= '<') || (*p >= '>' && *p <= '~'))
			out[i++] = *p;
		else 
			switch (*p) {
	    case '\n':
		    if (*(p-1) != '\r') {
			    out[i++] = '\r';
			    out[i++] = '\n';
		    }
		    columns = 0;
		    break;
	    case '\t':
	    case ' ' :
		    if (*(p+1) != '\r' && *(p+1) != '\n') {
			    out[i++] = *p;
			    break;
		    }	   
	    default:
		    char2hex(*p, hex, 3);
		    out[i++] = '=';
		    out[i++] = hex[0];
		    out[i++] = hex[1];
		    break;
		}
		if (columns == 76) {
			out[i++] = '=';
			out[i++] = '\r';
			out[i++] = '\n';
			columns = 0;
		}
	}
	out[i] = '\0';
	
	return i+1;
}

#define MIME_MAX_CHARSET	32
/* Examples (From RFC2047):
 * =?ISO-8859-1?Q?Andr=E9?=
 * =?ISO-8859-1?B?SWYgeW91IGNhbiByZWFkIHRoaXMgeW8=?= */ 
int mime_decode_header(const char *in, size_t inlen,
		       uint8_t *out, size_t outlen, char *charset)
{
	const char *p, *q, *offset;
	uint8_t *r;
	char *decode, *encoding;
	size_t delen;
	int i;
	int res;
	
	offset = in + inlen;
	
	/* Do a sanity check to ensure the string is not malformed. */
	for (p = in, i = 0; p < offset; p++) { if (*p == '?') i++; }
	if (in[0] != '=' || in[inlen-1] != '=' || i != 4)
		return -1;
	
	/* Get the charset. */
	for (p = in + 2, q = p+1; *q != '?' && q < offset; q++);
	assert(MIME_MAX_CHARSET >= (q-p+1));
	strncpy(charset, p, q-p);
	charset[q-p] = '\0';
	
	/* Get the encoding. */
	for (p = q+1, q = p+1; *q != '?' && q < offset; q++);
	encoding = (char *)malloc(q-p+1);
	strncpy(encoding, p, q-p);
	encoding[q-p] = '\0';
	
	/* Get the encoded string. */
	for (p = q+1, q = p+1; *q != '?' && q < offset; q++);
	delen = q-p;
	decode = (char *)malloc(delen+1);
	strncpy(decode, p, delen);
	decode[delen] = '\0';
	
	if (!strcasecmp(encoding, "B")) {    
		res = base64_decode(decode, delen, out, outlen);
	}
	else if (!strcasecmp(encoding, "Q")) {    
		res = qp_decode(decode, delen+1, out, outlen);
		
		/**
		 * Make one more extra pass to handle the "_" to SPACE variant 
		 * that RFC2047 defines for Quoted Printable decoding.
		 */
		for (r = out; r < out+res; r++) {
			if (*r == '_')
				*r = (uint8_t)' ';
		}
	}
	else
		return -1;
	
	free(decode);
	return res;
}

void plustospace(char *string, int size)
{
	char *p;
	
	for (p = string; p < string+size && *p != 0; p++) {
		if (*p == '+') 
			*p = ' ';
	}
}

int mime_encode_header(const uint8_t *in, size_t inlen, char *out, size_t outlen)
{
	int i;
	const char *q, *p;
	
	q = in + inlen;
	
	assert(outlen >= inlen);
	
	plustospace((char *)in, inlen);
	for (p = in, i = 0; p < q && *p != '\0'; p++, i++) {
		if (*p == '%') {
			out[i] = hex2char(p+1);
			p += 2;
		}
		else
			out[i] = *p;
	}
	out[i] = '\0';
	
	return i+1;
}
