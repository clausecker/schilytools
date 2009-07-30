/* @(#)tgetent.c	1.33 09/07/12 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)tgetent.c	1.33 09/07/12 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	Access routines for TERMCAP database.
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

/*
 * XXX Non POSIX imports from libschily: geterrno()
 */
#ifdef	BSH
#	include <schily/stdio.h>
#	include "bsh.h"
#endif

#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/signal.h>
#include <schily/errno.h>
#include <schily/schily.h>
#include <schily/termios.h>
#include <schily/ctype.h>
#include <schily/utypes.h>
#include <schily/termcap.h>

#ifdef	NO_LIBSCHILY
#	define	geterrno()	(errno)
#endif
#ifdef	BSH
#	define	getenv		getcurenv
#endif

#ifdef	pdp11
#define	TRDBUF		512	/* Size for read(2) buffer		*/
#else
#define	TRDBUF		8192	/* Size for read(2) buffer		*/
#endif
#define	TMAX		1024	/* Historical termcap buffer size	*/
#define	MAXLOOP		32	/* Max loop count for tc= directive	*/
#define	TSIZE_SPACE	14	/* Space needed for: "li#xxx:co#yyy:"	*/

LOCAL	char	_Eterm[]	= "TERM";
LOCAL	char	_Etermcap[]	= "TERMCAP";
LOCAL	char	_Etermpath[]	= "TERMPATH";
LOCAL	char	_termpath[]	= ".termcap /etc/termcap";
LOCAL	char	_tc[]		= "tc";
				/*
				 * Additional terminfo quotes
				 * "e^[" "::" ",," "s " "l\n"
				 */
LOCAL	char	_quotetab[]	= "E^^\\\\e::,,s l\nn\nr\rt\tb\bf\fv\v";

LOCAL	char	_etoolong[]	= "Termcap entry too long\n";
LOCAL	char	_ebad[]		= "Bad termcap entry\n";
LOCAL	char	_eloop[]	= "Infinite tc= loop\n";
LOCAL	char	_enomem[]	= "No memory for parsing termcap\n";

LOCAL	char	*tbuf = 0;
LOCAL	int	tbufsize = 0;
LOCAL	BOOL	tbufmalloc = FALSE;
LOCAL	int	tflags = 0;
LOCAL	int	loopcount = 0;

EXPORT	int	tgetent		__PR((char *bp, char *name));
EXPORT	int	tcsetflags	__PR((int flags));
EXPORT	char	*tcgetbuf	__PR((void));
LOCAL	int	tchktc		__PR((void));
LOCAL	BOOL	tmatch		__PR((char *name));
LOCAL	char	*tskip		__PR((char *ep));
LOCAL	char	*tfind		__PR((char *ep, char *ent));
EXPORT	int	tgetnum		__PR((char *ent));
EXPORT	BOOL	tgetflag	__PR((char *ent));
EXPORT	char	*tgetstr	__PR((char *ent, char **array));
EXPORT	char	*tdecode	__PR((char *ep, char **array));
#if	defined(TIOCGSIZE) || defined(TIOCGWINSZ)
LOCAL	void	tgetsize	__PR((void));
LOCAL	void	tdeldup		__PR((char *ent));
LOCAL	char	*tinsint	__PR((char *ep, int i));
#endif
LOCAL	void	tstrip		__PR((void));
LOCAL	char	*tmalloc	__PR((int size));
LOCAL	char	*trealloc	__PR((char *p, int size));

EXPORT int
tgetent(bp, name)
	char	*bp;
	char	*name;
{
			char	rdbuf[TRDBUF];
			char	*term;
			char	termpath[TMAX];
			char	*tp;
	register	char	*ep;
	register	char	*rbuf = rdbuf;
	register	char	c;
	register	int	count	= 0;
	register	int	tfd;
			int	err = 0;

	tbufsize = TMAX;
	if (tbufmalloc) {
		if (tbuf)
			free(tbuf);
		tbufmalloc = FALSE;
	}
	tbuf = NULL;
	if (name == NULL || *name == '\0') {
		if (bp)
			bp[0] = '\0';
		return (0);
	}
	if ((tbuf = bp) == NULL) {
		tbufmalloc = TRUE;
		tbuf = bp = tmalloc(tbufsize);
	}
	if ((tbuf = bp) == NULL)
		return (0);
	bp[0] = '\0';		/* Always start with clean termcap buffer */

	/*
	 * First look, if TERMCAP exists
	 */
	if (!(ep = getenv(_Etermcap)) || *ep == '\0') {
		/*
		 * If no TERMCAP environment or empty TERMCAP environment
		 * use default termpath.
		 * Search rules:
		 * 	If TERMPATH exists, use it
		 *	else concat $HOME/ with default termpath
		 *	default termpath is .termcap /etc/termcap
		 */
setpath:
		if ((ep = getenv(_Etermpath)) != NULL) {
			strncpy(termpath, ep, sizeof (termpath));
		} else {
			termpath[0] = '\0';
			if ((ep = getenv("HOME")) != NULL) {
				strncpy(termpath, ep,
					sizeof (termpath)-2-sizeof (_termpath));
				strcat(termpath, "/");
			}
			strcat(termpath, _termpath);
		}
	} else {
		if (*ep != '/') {
			/*
			 * This doesn't seem to be a filename...
			 * It must be a preparsed termcap entry.
			 */
			if (!(term = getenv(_Eterm)) ||
						strcmp(name, term) == 0) {
				/*
				 * If no TERM environment or TERM holds the
				 * same strings as "name" use preparsed entry.
				 */
				tbuf = ep;
				count = tmatch(name);
				tbuf = bp;
				if (count > 0) {
					count = strlen(ep) + 1 + TSIZE_SPACE;
					if (count > tbufsize) {
						if (tbufmalloc) {
							tbufsize = count;
							tbuf = trealloc(bp,
									count);
						} else {
							tbuf = NULL;
							write(STDERR_FILENO,
							_etoolong,
							sizeof (_etoolong) - 1);
						}
					}
					if (tbuf)
						strcpy(tbuf, ep);
					goto out;
				}
			}
			/*
			 * If the preparsed termcap entry does not match
			 * our term, we need to read the file.
			 * Set up the internal or external TERMPATH for
			 * this purpose.
			 */
			goto setpath;
		}
		/*
		 * If TERMCAP starts with a '/' use it as TERMPATH.
		 */
		strncpy(termpath, ep, sizeof (termpath));
	}
	termpath[sizeof (termpath)-1] = '\0';
	tp = termpath;

nextfile:
	/*
	 * Loop over TERMPATH string.
	 */
	ep = tp;
	while (*tp++) {
		if (*tp == ' ' || *tp == ':') {
			*tp++ = '\0';
			break;
		}
	}
	if (*ep == '\0') {
		if (err != 0)
			return (-1);
		return (0);
	}

	if ((tfd = open(ep, 0)) < 0) {
		err = geterrno();

#ifdef	SHOULD_WE
		if (err == ENOENT || err == EACCES)
			goto nextfile;
		return (-1);
#else
		goto nextfile;
#endif
	}

	/*
	 * Search TERM entry in one file.
	 */
	ep = bp;
	for (;;) {
		if (--count <= 0) {
			if ((count = read(tfd, rdbuf, sizeof (rdbuf))) <= 0) {
				close(tfd);
				goto nextfile;
			}
			rbuf = rdbuf;
		}
		c = *rbuf++;
		if (c == '\n') {
			if (ep > bp && ep[-1] == '\\') {
				ep--;
				continue;
			}
		} else if (ep >= bp + (tbufsize-1)) {
			if (tbufmalloc) {
				tbufsize += TMAX;
				if ((bp = trealloc(bp, tbufsize)) != NULL) {
					ep = bp + (ep - tbuf);
					tbuf = bp;
					*ep++ = c;
					continue;
				} else {
					tbuf = NULL;
					goto out;
				}
			}
			write(STDERR_FILENO, _etoolong, sizeof (_etoolong) - 1);
		} else {
			*ep++ = c;
			continue;
		}
		*ep = '\0';
		if (tmatch(name)) {
			close(tfd);
			goto out;
		}
		ep = bp;
	}
out:
	count = tchktc();
	bp = tbuf;
	if (tbufmalloc) {
		if (count <= 0) {
			if (bp)
				free(bp);
			tbuf = NULL;
			tbufmalloc = FALSE;
			return (count);
		}
		/*
		 * Did change size in tchktc() ?
		 */
		count = strlen(bp) + 1;
		if (count != tbufsize) {
			tbufsize = count;
			if ((tbuf = bp = trealloc(bp, tbufsize)) == NULL)
				return (0);
		}
		return (1);
	}
	if (count <= 0)		/* If no match (TERM not found) */
		bp[0] = '\0';	/* clear termcap buffer		*/
	return (count);
}

/*
 * Set the termcap flags.
 * It allows e.g. to prevent tgetent() from following tc= entries
 * and from modifying the co# and li# entries.
 * This is a libxtermcap extension.
 */
EXPORT int
tcsetflags(flags)
	int	flags;
{
	int	oflags = tflags;

	tflags = flags;
	return (oflags);
}

/*
 * Return the current buffer that holds the parsed termcap entry.
 * This function is needed if the buffer is allocated and a user
 * likes to do own string parsing on the buffer.
 * This is a libxtermcap extension.
 */
EXPORT char *
tcgetbuf()
{
	return (tbuf);
}

LOCAL int
tchktc()
{
	register	char	*ep;
	register	char	*np;
	register	char	*name;
			char	tcbuf[TMAX];
			char	*otbuf = tbuf;
			int	otbufsize = tbufsize;
			BOOL	otbufmalloc = tbufmalloc;
			BOOL	needfree;
			char	*xtbuf;
			int	ret;

	if (tbuf == NULL)
		return (0);

	ep = tbuf + strlen(tbuf) - 2;
	while (*--ep != ':') {
		if (ep < tbuf) {
			write(STDERR_FILENO, _ebad, sizeof (_ebad) - 1);
			return (0);
		}
	}
	ep++;
	if (ep[0] != 't' || ep[1] != 'c' || (tflags & TCF_NO_TC) != 0)
		goto out;

	ep = tfind(tbuf, _tc);
	if (ep == NULL || *ep != '=') {
		write(STDERR_FILENO, _ebad, sizeof (_ebad) - 1);
		return (0);
	}
	ep -= 2;				/* Correct for propper append*/
	strncpy(tcbuf, &ep[2], sizeof (tcbuf));
	name = tcbuf;
	name[sizeof (tcbuf)-1] = '\0';

	do {
		name++;
		for (np = name; *np; np++)
			if (*np == ':')
				break;
		*np = '\0';
		if (++loopcount > MAXLOOP) {
			write(STDERR_FILENO, _eloop, sizeof (_eloop) - 1);
			return (0);
		}
		tbufmalloc = FALSE;		/* Do not free buffer now! */
		ret = tgetent(NULL, name);
		*np = ':';
		xtbuf = tbuf;
		needfree = tbufmalloc;
		tbuf = otbuf;
		tbufsize = otbufsize;
		tbufmalloc = otbufmalloc;
		loopcount = 0;
		if (ret != 1) {
			if (needfree && xtbuf != NULL)
				free(xtbuf);
			return (ret);
		}
		np = tskip(xtbuf);	/* skip over the name part */
		/*
		 * Add nullbyte and 14 bytes for the space needed by tgetsize()
		 */
		ret = ep - otbuf + strlen(np) + 1 + TSIZE_SPACE;
		if (ret >= (unsigned)(tbufsize-1)) {
			if (tbufmalloc) {
				tbufsize = ret;
				if ((otbuf = trealloc(otbuf, tbufsize)) != NULL) {
					ep = otbuf + (ep - tbuf);
					tbuf = otbuf;
				} else {
					if (needfree && xtbuf != NULL)
						free(xtbuf);
					return (0);
				}
			} else {
				write(STDERR_FILENO, _etoolong,
							sizeof (_etoolong) - 1);
				ret = tbufsize - 1 - (ep - otbuf);
				if (ret < 0)
					ret = 0;
				np[ret] = '\0';
			}
		}
		strcpy(ep, np);
		ep += strlen(ep);
		if (needfree && xtbuf != NULL)
			free(xtbuf);

	} while ((name = tfind(name, _tc)) != NULL && *name == '=');
out:
#if	defined(TIOCGSIZE) || defined(TIOCGWINSZ)
	if ((tflags & TCF_NO_SIZE) == 0)
		tgetsize();
#endif
	if ((tflags & TCF_NO_STRIP) == 0)
		tstrip();
	return (1);
}

/*
 * Check if the current 'tbuf' contains a termcap entry for a terminal
 * that matches 'name'.
 */
LOCAL BOOL
tmatch(name)
	char	*name;
{
	register	char	*np;
	register	char	*ep;

	if (tbuf == NULL)
		return (FALSE);

	ep = tbuf;
	if (*ep == '#')					/* Kommentar */
		return (FALSE);
	for (; ; ep++) {
		for (np = name; *np; ep++, np++)	/* Solange name	*/
			if (*ep != *np)			/* gleich ist	*/
				break;
		if (*np == '\0') {			/* Name am Ende */
			if (*ep == '|' || *ep == ':' || *ep == '\0')
				return (TRUE);
		}
		while (*ep && *ep != '|' && *ep != ':')	/* Rest dieses	*/
			ep++;				/* Namens	*/
		if (*ep == ':' || *ep == '\0')
			return (FALSE);
	}
}

/*
 * Skip past next ':'.
 * If the are two consecutive ':', the returned pointer may point to ':'.
 */
LOCAL char *
tskip(ep)
	register	char	*ep;
{
	while (*ep) {
		if (*ep++ == ':')
			return (ep);	/* return first ':'	*/
	}
	return (ep);			/* not found		*/
}

/*
 * Find a two charater entry in string that is found in 'ep'.
 * Return the character that follows the two character entry (if found)
 * or NULL if the entry could not be found.
 */
LOCAL char *
tfind(ep, ent)
	register	char	*ep;
			char	*ent;
{
	register	char	e0 = ent[0];
	register	char	e1 = ent[1];

	for (;;) {
		ep = tskip(ep);
		if (*ep == '\0')
			break;
		if (*ep == ':')
			continue;
		if (e0 != *ep++)
			continue;
		if (*ep == '\0')
			break;
		if (e1 != *ep++)
			continue;
		return (ep);
	}
	return ((char *) NULL);
}

/*
 * Search for a numeric entry in form 'en#123' to represent a decimal number
 * or 'en#0123' to represent a octal number.
 * Return numeric value or -1 if found 'en@'.
 */
EXPORT int
tgetnum(ent)
	char	*ent;
{
	register	Uchar	*ep = (Uchar *)tbuf;
	register	int	val;
	register	int	base;

	if (tbuf == NULL)
		return (-1);

	for (;;) {
		ep = (Uchar *)tfind((char *)ep, ent);
		if (!ep || *ep == '@')
			return (-1);
		if (*ep == '#')
			break;
	}
	base = 10;
	if (*++ep == '0')
		base = 8;
	for (val = 0; isdigit(*ep); ) {
		val *= base;
		val += (*ep++ - '0');
	}
	return (val);
}

/*
 * Search for a boolean entry in form 'en' to represent a TRUE value
 * or 'en@' to represent a FALSE value.
 * An entry in the form 'en@' is mainly used to overwrite similar entries
 * found later from a tc= entry.
 */
EXPORT BOOL
tgetflag(ent)
	char	*ent;
{
	register	char	*ep = tbuf;

	if (tbuf == NULL)
		return (FALSE);

	for (;;) {
		ep = tfind(ep, ent);
		if (!ep || *ep == '@')
			return (FALSE);
		if (*ep == '\0' || *ep == ':')
			return (TRUE);
	}
}

/*
 * Search for a string entry in form 'en=val'.
 * Return string parameter or NULL if found 'en@'.
 */
EXPORT char *
tgetstr(ent, array)
	char	*ent;
	char	*array[];
{
	register	char	*ep = tbuf;
			char	*np;
			char	buf[TMAX];

	if (tbuf == NULL)
		return ((char *)0);

	if (array == NULL) {
		np = buf;
		array = &np;
	}
	for (;;) {
		ep = tfind(ep, ent);
		if (!ep || *ep == '@')
			return ((char *) NULL);
		if (*ep == '=') {
			ep = tdecode(++ep, array);
			if (ep == buf) {
				ep = tmalloc(strlen(ep)+1);
				if (ep != NULL)
					strcpy(ep, buf);
			}
			return (ep);
		}
	}
}

#define	isoctal(c)	((c) >= '0' && (c) <= '7')
/*
 * Decode a string and replace the escape sequences by what
 * they mean (e.g. \E by ESC).
 * The space used to hold the decoded string is taken from
 * the second parameter.
 * Note that old 'vi' implementations limit the total space for
 * all decoded strings to 256 bytes.
 */
EXPORT char *
tdecode(pp, array)
			char	*pp;
			char	*array[];
{
			int	i;
	register	Uchar	c;
	register	Uchar	*ep = (Uchar *)pp;
	register	Uchar	*bp;
	register	Uchar	*tp;

	bp = (Uchar *)array[0];

	for (; (c = *ep++) && c != ':'; *bp++ = c) {
		if (c == '^') {
			c = *ep++ & 0x1f;
		} else if (c == '\\') {
			c = *ep++;
			if (isoctal(c)) {
				for (c -= '0', i = 3; --i > 0 && isoctal(*ep); ) {
					c <<= 3;
					c |= *ep++ - '0';
				}
				/*
				 * Terminfo maps NULL chars to 0200
				 */
				if (c == '\0')
					c = '\200';
			} else for (tp = (Uchar *)_quotetab; *tp; tp++) {
				if (*tp++ == c) {
					c = *tp;
					break;
				}
			}
		}
	}
	*bp++ = '\0';
	ep = (Uchar *)array[0];
	array[0] = (char *)bp;
	return ((char *)ep);
}

#if	defined(TIOCGSIZE) || defined(TIOCGWINSZ)

/*
 * Get the current size of the terminal (window) and insert the
 * apropriate values for 'li#' and 'co#' before the other terminal
 * capabilities.
 */
LOCAL void
tgetsize()
{
#ifdef	TIOCGWINSZ
	struct		winsize ws;
#else
	struct		ttysize	ts;
#endif
	register	int	lines = 0;
	register	int	cols = 0;
	register	char	*ep;
	register	char	*lp;
	register	char	*cp;
			int	len;

	if (tbuf == NULL)
		return;

#ifdef	TIOCGWINSZ
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&ws) >= 0) {
		lines = ws.ws_row;
		cols = ws.ws_col;
	}
#else
	if (ioctl(STDOUT_FILENO, TIOCGSIZE, (char *)&ts) >= 0) {
		lines = ts.ts_lines;
		cols = ts.ts_cols;
	}
#endif
	if (lines == 0 || cols == 0 || lines > 999 || cols > 999)
		return;

	len = strlen(tbuf) + 1 + TSIZE_SPACE;
	if (len > tbufsize) {
		if (tbufmalloc) {
			tbufsize = len;
			if ((tbuf = trealloc(tbuf, tbufsize)) == NULL)
				return;
		} else {
			return;
		}
	}
	ep = tskip(tbuf);		/* skip over the name part */
	/*
	 * Backwards copy to create a gap for the string we like to add.
	 */
	lp = &tbuf[len-1-TSIZE_SPACE];	/* The curent end of the buffer */
	for (cp = &lp[TSIZE_SPACE]; lp >= ep; cp--, lp--)
		*cp = *lp;

	*ep++ = 'l';
	*ep++ = 'i';
	*ep++ = '#';
	ep = tinsint(ep, lines);
	*ep++ = ':';
	*ep++ = 'c';
	*ep++ = 'o';
	*ep++ = '#';
	ep = tinsint(ep, cols);
	*ep++ = ':';
	while (ep <= cp)
		*ep++ = ' ';
	*--ep = ':';
}

/*
 * Delete duplicate named numeric entries.
 */
LOCAL void
tdeldup(ent)
			char	*ent;
{
	register	char	*ep;
	register	char	*p;

	if (tbuf == NULL)
		return;

	if ((ep = tfind(tbuf, ent)) != NULL) {
		while ((ep = tfind(ep, ent)) && *ep == '#') {
			p = ep;
			while (*p)
				if (*p++ == ':')
					break;
			ep -= 3;
			strcpy(ep, --p);
		}
	}
}

/*
 * Insert a number into a terminal capability buffer.
 */
LOCAL char *
tinsint(ep, i)
	register	char	*ep;
	register	int	i;
{
	register	char	c;

	if ((c = i / 100) != 0) {
		*ep++ = c + '0';
		i %= 100;
		if (i / 10 == 0)
			*ep++ = '0';
	}
	if ((c = i / 10) != 0)
		*ep++ = c + '0';
	*ep++ = i % 10 + '0';
	return (ep);
}

#endif	/* defined(TIOCGSIZE) || defined(TIOCGWINSZ) */

/*
 * Strip down the termcap entry to make it as short as possible.
 * This is done by first deleting duplicate 'li#' and 'co#' entries
 * and then removing succesive ':' chars and spaces between ':'.
 */
LOCAL void
tstrip()
{
	register	char	*bp = tbuf;
	register	char	*p;

	if (bp == NULL)
		return;

#if	defined(TIOCGSIZE) || defined(TIOCGWINSZ)
	tdeldup("li");
	tdeldup("co");
#endif

#ifdef	needed
	while (*bp) {
		if (*bp++ == ':') {
			if (*bp == ':') {
				p = bp;
				while (*p == ':')
					p++;
				strcpy(bp, p);
			}
		}
	}
	bp = tbuf;
#endif
	while (*bp) {
		if (*bp++ == ':') {
			if (*bp == ':' || *bp == ' ' || *bp == '\t') {
				p = bp;
				while (*p)
					if (*p++ == ':')
						break;
				strcpy(bp--, p);
			}
		}
	}
}

LOCAL char *
tmalloc(size)
	int	size;
{
	char	*ret;

	if ((ret = malloc(size)) != NULL)
		return (ret);
	write(STDERR_FILENO, _enomem, sizeof (_enomem) - 1);
	return ((char *)NULL);
}

LOCAL char *
trealloc(p, size)
	char	*p;
	int	size;
{
	char	*ret;

	if ((ret = realloc(p, size)) != NULL)
		return (ret);
	write(STDERR_FILENO, _enomem, sizeof (_enomem) - 1);
	return ((char *)NULL);
}
