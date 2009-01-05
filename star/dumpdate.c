/* @(#)dumpdate.c	1.20 08/12/22 Copyright 2003-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)dumpdate.c	1.20 08/12/22 Copyright 2003-2008 J. Schilling";
#endif
/*
 *	Copyright (c) 2003-2008 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/errno.h>
#include <schily/utypes.h>
#include <schily/standard.h>
#include <schily/fcntl.h>
#include <schily/schily.h>

#include "dumpdate.h"
#include "starsubs.h"

/*#define	DEBUG*/

#ifdef	HAVE_LARGEFILES
/*
 * XXX Hack until fseeko()/ftello() are available everywhere or until
 * XXX we know a secure way to let autoconf ckeck for fseeko()/ftello()
 * XXX without defining FILE_OFFSETBITS to 64 in confdefs.h
 */
#	define	fseek	fseeko
#	define	ftell	ftello
#endif

#if	defined(HAVE_FLOCK) || defined(HAVE_LOCKF) || defined(HAVE_FCNTL_LOCKF)
#	define	HAVE_LOCKING
#endif

#ifndef	HAVE_FLOCK
#ifdef	HAVE_FCNTL_LOCKF
LOCAL	struct flock	__fl;
#define	flock(fd, flag)	(__fl.l_type = (flag), fcntl(fd, F_SETLKW, &__fl))
#define	LOCK_EX	F_WRLCK
#define	LOCK_SH	F_RDLCK
#define	LOCK_UN	F_UNLCK
#else
#define	flock(fd, flag)	lockf(fd, flag, (off_t)0)
#define	LOCK_EX	F_LOCK
#define	LOCK_SH	F_LOCK
#define	LOCK_UN	F_ULOCK
#endif
#endif	/* HAVE_FLOCK */

#ifndef	HAVE_LOCKING
#undef	flock
#define	flock(fd, flag)
#endif

LOCAL	dumpd_t	*dumpdates;
LOCAL	dumpd_t	**dumptail = &dumpdates;

EXPORT	void	initdumpdates	__PR((char *fname, BOOL doupdate));
LOCAL	void	readdumpdates	__PR((FILE *f, char *fname));
EXPORT	void	writedumpdates	__PR((char *fname, const char *filesys,
							int level, int dflags,
							struct timeval *date));
LOCAL	void	outentry	__PR((FILE *f, char *name, int level, int dflags,
							struct timeval *date));
EXPORT	char	*dumpdate	__PR((struct timeval *date));
LOCAL	char	*skipwht	__PR((char *p));
LOCAL	BOOL	getentry	__PR((char *line, char *fname));
EXPORT	BOOL	getdumptime	__PR((char *p, struct timeval *tvp));
EXPORT	dumpd_t *checkdumpdates	__PR((const char *name, int level, int dflags));
LOCAL	dumpd_t *_checkdumpdates __PR((const char *name, int level, int dflags));
EXPORT	void	adddumpdates	__PR((const char *name, int level, int dflags,
							struct timeval *date,
								BOOL useold));
LOCAL	dumpd_t *newdumpdates	__PR((const char *name, int level, int dflags,
							struct timeval *date));
LOCAL	dumpd_t	*freedumpdates	__PR((dumpd_t *dp));

EXPORT void
initdumpdates(fname, doupdate)
	char	*fname;
	BOOL	doupdate;
{
	FILE	*f;

	f = fileopen(fname, "r");
	if (f == NULL) {
		if (geterrno() == ENOENT) {
			errmsg("Warning no %s.\n", fname);
			return;
		}
		comerr("Cannot open %s.\n", fname);
	}
	if (doupdate && access(fname, W_OK) < 0)
		comerr("Cannot access '%s' for update\n", fname);

	(void) flock(fdown(f), LOCK_SH);
	readdumpdates(f, fname);
	(void) flock(fdown(f), LOCK_UN);
	fclose(f);
}

LOCAL void
readdumpdates(f, fname)
	FILE	*f;
	char	*fname;
{
	char	buf[4096];
	int	line = 0;

	while (fgetline(f, buf, sizeof (buf)) >= 0) {
		line++;

		if (!getentry(buf, fname)) {
			if (*skipwht(buf) != '\0') {
				errmsgno(EX_BAD,
					"Unknown format in '%s' line %d\n",
					fname, line);
			}
			continue;
		}
	}
}

EXPORT void
writedumpdates(fname, filesys, level, dflags, date)
	char		*fname;
	const char	*filesys;
	int		level;
	int		dflags;
	struct timeval	*date;
{
	FILE	*f;
	dumpd_t	*dp;
	off_t	fsize;
	off_t	fpos;

	f = fileopen(fname, "rwc");
	if (f == NULL) {
		errmsg("Cannot open '%s'.\n", fname);
		return;
	}
	(void) flock(fdown(f), LOCK_EX);

	for (dp = dumpdates; dp; dp = freedumpdates(dp))
		;
	dumpdates = NULL;
	dumptail = &dumpdates;

	readdumpdates(f, fname);
	adddumpdates(filesys, level, dflags, date, TRUE); /* Upd. curr. entry */
	fseek(f, 0L, SEEK_SET);

	fsize = filesize(f);
	for (dp = dumpdates; dp; dp = dp->next) {
		outentry(f, dp->name, dp->level, dp->flags, &dp->date);
	}
	fflush(f);
	fpos = filepos(f);
#ifdef	HAVE_FTRUNCATE
	if (fpos < fsize)
		ftruncate(fdown(f), fpos);
#else
	while (fpos++ < fsize)
		putc(' ', f);
#endif

	(void) flock(fdown(f), LOCK_UN);
	fclose(f);
	error("Dump record  level %d%s dump: %s written\n",
			level, (dflags & DD_PARTIAL) ? "P":" ",
			dumpdate(date));
}

LOCAL void
outentry(f, name, level, dflags, date)
	FILE		*f;
	char		*name;
	int		level;
	int		dflags;
	struct timeval	*date;
{
	int	len;
	time_t	t = date->tv_sec; /* FreeBSD/MacOS X -> broken tv_sec/time_t */

	len = strlen(name);
	if ((len % 8) == 0)
		len += 8;
	len += 7;
	len &= ~7;
	len = 41 -len;
	if (len < 1)
		len = 1;

	fprintf(f, "%s\t%*s%2d%s %10lld.%6.6lld %s",
		name,
		len,
		"",
		level,
		(dflags & DD_PARTIAL) ? "P":"",
		(Llong)date->tv_sec,
		(Llong)date->tv_usec,
		ctime(&t));
}

EXPORT char *
dumpdate(date)
	struct timeval	*date;
{
	char	*p;
	time_t	t = date->tv_sec; /* FreeBSD/MacOS X -> broken tv_sec/time_t */

	if (date->tv_sec == 0)
		return ("the epoch");
	p = ctime(&t);
	p[24] = '\0';
	return (p);
}

LOCAL char *
skipwht(p)
	char	*p;
{
	while (*p && (*p == ' ' || *p == '\t'))
		p++;

	return (p);
}

LOCAL BOOL
getentry(line, fname)
	char	*line;
	char	*fname;
{
	char	*p;
	int	level;
	int	dflags = 0;
	struct timeval	tdate;

	p = line;
	do {
		p = strchr(++p, '\t');
	} while (p && p[0] != '\0' && p[1] != ' ');

	if (p == NULL || p[0] == '\0') {
		errmsgno(EX_BAD, "Error parsing mount point in '%s'\n", fname);
		return (FALSE);
	}
	*p++ = '\0';

	p = skipwht(p);
	p = astoi(p, &level);
	if (*p == 'P') {
		dflags |= DD_PARTIAL;
		p++;
	}
	if (*p != ' ') {
		errmsgno(EX_BAD, "Error parsing dump level in '%s'\n", fname);
		return (FALSE);
	}
	p = skipwht(p);
	if (!getdumptime(p, &tdate))
		return (FALSE);
	adddumpdates(line, level, dflags, &tdate, FALSE);

	return (TRUE);
}

EXPORT BOOL
getdumptime(p, tvp)
	char		*p;
	struct timeval	*tvp;
{
	Llong		date;
	Llong		dfrac;

	p = astollb(p, &date, 10);
	if (*p == '.') {
		int	l;
		char	*s = ++p;

		p = astollb(s, &dfrac, 10);
		l = p - s;
		if (l > 9) {		/* Number too big, limit to nsec */
			char	*p2 = p;

			l = s[9];
			s[9] = '\0';
			p = astollb(s, &dfrac, 10);
			s[9] = l;
			l = p - s;
			p = p2;
		}
		while (l < 6) {		/* Convert to usecs */
			dfrac  *= 10;
			l++;
		}
		while (l > 6) {		/* Convert to usecs */
			dfrac  /= 10;
			l--;
		}
	}
	if (*p != ' ' && *p != '\0') {
		errmsgno(EX_BAD, "Error parsing dump date\n");
		return (FALSE);
	}
	tvp->tv_sec = date;
	tvp->tv_usec = dfrac;
	return (TRUE);
}

EXPORT dumpd_t *
checkdumpdates(name, level, dflags)
	const char	*name;
	int		level;
	int		dflags;
{
	dumpd_t	*rp = NULL;
	dumpd_t	*rp2 = NULL;

	rp = _checkdumpdates(name, level, dflags & ~DD_CUMULATIVE);
	rp2 = _checkdumpdates(name, level, dflags);

	if (rp && rp2 && (rp2->date.tv_sec > rp->date.tv_sec))
		return (rp2);
	return (rp);
}

LOCAL dumpd_t *
_checkdumpdates(name, level, dflags)
	const char	*name;
	int		level;
	int		dflags;
{
	dumpd_t	*dp = dumpdates;
	dumpd_t	*rp = NULL;

	for (; dp; dp = dp->next) {
		if (!streql(name, dp->name))
			continue;
		if ((dp->flags & DD_PARTIAL) != (dflags & DD_PARTIAL))
			continue;
		if ((dflags & DD_CUMULATIVE) && level == dp->level) {
			rp = dp;
			break;
		}
		/*
		 * We are not interested in tardump entries for
		 * this level or higher, so skip them.
		 */
		if (level <= dp->level)
			continue;

		/*
		 * If we did already find a more recent entry in tardumps
		 * we consider this one outdated.
		 */
		if (rp && rp->date.tv_sec > dp->date.tv_sec)
			continue;
		rp = dp;
	}
#ifdef	DEBUG
	if (rp)
		outentry(stderr, rp->name, rp->level, rp->flags, &rp->date);
#endif
	return (rp);
}

EXPORT void
adddumpdates(name, level, dflags, date, useold)
	const char	*name;
	int		level;
	int		dflags;
	struct timeval	*date;
	BOOL		useold;
{
	dumpd_t	*dp = dumpdates;

	for (; dp; dp = dp->next) {
		if (streql(name, dp->name) && level == dp->level &&
		    (dp->flags & DD_PARTIAL) == (dflags & DD_PARTIAL)) {
			if (useold) {
				dp->date = *date;
				return;
			}
			errmsgno(EX_BAD,
				"Duplicate tardumps entry '%s %d%s %lld'.\n",
				dp->name, dp->level,
				(dflags & DD_PARTIAL) ? "P":"",
				(Llong)dp->date.tv_sec);

			if (date->tv_sec == dp->date.tv_sec)
				return;
			comerrno(EX_BAD, "Timestamps differs - aborting.\n");
		}
	}
	dp = newdumpdates(name, level, dflags, date);
	*dumptail = dp;
	dumptail = &dp->next;
}

LOCAL dumpd_t *
newdumpdates(name, level, dflags, date)
	const char	*name;
	int		level;
	int		dflags;
	struct timeval	*date;
{
	dumpd_t	*dp;

	dp	  = ___malloc(sizeof (*dp), "tardumps entry");
	dp->next  = NULL;
	dp->name  = ___savestr(name);
	dp->level = level;
	dp->flags = dflags;
	dp->date  = *date;

	return (dp);
}

LOCAL dumpd_t *
freedumpdates(dp)
	dumpd_t	*dp;
{
	dumpd_t	*next = dp->next;

	free(dp->name);
	free(dp);

	return (next);
}
