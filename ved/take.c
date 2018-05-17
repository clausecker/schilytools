/* @(#)take.c	1.24 09/07/09 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)take.c	1.24 09/07/09 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	Routines that implement a set of take buffers
 *	and take care of backing up unused buffers into a file
 *
 *	Copyright (c) 1984-2009 J. Schilling
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

#define	MAXTAKEBUFS 32

/*
 * XXX eigentlich MAXFN - "vedT" - "." - sizeof (pid)
 * XXX Bei altem UNIX:	14 - 4 - 1 - 5 == 4
 * XXX Bei DOS:		8 - 4 == 4
 * XXX richtigen Algorithmus finden!!!!!
 */
#define	TAKENAMESIZE 17

typedef struct {
	Uchar	*name;
	Uchar	*pathname;
}  take_t;

LOCAL	take_t	takebufs[MAXTAKEBUFS];	/* the names of take buffers	    */
LOCAL	int	maxbuf;			/* # of take buffers currently in use */
LOCAL	int	curbuf;			/* index of current take buffer	    */
LOCAL	Uchar	*curpath;		/* pathname of current take buffer  */
EXPORT	char	TAKEBUF[] = "TAKE BUF";

EXPORT	void	settakename	__PR((ewin_t *wp, Uchar* name));
LOCAL	BOOL	settpath	__PR((Uchar* name, int idx));
LOCAL	int	checktake	__PR((Uchar* name));
EXPORT	Uchar	*findtake	__PR((ewin_t *wp, Uchar* name));
EXPORT	void	backuptake	__PR((ewin_t *wp));
EXPORT	void	loadtake	__PR((ewin_t *wp));
LOCAL	void	newtake		__PR((Uchar* name));
LOCAL	void	oldtake		__PR((ewin_t *wp, int idx));
EXPORT	void	deletetake	__PR((void));
EXPORT	int	fcopy		__PR((ewin_t *wp, FILE * from, FILE * to, epos_t size,
						char *fromname, char *toname));
LOCAL	void	toomany_tbufs	__PR((ewin_t *wp));

/*
 * Set the name of the current take buffer
 */
EXPORT void
settakename(wp, name)
	ewin_t	*wp;
	Uchar	*name;
{
	int	idx;
	Uchar	*p;
	Uchar	tname[TAKENAMESIZE];

	snprintf(C tname, sizeof (tname), "%s", name);
	p = tname;
	while (*p) {
		/*
		 * Do not allow white space in the take buffer name
		 */
		if (white(*p++)) {
			writeerr(wp, "BAD BUFFER NAME");
			return;
		}
	}
	idx = checktake(tname);
	if (idx >= 0) {
		/*
		 * A take buffer with this name already exists.
		 */
		if (curbuf == idx)
			return;
		backuptake(wp);
		oldtake(wp, idx);
		writetake(wp, tname);
		return;
	}

	if (maxbuf >= MAXTAKEBUFS) {
		/*
		 * We will not allow to open another take buffer.
		 */
		toomany_tbufs(wp);
		return;
	}
	/*
	 * Need to create a new take buffer
	 */
	backuptake(wp);
	newtake(tname);
	writetake(wp, tname);
}

/*
 * Set the name of the current take buffer
 */
LOCAL BOOL
settpath(name, idx)
	Uchar	*name;
	int	idx;
{
	Uchar	tpath[FNAMESIZE];
	Uchar	*p;

	p = (Uchar *)malloc(strlen(C name)+1);
	if (p == NULL)
		return (FALSE);

	strcpy(C p, C name);
	takebufs[idx].name = p;

	takepath(tpath, sizeof (tpath), name);
	p = (Uchar *)malloc(strlen(C tpath)+1);
	if (p == NULL)
		return (FALSE);

	strcpy(C p, C tpath);
	takebufs[idx].pathname = p;

	return (TRUE);
}

/*
 * Check if a named take buffer exists
 */
LOCAL int
checktake(name)
	Uchar	*name;
{
	int i;

	for (i = 0; i < maxbuf; i++)
		if (streql(C name, C takebufs[i].name))
			return (i);
	return (-1);
}

/*
 * Get the path name of a take buffer for the execute commands.
 * An empty name is translated into the current take buffer path.
 * This allows to use \ for the current take buffer.
 * If the named take bufer does not exist, create it - it may be loaded later.
 */
EXPORT Uchar *
findtake(wp, name)
	ewin_t	*wp;
	Uchar	*name;
{
	int idx;

	if (*name == 0)
		return (curpath);
	if ((idx = checktake(name)) >= 0)
		return (takebufs[idx].pathname);

	if (maxbuf >= MAXTAKEBUFS) {
		/*
		 * We will not allow to open another take buffer.
		 */
		toomany_tbufs(wp);
		return ((Uchar *)0);
	}
	settpath(name, maxbuf);
	return (takebufs[maxbuf++].pathname);
}

/*
 * Backup the current take file to its named take backup
 */
EXPORT void
backuptake(wp)
	ewin_t *wp;
{
	FILE	*f;

	if (maxbuf) {				/* Paranoia */
		if ((f = opensyserr(wp, curpath, "ctwub")) == NULL)
			return;
		stmpfmodes(curpath);
		fcopy(wp, takefile, f, takesize, TAKEBUF, TAKEBUF);
		fclose(f);
	}
}

/*
 * Reload the current take file from its named take backup
 */
EXPORT void
loadtake(wp)
	ewin_t	*wp;
{
	if (maxbuf) 				/* Paranoia */
		oldtake(wp, curbuf);
}

/*
 * Create a new named take buffer
 */
LOCAL void
newtake(name)
	Uchar	*name;
{
	settpath(name, maxbuf);
	takesize = 0;
	curbuf = maxbuf;
	curpath = takebufs[maxbuf++].pathname;
}

/*
 * Reload the current take file from a specific named take backup
 */
LOCAL void
oldtake(wp, idx)
	ewin_t	*wp;
	int	idx;
{
	FILE	*f;

	if (maxbuf == 0)			/* Paranoia */
		return;
	curbuf = idx;
	curpath = takebufs[idx].pathname;
	if ((f = opensyserr(wp, curpath, "rub")) == NULL)
		return;
	takesize = filesize(f);
	fcopy(wp, f, takefile, takesize, TAKEBUF, TAKEBUF);
	fclose(f);
}

/*
 * Delete all named take buffers
 */
EXPORT void
deletetake()
{
	int i;

	for (i = 0; i < maxbuf; i++) {
		if (takebufs[i].pathname[0] != '\0')
			unlink(C takebufs[i].pathname);
	}
}

/*
 * Copy one open file into another empty file
 */
EXPORT int
fcopy(wp, from, to, size, fromname, toname)
	ewin_t 	*wp;
	FILE	*from;
	FILE	*to;
	epos_t 	size;
	char 	*fromname;
	char	*toname;
{
	Uchar	buf[BUFSIZ < 8192 ? 8192 : BUFSIZ];
	int	amount;
	int	result;

	lseek(fdown(from), (off_t)0, SEEK_SET);
	lseek(fdown(to), (off_t)0, SEEK_SET);
	while (size > 0) {
		amount = min(size, sizeof (buf));
		if ((result = readsyserr(wp, from, buf, amount, UC fromname)) <= 0)
			return (result);
		if ((amount = writesyserr(wp, to, buf, result, UC toname)) != result)
			return (amount);
		size -= amount;
	}
	return (0);
}

LOCAL void
toomany_tbufs(wp)
	ewin_t	*wp;
{
	writeerr(wp, "TOO MANY TAKE BUFFERS");
}
