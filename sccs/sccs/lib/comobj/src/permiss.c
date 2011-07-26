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
 * This file contains modifications Copyright 2006-2011 J. Schilling
 *
 * @(#)permiss.c	1.12 11/06/29 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)permiss.c 1.12 11/06/29 J. Schilling"
#endif
/*
 * @(#)permiss.c 1.9 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)permiss.c"
#pragma ident	"@(#)sccs:lib/comobj/permiss.c"
#endif
# define	NEED_PRINTF_J		/* Need defines for js_snprintf()? */
# include	<defines.h>
# include       <i18n.h>

# define	BLANK(l)	while (!(*l == '\0' || *l == ' ' || *l == '\t')) l++;

static void ck_lock __PR((char *p, struct packet *pkt));

void
finduser(pkt)
register struct packet *pkt;
{
	register char *p;
	char	*user;
	char groupid[32];
	int none;
	int ok_user;
	extern char saveid[];

	none = 1;
#if 0
	user = logname();
#else
	user = saveid;
#endif
	snprintf(groupid, sizeof (groupid), "%ju", (UIntmax_t)getgid());
	while ((p = getline(pkt)) != NULL && *p != CTLCHAR) {
		none = 0;
		ok_user = 1;
		repl(p,'\n','\0');	/* this is done for equal test below */
		if(*p == '!') {
			++p;
			ok_user = 0;
			}
		if (!pkt->p_user)
			if (equal(user,p) || equal(groupid,p))
				pkt->p_user = ok_user;
		*(strend(p)) = '\n';	/* repl \0 end of line w/ \n again */
	}
	if (none)
		pkt->p_user = 1;
	if (p == NULL || p[1] != EUSERNAM)
		fmterr(pkt);
}


char	*Sflags[NFLAGS];	/* sync with hdr/defines.h */
char	SCOx;			/* sync with hdr/defines.h */

void
doflags(pkt)
struct packet *pkt;
{
	register char *p;
	register int k;

	for (k = 0; k < NFLAGS; k++)
		Sflags[k] = 0;
	while ((p = getline(pkt)) != NULL && *p++ == CTLCHAR && *p++ == FLAG) {
		NONBLANK(p);
		k = *p++ - 'a';
		if (k < 0 || k >= NFLAGS) {
			if (Ffile) {
				fprintf(stderr,
					gettext(
					"WARNING [%s]: unsupported flag at line %d\n"),
					Ffile, pkt->p_slnno);
			} else {
				fprintf(stderr,
					gettext(
					"WARNING: unsupported flag at line %d\n"),
					pkt->p_slnno);
			}
			continue;
		}
		NONBLANK(p);
		Sflags[k] = fmalloc(size(p));
		copy(p,Sflags[k]);
		for (p = Sflags[k]; *p++ != '\n'; )
			;
		*--p = 0;
	}

	/*
	 * We only support type "SCHILY" extensions. Check for "SCHILY" in the
	 * list of extensions.
	 * We ignore the SCO SCCS meaning for 'x': make files executable.
	 * We don't need the SCO SCCS meaning as a simple chmod +x SCCS/s.file
	 * is sufficient.
	 */
	p =  Sflags[EXTENSFLAG-'a'];
	if (p == NULL)			/* 'x' flag not set */
		return;
	NONBLANK(p);
	if (*p == '\0')
		SCOx++;
	for (k = 0; *p; ) {
		char	*p2;

		NONBLANK(p);
		p2 = p;
		BLANK(p2);
		if ((p2 - p) == 6 && strncmp("SCHILY", p, 6) == 0)
			k++;
		p = p2;
	}
	if (k == 0) {
		ffree(Sflags[EXTENSFLAG-'a']);
		Sflags[EXTENSFLAG-'a'] = NULL;
	}
}

void
permiss(pkt)
register struct packet *pkt;
{
	register char *p;
	register int n;
	extern char SccsError[];

	if (!pkt->p_user)
		fatal(gettext("not authorized to make deltas (co14)"));
	if ((p = Sflags[FLORFLAG - 'a']) != NULL) {
		if (((unsigned)pkt->p_reqsid.s_rel) < (n = patoi(p))) {
			sprintf(SccsError, gettext("release %d < %d floor (co15)"),
				pkt->p_reqsid.s_rel,
				n);
			fatal(SccsError);
		}
	}
	if ((p = Sflags[CEILFLAG - 'a']) != NULL) {
		if (((unsigned)pkt->p_reqsid.s_rel) > (n = patoi(p))) {
			sprintf(SccsError,gettext("release %d > %d ceiling (co16)"),
				pkt->p_reqsid.s_rel,
				n);
			fatal(SccsError);
		}
	}
	/*
	check to see if the file or any particular release is
	locked against editing. (Only if the `l' flag is set)
	*/
	if ((p = Sflags[LOCKFLAG - 'a']) != NULL)
		ck_lock(p,pkt);
}



/* 
 * Multiply  space needed for C locale by 3.  This should be adequate
 * for the longest localized strings.  The length is
 * strlen("SCCS file locked against editing (co23)") * 3 + 1)
 */
static char l_str[121];	

static void
ck_lock(p,pkt)
register char *p;
register struct packet *pkt;
{
	int l_rel;
	int locked;

	strcpy(l_str, (const char *) NOGETTEXT("SCCS file locked against editing (co23)"));
	locked = 0;
	if (*p == 'a')
		locked++;
	else while(*p) {
		p = satoi(p,&l_rel);
		++p;
		if (l_rel == pkt->p_gotsid.s_rel || l_rel == pkt->p_reqsid.s_rel) {
			locked++;
			sprintf(l_str, gettext("release `%d' locked against editing (co23)"),
				l_rel);
			break;
		}
	}
	if (locked)
		fatal(l_str);
}
