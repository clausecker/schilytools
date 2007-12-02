/* @(#)overlap.c	1.13 06/09/13 J. Schilling from cdparanoia-III-alpha9.8 */
#ifndef lint
static	char sccsid[] =
"@(#)overlap.c	1.13 06/09/13 J. Schilling from cdparanoia-III-alpha9.8";

#endif
/*
 * CopyPolicy: GNU Lesser General Public License v2.1 applies
 * Copyright (C) 1997-2001 by Monty (xiphmont@mit.edu)
 * Copyright (C) 2002-2006 by J. Schilling
 *
 * Statistic code and cache management for overlap settings
 *
 */

#include <schily/mconfig.h>
#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include "p_block.h"
#include "cdda_paranoia.h"
#include "overlap.h"
#include "isort.h"

EXPORT	void	paranoia_resetcache	__PR((cdrom_paranoia * p));
EXPORT	void	paranoia_resetall	__PR((cdrom_paranoia * p));
EXPORT	void	i_paranoia_trim		__PR((cdrom_paranoia * p,
						long beginword, long endword));
EXPORT	void	offset_adjust_settings	__PR((cdrom_paranoia * p,
						void (*callback) (long, int)));
EXPORT	void	offset_add_value	__PR((cdrom_paranoia * p,
						offsets * o, long value,
						void (*callback) (long, int)));

/*
 * Internal cache management
 */
EXPORT void
paranoia_resetcache(p)
	cdrom_paranoia	*p;
{
	c_block		*c = c_first(p);
	v_fragment	*v;

	while (c) {
		free_c_block(c);
		c = c_first(p);
	}

	v = v_first(p);
	while (v) {
		free_v_fragment(v);
		v = v_first(p);
	}
}

EXPORT void
paranoia_resetall(p)
	cdrom_paranoia	*p;
{
	p->root.returnedlimit = 0;
	p->dyndrift = 0;
	p->root.lastsector = 0;

	if (p->root.vector) {
		i_cblock_destructor(p->root.vector);
		p->root.vector = NULL;
	}
	paranoia_resetcache(p);
}

EXPORT void
i_paranoia_trim(p, beginword, endword)
	cdrom_paranoia	*p;
	long		beginword;
	long		endword;
{
	root_block	*root = &(p->root);

	if (root->vector != NULL) {
		long	target = beginword - p->maxdynoverlap;
		long	rbegin = cb(root->vector);
		long	rend = ce(root->vector);

		if (rbegin > beginword)
			goto rootfree;

		if (rbegin + p->maxdynoverlap < beginword) {
			if (target + MIN_WORDS_OVERLAP > rend)
				goto rootfree;

			{
				long	offset = target - rbegin;

				c_removef(root->vector, offset);
			}
		} {
			c_block	*c = c_first(p);

			while (c) {
				c_block	*next = c_next(c);

				if (ce(c) < beginword - p->maxdynoverlap)
					free_c_block(c);
				c = next;
			}
		}

	}
	return;

rootfree:

	i_cblock_destructor(root->vector);
	root->vector = NULL;
	root->returnedlimit = -1;
	root->lastsector = 0;

}

/*
 * Statistical and heuristic[al? :-] management
 */
EXPORT void
offset_adjust_settings(p, callback)
	cdrom_paranoia	*p;
	void		(*callback) __PR((long, int));
{
	if (p->stage2.offpoints >= 10) {
		/*
		 * drift: look at the average offset value.  If it's over one
		 * sector, frob it.  We just want a little hysteresis [sp?]
		 */
		long	av = (p->stage2.offpoints ? p->stage2.offaccum / p->stage2.offpoints : 0);

		if (abs(av) > p->dynoverlap / 4) {
			av = (av / MIN_SECTOR_EPSILON) * MIN_SECTOR_EPSILON;

			if (callback)
				(*callback) (ce(p->root.vector), PARANOIA_CB_DRIFT);
			p->dyndrift += av;

			/*
			 * Adjust all the values in the cache otherwise we get
			 * a (potentially unstable) feedback loop
			 */
			{
				c_block		*c = c_first(p);
				v_fragment	*v = v_first(p);

				while (v && v->one) {
					/*
					 * safeguard beginning bounds case with
					 * a hammer
					 */
					if (fb(v) < av || cb(v->one) < av) {
						v->one = NULL;
					} else {
						fb(v) -= av;
					}
					v = v_next(v);
				}
				while (c) {
					long	adj = min(av, cb(c));

					c_set(c, cb(c) - adj);
					c = c_next(c);
				}
			}

			p->stage2.offaccum = 0;
			p->stage2.offmin = 0;
			p->stage2.offmax = 0;
			p->stage2.offpoints = 0;
			p->stage2.newpoints = 0;
			p->stage2.offdiff = 0;
		}
	}
	if (p->stage1.offpoints >= 10) {
		/*
		 * dynoverlap: we arbitrarily set it to 4x the running
		 * difference value, unless min/max are more
		 */
		p->dynoverlap = (p->stage1.offpoints ? p->stage1.offdiff /
			p->stage1.offpoints * 3 : CD_FRAMEWORDS);

		if (p->dynoverlap < -p->stage1.offmin * 1.5)
			p->dynoverlap = -p->stage1.offmin * 1.5;

		if (p->dynoverlap < p->stage1.offmax * 1.5)
			p->dynoverlap = p->stage1.offmax * 1.5;

		if (p->dynoverlap < p->mindynoverlap)
			p->dynoverlap = p->mindynoverlap;
		if (p->dynoverlap > p->maxdynoverlap)
			p->dynoverlap = p->maxdynoverlap;

		if (callback)
			(*callback) (p->dynoverlap, PARANOIA_CB_OVERLAP);

		if (p->stage1.offpoints > 600) {
			/*
			 * bit of a bug; this routine is called too often
			 * due to the overlap mesh alg we use in stage 1
			 */
			p->stage1.offpoints /= 1.2;
			p->stage1.offaccum /= 1.2;
			p->stage1.offdiff /= 1.2;
		}
		p->stage1.offmin = 0;
		p->stage1.offmax = 0;
		p->stage1.newpoints = 0;
	}
}

EXPORT void
offset_add_value(p, o, value, callback)
	cdrom_paranoia	*p;
	offsets		*o;
	long		value;
	void		(*callback) __PR((long, int));
{
	if (o->offpoints != -1) {

		o->offdiff += abs(value);
		o->offpoints++;
		o->newpoints++;
		o->offaccum += value;
		if (value < o->offmin)
			o->offmin = value;
		if (value > o->offmax)
			o->offmax = value;

		if (o->newpoints >= 10)
			offset_adjust_settings(p, callback);
	}
}
