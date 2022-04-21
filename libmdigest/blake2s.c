/* @(#)blake2s.c	1.0 04/21/22 2022 N. Sonack */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)blake2s.c	1.0 04/21/22 2022 N. Sonack ";
#endif

/* Blake2s implementation taken from the RFC7693 */

/* blake2s.c                                   */
/* A simple Blake2s Reference Implementation.  */

#include <schily/types.h>
#include "byte_order.h"
#include <schily/blake2.h>

/* Little-endian byte access. */

#define B2S_GET32(p)					\
	(((UInt32_t) ((UInt8_t *) (p))[0]) ^		\
	 (((UInt32_t) ((UInt8_t *) (p))[1]) << 8) ^	\
	 (((UInt32_t) ((UInt8_t *) (p))[2]) << 16) ^	\
	 (((UInt32_t) ((UInt8_t *) (p))[3]) << 24))

/* Mixing function G. */

#define B2S_G(a, b, c, d, x, y) {			\
		v[a] = v[a] + v[b] + x;			\
		v[d] = ROTR32(v[d] ^ v[a], 16);		\
		v[c] = v[c] + v[d];			\
		v[b] = ROTR32(v[b] ^ v[c], 12);		\
		v[a] = v[a] + v[b] + y;			\
		v[d] = ROTR32(v[d] ^ v[a], 8);		\
		v[c] = v[c] + v[d];			\
		v[b] = ROTR32(v[b] ^ v[c], 7); }

/* Initialization Vector. */

static const UInt32_t BLAKE2s_iv[8] =
{
	0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
	0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

/* Compression function. "last" flag indicates last block. */

static void
BLAKE2s_compress(ctx, last)
	BLAKE2s_CTX	*ctx;
	int		last;
{
	const UInt8_t sigma[10][16] = {
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
		{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
		{ 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
		{ 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
		{ 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
		{ 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
		{ 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
		{ 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
		{ 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
		{ 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 }
	};
	int i;
	UInt32_t v[16], m[16];

	for (i = 0; i < 8; i++) {           /* init work variables */
		v[i] = ctx->h[i];
		v[i + 8] = BLAKE2s_iv[i];
	}

	v[12] ^= ctx->t[0];                 /* low 32 bits of offset */
	v[13] ^= ctx->t[1];                 /* high 32 bits */
	if (last)                           /* last block flag set ? */
		v[14] = ~v[14];

	for (i = 0; i < 16; i++)            /* get little-endian words */
		m[i] = B2S_GET32(&ctx->b[4 * i]);

	for (i = 0; i < 10; i++) {          /* ten rounds */
		B2S_G( 0, 4,  8, 12, m[sigma[i][ 0]], m[sigma[i][ 1]]);
		B2S_G( 1, 5,  9, 13, m[sigma[i][ 2]], m[sigma[i][ 3]]);
		B2S_G( 2, 6, 10, 14, m[sigma[i][ 4]], m[sigma[i][ 5]]);
		B2S_G( 3, 7, 11, 15, m[sigma[i][ 6]], m[sigma[i][ 7]]);
		B2S_G( 0, 5, 10, 15, m[sigma[i][ 8]], m[sigma[i][ 9]]);
		B2S_G( 1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
		B2S_G( 2, 7,  8, 13, m[sigma[i][12]], m[sigma[i][13]]);
		B2S_G( 3, 4,  9, 14, m[sigma[i][14]], m[sigma[i][15]]);
	}

	for( i = 0; i < 8; ++i )
		ctx->h[i] ^= v[i] ^ v[i + 8];
}

/*
 * Initialize the hashing context "ctx" with optional key "key".
 *      1 <= outlen <= 32 gives the digest size in bytes.
 *      Secret key (also <= 32 bytes) is optional (keylen = 0).
 */
int
BLAKE2s_Init(ctx, outlen, key, keylen)
	BLAKE2s_CTX	*ctx;
	size_t		outlen;
	const void	*key;
	size_t		keylen;     /* (keylen=0: no key) */
{
	size_t i;

	if (outlen == 0 || outlen > 32 || keylen > 32)
		return -1;                      /* illegal parameters */

	for (i = 0; i < 8; i++)             /* state, "param block" */
		ctx->h[i] = BLAKE2s_iv[i];
	ctx->h[0] ^= 0x01010000 ^ (keylen << 8) ^ outlen;

	ctx->t[0] = 0;                      /* input count low word */
	ctx->t[1] = 0;                      /* input count high word */
	ctx->c = 0;                         /* pointer within buffer */
	ctx->outlen = outlen;

	for (i = keylen; i < 64; i++)       /* zero input block */
		ctx->b[i] = 0;
	if (keylen > 0) {
		BLAKE2s_Update(ctx, key, keylen);
		ctx->c = 64;                    /* at the end */
	}

	return 0;
}


/*
 * Add "inlen" bytes from "in" into the hash.
 */
void
BLAKE2s_Update(ctx, in, inlen)
	BLAKE2s_CTX	*ctx;
	const void	*in;
	size_t		inlen;     /* data bytes */
{
	size_t i;

	for (i = 0; i < inlen; i++) {
		if (ctx->c == 64) {                 /* buffer full ? */
			ctx->t[0] += ctx->c;        /* add counters */
			if (ctx->t[0] < ctx->c)     /* carry overflow ? */
				ctx->t[1]++;        /* high word */
			BLAKE2s_compress(ctx, 0);   /* compress (not last) */
			ctx->c = 0;                 /* counter to zero */
		}
		ctx->b[ctx->c++] = ((const UInt8_t *) in)[i];
	}
}

/*
 * Generate the message digest (size given in init).
 *      Result placed in "out".
 */
void
BLAKE2s_Final(ctx, out)
	BLAKE2s_CTX	*ctx;
	void		*out;
{
	size_t i;

	ctx->t[0] += ctx->c;                /* mark last block offset */
	if (ctx->t[0] < ctx->c)             /* carry overflow */
		ctx->t[1]++;                /* high word */

	while (ctx->c < 64)                 /* fill up with zeros */
		ctx->b[ctx->c++] = 0;
	BLAKE2s_compress(ctx, 1);           /* final block flag = 1 */

	/* little endian convert and store */
	for (i = 0; i < ctx->outlen; i++) {
		((UInt8_t *) out)[i] =
			(ctx->h[i >> 2] >> (8 * (i & 3))) & 0xFF;
	}
}

/* Convenience functions for mdigest(1L) */
int
BLAKE2s_128_Init(ctx)
	BLAKE2s_CTX	*ctx;
{
	return (BLAKE2s_Init(ctx, BLAKE2S_128_DIGEST_LENGTH, NULL, 0));
}

int
BLAKE2s_256_Init(ctx)
	BLAKE2s_CTX	*ctx;
{
	return (BLAKE2s_Init(ctx, BLAKE2S_256_DIGEST_LENGTH, NULL, 0));
}

void
BLAKE2s_128_Final(out, ctx)
	UInt8_t		out[BLAKE2S_128_DIGEST_LENGTH];
	BLAKE2s_CTX	*ctx;
{
	BLAKE2s_Final(ctx, out);
}

void
BLAKE2s_256_Final(out, ctx)
	UInt8_t		out[BLAKE2S_256_DIGEST_LENGTH];
	BLAKE2s_CTX	*ctx;
{
	BLAKE2s_Final(ctx, out);
}
