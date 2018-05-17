/* @(#)substcmds.c	1.23 09/07/09 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)substcmds.c	1.23 09/07/09 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	Substitution commands called from ECS : command line
 *
 *	Copyright (c) 1986-2009 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include "ved.h"

#define	OLDSIZE	4096

#define	iswhite(c)	((c) == ' ' || (c) == '\t')

LOCAL	BOOL	subverbose;

EXPORT	void	subst		__PR((ewin_t *wp, Uchar* cmd, int cmdlen));
LOCAL	void	substitute	__PR((ewin_t *wp, Uchar* from, long fromlen, Uchar* to, long tolen));
LOCAL	BOOL	simpleto	__PR((Uchar* s, long len));
LOCAL	void	catsub		__PR((ewin_t *wp, Uchar* old, long oldlen, Uchar* to, long tolen));

/*
 * This is the external callable subsitution command routine.
 * It parses the command line and then calls the 'real' substitution command.
 * Syntax is: ESC: sub /old/new/v
 */
EXPORT void
subst(wp, cmd, cmdlen)
	ewin_t	*wp;
	Uchar	*cmd;
	int	cmdlen;
{
	register Uchar	*from;
	register Uchar	*to;
	register Uchar	*cp;
	register Uchar	*endp;
	register Uchar	c = '/';
	register Uchar	dc;
		long	fromlen;
		long	tolen;

	from = cmd;
	endp = &cmd[cmdlen];

	while (from < endp) {
		c = *from;
		if (iswhite(c))
			from++;
		else
			break;
	}
	dc = c;
	to = ++from;
	while (to < endp) {
		c = *to;
		if (c != dc)
			to++;
		else
			break;
	}
	fromlen = to-from;
	*to++ = '\0';
	cp = to;
	while (cp < endp) {
		c = *cp;
		if (c != dc)
			cp++;
		else
			break;
	}
	tolen = cp-to;
	*cp = '\0';
	subverbose = FALSE;
	if (++cp < endp && *cp == 'v')
		subverbose = TRUE;

	if (subverbose) {
		writeerr(wp, "'%s'%s'(%ld,%ld)", from, to, fromlen, tolen);
		sleep(1);
	}
	substitute(wp, from, fromlen, to, tolen);
}

/*
 * This is the 'real' sunstitution routine.
 * It gets called with pre-parsed strings.
 */
LOCAL void
substitute(wp, from, fromlen, to, tolen)
	ewin_t	*wp;
	Uchar	*from;
	long	fromlen;
	Uchar	*to;
	long	tolen;
{
/*	epos_t	savedot = dot;*/
	epos_t	newdot;
	epos_t	omark = wp->mark;
	int	omarkvalid = wp->markvalid;
	ecnt_t	n = wp->curnum;
	Uchar	old[NAMESIZE+1];
	long	oldlen = 0;
	BOOL	tosimple;

	if (fromlen == 0)
		return;
	tosimple = simpleto(to, tolen);

	while (n--) {
		/*
		 * Search the next occurence of the 'from' string.
		 * Use a temporary mark to tell whre the found pattern starts.
		 */
		if ((newdot = search(wp, wp->dot, from, fromlen, 1)) > wp->eof) {
			if (newdot == (wp->eof+2))
			not_found(wp);
			break;
		} else {
			wp->dot = newdot;
		}

		if (!tosimple) {
			/*
			 * We need to remember the old 'from' string before.
			 */
			oldlen = wp->dot-wp->mark > NAMESIZE ?
					NAMESIZE : (long)wp->dot-wp->mark;
			oldlen = extract(wp, wp->mark, old, (int)oldlen);
			if (subverbose) {
				writeerr(wp, "%ld'%s'", oldlen, old);
				sleep(1);
			}
		}
		/*
		 * Now delete the old string in the buffer
		 * and insert substitution
		 */
		rubchars(wp, wp->dot-wp->mark);	/* Mark is before dot */
		if (tosimple)
			insert(wp, to, tolen);
		else
			catsub(wp, old, oldlen, to, tolen);
		dispup(wp, wp->dot, wp->mark);
	}

	/*
	 * Re-set mark to remembered place
	 */
	resetmark(wp);
	if (omarkvalid)
		setmark(wp, omark);
}

/*
 * Check is this is a 'simple' 'to'-substitution string
 * that does not require to be expanded via 'catsub()'.
 */
LOCAL BOOL
simpleto(s, len)
	register Uchar	*s;
	register long	len;
{
	register Uchar	c;

	if (len <= 0)
		return (TRUE);
	while (--len >= 0) {
		c = *s++;
		if (c == '\\' || c == '&')
			return (FALSE);
	}
	return (TRUE);
}

/*
 * Insert the substitution string.
 * The '&' character in the to string is substituted with the old from string.
 */
LOCAL void
catsub(wp, old, oldlen, to, tolen)
		ewin_t	*wp;
	register Uchar	*old;
	register long	oldlen;
	register Uchar	*to;
	register long	tolen;
{
	if (tolen <= 0)
		return;

	while (--tolen >= 0) {
		if (*to == '\\') {
			if (--tolen >= 0)
				insert(wp, ++to, 1L);
		} else if (*to == '&') {
			insert(wp, old, oldlen);
		} else {
			insert(wp, to, 1L);
		}
		to++;
	}
}
