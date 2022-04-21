/* @(#)blake2s.h	1.0 04/21/22 2022 N. Sonack */
/*
 * Blake2 implementation taken from the RFC7693
 *
 * Portions Copyright (c) 2022 N. Sonack
 */

#ifndef	_SCHILY_BLAKE2S_H
#define	_SCHILY_BLAKE2S_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#include <schily/utypes.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* BLAKE2s Hashing Context and API Prototypes */

/* state context */
typedef struct {
	UInt8_t		b[64];         /* input buffer */
	UInt32_t	h[8];          /* chained state */
	UInt32_t	t[2];          /* total number of bytes */
	size_t		c;             /* pointer for b[] */
	size_t		outlen;        /* digest size */
} BLAKE2s_CTX;

/* Maybe also define all the others but eeeh */
#define BLAKE2S_128_DIGEST_LENGTH 16
#define BLAKE2S_256_DIGEST_LENGTH 32

extern int	BLAKE2s_Init	__PR((BLAKE2s_CTX *ctx, size_t outlen,
				      const void *key, size_t keylen));
extern void	BLAKE2s_Update	__PR((BLAKE2s_CTX *ctx,
				      const void *in, size_t inlen));
extern void	BLAKE2s_Final	__PR((BLAKE2s_CTX *ctx, void *out));

/* Convenience functions for mdigest(1L) */
extern int	BLAKE2s_128_Init	__PR((BLAKE2s_CTX *ctx));
extern int	BLAKE2s_256_Init	__PR((BLAKE2s_CTX *ctx));

extern void	BLAKE2s_128_Final	__PR((UInt8_t[BLAKE2S_128_DIGEST_LENGTH], BLAKE2s_CTX *ctx));
extern void	BLAKE2s_256_Final	__PR((UInt8_t[BLAKE2S_256_DIGEST_LENGTH], BLAKE2s_CTX *ctx));

#ifdef	HAVE_LONGLONG

/* BLAKE2b Hashing Context and API Prototypes */

/* state context */
typedef struct {
	UInt8_t b[128];                     /* input buffer */
	UInt64_t h[8];                      /* chained state */
	UInt64_t t[2];                      /* total number of bytes */
	size_t c;                           /* pointer for b[] */
	size_t outlen;                      /* digest size */
} BLAKE2b_CTX;

/* Maybe also define all the others but eeeh */
#define BLAKE2B_256_DIGEST_LENGTH 32
#define BLAKE2B_512_DIGEST_LENGTH 64

extern int	BLAKE2b_Init	__PR((BLAKE2b_CTX *ctx, size_t outlen,
				      const void *key, size_t keylen));
extern void	BLAKE2b_Update	__PR((BLAKE2b_CTX *ctx,
				      const void *in, size_t inlen));
extern void	BLAKE2b_Final	__PR((BLAKE2b_CTX *ctx, void *out));

/* Convenience functions for mdigest(1L) */
extern int	BLAKE2b_256_Init	__PR((BLAKE2b_CTX *ctx));
extern int	BLAKE2b_512_Init	__PR((BLAKE2b_CTX *ctx));

extern void	BLAKE2b_256_Final	__PR((UInt8_t[BLAKE2B_256_DIGEST_LENGTH], BLAKE2b_CTX *ctx));
extern void	BLAKE2b_512_Final	__PR((UInt8_t[BLAKE2B_512_DIGEST_LENGTH], BLAKE2b_CTX *ctx));

#endif	/* HAVE_LONGLONG */

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_BLAKE2_H */
