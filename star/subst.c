/* @(#)subst.c	1.9 06/10/31 Copyright 1986,2003-2006 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)subst.c	1.9 06/10/31 Copyright 1986,2003-2006 J. Schilling";
#endif
/*
 *	Substitution commands
 *
 *	Copyright (c) 1986,2003-2006 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/standard.h>
#include <schily/patmatch.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/schily.h>

#include <schily/patmatch.h>

#include "star.h"
#include "starsubs.h"

EXPORT	int	parsesubst	__PR((char *cmd, BOOL *arg));
EXPORT	BOOL	subst		__PR((FINFO *info));
LOCAL	char	*substitute	__PR((char *from, long fromlen, int idx, char *to, long tolen));
LOCAL	BOOL	simpleto	__PR((char *s, long len));
LOCAL	int	catsub		__PR((char *here, char *old, long oldlen, char *to, long tolen, char *limit));
EXPORT	BOOL	ia_change	__PR((TCB *ptb, FINFO *info));
LOCAL	BOOL	pax_change	__PR((TCB *ptb, FINFO *info));

#define	NPAT	100
LOCAL	int	npat;
LOCAL	Uchar	*pat[NPAT];
LOCAL	int	patlen[NPAT];
LOCAL	int	maxplen;
LOCAL	char	*substpat[NPAT];
LOCAL	int	substlen[NPAT];
LOCAL	int	*aux[NPAT];
LOCAL	int	alt[NPAT];
LOCAL	int	*state;
LOCAL	Int32_t	substcnt[NPAT];

extern	FILE	*tty;
extern	FILE	*vpr;
extern	int	verbose;
extern	BOOL	xflag;
extern	BOOL	nflag;
extern	BOOL	debug;
extern	BOOL	paxinteract;

/*
 * This is the command line parser for tar/pax substitution commands.
 * Syntax is: -s '/old/new/v'
 */
EXPORT int
parsesubst(cmd, arg)
	char	*cmd;
	BOOL	*arg;
{
	register char	*from;
	register char	*to;
	register char	*cp;
	register char	*endp;
	register char	c = '/';
	register char	dc;
		long	fromlen;
		long	tolen;
		int	cmdlen;
		char	*subopts = NULL;
		BOOL	printsubst = FALSE;
		Int32_t	count = 1;

	if (debug) {
		error("Add subst pattern: '%s'", cmd);
	}

	cmdlen = strlen(cmd);
	from = cmd;
	endp = &cmd[cmdlen];

	dc = c = *from;
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
	if (++cp < endp)
		subopts = cp;

	while (cp < endp) {
		c = *cp++;
		if (c == 'p') {
			printsubst = TRUE;
		} else if (c == 'g') {
			count = MAXINT32;
		} else {
			comerrno(EX_BAD, "Bad substitute option '%c'.\n", c);
		}
	}

	if (debug) {
		error("  '%s'%s'(%ld,%ld) opts '%s' simpleto: %d\n",
			from, to, fromlen, tolen,
			subopts, simpleto(to, tolen));
	}

	if (npat >= NPAT)
		comerrno(EX_BAD, "Too many substitute patterns (max is %d).\n", NPAT);

	pat[npat] = (Uchar *)__savestr(from);
	patlen[npat] = fromlen;
	substpat[npat] = __savestr(to);
	substlen[npat] = tolen;


	if (fromlen > maxplen)
		maxplen = fromlen;

	aux[npat] = __malloc(fromlen*sizeof (int), "compiled subst pattern");
	if ((alt[npat] = patcompile(pat[npat], patlen[npat], aux[npat])) == 0) {
		comerrno(EX_BAD, "Bad pattern: '%s'.\n", pat[npat]);
		return (-2);
	}

	if (printsubst)
		count *= -1;
	substcnt[npat] = count;
	*arg = TRUE;
	npat++;
	return (1);
}


EXPORT BOOL
subst(info)
	FINFO	*info;
{
	char	*to = NULL;
	register int	i;

	if (!state) {
		state = __malloc((maxplen+1)*sizeof (int), "pattern state");
	}

	info->f_namelen = strlen(info->f_name);
	/*
	 * Schleife über alle match & Subst Patterns
	 */
	for (i = 0; i < npat; i++) {
		to = substitute(info->f_name, info->f_namelen, i, substpat[i], substlen[i]);
		if (to)
			break;
	}
	if (to) {
		if (substcnt[i] < 0)
			error("%s >> %s\n", info->f_name, to);
		info->f_namelen = strlen(to);
		info->f_name = to;
		return (TRUE);
	}

	return (FALSE);
}


LOCAL	char	new[PATH_MAX+1];
/*
 * This is the 'real' substitution routine.
 * It gets called with pre-parsed strings.
 */
LOCAL char *
substitute(from, fromlen, idx, to, tolen)
	char	*from;
	long	fromlen;
	int	idx;
	char	*to;
	long	tolen;
{
	char	old[PATH_MAX+1];
	char	xold[PATH_MAX+1];
	long	oldlen = 0;
	BOOL	tosimple;
	Int32_t n = substcnt[idx];
	char	*end;
	char	*string;
	int	slen;
	BOOL	didmatch = FALSE;
	char	*limit = &new[PATH_MAX];

	if (fromlen == 0)
		return (NULL);
	tosimple = simpleto(to, tolen);

	string = from;
	slen = strlen(string);
	end = string;
	if (n < 0)
		n *= -1;
	while (n--) {

		/*
		 * Search the next occurence of the pattern in the 'from' string.
		 */
		while (*string != '\0') {
			if ((end = (char *)patmatch(pat[idx], aux[idx],
			    (Uchar *)string, 0, slen, alt[idx],
			    state)) == NULL) {

				string++;
				slen--;
				continue;
			}
			if (!didmatch) {
				didmatch = TRUE;
				strncpy((char *)new, (char *)from, PATH_MAX);
				new[PATH_MAX] = '\0';
				string = new + (string - from);
				end = new + (end - from);

				if (!tosimple) {
					/*
					 * We need to remember the old 'from' string before.
					 */
					oldlen = end - string;
					if (oldlen > PATH_MAX)
						oldlen = PATH_MAX;
					strncpy((char *)old, (char *)string, oldlen);
					old[oldlen] = '\0';
				}

			}
			break;
		}
		if (*string == '\0')
			break;

		/*
		 * Now delete the old string in the buffer
		 * and insert substitution
		 */
		if (tosimple) {
			strncpy((char *)xold, (char *)end, PATH_MAX);
			xold[PATH_MAX] = '\0';

			if ((string+tolen) >= limit)
				goto over;
			strncpy((char *)string, (char *)to, tolen);	/* insert */
			if ((&string[tolen] + strlen(xold)) >= limit)
				goto over;
			strcpy((char *)&string[tolen], (char *)xold);
			return (new);
over:
			errmsgno(EX_BAD, "Substitution path overflow.\n");
			new[0] = '\0';
			return (new);
		} else {
			tolen = catsub(string, old, oldlen, to, tolen, limit);
			if (tolen < 0) {
				new[0] = '\0';
				return (new);
			}
		}
		string = &string[tolen];
		slen = strlen(string);
	}
	if (didmatch)
		return (new);
	return (NULL);
}

/*
 * Check is this is a 'simple' 'to'-substitution string
 * that does not require to be expanded via 'catsub()'.
 */
LOCAL BOOL
simpleto(s, len)
	register char	*s;
	register long	len;
{
	register char	c;

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
LOCAL int
catsub(here, old, oldlen, to, tolen, limit)
	register char	*here;
	register char	*old;
	register long	oldlen;
	register char	*to;
	register long	tolen;
	register char	*limit;
{
	char	xold[PATH_MAX+1];
	char	*p = here;

	if (tolen <= 0)
		return (0);

	strncpy(xold, &here[oldlen], PATH_MAX);
	xold[PATH_MAX] = '\0';

	while (--tolen >= 0) {
		if (here >= limit)
			goto over;
		if (*to == '\\') {
			if (--tolen >= 0)
				*here++ = *++to;
		} else if (*to == '&') {
			if ((here+oldlen) >= limit)
				goto over;
			strncpy(here, old, oldlen);
			here += oldlen;
		} else {
			*here++ = *to;
		}
		to++;
	}
	if ((here+strlen(xold)) >= limit)
		goto over;
	strcpy(here, xold);
	return (here - p);
over:
	errmsgno(EX_BAD, "& Substitution path overflow.\n");
	return (-1);
}

/* ARGSUSED */
EXPORT BOOL
ia_change(ptb, info)
	TCB	*ptb;
	FINFO	*info;
{
	FINFO	cinfo;
	char	ans;
	int	len;

	if (paxinteract)
		return (pax_change(ptb, info));

	if (verbose)
		list_file(info);
	else
		vprint(info);
	if (nflag)
		return (FALSE);
	fprintf(vpr, "get/put ? Y(es)/N(o)/C(hange name) :"); fflush(vpr);
	fgetline(tty, new, 2);
	if ((ans = toupper(new[0])) == 'Y')
		return (TRUE);
	else if (ans == 'C') {
		for (;;) {
			fprintf(vpr, "Enter new name:");
			fflush(vpr);
			if ((len = fgetline(tty, new, sizeof (new))) == 0)
				continue;
			if (len > sizeof (new) - 1)
				errmsgno(EX_BAD, "Name too long.\n");
			else
				break;
		}
		info->f_name = new;
		if (xflag && newer(info, &cinfo))
			return (FALSE);
		return (TRUE);
	}
	return (FALSE);
}

/* ARGSUSED */
LOCAL BOOL
pax_change(ptb, info)
	TCB	*ptb;
	FINFO	*info;
{
	FINFO	cinfo;
	int	len;

	if (verbose)
		list_file(info);
	else
		vprint(info);
	if (nflag)
		return (FALSE);

	for (;;) {
		fprintf(vpr, "%s change?", info->f_name);
		fflush(vpr);
		if ((len = fgetline(tty, new, sizeof (new))) == 0)
			break;
		if (len > sizeof (new) - 1)
			errmsgno(EX_BAD, "Name too long.\n");
		else
			break;
	}
	if (new[0] == '\0')			/* Skip file */
		return (FALSE);
	if (new[0] == '.' && new[1] == '\0')	/* Leave name as is */
		return (TRUE);

	info->f_name = new;
	if (xflag && newer(info, &cinfo))
		return (FALSE);
	return (TRUE);
}
