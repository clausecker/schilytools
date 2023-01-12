/* @(#)defaults.c	1.19 19/10/13 Copyright 1998-2019 J. Schilling */
#include <schily/mconfig.h>
#include <stdlib.h>
#include <string.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)defaults.c	1.19 19/10/13 Copyright 1998-2019 J. Schilling";
#endif
/*
 *	Copyright (c) 1998-2019 J. Schilling
 *	Copyright (c) 2022-2023 the schilytools team
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

#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/deflts.h>
#include <schily/utypes.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include "star.h"
#include "starsubs.h"

extern	const char	*tarfiles[];
extern	long		bs;
extern	int		nblocks;
extern	BOOL		not_tape;
extern	Ullong		tsize;
extern	BOOL		silent;
extern	BOOL		print_artype;
extern	BOOL		xflag;
extern	BOOL		nflag;

EXPORT	char	*get_stardefaults __PR((char *name));
LOCAL	int	open_stardefaults __PR((char *dfltname));
EXPORT	void	star_defaults	__PR((long *fsp, BOOL *no_fsyncp,
						BOOL *secure_linkp,
						char *dfltname));
EXPORT	BOOL	star_darchive	__PR((char *arname, char *dfltname));
EXPORT	char	**get_args_for_helper	__PR((char *alg, char *section,
						char *dfltflg, char *xtraflg));

EXPORT char *
get_stardefaults(name)
	char	*name;
{
	char	*ret;

	if (name)
		return (name);
	/*
	 * WARNING you are only allowed to change this filename if you also
	 * change the documentation and add a statement that makes clear
	 * where the official location of the file is why you did choose a
	 * nonstandard location and that the nonstandard location only refers
	 * to inofficial star versions.
	 *
	 * I was forced to add this because some people change star without
	 * rational reason and then publish the result. As those people
	 * don't contribute work and don't give support, they are causing extra
	 * work for me and this way slow down the star development.
	 */
#if defined(PROTOTYPES) && defined(INS_RBASE)
	ret = INS_RBASE "/etc/default/star";
	while (ret[0] == '/' && ret[1] == '/')
		ret++;
#else
	ret = "/etc/default/star";
#endif
	return (ret);
}

LOCAL int
open_stardefaults(dfltname)
	char	*dfltname;
{
	/*
	 * We may later set options here....
	 */
	return (defltopen(dfltname));
}

EXPORT void
star_defaults(fsp, no_fsyncp, secure_linkp, dfltname)
	long	*fsp;
	BOOL	*no_fsyncp;
	BOOL	*secure_linkp;
	char	*dfltname;
{
	BOOL	is_open = FALSE;
	BOOL	nofsync = FALSE;
	BOOL	securelinks = FALSE;
	BOOL	nohint = FALSE;
	long	fs_cur	= 0L;
	long	fs_max	= -1L;

	dfltname = get_stardefaults(dfltname);

	if (getenv("STAR_NOHINT"))
		nohint = TRUE;

	if (fsp != NULL)
		fs_cur = *fsp;

	if (fs_cur <= 0L) {
		char	*p = NULL;

		if (open_stardefaults(dfltname) == 0) {
			is_open = TRUE;
			p = defltread("STAR_FIFOSIZE=");
		}
		if (p) {
			if (getnum(p, &fs_cur) != 1)
				comerrno(EX_BAD, "Bad fifo size default.\n");
		}
	}
	if (fs_cur > 0L) {
		char	*p = NULL;

		if (is_open) {
			defltfirst();
		} else if (open_stardefaults(dfltname) == 0) {
			is_open = TRUE;
		}
		if (is_open)
			p = defltread("STAR_FIFOSIZE_MAX=");
		if (p) {
			if (getnum(p, &fs_max) != 1) {
				comerrno(EX_BAD,
					"Bad max fifo size default.\n");
			}
		}
		if (fs_cur > fs_max)
			fs_cur = fs_max;
	}

	if (fs_cur > 0L && fsp != NULL)
		*fsp = fs_cur;

	if (no_fsyncp)
		nofsync = *no_fsyncp;
	if (nofsync < 0) {
		char	*p = NULL;
		BOOL	was_env = FALSE;

		p = getenv("STAR_FSYNC");
		if (p == NULL) {
			if (is_open) {
				defltfirst();
			} else if (open_stardefaults(dfltname) == 0) {
				is_open = TRUE;
			}
			if (is_open)
				p = defltread("STAR_FSYNC=");
		} else {
			was_env = TRUE;
		}
		if (p) {
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == 'n' || *p == 'N') {
				nofsync = TRUE;
				if (!silent && !nohint &&
				    !print_artype && xflag && !nflag)
					errmsgno(EX_BAD,
					"WARNING: fsync() disabled from '%s'.\n",
					was_env ? "environment" :
					dfltname);
			} else {
				nofsync = FALSE;
			}
		}
	}
	if (no_fsyncp && nofsync >= 0)
		*no_fsyncp = nofsync;

	if (secure_linkp)
		securelinks = *secure_linkp;
	if (securelinks < 0) {
		char	*p = NULL;
		BOOL	was_env = FALSE;

		p = getenv("STAR_SECURE_LINKS");
		if (p == NULL) {
			if (is_open) {
				defltfirst();
			} else if (open_stardefaults(dfltname) == 0) {
				is_open = TRUE;
			}
			if (is_open)
				p = defltread("STAR_SECURE_LINKS=");
		} else {
			was_env = TRUE;
		}
		if (p) {
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == 'n' || *p == 'N') {
				securelinks = TRUE;
				if (!silent && !nohint &&
				    !print_artype && xflag && !nflag)
					errmsgno(EX_BAD,
					"WARNING: -no-secure-links enabled from '%s'.\n",
					was_env ? "environment" :
					dfltname);
			} else {
				securelinks = FALSE;
			}
		}
	}
	if (secure_linkp && securelinks >= 0)
		*secure_linkp = securelinks;

	defltclose();
}

/*
 * Check 'dfltname' for an 'arname' pattern.
 *
 * A correct entry has 3..4 space separated entries:
 *	1)	The device (archive0=/dev/tape)
 *	2)	The bloking factor in 512 byte units (20)
 *	3)	The max media size in 1024 byte units
 *		0 means unlimited (no multi volume handling)
 *	4)	Whether this is a tape or not
 * Examples:
 *	archive0=/dev/tape 512 0 y
 *	archive1=/dev/fd0 1 1440 n
 */
EXPORT BOOL
star_darchive(arname, dfltname)
	char	*arname;
	char	*dfltname;
{
	char	*p;
	Llong	llbs	= 0;

	dfltname = get_stardefaults(dfltname);
	if (dfltname == NULL)
		return (FALSE);
	if (open_stardefaults(dfltname) != 0)
		return (FALSE);

	/*
	 * XXX Sun tar arbeitet mit Ignore Case
	 */
	if ((p = defltread(arname)) == NULL) {
		errmsgno(EX_BAD,
		    "Missing or invalid '%s' entry in %s.\n",
				arname, dfltname);
		return (FALSE);
	}
	if ((p = strtok(p, " \t")) == NULL) {
		errmsgno(EX_BAD,
		    "'%s' entry in %s is empty!\n", arname, dfltname);
		return (FALSE);
	} else {
		tarfiles[0] = ___savestr(p);
	}
	if ((p = strtok(NULL, " \t")) == NULL) {
		errmsgno(EX_BAD,
		    "Block component missing in '%s' entry in %s.\n",
		    arname, dfltname);
		return (FALSE);
	}
	if (getbnum(p, &llbs) != 1) {
		comerrno(EX_BAD, "Bad blocksize entry for '%s' in %s.\n",
			arname, dfltname);
	} else {
		int	iblocks = llbs;

		if ((llbs <= 0) || (iblocks != llbs)) {
			comerrno(EX_BAD,
				"Invalid blocksize '%s'.\n", p);
		}
		bs = llbs;
		nblocks = bs / TBLOCK;
	}
	if ((p = strtok(NULL, " \t")) == NULL) {
		errmsgno(EX_BAD,
		    "Size component missing in '%s' entry in %s.\n",
		    arname, dfltname);
		return (FALSE);
	}
	if (getknum(p, &llbs) != 1) {	/* tsize is Ullong */
		comerrno(EX_BAD, "Bad size entry for '%s' in %s.\n",
			arname, dfltname);
	}
	tsize = llbs / TBLOCK;
	/* XXX Sun Tar hat check auf min size von 250 kB */

	/*
	 * XXX Sun Tar setzt not_tape auch wenn kein Tape Feld vorhanden ist
	 * XXX und tsize != 0 ist.
	 */
	if ((p = strtok(NULL, " \t")) != NULL)		/* May be omited */
		not_tape = (*p == 'n' || *p == 'N');
	defltclose();
#ifdef DEBUG
	error("star_darchive: archive='%s'; tarfile='%s'\n", arname, tarfiles[0]);
	error("star_darchive: nblock='%d'; tsize='%llu'\n",
	    nblocks, tsize);
	error("star_darchive: not tape = %d\n", not_tape);
#endif
	return (TRUE);
}

LOCAL char *
star_get_cmd_flags(prog_name, section)
	char	*prog_name, *section;
{
	char	*dfltname;
	char	*cfg_name;
	char	*cfg_value;
	int	prog_name_len = strlen(prog_name);

	if (prog_name_len == 0)
		return (NULL);

	dfltname = get_stardefaults(NULL);
	if (dfltname == NULL)
		return (NULL);
	if (open_stardefaults(dfltname) != 0)
		return (NULL);

	if (defltsect(section) < 0)
		return (NULL);

	cfg_name = malloc(prog_name_len + sizeof "_CMD=");
	if (cfg_name == NULL)
		return (NULL);

	for (int i=0; i < prog_name_len; i++)
		cfg_name[i] = toupper(prog_name[i]);
	strcpy(&cfg_name[prog_name_len], "_CMD=");


	cfg_value = defltread(cfg_name);
	free(cfg_name);

	return (cfg_value);
}

/*
 * Get the arguments for compression/decompression helpers.
 * Return a pointer to a NULL-terminated argument vector or NULL
 * on error.
 *
 * Invariant: the function is either called for compression or
 * for decompression.
 *
 * Compression: section = "[compress]", dfltflg = NULL,
 * 	xtraflag = getenv("STAR_COMPRESS_FLAG").
 * Decompression: section = "[decompress]", dfltflg = "-d",
 * 	xtraflag = NULL.
 */
EXPORT char **
get_args_for_helper(alg, section, dfltflg, xtraflg)
	char	*alg, *section, *dfltflg, *xtraflg;
{
	int i, n;
	char *flag, *flags, *tokflags, **argv;
	static char *dfltargv[3];

	flags = star_get_cmd_flags(alg, section);
	if (flags == NULL)
		goto fallback;

	/* find number of arguments */
	n = 0;
	tokflags = strdup(flags);
	if (tokflags == NULL)
		return (NULL);

	flag = strtok(flags, " \t");
	while (flag != NULL) {
		n++;
		flag = strtok(NULL, " \t");
	}

	/* flags empty: treat as if unset */
	if (n == 0)
		goto fallback;

	argv = calloc(n + 2, sizeof *argv);
	if (argv == NULL)
		return (NULL);

	argv[0] = strtok(tokflags, " \t");
	for (i = 1; i < n; i++)
		argv[i] = strtok(NULL, " \t");

	if (xtraflg != NULL)
		argv[i++] = xtraflg;

	return (argv);

fallback:
	dfltargv[0] = alg;
	dfltargv[1] = dfltflg != NULL ? dfltflg : xtraflg;
	dfltargv[2] = NULL;

	return (dfltargv);
}
