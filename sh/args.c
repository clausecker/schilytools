/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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

/*
 * Copyright 1995 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)args.c	1.11	05/09/14 SMI"
#endif

#include "defs.h"
#ifdef	DO_SYSALIAS
#include "abbrev.h"
#endif
#include "version.h"

/*
 * Copyright 2008-2015 J. Schilling
 *
 * @(#)args.c	1.48 15/08/01 2008-2015 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)args.c	1.48 15/08/01 2008-2015 J. Schilling";
#endif

/*
 *	UNIX shell
 */

#include	"sh_policy.h"

static	void		prversion	__PR((void));
	int		options		__PR((int argc, unsigned char **argv));
	void		setopts		__PR((void));
	void		setargs		__PR((unsigned char *argi[]));
static void		freedolh	__PR((void));
	struct dolnod	*freeargs	__PR((struct dolnod *blk));
static struct dolnod	*copyargs	__PR((unsigned char *[], int));
static	struct dolnod	*clean_args	__PR((struct dolnod *blk));
	void		clearup		__PR((void));
	struct dolnod	*savargs	__PR((int funcntp));
	void		restorargs	__PR((struct dolnod *olddolh,
							int funcntp));
	struct dolnod	*useargs	__PR((void));
#if defined(DO_SET_O) && defined(DO_SYSALIAS)
static	void		listaliasowner	__PR((int parse, int flagidx));
#endif
#ifdef	DO_SET_O
static	void		listopts	__PR((int parse));
#endif

static struct dolnod *dolh;

/* Used to save outermost positional parameters */
static struct dolnod *globdolh;
static unsigned char **globdolv;
static int globdolc;

unsigned char	flagadr[20];

unsigned char	flagchar[] =
{
	'x',
	'n',
	'v',
	't',
	STDFLG,
	'i',
	'e',
	'r',
	'k',
	'u',
	'h',
	'f',
	'a',
	'm',
	'p',
	'P',
	'V',
#ifdef	DO_FDPIPE
	0,			/* set -o fdpipe */
#endif
#ifdef	DO_TIME
	0,			/* set -o time */
#endif
#ifdef	DO_FULLEXCODE
	0,			/* set -o fullexcode */
#endif
	0,			/* set -o globalaliases */
	0,			/* set -o localaliases */
	0,			/* set -o aliasowner */
#ifdef	INTERACTIVE
	0,			/* set -o vi */
	0,			/* set -o ved */
#endif
	0
};

#ifdef	DO_SET_O
char	*flagname[] =
{
	"xtrace",		/* -x POSIX */
	"noexec",		/* -n POSIX */
	"verbose",		/* -v POSIX */
	"onecmd",		/* -t bash name */
	"stdin",		/* -s Schily name */
	"interactive",		/* -i ksh93 name */
	"errexit",		/* -e POSIX */
	"restricted",		/* -r ksh93 name */
	"keyword",		/* -k bash/ksh93 name */
	"nounset",		/* -u POSIX */
	"hashall",		/* -h bash name (ksh93 uses "trackall") */
	"noglob",		/* -f POSIX */
	"allexport",		/* -a POSIX */
	"monitor",		/* -m POSIX */
	"privileged",		/* -p ksh93: only if really privileged */
	"pfsh",			/* -P Schily Bourne Shell */
	"version",		/* -V Schily Bourne Shell */
#ifdef	DO_FDPIPE
	"fdpipe",		/* e.g. 2| for pipe from stderr */
#endif
#ifdef	DO_TIME
	"time",			/* -o time, enable timing */
#endif
#ifdef	DO_FULLEXCODE
	"fullexitcode",		/* -o fullexitcode, so not mask $? */
#endif
	"globalaliases",
	"localaliases",
	"aliasowner",
#ifdef	INTERACTIVE
	"vi",
	"ved",
#endif
	0
};
#endif

unsigned long	flagval[]  =
{
	execpr,
	noexec,
	readpr,
	oneflg,
	stdflg,
	intflg,
	errflg,
	rshflg,
	keyflg,
	setflg,
	hashflg,
	nofngflg,
	exportflg,
	monitorflg,
	privflg,
	pfshflg,
	versflg,
#ifdef	DO_FDPIPE
	fl2 | fdpipeflg,
#endif
#ifdef	DO_TIME
	fl2 | timeflg,
#endif
#ifdef	DO_FULLEXCODE
	fl2 | fullexitcodeflg,
#endif
	globalaliasflg,
	localaliasflg,
	aliasownerflg,
#ifdef	INTERACTIVE
	viflg,
	vedflg,
#endif
	0
};

unsigned char *shvers;

/* ========	option handling	======== */

static void
prversion()
{
	char	vbuf[BUFFERSIZE];

	snprintf(vbuf, sizeof (vbuf),
	    "%s %s\n",
	    shname, shvers);
	prs(UC vbuf);
	if (dolv == NULL) {
		/*
		 * We have been called as a result of a sh command line flag.
		 * Print the version information and exit.
		 */
		prs(UC "\n");
		prs(UC "Copyright (C) 1984-1989 AT&T\n");
		prs(UC "Copyright (C) 1989-2009 Sun Microsystems\n");
#ifdef	INTERACTIVE
		prs(UC "Copyright (C) 1982-2015 Joerg Schilling\n");
#else
		prs(UC "Copyright (C) 1998-2015 Joerg Schilling\n");
#endif
		exitsh(0);
	}
}

int
options(argc, argv)
	int		argc;
	unsigned char **argv;

{
	unsigned char *cp;
	unsigned char **argp = argv;
	unsigned char *flagc;
	int		len;
	wchar_t		wc;
	unsigned long	fv;

	if (shvers == NULL) {
		char	vbuf[BUFFERSIZE];
		size_t	vlen;

		vlen = snprintf(vbuf, sizeof (vbuf),
			"version %s %s (%s-%s-%s)",
			VERSION_DATE, VERSION_STR,
			HOST_CPU, HOST_VENDOR, HOST_OS);
		shvers = alloc(vlen + 1);
		strcpy((char *)shvers, vbuf);
	}

#ifdef	DO_MULTI_OPT
again:
#endif
	if (argc > 1 && *argp[1] == '-')
	{
		cp = argp[1];
		/*
		 * Allow "--version" by mapping it to "-V".
		 */
		if ((strcmp((char *)&cp[1], "version") == 0) ||
		    (cp[1] == '-' && strcmp((char *)&cp[2], "version") == 0)) {
			cp = UC "-V";
		}

		/*
		 * if first argument is "--" then options are not
		 * to be changed. Fix for problems getting
		 * $1 starting with a "-"
		 */
		else if (cp[1] == '-')
		{
			argp[1] = argp[0];
			argc--;
#ifdef	DO_MULTI_OPT
			setopts();
#endif
			return (argc);
		}
		if (cp[1] == '\0')
			flags &= ~(execpr|readpr);

		/*
		 * Step along 'flagchar[]' looking for matches.
		 * 'sicrp' are not legal with 'set' command.
		 */
		(void) mbtowc(NULL, NULL, 0);
		cp++;
		while (*cp) {
			if ((len = mbtowc(&wc, (char *)cp, MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				len = 1;
				wc = (unsigned char)*cp;
				failed(argv[1], badopt);
			}
			cp += len;

#ifdef	DO_SET_O
			if (wc == 'o') {
				unsigned char *argarg;
				int	dolistopts = argc <= 2 ||
						argp[2][0] == '-' ||
						argp[2][0] == '+';

				if (dolistopts) {
					listopts(0);
					continue;
				}
				argarg = UC strchr((char *)argp[2], '=');
				if (argarg != NULL)
					*argarg = '\0';
				for (flagc = flagchar;
							/* LINTED */
				    flagname[flagc-flagchar]; flagc++) {
							/* LINTED */
					if (eq(argp[2],
					    flagname[flagc-flagchar])) {
						argp[1] = argp[0];
						argp++;
						argc--;
						wc = *flagc;
#ifdef	DO_SYSALIAS
							/* LINTED */
						if (flagval[flagc-flagchar] &
						    aliasownerflg) {
							char *owner;

							if (argarg != NULL)
								owner = (char *)
								    &argarg[1];
							else
								owner = "";
							ab_setaltowner(
							    GLOBAL_AB, owner);
							ab_setaltowner(
							    LOCAL_AB, owner);
						}
#endif
						break;
					}
				}
				if (argarg != NULL)
					*argarg = '=';
				if (wc != *flagc) {
					if (argc > 2)
						failed(argp[2], badopt);
					continue;
				}
			} else {
#else	/* !DO_SET_O */
			{
#endif
				flagc = flagchar;
				while (*flagc && wc != *flagc)
					flagc++;
			}
			if (wc == *flagc)
			{
				if (eq(argv[0], "set") &&
				    wc && any(wc, UC "sicrp")) {
					failed(argv[1], badopt);
				} else {
					unsigned long *fp = &flags;

							/* LINTED */
					fv = flagval[flagc-flagchar];
					if (fv & fl2)
						fp = &flags2;
					*fp |= fv & ~fl2;
					/*
					 * Disallow to set -n on an interactive
					 * shell as this cannot be reset.
					 */
					if (flags & intflg)
						flags &= ~noexec;
#ifdef	INTERACTIVE
					flags &= ~viflg;
#endif
					if (fv & errflg)
						eflag = errflg;
#ifdef	EXECATTR_FILENAME
					if (fv & pfshflg)
						secpolicy_init();
#endif
					if (fv & versflg) {
						flags &= ~versflg;
						prversion();
					}
#ifdef	DO_SYSALIAS
					if (fv & globalaliasflg) {
						catpath(homenod.namval,
						    UC globalname);
						ab_use(GLOBAL_AB,
						    (char *)make(curstak()));
					}
					if (fv & localaliasflg) {
						ab_use(LOCAL_AB,
							(char *)localname);
					}
#endif
				}
			} else if (wc == 'c' && argc > 2 && comdiv == 0) {
				comdiv = argp[2];
				argp[1] = argp[0];
				argp++;
				argc--;
			} else {
				failed(argv[1], badopt);
			}
		}
		argp[1] = argp[0];
		argc--;
		argp++;
	} else if (argc > 1 &&
		    *argp[1] == '+')	{ /* unset flags x, k, t, n, v, e, u */

		(void) mbtowc(NULL, NULL, 0);
		cp = argp[1];
		cp++;
		while (*cp)
		{
			if ((len = mbtowc(&wc, (char *)cp, MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				cp++;
				continue;
			}
			cp += len;

#ifdef	DO_SET_O
			if (wc == 'o') {
				int	dolistopts = argc <= 2 ||
						argp[2][0] == '-' ||
						argp[2][0] == '+';

				if (dolistopts) {
					listopts(1);
					continue;
				}
				for (flagc = flagchar;
							/* LINTED */
				    flagname[flagc-flagchar]; flagc++) {
							/* LINTED */
					if (eq(argp[2],
					    flagname[flagc-flagchar])) {
						argp[1] = argp[0];
						argp++;
						argc--;
						wc = *flagc;
						break;
					}
				}
				if (wc != *flagc) {
					if (argc > 2)
						failed(argp[2], badopt);
					continue;
				}
			} else {
#else	/* !DO_SET_O */
			{
#endif
				flagc = flagchar;
				while (*flagc && wc != *flagc)
					flagc++;
			}
			/*
			 * step through flags
			 */
			if (wc == 0 ||
			    (!any(wc, UC "sicrp") && wc == *flagc)) {
				unsigned long *fp = &flags;
							/* LINTED */
				fv = flagval[flagc-flagchar];
				if (fv & fl2)
					fp = &flags2;
				*fp &= ~fv;
				if (wc == 'e')
					eflag = 0;
#ifdef	EXECATTR_FILENAME
				if (fv & pfshflg)
					secpolicy_end();
#endif
#ifdef	DO_SYSALIAS
				if (fv & globalaliasflg) {
					ab_use(GLOBAL_AB, NULL);
				}
				if (fv & localaliasflg) {
					ab_use(LOCAL_AB, NULL);
				}
				if (fv & aliasownerflg) {
					ab_setaltowner(GLOBAL_AB, "");
					ab_setaltowner(LOCAL_AB, "");
				}
#endif
			}
		}
		argp[1] = argp[0];
		argc--;
		argp++;
	}
#ifdef	DO_MULTI_OPT
	if (argc > 1 && (*argp[1] == '-' || *argp[1] == '+'))
		goto again;
#endif

	setopts();
	return (argc);
}

/*
 * set up $-
 * $- is only constructed from flag values in the basic "flags"
 */
void
setopts()
{
	unsigned char	*flagc;
	unsigned char	*flagp;

	flagp = flagadr;
	if (flags) {
		flagc = flagchar;
		while (*flagc) {
						/* LINTED */
			if (flags & flagval[flagc-flagchar])
				*flagp++ = *flagc;
			flagc++;
		}
	}
	*flagp = 0;
}

/*
 * sets up positional parameters
 */
void
setargs(argi)
	unsigned char	*argi[];
{
	unsigned char **argp = argi;	/* count args */
	int argn = 0;

	while (*argp++ != UC ENDARGS)
		argn++;
	/*
	 * free old ones unless on for loop chain
	 */
	freedolh();
	dolh = copyargs(argi, argn);
	dolc = argn - 1;
}


static void
freedolh()
{
	unsigned char **argp;
	struct dolnod *argblk;

	if ((argblk = dolh) != NULL) {
		if ((--argblk->doluse) == 0) {
			for (argp = argblk->dolarg; *argp != UC ENDARGS; argp++)
				free(*argp);
			free(argblk->dolarg);
			free(argblk);
		}
	}
}

struct dolnod *
freeargs(blk)
	struct dolnod *blk;
{
	unsigned char **argp;
	struct dolnod *argr = 0;
	struct dolnod *argblk;
	int cnt;

	if ((argblk = blk) != NULL) {
		argr = argblk->dolnxt;
		cnt  = --argblk->doluse;

		if (argblk == dolh) {
			if (cnt == 1)
				return (argr);
			else
				return (argblk);
		} else {
			if (cnt == 0) {
				for (argp = argblk->dolarg;
				    *argp != UC ENDARGS; argp++) {
					free(*argp);
				}
				free(argblk->dolarg);
				free(argblk);
			}
		}
	}
	return (argr);
}

static struct dolnod *
copyargs(from, n)
	unsigned char	*from[];
	int		n;
{
	struct dolnod *np = (struct dolnod *)alloc(sizeof (struct dolnod));
	unsigned char **fp = from;
	unsigned char **pp;

	np -> dolnxt = 0;
	np->doluse = 1;	/* use count */
	pp = np->dolarg = (unsigned char **)alloc((n+1)*sizeof (char *));
	dolv = pp;

	while (n--)
		*pp++ = make(*fp++);
	*pp++ = ENDARGS;
	return (np);
}


static struct dolnod *
clean_args(blk)
	struct dolnod *blk;
{
	unsigned char **argp;
	struct dolnod *argr = 0;
	struct dolnod *argblk;

	if ((argblk = blk) != NULL)
	{
		argr = argblk->dolnxt;

		if (argblk == dolh)
			argblk->doluse = 1;
		else
		{
			for (argp = argblk->dolarg; *argp != UC ENDARGS; argp++)
				free(*argp);
			free(argblk->dolarg);
			free(argblk);
		}
	}
	return (argr);
}

void
clearup()
{
	/*
	 * force `for' $* lists to go away
	 */
	if (globdolv)
		dolv = globdolv;
	if (globdolc)
		dolc = globdolc;
	if (globdolh)
		dolh = globdolh;
	globdolv = 0;
	globdolc = 0;
	globdolh = 0;
	while ((argfor = clean_args(argfor)) != NULL)
		/* LINTED */
		;
	/*
	 * clean up io files
	 */
	while (pop())
		/* LINTED */
		;

	/*
	 * Clean up pipe file descriptor
	 * from command substitution
	 */

	if (savpipe != -1) {
		close(savpipe);
		savpipe = -1;
	}

	/*
	 * clean up tmp files
	 */
	while (poptemp())
		/* LINTED */
		;
}

/*
 * Save positiional parameters before outermost function invocation
 * in case we are interrupted.
 * Increment use count for current positional parameters so that they aren't
 * thrown away.
 */

struct dolnod *
savargs(funcntp)
int funcntp;
{
	if (!funcntp) {
		globdolh = dolh;
		globdolv = dolv;
		globdolc = dolc;
	}
	useargs();
	return (dolh);
}

/*
 * After function invocation, free positional parameters,
 * restore old positional parameters, and restore
 * use count.
 */

void
restorargs(olddolh, funcntp)
struct dolnod *olddolh;
int funcntp;
{
	if (argfor != olddolh)
		while ((argfor = clean_args(argfor)) != olddolh && argfor)
			/* LINTED */
			;
	if (!argfor)
		return;
	freedolh();
	dolh = olddolh;

	/*
	 * increment use count so arguments aren't freed
	 */
	if (dolh)
		dolh -> doluse++;
	argfor = freeargs(dolh);
	if (funcntp == 1) {
		globdolh = 0;
		globdolv = 0;
		globdolc = 0;
	}
}

struct dolnod *
useargs()
{
	if (dolh) {
		if (dolh->doluse++ == 1) {
			dolh->dolnxt = argfor;
			argfor = dolh;
		}
	}
	return (dolh);
}

#ifdef	DO_SET_O
#ifdef	DO_SYSALIAS
static void
listaliasowner(parse, flagidx)
	int	parse;
	int	flagidx;
{
	uid_t	altuid = ab_getaltowner(GLOBAL_AB);

	if (parse) {
		prs_buff(UC "set ");
		if (altuid == (uid_t)-1)
			prs_buff(UC "+o ");
		else
			prs_buff(UC "-o ");
	}
	prs_buff(UC flagname[flagidx]);
	if (altuid == (uid_t)-1 && parse) {
		prc_buff(NL);
		return;
	}
	prs_buff(UC "=");
	if (altuid != (uid_t)-1)
		prs_buff(UC ab_getaltoname(GLOBAL_AB));
	prc_buff(NL);
}
#endif

static void
listopts(parse)
	int	parse;
{
	unsigned char *flagc;
	int		len;
	unsigned long	fv;
	unsigned long	*fp;

					/* LINTED */
	for (flagc = flagchar; flagname[flagc-flagchar]; flagc++) {
		if (*flagc == 'V')
			continue;
					/* LINTED */
		fv = flagval[flagc-flagchar];
#ifdef	DO_SYSALIAS
		if (fv == aliasownerflg) {
					/* LINTED */
			listaliasowner(parse, flagc-flagchar);
			continue;
		}
#endif
		fp = &flags;
		if (fv & fl2) {
			fp = &flags2;
			fv &= ~fl2;
		}
		if (parse) {
			if (any(*flagc, UC "sicrp"))	/* Unsettable?    */
				continue;		/* so do not list */
			prs_buff(UC "set ");
			prs_buff(UC(*fp & fv ? "-":"+"));
			prs_buff(UC "o ");
		}
					/* LINTED */
		prs_buff(UC flagname[flagc-flagchar]);
		if (parse) {
			prc_buff(NL);
			continue;
		}
					/* LINTED */
		len = length(UC flagname[flagc-flagchar]);
		while (++len <= 16)
			prc_buff(SPACE);
		prc_buff(TAB);
		prs_buff(UC(*fp & fv ? "on":"off"));
		prc_buff(NL);
	}
}
#endif	/* DO_SET_O */
