/* @(#)tags.c	1.30 09/07/13 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)tags.c	1.30 09/07/13 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	Routines that handle references to the tags database.
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
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef nono
#include "ved.h"
#else
#include <schily/stdio.h>
#include <schily/unistd.h>	/* Include sys/types.h to make off_t available */
#include <schily/standard.h>
#include <schily/schily.h>
#endif

#define	iswhite(c)	((c) != '\t')

LOCAL	char	buf[512];	/* XXX Make non static */
LOCAL	char	*tagstring;
LOCAL	Llong	tagline;

EXPORT	int	taglength;	/* Maxnimum number of chars to compare	*/
EXPORT	Uchar	tags[NAMESIZE] = "tags .. /usr/lib/tags";
LOCAL	Uchar	dotdot[NAMESIZE] =
		"../../../../../../../../../../../../../../../../";
LOCAL	int	dotdotlen;
LOCAL	int	taglevel;

LOCAL	FILE	*tagfsearch	__PR((void));
LOCAL	FILE	*tagfopen	__PR((void));
EXPORT	int	gettag		__PR((Uchar** name));
EXPORT	epos_t	searchtag	__PR((ewin_t *wp, epos_t opos));
LOCAL	void	ts_push		__PR((ewin_t *wp));
LOCAL	void	ts_pop		__PR((void));
EXPORT	void	vtag		__PR((ewin_t *wp));
EXPORT	void	gototag		__PR((ewin_t *wp, Uchar* tag));
EXPORT	void	vrtag		__PR((ewin_t *wp));
LOCAL	int	l_breakline	__PR((char *linebuf, int delim, char **array, int len));

#ifdef nono
main(ac, av)
	int	ac;
	char	*av[];
{
	register FILE	*f;
	char	*array[3];
	int	ret;
	register char	cc;
	register int	d;
		off_t	start;
		off_t	mid;
		off_t	end;

	if (ac < 2)
		exit(1);

	cc = av[1][0];
	f = fileopen("tags", "r");
	start = (off_t)0;
	end = filesize(f);

	for (;;) {
		if (start > end) {
			fclose(f);
			return;
		}
		mid = (start + end)/2;
		fileseek(f, mid);
		if (mid > 0)
			if (fgetline(f, buf, sizeof (buf)) < 0) {
				end = mid - 1;
				continue;
			}
		if (fgetline(f, buf, sizeof (buf)) < 0) {
			end = mid - 1;
			continue;
		}
		if ((d = buf[0] - cc) < 0) {
			start = mid + 1;
			continue;
		} else if (d > 0) {
			end = mid - 1;
			continue;
		}
		l_breakline(buf, '\t', array, 2);
		if (taglength == 0) {
			d = strcmp(array[0], av[1]);
		} else {
			int	len;

			len = strlen(av[1]);
			if (taglength > len)
				len = taglength;
			d = strncmp(array[0], av[1], len);
		}

		if (d < 0) {
			start = mid + 1;
			continue;
		} else if (d > 0) {
			end = mid - 1;
			continue;
		} else {
			break;
		}
	}
	fclose(f);
	printf("%d: '%s' '%s' '%s'\n", ret, array[0], array[1], array[2]);
}
#else

/*
 * Search the tags file. Start at the current directory level.
 */
LOCAL FILE *
tagfsearch()
{
	register FILE	*f = 0;
	register Uchar	*p = &dotdot[dotdotlen];

	strcpy(C p, "tags");	/* No overflow, there is enough space */
	while (p >= dotdot) {
		if ((f = fileopen(C p, "r")) != NULL)
			break;
		p -= 3;
		taglevel++;
	}
	dotdot[dotdotlen] = 0;
	return (f);
}

/*
 * Open the tag database. Use the path information in the 'tags' string.
 */
LOCAL FILE *
tagfopen()
{
	register FILE	*f = 0;
	register Uchar	*p = tags;
	register Uchar	*p2;
	register Uchar	c;

	taglevel = 0;
	if (dotdotlen == 0)
		dotdotlen = strlen(C dotdot);
	dotdot[dotdotlen] = 0;
	while (f == 0) {
		while (*p && *p == ' ')
			p++;
		if (*p == '\0')
			break;
		for (p2 = p; *p2 && *p2 != ' '; p2++)
			;
		c = *p2;
		*p2 = '\0';
		if (streql(C p, ".."))
			f = tagfsearch();
		else
			f = fileopen(C p, "r");
		*p2 = c;
		p = p2;
	}

/*	f = fileopen(tags, "r");*/
	return (f);
}

/*
 * Find a named entry in the tag database
 */
EXPORT int
gettag(name)
	Uchar	**name;
{
	register FILE	*f;
		char	*array[3];
		char	*bp;
	register char	*cp;
	register char	c;
	register int	diff;
	register off_t	start;
	register off_t	mid;
	register off_t	end;
	register int	len;

	cp = C *name;
	c = *cp;

/*	if ((f = fileopen(tags, "r")) == (FILE *)0)*/
	if ((f = tagfopen()) == (FILE *)0)
		return (0);
	file_raise(f, FALSE);

	start = (off_t)0;
	end = filesize(f);

	for (;;) {
		if (start > end) {
			fclose(f);
			return (-1);
		}
		mid = (start + end)/2;
		fileseek(f, mid);
		if (mid > 0)
			if (fgetline(f, buf, sizeof (buf)) < 0) {
				end = mid - 1;
				continue;
			}
		if (fgetline(f, buf, sizeof (buf)) < 0) {
			end = mid - 1;
			continue;
		}
		if ((diff = buf[0] - c) < 0) {
			start = mid + 1;
			continue;
		} else if (diff > 0) {
			end = mid - 1;
			continue;
		}
		l_breakline(buf, '\t', array, 2);
		if (taglength == 0) {
			diff = strcmp(array[0], cp);
		} else {
			len = strlen(cp);
			if (taglength > len)
				len = taglength;
			diff = strncmp(array[0], cp, len);
		}

		if (diff < 0) {
			start = mid + 1;
			continue;
		} else if (diff > 0) {
			end = mid - 1;
			continue;
		} else {
			break;
		}
	}
	fclose(f);

	*name = UC array[1];
	if (taglevel && *name[0] != '/') {
		strncpy(C &dotdot[dotdotlen], C *name, NAMESIZE-dotdotlen);
		dotdot[NAMESIZE-1] = '\0';
		*name = &dotdot[dotdotlen-(3*taglevel)];
	}
	bp = array[2];
	tagline = (Llong)-1;
	if (*bp != '/' && *bp != '?') {
		tagstring = (char *)0;
		if (*astoll(bp, &tagline) != '\0')
			return (-1);
		return (1);
	}
	len = strlen(bp);
	if (bp[len-1] == '/' || bp[len-1] == '?')
		bp[--len] = '\0';
/*	if (bp[len-1] == '$')*/
/*		bp[--len] = '\0';*/


#ifdef	PATMATCH
	for (cp = bp; *cp != '\0'; cp++) {
		if (*cp == '#' ||
				*cp == '!' || *cp == '%' ||
				*cp == '[' || *cp == ']' ||
				*cp == '{' || *cp == '}')
			*cp = '?';
	}
#else
	for (cp = bp; *cp != '\0'; cp++) {
		if (*cp == '\\')
			strcpy(cp, cp+1); /* No overflow (gets shorter) */
	}
#endif
	tagstring = bp;
	return (1);
}

/*
 * Search a tag in the current bufer by using search/linenumber information
 * from the tag database.
 */
EXPORT epos_t
searchtag(wp, opos)
	ewin_t	*wp;
	epos_t	opos;
{
		epos_t	pos;
		BOOL	forw;
		char	*bp;
		BOOL	endm;
		BOOL	omagic;

	if (tagline >= (Llong)0)
		return (forwline(wp, (epos_t)0, (ecnt_t)tagline-1));

	if ((bp = tagstring) == NULL)
		return (wp->eof + 2);		/* Siehe search.c */
	forw = *bp++ == '/';

	if (*bp == '^')
/*		bp++;*/
		*bp = '\n';

	endm = FALSE;
	if (bp[strlen(bp)-1] == '$') {
		bp[strlen(bp)-1] = '\n';
		endm = TRUE;
	}

	omagic = wp->magic;
	wp->magic = FALSE;
	if (forw) {
		pos = search(wp, opos, UC bp, strlen(bp), 0);

		if (pos > wp->eof && opos != 0)
			pos = search(wp, (epos_t)0, UC bp, strlen(bp), 0);
	} else {
		pos = reverse(wp, opos, UC bp, strlen(bp), 0);

		if (pos > wp->eof && opos != 0L)
			pos = reverse(wp, wp->eof, UC bp, strlen(bp), 0);
	}
	wp->magic = omagic;
	if (pos > wp->eof) {
		not_found(wp);
		return (pos);
	}

	pos = revline(wp, pos, endm ? (ecnt_t)2 : (ecnt_t)1);
	return (pos);
}

typedef struct tstack {
	struct tstack 	*ts_next;
	epos_t		ts_pos;
	int		ts_col;
	char		*ts_name;
} tstack;

LOCAL	tstack	*tstackp;

/*
 * Low level push tag stack routine
 */
LOCAL void
ts_push(wp)
	ewin_t	*wp;
{
	register tstack	*tsp;

	if ((tsp = (tstack *)malloc(sizeof (tstack))) == 0)
		return;
	if ((tsp->ts_name = (char *)malloc(strlen(C wp->curfile)+1)) == 0) {
		free(tsp);
		return;
	}
	strcpy(tsp->ts_name, C wp->curfile);
	tsp->ts_pos = wp->dot;
	tsp->ts_col = wp->column;
	tsp->ts_next = tstackp;
	tstackp = tsp;
}

/*
 * Low level pop tag stack routine
 */
LOCAL void
ts_pop()
{
	register tstack	*tsp;

	if (tstackp == 0)
		return;
	tsp = tstackp->ts_next;
	free(tstackp->ts_name);
	free(tstackp);
	tstackp = tsp;
}

/*
 * Go to tag found on current cursor position
 */
EXPORT void
vtag(wp)
	ewin_t	*wp;
{
	Uchar	tbuf[512];
	epos_t	pos;
	epos_t	len;
	BOOL	omagic;
extern	Uchar	notinword[];

	omagic = wp->magic;
	wp->magic = TRUE;
	pos = search(wp, wp->dot, notinword, strlen(C notinword), 0);
	wp->magic = omagic;

	if ((len = --pos - wp->dot) > 512 - 1) {
		writeerr(wp, "Word too long");
	} else {
		extract(wp, wp->dot, tbuf, (int)len);
/*		writeerr(wp, "'%s'", tbuf); sleep(1);*/
		gototag(wp, tbuf);
	}
}

/*
 * Go to named tag
 */
EXPORT void
gototag(wp, tag)
	ewin_t	*wp;
	Uchar	*tag;
{
	epos_t	pos;
	BOOL	newfile = FALSE;

	switch (gettag(&tag)) {

	case -1: writeerr(wp, "No such Tag '%s'", tag); return;
	case  0: write_errno(wp, "Can't open tags"); return;
	}
	ts_push(wp);
/*	writeerr(wp, "%s", tag); sleep(1);*/
	if (!streql(C tag, C wp->curfile)) {
		if (!change_file(wp, tag)) {
			ts_pop();
			return;
		}
		newfile = TRUE;
	}
	if ((pos = searchtag(wp, wp->dot)) <= wp->eof)
		wp->dot = pos;
	if (newfile)
		newwindow(wp);
}

/*
 * Pop the tag stack
 */
EXPORT void
vrtag(wp)
	ewin_t	*wp;
{
	BOOL	newfile = FALSE;

	if (tstackp == 0) {
		writeerr(wp, "Stack empty");
		return;
	}
	if (!streql(tstackp->ts_name, C wp->curfile)) {
		if (!change_file(wp, UC tstackp->ts_name)) {
			ts_pop();
			return;
		}
		newfile = TRUE;
	}
	wp->dot = tstackp->ts_pos;
	wp->column = tstackp->ts_col;
	ts_pop();
	if (newfile)
		newwindow(wp);
}
#endif

/*
 * modified version of breakline
 * --> array[len] contains rest of line
 */
/*
 * break up a line into fields
 * returns the # of tokens found (>= 1)
 * array[found ... len] points to '\0'
 */

LOCAL int
l_breakline(linebuf, delim, array, len)
		char	*linebuf;	/* source line, */
	register char	delim;		/* broken at every delimiter, */
	register char	*array[];	/* and returned in these */
	register int	len;		/* the (maximum) number of pointers */
{
	register char	*bp = linebuf;
	register char	*ep;
	register int	i;
	register int	found;

	for (i = 0, found = 1; i < len; i++) {
		for (ep = bp; *ep != '\0' && *ep != delim; ep++)
			;

		array[i] = bp;
		if (*ep == delim) {
			*ep++ = '\0';
			found++;
		}
		bp = ep;
	}
	array[i] = bp;
	return (found);
}
