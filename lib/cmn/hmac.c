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
 * @(#)hmac.c: hashed message authentication code (RFC2104 HMAC) routines
 * $Id: hmac.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#include <proto.h>
#include <md5.h>
#include <sha1.h>
#include <hmac.h>

static const char *hmac_hex = "0123456789abcdef";

/**
 * RFC 2104 HMAC hashing for md5 digest
 * Accepts: text to hash
 *          text length
 *          key
 *          key length
 *          digest buffer
 * Returns: hash as text, always
 */
unsigned char *hmac_md5(const uint8_t *text, size_t text_len, const uint8_t *key,
			size_t key_len, uint8_t digest[MD5_DIGESTSIZE])
{
	int i, j;
	static uint8_t hshbuf[2*MD5_DIGESTSIZE + 1];
	char *s;
	md5_t ctx;
	uint8_t k_ipad[MD5_BLOCKSIZE+1], k_opad[MD5_BLOCKSIZE+1];
	
	if (key_len > MD5_BLOCKSIZE) {		/* key longer than pad length? */
		md5_init(&ctx);			/* yes, set key as MD5(key) */
		md5_update(&ctx, (uint8_t *)key, key_len);
		md5_final(digest, &ctx);
		key = (char *)digest;
		key_len = MD5_DIGESTSIZE;
	}
	
	memcpy(k_ipad, key, key_len);		/* store key in pads */
	memset(k_ipad+key_len, 0, (MD5_BLOCKSIZE+1)-key_len);
	memcpy(k_opad, k_ipad, MD5_BLOCKSIZE+1);
	/* XOR key with ipad and opad values */
	for (i = 0; i < MD5_BLOCKSIZE; i++) {	/* for each byte of pad */
		k_ipad[i] ^= 0x36;		/* XOR key with ipad */
		k_opad[i] ^= 0x5c;		/*  and opad values */
	}
	
	md5_init(&ctx);		/* inner MD5: hash ipad and text */
	md5_update(&ctx, k_ipad, MD5_BLOCKSIZE);
	md5_update(&ctx, (uint8_t *)text, text_len);
	md5_final(digest, &ctx);
	md5_init(&ctx);		/* outer MD5: hash opad and inner results */
	md5_update(&ctx, k_opad, MD5_BLOCKSIZE);
	md5_update(&ctx, digest, MD5_DIGESTSIZE);
	md5_final(digest, &ctx);
	
	/* convert to printable hex */
	for (i = 0, s = hshbuf; i < MD5_DIGESTSIZE; i++) {
		*s++ = hmac_hex[(j = digest[i]) >> 4];
		*s++ = hmac_hex[j & 0xf];
	}
	*s = '\0';	/* tie off hash text */
	
	return hshbuf;
}

#ifdef HMAC_SHA1_DATA_PROBLEMS
unsigned int sha1_data_problems = 0;
#endif

unsigned char *hmac_sha1(const uint8_t *text, size_t text_len, const uint8_t *key,
			 size_t key_len, uint8_t digest[SHA1_DIGESTSIZE])
{
        sha1_t context;
	int i, j;
	static uint8_t hshbuf[2*SHA1_DIGESTSIZE + 1];
	char *s;
        uint8_t k_ipad[SHA1_BLOCKSIZE+1], k_opad[SHA1_BLOCKSIZE+1];
        unsigned char tk[SHA1_DIGESTSIZE];

        /* if key is longer than 64 bytes reset it to key=SHA1(key) */
        if (key_len > SHA1_BLOCKSIZE) {
                sha1_t tctx;
                sha1_init(&tctx);
                sha1_update(&tctx, key, key_len);
                sha1_final(tk, &tctx);
                key = tk;
                key_len = SHA1_DIGESTSIZE;
        }

#ifdef HMAC_SHA1_DATA_PROBLEMS
	if (sha1_data_problems) {
		int j, k;
		printf("\nhmac-sha1 key(%d): ", key_len);
		j = 0; k = 0;
		for (i = 0; i < key_len; i++) {
			if (j == 4) {
				printf("_");
				j=0;
			}
			j++;
			printf("%02x", key[i]);
		}
		printf("\nDATA: (%d)    ",text_len);
		
		j = 0; k = 0;
		for (i = 0; i < text_len; i++) {
			if (k == SHA1_DIGESTSIZE) {
				printf("\n            ");
				k=0;
				j=0;
			}
			if (j == 4) {
				printf("_");
				j=0;
			}
			k++;
			j++;
			printf("%02x", text[i]);
		}
		printf("\n");
	}
#endif

        /*
         * the HMAC_SHA1 transform looks like:
         *
         * SHA1(K XOR opad, SHA1(K XOR ipad, text))
         *
         * where K is an n byte key
         * ipad is the byte 0x36 repeated 64 times
         * opad is the byte 0x5c repeated 64 times
         * and text is the data being protected
         */

        /* start out by storing key in pads */
        memset(k_ipad, 0, sizeof (k_ipad));
        memset(k_opad, 0, sizeof (k_opad));
        memcpy(k_ipad, key, key_len);
        memcpy(k_opad, key, key_len);

        /* XOR key with ipad and opad values */
        for (i = 0; i < SHA1_BLOCKSIZE; i++) {
                k_ipad[i] ^= 0x36;
                k_opad[i] ^= 0x5c;
        }
        /*
         * perform inner SHA1
         */
        sha1_init(&context);                   /* init context for 1st
                                              * pass */
        sha1_update(&context, k_ipad, 64);      /* start with inner pad */
        sha1_update(&context, text, text_len); /* then text of datagram */
        sha1_final(digest, &context);          /* finish up 1st pass */
        /* perform outer MD5 */
        sha1_init(&context);                   /* init context for 2nd
                                              * pass */
        sha1_update(&context, k_opad, 64);     /* start with outer pad */
        sha1_update(&context, digest, 20);     /* then results of 1st
                                              * hash */
        sha1_final(digest, &context);          /* finish up 2nd pass */

#ifdef HMAC_SHA1_DATA_PROBLEMS
	if (sha1_data_problems)
	{
		int j = 0;
		printf("\nhmac-sha1 mac(20): ");
		for (i = 0; i < 20; i++) {
			if (j == 4) {
				printf("_");
				j=0;
			}
			j++;
			
			printf("%02x", digest[i]);
		}
		printf("\n");
	}
#endif
	
	/* convert to printable hex */
	for (i = 0, s = hshbuf; i < MD5_DIGESTSIZE; i++) {
		*s++ = hmac_hex[(j = digest[i]) >> 4];
		*s++ = hmac_hex[j & 0xf];
	}
	*s = '\0';	/* tie off hash text */
	
	return hshbuf;
}

#ifdef TEST
#include <stdio.h>
#include <string.h>

char *prompt(char *output, char *input);
/*
 * Test Vectors (Trailing '\0' of a character string not included in test):
 *
 *  key =         "Jefe"
 *  data =        "what do ya want for nothing?"
 *  data_len =    28 bytes
 *  digest =
 *
 *  key =         0xAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
 *
 *  key_len       16 bytes
 *  data =        0xDDDDDDDDDDDDDDDDDDDD...
 *                ..DDDDDDDDDDDDDDDDDDDD...
 *                ..DDDDDDDDDDDDDDDDDDDD...
 *                ..DDDDDDDDDDDDDDDDDDDD...
 *                ..DDDDDDDDDDDDDDDDDDDD
 *  data_len =    50 bytes
 *  digest =      0x56be34521d144c88dbb8c733f0e8b3f6
 */
int test_hmac_sha1()
{
	uint8_t txt[1024], key[1024];
	int tlen, klen;
	printf("HMAC-SHA1 Sample! \r\n");
	while (1) {
		tlen = strlen(prompt("text", txt));
		klen = strlen(prompt("key", key));
		printf("hmac_sha1 result: %s\r\n", hmac_sha1(txt, tlen, key, klen));
	}
	return 0;
}

int test_hmac_md5()
{
	uint8_t txt[1024], key[1024];
	int tlen, klen;
	printf("HMAC-MD5 Sample! \r\n");
	while (1) {
		tlen = strlen(prompt("text", txt));
		klen = strlen(prompt("key", key));
		printf("hmac_md5 result: %s\r\n", hmac_md5(txt, tlen, key, klen));
	}
	return 0;
}

int main(int argc, char* argv[])
{
	test_hmac_md5();
}

char *prompt(char *output, char *input)
{
	printf("%s:", output);
	return gets(input);
}

#endif /* TEST */

