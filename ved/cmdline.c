/* @(#)cmdline.c	1.25 09/07/09 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cmdline.c	1.25 09/07/09 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	Routines for command line input for ved
 *	Command line input is done on the top (status) line of the screen.
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
#include "terminal.h"
#include <schily/varargs.h>

EXPORT	void	wait_for_confirm __PR((ewin_t *wp));
EXPORT	void	wait_continue	__PR((ewin_t *wp));
EXPORT	int	getcmdchar	__PR((ewin_t *wp, char *ans, char *msg, ...));
EXPORT	int	getcmdline	__PR((ewin_t *wp, Uchar *result, int len, char *msg, ...));
EXPORT	int	getccmdline	__PR((ewin_t *wp, int c, Uchar *result, int len, char *msg, ...));
LOCAL	int	_getcmdline	__PR((ewin_t *wp, int ch, Uchar *result, int len, char *msg, va_list args));
LOCAL	Uchar	mygchar		__PR((ewin_t *wp));
LOCAL	void	backspace	__PR((int size));
LOCAL	void	space		__PR((int size));

/*
 * Allow the user to read the current screen before returning
 * to the edit session.
 */
EXPORT void
wait_for_confirm(wp)
	ewin_t	*wp;
{
	output(UC "TYPE ANY CHAR TO RETURN TO EDITOR");
	CLEAR_TO_EOF_LINE(wp);
	flush();
	nigchar(wp);
}

/*
 * Wait for the user to allow us to output the next page.
 */
EXPORT void
wait_continue(wp)
	ewin_t	*wp;
{
	output(UC "TYPE ANY CHAR TO CONTINUE");
	CLEAR_TO_EOF_LINE(wp);
	flush();
	nigchar(wp);
}

/*
 * Print a formatted message and get a conformation.
 * If ans is NULL, confirmation may be in the range from space to DEL.
 * If ans is != NULL confirmation may only be one of the chars in ans (TRUE).
 * Any other input is taken as abortion.
 * The calling routine is responsable for evaluating any non-abort input.
 */
/* PRINTFLIKE3 */
#ifdef	PROTOTYPES
EXPORT int
getcmdchar(ewin_t *wp, char *ans, char *msg, ...)
#else
EXPORT int
getcmdchar(wp, ans, msg, va_alist)
	ewin_t	*wp;
	char	*ans;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	Uchar	c;
	char	tbuf[NAMESIZE];
	int	ret = 0;

	writemsg(wp, "");
	CURSOR_HOME(wp);
#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	snprintf(tbuf, sizeof (tbuf), "%r", msg, args);
	va_end(args);
	output(UC tbuf);
	flush();

	c = nigchar(wp);
	if (ans == NULL && c > ' ' && c < 0177)
		ret = c;
	else if (ans != NULL && strchr(ans, c))
		ret = c;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	writemsg(wp, "%.20r%c", msg, args, (ret? c : ' '));
	va_end(args);
	if (!ret)
		abortmsg(wp);
	updatesysline = 1;
	return (ret);
}

/*
 * Print a formatted message and read a string.
 * First character of input is not yet known.
 */
/* PRINTFLIKE4 */
#ifdef	PROTOTYPES
EXPORT int
getcmdline(ewin_t *wp, Uchar *result, int len, char *msg, ...)
#else
EXPORT int
getcmdline(wp, result, len, msg, va_alist)
	ewin_t	*wp;
	Uchar	*result;
	int	len;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret =  _getcmdline(wp, -1, result, len, msg, args);
	va_end(args);

	return (ret);
}

/*
 * Print a formatted message and read a string.
 * First character of input is not yet known.
 */
/* PRINTFLIKE5 */
#ifdef	PROTOTYPES
EXPORT int
getccmdline(ewin_t *wp, int c, Uchar *result, int len, char *msg, ...)
#else
EXPORT int
getccmdline(wp, c, result, len, msg, va_alist)
	ewin_t	*wp;
	int	c;
	Uchar	*result;
	int	len;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret =  _getcmdline(wp, c, result, len, msg, args);
	va_end(args);

	return (ret);
}

/*
 * Print a formatted message and read a string.
 * If ch is != -1 take it as the first character.
 * Return number of characters read.
 */
LOCAL int
_getcmdline(wp, ch, result, len, msg, args)
	ewin_t	*wp;
	int	ch;
	Uchar	*result;
	register int len;
	char	*msg;
	va_list	args;
{
	extern   Uchar csize[];
	register Uchar *rcsize = csize;
	register Uchar *str = result;
	register int   size;
	register int   i = 0;
	register Uchar c;
	char	 tbuf[NAMESIZE];

	writemsg(wp, "");
	CURSOR_HOME(wp);
	snprintf(tbuf, sizeof (tbuf), "%r", msg, args);
	output(UC tbuf);
	flush();

	if (ch == -1)
		c = mygchar(wp);
	else
		c = (Uchar)ch;

	while (c != '\012' && c != '\015') {	/* c != ^J && c != ^M */
		if (c == 0177) {		/* DEL */
			if (i <= 0) {
				ringbell();
			} else {
				--i; --str;
				cpos.hp -= size = rcsize[*str];
				backspace(size);
				space(size);
				backspace(size);
			}
		} else if (c == '\025' ||	/* ^U -> Kill complete line */
			    c == ('C' & 0x1F)) { /* ^C -> Kill complete line */
			*result = i = 0;
			break;
		} else if (c == '\022') {	/* ^R -> Retype line */
			*str = 0;
			CURSOR_HOME(wp);
			CLEAR_TO_EOF_LINE(wp);
			output(UC msg);
			printstring(result, i);
		} else if (c == '\033' ||	/* ^[ -> Escape next char */
			    c == ('^' & 0x1F)) { /* ^^ -> Escape next char */
			putoutchar('$');
			backspace(1);
			flush();
			c = mygchar(wp);
			*str++ = c;
			i++;
			printstring(&str[-1], 1);
		} else {
			*str++ = c;
			i++;
			printstring(&str[-1], 1);
		}
		flush();
		if (i >= len) {
			CURSOR_HOME(wp);
			writeerr(wp, "OVERFLOW IN CMD LINE");
			do {
				c = nigchar(wp);
			} while (c != '\03' &&
				    c != '\012' && c != '\015' && c != '\025');
				/* c != ^C && c != ^J && c != ^M && c != ^U */
			i = 0;
			str--;
			break;
		}
		c = mygchar(wp);
	}
	*str = '\0';
	writemsg(wp, "%.20s%s ...", msg, result);
	if (i == 0)
		abortmsg(wp);
	updatesysline = 1;
	return (i);
}

/*
 * Get a character from input.
 * Close input line if environment SLASH is off and a slash was seen.
 */
LOCAL Uchar
mygchar(wp)
	ewin_t	*wp;
{
	register Uchar c = nigchar(wp);

	if (c == '/' && streql(getenv("SLASH"), "off"))
		c = '\r';
	return (c);
}

/*
 * Write 'size' backspaces
 */
LOCAL void
backspace(size)
	int	size;
{
	while (size-- > 0)
		putoutchar('\b');
}

/*
 * Write 'size' spaces
 */
LOCAL void
space(size)
	int	size;
{
	while (size-- > 0)
		putoutchar(' ');
}
