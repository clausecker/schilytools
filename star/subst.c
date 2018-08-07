/* @(#)subst.c	1.23 18/08/07 Copyright 1986,2003-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)subst.c	1.23 18/08/07 Copyright 1986,2003-2018 J. Schilling";
#endif
/*
 *	Substitution commands
 *
 *	Copyright (c) 1986,2003-2018 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/patmatch.h>
#include <schily/string.h>
#include <schily/utypes.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>

#include <schily/patmatch.h>

#include "star.h"
#include "starsubs.h"
#include "pathname.h"

EXPORT	int	parsesubst	__PR((char *cmd, BOOL *arg));
EXPORT	BOOL	subst		__PR((FINFO *info));
LOCAL	char	*substitute	__PR((char *from, long fromlen, int idx, char *to, long tolen));
LOCAL	BOOL	simpleto	__PR((char *s, long len));
LOCAL	int	catsub		__PR((char *here, char *old, long oldlen, char *to, long tolen, char *limit));
EXPORT	BOOL	ia_change	__PR((TCB *ptb, FINFO *info));
LOCAL	BOOL	pax_change	__PR((TCB *ptb, FINFO *info));
LOCAL	void	s_enomem	__PR((void));
EXPORT	int	fpgetstr	__PR((FILE *, pathstore_t *));

#define	NPAT	100
LOCAL	int	npat;		/* Number of defined patterns */
LOCAL	Uchar	*pat[NPAT];	/* Saved list of defined 'from' patterns */
LOCAL	int	patlen[NPAT];	/* Length of the 'from' pattern */
LOCAL	int	maxplen;	/* Maximum length of 'from' pattern */
LOCAL	char	*substpat[NPAT]; /* Saved list of defined 'to' patterns */
LOCAL	int	substlen[NPAT];	/* Length of the 'to' pattern */
LOCAL	int	*aux[NPAT];	/* Aux array (compiled pattern) */
LOCAL	int	alt[NPAT];	/* List of results from patcompile() */
LOCAL	int	*state;		/* State array used by patmatch() */
LOCAL	Int32_t	substcnt[NPAT];	/* Subst. count or MAXINT32 for 'g', < 0: 'v' */

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
	register char	dc;		/* Delimiting character */
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
	if (to >= endp || c != dc)
		comerrno(EX_BAD, "Missing '%c' delimiter after 'from' substitute string.\n", dc);

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
	if (to >= endp || c != dc)
		comerrno(EX_BAD, "Missing '%c' delimiter after 'to' substitute string.\n", dc);

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

	pat[npat] = (Uchar *)___savestr(from);
	patlen[npat] = fromlen;
	substpat[npat] = ___savestr(to);
	substlen[npat] = tolen;


	if (fromlen > maxplen)
		maxplen = fromlen;

	aux[npat] = ___malloc(fromlen*sizeof (int), "compiled subst pattern");
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
		state = ___malloc((maxplen+1)*sizeof (int), "pattern state");
	}

	info->f_namelen = strlen(info->f_name);
	/*
	 * Loop over all match & Subst Patterns.
	 * Stop after the first match has been seen.
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


LOCAL	pathstore_t	new;
/*
 * This is the 'real' substitution routine.
 * It gets called with pre-parsed strings.
 *
 * Returns NULL on no-match and on error.
 */
LOCAL char *
substitute(from, fromlen, idx, to, tolen)
	char	*from;			/* The original string to modify */
	long	fromlen;		/* strlen(from)			*/
	int	idx;			/* The index in the pat[] array	*/
	char	*to;			/* The substitution		*/
	long	tolen;			/* strlen(to)			*/
{
	char	old[PATH_MAX+1];
	char	*oldp = old;
	long	oldlen = 0;
	BOOL	tosimple;
	Int32_t n = substcnt[idx];
	char	*end;
	char	*string;
	size_t	soff;
	int	slen;
	BOOL	didmatch = FALSE;
#define	limit	(new.ps_path + new.ps_size)

	if (fromlen == 0)
		return (NULL);
	if (new.ps_size == 0 && init_pspace(PS_EXIT, &new) < 0)
		return (NULL);

	tosimple = simpleto(to, tolen);

	string = from;
	slen = strlen(string);
	end = string;
	/*
	 * We simply ignore the 'p'rint statement here as the printing happens
	 * in the subst() function.
	 */
	if (n < 0)
		n *= -1;
	while (n-- > 0) {

		/*
		 * Search the next occurence of the pattern in the 'from' string.
		 */
		while (*string != '\0') {
			/*
			 * Loop over the from string for a possible match
			 */
			if ((end = (char *)patmatch(pat[idx], aux[idx],
			    (Uchar *)string, 0, slen, alt[idx],
			    state)) == NULL) {

				string++;
				slen--;
				continue;
			}
			if (!didmatch) {
				/*
				 * We had a first match. Copy the 'from' string
				 * into our result storage.
				 */
				didmatch = TRUE;
				strcpy_pspace(PS_EXIT, &new, from);

				/*
				 * Let 'string' and 'end' have the same offset
				 * in 'new' as they had in 'from' before.
				 */
				string = new.ps_path + (string - from);
				end = new.ps_path + (end - from);

				if (!tosimple) {
					/*
					 * We need to remember the old 'from'
					 * string before, since the replacement
					 * refers to the old 'from' string.
					 */
					oldlen = end - string;
					if (strlcpy(old, string, oldlen+1) >=
									oldlen) {
						oldp = strndup(string, oldlen);
						if (oldp == NULL) {
							s_enomem();
							return (NULL);
						}
					} else {
						oldp = old;
					}
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
			char	xold[PATH_MAX+1];
			char	*xoldp;

			/*
			 * Remember the old string after the matching part.
			 */
			if (strlcpy(xold, end, sizeof (xold)) >=
							sizeof (xold)) {
				xoldp = strdup(end);
				if (xoldp == NULL) {
					s_enomem();
					return (NULL);
				}
			} else {
				xoldp = xold;
			}


			if ((string+tolen) >= limit) {
				soff = string - new.ps_path;
				if (incr_pspace(PS_STDERR, &new,
					    1 + (string+tolen) - limit) < 0) {
					s_enomem();
					if (xoldp != xold)
						free(xoldp);
					goto over;
				}
				string = new.ps_path + soff;
			}
			strlcpy((char *)string, (char *)to, tolen+1);	/* insert */

			/*
			 * Append non-maching old tail.
			 */
			if ((&string[tolen] + strlen(xoldp)) >= limit) {
				soff = string - new.ps_path;
				if (incr_pspace(PS_STDERR, &new,
					    1 + (string+tolen) - limit) < 0) {
					s_enomem();
					if (xoldp != xold)
						free(xoldp);
					goto over;
				}
				string = new.ps_path + soff;
			}
			strcpy((char *)&string[tolen], xoldp);
			if (xoldp != xold)
				free(xoldp);
		} else {
			soff = string - new.ps_path;
			tolen = catsub(string, old, oldlen, to, tolen, limit);
			string = new.ps_path + soff;
			if (oldp != old)
				free(oldp);
			if (tolen < 0) {
				if (new.ps_path)
					new.ps_path[0] = '\0';
				return (new.ps_path);
			}
		}
		string = &string[tolen];
		slen = strlen(string);
	}
	if (didmatch)
		return (new.ps_path);
	return (NULL);
over:
	errmsgno(EX_BAD, "Substitution path overflow.\n");
	if (new.ps_path)
		new.ps_path[0] = '\0';
	return (new.ps_path);
}
#undef	limit

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
	char	*xoldp;
	char	*p = here;
	size_t	len;
	size_t	hoff;

	if (tolen <= 0)
		return (0);

	/*
	 * Remember the old string after the matching part.
	 */
	if (strlcpy(xold, &here[oldlen], sizeof (xold)) >= sizeof (xold)) {
		xoldp = strdup(&here[oldlen]);
		if (xoldp == NULL) {
			s_enomem();
			return (-1);
		}
	} else {
		xoldp = xold;
	}

	while (--tolen >= 0) {
		if (here >= limit) {
			hoff = here - new.ps_path;
			if (incr_pspace(PS_STDERR, &new,
				    1 + here - limit) < 0) {
				s_enomem();
				goto over;
			}
			here = new.ps_path + hoff;
		}
		if (*to == '\\') {
			if (--tolen >= 0)
				*here++ = *++to;
		} else if (*to == '&') {
			if ((here+oldlen) >= limit) {
				hoff = here - new.ps_path;
				if (incr_pspace(PS_STDERR, &new,
					    1 + (here+oldlen) - limit) < 0) {
					s_enomem();
					goto over;
				}
				here = new.ps_path + hoff;
			}
			strlcpy(here, old, oldlen+1);
			here += oldlen;
		} else {
			*here++ = *to;
		}
		to++;
	}
	len = strlen(xoldp);
	if ((here+len) >= limit) {
		hoff = here - new.ps_path;
		if (incr_pspace(PS_STDERR, &new,
			    1 + (here+len) - limit) < 0) {
			s_enomem();
			goto over;
		}
		here = new.ps_path + hoff;
	}
	strcpy(here, xoldp);
	if (xoldp != xold)
		free(xoldp);
	return (here - p);
over:
	errmsgno(EX_BAD, "& Substitution path overflow.\n");
	if (xoldp != xold)
		free(xoldp);
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
	char	abuf[3];
	int	len;

	if (paxinteract)
		return (pax_change(ptb, info));

	if (verbose)
		list_file(info);
	else
		vprint(info);
	if (nflag)
		return (FALSE);
	fgtprintf(vpr, "get/put ? Y(es)/N(o)/C(hange name) :"); fflush(vpr);
	abuf[0] = '\0';
	len = fgetstr(tty, abuf, sizeof (abuf));
	if (len > 0 && abuf[len-1] != '\n') {
		while(getc(tty) != '\n') {
			if (feof(tty) || ferror(tty))
				break;
		}
	}
	if ((ans = toupper(abuf[0])) == 'Y')
		return (TRUE);
	else if (ans == 'C') {
		for (;;) {
			fgtprintf(vpr, "Enter new name:");
			fflush(vpr);
			len = fpgetstr(tty, &new);
			if (len < 0)
				comexit(-2);
			else if (len > 0)
				break;
		}
		info->f_name = new.ps_path;
		if (xflag) {
			if (newer(info, &cinfo))
				return (FALSE);
			if (is_symlink(info) && same_symlink(info))
				return (FALSE);
		}
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
		fgtprintf(vpr, "%s change?", info->f_name);
		fflush(vpr);
		len = fpgetstr(tty, &new);
		if (len < 0)
			comexit(-2);
		else
			break;
	}
	if (new.ps_path[0] == '\0')		/* Skip file */
		return (FALSE);
	if (new.ps_path[0] == '.' &&
	    new.ps_path[1] == '\0')		/* Leave name as is */
		return (TRUE);

	info->f_name = new.ps_path;
	if (xflag && newer(info, &cinfo))
		return (FALSE);
	return (TRUE);
}

LOCAL void
s_enomem()
{
	errmsgno(EX_BAD, "No memory for substitution.\n");
	xstats.s_substerrs++;
}

/*
 * Read a line of unspecified and arbitrary length from FILE *
 * and place the result in a pathstore_t object.
 */
EXPORT int
fpgetstr(f, p)
	register	FILE	*f;
		pathstore_t	*p;
{
	int	ret = getdelim(&p->ps_path, &p->ps_size, '\n', f);

	if (ret <= 0)
		return (ret);

	if (p->ps_path[ret-1] == '\n')
		p->ps_path[--ret] = '\0';
	return (ret);
}
