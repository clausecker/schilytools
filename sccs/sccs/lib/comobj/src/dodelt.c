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
 * Copyright 2006-2019 J. Schilling
 *
 * @(#)dodelt.c	1.27 19/11/08 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)dodelt.c 1.27 19/11/08 J. Schilling"
#endif
/*
 * @(#)dodelt.c 1.8 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)dodelt.c"
#pragma ident	"@(#)sccs:lib/comobj/dodelt.c"
#endif
#include	<defines.h>
#include	<had.h>

# define ONEYEAR 31536000L

static char	getadel	__PR((struct packet *,struct deltab *));
static void	doixg	__PR((char *,struct ixg **));

dtime_t	Timenow;

struct idel *
dodelt(pkt,statp,sidp,type)
register struct packet *pkt;
struct stats *statp;
struct sid *sidp;
char type;
{
	char *c = NULL;
	struct deltab dt;
	register struct idel *rdp = NULL;
	int n, founddel;
	int	lhash;
	register char *p;
	time_t	TN;
	int	Tns;

	pkt->p_idel = 0;
	founddel = 0;

	dtime(&Timenow);
	TN = Timenow.dt_sec;
	Tns = Timenow.dt_nsec;
#if defined(BUG_1205145) || defined(GMT_TIME)
	TN += Timenow.dt_zone;
#else
	if (pkt->p_flags & PF_GMT)
		TN += Timenow.dt_zone;
#endif
	stats_ab(pkt,statp);
	lhash = pkt->p_clhash;
	while (getadel(pkt,&dt) == BDELTAB) {
		if (pkt->p_idel == 0) {
			if ((TN < dt.d_dtime.dt_sec) ||
			    ((TN == dt.d_dtime.dt_sec) &&
			    (Tns < dt.d_dtime.dt_nsec))) {
				fprintf(stderr,
				    gettext(
			"Time stamp later than current clock time (co10)\n"));
			}
			pkt->p_idel = (struct idel *)
					fmalloc((unsigned) (n=((dt.d_serial+1)*
					sizeof (*pkt->p_idel))));
			zero((char *) pkt->p_idel,n);
			pkt->p_apply = (struct apply *)
					fmalloc((unsigned) (n=((dt.d_serial+1)*
					sizeof (*pkt->p_apply))));
			zero((char *) pkt->p_apply,n);
			if (pkt->p_pgmrs != NULL) {
				pkt->p_pgmrs = (char **)
					fmalloc((unsigned) (n=((dt.d_serial+1)*
					sizeof (char *))));
				zero((char *) pkt->p_pgmrs, n);
			}
			pkt->p_idel->i_pred = dt.d_serial;
		}
		if (dt.d_type == 'D' ||				/* Delta */
		    dt.d_type == 'U') {				/* Unlink */
			/*
			 * sidp is != NULL only for rmchg
			 */
			if (sidp && eqsid(&dt.d_sid,sidp)) {
				copy(dt.d_pgmr, pkt->p_pgmr);	/* for rmchg */
				zero((char *) sidp,sizeof (*sidp));
				founddel = 1;
				pkt->p_first_esc = 1;
				pkt->p_first_cmt = 1;
				pkt->p_cdid_mrs = 0;
				for (p = pkt->p_line;
				    *p && *p != dt.d_type; p++) {
					;
				}
				if (*p) {
					/*
					 * Also correct saved line hash, used
					 * for putline() optimization.
					 */
					pkt->p_clhash -= dt.d_type;   /* D/U */
					pkt->p_uclhash -= dt.d_type;  /* D/U */
					pkt->p_clhash += type;
					pkt->p_uclhash += type;
					*p = type;
				}
				if (type == 0) {
					/*
					 * Go back before last stats line. The
					 * value 21 is the lenght of any stats
					 * line. Never change this length.
					 */
					pkt->p_nhash -= lhash;
					fseek(pkt->p_xiop, (off_t)-21, SEEK_CUR);
					/*
					 * Skip this delta table entry.
					 */
					pkt->p_wrttn = 1;
					while ((c = getline(pkt)) != NULL) {
						pkt->p_wrttn = 1;
						if (pkt->p_line[0] != CTLCHAR)
							break;
						if (pkt->p_line[1] == EDELTAB)
							break;
					}
				}
			}
			else
				pkt->p_first_esc = pkt->p_first_cmt = founddel = 0;
			pkt->p_maxr = max(pkt->p_maxr,dt.d_sid.s_rel);
			if (pkt->p_pgmrs != NULL) {
				pkt->p_pgmrs[dt.d_serial] =
						lhash_lookup(dt.d_pgmr);
			}
			rdp = &pkt->p_idel[dt.d_serial];
			rdp->i_sid.s_rel = dt.d_sid.s_rel;
			rdp->i_sid.s_lev = dt.d_sid.s_lev;
			rdp->i_sid.s_br = dt.d_sid.s_br;
			rdp->i_sid.s_seq = dt.d_sid.s_seq;
			rdp->i_pred = dt.d_pred;
			rdp->i_dtype = dt.d_type;
			rdp->i_datetime.tv_sec = dt.d_dtime.dt_sec;
			rdp->i_datetime.tv_nsec = dt.d_dtime.dt_nsec;
			if (founddel && type == 0)	/* Already skipped */
				goto nextdelta;
		}
		while ((c = getline(pkt)) != NULL)
			if (pkt->p_line[0] != CTLCHAR)
				break;
			else {
				switch (pkt->p_line[1]) {
				case EDELTAB:
					break;
				case COMMENTS:
					if (pkt->p_line[2] == '_')
						sidext_v4compat_ab(pkt, &dt);
					/* FALLTHRU */
				case MRNUM:
					if (founddel)
					{
						(*pkt->p_escdodelt)(pkt);
						if (type == 'R' && HADZ && pkt->p_line[1] == MRNUM) {
							(*pkt->p_fredck)(pkt);
						}
					}
				continue;

				case SIDEXTENS:
					if (pkt->p_flags & PF_V6) {
						sidext_ab(pkt, &dt, &pkt->p_line[2]);
						continue;
					}
					/* FALLTHRU */
				default:
					fmterr(pkt);
				/*FALLTHRU*/
				case INCLUDE:
				case EXCLUDE:
				case IGNORE:
					/*
					 * Is this really needed for type 'U' as well?
					 */
					if (dt.d_type == 'D' ||
					    dt.d_type == 'U') {
						doixg(pkt->p_line,&rdp->i_ixg);
					}
					continue;
				}
				break;
			}
	nextdelta:
		if (c == NULL || pkt->p_line[0] != CTLCHAR || getline(pkt) == NULL)
			fmterr(pkt);
		if (pkt->p_line[0] != CTLCHAR || pkt->p_line[1] != STATS)
			break;
		lhash = pkt->p_clhash;
	}
	return (pkt->p_idel);
}

static char
getadel(pkt,dt)
register struct packet *pkt;
register struct deltab *dt;
{
	if (getline(pkt) == NULL)
		fmterr(pkt);
	return (del_ab(pkt->p_line,dt, pkt));
}

static void
doixg(p,ixgp)
char *p;
struct ixg **ixgp;
{
	int *v, *ip;
	int type, cnt, i;
	struct ixg *curp, *prevp;
	int xtmp[MAXLINE];

	v = ip = xtmp;
	++p;
	type = *p++;
	NONBLANK(p);
	while (numeric(*p)) {
		if ((ip - v) >= MAXLINE)
			fatal(gettext("too many include exclude or ignore entries (co30)"));
		p = satoi(p,ip++);
		NONBLANK(p);
	}
	cnt = ip - v;
	for (prevp = curp = (*ixgp); curp; curp = (prevp = curp)->i_next)
		;
	curp = (struct ixg *) fmalloc((unsigned)
		(sizeof (struct ixg) + (cnt-1)*sizeof (curp->i_ser[0])));
	if (*ixgp == 0)
		*ixgp = curp;
	else
		prevp->i_next = curp;
	curp->i_next = 0;
	curp->i_type = (char) type;
	curp->i_cnt = (char) cnt;
	for (i=0; cnt>0; --cnt)
		curp->i_ser[i++] = *v++;
}
