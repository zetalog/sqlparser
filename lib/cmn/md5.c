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
 * @(#)md5.c: message digest 5 (MD5) routines
 * $Id: md5.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#include <proto.h>
#include <md5.h>

static void md5_transform(uint32_t *state, uint8_t *block);
static void md5_encode(uint8_t *dst, uint32_t *src, int len);
static void md5_decode(uint32_t *dst, uint8_t *src, int len);

/* Everything after this point is derived from the RSA Data Security, Inc.
 * MD5 Message-Digest Algorithm
 */

#define RND1(a,b,c,d,x,s,ac) \
 a += ((b & c) | (d & ~b)) + x + (uint32_t) ac; \
 a &= 0xffffffff; \
 a = b + ((a << s) | (a >> (32 - s)));

#define RND2(a,b,c,d,x,s,ac) \
 a += ((b & d) | (c & ~d)) + x + (uint32_t) ac; \
 a &= 0xffffffff; \
 a = b + ((a << s) | (a >> (32 - s)));

#define RND3(a,b,c,d,x,s,ac) \
 a += (b ^ c ^ d) + x + (uint32_t) ac; \
 a &= 0xffffffff; \
 a = b + ((a << s) | (a >> (32 - s)));

#define RND4(a,b,c,d,x,s,ac) \
 a += (c ^ (b | ~d)) + x + (uint32_t) ac; \
 a &= 0xffffffff; \
 a = b + ((a << s) | (a >> (32 - s)));

/* Initialize MD5 context
 * Accepts: context to initialize
 */
void md5_init(md5_t *ctx)
{
    ctx->clow = ctx->chigh = 0; /* initialize byte count to zero */
    /* initialization constants */
    ctx->state[0] = 0x67452301; ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe; ctx->state[3] = 0x10325476;
    ctx->ptr = ctx->buf;        /* reset buffer pointer */
}


/* MD5 add data to context
 * Accepts: context
 *          input data
 *          length of data
 */
void md5_update(md5_t *ctx, const uint8_t *data, size_t len)
{
    uint32_t i = (ctx->buf + MD5_BLOCKSIZE) - ctx->ptr;
    /* update double precision number of bytes */
    if ((ctx->clow += len) < len) ctx->chigh++;
    while (i <= len) {        /* copy/transform data, 64 bytes at a time */
        memcpy(ctx->ptr, data, i);     /* fill up 64 byte chunk */
        md5_transform(ctx->state, ctx->ptr = ctx->buf);
        data += i, len -= i, i = MD5_BLOCKSIZE;
    }
    memcpy(ctx->ptr, data, len);   /* copy final bit of data in buffer */
    ctx->ptr += len;        /* update buffer pointer */
}

/* MD5 Finalization
 * Accepts: destination digest
 *          context
 */
void md5_final(uint8_t *digest, md5_t *ctx)
{
    uint32_t i, bits[2];
    bits[0] = ctx->clow << 3;	/* calculate length in bits (before padding) */
    bits[1] = (ctx->chigh << 3) + (ctx->clow >> 29);
    *ctx->ptr++ = 0x80;		/* padding byte */
    if ((i = (ctx->buf + MD5_BLOCKSIZE) - ctx->ptr) < 8) {
        memset(ctx->ptr, 0, i);	/* pad out buffer with zeros */
        md5_transform(ctx->state, ctx->buf);
        /* pad out with zeros, leaving 8 bytes */
        memset(ctx->buf, 0, MD5_BLOCKSIZE - 8);
        ctx->ptr = ctx->buf + MD5_BLOCKSIZE - 8;
    }
    else if (i -= 8) {		/* need to pad this buffer? */
        memset(ctx->ptr, 0, i);	/* yes, pad out with zeros, leaving 8 bytes */
        ctx->ptr += i;
    }
    md5_encode(ctx->ptr, bits, 2);	/* make LSB-first length */
    md5_transform(ctx->state, ctx->buf);
    /* store state in digest */
    md5_encode(digest, ctx->state, 4);
    /* erase context */
    memset(ctx, 0, sizeof (md5_t));
}

/* MD5 basic transformation
 * Accepts: state vector
 *          current 64-byte block
 */
static void md5_transform(uint32_t *state, uint8_t *block)
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    md5_decode(x, block, 16);	/* decode into 16 longs */
    /* round 1 */
    RND1(a,b,c,d,x[ 0], 7,0xd76aa478); RND1(d,a,b,c,x[ 1],12,0xe8c7b756);
    RND1(c,d,a,b,x[ 2],17,0x242070db); RND1(b,c,d,a,x[ 3],22,0xc1bdceee);
    RND1(a,b,c,d,x[ 4], 7,0xf57c0faf); RND1(d,a,b,c,x[ 5],12,0x4787c62a);
    RND1(c,d,a,b,x[ 6],17,0xa8304613); RND1(b,c,d,a,x[ 7],22,0xfd469501);
    RND1(a,b,c,d,x[ 8], 7,0x698098d8); RND1(d,a,b,c,x[ 9],12,0x8b44f7af);
    RND1(c,d,a,b,x[10],17,0xffff5bb1); RND1(b,c,d,a,x[11],22,0x895cd7be);
    RND1(a,b,c,d,x[12], 7,0x6b901122); RND1(d,a,b,c,x[13],12,0xfd987193);
    RND1(c,d,a,b,x[14],17,0xa679438e); RND1(b,c,d,a,x[15],22,0x49b40821);
    /* round 2 */
    RND2(a,b,c,d,x[ 1], 5,0xf61e2562); RND2(d,a,b,c,x[ 6], 9,0xc040b340);
    RND2(c,d,a,b,x[11],14,0x265e5a51); RND2(b,c,d,a,x[ 0],20,0xe9b6c7aa);
    RND2(a,b,c,d,x[ 5], 5,0xd62f105d); RND2(d,a,b,c,x[10], 9, 0x2441453);
    RND2(c,d,a,b,x[15],14,0xd8a1e681); RND2(b,c,d,a,x[ 4],20,0xe7d3fbc8);
    RND2(a,b,c,d,x[ 9], 5,0x21e1cde6); RND2(d,a,b,c,x[14], 9,0xc33707d6);
    RND2(c,d,a,b,x[ 3],14,0xf4d50d87); RND2(b,c,d,a,x[ 8],20,0x455a14ed);
    RND2(a,b,c,d,x[13], 5,0xa9e3e905); RND2(d,a,b,c,x[ 2], 9,0xfcefa3f8);
    RND2(c,d,a,b,x[ 7],14,0x676f02d9); RND2(b,c,d,a,x[12],20,0x8d2a4c8a);
    /* round 3 */
    RND3(a,b,c,d,x[ 5], 4,0xfffa3942); RND3(d,a,b,c,x[ 8],11,0x8771f681);
    RND3(c,d,a,b,x[11],16,0x6d9d6122); RND3(b,c,d,a,x[14],23,0xfde5380c);
    RND3(a,b,c,d,x[ 1], 4,0xa4beea44); RND3(d,a,b,c,x[ 4],11,0x4bdecfa9);
    RND3(c,d,a,b,x[ 7],16,0xf6bb4b60); RND3(b,c,d,a,x[10],23,0xbebfbc70);
    RND3(a,b,c,d,x[13], 4,0x289b7ec6); RND3(d,a,b,c,x[ 0],11,0xeaa127fa);
    RND3(c,d,a,b,x[ 3],16,0xd4ef3085); RND3(b,c,d,a,x[ 6],23, 0x4881d05);
    RND3(a,b,c,d,x[ 9], 4,0xd9d4d039); RND3(d,a,b,c,x[12],11,0xe6db99e5);
    RND3(c,d,a,b,x[15],16,0x1fa27cf8); RND3(b,c,d,a,x[ 2],23,0xc4ac5665);
    /* round 4 */
    RND4(a,b,c,d,x[ 0], 6,0xf4292244); RND4(d,a,b,c,x[ 7],10,0x432aff97);
    RND4(c,d,a,b,x[14],15,0xab9423a7); RND4(b,c,d,a,x[ 5],21,0xfc93a039);
    RND4(a,b,c,d,x[12], 6,0x655b59c3); RND4(d,a,b,c,x[ 3],10,0x8f0ccc92);
    RND4(c,d,a,b,x[10],15,0xffeff47d); RND4(b,c,d,a,x[ 1],21,0x85845dd1);
    RND4(a,b,c,d,x[ 8], 6,0x6fa87e4f); RND4(d,a,b,c,x[15],10,0xfe2ce6e0);
    RND4(c,d,a,b,x[ 6],15,0xa3014314); RND4(b,c,d,a,x[13],21,0x4e0811a1);
    RND4(a,b,c,d,x[ 4], 6,0xf7537e82); RND4(d,a,b,c,x[11],10,0xbd3af235);
    RND4(c,d,a,b,x[ 2],15,0x2ad7d2bb); RND4(b,c,d,a,x[ 9],21,0xeb86d391);
    /* update state */
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    memset(x, 0, sizeof (x));	/* erase sensitive data */
}

/* MD5 encode uint32_t into LSB-first bytes
 * Accepts: destination pointer
 *          source
 *          length of source
 */ 
static void md5_encode(uint8_t *dst, uint32_t *src, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        *dst++ = (uint8_t)(src[i] & 0xff);
        *dst++ = (uint8_t)((src[i] >> 8) & 0xff);
        *dst++ = (uint8_t)((src[i] >> 16) & 0xff);
        *dst++ = (uint8_t)((src[i] >> 24) & 0xff);
    }
}


/* MD5 decode LSB-first bytes into uint32_t
 * Accepts: destination pointer
 *          source
 *          length of destination
 */ 
static void md5_decode(uint32_t *dst, uint8_t *src, int len)
{
    int i, j;
    for (i = 0, j = 0; i < len; i++, j += 4)
        dst[i] = ((uint32_t)src[j]) | (((uint32_t)src[j+1]) << 8) |
                 (((uint32_t)src[j+2]) << 16) | (((uint32_t)src[j+3]) << 24);
}
