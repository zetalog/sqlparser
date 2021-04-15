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
 * @(#)sha1.h: NIST proposed secure hash algorithm interfaces
 * $Id: sha1.h,v 1.3 2007-10-29 02:14:01 zhenglv Exp $
 */

#ifndef __SHA1_H_INCLUDE__
#define __SHA1_H_INCLUDE__

#ifdef __cplusplus
extern "C" {
#endif /* __cpluscplus */

/* The SHA1 block size and message digest sizes, in bytes */

#define SHA1_BLOCKSIZE	64
#define SHA1_DIGESTSIZE	20

/* The structure for storing SHA1 context */

typedef struct _sha1_t {
	uint32_t state[5];	/* Message digest */
	uint32_t chigh;		/* high 32bits of byte count */
	uint32_t clow;		/* low 32bits of byte count */
	uint32_t data[16];	/* SHA1 data buffer */
} sha1_t;

void sha1_init(sha1_t *ctx);
void sha1_update(sha1_t *ctx, const uint8_t *buffer, size_t count);
void sha1_final(uint8_t *digest, sha1_t *ctx);

#ifdef __cplusplus
}
#endif /* __cpluscplus */

#endif /* __SHA1_H_INCLUDE__ */
