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
 * This file contains modifications Copyright 2009 J. Schilling
 *
 * @(#)pf_ab.c	1.3 09/11/08 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)pf_ab.c 1.3 09/11/08 J. Schilling"
#endif
/*
 * @(#)pf_ab.c 1.6 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)pf_ab.c"
#pragma ident	"@(#)sccs:lib/comobj/pf_ab.c"
#endif
# include	<defines.h>

void
pf_ab(s,pp,all)
char *s;
register struct pfile *pp;
int all;
{
	register char *p;
	register int i;
	extern char *Datep;
	char *xp;
	char stmp[MAXLINE];

	xp = p = stmp;
	copy(s,p);
	for (; *p; p++)
		if (*p == '\n') {
			*p = 0;
			break;
		}
	p = xp;
	p = sid_ab(p,&pp->pf_gsid);
	++p;
	p = sid_ab(p,&pp->pf_nsid);
	++p;
	i = sccs_index(p," ");
	pp->pf_user[0] = 0;
	if (((unsigned)i) < LOGSIZE) {
		strncpy(pp->pf_user,p,(unsigned) i);
		pp->pf_user[i] = 0;
	}
	else
		fatal(gettext("bad p-file format (co17)"));
	p = p + i + 1;
	date_ab(p,&pp->pf_date);
	p = Datep;
	pp->pf_ilist = 0;
	pp->pf_elist = 0;
	pp->pf_cmrlist = 0;
	if (!all || !*p)
		return;
	p += 2;
	xp = fmalloc(size(p));
	copy(p,xp);
	p = xp;
	if (*p == 'i') {
		pp->pf_ilist = ++p;
		for (; *p; p++)
			if (*p == ' ') {
				*p++ = 0;
				p++;
				break;
			}
	}
	if (*p == 'x')
		{
		pp->pf_elist = ++p;
		for(;*p;p++)
			if(*p == ' ')
			{
				*p++ = 0;
				p++;
				break;
			}
	}
	if(*p == 'z')
		{
		pp->pf_cmrlist = ++p;
		}
}
