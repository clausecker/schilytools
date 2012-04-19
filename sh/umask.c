/* @(#)umask.c	1.1 12/04/15 Copyright 2004-2012 J. Schilling */
#include <schily/mconfig.h>
#ifndef RES
static	UConst char sccsid[] =
	"@(#)umask.c	1.1 12/04/15 Copyright 2004-2012 J. Schilling";
/*
 *	Parser for chmod(1)/find(1)-perm, ....
 *	The code has been taken from libschily/getperm.c and adopted to
 *	avoid stdio for the Bourne Shell.
 *
 *	Copyright (c) 2004-2012 J. Schilling
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

#include "defs.h"

#define	LOCAL	static

/*
 * getperm() flags:
 */
#define	GP_NOX		0	/* This is not a dir and 'X' is not valid */
#define	GP_DOX		1	/* 'X' perm character is valid		  */
#define	GP_XERR		2	/* 'X' perm characters are invalid	  */
#define	GP_FPERM	4	/* TRUE if we implement find -perm	  */

LOCAL	int	getperm		__PR((char *perm, mode_t *modep, int smode, int flag));
LOCAL	char	*getsperm	__PR((char *p, mode_t *mp, int smode, int isX));
LOCAL	mode_t	iswho		__PR((int c));
LOCAL	int	isop		__PR((int c));
LOCAL	mode_t	isperm		__PR((int c, int isX));
LOCAL	int	mval		__PR((mode_t m));
LOCAL	void	promask		__PR((void));
	void	sysumask	__PR((int argc, char **argv));

/*
 * This is the mode bit translation code stolen from star...
 */
#define	TSUID		04000	/* Set UID on execution */
#define	TSGID		02000	/* Set GID on execution */
#define	TSVTX		01000	/* On directories, restricted deletion flag */
#define	TUREAD		00400	/* Read by owner */
#define	TUWRITE		00200	/* Write by owner special */
#define	TUEXEC		00100	/* Execute/search by owner */
#define	TGREAD		00040	/* Read by group */
#define	TGWRITE		00020	/* Write by group */
#define	TGEXEC		00010	/* Execute/search by group */
#define	TOREAD		00004	/* Read by other */
#define	TOWRITE		00002	/* Write by other */
#define	TOEXEC		00001	/* Execute/search by other */

#define	TALLMODES	07777	/* The low 12 bits mentioned in the standard */

#define	S_ALLPERM	(S_IRWXU|S_IRWXG|S_IRWXO)
#define	S_ALLFLAGS	(S_ISUID|S_ISGID|S_ISVTX)
#define	S_ALLMODES	(S_ALLFLAGS | S_ALLPERM)

#if	S_ISUID == TSUID && S_ISGID == TSGID && S_ISVTX == TSVTX && \
	S_IRUSR == TUREAD && S_IWUSR == TUWRITE && S_IXUSR == TUEXEC && \
	S_IRGRP == TGREAD && S_IWGRP == TGWRITE && S_IXGRP == TGEXEC && \
	S_IROTH == TOREAD && S_IWOTH == TOWRITE && S_IXOTH == TOEXEC

#define	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#endif

#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#define	OSMODE(xmode)	    (xmode)
#define	TARMODE(xmode)	    (xmode)
#else
#define	OSMODE(xmode)	    ((xmode  & TSUID   ? S_ISUID : 0) \
			    | (xmode & TSGID   ? S_ISGID : 0) \
			    | (xmode & TSVTX   ? S_ISVTX : 0) \
			    | (xmode & TUREAD  ? S_IRUSR : 0) \
			    | (xmode & TUWRITE ? S_IWUSR : 0) \
			    | (xmode & TUEXEC  ? S_IXUSR : 0) \
			    | (xmode & TGREAD  ? S_IRGRP : 0) \
			    | (xmode & TGWRITE ? S_IWGRP : 0) \
			    | (xmode & TGEXEC  ? S_IXGRP : 0) \
			    | (xmode & TOREAD  ? S_IROTH : 0) \
			    | (xmode & TOWRITE ? S_IWOTH : 0) \
			    | (xmode & TOEXEC  ? S_IXOTH : 0))

#define	TARMODE(xmode)	    ((xmode  & S_ISUID ? TSUID   : 0) \
			    | (xmode & S_ISGID ? TSGID   : 0) \
			    | (xmode & S_ISVTX ? TSVTX   : 0) \
			    | (xmode & S_IRUSR ? TUREAD  : 0) \
			    | (xmode & S_IWUSR ? TUWRITE : 0) \
			    | (xmode & S_IXUSR ? TUEXEC  : 0) \
			    | (xmode & S_IRGRP ? TGREAD  : 0) \
			    | (xmode & S_IWGRP ? TGWRITE : 0) \
			    | (xmode & S_IXGRP ? TGEXEC  : 0) \
			    | (xmode & S_IROTH ? TOREAD  : 0) \
			    | (xmode & S_IWOTH ? TOWRITE : 0) \
			    | (xmode & S_IXOTH ? TOEXEC  : 0))
#endif

LOCAL int
getperm(perm, modep, smode, flag)
	char	*perm;			/* Perm string to parse		    */
	mode_t	*modep;			/* The mode result		    */
	int	smode;			/* The start mode for the computation */
	int	flag;			/* Flags, see getperm() flag defs */
{
	char	*p;
#ifdef	DO_OCTAL
	Llong	ll;
	mode_t	mm;
#endif

	p = perm;
	if ((flag & GP_FPERM) && *p == '-')
		p++;

#ifdef	DO_OCTAL
	if (*p >= '0' && *p <= '7') {
		p = astollb(p, &ll, 8);
		if (*p) {
			return (-1);
		}
		mm = ll & TALLMODES;
		*modep = OSMODE(mm);
		return (0);
	}
#endif
	flag &= ~GP_FPERM;
	if (flag & GP_XERR)
		flag = -1;
	p = getsperm(p, modep, smode, flag);
	if (p && *p != '\0') {
		return (-1);
	}
	return (0);
}

LOCAL char *
getsperm(p, mp, smode, isX)
	char	*p;		/* The perm input string		*/
	mode_t	*mp;		/* To set the mode			*/
	int	smode;		/* The start mode for the computation	*/
	int	isX;		/* -1: Ignore X, 0: no dir/X 1: X OK	*/
{
	mode_t	permval = smode;	/* POSIX start value for "find" */
	mode_t	who;
	mode_t	m;
	int	op;
	mode_t	perms;
	mode_t	pm;

nextclause:
	who = 0;
	while ((m = iswho(*p)) != 0) {
		p++;
		who |= m;
	}
	if (who == 0) {
#ifndef	BOURNE_SHELL
		mode_t	mask = umask(0);

		umask(mask);
		who = ~mask;
		who &= (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
#else
		who = (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
#endif
	}

nextop:
	if ((op = isop(*p)) != '\0')
		p++;
	if (op == 0) {
		return (p);
	}

	perms = 0;
	while ((pm = isperm(*p, isX)) != (mode_t)-1) {
		p++;
		perms |= pm;
	}
	if ((perms == 0) && (*p == 'u' || *p == 'g' || *p == 'o')) {
		mode_t	what = 0;

		/*
		 * First select the bit triple we like to copy.
		 */
		switch (*p) {

		case 'u':
			what = permval & S_IRWXU;
			break;
		case 'g':
			what = permval & S_IRWXG;
			break;
		case 'o':
			what = permval & S_IRWXO;
			break;
		}
		/*
		 * Now copy over bit by bit without relying on shifts
		 * that would make implicit assumptions on values.
		 */
		if (what & (S_IRUSR|S_IRGRP|S_IROTH))
			perms |= (who & (S_IRUSR|S_IRGRP|S_IROTH));
		if (what & (S_IWUSR|S_IWGRP|S_IWOTH))
			perms |= (who & (S_IWUSR|S_IWGRP|S_IWOTH));
		if (what & (S_IXUSR|S_IXGRP|S_IXOTH))
			perms |= (who & (S_IXUSR|S_IXGRP|S_IXOTH));
		p++;
	}
	switch (op) {

	case '=':
		permval &= ~who;
		/* FALLTHRU */
	case '+':
		permval |= (who & perms);
		break;

	case '-':
		permval &= ~(who & perms);
		break;
	}
	if (isop(*p))
		goto nextop;
	if (*p == ',') {
		p++;
		goto nextclause;
	}
	*mp = permval;
	return (p);
}

LOCAL mode_t
iswho(c)
	int	c;
{
	switch (c) {

	case 'u':					/* user */
		return (S_ISUID|S_ISVTX|S_IRWXU);
	case 'g':					/* group */
		return (S_ISGID|S_ISVTX|S_IRWXG);
	case 'o':					/* other */
		return (S_ISVTX|S_IRWXO);
	case 'a':					/* all */
		return (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
	default:
		return (0);
	}
}

LOCAL int
isop(c)
	int	c;		/* The current perm character to parse	*/
{
	switch (c) {

	case '+':
	case '-':
	case '=':
		return (c);
	default:
		return (0);
	}
}

LOCAL mode_t
isperm(c, isX)
	int	c;		/* The current perm character to parse	*/
	int	isX;		/* -1: Ignore X, 0: no dir/X 1: X OK	*/
{
	switch (c) {

	case 'r':
		return (S_IRUSR|S_IRGRP|S_IROTH);
	case 'w':
		return (S_IWUSR|S_IWGRP|S_IWOTH);
	case 'X':
		if (isX < 0)
			return ((mode_t)-1);	/* Singnal parse error	*/
		if (isX == 0)
			return ((mode_t)0);	/* No 'X' handling here	*/
		/* FALLTHRU */
	case 'x':
		return (S_IXUSR|S_IXGRP|S_IXOTH);
	case 's':
		return (S_ISUID|S_ISGID);
	case 'l':
		return (S_ISGID);
	case 't':
		return (S_ISVTX);
	default:
		return ((mode_t)-1);
	}
}

static char *umodtab[] =
{"", "x", "w", "wx", "r", "rx", "rw", "rwx"};

#ifdef	PROTOTYPES
LOCAL int
mval(mode_t m)
#else
LOCAL int
mval(m)
	mode_t	m;
#endif
{
	int	ret = 0;

	if (m & (S_IRUSR|S_IRGRP|S_IROTH))
		ret |= 4;
	if (m & (S_IWUSR|S_IWGRP|S_IWOTH))
		ret |= 2;
	if (m & (S_IXUSR|S_IXGRP|S_IXOTH))
		ret |= 1;

	return (ret);
}

LOCAL void
promask()
{
	mode_t	i;
	int	j;

	umask(i = umask(0));
	i = TARMODE(i);
	prc_buff('0');
	for (j = 6; j >= 0; j -= 3)
		prc_buff(((i >> j) & 07) +'0');
	prc_buff(NL);
}

void
sysumask(argc, argv)
	int	argc;
	char	**argv;
{
	char	*a1 = argv[1];

	if (argc == 2 && eq(a1, "-S")) {
		mode_t	m;

		umask(m = umask(0));
		m = ~m;

		prs_buff((unsigned char *)"u=");
		prs_buff((unsigned char *)umodtab[mval(m & S_IRWXU)]);
		prs_buff((unsigned char *)",g=");
		prs_buff((unsigned char *)umodtab[mval(m & S_IRWXG)]);
		prs_buff((unsigned char *)",o=");
		prs_buff((unsigned char *)umodtab[mval(m & S_IRWXO)]);
		prc_buff(NL);
	} else if (a1) {
		int	c;
		mode_t	i;

		/*
		 * POSIX requires "umask -r" to fail.
		 * Must use "umask -- -r" to set this mode.
		 */
		if (a1[0] == '-') {
			if (a1[1] == '-' && a1[2] == '\0') {	/* found -- */
				if (argc <= 2) {		/* umask -- */
					promask();
					return;
				}
				argv++;
				a1 = argv[1];
			} else {
				/*
				 * This is a "bad option".
				 */
				failed((unsigned char *)&badumask[4], badumask);
			}
		}
		i = 0;
		while ((c = *a1++) >= '0' && c <= '7')
			i = (i << 3) + c - '0';

		if (c == '\0') {	/* Fully octal arg */
			umask(OSMODE(i));
			return;
		}

		umask(i = umask(0));
		i = ~i;
		if (getperm(argv[1], &i, i, GP_NOX)) {
			failed((unsigned char *)&badumask[4], badumask);
		}
		umask(~i);
	} else {
		promask();
	}
}

#endif /* RES */
