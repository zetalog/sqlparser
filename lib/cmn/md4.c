/*
 * ZETALOG's Personal COPYRIGHT
 *
 * Copyright (c) 2007
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
 * @(#)md4.c: message digest 4 (MD4) routines
 * $Id: md4.c,v 1.1 2007-04-20 18:35:43 zhenglv Exp $
 */

#include <proto.h>
#include <md4.h>

static void md4_transform(uint32_t *state, const uint32_t in[MD4_DIGESTSIZE]);

#ifndef WORDS_BIGENDIAN
#define byteReverse(buf, len)	/* Nothing */
#else
/* Note: this code is harmless on little-endian machines */
static void byteReverse(unsigned char *buf, unsigned longs)
{
	uint32_t t;

	do {
		t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
				((unsigned) buf[1] << 8 | buf[0]);
		*(uint32_t *) buf = t;
		buf += 4;
	} while (--longs);
}
#endif

#define rotl32(x,n)   (((x) << ((uint32_t)(n))) | ((x) >> (32 - (uint32_t)(n))))

/*
 * Start MD4 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void md4_init(md4_t *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}

/*
 * update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void md4_update(md4_t *ctx, unsigned char const *buf, unsigned len)
{
	register uint32_t t;

	/* update bitcount */
	t = ctx->bits[0];
	if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
		ctx->bits[1]++;	/* Carry from low to high */
	ctx->bits[1] += len >> 29;

	t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

	/* handle any leading odd-sized chunks */
	if (t) {
		unsigned char *p = (unsigned char *) ctx->in + t;

		t = 64 - t;
		if (len < t) {
			memcpy(p, buf, len);
			return;
		}
		memcpy(p, buf, t);
		byteReverse(ctx->in, 16);
		md4_transform(ctx->buf, (uint32_t *) ctx->in);
		buf += t;
		len -= t;
	}

	/* process data in 64-byte chunks */
	while (len >= 64) {
		memcpy(ctx->in, buf, 64);
		byteReverse(ctx->in, 16);
		md4_transform(ctx->buf, (uint32_t *) ctx->in);
		buf += 64;
		len -= 64;
	}

	/* handle any remaining bytes of data. */
	memcpy(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void md4_final(md4_t *ctx, unsigned char *digest)
{
	unsigned int count;
	unsigned char *p;

	/* compute number of bytes mod 64 */
	count = (ctx->bits[0] >> 3) & 0x3F;

	/*
	 * set the first char of padding to 0x80.  This is safe since there is
	 * always at least one byte free
	 */
	p = ctx->in + count;
	*p++ = 0x80;

	/* bytes of padding needed to make 64 bytes */
	count = 64 - 1 - count;

	/* pad out to 56 mod 64 */
	if (count < 8) {
		/* two lots of padding:  pad the first block to 64 bytes */
		memset(p, 0, count);
		byteReverse(ctx->in, 16);
		md4_transform(ctx->buf, (uint32_t *) ctx->in);

		/* now fill the next block with 56 bytes */
		memset(ctx->in, 0, 56);
	} else {
		/* pad block to 56 bytes */
		memset(p, 0, count - 8);
	}
	byteReverse(ctx->in, 14);

	/* append length in bits and transform */
	((uint32_t *) ctx->in)[14] = ctx->bits[0];
	((uint32_t *) ctx->in)[15] = ctx->bits[1];

	md4_transform(ctx->buf, (uint32_t *) ctx->in);
	byteReverse((unsigned char *) ctx->buf, 4);

	if (digest != NULL)
		memcpy(digest, ctx->buf, 16);
	memset(ctx, 0, sizeof(ctx));	/* In case it's sensitive */
}

/* the three core functions */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

#define FF(a, b, c, d, x, s) { \
	(a) += F ((b), (c), (d)) + (x); \
	(a) = rotl32 ((a), (s)); \
}
#define GG(a, b, c, d, x, s) { \
	(a) += G ((b), (c), (d)) + (x) + (uint32_t)0x5a827999; \
	(a) = rotl32 ((a), (s)); \
}
#define HH(a, b, c, d, x, s) { \
	(a) += H ((b), (c), (d)) + (x) + (uint32_t)0x6ed9eba1; \
	(a) = rotl32 ((a), (s)); \
}

/* the core of the MD4 algorithm */
void md4_transform(uint32_t buf[4], uint32_t const in[16])
{
	register uint32_t a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	FF(a, b, c, d, in[0], 3);	/* 1 */
	FF(d, a, b, c, in[1], 7);	/* 2 */
	FF(c, d, a, b, in[2], 11);	/* 3 */
	FF(b, c, d, a, in[3], 19);	/* 4 */
	FF(a, b, c, d, in[4], 3);	/* 5 */
	FF(d, a, b, c, in[5], 7);	/* 6 */
	FF(c, d, a, b, in[6], 11);	/* 7 */
	FF(b, c, d, a, in[7], 19);	/* 8 */
	FF(a, b, c, d, in[8], 3);	/* 9 */
	FF(d, a, b, c, in[9], 7);	/* 10 */
	FF(c, d, a, b, in[10], 11);	/* 11 */
	FF(b, c, d, a, in[11], 19);	/* 12 */
	FF(a, b, c, d, in[12], 3);	/* 13 */
	FF(d, a, b, c, in[13], 7);	/* 14 */
	FF(c, d, a, b, in[14], 11);	/* 15 */
	FF(b, c, d, a, in[15], 19);	/* 16 */

	GG(a, b, c, d, in[0], 3);	/* 17 */
	GG(d, a, b, c, in[4], 5);	/* 18 */
	GG(c, d, a, b, in[8], 9);	/* 19 */
	GG(b, c, d, a, in[12], 13);	/* 20 */
	GG(a, b, c, d, in[1], 3);	/* 21 */
	GG(d, a, b, c, in[5], 5);	/* 22 */
	GG(c, d, a, b, in[9], 9);	/* 23 */
	GG(b, c, d, a, in[13], 13);	/* 24 */
	GG(a, b, c, d, in[2], 3);	/* 25 */
	GG(d, a, b, c, in[6], 5);	/* 26 */
	GG(c, d, a, b, in[10], 9);	/* 27 */
	GG(b, c, d, a, in[14], 13);	/* 28 */
	GG(a, b, c, d, in[3], 3);	/* 29 */
	GG(d, a, b, c, in[7], 5);	/* 30 */
	GG(c, d, a, b, in[11], 9);	/* 31 */
	GG(b, c, d, a, in[15], 13);	/* 32 */

	HH(a, b, c, d, in[0], 3);	/* 33 */
	HH(d, a, b, c, in[8], 9);	/* 34 */
	HH(c, d, a, b, in[4], 11);	/* 35 */
	HH(b, c, d, a, in[12], 15);	/* 36 */
	HH(a, b, c, d, in[2], 3);	/* 37 */
	HH(d, a, b, c, in[10], 9);	/* 38 */
	HH(c, d, a, b, in[6], 11);	/* 39 */
	HH(b, c, d, a, in[14], 15);	/* 40 */
	HH(a, b, c, d, in[1], 3);	/* 41 */
	HH(d, a, b, c, in[9], 9);	/* 42 */
	HH(c, d, a, b, in[5], 11);	/* 43 */
	HH(b, c, d, a, in[13], 15);	/* 44 */
	HH(a, b, c, d, in[3], 3);	/* 45 */
	HH(d, a, b, c, in[11], 9);	/* 46 */
	HH(c, d, a, b, in[7], 11);	/* 47 */
	HH(b, c, d, a, in[15], 15);	/* 48 */

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}
