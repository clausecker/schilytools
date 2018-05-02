/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2011-2018 J. Schilling
 *
 * @(#)del_ab.c	1.8 18/04/29 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)del_ab.c 1.8 18/04/29 J. Schilling"
#endif
/*
 * @(#)del_ab.c 1.7 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)del_ab.c"
#pragma ident	"@(#)sccs:lib/comobj/del_ab.c"
#endif
#include	<defines.h>

char
del_ab(p, dt, pkt)
register char *p;
register struct deltab *dt;
struct packet *pkt;
{
	int n;
	int n2;
	extern char *Datep;

	if (*p++ != CTLCHAR)
		fmterr(pkt);
	if (*p++ != BDELTAB)
		return (*--p);
	NONBLANK(p);
	dt->d_type = *p++;
	NONBLANK(p);
	p = sid_ab(p, &dt->d_sid);
	NONBLANK(p);
	if (date_abz(p, &dt->d_dtime, pkt->p_flags) > 0) {
		if (Ffile) {
			fprintf(stderr, gettext("WARNING [%s]: date format violation at line %d\n"),
					Ffile, pkt->p_slnno);
		} else {
			fprintf(stderr, gettext("WARNING: date format violation at line %d\n"),
					pkt->p_slnno);
		}
	}
	p = Datep;
	NONBLANK(p);
	if ((n2 = n = sccs_index(p, " ")) < 0)
		fmterr(pkt);
	if (++n2 > sizeof (dt->d_pgmr))
		n2 = sizeof (dt->d_pgmr);
	strlcpy(dt->d_pgmr, p, (unsigned) n2);
	p += n + 1;
	NONBLANK(p);
	p = satoi(p, &dt->d_serial);
	NONBLANK(p);
	p = satoi(p, &dt->d_pred);
	if (*p != '\n')
		fmterr(pkt);
	return (BDELTAB);
}

/*
 *	We assume that p looks like "\001d D 1.10 97/08/07 12:52:00 vvg 10 9\n"
 */
void
get_Del_Date_time(p, dt, pkt, p_tm)
	register char	*p;
	struct deltab	*dt;
	struct packet	*pkt;
	struct tm	*p_tm;
{
	char	* cp;

	cp  = p;
	cp += 2;
	NONBLANK(cp);
	dt->d_type = *cp++;
	NONBLANK(cp);
	cp = sid_ab(cp, &dt->d_sid);
	NONBLANK(cp);
	if (mystrptime(cp, p_tm, 1) > 0) {
		if (Ffile) {
			fprintf(stderr, gettext("WARNING [%s]: date format violation at line %d\n"),
					Ffile, pkt->p_slnno);
		} else {
			fprintf(stderr, gettext("WARNING: date format violation at line %d\n"),
					pkt->p_slnno);
		}
	}
}
