/* @(#)getfp.c	1.20 18/04/09 Copyright 1988-2018 J. Schilling */
/*
 *	Get frame pointer
 *
 *	Copyright (c) 1988-2018 J. Schilling
 */
/*@@C@@*/

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	IS_AVOFFSET
/*
 * We usually don't like to compile a getfp() that returns junk data in case
 * we cannot scan the stack.
 * The only way to find this out is by including avoffset.h. Unfortunately, we
 * need to be able to use getfp() from avoffset.c in order to check if the
 * return value is usable or junk. To be able to do this, we include getfp.c
 * from avoffset.c and define IS_AVOFFSET before.
 */
#include <schily/avoffset.h>

#if	!defined(AV_OFFSET) || !defined(FP_INDIR)
#	ifdef	HAVE_SCANSTACK
#	undef	HAVE_SCANSTACK
#	endif
#endif
#endif

#ifdef	HAVE_SCANSTACK
#include <schily/stkframe.h>

#define	MAXWINDOWS	32
#define	NWINDOWS	7

#if defined(sparc) && defined(__GNUC__)
#	define	FP_OFF		0x10	/* some strange things on sparc gcc */
#else
#	define	FP_OFF		0
#endif

#if defined(__clang__) || \
	(defined(__GNUC__) && \
	    ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ > 7)))
#define	ATTRIBUTE_NO_SANITIZE_ADDRESS	__attribute__((no_sanitize_address))
#else
#define	ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif

EXPORT	void	**___fpoff	__PR((char *cp));

ATTRIBUTE_NO_SANITIZE_ADDRESS
EXPORT void **
getfp()
{
		long	**dummy[1];

#ifdef	sparc
	flush_reg_windows(MAXWINDOWS-2);
#endif
	return ((void **)((struct frame *)___fpoff((char *)&dummy[0]))->fr_savfp);
}

#ifdef	sparc
EXPORT int
flush_reg_windows(n)
	int	n;
{
	if (--n > 0)
		flush_reg_windows(n);
	return (0);
}
#endif

#else	/* HAVE_SCANSTACK */

EXPORT void **
getfp()
{
	raisecond("getfp", 0);
	return ((void **)0);
}

#endif	/* HAVE_SCANSTACK */
