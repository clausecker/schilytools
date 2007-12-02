/* @(#)sha.h	1.6 06/09/13 Copyright 1998,1999 Heiko Eissfeldt */
#ifndef	SHA_H
#define	SHA_H

/* NIST Secure Hash Algorithm */
/* heavily modified by Uwe Hollerbach <uh@alumni.caltech edu> */
/* from Peter C. Gutmann's implementation as found in */
/* Applied Cryptography by Bruce Schneier */

/* This code is in the public domain */

/* Useful defines & typedefs */

typedef	unsigned char	BYTE;		/* 8-bit quantity */
typedef	unsigned long	ULONG;		/* 32-or-more-bit quantity */

#define	SHA_BLOCKSIZE		64
#define	SHA_DIGESTSIZE		20

typedef struct {
    ULONG	digest[5];		/* message digest */
    ULONG	count_lo, count_hi;	/* 64-bit bit count */
    BYTE	data[SHA_BLOCKSIZE];	/* SHA data buffer */
    int		local;			/* unprocessed amount in data */
} SHA_INFO;

extern	void	sha_init	__PR((SHA_INFO *));
extern	void	sha_update	__PR((SHA_INFO *, BYTE *, int));
extern	void	sha_final	__PR((unsigned char [20], SHA_INFO *));

#ifdef SHA_FOR_C

#include <schily/mconfig.h>
#include <schily/stdlib.h>
#include <stdio.h>

extern	void	sha_stream	__PR((unsigned char [20], SHA_INFO *, FILE *));
extern	void	sha_print	__PR((unsigned char [20]));
extern	char	*sha_version	__PR((void));

#endif /* SHA_FOR_C */

#define	SHA_VERSION	1

#ifndef WIN32
#ifdef WORDS_BIGENDIAN
#	if SIZEOF_UNSIGNED_LONG_INT == 4
#		define SHA_BYTE_ORDER		4321
#	else
#		if SIZEOF_UNSIGNED_LONG_INT == 8
#			define	SHA_BYTE_ORDER	87654321
#		endif
#	endif
#else
#	if SIZEOF_UNSIGNED_LONG_INT == 4
#		define SHA_BYTE_ORDER		1234
#	else
#		if SIZEOF_UNSIGNED_LONG_INT == 8
#			define	SHA_BYTE_ORDER	12345678
#		endif
#	endif
#endif

#else

#define	SHA_BYTE_ORDER				1234

#endif

#endif /* SHA_H */
