/* @(#)inputc.c	1.53 09/04/16 Copyright 1982, 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)inputc.c	1.53 09/04/16 Copyright 1982, 1984-2009 J. Schilling";
#endif
/*
 *	inputc.c
 *
 *	characterwise input for bsh
 *	line-editing
 *	history using cursor-block
 *
 *	This is the second implementation that has been integrated into bsh.
 *	It offeres exactly the same functionality as you could find in the
 *	first implementation that was integrated into bsh and has been finished
 *	around August 1984.
 *
 *	Both implementations are based on a simple shell prototype that I wrote
 *	in 1982 and 1983. This prototype only contained the editor and called
 *	shell commands via system().
 *
 *	Copyright (c) 1982, 1984-2009 J. Schilling
 *	This version was first coded August 1984 and rewritten 01/22/85
 *
 *	Exported functions:
 *		getinfile()	Return the current input FILE *
 *		get_histlen()	Return the max. number of history entries
 *		chghistory()	Change history size to string argument
 *		init_input()	Init editor data structures
 *		getnextc()	Noninterruptable getc() -> export to map.c
 *		nextc()		Read mapped input from map.c (Input to inputc.c)
 *		space()		Output n spaces
 *		append_line()	Append a line into history (for hashcmd.c #lh)
 *		match_hist()	Return matched history line for csh: !line
 *		make_line()	Read a line from a file anr return allocated string
 *		get_line()	-> Write Prompt, then return edited line
 *		put_history()	Put out history to FILE *
 *		save_history()	Save the history to ~/.history
 *		read_init_history()	Initialize the history from ~/.history
 *		readhistory()	Read history from a file
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
#include "bsh.h"
#include "node.h"
#include "str.h"
#include "strsubs.h"
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/fcntl.h>
#include <schily/patmatch.h>
#include "ctype.h"
#undef	toint		/* Atari MiNT has this nonstandard definition */
#include "map.h"

/*#define	XDEBUG*/
#ifdef	XDEBUG		/* eXpand Debug */
#define	DO_DEBUG
#else
#endif

#ifdef	CWDEBUG		/* Clear word Debug */
#define	DO_DEBUG
#else
#endif

#ifdef INTERACTIVE

#define	BUFSIZE	133

typedef struct {
	int b_index;
	char b_buf[BUFSIZE];
} BUF;

#define	LINEQUANT	64
#define	BEGINLINE	1
#define	SHOW		2
#define	EXPAND		3
#define	CTRLD		4
#define	ENDLINE		5
#define	FORWARD		6
#define	BEEP		7
#define	BACKSPACE	8
#define	TAB		9
#define	DOWNWARD	14
#define	UPWARD		16
#define	RETYPE		18
#define	CTRLU		21
#define	CTRLW		23
#define	ESC		27
#define	QUOTECH		30
#define	UNDO		31
#define	BLANK		' '
#define	BACKSLASH	'\\'
#define	DELETE		127

#define	BYTEMASK	0xFF
#define	SEARCH		0x100
#define	RESTORE		0x200

#define	SEARCHUP	(SEARCH|UPWARD)
#define	SEARCHDOWN	(SEARCH|DOWNWARD)

typedef struct histptr {
	struct histptr	*h_prev,
			*h_next;
	char		*h_line;
	unsigned	h_len;
	unsigned	h_pos;
	unsigned char	h_flags;
} _HISTPTR, *HISTPTR;

#define	F_TMP	0x01		/* Claimed by a tmp pointer */

EXPORT	FILE	*getinfile	__PR((void));
EXPORT	int	get_histlen	__PR((void));
EXPORT	void	chghistory	__PR((char *cp));
LOCAL	void	changehistory	__PR((int n));
EXPORT	void	init_input	__PR((void));
EXPORT	int	getnextc	__PR((void));
EXPORT	int	nextc		__PR((void));
LOCAL	void	writec		__PR((int c));
LOCAL	void	writes		__PR((char *p));
LOCAL	void	prettyp		__PR((int c, BUF * b));
LOCAL	void	bflush		__PR((void));
LOCAL	void	pch		__PR((int c, BUF * bp));
LOCAL	void	putch		__PR((int c));
LOCAL	void	beep		__PR((void));
LOCAL	int	chlen		__PR((int c));
LOCAL	int	linelen		__PR((char *s));
LOCAL	int	linediff	__PR((char *a, char *e));
LOCAL	void	backspace	__PR((int n));
EXPORT	void	space		__PR((int n));
LOCAL	void	delete		__PR((char *p));
LOCAL	char	*clearword	__PR((char *lp, char *cp, unsigned int	*lenp));
LOCAL	void	clearline	__PR((char *lp, char *cp));
LOCAL	void	ins_char	__PR((char *cp, int c));
LOCAL	char	*ins_word	__PR((char *cp, char *s));
LOCAL	void	del_char	__PR((char *cp));
LOCAL	void	free_line	__PR((HISTPTR p));
LOCAL	HISTPTR	mktmp		__PR((void));
LOCAL	HISTPTR	hold_line	__PR((HISTPTR p));
LOCAL	void	unhold_line	__PR((HISTPTR p, HISTPTR op));
LOCAL	HISTPTR	remove_line	__PR((HISTPTR p));
EXPORT	void	append_line	__PR((char *linep, unsigned int len, unsigned int pos));
LOCAL	void	append_hline	__PR((char *linep, unsigned int len));
LOCAL	void	move_to_end	__PR((HISTPTR p));
LOCAL	void	stripout	__PR((void));
LOCAL	HISTPTR match_input	__PR((HISTPTR cur_line, HISTPTR tmp_line, BOOL up));
EXPORT	char	*match_hist	__PR((char *pattern));
LOCAL	HISTPTR match		__PR((HISTPTR cur_line, char *pattern, BOOL up));
LOCAL	int	edit_line	__PR((HISTPTR cur_line));
LOCAL	void	redisp		__PR((char *lp, char *cp));
LOCAL	char	*insert		__PR((char *cp, char *s, unsigned int *lenp));
LOCAL	char	*undo_del	__PR((char *lp, char *cp, unsigned int *lenp));
LOCAL	char	*xpstr		__PR((char *cp));
LOCAL	char	*xp_files	__PR((char *lp, char *cp, BOOL show, int *multip));
LOCAL	char	*exp_files	__PR((char **lpp, char *cp, unsigned int *lenp, unsigned int *maxlenp, int *multip));
LOCAL	void	show_files	__PR((char *lp, char *cp));
LOCAL	int	get_request	__PR((void));
LOCAL	char	*esc_process	__PR((int xc, char *lp, char *cp, unsigned int *lenp));
LOCAL	char	*sget_line	__PR((void));
LOCAL	char	*iget_line	__PR((void));
EXPORT	char	*make_line	__PR((int (*f)(FILE *), FILE * arg));
LOCAL	char	*fread_line	__PR((FILE * f));
EXPORT	char	*get_line	__PR((int n, FILE * f));
EXPORT	void	put_history	__PR((FILE * f, int intrflg));
EXPORT	void	save_history	__PR((int intrflg));
EXPORT	void	read_init_history	__PR((void));
EXPORT	void	readhistory	__PR((FILE * f));
LOCAL	void	term_init	__PR((void));
LOCAL	void	tty_init	__PR((void));
LOCAL	void	tty_term	__PR((void));
#ifdef	DO_DEBUG
LOCAL	void	cdbg		__PR((char *fmt, ...));
#endif

extern	pid_t	mypgrp;
extern	int	delim;
extern	int	prflg;
extern	int	ttyflg;
extern	int	mapflag;
extern	BOOL	mp_init;
extern	char	*inithome;
extern	BOOL	ins_mode;
extern	BOOL	i_should_echo;

LOCAL	FILE	*infile		= 0;
LOCAL	HISTPTR	first_line	= (HISTPTR) NULL;
LOCAL	HISTPTR	last_line	= (HISTPTR) NULL;
LOCAL	HISTPTR	rub_line	= (HISTPTR) NULL;
LOCAL	HISTPTR	del_line	= (HISTPTR) NULL;
LOCAL	char	*hfilename	= NULL;
LOCAL	char	*line_pointer	= NULL;
LOCAL	char	*iprompt	= NULL;
LOCAL	int	histlen		= 0;
LOCAL	int	no_lines	= 0;
LOCAL	BUF	buf		= {0};
LOCAL	char	mapesc		= '\0';
#ifdef	notneeded
LOCAL	int	eof;
#endif


/*
 * Return actual input FILE *
 */
EXPORT FILE *
getinfile()
{
	return (infile);
}


/*
 * Return the actual length of the history.
 */
EXPORT int
get_histlen()
{
	return (histlen);
}


/*
 * Read new value and change the length of the current history to this value.
 */
EXPORT void
chghistory(cp)
	char	*cp;
{
	int	n;

#ifdef DEBUG
	printf("chghistory ('%s')\r\n", cp);
#endif
	if (!toint(gstd, cp, &n) || n < 0) {
		berror("Bad value '%s' for %s.", cp, histname);
		return;
	}
	changehistory(n);
}


/*
 * Change the length of the current history.
 */
LOCAL void
changehistory(n)
	register int	n;
{
	while (n < no_lines)
		remove_line(first_line);
	histlen = n;
}


/*
 * Init the history to the default size.
 */
EXPORT void
init_input()
{
	register char	*p;

#ifdef DEBUGX
	printf("init_input()\r\n");
#endif
	if ((p = getcurenv(histname)) == NULL) {
		p = "100";
		ev_insert(concat(histname, eql, p, (char *)NULL));
	}
	chghistory(p);
	rub_line = mktmp();
	del_line = mktmp();
}


/*
 * Non interruptable version of getc() from stdio lib.
 */
EXPORT int
getnextc()
{
	int	c;
	int	errs = 0;

again:
	c = getc(infile);
	if (c < 0 && geterrno() == EINTR) {
		clearerr(infile);
		if (++errs < 10)
			goto again;
	}

#if	defined(F_GETFL) && defined(O_NONBLOCK) && (defined(EAGAIN) || defined(EWOULDBLOCK))
#ifndef	EWOULDBLOCK			/* Not present on DJGPP */
#define	EWOULDBLOCK	EAGAIN
#endif
#ifndef	EAGAIN				/* Make it symmetric */
#define	EAGAIN		EWOULDBLOCK
#endif
	if (c < 0 && (geterrno() == EAGAIN || geterrno() == EWOULDBLOCK)) {
		int	fl;

		fl = fcntl(fdown(infile), F_GETFL, 0);
		fl &= ~O_NONBLOCK;
		fcntl(fdown(infile), F_SETFL, fl);
		clearerr(infile);
		if (++errs < 10)
			goto again;
	}
#endif
	return (c);
}


/*
 * Get next character from mapper.
 * This allows us to implement cursor key mappings.
 */
EXPORT int
nextc()
{
	register int	c;

	if (!mapflag) {
		if ((c = mapgetc()) == mapesc)
			c = mapgetc();
		else if (rmap(c))
			c = gmap();
	} else if ((c = gmap()) == 0) {
		c = nextc();
	}
	return (c);
}


/*
 * Write a single character to our tty
 */
LOCAL void
writec(c)
	char	c;
{
	prettyp(c, &buf);
}


/*
 * Write a string
 */
LOCAL void
writes(p)
	register char	*p;
{
	register BUF	*rb = &buf;

	while (*p)
		prettyp(*p++, rb);
	bflush();
}


/*
 * Write a single character in expanded (readable) form.
 * Non printable characters are frefixed by '~' if they
 * are beyond 0x7F and prefixed by '^' if they are a
 * control character.
 */
LOCAL void
prettyp(c, b)
	register char c;
	register BUF	*b;
{
	if (isprint((unsigned char)c) /*|| c == '\n'*/) {
		pch(c, b);
		return;
	}
	if (c & 0x80) {
		pch('~', b);
		prettyp(c & 0177, b);
	} else {
		pch('^', b);
		pch(c ^ 0100, b);
	}
}


/*
 * Flush our internal line buffering module.
 */
LOCAL void
bflush()
{
	register BUF *bp = &buf;

	(void) filewrite(stdout, bp->b_buf, bp->b_index);
	(void) fflush(stdout);
	bp->b_index = 0;
}


/*
 * Put a character into out line buffering module.
 */
LOCAL void
pch(c, bp)
	char		c;
	register BUF	*bp;
{
	if (bp->b_index >= BUFSIZE)
		bflush();
	bp->b_buf[bp->b_index++] = c;
}


/*
 * Simple putchar() replacement that uses our line buffering.
 */
LOCAL void
putch(c)
	char	c;
{
	pch(c, &buf);
}


/*
 * Ring the bell.
 */
LOCAL void
beep()
{
	char	*p = getcurenv("BEEP");

	if (p != NULL && streql(p, "off"))
		return;
	putch(BEEP);
	bflush();
}


/*
 * Compute the visible length of a character.
 */
LOCAL int
chlen(c)
	register char	c;
{
	if (isprint((unsigned char)c) /*|| c == '\n'*/)
		return (1);
	if (c & 0x80)
		return (2 + !isprint(c&0177));
	return (2);
}


/*
 * Compute the visible length of a string.
 */
LOCAL int
linelen(s)
	register char	*s;
{
	register int	len = 0;
	while (*s)
		len += chlen(*s++);
	return (len);
}


/*
 * Compute the visible difference of two characters in a string.
 */
LOCAL int
linediff(a, e)
	register char	*a;
	register char	*e;
{
	register int	diff = 0;
	while (*a && a < e)
		diff += chlen(*a++);
	return (diff);
}


/*
 * Back space cursor by 'n'.
 */
LOCAL void
backspace(n)
	register int	n;
{
	register BUF	*rb = &buf;

	while (n--)
		pch(BACKSPACE, rb);
	bflush();
}


/*
 * Forward space cursor by 'n'. Do this by writing spaces.
 */
EXPORT void
space(n)
	register int	n;
{
	register BUF	*rb = &buf;

	while (n--)
		pch(BLANK, rb);
	bflush();
}


/*
 * loescht bis Ende der Zeile
 */
LOCAL void
delete(p)
	register char	*p;
{
	register int	len = 0;

	while (*p)
		len += chlen(*p++);
	space(len);
	backspace(len);
}

/*
 * loescht ein Wort rueckwaerts wie im Terminaltreiber mit ^W
 */
LOCAL char *
clearword(lp, cp, lenp)
	register char	*lp;
	register char	*cp;
	unsigned	*lenp;
{
	register int	dl	= 0;		/* Displayed len of word */
	register int	wl;			/* Length of word in chars */
	register char	*p;
		char	*sp	= cp;

	if (cp == lp) {				/* At begin of line */
		beep();
		return (cp);
	}
	cp--;
	while (cp >= lp && *cp == BLANK) {	/* Skip space behind word */
		--cp, dl++;
	}
	while (cp >= lp && *cp != BLANK)	/* Skip chars in word	*/
		dl += chlen(*cp--);
	backspace(dl);				/* Backspace over del chars */
	cp++;

	wl = sp - cp;

	for (p = cp; ; p++) {
		if ((*p = *(p+wl)) == '\0')
			break;
	}
	writes(cp);				/* Write new end of line */
	space(dl);				/* Overwrite deleted space */
	backspace(linelen(cp) + dl);		/* Readjust cursor position */


	*lenp -= wl;
	return (cp);
}

/*
 * loescht die ganze Zeile
 */
LOCAL void
clearline(lp, cp)
	char	*lp,
		*cp;
{
	backspace(linediff(lp, cp));
	delete(lp);
}

/*
 * Fuegt char c bei cp ein.
 */
LOCAL void
ins_char(cp, c)
	register char	*cp;
	char		c;
{
	register char	*ep = cp;

	writes(cp);
	backspace(linelen(cp));
	while (*ep++);
	ep--;
	do {
		*(ep + 1) = *ep;
	} while (ep-- > cp);
	*cp = c;
}


/*
 * Fuegt String s bei cp ein.
 */
LOCAL char *
ins_word(cp, s)
	register char	*cp;
	register char	*s;
{
	register char	*ep = cp;
	register int	len;

	len = strlen(s);
	writes(cp);
	backspace(linelen(cp));
	while (*ep++);
	ep--;
	do {
		*(ep + len) = *ep;
	} while (ep-- > cp);
	while (*s)
		*cp++ = *s++;
	return (cp);
}


/*
 * loescht Zeichen *cp und stellt dar
 */
LOCAL void
del_char(cp)
	register char	*cp;
{
	register char	*p;
	register int dl = 0;

	backspace(dl = chlen(*cp));
	for (p = cp; *p; p++)
		*p = *(p+1);
	writes(cp);
	space(dl);
	backspace(linelen(cp) + dl);
}

#ifdef	OOO
LOCAL char *
new_line(lp, old, new)
	char		*lp;
	unsigned	old;
	unsigned	new;
{
	register char	*np;
					/* realloc !!! */
	np = malloc(new, (char *)NULL);
	movebytes(lp, np, (int)old);		/* make_line hat kein NULL Byte ! */
	free(lp);
	return (np);
}
#else
#define	new_line(lp, old, new)	realloc(lp, new)
#endif

/*
 * Free a history line Tnode.
 */
LOCAL void
free_line(p)
	register HISTPTR	p;
{
	p->h_flags = 0;
	free(p->h_line);
	free((char *) p);
}

/*
 * Create a temporary history line template.
 */
LOCAL HISTPTR
mktmp()
{
	register HISTPTR	tmp;

	tmp = (HISTPTR)malloc(sizeof (_HISTPTR));
	tmp->h_prev = tmp->h_next = NULL;
	tmp->h_line = malloc(LINEQUANT);
	tmp->h_len = LINEQUANT;
	tmp->h_pos = 0;
	tmp->h_flags = F_TMP;		/* Mark it temporary */
	tmp->h_line[0] = '\0';
	return (tmp);
}

/*
 * Create an editable copy of a history line.
 */
LOCAL HISTPTR
hold_line(p)
	register HISTPTR	p;
{
	register HISTPTR	tmp;

	tmp = (HISTPTR)malloc(sizeof (_HISTPTR));
	tmp->h_prev = p->h_prev;
	tmp->h_next = p->h_next;
	tmp->h_line = malloc(p->h_len);
	tmp->h_len = p->h_len;
	tmp->h_pos = p->h_pos;
	tmp->h_flags = F_TMP;		/* Mark it temporary */
	movebytes(p->h_line, tmp->h_line, p->h_len);
	return (tmp);
}

LOCAL void
unhold_line(p, op)
	register HISTPTR	p;	/* copy of hold line */
	register HISTPTR	op;	/* original hold line */
{
	if (p == (HISTPTR) NULL)
		return;

	if (op) {
		int len = strlen(op->h_line);

		if (p->h_pos > len)
			op->h_pos = len;
		else
			op->h_pos = p->h_pos;
	}
	if ((p->h_flags & F_TMP) != 0)
		free_line(p);
}

#ifdef DEBUG
LOCAL void
trace_hist()
{
	register HISTPTR	p;

	for (p = first_line; p; p = p->h_next) {
		printf("{ %s } at %p, prev at %p, next at %p\r\n",
			p->h_line, p, p->h_prev, p->h_next);
	}
}
#endif


/*
 * Remove a line from the history list.
 */
LOCAL HISTPTR
remove_line(p)
	register HISTPTR	p;
{
	register HISTPTR	lp;
	register HISTPTR	np;

	if (p == (HISTPTR) NULL)
		return ((HISTPTR) NULL);
#ifdef DEBUG
	printf("removing line at %p, chars at %p\r\n", p, p->h_line);
#endif
	lp = p->h_prev;
	np = p->h_next;
	if ((p->h_flags & F_TMP) == 0) {
		/*
		 * Only free it if not claimed by a temporary pointer.
		 */
		free(p->h_line);
		free((char *) p);
	}
	if (lp)
		lp->h_next = np;
	else
		first_line = np;
	if (np)
		np->h_prev = lp;
	else
		last_line = lp;
	no_lines--;
#ifdef DEBUG
	printf("removed. first at %p, last at %p, no_lines = %d\r\n",
		first_line, last_line, no_lines);
#endif
	if (lp == (HISTPTR) NULL)
		lp = first_line;
	return (lp);
}


/*
 * Append a line to the end of the history list.
 */
EXPORT void
append_line(linep, len, pos)
	char		*linep;
	unsigned	len;
	unsigned	pos;
{
	register HISTPTR p;
	register char		*lp;

#ifdef DEBUG
	printf("appending line, histlen = %d.\r\n", histlen);
#endif
	if (histlen == 0)
		return;
	if (no_lines == histlen)
		remove_line(first_line);
	p = (HISTPTR)malloc(sizeof (_HISTPTR));
	lp = malloc(len);
	strcpy(lp, linep);
	p->h_prev = last_line;
	p->h_line = lp;
	p->h_len  = len;
	p->h_pos  = pos;
	p->h_flags = 0;
	p->h_next = (HISTPTR) NULL;
	if (last_line)
		last_line->h_next = p;
	else
		first_line = p;
	last_line = p;
	no_lines++;
#ifdef DEBUG
	printf("appended line: first at %p, last at %p, no_lines = %d\r\n",
		first_line, last_line, no_lines);
#endif
}


/*
 * Provisorisch Folgezeilen in History (fuer if then else ... History)
 *
 */
LOCAL void
append_hline(linep, len)
	char	*linep;
	unsigned len;
{
	register HISTPTR p = last_line;
	register char	*lp;

	len += p->h_len;
	lp = malloc(len);
	strcatl(lp, p->h_line, "\205", linep, (char *)NULL);
	free(p->h_line);
	p->h_line = lp;
	p->h_len  = len;
	/*
	 * Keep cursor position of first line.
	 */
}


/*
 * Move line p to the end of the history.
 */
LOCAL void
move_to_end(p)
	HISTPTR p;
{
	register HISTPTR lp;
	register HISTPTR np;

	if ((np = p->h_next) != NULL) {
		lp = p->h_prev;
		if (lp)	{
			/* middle of history */
			lp->h_next = np;
			np->h_prev = lp;
		} else {
			/* first line of history */
			np->h_prev = (HISTPTR) NULL;
			first_line = np;
		}
		last_line->h_next = p;
		p->h_prev = last_line;
		p->h_next = (HISTPTR) NULL;
		p->h_flags &= ~F_TMP;
		last_line = p;
	}
#ifdef DEBUG
	printf("moved line: first at %p, last at %p\r\n",
						first_line, last_line);
#endif
}


/*---------------------------------------------------------------------------
|
| Stripout entfernt alle Zeilen, die den gleichen Inhalt wie die lezte Zeile
| haben. Stripout wird aufgerufen, nachdem die aktuell editierte Zeile die
| lezte Zeile geworden ist.
|
+---------------------------------------------------------------------------*/

LOCAL void
stripout()
{
	register HISTPTR p;
	register char	*linep;

	if (!last_line)
		return;
	linep = last_line->h_line;
#ifdef DEBUG
	printf("stripping out '%s'.\r\n", linep);
#endif
	for (p = first_line; p && p != last_line; p = p->h_next) {
#ifdef DEBUG
		printf("stripout: '%s'.\r\n", p->h_line);
#endif
		if (strlen(p->h_line) == 0 || streql(linep, p->h_line)) {
			p = remove_line(p);
			if (p == (HISTPTR)NULL)
				break;
		}
	}
}


/*
 * Implement the bsh history search function.
 */
LOCAL HISTPTR
match_input(cur_line, tmp_line, up)
	HISTPTR	cur_line;
	HISTPTR	tmp_line;
	BOOL	up;
{
	static	HISTPTR	match_line = (HISTPTR) NULL;
		HISTPTR	hp;
		char		*oldprompt;
		char		*pattern;

	if (!match_line)
		match_line = mktmp();
	oldprompt = iprompt;
	iprompt = up ? "search up: " : "search down: ";
	(void) fprintf(stderr, "\r\n%s", iprompt);
	(void) fflush(stderr);
	for (;;)
		if (edit_line(match_line) == '\n')
			break;
		else
			beep();
	pattern = match_line->h_line;
	if ((hp = match(cur_line, pattern, up)) == (HISTPTR) NULL)
		hp = tmp_line;
	iprompt = oldprompt;
	(void) fprintf(stderr, iprompt);
	(void) fflush(stderr);
	return (hp);
}


/*
 * Implement the !pattern search that is a simple csh feature.
 */
EXPORT char *
match_hist(pattern)
		char		*pattern;
{
	register HISTPTR	hp;

	if (streql(pattern, "!"))
		pattern = "*";
	remove_line(last_line);
	if ((hp = match(last_line, pattern, TRUE)) == (HISTPTR) NULL)
		return (NULL);
	move_to_end(hp);
	(void) fprintf(stderr, "%s\r\n", hp->h_line);
	(void) fflush(stderr);
	return (hp->h_line);
}


/*
 * Find a line that matches pattern in history.
 */
LOCAL HISTPTR
match(cur_line, pattern, up)
		HISTPTR	cur_line;
	register char		*pattern;
	register BOOL		up;
{
	register int		patlen;
	register int		alt = 0;
	register int		*aux;
	register int		*state;
	register HISTPTR	hp = (HISTPTR) NULL;
	register char		*lp;

	if (pattern) {
		patlen = strlen(pattern);
		aux = (int *)malloc((size_t)patlen * sizeof (int));
		state = (int *)malloc((size_t)(patlen+1) * sizeof (int));
		if ((alt = patcompile((unsigned char *)pattern, patlen, aux)) != 0)
			for (hp = cur_line; hp; ) {
				lp = hp->h_line;
				if (patmatch((unsigned char *)pattern, aux,
						(unsigned char *)lp, 0, strlen(lp), alt, state))
					break;
			if (up)
				hp = hp->h_prev;
			else
				hp = hp->h_next;
			}
		free((char *) aux);
		free((char *) state);
	}
	if (!hp) {
		if (!alt)
			berror(ebadpattern);
		else
			berror(enotfound);
		return ((HISTPTR) NULL);
	}
	return (hp);
}


/*
 * Implementation of basic editing functions.
 */
LOCAL int
edit_line(cur_line)
	HISTPTR cur_line;
{
	register int	c;
	register char	*lp, *cp;
	char		*lpp;
	unsigned	llen, maxlen;
	int		diff;
	int		multi = 0;

	get_tty_modes(infile);
#if	JOBCONTROL
	tty_setpgrp(fdown(infile), mypgrp);
#endif
#ifdef	DEBUG
	printf("editline at %p.\r\n", cur_line);
#endif
	maxlen = cur_line->h_len;
	cp = lp = cur_line->h_line;
	llen = strlen(lp) + 1;
	writes(lp);
	cp = &lp[cur_line->h_pos];
	backspace(linelen(cp));
	ins_mode = !*cp;
#ifdef	notneeded
	eof = 0;
#endif
	for (;;) {
		*cp ? set_insert_modes(infile) : set_append_modes(infile);
		c = nextc();
		switch (c) {

		case '\r':		/* eigentlich nicht noetig */
		case '\n':
			delim = '\n';
			putch('\r');
			putch('\n');
			bflush();
			cur_line->h_len = maxlen;
			cur_line->h_line = lp;
			cur_line->h_pos = cp - lp;
			reset_tty_modes();
#ifdef	DEBUG
			printf("ctlc: %d\r\n", ctlc);
#endif
			return ('\n');
		case RETYPE:
			redisp(lp, cp);
			break;
		case UNDO:
			cp = undo_del(lp, cp, &llen);
			break;
		case SHOW:
			show_files(lp, cp);
			break;
		case '\t':
		case EXPAND:
			if (multi) {
				show_files(lp, cp);
				break;
			}
			lpp = lp;
			cp = exp_files(&lpp, cp, &llen, &maxlen, &multi);
			lp = lpp;
			break;
		case BACKSPACE:
			multi = 0;
			if (cp > lp)
				backspace(chlen(*(--cp)));
			else
				beep();
			break;
		case FORWARD:
			multi = 0;
			if (*cp) {
				writec(*cp++);
				bflush();
			}
			else
				beep();
			break;
		case EOF:
#ifdef	notneeded
			if (++eof < 4)
				break;
			clearerr(infile);
#endif
			if (delim == EOF)
				exitbsh(0);
			/* FALLTHROUGH */
		case CTRLD:
			multi = 0;
			if ((cp == lp && !strlen(lp) &&
				!ev_eql(ignoreeofname, on)) || c == EOF) {
				delim = EOF;
				clearline(lp, cp);
				reset_tty_modes();
				putch('\r');
				putch('\n');
				writes("<EOF>");
				putch('\r');
				putch('\n');
				bflush();
				return (EOF);
			}
			if (*cp) {
				writec(*cp);
				bflush();
				del_char(cp);
				--llen;
			}
			else
				beep();

			break;
		case CTRLU:
			multi = 0;
			clearline(lp, cp);
			*lp = 0;
			llen = 1;		/* NULL Byte !!! */
			cp = lp;
			break;
		case CTRLW:
			multi = 0;
			cp = clearword(lp, cp, &llen);
			break;
		case ESC:
			multi = 0;
			c = get_request();
			if (c == RESTORE)
				clearline(lp, cp);
			if (c & ~BYTEMASK) {
				cur_line->h_line = lp;
				cur_line->h_len = maxlen;
				cur_line->h_pos = cp - lp;
				reset_tty_modes();
				return (c);
			}
			else
				cp = esc_process(c, lp, cp, &llen);
			break;
		case UPWARD:
		case DOWNWARD:
			clearline(lp, cp);
			cur_line->h_line = lp;
			cur_line->h_len = maxlen;
			cur_line->h_pos = cp - lp;
			return (c);
		case DELETE:
			multi = 0;
			if (cp > lp) {
				del_char(--cp);
				llen--;
			}
			else
				beep();
			break;
		case BEGINLINE:
			multi = 0;
			backspace(linediff(lp, cp));
			cp = lp;
			break;
		case ENDLINE:
			multi = 0;
			while (*cp)
				writec(*cp++);
			bflush();
			break;
		case QUOTECH:
			set_insert_modes(infile);
			c = nextc();
			if ((toupper(c) < 0140) && c >= '@')
				c &= 037;
		default:
			multi = 0;
			if (i_should_echo || !isprint(c))
				writec(c);
			bflush();
			if (llen == maxlen) {
				maxlen += LINEQUANT;
				diff = cp - lp;
				lp = new_line(lp, llen, maxlen);
				cp = lp + diff;
			}
			ins_char(cp, c);
			cp++;
			llen++;
		}
	}
}


/*
 * Redisplay current line.
 */
LOCAL void
redisp(lp, cp)
	char	*lp;
	char	*cp;
{
	(void) fprintf(stderr, "\r\n%s", iprompt);
	(void) fflush(stderr);
	writes(lp);
	backspace(linelen(cp));
}


/*
 * Insert a string a current cursor position.
 */
LOCAL char *
insert(cp, s, lenp)
	register char	*cp;
	register char	*s;
	unsigned	*lenp;
{
	*lenp += strlen(s);
	writes(s);
	cp = ins_word(cp, s);
	return (cp);
}


/*
 * Undo last delete(s).
 */
LOCAL char *
undo_del(lp, cp, lenp)
	register char	*lp;
	register char	*cp;
	unsigned	*lenp;
{
#ifdef	NEW
	register int	fdellen;

	cp = insert(cp, "undo", lenp);
	cp = insert(cp, "del", lenp);
	fdellen = strlen("del");
	backspace(fdellen);
	cp -= fdellen;
#endif
	return (cp);
}

/*
 * The characters ' ' ... '&' are handled by the shell parser,
 * the characters '!' ... '$' are pattern matcher meta characters.
 * The complete pattern matcher meta characters are "!#%*?\\{}[]^$", but
 * the '%' has been checked before in the shell parser charracter set.
 */
LOCAL char xchars[] = " \t<>%|;()&!#*?\\{}[]^$"; /* Chars that need quoting */

/*
 * Expand a string and return a malloc()ed copy.
 * Any character that needs quoting is prepended with a '\\'.
 */
LOCAL char *
xpstr(cp)
	char	*cp;
{
	char	*ret;
	char	*p = cp;
	int	len = 0;

	while (*p) {
		len++;
		if (strchr(xchars, *p++))
			len++;
	}
	ret = malloc(len+1);
	for (p = ret; *cp; ) {
		if (strchr(xchars, *cp))
			*p++ = '\\';
		*p++ = *cp++;
	}
	*p = '\0';
	return (ret);
}

/*
 * The characters ' ' ... '&' are handled by the shell parser as word separators
 */
LOCAL char wschars[] = " \t<>%|;()&"; /* Chars that are word separators */

/*
 * Expand filenames (implement file name completion).
 * This is the basic function that either expands or lists filenames.
 * It gets called by exp_files() and by show_files().
 */
LOCAL char *
xp_files(lp, cp, show, multip)
	register char	*lp;	/* Begin of current line		*/
	register char	*cp;	/* Current cursor position		*/
		BOOL	show;	/* Show list of multi-results		*/
		int	*multip; /* Found mult results in non show mode */
{
	Tnode	*np;
	Tnode	*l1;
	char	*wp;
	char	*wp2 = NULL;
	char	*tp;
	int	len;
	int	xlen;
	int	dir = 0;
	char	*p1;
	char	*p2;
	int	multi = 0;	/* No mutiple results found yet	*/

	/*
	 * Check whether to expand (complete) the filename.
	 * This depends on the character that is currently under the cursor.
	 */
	if (*cp != '\0' && !strchr(wschars, *cp)) {
/*		beep();*/
		return (0);
	}
	if (show) {
		(void) fprintf(stderr, "\r\n");
		(void) fflush(stderr);
	}
	/*
	 * Set "wp" to current cursor position and then step back into the text
	 */
	wp = cp;
	if (*wp == '\0' || strchr(wschars, *wp))
		wp--;


	/*
	 * Step back to the beginning of the current word.
	 * Do not stop on quoted delimiters.
	 */
again:
	while (wp > lp && !strchr(wschars, *wp))
		wp--;
	if (wp > lp && strchr(wschars, *wp) &&
	    wp[-1] == '\\') {
		wp--;
		goto again;
	}


	/*
	 * Advance again into current word.
	 */
	if (strchr(wschars, *wp))
		wp++;


	len = cp - wp;
	tp = malloc(len+2);
	strncpy(tp, wp, len);
	tp[len] = '*';
	tp[len+1] = '\0';
	np = expand(tp);
	if (np == NULL) {
		free(tp);
/*		beep();*/
		return (0);
	}
	/*
	 * More than one result?
	 */
	if (np->tn_right.tn_node)
		multi++;

	wp = np->tn_left.tn_str;		/* First in list	 */
	wp = xpstr(wp);			/* Insert '\\' if needed */
	p2 = strrchr(wp, '/');
	xlen = 0;
	if (p2)
		xlen = p2 - wp + 1;
	p2 = wp;
	for (l1 = np; l1 != (Tnode *) NULL; l1 = l1->tn_right.tn_node) {
		if (l1->tn_right.tn_node == NULL)
			/*
			 * Last in list for max. differences to
			 * compare with first one.
			 */
			p2 = l1->tn_left.tn_str;
		if (show) {
			writes(&l1->tn_left.tn_str[xlen]);
			writes(" ");
		}
	}
	/*
	 * For multiple results, find longest common match.
	 */
	if (multi) {
		wp2 = xpstr(p2);
		for (p1 = wp, p2 = wp2; *p1; ) {
			if (*p1++ != *p2++) {
				p1--;
				break;
			}
		}
		xlen = p1 - wp;
		if (*p1 == '\0' && *p2 == '\0') {
			p1 = NULL;
		}
	} else {
		xlen = strlen(wp);
		p1 = NULL;
	}

	if (!show) {
		if (p1 == NULL)
			dir = is_dir(np->tn_left.tn_str);
		if ((xlen - len) > 0 || dir) {
			p2 = malloc(xlen-len+2);
		} else {
			/*
			 * Nothing to add.
			 */
			p2 = NULL;
/*			beep();*/
			if (multip)
				*multip = multi;
		}
	}
	if (!show && p2) {
		strncpy(p2, &wp[len], xlen-len);
		if (p1) {
			if ((xlen - len) == 0)
				beep();
			p2[xlen-len] = '\0';
		} else {
			p2[xlen-len] = dir ? '/' : ' ';
			p2[xlen-len+1] = '\0';
		}
	} else {
		p2 = NULL;
	}
	freetree(np);
	free(tp);
	free(wp);
	if (wp2)
		free(wp2);
	return (p2);
}


/*
 * Expand filenames (implement file name completion).
 * Insert expansion result into current line if applicable.
 */
LOCAL char *
exp_files(lpp, cp, lenp, maxlenp, multip)
	register char	**lpp;
	register char	*cp;
	unsigned	*lenp;
	unsigned	*maxlenp;
		int	*multip;
{
	char	*p;
	int	diff;

	p = xp_files(*lpp, cp, FALSE, multip);
	if (p) {
		diff = strlen(p);
		if (*lenp + diff >= *maxlenp) {
			diff = ((diff + LINEQUANT)/LINEQUANT) * LINEQUANT;
			*maxlenp += diff;
			diff = cp - *lpp;
			*lpp = new_line(*lpp, *lenp, *maxlenp);
			cp = *lpp + diff;
		}
		cp = insert(cp, p, lenp);
		free(p);
	} else {
		beep();
	}
	return (cp);
}


/*
 * Show filename list (implement file name completion).
 */
LOCAL void
show_files(lp, cp)
	register char	*lp;
	register char	*cp;
{
	xp_files(lp, cp, TRUE, NULL);
	redisp(lp, cp);
}


/*
 * Get an ESC request
 */
LOCAL int
get_request()
{
	register int c;

		set_insert_modes(infile);
	c = nextc();
	if (c == UPWARD || c == DOWNWARD)
		return (c | SEARCH);
	if (c == '\n' || c == '\r')
		return (RESTORE);
	return (c);
}


/*
 * Implementation of functions that follow an ESC in an ESC sequence.
 */
LOCAL char *
esc_process(xc, lp, cp, lenp)
	register int	xc;
	register char	*lp;
	register char	*cp;
		unsigned *lenp;
{
	register char	*pp;
	register int	dl = 0;

	switch (xc) {

	case BACKSPACE:
		if (cp == lp) {
			beep();
			return (cp);
		}
		cp--;
		while (cp >= lp && *cp == BLANK) {
			--cp, dl++;
		}
		while (cp >= lp && *cp != BLANK)
			dl += chlen(*cp--);
		backspace(dl);
		return (++cp);
	case FORWARD:
		if (!*cp)
			beep();
		else {
			while (*cp && *cp != BLANK)
				writec(*cp++);
			while (*cp == BLANK)
				writec(*cp++);
			bflush();
		}
		return (cp);
	case DELETE:
	case CTRLD:
		pp = cp;
		if (xc == CTRLD) {
			if (!*cp) {
				beep();
				return (cp);
			}
			while (*pp && *pp != BLANK)
				dl += chlen(*pp++);
			while (*pp == BLANK)
				dl += chlen(*pp++);
		} else {
			if (cp == lp) {
				beep();
				return (cp);
			}
			while (--cp > lp && *cp == BLANK);
			while (cp > lp && *(cp-1) != BLANK)
				--cp;
			dl = linediff(cp, pp);
			backspace(dl);
		}
		strcpy(cp, pp);
		writes(cp);
		space(dl);
		backspace(linelen(cp) + dl);
		(*lenp) -= (pp - cp);
		return (cp);
	default:
		beep();
		return (cp);
	}
}


/*
 * Interactive line editor function.
 * Allows only a limited subset if the editing functions.
 */
LOCAL char *
sget_line()
{
		char		*lp;
		BOOL		edit = TRUE;
		int		cmd;
	register HISTPTR	tmp_line;

	tmp_line = mktmp();
	while (edit) {
		cmd = edit_line(tmp_line);
		switch (cmd) {

		case UPWARD:
		case DOWNWARD:
		case SEARCHUP:
		case SEARCHDOWN:
		case RESTORE:
				beep();
				break;
		case EOF:
				return (NULL);
		case '\r':
		case '\n':
				edit = FALSE;
		}
	}
	lp = tmp_line->h_line;
	append_hline(lp, tmp_line->h_len);
	free((char *) tmp_line);
	return (lp);
}


/*
 * Interactive line editor function.
 * Allows all editing functions.
 */
LOCAL char *
iget_line()
{
	register char		*lp;
		char		*np;
		BOOL		edit = TRUE;
	register int		cmd = '\0';
	register HISTPTR	cur_line;
	register HISTPTR	tmp_line;			/* empty tmp */
	register HISTPTR	save_line;
	register HISTPTR	etmp_line = (HISTPTR) NULL;	/* tmp copy */
	register HISTPTR	orig_line = (HISTPTR) NULL;	/* tmp original */

	save_line = cur_line = tmp_line = mktmp();
	lp = cur_line->h_line;
	cur_line->h_prev = last_line;
	cur_line->h_next = first_line;
#ifdef DEBUG
	printf("History: first at %p, last at %p\r\n", first_line, last_line);
#endif
	while (edit) {
		if (!(cmd & ~BYTEMASK))
			save_line = cur_line;
		cmd = edit_line(cur_line);
#ifdef DEBUG
		printf("Edited line at %p, line at %p '%s'.\r\n", cur_line,
					cur_line->h_line, cur_line->h_line);
#endif
		switch (cmd) {

		case UPWARD:
		case DOWNWARD:
		case SEARCHUP:
		case SEARCHDOWN:
#ifdef DEBUG
			printf("Changing line at %p.\r\n", cur_line);
#endif
			if ((cmd & BYTEMASK) == UPWARD)
				cur_line = cur_line->h_prev;
			else
				cur_line = cur_line->h_next;
			if (etmp_line) {
				if (etmp_line == save_line)
					save_line = orig_line;
				unhold_line(etmp_line, orig_line);
				etmp_line = NULL;
			}
#ifdef DEBUG
			printf("New line at %p.\r\n", cur_line);
#endif
			if (cur_line == (HISTPTR) NULL)
				cur_line = tmp_line;
			if ((cmd & ~BYTEMASK) == SEARCH) {
				cur_line = match_input(cur_line,
						tmp_line, cmd == SEARCHUP);
			}
			orig_line = cur_line;
			etmp_line = cur_line = hold_line(cur_line);
			break;
		case RESTORE:
			if (etmp_line) {
				if (etmp_line == save_line)
					save_line = orig_line;
				unhold_line(etmp_line, orig_line);
				etmp_line = NULL;
			}
			cur_line = save_line;
			break;
		case EOF:
			return (NULL);
		case '\r':
		case '\n':
#ifdef DEBUG
			printf("End of line.\r\n");
#endif
			edit = FALSE;
		}
	}
	lp = cur_line->h_line;
	np = makestr(lp);
	if (cur_line == tmp_line || cur_line == etmp_line) {
#ifdef DEBUG
		printf("tmp_line to append....\r\n");
#endif
		if (*lp)
			append_line(lp, cur_line->h_len, cur_line->h_pos);
	} else {
#ifdef DEBUG
		printf("previous line to move....\r\n");
#endif
		if (*lp)
			move_to_end(cur_line);
	}

	stripout();
	if ((tmp_line->h_flags & F_TMP) != 0) {
		free_line(tmp_line);
	}
	if (etmp_line && (etmp_line->h_flags & F_TMP) != 0) {
		free_line(etmp_line);
	}
#ifdef DEBUG
	printf("History: first at %p, last at %p\r\n", first_line, last_line);
#endif
	return (np);
}


/*
 * Read a line using func pointer and return result in allocated string.
 */
/* VARARGS1 */
EXPORT char *
make_line(f, arg)
	register int	(*f) __PR((FILE *));
	register FILE	*arg;
{
	register unsigned	maxl;
	register unsigned	llen;
			char	*lp;
	register	char	*p;
	register 	int	c;

#ifdef DEBUG
	printf("        make_line\r\n");
#endif
	lp = p = malloc(maxl = LINEQUANT);
	llen = 0;
	for (;;) {
		if ((c = (*f)(arg)) == EOF || c == '\n' || c == '\205') {
			if (c == EOF)
				delim = EOF;
			else
				delim = '\n'; /*XXX ??? \205 ???*/
			*p = 0;
			return (lp);
		}
		*p++ = (char)c;
		if (++llen == maxl) {
			maxl += LINEQUANT;
			lp = new_line(lp, llen, maxl);
			p = lp + llen;
		}
	}
}


/*
 * Read a line from a file in a way compatible to [is]get_line().
 */
LOCAL char *
fread_line(f)
	FILE	*f;
{
	extern int	fgetc __PR((FILE *));

	return (make_line(fgetc, f));
}


/*
 * External interface to the line editor.
 * Use fread_line() if input is from a file.
 * Use iget_line() if input is from tty and promt counter is 0.
 * Use sget_line() if input is from tty and promt counter is > 0.
 */
EXPORT char *
get_line(n, f)
	int	n;
	FILE	*f;
{
	if (line_pointer) {
		free(line_pointer);
		line_pointer = NULL;
	}
	if (ttyflg && mp_init) {
		map_init();
		term_init();
	}
	if (prflg) {
		iprompt = prompts[n?1:0];
		(void) fprintf(stderr, iprompt);
		(void) fflush(stderr);
	}
	if (!ttyflg) {
#ifdef	DEBUG
		printf("        get_line: fread_line(%p).\r\n", f);
#endif
		line_pointer = fread_line(f);
	} else {
		infile = f;
		tty_init();
		if (n)
			line_pointer = sget_line();
		else
			line_pointer = iget_line();
		tty_term();
	}
	return (line_pointer?line_pointer:nullstr);
}


/*
 * Write current content of the history to an open file.
 * Add {} brackets if we are writing to stdout.
 */
EXPORT void
put_history(f, intrflg)
	register FILE	*f;
	register int	intrflg;
{
	register HISTPTR p;

	for (p = first_line; p; p = p->h_next) {
		if (ctlc && intrflg)
			break;
		if (f == stdout) {
			writes("{ ");
			writes(p->h_line);
			writes(" }");
			putch('\n');
		} else {
			fprintf(f, "%s\n", p->h_line);
		}
	}
	if (f == stdout)
		bflush();
}


/*
 * Save the history by writing to ~/.history
 */
EXPORT void
save_history(intrflg)
	int intrflg;
{
	FILE	*f;

	if (no_lines == 0)	/* don't damage history File */
		return;
	f = fileopen(hfilename, for_wct);
	if (f) {
		put_history(f, intrflg);
		fclose(f);
	}
}


/*
 * Init the history by reading from ~/.history
 */
EXPORT void
read_init_history()
{
	FILE	*f;

	hfilename = concat(inithome, slash, historyname, (char *)NULL);
	f = fileopen(hfilename, for_read);
	if (f) {
		readhistory(f);
		fclose(f);
	}
}

/*
 * Read in the history from a file.
 * Used to init history and for source -h
 */
EXPORT void
readhistory(f)
	register FILE	*f;
{
		char	line[512];
	register char	*s = line;
	register int	len;

	while ((len = fgetline(f, s, sizeof (line))) >= 0) {
		if (len == 0)
			continue;
		/*
		 * Skip bash timestamps
		 */
		if (line[0] == '#' && line[1] == '+') {
			register char	*p;

			for (p = &line[2]; *p != '\0'; p++)
				if (!isdigit((unsigned char)*p))
					break;
			if (*p == '\0')
				continue;
		}
#ifdef	DEBUG
		fprintf(stderr, "appending: %s\r\n", s);
#endif
		len = strlen(s);
		append_line(s, (unsigned) len+1, len);
	}
}

#include <schily/termcap.h>
LOCAL	char	*KE;		/* Keypad end transmit mode	"ke"	*/
LOCAL	char	*KS;		/* Keypad start transmit mode	"ks"	*/
LOCAL	char	*VE;		/* Visual end sequence		"ve"	*/
LOCAL	char	*VS;		/* Visual start sequence	"vs"	*/

LOCAL	char	**tstrs[] = {
		&KE, &KS, &VE, &VS,
};

LOCAL void
term_init()
{
	register char	*np = "keksvevs";
	register char	***sp = tstrs;

	if (KE) free(KE);
	if (KS) free(KS);
	if (VE) free(VE);
	if (VS) free(VS);
	do {
		*(*sp++) = tgetstr(np, NULL);
		np += 2;
	} while (*np);
}

#define	f_putch		((int (*)__PR((int)))putch)

LOCAL void
tty_init()
{
	tputs(VS, 0, f_putch);	/* make cursor visible */
	tputs(KS, 0, f_putch);	/* start keypad transmit mode */
}

LOCAL void
tty_term()
{
	tputs(VE, 0, f_putch);
	tputs(KE, 0, f_putch);
	bflush();
}


#ifdef	DO_DEBUG
#include <schily/varargs.h>
/*
 * Do formatted debugging output to the console
 */
/* PRINTFLIKE1 */
#ifdef	PROTOTYPES
LOCAL void
cdbg(char *fmt, ...)
#else
LOCAL void
cdbg(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	char	lbuf[1024];
	va_list	args;
	static	int	f;
	int	len;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	len = snprintf(lbuf, sizeof (lbuf), "%r", fmt, args);
	va_end(args);

	if (f == 0) {
		char	*cname;
		if ((cname = getcurenv("BSH_DBGTERM")) == NULL)
			cname = "/dev/console";

		f = open(cname, O_WRONLY);
		if (f == 0)
			return;
#ifdef	F_SETFD
		fcntl(f, F_SETFD, 1);	/* Set close on exec */
#endif
	}
	write(f, lbuf, len);
}
#endif	/* DO_DEBUG */

#endif /* INTERACTIVE */
