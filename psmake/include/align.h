/*
 * This file has been generated automatically
 * by @(#)align_test.c	1.5 96/02/04 Copyright 1995 J. Schilling
 * do not edit by hand.
 */

#define	ALIGN_SHORT	2	/* alignement value for (short *)	*/
#define	ALIGN_SMASK	1	/* alignement mask  for (short *)	*/

#define	ALIGN_INT	4	/* alignement value for (int *)		*/
#define	ALIGN_IMASK	3	/* alignement mask  for (int *)		*/

#define	ALIGN_LONG	8	/* alignement value for (long *)	*/
#define	ALIGN_LMASK	7	/* alignement mask  for (long *)	*/

#define	ALIGN_LLONG	8	/* alignement value for (long long *)	*/
#define	ALIGN_LLMASK	7	/* alignement mask  for (long long *)	*/

#define	ALIGN_DOUBLE	4	/* alignement value for (double *)	*/
#define	ALIGN_DMASK	3	/* alignement mask  for (double *)	*/

#define	xaligned(a, s)		((((int)(a)) & s) == 0 )
#define	x2aligned(a, b, s)	(((((int)(a)) | ((int)(b))) & s) == 0 )

#define	saligned(a)		xaligned(a, ALIGN_SMASK)
#define	s2aligned(a, b)		x2aligned(a, b, ALIGN_SMASK)

#define	ialigned(a)		xaligned(a, ALIGN_IMASK)
#define	i2aligned(a, b)		x2aligned(a, b, ALIGN_IMASK)

#define	laligned(a)		xaligned(a, ALIGN_LMASK)
#define	l2aligned(a, b)		x2aligned(a, b, ALIGN_LMASK)

#define	llaligned(a)		xaligned(a, ALIGN_LLMASK)
#define	ll2aligned(a, b)	x2aligned(a, b, ALIGN_LLMASK)

#define	daligned(a)		xaligned(a, ALIGN_DMASK)
#define	d2aligned(a, b)		x2aligned(a, b, ALIGN_DMASK)


#define	xalign(x, a, m)		( ((char *)(x)) + ( (a) - (((int)(x))&(m))) )

#define	salign(x)		xalign((x), ALIGN_SHORT, ALIGN_SMASK)
#define	ialign(x)		xalign((x), ALIGN_INT, ALIGN_IMASK)
#define	lalign(x)		xalign((x), ALIGN_LONG, ALIGN_LMASK)
#define	llalign(x)		xalign((x), ALIGN_LLONG, ALIGN_LLMASK)
#define	dalign(x)		xalign((x), ALIGN_DOUBLE, ALIGN_DMASK)
