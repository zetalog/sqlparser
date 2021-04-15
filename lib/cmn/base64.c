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
 * @(#)base64.c: base64 format MIME encoding routines
 * $Id: base64.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#include <proto.h>
#include <base64.h>

static uint8_t basis_64[] = 
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* with \r\n per 60 bytes */
unsigned char *mime_binary(void *src, unsigned long srcl, unsigned long *len)
{
	unsigned char *ret,*d;
	unsigned char *s = (unsigned char *) src;
	char *v = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned long i = ((srcl + 2) / 3) * 4;
	*len = i += 2 * ((i / 60) + 1);
	d = ret = (unsigned char *) malloc ((size_t) ++i);
	/* process tuplets */
	for (i = 0; srcl >= 3; s += 3, srcl -= 3) {
		*d++ = v[s[0] >> 2];	/* byte 1: high 6 bits (1) */
		/* byte 2: low 2 bits (1), high 4 bits (2) */
		*d++ = v[((s[0] << 4) + (s[1] >> 4)) & 0x3f];
		/* byte 3: low 4 bits (2), high 2 bits (3) */
		*d++ = v[((s[1] << 2) + (s[2] >> 6)) & 0x3f];
		*d++ = v[s[2] & 0x3f];	/* byte 4: low 6 bits (3) */
		if ((++i) == 15) {		/* output 60 characters? */
			i = 0;			/* restart line break count, insert CRLF */
			*d++ = '\015'; *d++ = '\012';
		}
	}
	if (srcl) {
		*d++ = v[s[0] >> 2];	/* byte 1: high 6 bits (1) */
		/* byte 2: low 2 bits (1), high 4 bits (2) */
		*d++ = v[((s[0] << 4) + (--srcl ? (s[1] >> 4) : 0)) & 0x3f];
		/* byte 3: low 4 bits (2), high 2 bits (3) */
		*d++ = srcl ? v[((s[1] << 2) + (--srcl ? (s[2] >> 6) : 0)) & 0x3f] : '=';
		/* byte 4: low 6 bits (3) */
		*d++ = srcl ? v[s[2] & 0x3f] : '=';
		if (srcl) srcl--;		/* count third character if processed */
		if ((++i) == 15) {		/* output 60 characters? */
			i = 0;			/* restart line break count, insert CRLF */
			*d++ = '\015'; *d++ = '\012';
		}
	}
	*d++ = '\015'; *d++ = '\012';	/* insert final CRLF */
	*d = '\0';			/* tie off string */
	assert (((unsigned long) (d - ret)) == *len);
	return ret;			/* return the resulting string */
}

int base64_encode(uint8_t *in, size_t inlen, uint8_t *out, size_t outlen)
{
	unsigned int i = 0, j = 0;
	int pad;

	assert(outlen >= ((inlen+1) * 4 / 3));

	while (i < inlen) {
		pad = 3 - (inlen - i);
		if (pad == 2) {
			out[j  ] = basis_64[in[i]>>2];
			out[j+1] = basis_64[(in[i] & 0x03) << 4];
			out[j+2] = '=';
			out[j+3] = '=';
		} else if (pad == 1) {
			out[j  ] = basis_64[in[i]>>2];
			out[j+1] = basis_64[((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4)];
			out[j+2] = basis_64[(in[i+1] & 0x0f) << 2];
			out[j+3] = '=';
		} else{
			out[j  ] = basis_64[in[i]>>2];
			out[j+1] = basis_64[((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4)];
			out[j+2] = basis_64[((in[i+1] & 0x0f) << 2) | ((in[i+2] & 0xc0) >> 6)];
			out[j+3] = basis_64[in[i+2] & 0x3f];
		}
		i += 3;
		j += 4;
	}
	return j;
}

/* This assumes that an uint8_t is exactly 8 bits. Not portable code! :-) */
static uint8_t index_64[128] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 62,   0xff, 0xff, 0xff, 63,
	52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,
	15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
	41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,   0xff, 0xff, 0xff, 0xff, 0xff
};

#define char64(c)  ((c > 127) ? 0xff : index_64[(c)])

int base64_decode(uint8_t *in, size_t inlen, uint8_t *out, size_t outlen)
{
	unsigned int i = 0, j = 0;
	int pad;
	uint8_t	c[4];

	assert((outlen+1) >= (inlen * 3 / 4));
	assert((inlen % 4) == 0);
	while ((i + 3) < inlen) {
		pad  = 0;
		c[0] = char64(in[i  ]); pad += (c[0] == 0xff);
		c[1] = char64(in[i+1]); pad += (c[1] == 0xff);
		c[2] = char64(in[i+2]); pad += (c[2] == 0xff);
		c[3] = char64(in[i+3]); pad += (c[3] == 0xff);
		if (pad == 2) {
			out[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
			out[j]   = (c[1] & 0x0f) << 4;
		} else if (pad == 1) {
			out[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
			out[j++] = ((c[1] & 0x0f) << 4) | ((c[2] & 0x3c) >> 2);
			out[j]   = (c[2] & 0x03) << 6;
		} else {
			out[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
			out[j++] = ((c[1] & 0x0f) << 4) | ((c[2] & 0x3c) >> 2);
			out[j++] = ((c[2] & 0x03) << 6) | (c[3] & 0x3f);
		}
		i += 4;
	}
	return j;
}
