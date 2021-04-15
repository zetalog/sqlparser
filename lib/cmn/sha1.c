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
 * @(#)sha1.c: NIST proposed secure hash algorithm (SHA1) functions
 * $Id: sha1.c,v 1.3 2007-10-29 02:14:01 zhenglv Exp $
 */

#include <proto.h>
#include <sha1.h>

/* The SHA1 f()-functions */

#define f1(x,y,z)	((x & y) | (~x & z ))		/* Rounds  0-19 */
#define f2(x,y,z)	(x ^ y ^ z)			/* Rounds 20-39 */
#define f3(x,y,z)	((x & y) | (x & z) | (y & z))	/* Rounds 40-59 */
#define f4(x,y,z)	(x ^ y ^ z)	/* Rounds 60-79 */

/* The SHA1 Mysterious Constants */

#define K1	0x5A827999L	/* Rounds  0-19 */
#define K2	0x6ED9EBA1L	/* Rounds 20-39 */
#define K3	0x8F1BBCDCL	/* Rounds 40-59 */
#define K4	0xCA62C1D6L	/* Rounds 60-79 */

/* SHA1 initial values */

#define h0init	0x67452301L
#define h1init	0xEFCDAB89L
#define h2init	0x98BADCFEL
#define h3init	0x10325476L
#define h4init	0xC3D2E1F0L

/* 32-bit rotate - kludged with shifts */

#define S(n,X)	((X << n) | (X >>(32 - n)))

/* The initial expanding function */

#define expand(count)	W[count] = W[count - 3] ^ W[count - 8] ^ W[count - 14] ^ W[count - 16]

/* The four SHA1 sub-rounds */

#define RND1(count)	\
	{ \
		temp = S(5, A) + f1(B, C, D) + E + W[count] + K1; \
		E = D, D = C; \
		C = S(30, B); \
		B = A, A = temp; \
	}

#define RND2(count)	\
	{ \
		temp = S(5, A) + f2(B, C, D) + E + W[count] + K2; \
		E = D, D = C; \
		C = S(30, B); \
		B = A, A = temp; \
	}

#define RND3(count)	\
	{ \
		temp = S(5, A) + f3(B, C, D) + E + W[count] + K3; \
		E = D, D = C; \
		C = S(30, B); \
		B = A, A = temp; \
	}

#define RND4(count)	\
	{ \
		temp = S(5, A) + f4(B, C, D) + E + W[count] + K4; \
		E = D; \
		D = C; \
		C = S(30, B); \
		B = A; \
		A = temp; \
	}

/* The two buffers of 5 32-bit words */

uint32_t h0, h1, h2, h3, h4;
uint32_t A, B, C, D, E;

static void byte_reverse(uint32_t *buffer, size_t byteCount);
void sha1_transform(sha1_t *ctx);

/* Initialize the SHA1 values */
void sha1_init(sha1_t *ctx)
{
	/* Set the h-vars to their initial values */
	ctx->state[0] = h0init;
	ctx->state[1] = h1init;
	ctx->state[2] = h2init;
	ctx->state[3] = h3init;
	ctx->state[4] = h4init;
	
	/* Initialise bit count */
	ctx->clow = ctx->chigh = 0L;
}

/*
 * Perform the SHA1 transformation.  Note that this code, like MD5, seems to
 * break some optimizing compilers - it may be necessary to split it into
 * sections, eg based on the four subrounds
 */
void sha1_transform(sha1_t *ctx)
{
	uint32_t W[80], temp;
	int i;
	
	/* Step A.	Copy the data buffer into the local work buffer */
	for (i = 0; i < 16; i++)
		W[i] = ctx->data[i];
	
	/* Step B.	Expand the 16 words into 64 temporary data words */
	expand(16); expand(17); expand(18); expand(19); expand(20);
	expand(21); expand(22); expand(23); expand(24); expand(25);
	expand(26); expand(27); expand(28); expand(29); expand(30);
	expand(31); expand(32); expand(33); expand(34); expand(35);
	expand(36); expand(37); expand(38); expand(39); expand(40);
	expand(41); expand(42); expand(43); expand(44); expand(45);
	expand(46); expand(47); expand(48); expand(49); expand(50);
	expand(51); expand(52); expand(53); expand(54); expand(55);
	expand(56); expand(57); expand(58); expand(59); expand(60);
	expand(61); expand(62); expand(63); expand(64); expand(65);
	expand(66); expand(67); expand(68); expand(69); expand(70);
	expand(71); expand(72); expand(73); expand(74); expand(75);
	expand(76); expand(77); expand(78); expand(79);
	
	/* Step C.	Set up first buffer */
	A = ctx->state[0];
	B = ctx->state[1];
	C = ctx->state[2];
	D = ctx->state[3];
	E = ctx->state[4];
	
	/* Step D.	Serious mangling, divided into four sub-rounds */
	RND1 (0); RND1 (1); RND1 (2); RND1 (3);
	RND1 (4); RND1 (5); RND1 (6); RND1 (7);
	RND1 (8); RND1 (9); RND1(10); RND1(11);
	RND1(12); RND1(13); RND1(14); RND1(15);
	RND1(16); RND1(17); RND1(18); RND1(19);
	
	RND2(20); RND2(21); RND2(22); RND2(23);
	RND2(24); RND2(25); RND2(26); RND2(27);
	RND2(28); RND2(29); RND2(30); RND2(31);
	RND2(32); RND2(33); RND2(34); RND2(35);
	RND2(36); RND2(37); RND2(38); RND2(39);
	
	RND3(40); RND3(41); RND3(42); RND3(43);
	RND3(44); RND3(45); RND3(46); RND3(47);
	RND3(48); RND3(49); RND3(50); RND3(51);
	RND3(52); RND3(53); RND3(54); RND3(55);
	RND3(56); RND3(57); RND3(58); RND3(59);
	
	RND4(60); RND4(61); RND4(62); RND4(63);
	RND4(64); RND4(65); RND4(66); RND4(67);
	RND4(68); RND4(69); RND4(70); RND4(71);
	RND4(72); RND4(73); RND4(74); RND4(75);
	RND4(76); RND4(77); RND4(78); RND4(79);
	
	/* Step E.	Build message digest */
	ctx->state[0] += A;
	ctx->state[1] += B;
	ctx->state[2] += C;
	ctx->state[3] += D;
	ctx->state[4] += E;
}

static void byte_reverse(uint32_t *buffer, size_t byteCount)
{
	uint32_t value;
	unsigned int count;

	/*
	 * Find out what the byte order is on this machine.
	 * Big endian is for machines that place the most significant byte
	 * first (eg. Sun SPARC). Little endian is for machines that place
	 * the least significant byte first (eg. VAX).
	 *
	 * We figure out the byte order by stuffing a 2 byte string into a
	 * short and examining the left byte. '@' = 0x40  and  'P' = 0x50
	 * If the left byte is the 'high' byte, then it is 'big endian'.
	 * If the left byte is the 'low' byte, then the machine is 'little
	 * endian'.
	 *
	 *                          -- Shawn A. Clifford (sac@eng.ufl.edu)
	 */

	/*
	 * Several bugs fixed       -- Pat Myrto (pat@rwing.uucp)
	 */

	if ((*(uint16_t *)("@P") >> 8) == '@')
		return;
	
	byteCount /= sizeof (uint32_t);
	for (count = 0; count < byteCount; count++) {
		value = (buffer[count] << 16) | (buffer[count] >> 16);
		buffer[count] = ((value & 0xFF00FF00L) >> 8) | ((value & 0x00FF00FFL) << 8);
	}
}

/*
 * Update SHA1 for a block of data.  This code assumes that the buffer size is
 * a multiple of SHA1_BLOCKSIZE bytes long, which makes the code a lot more
 * efficient since it does away with the need to handle partial blocks
 * between calls to shsUpdate()
 */
void sha1_update(sha1_t *ctx, const uint8_t *buffer, size_t count)
{
	/* Update bitcount */
	if ((ctx->clow + ((uint32_t) count << 3)) < ctx->clow)
		ctx->chigh++;	/* Carry from low to high bitCount */
	ctx->clow += ((uint32_t) count << 3);
	ctx->chigh += ((uint32_t) count >> 29);
	
	/* Process data in SHA1_BLOCKSIZE chunks */
	while (count >= SHA1_BLOCKSIZE) {
		memcpy(ctx->data, buffer, SHA1_BLOCKSIZE);
		byte_reverse(ctx->data, SHA1_BLOCKSIZE);
		sha1_transform(ctx);
		buffer += SHA1_BLOCKSIZE;
		count -= SHA1_BLOCKSIZE;
	}
    
	/*
	 * Handle any remaining bytes of data.
	 * This should only happen once on the final lot of data
	 */
	memcpy(ctx->data, buffer, count);
}

static void sha1_encode(uint8_t *dst, uint32_t *src, size_t len)
{
	unsigned int i;
	for (i = 0; i < len; i++) {
		*dst++ = (uint8_t)(src[i] & 0xff);
		*dst++ = (uint8_t)((src[i] >> 8) & 0xff);
		*dst++ = (uint8_t)((src[i] >> 16) & 0xff);
		*dst++ = (uint8_t)((src[i] >> 24) & 0xff);
	}
}

void sha1_final(uint8_t *digest, sha1_t *ctx)
{
	int count;
	uint32_t lowBitcount = ctx->clow, highBitcount = ctx->chigh;
	
	/* Compute number of bytes mod 64 */
	count = (int)((ctx->clow >> 3) & 0x3F);
	
	/*
	* Set the first char of padding to 0x80.
	* This is safe since there is always at least one byte free
	*/
	((uint8_t *) ctx->data)[count++] = 0x80;
	
	/* Pad out to 56 mod 64 */
	if (count > 56) {
		/* Two lots of padding:  Pad the first block to 64 bytes */
		memset((uint8_t *) ctx->data + count, 0, 64 - count);
		byte_reverse(ctx->data, SHA1_BLOCKSIZE);
		sha1_transform(ctx);
		
		/* Now fill the next block with 56 bytes */
		memset(ctx->data, 0, 56);
	} else {
		/* Pad block to 56 bytes */
		memset((uint8_t *) ctx->data + count, 0, 56 - count);
	}
	byte_reverse(ctx->data, SHA1_BLOCKSIZE);
	
	/* Append length in bits and transform */
	ctx->data[14] = highBitcount;
	ctx->data[15] = lowBitcount;
	
	sha1_transform(ctx);
	byte_reverse(ctx->data, SHA1_DIGESTSIZE);
	
	/* store state in digest */
	sha1_encode(digest, ctx->state, 5);
	/* erase context */
	memset(ctx, 0, sizeof (sha1_t));
}

/*
 * Prints message digest buffer in shsInfo as 40 hexadecimal digits. Order is
 * from low-order byte to high-order byte of digest. Each byte is printed
 * with high-order hexadecimal digit first.
 */
void sha1_print(sha1_t *ctx)
{
	int i;
	
	for (i = 0; i < 5; i++)
		printf ("%08lx", ctx->state[i]);
}
