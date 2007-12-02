/* @(#)gap.c	1.14 06/09/13 J. Schilling from cdparanoia-III-alpha9.8 */
#ifndef lint
static	char sccsid[] =
"@(#)gap.c	1.14 06/09/13 J. Schilling from cdparanoia-III-alpha9.8";

#endif
/*
 * CopyPolicy: GNU Lesser General Public License v2.1 applies
 * Copyright (C) 1997-2001 by Monty (xiphmont@mit.edu)
 * Copyright (C) 2002-2006 by J. Schilling
 *
 * Gapa analysis support code for paranoia
 *
 */

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/string.h>
#include "p_block.h"
#include "cdda_paranoia.h"
#include "gap.h"

EXPORT long	i_paranoia_overlap_r	__PR((Int16_t * buffA, Int16_t * buffB,
						long offsetA, long offsetB));
EXPORT long	i_paranoia_overlap_f	__PR((Int16_t * buffA, Int16_t * buffB,
						long offsetA, long offsetB,
						long sizeA, long sizeB));
EXPORT int	i_stutter_or_gap	__PR((Int16_t * A, Int16_t * B,
						long offA, long offB,
						long gap));
EXPORT void	i_analyze_rift_f	__PR((Int16_t * A, Int16_t * B,
						long sizeA, long sizeB,
						long aoffset, long boffset,
						long *matchA, long *matchB,
						long *matchC));
EXPORT void	i_analyze_rift_r	__PR((Int16_t * A, Int16_t * B,
						long sizeA, long sizeB,
						long aoffset, long boffset,
						long *matchA, long *matchB,
						long *matchC));

EXPORT void	analyze_rift_silence_f	__PR((Int16_t * A, Int16_t * B,
						long sizeA, long sizeB,
						long aoffset, long boffset,
						long *matchA, long *matchB));

/*
 * Gap analysis code
 */
EXPORT long
i_paranoia_overlap_r(buffA, buffB, offsetA, offsetB)
	Int16_t	*buffA;
	Int16_t	*buffB;
	long	offsetA;
	long	offsetB;
{
	long		beginA = offsetA;
	long		beginB = offsetB;

	for (; beginA >= 0 && beginB >= 0; beginA--, beginB--)
		if (buffA[beginA] != buffB[beginB])
			break;
	beginA++;
	beginB++;

	return (offsetA - beginA);
}

EXPORT long
i_paranoia_overlap_f(buffA, buffB, offsetA, offsetB, sizeA, sizeB)
	Int16_t	*buffA;
	Int16_t	*buffB;
	long	offsetA;
	long	offsetB;
	long	sizeA;
	long	sizeB;
{
	long		endA = offsetA;
	long		endB = offsetB;

	for (; endA < sizeA && endB < sizeB; endA++, endB++)
		if (buffA[endA] != buffB[endB])
			break;

	return (endA - offsetA);
}

EXPORT int
i_stutter_or_gap(A, B, offA, offB, gap)
	Int16_t	*A;
	Int16_t	*B;
	long	offA;
	long	offB;
	long	gap;
{
	long		a1 = offA;
	long		b1 = offB;

	if (a1 < 0) {
		b1 -= a1;
		gap += a1;
		a1 = 0;
	}
	return (memcmp(A + a1, B + b1, gap * 2));
}

/*
 * riftv is the first value into the rift -> or <-
 */
EXPORT void
i_analyze_rift_f(A, B, sizeA, sizeB, aoffset, boffset, matchA, matchB, matchC)
	Int16_t	*A;
	Int16_t	*B;
	long	sizeA;
	long	sizeB;
	long	aoffset;
	long	boffset;
	long	*matchA;
	long	*matchB;
	long	*matchC;
{

	long		apast = sizeA - aoffset;
	long		bpast = sizeB - boffset;
	long		i;

	*matchA = 0, *matchB = 0, *matchC = 0;

	/*
	 * Look for three possible matches... (A) Ariftv->B,
	 * (B) Briftv->A and (c) AB->AB.
	 */
	for (i = 0; ; i++) {
		if (i < bpast)	/* A */
			if (i_paranoia_overlap_f(A, B, aoffset, boffset + i, sizeA, sizeB) >= MIN_WORDS_RIFT) {
				*matchA = i;
				break;
			}
		if (i < apast) {	/* B */
			if (i_paranoia_overlap_f(A, B, aoffset + i, boffset, sizeA, sizeB) >= MIN_WORDS_RIFT) {
				*matchB = i;
				break;
			}
			if (i < bpast)	/* C */
				if (i_paranoia_overlap_f(A, B, aoffset + i, boffset + i, sizeA, sizeB) >= MIN_WORDS_RIFT) {
					*matchC = i;
					break;
				}
		} else if (i >= bpast)
			break;

	}

	if (*matchA == 0 && *matchB == 0 && *matchC == 0)
		return;

	if (*matchC)
		return;
	if (*matchA) {
		if (i_stutter_or_gap(A, B, aoffset - *matchA, boffset, *matchA))
			return;
		*matchB = -*matchA;	/* signify we need to remove n bytes */
					/* from B */
		*matchA = 0;
		return;
	} else {
		if (i_stutter_or_gap(B, A, boffset - *matchB, aoffset, *matchB))
			return;
		*matchA = -*matchB;
		*matchB = 0;
		return;
	}
}

/*
 * riftv must be first even val of rift moving back
 */
EXPORT void
i_analyze_rift_r(A, B, sizeA, sizeB, aoffset, boffset, matchA, matchB, matchC)
	Int16_t	*A;
	Int16_t	*B;
	long	sizeA;
	long	sizeB;
	long	aoffset;
	long	boffset;
	long	*matchA;
	long	*matchB;
	long	*matchC;
{

	long		apast = aoffset + 1;
	long		bpast = boffset + 1;
	long		i;

	*matchA = 0, *matchB = 0, *matchC = 0;

	/*
	 * Look for three possible matches... (A) Ariftv->B, (B) Briftv->A and
	 * (c) AB->AB.
	 */
	for (i = 0; ; i++) {
		if (i < bpast)	/* A */
			if (i_paranoia_overlap_r(A, B, aoffset, boffset - i) >= MIN_WORDS_RIFT) {
				*matchA = i;
				break;
			}
		if (i < apast) {	/* B */
			if (i_paranoia_overlap_r(A, B, aoffset - i, boffset) >= MIN_WORDS_RIFT) {
				*matchB = i;
				break;
			}
			if (i < bpast)	/* C */
				if (i_paranoia_overlap_r(A, B, aoffset - i, boffset - i) >= MIN_WORDS_RIFT) {
					*matchC = i;
					break;
				}
		} else if (i >= bpast)
			break;

	}

	if (*matchA == 0 && *matchB == 0 && *matchC == 0)
		return;

	if (*matchC)
		return;

	if (*matchA) {
		if (i_stutter_or_gap(A, B, aoffset + 1, boffset - *matchA + 1, *matchA))
			return;
		*matchB = -*matchA;	/* signify we need to remove n bytes */
					/* from B */
		*matchA = 0;
		return;
	} else {
		if (i_stutter_or_gap(B, A, boffset + 1, aoffset - *matchB + 1, *matchB))
			return;
		*matchA = -*matchB;
		*matchB = 0;
		return;
	}
}

EXPORT void
analyze_rift_silence_f(A, B, sizeA, sizeB, aoffset, boffset, matchA, matchB)
	Int16_t	*A;
	Int16_t	*B;
	long	sizeA;
	long	sizeB;
	long	aoffset;
	long	boffset;
	long	*matchA;
	long	*matchB;
{
	*matchA = -1;
	*matchB = -1;

	sizeA = min(sizeA, aoffset + MIN_WORDS_RIFT);
	sizeB = min(sizeB, boffset + MIN_WORDS_RIFT);

	aoffset++;
	boffset++;

	while (aoffset < sizeA) {
		if (A[aoffset] != A[aoffset - 1]) {
			*matchA = 0;
			break;
		}
		aoffset++;
	}

	while (boffset < sizeB) {
		if (B[boffset] != B[boffset - 1]) {
			*matchB = 0;
			break;
		}
		boffset++;
	}
}
