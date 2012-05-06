/* @(#)io.c	1.38 12/04/24 Copyright 1984-2012 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)io.c	1.38 12/04/24 Copyright 1984-2012 J. Schilling";
#endif
/*
 *	Low level routines for Input from keyboard and output to screen.
 *
 *	Copyright (c) 1984-2012 J. Schilling
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

/*
 * All usual input of the editor is done in edit() which uses gchar() to get
 * the next mapped and macro expanded character. If for some reason a hard EOF
 * condition is reached, gchar() exits.
 *
 * To be able to recover a crashed edit session, a recover protocol file is
 * maintained by the low level input routine getnextc().
 *
 * To improve the output, all outout is buffered. This usually done by our
 * private buffering routines (if BFSIZE) is defined.
 *
 * The following output routines are available:
 *
 *	putoutchar	- for buffered simple output, used by very few funtions
 *			  that don't want side effects (e.g. terminal.c)
 *	addchar		- for buffered output that maintaines cursor posiotion
 *			  and currectly handles alternate video
 *			  The alternate video buffer is currently too small
 *			  to efficiently handle more than printing the mark.
 *	output		- string version of addchar()
 *	onmark		- to start alternalte video
 *	offmark		- to end alternalte video
 *	printfield	- used only to output the fields of the system line
 *	printstring	- prints a non null terminated string. Used only by the
 *			  command line module.
 *	ringbell	- ring the bell on screen
 *
 */

#include "ved.h"
#include "terminal.h"
#include <schily/setjmp.h>
#include <schily/jmpdefs.h>
#include <schily/termios.h>		/* For WIN-DOS test and USE_GETCH */
#include <schily/errno.h>

#define	RECOVERBUFSIZE	512
#define	RECOVERNAMESIZE	FNAMESIZE	/* Must be equal */
#define	PROTBUFSIZE	512
#define	PROTMARGIN	8	/* Nach PROTMARGIN Änderungen erfolgt sync */

typedef	union {
		Uchar	r_buf[FNAMESIZE+RECOVERBUFSIZE];
		struct r_header {
			Uchar	r_name[RECOVERNAMESIZE];
#ifdef	OLDPROT
			Uchar	r_pos[12];
			Uchar	r_col[12];
#else
			Uchar	r_pos[22];	/* enough for 64 bits */
			Uchar	r_col[22];	/* enough for 64 bits */
			Uchar	r_version[12];	/* e.g. ved-1.5	    */
#endif
		} r_head;
} _RBUF, *RBUF;

LOCAL	FILE	*protfile;		/* FILE * of current protocol file   */
extern	Uchar	protname[TMPNSIZE];	/* file name of current protocol file */
LOCAL	Uchar	protbuf[PROTBUFSIZE];	/* Buffer for protocol		    */
LOCAL	Uchar	*protp;			/* Actual protocol buffer write ptr */

LOCAL	FILE	*rec_file;		/* FILE * of current recover file   */
LOCAL	Uchar	*rec_name;		/* file name of current recover file */

EXPORT	BOOL	markon;		/* if true: we are just printing the mark   */
LOCAL	Uchar	Markbuffer[128]; /* temp space for alt video printing	    */
LOCAL	Uchar	*markbuf;	/* write ptr for printing alt video	    */

EXPORT	iobuf_t	_bb;

EXPORT	Uchar	gchar		__PR((ewin_t *wp));
EXPORT	Uchar	nigchar		__PR((ewin_t *wp));
LOCAL	int	inchar		__PR((ewin_t *wp));
EXPORT	int	getnextc	__PR((ewin_t *wp));
EXPORT	int	nigetnextc	__PR((ewin_t *wp));
EXPORT	void	flushprot	__PR((ewin_t *wp));
EXPORT	void	deleteprot	__PR((void));
EXPORT	void	newprot		__PR((ewin_t *wp));
EXPORT	void	openrecoverfile	__PR((ewin_t *wp, char *name));
EXPORT	Uchar*	getrecoverfile	__PR((epos_t *posp, int *colp));
EXPORT	int	putoutchar	__PR((int c));
LOCAL	void	pchar		__PR((int c));
EXPORT	void	addchar		__PR((int c));
EXPORT	void	onmark		__PR((void));
EXPORT	void	offmark		__PR((void));
EXPORT	void	addstr		__PR((Uchar *s));
EXPORT	void	output		__PR((Uchar *s));
EXPORT	void	printfield	__PR((Uchar *str, int len));
EXPORT	void	printstring	__PR((Uchar *str, int nchars));
EXPORT	void	ringbell	__PR((void));
EXPORT	int	_bflush		__PR((int c));
EXPORT	int	_bufflush	__PR((void));

/*---------------------------------------------------------------------------
|
| Input routines
|
+---------------------------------------------------------------------------*/

/*
 * Read the next character from the terminal (stdin). Exit on read error or EOF
 * Used by macro.c (for internal use) and the only 'real' user edit().
 * Expands the input first by the mapper and then by the macro package.
 */
EXPORT Uchar
gchar(wp)
	ewin_t	*wp;
{
	int	c;

	if (mflag == 0) {
		c = inchar(wp);
	} else if ((c = gmacro()) == 0) {
		c = inchar(wp);
	}
	if (c >= 0)
		return ((Uchar)c);
	eexit(wp);
	exit(0);		/* No Return */
	return (0);		/* Keep lint happy */
}

EXPORT	jmps_t	*sjp;
EXPORT	BOOL	interrupted;
extern	int	intrchar;

/*
 * Non-interruptable version of gchar() used by command line input and all
 * other places whre only one additional character needs to be read.
 * Catches the interrupt and maps the interrupt character back
 * to a usable input character.
 */
EXPORT Uchar
nigchar(wp)
	ewin_t	*wp;
{
static	jmps_t	gcjmp;
	jmps_t	*savjp = sjp;
	Uchar	c;

	if (setjmp(gcjmp.jb)) {
		if (intrchar > 0)
			interrupted++;
	} else {
		sjp = &gcjmp;
	}
	c = gchar(wp);
	sjp = savjp;
	return (c);
}

#include "map.h"
/*
 * Internal function used by gchar() to get the next character from
 * mapped input stream maintained by map.c Input is either taken from
 * the mapper outpout or from terminal inpout (see explanation im map.c).
 */
LOCAL int
inchar(wp)
	ewin_t	*wp;
/*int nextc()*/
{
	register int	c;

	if (!mapflag) {
#ifdef MAPESC
		if ((c = mapgetc()) == mapesc)
			c = mapgetc();
		else if (rmap(c))
#else
		c = mapgetc(wp);
		if (c < 0)
			return (c);
		if (rmap(wp, c))
#endif
			c = gmap();
	} else if ((c = gmap()) == 0) {
/*		return (nextc());*/
		return (inchar(wp));
	}
	return (c);
}

LOCAL	long	pmodflg;

/*
 * Low level function to read the next character from either the input
 * terminal or the recover protocol file of a crashed edit session.
 * This function is only used by map.c to get the next character.
 */
EXPORT int
getnextc(wp)
	ewin_t	*wp;
/*int inchar()*/
{
	int	c;

	if (recover) {
		c = getc(rec_file);
		if (c == EOF) {
			fclose(rec_file);
			recover = 0;
			return (inchar(wp));
		} else {
			return (c);
		}
	} else {
		if (interrupted) {
			interrupted = FALSE;
			c = intrchar;
		} else {
#ifdef	USE_GETCH
			c = getch();	/* DOS console input */
#else
/*#define	USE_GETCHAR*/
#ifdef	USE_GETCHAR
			/*
			 * Cannot use getchar() because of
			 * a bug in Linux stdio that repeates
			 * the last character after receiving a signal.
			 */
			while ((c = getchar()) < 0) {
				if (geterrno() != EINTR)
					break;
				clearerr(stdin);
			}
#else
			Uchar cc;

			while ((c = read(STDIN_FILENO, &cc, 1)) < 0) {
				c = -1;
				if (geterrno() != EINTR)
					break;
			}
			if (c == 0)
				c = -1;
			else
				c = cc;
#endif
#endif	/* USE_GETCH */
		}
		*protp++ = (Uchar) c;
		if (wp->modflg - pmodflg >= PROTMARGIN || protp >= &protbuf[PROTBUFSIZE])
			flushprot(wp);
		return (c);
	}
}

/*
 * Non-interruptable version of nigetnextc() used by map.c
 * Catches the interrupt and maps the interrupt character back
 * to a usable input character.
 */
EXPORT int
nigetnextc(wp)
	ewin_t	*wp;
{
static	jmps_t	nxjmp;
	jmps_t	*savjp = sjp;
	int	c;

	if (setjmp(nxjmp.jb)) {
		if (intrchar > 0)
			interrupted++;
	} else {
		sjp = &nxjmp;
	}
	c = getnextc(wp);
	sjp = savjp;
	return (c);
}

/*
 * Flush the recover protocol file
 */
EXPORT void
flushprot(wp)
	ewin_t	*wp;
{
	filewrite(protfile, C protbuf, protp - protbuf);
	fflush(protfile);
	protp = protbuf;
	pmodflg = wp->modflg;
}

/*
 * Close and delete the current recover protocol file
 */
EXPORT void
deleteprot()
{
	if (protfile)
		fclose(protfile);
	if (protname[0] != '\0')
		unlink(C protname);
}

/*
 * Open/create a new recover protocol file.
 */
EXPORT void
newprot(wp)
	ewin_t	*wp;
{
		_RBUF	nbuf;
	register Uchar	*rbuf;
	register int	i;

	for (i = 0, rbuf = (Uchar *) &nbuf; i < sizeof (nbuf); i++)
		*rbuf++ = '\0';
	if (strlen(C wp->curfile) > ((unsigned)(RECOVERNAMESIZE - 1)))
		writeerr(wp, "FILE NAME TO LONG");
	sprintf(C nbuf.r_head.r_name, "%.*s", RECOVERNAMESIZE - 1, wp->curfile);
#ifdef	OLDPROT
	sprintf(C nbuf.r_head.r_pos, "%11ld", wp->dot);
	sprintf(C nbuf.r_head.r_col, "%11d", wp->column);
#else
	js_snprintf(C nbuf.r_head.r_pos, sizeof (nbuf.r_head.r_pos),
						"%21lld", (Llong)wp->dot);
	js_snprintf(C nbuf.r_head.r_col, sizeof (nbuf.r_head.r_col),
						"%21lld", (Llong)wp->column);
	js_snprintf(C nbuf.r_head.r_version, sizeof (nbuf.r_head.r_version),
						"ved-%s", ved_version);
#endif

	if (protfile) fclose(protfile);
	protfile = tmpfopen(wp, protname, "cwtub");
	filewrite(protfile, C &nbuf, sizeof (nbuf));
	protp = protbuf;
}

/*
 * Open the recover protocol file for read access.
 */
EXPORT void
openrecoverfile(wp, name)
	ewin_t	*wp;
	char	*name;
{
	rec_name = UC name;
	rec_file = opencomerr(wp, rec_name, "rb");
}

/*
 * Get the name of the file to recover as well as the file offset and
 * cursor column at start of the crashed edit session.
 */
EXPORT Uchar *
getrecoverfile(posp, colp)
	epos_t	*posp;
	int	*colp;
{
	_RBUF	rbuf;
	char	*p;
	char	vbuf[32];
	Llong	ll;

	fileread(rec_file, C &rbuf, sizeof (rbuf));

	strncpy(C curfname, C rbuf.r_head.r_name, sizeof (curfname));
	curfname[sizeof (curfname)-1] = '\0';

#ifdef	OLDPROT
	astol(C rbuf.r_head.r_pos, posp);
	astoi(C rbuf.r_head.r_col, colp);
#else
	p = astoll(C rbuf.r_head.r_pos, &ll);
	*posp = ll;
	astoll(++p, &ll);
	*colp = ll;
	js_snprintf(vbuf, sizeof (vbuf), "ved-%s", ved_version);
	if (!streql(vbuf, C rbuf.r_head.r_version)) {
		errmsgno(EX_BAD, "Warning: recoverfile is from '%s', this is '%s'.\n",
				C rbuf.r_head.r_version, vbuf);
		sleep(1);
	}
#endif

	return (curfname);
}


/*---------------------------------------------------------------------------
|
| Output routines
|
+---------------------------------------------------------------------------*/

/*
 * A putchar that is callable via function pointers.
 * Does not maintain 'cpos' values.
 */
EXPORT int
putoutchar(c)
	Uchar	c;
{
	return (putchar(c));
}

#define	_pchar(c)	(((c) == '\n'?\
				(putchar('\r'), cpos.vp++, cpos.hp = 0) \
			:\
				cpos.hp++), \
			putchar(c))

/*
 * Output a character and maintain 'cpos' (cpos.hp and cpos.vp) values.
 * pchar() should only be called from functions that deal with cursor
 * positioning. Other functions use putoutchar() instead.
 * putchar() is usually not the standard c-library/stdio character output
 * macro. It uses our private increased buffering instead.
 */
LOCAL void
pchar(c)
	Uchar	c;
{
#ifdef	_pchar
	_pchar(c);
#else
	if (c == '\n') {
		putchar('\r');
		cpos.vp++;	/* we are at the beginning of the next line */
		cpos.hp = 0;
	} else {
		cpos.hp++;
	}
	putchar(c);
#endif
}

/*
 * External interface to pchar() that handles alternate video properly
 */
EXPORT void
addchar(c)
	Uchar	c;
{
	if (markon) {
		if (c == '\n') {
			cpos.vp++;
			cpos.hp = 0;
		} else {
			cpos.hp++;
		}
		*markbuf++ = c;
		if (markbuf >= Markbuffer + sizeof (Markbuffer) - 1) {
			offmark();
			onmark();
		}
	} else {
#ifdef	_pchar
		_pchar(c);
#else
		pchar(c);
#endif
	}
}

/*
 * Call this to start writing in aternate video
 */
EXPORT void
onmark()
{
	if (! f_alternate_video)
		return;
	markbuf = Markbuffer;
	markon = 1;
}

/*
 * Call this to stop writing in aternate video and flush the altvideo buffer
 */
EXPORT void
offmark()
{
	if (! markon)
		return;
	*markbuf = '\0';
	WRITE_ALT(C Markbuffer);
	markon = 0;
}

/*
 * String interface for addchar()
 */
#ifdef	__used__
EXPORT void
addstr(s)
	register Uchar	*s;
{
	while (*s) {
		addchar(*s++);
	}
}
#endif

/*
 * String interface for pchar()
 */
EXPORT void
output(s)
	register Uchar	*s;
{
	while (*s) {
		pchar(*s++);
	}
}

/*
 * Print a (null terminated) string.
 * Use the character table to expand the string.
 * Use pchar() to output the character to maintain 'cpos' values.
 * Print exactly 'len' "real" characters by either truncating the string or
 * padding with space characters.
 */
EXPORT void
printfield(str, len)
	register Uchar	*str;
	register int	len;
{
	extern   Uchar	*ctab[];
	register Uchar	*s;
	register Uchar	**rctab = ctab;

	while (len > 0)
		if (*str) {
			s = rctab[*str++];
			while (*s && --len >= 0)
				pchar(*s++);
		} else {
			pchar((Uchar) ' ');
			len--;
		}
}

/*
 * Print a (non null terminated) string.
 * Use the character table to expand the string.
 * Use pchar() to output the character to maintain 'cpos' values.
 */
EXPORT void
printstring(str, len)
	register Uchar	*str;
	register int	len;
{
	extern   Uchar	*ctab[];
	register Uchar	*s;
	register Uchar	**rctab = ctab;

	while (--len >= 0) {
		s = rctab[*str++];
		while (*s)
			pchar(*s++);
	}
}

/*
 * Ring the bell on screen
 */
EXPORT void
ringbell()
{
	if (streql(getenv("BEEP"), "off"))
		return;
	putoutchar(7);
}

#ifdef	BFSIZE
/*
 * Flush our private outpout buffer, take next character as arg
 */
EXPORT int
_bflush(c)
	Uchar	c;
{
	if (_bb._ptr)
		filewrite(stdout, C _bb._buf, BFSIZE - ++_bb._count);
	_bb._ptr = _bb._buf;
	_bb._count = BFSIZE - 1;
	*_bb._ptr++ = c;
	return (0);
}

/*
 * Flush our private outpout buffer (no arg version)
 */
EXPORT int
_bufflush()
{
	if (_bb._ptr)
		filewrite(stdout, C _bb._buf, BFSIZE - _bb._count);
	_bb._ptr = _bb._buf;
	_bb._count = BFSIZE;
	return (fflush(stdout));
}
#endif
