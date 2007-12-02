/* @(#)paranoia.c	1.36 07/02/09 J. Schilling from cdparanoia-III-alpha9.8 */
#ifndef lint
static	char sccsid[] =
"@(#)paranoia.c	1.36 07/02/09 J. Schilling from cdparanoia-III-alpha9.8";

#endif
/*
 * CopyPolicy: GNU Lesser General Public License v2.1 applies
 * Copyright (C) 1997-2001 by Monty (xiphmont@mit.edu)
 * Copyright (C) 2002-2006 by J. Schilling
 *
 * Toplevel file for the paranoia abstraction over the cdda lib
 *
 */

/* immediate todo:: */
/* Allow disabling of root fixups? */

/*
 * Dupe bytes are creeping into cases that require greater overlap
 * than a single fragment can provide.  We need to check against a
 * larger area* (+/-32 sectors of root?) to better eliminate
 *  dupes. Of course this leads to other problems... Is it actually a
 *  practically solvable problem?
 */
/* Bimodal overlap distributions break us. */
/* scratch detection/tolerance not implemented yet */

/*
 * Da new shtick: verification now a two-step assymetric process.
 *
 * A single 'verified/reconstructed' data segment cache, and then the
 * multiple fragment cache
 *
 * verify a newly read block against previous blocks; do it only this
 * once. We maintain a list of 'verified sections' from these matches.
 *
 * We then glom these verified areas into a new data buffer.
 * Defragmentation fixups are allowed here alone.
 *
 * We also now track where read boundaries actually happened; do not
 * verify across matching boundaries.
 */

/*
 * Silence.  "It's BAAAAAAaaack."
 *
 * audio is now treated as great continents of values floating on a
 * mantle of molten silence.  Silence is not handled by basic
 * verification at all; we simply anchor sections of nonzero audio to a
 * position and fill in everything else as silence.  We also note the
 * audio that interfaces with silence; an edge must be 'wet'.
 */

#include <schily/mconfig.h>
#include <schily/alloca.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <stdio.h>
#include <schily/string.h>
#include "p_block.h"
#include "cdda_paranoia.h"
#include "overlap.h"
#include "gap.h"
#include "isort.h"
#include "pmalloc.h"

/*
 * used by: i_iterate_stage2() / i_stage2_each()
 */
typedef struct sync_result {
	long		offset;
	long		begin;
	long		end;
} sync_result;

LOCAL	inline long	re		__PR((root_block * root));
LOCAL	inline long	rb		__PR((root_block * root));
LOCAL	inline long	rs		__PR((root_block * root));
LOCAL	inline Int16_t	*rv		__PR((root_block * root));
LOCAL	inline long i_paranoia_overlap	__PR((Int16_t * buffA, Int16_t * buffB,
						long offsetA, long offsetB,
						long sizeA, long sizeB,
						long *ret_begin, long *ret_end));
LOCAL	inline long i_paranoia_overlap2	__PR((Int16_t * buffA, Int16_t * buffB,
						Uchar *flagsA, Uchar *flagsB,
						long offsetA, long offsetB,
						long sizeA, long sizeB,
						long *ret_begin, long *ret_end));
LOCAL	inline long do_const_sync	__PR((c_block * A,
						sort_info * B, Uchar *flagB,
						long posA, long posB,
						long *begin, long *end, long *offset));
LOCAL	inline long try_sort_sync	__PR((cdrom_paranoia * p,
						sort_info * A, Uchar *Aflags,
						c_block * B,
						long post, long *begin, long *end,
						long *offset, void (*callback) (long, int)));
LOCAL	inline void stage1_matched	__PR((c_block * old, c_block * new,
						long matchbegin, long matchend,
						long matchoffset, void (*callback) (long, int)));
LOCAL	long	i_iterate_stage1	__PR((cdrom_paranoia * p, c_block * old, c_block * new,
						void (*callback) (long, int)));
LOCAL	long	i_stage1		__PR((cdrom_paranoia * p, c_block * new,
						void (*callback) (long, int)));
LOCAL	long	i_iterate_stage2	__PR((cdrom_paranoia * p, v_fragment * v,
						sync_result * r, void (*callback) (long, int)));
LOCAL	void	i_silence_test		__PR((root_block * root));
LOCAL	long	i_silence_match		__PR((root_block * root, v_fragment * v,
						void (*callback) (long, int)));
LOCAL	long	i_stage2_each		__PR((root_block * root, v_fragment * v,
						void (*callback) (long, int)));
LOCAL	int	i_init_root		__PR((root_block * root, v_fragment * v, long begin,
						void (*callback) (long, int)));
LOCAL	int	vsort			__PR((const void *a, const void *b));
LOCAL	int	i_stage2		__PR((cdrom_paranoia * p, long beginword, long endword,
						void (*callback) (long, int)));
LOCAL	void	i_end_case		__PR((cdrom_paranoia * p, long endword,
						void (*callback) (long, int)));
LOCAL	void	verify_skip_case	__PR((cdrom_paranoia * p, void (*callback) (long, int)));
EXPORT	void	paranoia_free		__PR((cdrom_paranoia * p));
EXPORT	void	paranoia_modeset	__PR((cdrom_paranoia * p, int enable));
EXPORT	long	paranoia_seek		__PR((cdrom_paranoia * p, long seek, int mode));
c_block		*i_read_c_block		__PR((cdrom_paranoia * p, long beginword, long endword,
						void (*callback) (long, int)));
EXPORT	Int16_t	*paranoia_read		__PR((cdrom_paranoia * p, void (*callback) (long, int)));
EXPORT	Int16_t	*paranoia_read_limited	__PR((cdrom_paranoia * p, void (*callback) (long, int),
						int max_retries));
EXPORT	void	paranoia_overlapset	__PR((cdrom_paranoia * p, long overlap));


LOCAL inline long
re(root)
	root_block	*root;
{
	if (!root)
		return (-1);
	if (!root->vector)
		return (-1);
	return (ce(root->vector));
}

LOCAL inline long
rb(root)
	root_block	*root;
{
	if (!root)
		return (-1);
	if (!root->vector)
		return (-1);
	return (cb(root->vector));
}

LOCAL inline long
rs(root)
	root_block	*root;
{
	if (!root)
		return (-1);
	if (!root->vector)
		return (-1);
	return (cs(root->vector));
}

LOCAL inline Int16_t *
rv(root)
	root_block	*root;
{
	if (!root)
		return (NULL);
	if (!root->vector)
		return (NULL);
	return (cv(root->vector));
}

#define	rc(r)	((r)->vector)

/*
 * matching and analysis code
 */
LOCAL inline long
i_paranoia_overlap(buffA, buffB, offsetA, offsetB, sizeA, sizeB, ret_begin, ret_end)
	Int16_t	*buffA;
	Int16_t	*buffB;
	long	offsetA;
	long	offsetB;
	long	sizeA;
	long	sizeB;
	long	*ret_begin;
	long	*ret_end;
{
	long	beginA = offsetA;
	long	endA = offsetA;
	long	beginB = offsetB;
	long	endB = offsetB;

	for (; beginA >= 0 && beginB >= 0; beginA--, beginB--)
		if (buffA[beginA] != buffB[beginB])
			break;
	beginA++;
	beginB++;

	for (; endA < sizeA && endB < sizeB; endA++, endB++)
		if (buffA[endA] != buffB[endB])
			break;

	if (ret_begin)
		*ret_begin = beginA;
	if (ret_end)
		*ret_end = endA;
	return (endA - beginA);
}

LOCAL inline long
i_paranoia_overlap2(buffA, buffB, flagsA, flagsB, offsetA, offsetB, sizeA, sizeB, ret_begin, ret_end)
	Int16_t	*buffA;
	Int16_t	*buffB;
	Uchar	*flagsA;
	Uchar	*flagsB;
	long	offsetA;
	long	offsetB;
	long	sizeA;
	long	sizeB;
	long	*ret_begin;
	long	*ret_end;
{
	long		beginA = offsetA;
	long		endA = offsetA;
	long		beginB = offsetB;
	long		endB = offsetB;

	for (; beginA >= 0 && beginB >= 0; beginA--, beginB--) {
		if (buffA[beginA] != buffB[beginB])
			break;
		/*
		 * don't allow matching across matching sector boundaries
		 */
		if ((flagsA[beginA] & flagsB[beginB] & 1)) {
			beginA--;
			beginB--;
			break;
		}
		/*
		 * don't allow matching through known missing data
		 */
		if ((flagsA[beginA] & 2) || (flagsB[beginB] & 2))
			break;
	}
	beginA++;
	beginB++;

	for (; endA < sizeA && endB < sizeB; endA++, endB++) {
		if (buffA[endA] != buffB[endB])
			break;
		/*
		 * don't allow matching across matching sector boundaries
		 */
		if ((flagsA[endA] & flagsB[endB] & 1) && endA != beginA) {
			break;
		}
		/*
		 * don't allow matching through known missing data
		 */
		if ((flagsA[endA] & 2) || (flagsB[endB] & 2))
			break;
	}

	if (ret_begin)
		*ret_begin = beginA;
	if (ret_end)
		*ret_end = endA;
	return (endA - beginA);
}

/* Top level of the first stage matcher */

/*
 * We match each analysis point of new to the preexisting blocks
 * recursively.  We can also optionally maintain a list of fragments of
 * the preexisting block that didn't match anything, and match them back
 * afterward.
 */
#define	OVERLAP_ADJ	(MIN_WORDS_OVERLAP/2-1)

LOCAL inline long
do_const_sync(A, B, flagB, posA, posB, begin, end, offset)
	c_block		*A;
	sort_info	*B;
	Uchar		*flagB;
	long		posA;
	long		posB;
	long		*begin;
	long		*end;
	long		*offset;
{
	Uchar		*flagA = A->flags;
	long		ret = 0;

	if (flagB == NULL)
		ret = i_paranoia_overlap(cv(A), iv(B), posA, posB,
			cs(A), is(B), begin, end);
	else if ((flagB[posB] & 2) == 0)
		ret = i_paranoia_overlap2(cv(A), iv(B), flagA, flagB, posA, posB, cs(A),
			is(B), begin, end);

	if (ret > MIN_WORDS_SEARCH) {
		*offset = (posA + cb(A)) - (posB + ib(B));
		*begin += cb(A);
		*end += cb(A);
		return (ret);
	}
	return (0);
}

/*
 * post is w.r.t. B.  in stage one, we post from old.  In stage 2 we
 * post from root. Begin, end, offset count from B's frame of
 * reference
 */
LOCAL inline long
try_sort_sync(p, A, Aflags, B, post, begin, end, offset, callback)
	cdrom_paranoia	*p;
	sort_info	*A;
	Uchar		*Aflags;
	c_block		*B;
	long		post;
	long		*begin;
	long		*end;
	long		*offset;
	void		(*callback) __PR((long, int));
{

	long		dynoverlap = p->dynoverlap;
	sort_link	*ptr = NULL;
	Uchar		*Bflags = B->flags;

	/*
	 * block flag matches 0x02 (unmatchable)
	 */
	if (Bflags == NULL || (Bflags[post - cb(B)] & 2) == 0) {
		/*
		 * always try absolute offset zero first!
		 */
		{
			long		zeropos = post - ib(A);

			if (zeropos >= 0 && zeropos < is(A)) {
				if (cv(B)[post - cb(B)] == iv(A)[zeropos]) {
					if (do_const_sync(B, A, Aflags,
							post - cb(B), zeropos,
							begin, end, offset)) {

						offset_add_value(p, &(p->stage1), *offset, callback);

						return (1);
					}
				}
			}
		}
	} else
		return (0);

	ptr = sort_getmatch(A, post - ib(A), dynoverlap, cv(B)[post - cb(B)]);

	while (ptr) {

		if (do_const_sync(B, A, Aflags,
				post - cb(B), ipos(A, ptr),
				begin, end, offset)) {
			offset_add_value(p, &(p->stage1), *offset, callback);
			return (1);
		}
		ptr = sort_nextmatch(A, ptr);
	}

	*begin = -1;
	*end = -1;
	*offset = -1;
	return (0);
}

LOCAL inline void
stage1_matched(old, new, matchbegin, matchend, matchoffset, callback)
	c_block	*old;
	c_block	*new;
	long	matchbegin;
	long	matchend;
	long	matchoffset;
	void	(*callback) __PR((long, int));
{
	long		i;
	long		oldadjbegin = matchbegin - cb(old);
	long		oldadjend = matchend - cb(old);
	long		newadjbegin = matchbegin - matchoffset - cb(new);
	long		newadjend = matchend - matchoffset - cb(new);

	if (matchbegin - matchoffset <= cb(new) ||
		matchbegin <= cb(old) ||
		(new->flags[newadjbegin] & 1) ||
		(old->flags[oldadjbegin] & 1)) {
		if (matchoffset)
			if (callback)
				(*callback) (matchbegin, PARANOIA_CB_FIXUP_EDGE);
	} else if (callback)
		(*callback) (matchbegin, PARANOIA_CB_FIXUP_ATOM);

	if (matchend - matchoffset >= ce(new) ||
		(new->flags[newadjend] & 1) ||
		matchend >= ce(old) ||
		(old->flags[oldadjend] & 1)) {
		if (matchoffset)
			if (callback)
				(*callback) (matchend, PARANOIA_CB_FIXUP_EDGE);
	} else if (callback)
		(*callback) (matchend, PARANOIA_CB_FIXUP_ATOM);

	/*
	 * Mark the verification flags.  Don't mark the first or last OVERLAP/2
	 * elements so that overlapping fragments have to overlap by OVERLAP to
	 * actually merge. We also remove elements from the sort such that
	 * later sorts do not have to sift through already matched data
	 */
	newadjbegin += OVERLAP_ADJ;
	newadjend -= OVERLAP_ADJ;
	for (i = newadjbegin; i < newadjend; i++)
		new->flags[i] |= 4;	/* mark verified */

	oldadjbegin += OVERLAP_ADJ;
	oldadjend -= OVERLAP_ADJ;
	for (i = oldadjbegin; i < oldadjend; i++)
		old->flags[i] |= 4;	/* mark verified */

}

#define	CB_NULL		(void (*) __PR((long, int)))0

LOCAL long
i_iterate_stage1(p, old, new, callback)
	cdrom_paranoia	*p;
	c_block		*old;
	c_block		*new;
	void		(*callback) __PR((long, int));
{

	long		matchbegin = -1;
	long		matchend = -1;
	long		matchoffset;

	/*
	 * we no longer try to spread the stage one search area by dynoverlap
	 */
	long		searchend = min(ce(old), ce(new));
	long		searchbegin = max(cb(old), cb(new));
	long		searchsize = searchend - searchbegin;
	sort_info	*i = p->sortcache;
	long		ret = 0;
	long		j;

	long		tried = 0;
	long		matched = 0;

	if (searchsize <= 0)
		return (0);

	/*
	 * match return values are in terms of the new vector, not old
	 */
	for (j = searchbegin; j < searchend; j += 23) {
		if ((new->flags[j - cb(new)] & 6) == 0) {
			tried++;
			if (try_sort_sync(p, i, new->flags, old, j, &matchbegin, &matchend, &matchoffset,
					callback) == 1) {

				matched += matchend - matchbegin;

				/*
				 * purely cosmetic: if we're matching zeros,
				 * don't use the callback because they will
				 * appear to be all skewed
				 */
				{
					long		jj = matchbegin - cb(old);
					long		end = matchend - cb(old);

					for (; jj < end; jj++)
						if (cv(old)[jj] != 0)
							break;
					if (jj < end) {
						stage1_matched(old, new, matchbegin, matchend, matchoffset, callback);
					} else {
						stage1_matched(old, new, matchbegin, matchend, matchoffset, CB_NULL);
					}
				}
				ret++;
				if (matchend - 1 > j)
					j = matchend - 1;
			}
		}
	}
#ifdef NOISY
	fprintf(stderr, "iterate_stage1: search area=%ld[%ld-%ld] tried=%ld matched=%ld spans=%ld\n",
		searchsize, searchbegin, searchend, tried, matched, ret);
#endif

	return (ret);
}

LOCAL long
i_stage1(p, new, callback)
	cdrom_paranoia	*p;
	c_block		*new;
	void		(*callback) __PR((long, int));
{

	long		size = cs(new);
	c_block		*ptr = c_last(p);
	int		ret = 0;
	long		begin = 0;
	long		end;

	if (ptr)
		sort_setup(p->sortcache, cv(new), &cb(new), cs(new),
			cb(new), ce(new));

	while (ptr && ptr != new) {

		if (callback)
			(*callback) (cb(new), PARANOIA_CB_VERIFY);
		i_iterate_stage1(p, ptr, new, callback);

		ptr = c_prev(ptr);
	}

	/*
	 * parse the verified areas of new into v_fragments
	 */
	begin = 0;
	while (begin < size) {
		for (; begin < size; begin++)
			if (new->flags[begin] & 4)
				break;
		for (end = begin; end < size; end++)
			if ((new->flags[end] & 4) == 0)
				break;
		if (begin >= size)
			break;

		ret++;

		new_v_fragment(p, new, cb(new) + max(0, begin - OVERLAP_ADJ),
			cb(new) + min(size, end + OVERLAP_ADJ),
			(end + OVERLAP_ADJ >= size && new->lastsector));

		begin = end;
	}

	return (ret);
}

/*
 * reconcile v_fragments to root buffer.  Free if matched, fragment/fixup root
 * if necessary
 *
 * do *not* match using zero posts
 */
LOCAL long
i_iterate_stage2(p, v, r, callback)
	cdrom_paranoia	*p;
	v_fragment	*v;
	sync_result	*r;
	void		(*callback) __PR((long, int));
{
	root_block	*root = &(p->root);
	long		matchbegin = -1;
	long		matchend = -1;
	long		offset;
	long		fbv;
	long		fev;

#ifdef NOISY
	fprintf(stderr, "Stage 2 search: fbv=%ld fev=%ld\n", fb(v), fe(v));
#endif

	if (min(fe(v) + p->dynoverlap, re(root)) -
		max(fb(v) - p->dynoverlap, rb(root)) <= 0)
		return (0);

	if (callback)
		(*callback) (fb(v), PARANOIA_CB_VERIFY);

	/*
	 * just a bit of v; determine the correct area
	 */
	fbv = max(fb(v), rb(root) - p->dynoverlap);

	/*
	 * we want to avoid zeroes
	 */
	while (fbv < fe(v) && fv(v)[fbv - fb(v)] == 0)
		fbv++;
	if (fbv == fe(v))
		return (0);
	fev = min(min(fbv + 256, re(root) + p->dynoverlap), fe(v));

	{
		/*
		 * spread the search area a bit.  We post from root, so
		 * containment must strictly adhere to root
		 */
		long		searchend = min(fev + p->dynoverlap, re(root));
		long		searchbegin = max(fbv - p->dynoverlap, rb(root));
		sort_info	*i = p->sortcache;
		long		j;

		sort_setup(i, fv(v), &fb(v), fs(v), fbv, fev);
		for (j = searchbegin; j < searchend; j += 23) {
			while (j < searchend && rv(root)[j - rb(root)] == 0)
				j++;
			if (j == searchend)
				break;

			if (try_sort_sync(p, i, (Uchar *)0, rc(root), j,
					&matchbegin, &matchend, &offset, callback)) {

				r->begin = matchbegin;
				r->end = matchend;
				r->offset = -offset;
				if (offset)
					if (callback)
						(*callback) (r->begin, PARANOIA_CB_FIXUP_EDGE);
				return (1);
			}
		}
	}
	return (0);
}

/*
 * simple test for a root vector that ends in silence
 */
LOCAL void
i_silence_test(root)
	root_block	*root;
{
	Int16_t		*vec = rv(root);
	long		end = re(root) - rb(root) - 1;
	long		j;

	for (j = end - 1; j >= 0; j--)
		if (vec[j] != 0)
			break;
	if (j < 0 || end - j > MIN_SILENCE_BOUNDARY) {
		if (j < 0)
			j = 0;
		root->silenceflag = 1;
		root->silencebegin = rb(root) + j;
		if (root->silencebegin < root->returnedlimit)
			root->silencebegin = root->returnedlimit;
	}
}

/*
 * match into silence vectors at offset zero if at all possible.  This
 * also must be called with vectors in ascending begin order in case
 * there are nonzero islands
 */
LOCAL long
i_silence_match(root, v, callback)
	root_block	*root;
	v_fragment	*v;
	void		(*callback) __PR((long, int));
{

	cdrom_paranoia	*p = v->p;
	Int16_t	*vec = fv(v);
	long		end = fs(v);
	long		begin;
	long		j;

	/*
	 * does this vector begin wet?
	 */
	if (end < MIN_SILENCE_BOUNDARY)
		return (0);
	for (j = 0; j < end; j++)
		if (vec[j] != 0)
			break;
	if (j < MIN_SILENCE_BOUNDARY)
		return (0);
	j += fb(v);

	/*
	 * is the new silent section ahead of the end of the old
	 * by < p->dynoverlap?
	 */
	if (fb(v) >= re(root) && fb(v) - p->dynoverlap < re(root)) {
		/*
		 * extend the zeroed area of root
		 * XXX dynarrays are not needed here.
		 */
		long		addto = fb(v) + MIN_SILENCE_BOUNDARY - re(root);
/*		Int16_t		avec[addto];*/
#ifdef	HAVE_ALLOCA
		Int16_t		*avec = alloca(addto * sizeof (Int16_t));
#else
		Int16_t		*avec = _pmalloc(addto * sizeof (Int16_t));
#endif

		memset(avec, 0, sizeof (avec));
		c_append(rc(root), avec, addto);
#ifndef	HAVE_ALLOCA
		_pfree(avec);
#endif
	}
	/*
	 * do we have an 'effortless' overlap?
	 */
	begin = max(fb(v), root->silencebegin);
	end = min(j, re(root));

	if (begin < end) {
		/*
		 * don't use it unless it will extend...
		 */
		if (fe(v) > re(root)) {
			long		voff = begin - fb(v);

			c_remove(rc(root), begin - rb(root), -1);
			c_append(rc(root), vec + voff, fs(v) - voff);
		}
		offset_add_value(p, &p->stage2, 0, callback);

	} else {
		if (j < begin) {
			/*
			 * OK, we'll have to force it a bit as the root is
			 * jittered forward
			 */
			long		voff = j - fb(v);

			/*
			 * don't use it unless it will extend...
			 */
			if (begin + fs(v) - voff > re(root)) {
				c_remove(rc(root), root->silencebegin - rb(root), -1);
				c_append(rc(root), vec + voff, fs(v) - voff);
			}
			offset_add_value(p, &p->stage2, end - begin, callback);
		} else
			return (0);
	}

	/*
	 * test the new root vector for ending in silence
	 */
	root->silenceflag = 0;
	i_silence_test(root);

	if (v->lastsector)
		root->lastsector = 1;
	free_v_fragment(v);
	return (1);
}

LOCAL long
i_stage2_each(root, v, callback)
	root_block	*root;
	v_fragment	*v;
	void		(*callback) __PR((long, int));
{

	cdrom_paranoia *p = v->p;
	long		dynoverlap = p->dynoverlap / 2 * 2;

	if (!v || !v->one)
		return (0);

	if (!rv(root)) {
		return (0);
	} else {
		sync_result	r;

		if (i_iterate_stage2(p, v, &r, callback)) {

			long		begin = r.begin - rb(root);
			long		end = r.end - rb(root);
			long		offset = r.begin + r.offset - fb(v) - begin;
			long		temp;
			c_block		*l = NULL;

			/*
			 * we have a match! We don't rematch off rift, we chase
			 * the match all the way to both extremes doing rift
			 * analysis.
			 */
#ifdef NOISY
			fprintf(stderr, "Stage 2 match\n");
#endif

			/*
			 * chase backward
			 * note that we don't extend back right now, only
			 * forward.
			 */
			while ((begin + offset > 0 && begin > 0)) {
				long		matchA = 0,
						matchB = 0,
						matchC = 0;
				long		beginL = begin + offset;

				if (l == NULL) {
					Int16_t	*buff = _pmalloc(fs(v) * sizeof (Int16_t));

					l = c_alloc(buff, fb(v), fs(v));
					memcpy(buff, fv(v), fs(v) * sizeof (Int16_t));
				}
				i_analyze_rift_r(rv(root), cv(l),
					rs(root), cs(l),
					begin - 1, beginL - 1,
					&matchA, &matchB, &matchC);

#ifdef NOISY
				fprintf(stderr, "matching rootR: matchA:%ld matchB:%ld matchC:%ld\n",
					matchA, matchB, matchC);
#endif

				if (matchA) {
					/*
					 * a problem with root
					 */
					if (matchA > 0) {
						/*
						 * dropped bytes; add back from v
						 */
						if (callback)
							(*callback) (begin + rb(root) - 1, PARANOIA_CB_FIXUP_DROPPED);
						if (rb(root) + begin < p->root.returnedlimit)
							break;
						else {
							c_insert(rc(root), begin, cv(l) + beginL - matchA,
								matchA);
							offset -= matchA;
							begin += matchA;
							end += matchA;
						}
					} else {
						/*
						 * duplicate bytes; drop from root
						 */
						if (callback)
							(*callback) (begin + rb(root) - 1, PARANOIA_CB_FIXUP_DUPED);
						if (rb(root) + begin + matchA < p->root.returnedlimit)
							break;
						else {
							c_remove(rc(root), begin + matchA, -matchA);
							offset -= matchA;
							begin += matchA;
							end += matchA;
						}
					}
				} else if (matchB) {
					/*
					 * a problem with the fragment
					 */
					if (matchB > 0) {
						/*
						 * dropped bytes
						 */
						if (callback)
							(*callback) (begin + rb(root) - 1, PARANOIA_CB_FIXUP_DROPPED);
						c_insert(l, beginL, rv(root) + begin - matchB,
							matchB);
						offset += matchB;
					} else {
						/*
						 * duplicate bytes
						 */
						if (callback)
							(*callback) (begin + rb(root) - 1, PARANOIA_CB_FIXUP_DUPED);
						c_remove(l, beginL + matchB, -matchB);
						offset += matchB;
					}
				} else if (matchC) {
					/*
					 * Uhh... problem with both
					 * Set 'disagree' flags in root
					 */
					if (rb(root) + begin - matchC < p->root.returnedlimit)
						break;
					c_overwrite(rc(root), begin - matchC,
						cv(l) + beginL - matchC, matchC);

				} else {
					/*
					 * do we have a mismatch due to silence
					 * beginning/end case?
					 * in the 'chase back' case, we don't
					 * do anything.
					 * Did not determine nature of
					 * difficulty... report and bail
					 */
					/* RRR(*callback)(post,PARANOIA_CB_XXX); */
					break;
				}
				/*
				 * not the most efficient way, but it will do
				 * for now
				 */
				beginL = begin + offset;
				i_paranoia_overlap(rv(root), cv(l),
					begin, beginL,
					rs(root), cs(l),
					&begin, &end);
			}

			/*
			 * chase forward
			 */
			temp = l ? cs(l) : fs(v);
			while (end + offset < temp && end < rs(root)) {
				long	matchA = 0,
					matchB = 0,
					matchC = 0;
				long	beginL = begin + offset;
				long	endL = end + offset;

				if (l == NULL) {
					Int16_t	*buff = _pmalloc(fs(v) * sizeof (Int16_t));

					l = c_alloc(buff, fb(v), fs(v));
					memcpy(buff, fv(v), fs(v) * sizeof (Int16_t));
				}
				i_analyze_rift_f(rv(root), cv(l),
					rs(root), cs(l),
					end, endL,
					&matchA, &matchB, &matchC);

#ifdef NOISY
				fprintf(stderr, "matching rootF: matchA:%ld matchB:%ld matchC:%ld\n",
					matchA, matchB, matchC);
#endif

				if (matchA) {
					/*
					 * a problem with root
					 */
					if (matchA > 0) {
						/*
						 * dropped bytes; add back from v
						 */
						if (callback)
							(*callback) (end + rb(root), PARANOIA_CB_FIXUP_DROPPED);
						if (end + rb(root) < p->root.returnedlimit)
							break;
						c_insert(rc(root), end, cv(l) + endL, matchA);
					} else {
						/*
						 * duplicate bytes; drop from root
						 */
						if (callback)
							(*callback) (end + rb(root), PARANOIA_CB_FIXUP_DUPED);
						if (end + rb(root) < p->root.returnedlimit)
							break;
						c_remove(rc(root), end, -matchA);
					}
				} else if (matchB) {
					/*
					 * a problem with the fragment
					 */
					if (matchB > 0) {
						/*
						 * dropped bytes
						 */
						if (callback)
							(*callback) (end + rb(root), PARANOIA_CB_FIXUP_DROPPED);
						c_insert(l, endL, rv(root) + end, matchB);
					} else {
						/*
						 * duplicate bytes
						 */
						if (callback)
							(*callback) (end + rb(root), PARANOIA_CB_FIXUP_DUPED);
						c_remove(l, endL, -matchB);
					}
				} else if (matchC) {
					/*
					 * Uhh... problem with both
					 * Set 'disagree' flags in root
					 */
					if (end + rb(root) < p->root.returnedlimit)
						break;
					c_overwrite(rc(root), end, cv(l) + endL, matchC);
				} else {
					analyze_rift_silence_f(rv(root), cv(l),
						rs(root), cs(l),
						end, endL,
						&matchA, &matchB);
					if (matchA) {
						/*
						 * silence in root
						 * Can only do this if we haven't
						 * already returned data
						 */
						if (end + rb(root) >= p->root.returnedlimit) {
							c_remove(rc(root), end, -1);
						}
					} else if (matchB) {
						/*
						 * silence in fragment; lose it
						 */
						if (l)
							i_cblock_destructor(l);
						free_v_fragment(v);
						return (1);

					} else {
						/*
						 * Could not determine nature of
						 * difficulty... report and bail
						 */
						/* RRR(*callback)(post,PARANOIA_CB_XXX); */
					}
					break;
				}
				/*
				 * not the most efficient way, but it will do for now
				 */
				i_paranoia_overlap(rv(root), cv(l),
					begin, beginL,
					rs(root), cs(l),
					(long *)0, &end);
			}

			/*
			 * if this extends our range, let's glom
			 */
			{
				long	sizeA = rs(root);
				long	sizeB;
				long	vecbegin;
				Int16_t	*vector;

				if (l) {
					sizeB = cs(l);
					vector = cv(l);
					vecbegin = cb(l);
				} else {
					sizeB = fs(v);
					vector = fv(v);
					vecbegin = fb(v);
				}

				if (sizeB - offset > sizeA || v->lastsector) {
					if (v->lastsector) {
						root->lastsector = 1;
					}
					if (end < sizeA)
						c_remove(rc(root), end, -1);

					if (sizeB - offset - end)
						c_append(rc(root), vector + end + offset,
							sizeB - offset - end);

					i_silence_test(root);

					/*
					 * add offset into dynoverlap stats
					 */
					offset_add_value(p, &p->stage2, offset + vecbegin - rb(root), callback);
				}
			}
			if (l)
				i_cblock_destructor(l);
			free_v_fragment(v);
			return (1);

		} else {
			/*
			 * D'oh.  No match.  What to do with the fragment?
			 */
			if (fe(v) + dynoverlap < re(root) && !root->silenceflag) {
				/*
				 * It *should* have matched.  No good; free it.
				 */
				free_v_fragment(v);
			}
			/*
			 * otherwise, we likely want this for an upcoming match
			 * we don't free the sort info (if it was collected)
			 */
			return (0);

		}
	}
}

LOCAL int
i_init_root(root, v, begin, callback)
	root_block	*root;
	v_fragment	*v;
	long		begin;
	void		(*callback) __PR((long, int));
{
	if (fb(v) <= begin && fe(v) > begin) {

		root->lastsector = v->lastsector;
		root->returnedlimit = begin;

		if (rv(root)) {
			i_cblock_destructor(rc(root));
			rc(root) = NULL;
		}
		{
			Int16_t		*buff = _pmalloc(fs(v) * sizeof (Int16_t));

			memcpy(buff, fv(v), fs(v) * sizeof (Int16_t));
			root->vector = c_alloc(buff, fb(v), fs(v));
		}
		i_silence_test(root);

		return (1);
	} else
		return (0);
}

LOCAL int
vsort(a, b)
	const void	*a;
	const void	*b;
{
	return ((*(v_fragment **) a)->begin - (*(v_fragment **) b)->begin);
}

LOCAL int
i_stage2(p, beginword, endword, callback)
	cdrom_paranoia	*p;
	long		beginword;
	long		endword;
	void		(*callback) __PR((long, int));
{

	int		flag = 1;
	int		ret = 0;
	root_block	*root = &(p->root);

#ifdef NOISY
	fprintf(stderr, "Fragments:%ld\n", p->fragments->active);
	fflush(stderr);
#endif

	/*
	 * even when the 'silence flag' is lit, we try to do non-silence
	 * matching in the event that there are still audio vectors with
	 * content to be sunk before the silence
	 */
	while (flag) {
		/*
		 * loop through all the current fragments
		 */
		v_fragment	*first = v_first(p);
		long		active = p->fragments->active,
				count = 0;
#ifdef	HAVE_DYN_ARRAYS
		v_fragment	*list[active];
#else
		v_fragment	**list = _pmalloc(active * sizeof (v_fragment *));
#endif

		while (first) {
			v_fragment	*next = v_next(first);

			list[count++] = first;
			first = next;
		}

		flag = 0;
		if (count) {
			/*
			 * sorted in ascending order of beginning
			 */
			qsort(list, active, sizeof (v_fragment *), vsort);

			/*
			 * we try a nonzero based match even if in silent mode
			 * in the case that there are still cached vectors to
			 * sink behind continent->ocean boundary
			 */
			for (count = 0; count < active; count++) {
				first = list[count];
				if (first->one) {
					if (rv(root) == NULL) {
						if (i_init_root(&(p->root), first, beginword, callback)) {
							free_v_fragment(first);
							flag = 1;
							ret++;
						}
					} else {
						if (i_stage2_each(root, first, callback)) {
							ret++;
							flag = 1;
						}
					}
				}
			}

			/*
			 * silence handling
			 */
			if (!flag && p->root.silenceflag) {
				for (count = 0; count < active; count++) {
					first = list[count];
					if (first->one) {
						if (rv(root) != NULL) {
							if (i_silence_match(root, first, callback)) {
								ret++;
								flag = 1;
							}
						}
					}
				}
			}
		}
#ifndef	HAVE_DYN_ARRAYS
		_pfree(list);
#endif
	}
	return (ret);
}

LOCAL void
i_end_case(p, endword, callback)
	cdrom_paranoia	*p;
	long		endword;
	void		(*callback) __PR((long, int));
{

	root_block	*root = &p->root;

	/*
	 * have an 'end' flag; if we've just read in the last sector in a
	 * session, set the flag.  If we verify to the end of a fragment
	 * which has the end flag set, we're done (set a done flag).
	 * Pad zeroes to the end of the read
	 */
	if (root->lastsector == 0)
		return;
	if (endword < re(root))
		return;

	{
		long		addto = endword - re(root);
		char		*temp = _pcalloc(addto, sizeof (char) * 2);

		c_append(rc(root), (void *) temp, addto);
		_pfree(temp);

		/*
		 * trash da cache
		 */
		paranoia_resetcache(p);

	}
}

/*
 * We want to add a sector. Look through the caches for something that
 * spans.  Also look at the flags on the c_block... if this is an
 * obliterated sector, get a bit of a chunk past the obliteration.
 *
 * Not terribly smart right now, actually.  We can probably find
 * *some* match with a cache block somewhere.  Take it and continue it
 * through the skip
 */
LOCAL void
verify_skip_case(p, callback)
	cdrom_paranoia	*p;
	void		(*callback) __PR((long, int));
{

	root_block	*root = &(p->root);
	c_block		*graft = NULL;
	int		vflag = 0;
	int		gend = 0;
	long		post;

#ifdef NOISY
	fprintf(stderr, "\nskipping\n");
#endif

	if (rv(root) == NULL) {
		post = 0;
	} else {
		post = re(root);
	}
	if (post == -1)
		post = 0;

	if (callback)
		(*callback) (post, PARANOIA_CB_SKIP);

	if (p->enable & PARANOIA_MODE_NEVERSKIP)
		return;

	/*
	 * We want to add a sector.  Look for a c_block that spans,
	 * preferrably a verified area
	 */
	{
		c_block		*c = c_first(p);

		while (c) {
			long		cbegin = cb(c);
			long		cend = ce(c);

			if (cbegin <= post && cend > post) {
				long		vend = post;

				if (c->flags[post - cbegin] & 4) {
					/*
					 * verified area!
					 */
					while (vend < cend && (c->flags[vend - cbegin] & 4))
						vend++;
					if (!vflag || vend > vflag) {
						graft = c;
						gend = vend;
					}
					vflag = 1;
				} else {
					/*
					 * not a verified area
					 */
					if (!vflag) {
						while (vend < cend && (c->flags[vend - cbegin] & 4) == 0)
							vend++;
						if (graft == NULL || gend > vend) {
							/*
							 * smallest unverified area
							 */
							graft = c;
							gend = vend;
						}
					}
				}
			}
			c = c_next(c);
		}

		if (graft) {
			long		cbegin = cb(graft);
			long		cend = ce(graft);

			while (gend < cend && (graft->flags[gend - cbegin] & 4))
				gend++;
			gend = min(gend + OVERLAP_ADJ, cend);

			if (rv(root) == NULL) {
				Int16_t	*buff = _pmalloc(cs(graft));

				memcpy(buff, cv(graft), cs(graft));
				rc(root) = c_alloc(buff, cb(graft), cs(graft));
			} else {
				c_append(rc(root), cv(graft) + post - cbegin,
					gend - post);
			}

			root->returnedlimit = re(root);
			return;
		}
	}

	/*
	 * No?  Fine.  Great.  Write in some zeroes :-P
	 */
	{
		void	*temp = _pcalloc(CD_FRAMESIZE_RAW, sizeof (Int16_t));

		if (rv(root) == NULL) {
			rc(root) = c_alloc(temp, post, CD_FRAMESIZE_RAW);
		} else {
			c_append(rc(root), temp, CD_FRAMESIZE_RAW);
			_pfree(temp);
		}
		root->returnedlimit = re(root);
	}
}

/*
 * toplevel
 */
EXPORT void
paranoia_free(p)
	cdrom_paranoia	*p;
{
	paranoia_resetall(p);
	sort_free(p->sortcache);
	_pfree(p);
}

EXPORT void
paranoia_modeset(p, enable)
	cdrom_paranoia	*p;
	int		enable;
{
	p->enable = enable;
}

EXPORT long
paranoia_seek(p, seek, mode)
	cdrom_paranoia	*p;
	long		seek;
	int		mode;
{
	long	sector;
	long	ret;

	switch (mode) {
	case SEEK_SET:
		sector = seek;
		break;
	case SEEK_END:
		sector = p->d_disc_lastsector(p->d) + seek;
		break;
	default:
		sector = p->cursor + seek;
		break;
	}

	if (p->d_sector_gettrack(p->d, sector) == -1)
		return (-1);

	i_cblock_destructor(p->root.vector);
	p->root.vector = NULL;
	p->root.lastsector = 0;
	p->root.returnedlimit = 0;

	ret = p->cursor;
	p->cursor = sector;

	i_paranoia_firstlast(p);

	/*
	 * Evil hack to fix pregap patch for NEC drives! To be rooted out in a10
	 */
	p->current_firstsector = sector;

	return (ret);
}

/*
 * returns last block read, -1 on error
 */
EXPORT c_block *
i_read_c_block(p, beginword, endword, callback)
	cdrom_paranoia	*p;
	long		beginword;
	long		endword;
	void		(*callback) __PR((long, int));
{
	/*
	 * why do it this way?  We need to read lots of sectors to kludge
	 * around stupid read ahead buffers on cheap drives, as well as avoid
	 * expensive back-seeking. We also want to 'jiggle' the start address
	 * to try to break borderline drives more noticeably (and make broken
	 * drives with unaddressable sectors behave more often).
	 */
	long		readat;
	long		firstread;
	long		totaltoread = p->readahead;
	long		sectatonce = p->nsectors;
	long		driftcomp = (float) p->dyndrift / CD_FRAMEWORDS + .5;
	c_block		*new = NULL;
	root_block	*root = &p->root;
	Int16_t		*buffer = NULL;
	void		*bufbase = NULL;
	Uchar		*flags = NULL;
	long		sofar;
	long		dynoverlap = (p->dynoverlap + CD_FRAMEWORDS - 1) / CD_FRAMEWORDS;
	long		anyflag = 0;
	int		reduce = 0;
static	int		pagesize = -1;
#define	valign(x, a)	(((char *)(x)) + ((a) - 1 - ((((UIntptr_t)(x))-1)%(a))))

	/*
	 * What is the first sector to read?  want some pre-buffer if we're not
	 * at the extreme beginning of the disc
	 */
	if (p->enable & (PARANOIA_MODE_VERIFY | PARANOIA_MODE_OVERLAP)) {

		/*
		 * we want to jitter the read alignment boundary
		 */
		long	target;

		if (rv(root) == NULL || rb(root) > beginword)
			target = p->cursor - dynoverlap;
		else
			target = re(root) / (CD_FRAMEWORDS) - dynoverlap;

		if (target + MIN_SECTOR_BACKUP > p->lastread && target <= p->lastread)
			target = p->lastread - MIN_SECTOR_BACKUP;

		/*
		 * we want to jitter the read alignment boundary, as some
		 * drives, beginning from a specific point, will tend to
		 * lose bytes between sectors in the same place.  Also, as
		 * our vectors are being made up of multiple reads, we want
		 * the overlap boundaries to move....
		 */
		readat = (target & (~((long) JIGGLE_MODULO - 1))) + p->jitter;
		if (readat > target)
			readat -= JIGGLE_MODULO;
		p->jitter++;
		if (p->jitter >= JIGGLE_MODULO)
			p->jitter = 0;

	} else {
		readat = p->cursor;
	}

	readat += driftcomp;

	if (p->enable & (PARANOIA_MODE_OVERLAP | PARANOIA_MODE_VERIFY)) {
		flags = _pcalloc(totaltoread * CD_FRAMEWORDS, 1);
		new = new_c_block(p);
		recover_cache(p);
	} else {
		/*
		 * in the case of root it's just the buffer
		 */
		paranoia_resetall(p);
		new = new_c_block(p);
	}

	/*
	 * Do not use valloc() as valloc() in glibc is buggy and returns memory
	 * that cannot be passed to free().
	 */
	if (pagesize < 0) {
#ifdef	_SC_PAGESIZE
		pagesize = sysconf(_SC_PAGESIZE);
#else
		pagesize = getpagesize();
#endif
		if (pagesize < 0)
			pagesize = 4096;	/* Just a guess */
	}
	reduce = pagesize / CD_FRAMESIZE_RAW;
	bufbase = _pmalloc(totaltoread * CD_FRAMESIZE_RAW + pagesize);
	buffer = (Int16_t *)valign(bufbase, pagesize);
	sofar = 0;
	firstread = -1;

	/*
	 * actual read loop
	 */
	while (sofar < totaltoread) {
		long	secread = sectatonce;
		long	adjread = readat;
		long	thisread;

		/*
		 * don't under/overflow the audio session
		 */
		if (adjread < p->current_firstsector) {
			secread -= p->current_firstsector - adjread;
			adjread = p->current_firstsector;
		}
		if (adjread + secread - 1 > p->current_lastsector)
			secread = p->current_lastsector - adjread + 1;

		if (sofar + secread > totaltoread)
			secread = totaltoread - sofar;

		/*
		 * If we are inside the buffer, the transfers are no longer
		 * page aligned. Reduce the transfer size to avoid problems.
		 * Such problems are definitely known to appear on FreeBSD.
		 */
		if ((sofar > 0) && (secread > (sectatonce - reduce)))
			secread = sectatonce - reduce;

		if (secread > 0) {

			if (firstread < 0)
				firstread = adjread;
			if ((thisread = p->d_read(p->d, buffer + sofar * CD_FRAMEWORDS, adjread,
						secread)) < secread) {

				if (thisread < 0)
					thisread = 0;

				/*
				 * Uhhh... right.  Make something up. But
				 * don't make us seek backward!
				 */
				if (callback)
					(*callback) ((adjread + thisread) * CD_FRAMEWORDS, PARANOIA_CB_READERR);
				memset(buffer + (sofar + thisread) * CD_FRAMEWORDS, 0,
					CD_FRAMESIZE_RAW * (secread - thisread));
				if (flags)
					memset(flags + (sofar + thisread) * CD_FRAMEWORDS, 2,
						CD_FRAMEWORDS * (secread - thisread));
			}
			if (thisread != 0)
				anyflag = 1;

			if (flags && sofar != 0) {
				/*
				 * Don't verify across overlaps that are too
				 * close to one another
				 */
				int	i = 0;

				for (i = -MIN_WORDS_OVERLAP / 2; i < MIN_WORDS_OVERLAP / 2; i++)
					flags[sofar * CD_FRAMEWORDS + i] |= 1;
			}
			p->lastread = adjread + secread;

			if (adjread + secread - 1 == p->current_lastsector)
				new->lastsector = -1;

			if (callback)
				(*callback) ((adjread + secread - 1) * CD_FRAMEWORDS, PARANOIA_CB_READ);

			sofar += secread;
			readat = adjread + secread;
		} else if (readat < p->current_firstsector) {
			readat += sectatonce;
						/*
						 * due to being before the
						 * readable area
						 */
		} else {
			break;	/* due to being past the readable area */
		}
	}

	if (anyflag) {
		new->vector = _pmalloc(totaltoread * CD_FRAMESIZE_RAW);
		memcpy(new->vector, buffer, totaltoread * CD_FRAMESIZE_RAW);
		_pfree(bufbase);

		new->begin = firstread * CD_FRAMEWORDS - p->dyndrift;
		new->size = sofar * CD_FRAMEWORDS;
		new->flags = flags;
	} else {
		if (new)
			free_c_block(new);
		if (bufbase)
			_pfree(bufbase);
		if (flags)
			_pfree(flags);
		new = NULL;
		bufbase = NULL;
		flags = NULL;
	}
	return (new);
}

/*
 * The returned buffer is *not* to be freed by the caller.  It will
 * persist only until the next call to paranoia_read() for this p
 */
EXPORT Int16_t *
paranoia_read(p, callback)
	cdrom_paranoia	*p;
	void		(*callback) __PR((long, int));
{
	return (paranoia_read_limited(p, callback, 20));
}

/*
 * I added max_retry functionality this way in order to avoid breaking any
 * old apps using the nerw libs.  cdparanoia 9.8 will need the updated libs,
 * but nothing else will require it.
 */
EXPORT Int16_t *
paranoia_read_limited(p, callback, max_retries)
	cdrom_paranoia	*p;
	void		(*callback) __PR((long, int));
	int		max_retries;
{
	long		beginword = p->cursor * (CD_FRAMEWORDS);
	long		endword = beginword + CD_FRAMEWORDS;
	long		retry_count = 0;
	long		lastend = -2;
	root_block	*root = &p->root;

	if (beginword > p->root.returnedlimit)
		p->root.returnedlimit = beginword;
	lastend = re(root);

	/*
	 * First, is the sector we want already in the root?
	 */
	while (rv(root) == NULL ||
		rb(root) > beginword ||
		(re(root) < endword + p->maxdynoverlap &&
			p->enable & (PARANOIA_MODE_VERIFY | PARANOIA_MODE_OVERLAP)) ||
		re(root) < endword) {

		/*
		 * Nope; we need to build or extend the root verified range
		 */
		if (p->enable & (PARANOIA_MODE_VERIFY | PARANOIA_MODE_OVERLAP)) {
			i_paranoia_trim(p, beginword, endword);
			recover_cache(p);
			if (rb(root) != -1 && p->root.lastsector)
				i_end_case(p, endword + p->maxdynoverlap,
					callback);
			else
				i_stage2(p, beginword,
					endword + p->maxdynoverlap,
					callback);
		} else
			i_end_case(p, endword + p->maxdynoverlap,
				callback);	/* only trips if we're already */
						/* done */

		if (!(rb(root) == -1 || rb(root) > beginword ||
				re(root) < endword + p->maxdynoverlap))
			break;

		/*
		 * Hmm, need more.  Read another block
		 */
		{
			c_block	*new = i_read_c_block(p, beginword, endword, callback);

			if (new) {
				if (p->enable & (PARANOIA_MODE_OVERLAP | PARANOIA_MODE_VERIFY)) {

					if (p->enable & PARANOIA_MODE_VERIFY)
						i_stage1(p, new, callback);
					else {
						/*
						 * just make v_fragments from the
						 * boundary information.
						 */
						long	begin = 0,
							end = 0;

						while (begin < cs(new)) {
							while (end < cs(new) && (new->flags[begin] & 1))
								begin++;
							end = begin + 1;
							while (end < cs(new) && (new->flags[end] & 1) == 0)
								end++;
							{
								new_v_fragment(p, new, begin + cb(new),
									end + cb(new),
									(new->lastsector && cb(new) + end == ce(new)));
							}
							begin = end;
						}
					}

				} else {

					if (p->root.vector)
						i_cblock_destructor(p->root.vector);
					free_elem(new->e, 0);
					p->root.vector = new;

					i_end_case(p, endword + p->maxdynoverlap,
						callback);

				}
			}
		}

		/*
		 * Are we doing lots of retries?  **********************
		 *
		 * Check unaddressable sectors first.  There's no backoff
		 * here; jiggle and minimum backseek handle that for us
		 */
		if (rb(root) != -1 && lastend + 588 < re(root)) {
			/*
			 * If we've not grown half a sector
			 */
			lastend = re(root);
			retry_count = 0;
		} else {
			/*
			 * increase overlap or bail
			 */
			retry_count++;

			/*
			 * The better way to do this is to look at how many
			 * actual matches we're getting and what kind of gap
			 */
			if (retry_count % 5 == 0) {
				if (p->dynoverlap == p->maxdynoverlap ||
						retry_count == max_retries) {
					verify_skip_case(p, callback);
					retry_count = 0;
				} else {
					if (p->stage1.offpoints != -1) {	/* hack */
						p->dynoverlap *= 1.5;
						if (p->dynoverlap > p->maxdynoverlap)
							p->dynoverlap = p->maxdynoverlap;
						if (callback)
							(*callback) (p->dynoverlap, PARANOIA_CB_OVERLAP);
					}
				}
			}
		}
	}
	p->cursor++;

	return (rv(root) + (beginword - rb(root)));
}

/*
 * a temporary hack
 */
EXPORT void
paranoia_overlapset(p, overlap)
	cdrom_paranoia	*p;
	long		overlap;
{
	p->dynoverlap = overlap * CD_FRAMEWORDS;
	p->stage1.offpoints = -1;
}
