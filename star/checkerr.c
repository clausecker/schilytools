/* @(#)checkerr.c	1.28 19/02/28 Copyright 2003-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)checkerr.c	1.28 19/02/28 Copyright 2003-2019 J. Schilling";
#endif
/*
 *	Error control for star.
 *
 *	Copyright (c) 2003-2019 J. Schilling
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

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/patmatch.h>
#include <schily/string.h>
#include <schily/utypes.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>

#include "starsubs.h"
#include "checkerr.h"

typedef struct errconf {
	struct errconf	*ec_next;	/* Next in list			    */
	const	Uchar	*ec_pat;	/* File name pattern		    */
	int		*ec_aux;	/* Aux array from pattern compiler  */
	int		ec_alt;		/* Alt return from pattern compiler */
	int		ec_plen;	/* String length of pattern	    */
	UInt32_t	ec_flags[E_NFL]; /* Error condition flags	    */
} ec_t;

LOCAL	int	*ec_state;		/* State array for pattern compiler */
LOCAL	ec_t	*ec_root;		/* Root node of error config list   */
LOCAL	ec_t	**ec_last = &ec_root;	/* Last pointer in root node list   */
LOCAL	int	maxplen;

EXPORT	int	errconfig	__PR((char *name));
LOCAL	char	*_endword	__PR((char *p));
LOCAL	void	parse_errctl	__PR((char *line));
LOCAL	UInt32_t errflags	__PR((char *eflag, BOOL doexit, UInt32_t fla[E_NFL]));
LOCAL	ec_t	*_errptr	__PR((int etype, const char *fname));
EXPORT	BOOL	errhidden	__PR((int etype, const char *fname));
EXPORT	BOOL	errwarnonly	__PR((int etype, const char *fname));
EXPORT	BOOL	errabort	__PR((int etype, const char *fname, BOOL doexit));

/*
 * Read and parse error configuration file
 */
EXPORT int
errconfig(name)
	char	*name;
{
	char	line[8192];
	FILE	*f;
	int	omaxplen = maxplen;

	if ((f = fileopen(name, "r")) == NULL) {
		if (errflags(name, FALSE, NULL) != 0)
			parse_errctl(name);
		else
			comerr("Cannot open '%s'.\n", name);
	} else {
		while (fgetline(f, line, sizeof (line)) >= 0) {
			parse_errctl(line);
		}
		fclose(f);
	}
	if (maxplen > omaxplen) {
		ec_state = ___realloc(ec_state, (maxplen+1)*sizeof (int),
							"pattern state");
	}
	return (1);
}

LOCAL char *
_endword(p)
	char	*p;
{
	/*
	 * Find end of word.
	 */
	for (;  *p != '\0' &&
		*p != '\t' &&
		*p != ' ';
				p++) {
		;
		/* LINTED */
	}
	return (p);
}

LOCAL void
parse_errctl(line)
	char	*line;
{
	int	plen;
	char	*pattern;
	ec_t	*ep;

	/*
	 * Find end of word.
	 */
	pattern = _endword(line);

	if (pattern == line || *pattern == '\0') {
		comerrno(EX_BAD,
		"Bad error configuration line '%s'.\n", line);
	}
	/*
	 * Find end of white space after word.
	 */
	for (pattern++; *pattern != '\0' &&
	    (*pattern == '\t' || *pattern == ' ');
				pattern++) {
		;
		/* LINTED */
	}
	ep = ___malloc(sizeof (ec_t), "errcheck node");
	errflags(line, TRUE, ep->ec_flags);
	ep->ec_plen = plen = strlen(pattern);
	if (ep->ec_plen > maxplen)
		maxplen = ep->ec_plen;
	ep->ec_pat = (const Uchar *)___savestr(pattern);
	ep->ec_aux = ___malloc(plen*sizeof (int), "compiled pattern");
	if ((ep->ec_alt = patcompile((const Uchar *)pattern,
						plen, ep->ec_aux)) == 0)
		comerrno(EX_BAD, "Bad errctl pattern: '%s'.\n", pattern);

	ep->ec_next = NULL;
	*ec_last = ep;
	ec_last = &ep->ec_next;
}

LOCAL struct eflags {
	char	*fname;
	UInt32_t fval;
} eflags[] = {
	{ "STAT",		E_STAT },
	{ "GETACL",		E_GETACL },
	{ "OPEN",		E_OPEN },
	{ "READ",		E_READ },
	{ "WRITE",		E_WRITE },
	{ "GROW",		E_GROW },
	{ "SHRINK",		E_SHRINK },
	{ "MISSLINK",		E_MISSLINK },
	{ "NAMETOOLONG",	E_NAMETOOLONG },
	{ "FILETOOBIG",		E_FILETOOBIG },
	{ "SPECIALFILE",	E_SPECIALFILE },
	{ "READLINK",		E_READLINK },
	{ "GETXATTR",		E_GETXATTR },
	{ "CHDIR",		E_CHDIR },
	{ "ICONV",		E_ICONV },
	{ "ID",			E_ID },
	{ "TIME",		E_TIME },

	{ "SETTIME",		E_SETTIME },
	{ "SETMODE",		E_SETMODE },
	{ "SECURITY",		E_SECURITY },
	{ "LSECURITY",		E_LSECURITY },
	{ "SAMEFILE",		E_SAMEFILE },
	{ "BADACL",		E_BADACL },
	{ "SETACL",		E_SETACL },
	{ "SETXATTR",		E_SETXATTR },

	{ "ALL",		E_ALL },

	{ "DIFF",		E_DIFF },
	{ "WARN",		E_WARN },
	{ "ABORT",		E_ABORT },

	{ NULL,			0 }
};

/*
 * Convert error condition string into flag word
 */
LOCAL UInt32_t
errflags(eflag, doexit, fla)
	char	*eflag;
	BOOL	doexit;
	UInt32_t fla[E_NFL];
{
	register char		*p = eflag;
		char		*ef = _endword(eflag);
	register struct eflags	*ep;
	register int		slen;
	register UInt32_t	nflags = 0;

	do {
		for (ep = eflags; ep->fname; ep++) {
			slen = strlen(ep->fname);
			if ((strncmp(ep->fname, p, slen) == 0) &&
			    (p[slen] == '|' || p[slen] == ' ' ||
			    p[slen] == '\0')) {
				UInt32_t nfl = ep->fval;

				nflags |= nfl;
				if (fla) {
					int	idx = (nfl & E_EMASK) >> E_SHIFT;

					fla[idx] |= nfl;
				}
				break;
			}
		}
		if (ep->fname == NULL) {
			if (doexit)
				comerrno(EX_BAD, "Bad flag '%s'\n", p);
			return (0);
		}
		p = strchr(p, '|');
	} while (p < ef && p && *p++ == '|');

	if ((nflags & ~(UInt32_t)(E_ABORT|E_WARN)) == 0) {
		if (doexit)
			comerrno(EX_BAD, "Bad error condition '%s'.\n", eflag);
		return (0);
	}
	return (nflags);
}

LOCAL ec_t *
_errptr(etype, fname)
		int	etype;
	const	char	*fname;
{
	ec_t		*ep = ec_root;
	char		*ret;
	const Uchar	*name = (const Uchar *)fname;
	int		nlen;
	int		idx = (etype & E_EMASK) >> E_SHIFT;

	if (fname == NULL) {
		errmsgno(EX_BAD,
			"Implementation botch for errhidden(0x%X, NULL)\n",
			etype);
		errmsgno(EX_BAD, "Please report this bug!\n");
		errmsgno(EX_BAD, "Error cannot be ignored.\n");
		return ((ec_t *)NULL);
	}
	nlen  = strlen(fname);
	etype &= ~E_EMASK;
	while (ep) {
		if ((ep->ec_flags[idx] & etype) != 0) {
			ret = (char *)patmatch(ep->ec_pat, ep->ec_aux,
					name, 0,
					nlen, ep->ec_alt, ec_state);
			if (ret != NULL && *ret == '\0')
				return (ep);
		}
		ep = ep->ec_next;
	}
	return ((ec_t *)NULL);
}

/*
 * Check whether error condition should be ignored for file name.
 */
EXPORT BOOL
errhidden(etype, fname)
		int	etype;
	const	char	*fname;
{
	ec_t		*ep;

	if ((ep = _errptr(etype, fname)) != NULL) {
		if ((ep->ec_flags[0] & (E_ABORT|E_WARN)) != 0)
			return (FALSE);
		return (TRUE);
	}
	return (FALSE);
}

/*
 * Check whether error condition should not affect exit code for file name.
 */
EXPORT BOOL
errwarnonly(etype, fname)
		int	etype;
	const	char	*fname;
{
	ec_t		*ep;

	if ((ep = _errptr(etype, fname)) != NULL) {
		if ((ep->ec_flags[0] & E_WARN) != 0)
			return (TRUE);
		return (FALSE);
	}
	return (FALSE);
}

/*
 * Check whether error condition should be fatal for file name.
 */
EXPORT BOOL
errabort(etype, fname, doexit)
		int	etype;
	const	char	*fname;
		BOOL	doexit;
{
	ec_t	*ep;
extern	BOOL	cflag;
extern	BOOL	errflag;
extern	int	intr;

	if ((ep = _errptr(etype, fname)) == NULL) {
		if (!errflag)
			return (FALSE);
	} else if ((ep->ec_flags[0] & E_ABORT) == 0)
		return (FALSE);

	if (doexit) {
		errmsgno(EX_BAD, "Error is considered fatal, aborting.\n");
		if (cflag) {
			intr++;
		} else {
			prstats();
			comexit(-3);
		}
	}
	return (TRUE);
}
