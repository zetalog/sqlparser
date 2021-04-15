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
 * @(#)md5.h: message digest 5 interface
 * $Id: md5.h,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#ifndef __MD5_H_INCLUDE__
#define __MD5_H_INCLUDE__

#ifdef __cpluscplus
extern "C" {
#endif /* __cpluscplus */

#include <proto.h>

/* MD5 context */
typedef struct _md5_t {
#define MD5_BLOCKSIZE	64		/* MD5 block size */
#define MD5_DIGESTSIZE	16		/* MD5 digest size */
	uint32_t chigh;			/* high 32bits of byte count */
	uint32_t clow;			/* low 32bits of byte count */
	uint32_t state[4];		/* state (ABCD) */
	uint8_t buf[MD5_BLOCKSIZE];	/* input buffer */
	uint8_t *ptr;			/* buffer position */
} md5_t;

void md5_init(md5_t *ctx);
void md5_update(md5_t *ctx, const uint8_t *data, size_t len);
void md5_final(uint8_t *digest, md5_t *ctx);

#ifdef __cpluscplus
}
#endif /* __cpluscplus */

#endif /* __MD5_H_INCLUDE__ */
