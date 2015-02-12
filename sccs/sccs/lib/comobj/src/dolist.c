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
 * Copyright 1995 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2015 J. Schilling
 *
 * @(#)dolist.c	1.8 15/02/06 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)dolist.c 1.8 15/02/06 J. Schilling"
#endif
/*
 * @(#)dolist.c 1.5 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)dolist.c"
#pragma ident	"@(#)sccs:lib/comobj/dolist.c"
#endif
# include	<defines.h>
# include       <locale.h>

static char *getasid __PR((register char *p, register struct sid *sp));

void
dolist(pkt,list,ch)
struct packet *pkt;
register char *list;
char ch;
{
	struct sid lowsid, highsid, sid;
	int n;
	void    (*f_enter) __PR((struct packet *_pkt, int _ch, int _n, struct sid *sidp));

	f_enter	= pkt->p_enter;

	while (*list) {
		list = getasid(list,&lowsid);
		if (*list == '-') {
			++list;
			list = getasid(list,&highsid);
			if (lowsid.s_br == 0) {
				if ((highsid.s_br || highsid.s_seq ||
					highsid.s_rel < lowsid.s_rel ||
					(highsid.s_rel == lowsid.s_rel &&
					highsid.s_lev < lowsid.s_lev)))
				        fatal(gettext("bad range (co12)"));
				sid.s_br = sid.s_seq = 0;
				for (sid.s_rel = lowsid.s_rel;
				     sid.s_rel <= highsid.s_rel;
				     sid.s_rel++) {
				   sid.s_lev = (sid.s_rel == lowsid.s_rel?
					        lowsid.s_lev : 1);
				   if (sid.s_rel < highsid.s_rel) {
				      for (; (n = sidtoser(&sid,pkt)) != 0;
					   sid.s_lev++)
					 (*f_enter)(pkt,ch,n,&sid);
				   } else { /* == */
				      for (;
				           (sid.s_lev <= highsid.s_lev) && 
				           (n = sidtoser(&sid,pkt));
					   sid.s_lev++ )
					 (*f_enter)(pkt,ch,n,&sid);
				   }	 
				}
			}
			else {
				if (!(highsid.s_rel == lowsid.s_rel &&
					highsid.s_lev == lowsid.s_lev &&
					highsid.s_br == lowsid.s_br &&
					highsid.s_seq >= lowsid.s_seq))
				       fatal(gettext("bad range (co12)"));
				for (;
				     (lowsid.s_seq <= highsid.s_seq) &&
				     (n = sidtoser(&lowsid,pkt));
				     lowsid.s_seq++ )
				   (*f_enter)(pkt,ch,n,&lowsid);
			}
		}
		else {
			if ((n = sidtoser(&lowsid,pkt)) != 0)
				(*f_enter)(pkt,ch,n,&lowsid);
		}
		if (*list == ',')
			++list;
	}
}


static char *
getasid(p,sp)
register char *p;
register struct sid *sp;
{
	register char *old;

	p = sid_ab(old = p,sp);
	if (old == p || sp->s_rel == 0)
		fatal(gettext("delta list syntax (co13)"));
	if (sp->s_lev == 0) {
		sp->s_lev = MAXL;
		if (sp->s_br || sp->s_seq)
			fatal(gettext("delta list syntax (co13)"));
	}
	else if (sp->s_br) {
		if (sp->s_seq == 0)
			sp->s_seq = MAXL;
	}
	else if (sp->s_seq)
		fatal(gettext("delta list syntax (co13)"));
	return(p);
}
