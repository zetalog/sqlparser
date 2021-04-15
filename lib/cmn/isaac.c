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
 * @(#)isaac.c: ISAAC random number generator
 *              (http://burtleburtle.net/bob/rand/isaac.html)
 * $Id: isaac.c,v 1.2 2007/03/10 06:55:55 cvsroot Exp $
 */

#include <proto.h>
#include <isaac.h>

/* recommend 8 for crypto, 4 for simulations */
#define ISAAC_RANDOFFSET	(8)
#define ISAAC_RANDSIZE		(1<<ISAAC_RANDOFFSET)

#define ind(mm,x)	((mm)[(x>>2)&(ISAAC_RANDSIZE-1)])
#define rngstep(mix,a,b,mm,m,m2,r,x) \
	do { \
		x = *m;  \
		a = ((a^(mix)) + *(m2++)) & 0xffffffff; \
		*(m++) = y = (ind(mm,x) + a + b) & 0xffffffff; \
		*(r++) = b = (ind(mm,y>>ISAAC_RANDOFFSET) + x) & 0xffffffff; \
	} while (0)

void isaac_rand(isaac_t *ctx)
{
	register uint32_t a,b,x,y,*m,*mm,*m2,*r,*mend;
	mm=ctx->randmem; r=ctx->randrsl;
	a = ctx->randa; b = (ctx->randb + (++ctx->randc)) & 0xffffffff;
	for (m = mm, mend = m2 = m+(ISAAC_RANDSIZE/2); m<mend; )
	{
		rngstep( a<<13, a, b, mm, m, m2, r, x);
		rngstep( a>>6 , a, b, mm, m, m2, r, x);
		rngstep( a<<2 , a, b, mm, m, m2, r, x);
		rngstep( a>>16, a, b, mm, m, m2, r, x);
	}
	for (m2 = mm; m2<mend; )
	{
		rngstep( a<<13, a, b, mm, m, m2, r, x);
		rngstep( a>>6 , a, b, mm, m, m2, r, x);
		rngstep( a<<2 , a, b, mm, m, m2, r, x);
		rngstep( a>>16, a, b, mm, m, m2, r, x);
	}
	ctx->randb = b; ctx->randa = a;
}

#define mix(a,b,c,d,e,f,g,h) \
	do { \
		a^=b<<11; d+=a; b+=c; \
		b^=c>>2;  e+=b; c+=d; \
		c^=d<<8;  f+=c; d+=e; \
		d^=e>>16; g+=d; e+=f; \
		e^=f<<10; h+=e; f+=g; \
		f^=g>>4;  a+=f; g+=h; \
		g^=h<<8;  b+=g; h+=a; \
		h^=a>>9;  c+=h; a+=b; \
	} while (0)

/* if (flag==1), then use the contents of randrsl[] to initialize mm[]. */
void isaac_init(isaac_t *ctx, int flag)
{
	int i;
	uint32_t a,b,c,d,e,f,g,h;
	uint32_t *m,*r;
	ctx->randa = ctx->randb = ctx->randc = 0;
	m = ctx->randmem;
	r = ctx->randrsl;
	/* the golden ratio */
	a = b = c = d = e = f = g = h = 0x9e3779b9;
	
	/* scramble it */
	for (i = 0; i < 4; ++i)
		mix(a, b, c, d, e, f, g, h);
	
	if (flag) {
		/* initialize using the contents of r[] as the seed */
		for (i = 0; i < ISAAC_RANDSIZE; i += 8) {
			a += r[i  ]; b += r[i+1]; c += r[i+2]; d += r[i+3];
			e += r[i+4]; f += r[i+5]; g += r[i+6]; h += r[i+7];
			mix(a, b, c, d, e, f, g, h);
			m[i  ] = a; m[i+1] = b; m[i+2] = c; m[i+3] = d;
			m[i+4] = e; m[i+5] = f; m[i+6] = g; m[i+7] = h;
		}
		/* do a second pass to make all of the seed affect all of m */
		for (i = 0; i < ISAAC_RANDSIZE; i += 8) {
			a += m[i  ]; b += m[i+1]; c += m[i+2]; d += m[i+3];
			e += m[i+4]; f += m[i+5]; g += m[i+6]; h += m[i+7];
			mix(a,b,c,d,e,f,g,h);
			m[i  ] = a; m[i+1] = b; m[i+2] = c; m[i+3] = d;
			m[i+4] = e; m[i+5] = f; m[i+6] = g; m[i+7] = h;
		}
	} else {
		for (i = 0; i < ISAAC_RANDSIZE; i += 8) {
			/* fill in mm[] with messy stuff */
			mix(a,b,c,d,e,f,g,h);
			m[i  ] = a; m[i+1] = b; m[i+2] = c; m[i+3] = d;
			m[i+4] = e; m[i+5] = f; m[i+6] = g; m[i+7] = h;
		}
	}
	
	/* fill in the first set of results */
	isaac_rand(ctx);
	/* prepare to use the first set of results */
	ctx->randcnt = ISAAC_RANDSIZE;
}

#ifdef TEST
/*
 *  For testing.  Output should be the same as
 *  http://burtleburtle.net/bob/rand/randvect.txt
 */
int main()
{
	uint32_t i,j;
	isaac_t ctx;
	
	ctx.randa = ctx.randb = ctx.randc = (uint32_t)0;
	
	for (i = 0; i < 256; ++i)
		ctx.randrsl[i] = (uint32_t)0;
	isaac_init(&ctx, 1);
	for (i = 0; i < 2; ++i) {
		isaac_rand(&ctx);
		for (j = 0; j < 256; ++j) {
			printf("%.8lx", ctx.randrsl[j]);
			if ((j&7) == 7)
				printf("\n");
		}
	}
}
#endif
