/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
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
 * This file contains modifications Copyright 2006-2009 J. Schilling
 *
 * @(#)newsid.c	1.4 09/11/01 J. Schilling
 */
#if defined(sun)
#ident "@(#)newsid.c 1.4 09/11/01 J. Schilling"
#endif
/*
 * @(#)newsid.c 1.7 06/12/12
 */

#if defined(sun)
#ident	"@(#)sidtoser.c"
#ident	"@(#)sccs:lib/comobj/sidtoser.c"
#endif
# include	<defines.h>

static int	in_pfile	__PR((struct sid *sp, struct packet *pkt));

void
newsid(pkt,branch)
register struct packet *pkt;
int branch;
{
	int chkbr;

	chkbr = 0;
	/* if branch value is 0 set newsid level to 1 */
	if (pkt->p_reqsid.s_br == 0) {
		pkt->p_reqsid.s_lev += 1;
		/*
		if the sid requested has been deltaed or the branch
		flag was set or the requested SID exists in the p-file
		then create a branch delta off of the gotten SID
		*/
		if (sidtoser(&pkt->p_reqsid,pkt) ||
			pkt->p_maxr > pkt->p_reqsid.s_rel || branch ||
			in_pfile(&pkt->p_reqsid,pkt)) {
				pkt->p_reqsid.s_rel = pkt->p_gotsid.s_rel;
				pkt->p_reqsid.s_lev = pkt->p_gotsid.s_lev;
				pkt->p_reqsid.s_br = pkt->p_gotsid.s_br + 1;
				pkt->p_reqsid.s_seq = 1;
				chkbr++;
		}
	}
	/*
	if a three component SID was given as the -r argument value
	and the b flag is not set then up the gotten SID sequence
	number by 1
	*/
	else if (pkt->p_reqsid.s_seq == 0 && !branch)
		pkt->p_reqsid.s_seq = pkt->p_gotsid.s_seq + 1;
	else {
		/*
		if sequence number is non-zero then increment the
		requested SID sequence number by 1
		*/
		pkt->p_reqsid.s_seq += 1;
		if (branch || sidtoser(&pkt->p_reqsid,pkt) ||
			in_pfile(&pkt->p_reqsid,pkt)) {
			pkt->p_reqsid.s_br += 1;
			pkt->p_reqsid.s_seq = 1;
			chkbr++;
		}
	}
	/*
	keep checking the requested SID until a good SID to be
	made is calculated or all possibilities have been tried
	*/
	while (chkbr) {
		--chkbr;
		while (in_pfile(&pkt->p_reqsid,pkt)) {
			pkt->p_reqsid.s_br += 1;
			++chkbr;
		}
		while (sidtoser(&pkt->p_reqsid,pkt)) {
			pkt->p_reqsid.s_br += 1;
			++chkbr;
		}
	}
	if (sidtoser(&pkt->p_reqsid,pkt) || in_pfile(&pkt->p_reqsid,pkt))
		fatal(gettext("bad SID calculated in newsid()"));
}

extern char *Sflags[];

static int
in_pfile(sp,pkt)
struct	sid	*sp;
struct	packet	*pkt;
{
	struct	pfile	pf;
	char	line[BUFSIZ];
	char	*p;
	FILE	*in;

	if (Sflags[JOINTFLAG - 'a']) {
		if (exists(auxf(pkt->p_file,'p'))) {
			in = xfopen(auxf(pkt->p_file,'p'), O_RDONLY|O_BINARY);
			while ((p = fgets(line,sizeof(line),in)) != NULL) {
				p[length(p) - 1] = 0;
				pf_ab(p,&pf,0);
				if (pf.pf_nsid.s_rel == sp->s_rel &&
					pf.pf_nsid.s_lev == sp->s_lev &&
					pf.pf_nsid.s_br == sp->s_br &&
					pf.pf_nsid.s_seq == sp->s_seq) {
						fclose(in);
						return(1);
				}
			}
			fclose(in);
			return(0);
		}
		else return(0);
	}
	else return(0);
	/*NOTREACHED*/
}
