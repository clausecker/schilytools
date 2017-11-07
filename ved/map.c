/* @(#)map.c	1.39 17/11/05 Copyright 1986-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)map.c	1.39 17/11/05 Copyright 1986-2017 J. Schilling";
#endif
/*
 *	The map package for BSH & VED
 *
 *	Copyright (c) 1986-2017 J. Schilling
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

/*
 * If a map is found, it is loaded into 'mapstr' and then read from 'mapstr'.
 * No recursion is allowed on maps.
 *
 * The following external routines are available:
 *
 *	map_init	- Init the package and load the map file.
 *	rxmap		- Look for a map and put it into 'mapstr'.
 *			  Also sets 'mapflag' as an intication that 'gmap()'
 *			  should be used to read further characters.
 *			  The macro 'rmap()' should be used in favor of rxmap()
 *	gmap		- Get the next character from the macro string.
 *			  Returns '0' and resets 'mapflag' at the end of a
 *			  translation.
 *	mapgetc		- Used to read characters if no mapping is loaded.
 *	remap		- Set the 'mp_init' flag as a notice that map_init()
 *			  should be called.
 *	add_map		- Add a new map translation
 *	del_map		- Delete a map translation
 *
 * Uses getnextc() to read characters and tdecode to decode escape sequences.
 *
 * The global 'mapflag' is used to know whether the input should be
 * taken from the map string by calling 'gmap()' in favour of 'mapgetc()'.
 */

#include <schily/stdio.h>

#ifdef	BSH
#include "bsh.h"
#include "str.h"
#include "strsubs.h"
#else
#include <schily/standard.h>
#include "ved.h"
#endif

#include <schily/stdlib.h>
#include <schily/string.h>
#include "map.h"
#include "ctype.h"
#include <schily/termcap.h>
#include <schily/errno.h>

#ifndef	BSH
#define	INTERACTIVE
#define	strbeg(x, y)	(strstr((y), (x)) == (y))

char	slash[] = "/";
char	mapname[] = ".vedmap";
char	for_read[] = "rb";
/*
 * Use non-interruptable version
 */
#define	getnextc	nigetnextc

#else
#define	Uchar	unsigned char
#define	UC	(unsigned char *)
#endif

#ifdef	INTERACTIVE

#define	M_NAMELEN	(unsigned)16
#define	M_STRINGLEN	(unsigned)128

typedef struct {
		int	st_cnt;
		Uchar	*st_bp;
		Uchar	st_buf[M_NAMELEN + 1];
} maps_t;

typedef struct m_map {
	struct	m_map	*m_next;
	char		m_from[M_NAMELEN + 1];
	char		m_to[M_STRINGLEN + 1];
	char		*m_comment;
} smap_t;

EXPORT	Uchar	maptab[256];
EXPORT	int	mapflag;
EXPORT	BOOL	mp_init		= TRUE;

LOCAL	char	*mapstr;

LOCAL	smap_t	*first_map;
LOCAL	maps_t	map_str;

LOCAL	void	init_mapstream	__PR((void));
#ifdef	BSH
EXPORT	int	mapgetc		__PR((void));
#else
EXPORT	int	mapgetc		__PR((ewin_t *wp));
#endif
LOCAL	void	pushmap		__PR((char *sp, int n));
EXPORT	void	map_init	__PR((void));
#ifdef	BSH
EXPORT	int	rxmap		__PR((int c));
#else
EXPORT	int	rxmap		__PR((ewin_t *wp, int c));
#endif
EXPORT	int	gmap		__PR((void));
EXPORT	void	remap		__PR((void));
EXPORT	BOOL	add_map		__PR((char *from, char *to, char *comment));
EXPORT	BOOL	del_map		__PR((char *from));
LOCAL	BOOL	_add_map	__PR((Uchar *mn, Uchar *ms, char *comment));
LOCAL	BOOL	_del_map	__PR((char *mn));
#ifdef	BSH
EXPORT	void	list_map	__PR((FILE *f));
LOCAL	char	*get_map	__PR((int c));
#else
EXPORT	void	list_map	__PR((ewin_t *wp));
LOCAL	char	*get_map	__PR((ewin_t *wp, int c));
#endif
LOCAL	void	init_cursor_maps __PR((void));
#ifndef	BSH
EXPORT	void	init_fk_maps	__PR((void));
LOCAL	char	*pretty_string	__PR((Uchar *s));
#endif

/*
 * Initialize the intermediate character stack.
 * This character stack is used to store already read characters
 * so that thay are not lost after we discovered that there is
 * no map that starts with a specific sequence.
 * This character stack handles null bytes correctly.
 */
LOCAL void
init_mapstream()
{
	map_str.st_cnt = 0;
	*(map_str.st_bp = map_str.st_buf) = '\0';
}

/*
 * Get the next character (either from low level input or from the
 * intermediate character stack).
 */
EXPORT int
#ifdef	BSH
mapgetc()
#else
mapgetc(wp)
	ewin_t	*wp;
#endif
{
	if (map_str.st_cnt > 0) {
		map_str.st_cnt--;
		return (*map_str.st_bp++);
	} else {
#ifdef	BSH
		return (getnextc());
#else
		return (getnextc(wp));
#endif
	}
}

/*
 * Push a sequence of characters on the intermadiate character stack.
 * These charcaters have been read while following a map start sequence.
 */
LOCAL void
pushmap(sp, n)
	char	*sp;
	int	n;
{
	register int	i;
	register char	*p1;
	register char	*p2;

	/*
	 * Move the old contents to the proper place
	 */
	i = map_str.st_cnt;
	p1 = (char *)map_str.st_bp;
	p2 = (char *)&map_str.st_buf[n];
	if (p1 != p2) {
		while (--i >= 0)
			*p2++ = *p1++;
	}

	/*
	 * Insert new stuff before old contents
	 */
	i = n;
	p2 = (char *)map_str.st_buf;
	p1 = sp;
	while (--i >= 0)
		*p2++ = *p1++;
	map_str.st_cnt += n;
	map_str.st_bp = map_str.st_buf;
}

/*
 * Initialize the map package and load the map file.
 */
EXPORT void
map_init()
{
#define	BUF_SIZE	8192
	register FILE	*f;
		char	mapfname[512];
		char	linebuf[BUF_SIZE+1];	/* + space for null byte */
		char	*array[3];
		char	*home;
	register char	**ap;
	register char	*lp;
	register int	amt;

	home = myhome();
	if (home != NULL) {
		snprintf(mapfname, sizeof (mapfname), "%s%s%s",
						myhome(), slash, mapname);
	} else {
		strcpy(mapfname, mapname);
	}


#ifdef	BSH
	/*
	 * Der ved kann z.Zt. noch kein map/remap nach der
	 * Initialisierung.
	 *
	 */
	while (first_map)
		_del_map(first_map->m_from);
#endif
	mp_init = FALSE;
	init_mapstream();
	init_cursor_maps();
	if ((f = fileopen(mapfname, for_read)) == (FILE *)NULL) {
		if (geterrno() == ENOENT)
			return;
#ifdef	BSH
		berror(ecantopen, mapfname, errstr(geterrno()));
#else
		errmsg("Cannot open '%s'.\n", mapfname);
#endif
		return;
	}

	ap = array;
	lp = linebuf;
	amt = BUF_SIZE;
	linebuf[amt] = '\0';			/* Final null byte after buf */

	while ((amt = fileread(f, lp, amt)) > 0) {
		register char	*ep;

		amt += lp - linebuf;		/* Continue on whole rest */
		lp = linebuf;

	again:
		ep = strchr(lp, '\n');
		if (ep == NULL && lp > linebuf && amt >= BUF_SIZE) {
			/*
			 * If no '\n' could be found, we need to check whether
			 * we are in the middle of a line. If the buffer was
			 * not full, we are at EOF already.
			 */
			amt = amt - (lp - linebuf);	/* Unprocessed amt */
			movebytes(lp, linebuf, amt);	/* Move to start   */
			lp = &linebuf[amt];		/* Point past old  */
			amt = BUF_SIZE - amt;		/* Compute remaining */
			continue;			/* Fill up buf	   */
		}
		if (ep)					/* Buf contains '\n' */
			*ep = '\0';			/* so clear it	   */

		if (breakline(lp, ':', ap, 3) < 2)
			continue;
		if (ap[1][0] == '\0' && ap[2][0] == '*') {
			/*
			 * If the to string is empty and the comment starts
			 * with a '*', delete an unwanted mapping that may
			 * have been introduced from termcap.
			 */
			del_map(ap[0]);
		} else if (!add_map(ap[0], ap[1], ap[2])) {
			/*EMPTY*/
#ifdef	DEBUG_ALREADY
error("'%s' already defined.", pretty_string(UC ap[0]));
#endif
			;
		}

		if (ep) {			/* Found '\n', check rest */
			lp = &ep[1];
			if ((lp - linebuf) >= amt && amt < BUF_SIZE) /* EOF */
				break;
			goto again;
		} else {
			if (amt < BUF_SIZE)			    /* EOF */
				break;
			lp = linebuf;
			amt = BUF_SIZE;
		}
	}
	fclose(f);
}

/*
 * Look for map and load it into 'mapstr' if found.
 */
EXPORT int
#ifdef	BSH
rxmap(c)
#else
rxmap(wp, c)
	ewin_t	*wp;
#endif
	int	c;
{
	if (mapflag) {
#ifdef	BSH
		berror("\nMAP ABORTED");	/* only one map at a time */
#else
		writeerr(wp, "MAP ABORTED");	/* only one map at a time */
		/*
		 * May be flushed immediately by following characters.
		 */
		sleep(1);
#endif
		return (FALSE);
	}
#ifdef	BSH
	if ((mapstr = get_map(c)) == NULL)
#else
	if ((mapstr = get_map(wp, c)) == NULL)
#endif
		mapflag = 0;
	else
		mapflag++;
	return (mapflag);
}

/*
 * Get the next character from the map replacement string.
 */
EXPORT int
gmap()
{
	char	c;

	if ((c = *mapstr++) == 0)
		mapflag--;
	return ((Uchar)c);
}

/*
 * Set mp_init to force a call of map_init() to reload the map file.
 */
EXPORT void
remap()
{
	mp_init = TRUE;
}

/*
 * Add a mapping. Use tdecode() to decode escape sequences.
 */
EXPORT BOOL
add_map(from, to, comment)
	char	*from;
	char	*to;
	char	*comment;
{
	char	froms[M_NAMELEN + 1];
	char	tos[M_STRINGLEN + 1];
	char	*pf;
	char	*pt;

	if (strlen(from) > M_NAMELEN || strlen(to) > M_STRINGLEN)
		return (FALSE);

	pf = froms;
	pt = tos;
	return (_add_map(UC tdecode(from, &pf), UC tdecode(to, &pt), comment));
}

/*
 * Delete a mapping. Use tdecode() to decode escape sequences.
 */
EXPORT BOOL
del_map(from)
	char	*from;
{
	char	froms[M_NAMELEN + 1];
	char	*pf;

	if (strlen(from) > M_NAMELEN)
		return (FALSE);

	pf = froms;
	return (_del_map(tdecode(from, &pf)));
}

/*
 * Add a new map to the list of known maps.
 */
LOCAL BOOL
_add_map(mn, ms, comment)
	register	Uchar	*mn;
			Uchar	*ms;
			char	*comment;
{
	register	smap_t	*np;
	register	smap_t	*tn;
	register	smap_t	*last;
	register	int	cmp;

	if (streql((char *)mn, (char *)ms))
		return (FALSE);
	/*
	 * First create and init new map node.
	 */
	tn = (smap_t *)malloc(sizeof (*tn));
	if (tn == (smap_t *)NULL)
		return (FALSE);
#ifdef	__support_null__
	*movebytes((char *)mn, (char *)tn->m_from, M_NAMELEN) = '\0';
	*movebytes((char *)ms, (char *)tn->m_to, M_STRINGLEN) = '\0';
#else
	strlcpy((char *)tn->m_from, (char *)mn, sizeof (tn->m_from));
	strlcpy((char *)tn->m_to,   (char *)ms, sizeof (tn->m_to));
#endif
	if (comment) {
		tn->m_comment = malloc(strlen(comment)+1);
		if (tn->m_comment)
			strcpy(tn->m_comment, comment);
	} else {
		tn->m_comment = NULL;
	}
	tn->m_next = (smap_t *)NULL;

	if (++maptab[*mn] > 254) {		/* Too many Entrys */
		if (tn->m_comment)
			free(tn->m_comment);
		free((char *)tn);
		maptab[*mn]--;
		return (FALSE);
	}

	if (first_map == (smap_t *)NULL) {
		first_map = tn;
		return (TRUE);
	}

	/*
	 * Insert new map in order.
	 */
	np = last = first_map;
	for (; ; np = np->m_next) {
		if (np == (smap_t *)NULL) {
			/*
			 * Append to end of list
			 */
			last->m_next = tn;
			return (TRUE);
		}

		cmp = strcmp((char *)mn, np->m_from);

		if (cmp == 0) {
			/*
			 * Map is already defined
			 */
			if (tn->m_comment)
				free(tn->m_comment);
			free((char *)tn);
			maptab[*mn]--;
			return (FALSE);
		}
		if (cmp < 0) {
			if (first_map == np) {
				/*
				 * Make it the first in list.
				 */
				tn->m_next = first_map;
				first_map = tn;
				return (TRUE);
			} else {
				/*
				 * Insert in list
				 */
				last->m_next = tn;
				tn->m_next = np;
				return (TRUE);
			}
		}
		last = np;
	}
}


/*
 * Delete a map
 */
LOCAL BOOL
_del_map(mn)
	register	char	*mn;
{
	register	smap_t	*np = first_map;
	register	smap_t	*tn;

	if (streql(mn, np->m_from)) {
		first_map = np->m_next;
		if (np->m_comment)
			free(np->m_comment);
		free((char *)np);
		maptab[(Uchar) *mn]--;
		return (TRUE);
	}
	for (; ; np = np->m_next) {
		if (np->m_next == (smap_t *)NULL) {
#ifdef	BSH
			berror("'%s' not found", mn);
			ex_status = 1;
#else
#ifdef	__use_writerr_on_not_found__
			writeerr(wp, "'%s' not found", mn);
#else
			error("'%s' not found", mn);
#endif
#endif
			return (FALSE);
		}
		if (streql(mn, np->m_next->m_from)) {
			tn = np->m_next;
			np->m_next = np->m_next->m_next;
			if (tn->m_comment)
				free(tn->m_comment);
			free((char *)tn);
			maptab[(Uchar) *mn]--;
			return (TRUE);
		}
	}
}


/*
 * Lists all maps
 */
EXPORT void
#ifdef	BSH
list_map(f)
	register	FILE	*f;
#else
list_map(wp)
	ewin_t	*wp;
#endif
{
	register	smap_t	*np;

	for (np = first_map; np; np = np->m_next) {
#ifdef	BSH
#ifdef	LIB_SHEDIT
		if (*f == STDOUT_FILENO && isatty(*f)) {
#else
		if (f == stdout) {
#endif
			printf("%-16s ", pretty_string(UC np->m_from));
			printf("%-16s", pretty_string(UC np->m_to));
		} else {
			fprintf(f, "%-16s %-16s", np->m_from, np->m_to);
		}
		if (np->m_comment)
			fprintf(f, "%s", np->m_comment);
		fprintf(f, "\n");
#else
		printscreen(wp, "%-16s ", pretty_string(UC np->m_from));
		printscreen(wp, "%-16s", pretty_string(UC np->m_to));
		if (np->m_comment)
			printscreen(wp, " %s\n", np->m_comment);
		else
			printscreen(wp, "\n");
#endif
	}
}

/*
 * Do a lookup for a map.
 * Return the mapped string on success, else return NULL.
 */
LOCAL char *
#ifdef	BSH
get_map(c)
#else
get_map(wp, c)
	ewin_t	*wp;
#endif
	char	c;
{
			char	m_from[M_NAMELEN + 1];
	register	smap_t	*tn;
	register	int	i;
	register	char	*cp;
	register	char	*name;

#ifndef	BSH
#ifdef	GETMAP_DEBUG
writeerr("getm %d", c);
#endif
#endif
	cp = name = m_from;
	*cp++ = c;
	tn = first_map;
	for (i = 0; i < M_NAMELEN; i++) {
		*cp = '\0';
		for (; ; tn = tn->m_next) {
			if (tn == (smap_t *)NULL) {
				pushmap(&name[1], cp - &name[1]);
				return (NULL);
			}
#ifdef	GETMAP_DEBUG
cdbg("name '%s' from '%s' %d", name, pretty_string(tn->m_from), cp - name);
#endif
			if (strcmp(name, tn->m_from) == 0)
				return (tn->m_to);
			if (cmpbytes(name, tn->m_from, cp-name) >= (cp-name))
				break;
		}
#ifdef	GETMAP_DEBUG
cdbg("mapgetc()");
#endif
#ifdef	BSH
		*cp++ = mapgetc();	/* XXX EOF ??? */
#else
		*cp++ = mapgetc(wp);	/* XXX EOF ??? */
#endif
#ifndef	BSH
#ifdef	GETMAP_DEBUG
writeerr("mapg %d", cp[-1]);
#endif
#endif
	}
	return (NULL); /* XXX NOTREACHED ??? */
}

#ifndef BSH

/*
 * Initialize cursor mappings for ved. Tgetent has been called before.
 */
LOCAL void
init_cursor_maps()
{
	extern char	*KU;
	extern char	*KD;
	extern char	*KR;
	extern char	*KL;

	if (KU) {
		_add_map(UC KU, UC "", "Cursor up");
	} else {
		_add_map(UC "\33OA", UC "", "Cursor up");
		_add_map(UC "\33[A", UC "", "Cursor up");
	}
	if (KD) {
		_add_map(UC KD, UC "", "Cursor down");
	} else {
		_add_map(UC "\33OB", UC "", "Cursor down");
		_add_map(UC "\33[B", UC "", "Cursor down");
	}
	if (KR) {
		_add_map(UC KR, UC "", "Cursor forward");
	} else {
		_add_map(UC "\33OC", UC "", "Cursor forward");
		_add_map(UC "\33[C", UC "", "Cursor forward");
	}
	if (KL) {
		_add_map(UC KL, UC "", "Cursor left");
	} else {
		_add_map(UC "\33OD", UC "", "Cursor left");
		_add_map(UC "\33[D", UC "", "Cursor left");
	}
}

LOCAL struct fk_maps {
	char	*fk_tc;
	Uchar	*fk_map;
	char	*fk_comment;
} fk_maps[] = {
	{ "k0", 0,	0 },
	{ "k1", UC "", "Quit Editor  (F1)" },
	{ "k2", UC "", "Top of File  (F2)" },
	{ "k3", UC "", "Delete char  (F3)" },
	{ "k4", UC "", "Delete line  (F4)" },
	{ "k5", UC "", "Open line    (F5)" },
	{ "k6", UC "", "Cut line     (F6)" },
	{ "k7", UC "", "Paste        (F7)" },
	{ "k8", UC "", "Change buffer(F8)" },
	{ "k9", UC "", "Search down  (F9)" },
#ifdef	__coment__
	{ "k;", UC "^Z", "Re search    (F10)" },
/* XXX Real ^Z replaced by ^ Z to allow compilation on DOS/WNT */
#endif
	{ "k;", UC "\032", "Re search    (F10)" },

	{ "F1", UC "", "Get from     (F11)" },
	{ "F2", UC "", "Write to     (F12)" },

	{ "kA", UC "",	"Insert line" },
	{ "kD", UC "\177",	"Delete char" },
	{ "kb", UC "\177",	"Key Backspace -> Delete char" },
	{ "kE", UC "",	"Delete to eol" },
	{ "@7", UC "",	"Go to eol" },
	{ "kh", UC "",	"Go to sol" },
	{ "kL", UC "",	"Delete line" },
	{ "kN", UC "n",	"Page down"},
	{ "kP", UC "p",	"Page up"},
				/* \015 was ^M before Mac OS X */
	{ "kS",	UC "999999\015",	"Delete to end of screen" },

	{ 0, 0, 0},
};
#ifdef VI
{					/* Command mappings. */
	{"kA",    "O",	"insert line"},
	{"kD",    "x",	"delete character"},
	{"kd",    "j",	"cursor down"},
	{"kE",    "D",	"delete to eol"},
	{"kF", "\004",	"scroll down"},
	{"kH",    "$",	"go to eol"},
	{"kh",    "^",	"go to sol"},
	{"kI",    "i",	"insert at cursor"},
	{"kL",   "dd",	"delete line"},
	{"kl",    "h",	"cursor left"},
	{"kN", "\006",	"page down"},
	{"kP", "\002",	"page up"},
	{"kR", "\025",	"scroll up"},
	{"kS",	 "dG",	"delete to end of screen"},
	{"kr",    "l",	"cursor right"},
	{"ku",    "k",	"cursor up"},
	{NULL},
};
#endif

/*
 * Initialize function key mappings.
 */
EXPORT void
init_fk_maps()
{
	char	*p;
	struct fk_maps *mp;
	extern char	**tty_entry __PR((void));

	for (mp = fk_maps; mp->fk_tc; mp++) {
		if (mp->fk_map) {
			p = tgetstr(mp->fk_tc, tty_entry());
#ifdef	INIT_FP_MAPS_DEBUG
error("tc: '%s' map: '%s' %X\r\n", mp->fk_tc, p, *tty_entry());
#endif
			if (p == NULL) {
				if (mp->fk_tc[0] == 'k' &&
				    mp->fk_tc[1] == 'h') {
					p = "\33[7~";
				}
				if (mp->fk_tc[0] == '@' &&
				    mp->fk_tc[1] == '7') {
					p = "\33[8~";
				}
				if (mp->fk_tc[0] == 'k' &&
				    mp->fk_tc[1] == 'D') {
					p = "\33[3~";
				}
			}
			if (p)
				_add_map(UC p, mp->fk_map, mp->fk_comment);
		}
	}
}

/*
 * Make a string readable - it may contain comtrol characters.
 */
LOCAL char *
pretty_string(s)
	register Uchar	*s;
{
	static	 Uchar	buf[16];
	static	 Uchar	*str = 0;
	register Uchar	*s1  = 0;
	register int	len;

	if (str && str != buf)
		free(str);

	len = 3 *(unsigned)strlen((char *)s) + 1;
	if (len > sizeof (buf))
		s1 = str = (Uchar *)malloc(len);

	if (s1 == 0) {
		len = sizeof (buf);
		s1 = str = buf;
	}
	while (*s && --len > 0) {
		if (isprint(*s)) {
			*s1++ = *s++;
			continue;
		}
		if (*s & 0x80) {
			*s1++ = '~';
			len--;
		}
		if (*s != 127 && *s & 0x60) {
			*s1++ = *s++ & 0x7F;
		} else {
			*s1++ = '^';
			*s1++ = (*s++ & 0x7F) ^ 0100;
			len--;
		}
	}
	*s1 = '\0';
	return ((char *)str);
}

#else	/* This is for BSH */

/*
 * Initialize cursor mappings for bsh. Tgetent has not been called before.
 */
LOCAL void
init_cursor_maps()
{
		char	stbuf[1024];
		char	*sbp;
		char	*ku;
		char	*kd;
		char	*kr;
		char	*kl;
		char	*kh;
		char	*ke;
		char	*kD;
		char	*kb;
		char	*tname;
		char	**esav;
	extern	char	**environ;

	sbp = stbuf;

	/*
	 * Let getenv() simulate the behaviour of getcurenv().
	 * We need TERMCAP=, TERMPATH=, HOME= and TERM=.
	 * This allows us to use the same tgetent() for bsh and ved
	 * and we may use -lxtermcap
	 */
	esav = environ;
	environ = evarray;
	if ((tname = getcurenv(termname)) != NULL &&
					tgetent(NULL, tname) == 1) {
		ev_insert(concat(termcapname, eql, tcgetbuf(), (char *)NULL));
		ku = tgetstr("ku", &sbp);		/* Cursor up */
		kd = tgetstr("kd", &sbp);		/* Cursor down */
		kr = tgetstr("kr", &sbp);		/* Cursor forward */
		kl = tgetstr("kl", &sbp);		/* Cursor left */
		kh = tgetstr("kh", &sbp);		/* Cursor -> Home */
		ke = tgetstr("@7", &sbp);		/* Cursor -> End */
		kD = tgetstr("kD", &sbp);		/* Delete Character */
		kb = tgetstr("kb", &sbp);		/* Key Backspace */

		if (ku) {
			_add_map(UC ku, UC "", "Cursor up");
		} else {
			_add_map(UC "\33OA", UC "", "Cursor up");
			_add_map(UC "\33[A", UC "", "Cursor up");
		}
		if (kd) {
			_add_map(UC kd, UC "", "Cursor down");
		} else {
			_add_map(UC "\33OB", UC "", "Cursor down");
			_add_map(UC "\33[B", UC "", "Cursor down");
		}
		if (kr) {
			_add_map(UC kr, UC "", "Cursor forward");
		} else {
			_add_map(UC "\33OC", UC "", "Cursor forward");
			_add_map(UC "\33[C", UC "", "Cursor forward");
		}
		if (kl) {
			_add_map(UC kl, UC "", "Cursor left");
		} else {
			_add_map(UC "\33OD", UC "", "Cursor left");
			_add_map(UC "\33[D", UC "", "Cursor left");
		}

		if (kh)
			_add_map(UC kh, UC "", "Cursor Home");
		else
			_add_map(UC "\33[7~", UC "", "Cursor Home");
		if (ke)
			_add_map(UC ke, UC "", "Cursor End");
		else
			_add_map(UC "\33[8~", UC "", "Cursor End");
		if (kD)
			_add_map(UC kD, UC "\177", "Delete Char");
		else
			_add_map(UC "\33[3~", UC "\177", "Delete Char");

		if (kb)
			_add_map(UC kb, UC "\177", "Key Backspace -> Delete Char");
	} else {
		/*
		 * ANSI Cursor mapping in "edit mode".
		 */
		_add_map(UC "\33OA", UC "", "Cursor up");
		_add_map(UC "\33OB", UC "", "Cursor down");
		_add_map(UC "\33OC", UC "", "Cursor forward");
		_add_map(UC "\33OD", UC "", "Cursor left");
		/*
		 * ANSI Cursor mapping in "default mode".
		 */
		_add_map(UC "\33[A", UC "", "Cursor up");
		_add_map(UC "\33[B", UC "", "Cursor down");
		_add_map(UC "\33[C", UC "", "Cursor forward");
		_add_map(UC "\33[D", UC "", "Cursor left");

		/*
		 * PC keyboard keys "Del", "Pos1", "End"
		 */
		_add_map(UC "\33[7~", UC "", "Cursor Home");
		_add_map(UC "\33[8~", UC "", "Cursor End");
		_add_map(UC "\33[3~", UC "\177", "Delete Char");
	}
	_add_map(UC "\33n", UC "\33",
				"Search down after clearing previous search");
	_add_map(UC "\33p", UC "\33",
				"Search up after clearing previous search");
	environ = esav;
}
#endif

#endif	/* INTERACTIVE */
