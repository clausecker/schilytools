/* @(#)input.c	1.41 18/10/07 Copyright 1985-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)input.c	1.41 18/10/07 Copyright 1985-2018 J. Schilling";
#endif
/*
 *	bsh command interpreter - Input handling & Alias/Macro Expansion
 *
 *	Copyright (c) 1985-2018 J. Schilling
 *	Copyright (c) 2022 the schilytools project
 *
 *	Exported functions:
 *		setinput(f)	replaces the current input file
 *		nextch()	fetches next char and places it into 'delim'
 *		peekch()	peek next char - do not change 'delim'
 *		ungetch(c)	pushes 'c' back into the input fstream
 *		nextline()	return next line from input
 *		pushline(s)	pushes back a line to input
 *		quote()		increase quoting level
 *		unquote()	decrease quoting level
 *		quoting()	return current quoting level
 *		begina(beg)	set begin alias flag for alias expansion
 *		sclearerr()	clear any error condition on input FILE *
 *
 *	Exported vars:
 *		int delim	last character read by nextch()
 *
 *	Imported functions:
 *		ab_value()	from asym.c
 *		getpwdir()	from bsh.c (if ~ expansion is enabled)
 *		make_line()	from inputc.c
 *		makestr()	from strsubs.c
 *		mkfstream()	from libschily::fstream.c
 *		mypwhome()	from bsh.c (if ~ expansion is enabled)
 *		fspushcha()	from libschily::fstream.c
 *		fspushstr()	from libschily::fstream.c
 *		fssetfile()	from libschily::fstream.c
 *		fsgetc()	from libschily::fstream.c
 *		syntax()	from parse.c
 *
 *	Imported vars:
 *		int ctlc	from bsh.c
 *		erestricted	from str.c
 *		BOOL noslash	from bsh.c
 *		nullstr		from str.c
 *		int prompt	from bsh.c
 *		slash		from str.c
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

#include <schily/stdio.h>
#include "bsh.h"
#include "abbrev.h"
#include "str.h"
#include "strsubs.h"
#include "ctype.h"
#include <schily/fstream.h>
#include <schily/unistd.h>	/* getpid() */
#include <schily/stdlib.h>

#define	DOL		/* include '$' var expansion */
#ifdef	NO_DOL
#undef	DOL
#endif

#define	MAX_ALIAS_NEST	64
#define	IWORD_SIZE	1024
LOCAL char fsep[] = " \t\n\\':$~/|&;()><%\"=-"; /* Field separators for Expansion */

EXPORT int	delim = EOF;			/* Current input character */

extern BOOL	noslash;
extern char	*inithome;
extern pid_t	lastbackgrnd;

LOCAL fstream	*instrm = (fstream *) NULL;	/* Alias expanded input fstream */
LOCAL fstream	*rawstrm = (fstream *) NULL;	/* Unexpanded input fstream */
LOCAL int	qlevel = 0;			/* Current quoting level */
LOCAL int	dqlevel = 0;			/* Current double quoting level */
LOCAL int	xqlevel = 0;			/* At the begin of "" quoting */
LOCAL int	begalias = 0;			/* Begin aliases allowed? */
LOCAL int	nextbegin = 0;			/* Begin aliases on next word */

LOCAL	int	_fsgetc		__PR((fstream *is));
LOCAL	int	fillbuf		__PR((int c, char *wbuf, fstream *is));
LOCAL	int	readchar	__PR((fstream *fsp));
EXPORT	int	nextch		__PR((void));
EXPORT	int	peekch		__PR((void));
EXPORT	void	ungetch		__PR((int c));
EXPORT	char	*nextline	__PR((void));
EXPORT	FILE	*setinput	__PR((FILE * f));
EXPORT	void	sclearerr	__PR((void));
EXPORT	void	pushline	__PR((char *s));
EXPORT	void	quote		__PR((void));
EXPORT	void	unquote		__PR((void));
EXPORT	int	quoting		__PR((void));
EXPORT	void	dquote		__PR((void));
EXPORT	void	undquote	__PR((void));
EXPORT	int	dquoting	__PR((void));
EXPORT	void	xquote		__PR((void));
EXPORT	void	unxquote	__PR((void));
EXPORT	int	begina		__PR((BOOL beg));
LOCAL	int	input_expand	__PR((fstream * os, fstream * is));

/*
 * Wrapper for fsgetc() that calls fspop() is needed.
 * This is needed since we added a fspush() to implement the push
 * of data for the input from an alias replacement. fspush() is needed
 * in order to keep track of the end of the related replacement text.
 */
LOCAL int
_fsgetc(is)
	register fstream	*is;
{
	int	c;

again:
	c = fsgetc(is);		/* The real stream lib ibterface */
	if (c == EOF) {
		fstream	*pushed;

		if ((pushed = fspushed(is)) != NULL) {
			/*
			 * If we had a pushed begin alias that ends in
			 * a space or a TAB, re-enable begin aliases.
			 */
			if (pushed->fstr_flags & 1)
				begina(TRUE);
			fspop(is);
			goto again;
		}
		return (EOF);
	}
	return (c);
}
#define fsgetc _fsgetc

LOCAL int
fillbuf(c, wbuf, is)
	register int		c;
	register char		*wbuf;
	register fstream	*is;
{
	register char	*s;

	s = wbuf;
	do {
		*s++ = (char)c;
		c = fsgetc(is);
	} while (s < wbuf + IWORD_SIZE && c != EOF && !strchr(fsep, (char)c));
	*s = '\0';
#ifdef SINGLEQUOTE
	if (c != (int)'\'')
#endif
		fspushcha(is, c);	/* Not yet wanted - push back */

	return (c);
}

LOCAL int
readchar(fsp)
	register fstream	*fsp;
{
#ifdef	INTERACTIVE
extern int	prompt;

	pushline(get_line(prompt++, fsp->fstr_file));
	return (fsgetc(fsp));
#else
	return (getc(fsp->fstr_file));	/* read from FILE */
#endif
}

/*
 * Fetch next expanded character from input and put it into 'delim'
 */
EXPORT int
nextch()
{
	int	c;

	c = fsgetc(instrm);

	/*
	 * XXX: As long as we do not add special code, the handling of ^C will
	 * XXX: cause a memory leak from the laready parsed code.
	 */
	if (c == EOF && delim == 3) {		/* ^C signalled from editor */
		delim = '\n';			/* avoid reading 'till EOL */
		raisecond(sn_ctlc, (long)NULL);	/* raise the condition */
		return (delim);
	}

	if ((delim = c) == '/' && noslash)
		syntax(erestricted, slash);
#ifdef DEBUG
	putc(delim, stderr);
#endif
	return (delim);
}

/*
 * Peek next char but do not change 'delim'
 */
EXPORT int
peekch()
{
	int	odelim = delim;
	int	this = nextch();

	ungetch(this);
	delim = odelim;

	return (this);
}

/*
 * Push back a single character
 */
EXPORT void
ungetch(c)
	int	c;
{
	fspushcha(instrm, c);
}

/*
 * Return next line in allocated string
 */
#define	f_nextch	((int (*)__PR((FILE *)))nextch)

EXPORT char *
nextline()
{
	return (make_line(f_nextch, (void *)0));
}

/*
 * Set up file from where the inout should be read,
 * returns the old FILE * value.
 */
#define	f_input_expand	((fstr_fun)input_expand)

EXPORT FILE *
setinput(f)
	FILE	*f;
{
	/*
	 * The raw fstream contains the unprocessed input.
	 */
	if (rawstrm == (fstream *) NULL)
		rawstrm = mkfstream(f, (fstr_fun)0, readchar, (fstr_efun)berror);
	else
		f = fssetfile(rawstrm, f);

	/*
	 * The secondary fstream is a store for the processed input.
	 */
	if (instrm == (fstream *) NULL)			/* Pfusch in fsgetc */
		instrm = mkfstream((FILE *) rawstrm, f_input_expand, (fstr_rfun)0,
							(fstr_efun)berror);
	qlevel = 0;
	return (f);
}

/*
 * clear error in rawstream
 */
EXPORT void
sclearerr()
{
	clearerr(rawstrm->fstr_file);
}

/*
 * Push back a complete line
 */
EXPORT void
pushline(s)
	char	*s;
{
	if (s && rawstrm != (fstream *) NULL) {
		fspushcha(rawstrm, delim);
		fspushstr(rawstrm, s);
	}
}


/*
 * Increase quoting level by one
 */
EXPORT void
quote()
{
#ifdef DEBUG
	fprintf(stderr, "QUOTE\n");
	fflush(stderr);
#endif
	qlevel++;
}

/*
 * Decrease quoting level by one
 */
EXPORT void
unquote()
{
#ifdef DEBUG
	fprintf(stderr, "UNQUOTE\n");
	fflush(stderr);
#endif
	qlevel--;
}

EXPORT int
quoting()
{
	return (qlevel);
}

/*
 * Increase double quoting level by one
 */
EXPORT void
dquote()
{
#ifdef DEBUG
	fprintf(stderr, "DQUOTE\n");
	fflush(stderr);
#endif
	dqlevel++;
}

/*
 * Decrease double quoting level by one
 */
EXPORT void
undquote()
{
#ifdef DEBUG
	fprintf(stderr, "UNDQUOTE\n");
	fflush(stderr);
#endif
	dqlevel--;
}

EXPORT int
dquoting()
{
	return (dqlevel);
}

/*
 * Increase X-double quoting level by one
 */
EXPORT void
xquote()
{
	xqlevel++;
}

/*
 * Decrease X-double quoting level by one
 */
EXPORT void
unxquote()
{
	xqlevel--;
}


/*
 * Set Begin Alias expansion flag based on parameter
 */
EXPORT int
begina(beg)
	BOOL	beg;
{
	int	obegalias = begalias;

#ifdef DEBUG
	fprintf(stderr, "begalias = %d\n", beg);
	fflush(stderr);
#endif
	begalias = beg?AB_BEGIN:0;

	return (obegalias != 0);
}

/*
 * Get the current state for Begin Alias expansion
 */
EXPORT int
getbegina()
{
	return (begalias != 0);
}

/*
 * Set Begin Alias expansion flag based on whether the alias
 * layer previously detected an alias that ends in white space.
 */
EXPORT int
setbegina()
{
	int	obegalias = begalias;

	begina(nextbegin);
	nextbegin = 0;

	return (obegalias != 0);
}


/*
 * Apply macro expansion on Input while copying data from 'is' to 'os'
 */
LOCAL int
input_expand(os, is)
	fstream	*os;
	fstream	*is;
{
	register int	c;
	register char	*val;
		char	buf[IWORD_SIZE+1];
		int	loopcnt	= MAX_ALIAS_NEST;
#ifdef	DOL
		int	itmp	= 0;
		int	vectype;
#endif

	for (;;) {
		c = fsgetc(is);
		if (c == EOF) {
			return (EOF);
		}
		if (c == '\\') {
			c = fsgetc(is);
			if (c != '\n' || qlevel) {
				fspushcha(os, c);
				/*if (qlevel)*/ /* old Quoting */
				fspushcha(os, '\\');
				begina(FALSE);
			} else {
				fspushcha(os, ' ');
			}
			break;
		} else if (c == 3 && ctlc) {	/* ^C signalled from editor */
			return (EOF);
		} else if (qlevel != 0) {
			/*
			 * In quote mode just copy data
			 */
			fspushcha(os, c);
			begina(FALSE);
			break;
		} else if (!strchr(fsep, c) && !ctlc) {
			void	*seen = 0;
			fstream	*push = 0;

			/*
			 * Could be a word, so try alias expansion
			 */
			c = fillbuf(c, buf, is);
			if ((push = fspushed(is)) != NULL)
				seen = push->fstr_auxp;
			if ((val = ab_value(LOCAL_AB, buf, &seen, begalias)) == NULL)
				val = ab_value(GLOBAL_AB, buf, &seen, begalias);
#ifdef DEBUG
			fprintf(stderr, "expanding '%s': ", buf);
			fflush(stderr);
			if (val == NULL)
				fprintf(stderr, "-> NONE\n");
			else
				fprintf(stderr, "-> '%s'\n", val);
			fflush(stderr);
#endif
			if (val != NULL) {
				if (--loopcnt >= 0) {
					fstream	*pushed = fspush(is,
							    (fstr_efun)berror);

					if (pushed)
						pushed->fstr_auxp = seen;
					/*
					 * If a begin alias was expanded and the
					 * new text ends in a space or tab, the
					 * next word will be a begin alias too.
					 */
					if (pushed && begalias != 0) {
						int len = strlen(val);

						if (len > 0 &&
						    (val[len-1] == ' ' ||
						    val[len-1] == '\t')) {
							pushed->fstr_flags |= 1;
						}
					}
					fspushstr(pushed?pushed:is, val);
				} else {
					syntax("Alias loop on '%s'.", buf);
					break;
				}
			} else {
				/*
				 * No replacement text found, so just push
				 * the original text.
				 */
				fspushstr(os, buf);
				begina(FALSE);
				break;
			}
		} else if (c == '~' && !ctlc) {
			c = fsgetc(is);
			if (!strchr(fsep, c)) {
				c = fillbuf(c, buf, is);
				val = getpwdir(buf);
				if (!val) {
					fspushstr(is, buf);
					val = makestr("~");
				}
			} else {
				fspushcha(is, c);
				if (!(val = mypwhome()))
					val = makestr(nullstr);
			}
			fspushstr(os, val);
			free(val);
			begina(FALSE);
			break;
#ifdef	DOL
		} else if (c == '$' && !ctlc) {
			c = fsgetc(is);
			if (!isdigit(c) && c != 'r' && c != '*' && c != '@') {
				if (c == '#') {
					val = malloc(10);
					sprintf(val, "%d", vac);
					fspushstr(os, val);
					free(val);
				} else if (c == '!' && lastbackgrnd) {
					val = malloc(10);
					sprintf(val, "%ld", (long)lastbackgrnd);
					fspushstr(os, val);
					free(val);
				} else if (c == '$') {
					val = malloc(10);
					sprintf(val, "%ld", (long)getpid());
					fspushstr(os, val);
					free(val);
				} else if (c == '?') {
					val = malloc(10);
					sprintf(val, "%d", ex_status);
					fspushstr(os, val);
					free(val);
				} else if (!strchr(fsep, c)) {
					c = fillbuf(c, buf, is);
					if ((val = getcurenv(buf)) != NULL) {
						fspushstr(is, val);
					} else {
						fspushstr(is, buf);
						fspushcha(os, '$');
					}
				} else {
					fspushcha(is, c);
					fspushcha(os, '$');
				}
				begina(FALSE);
				break;
			}
			vectype = 0;
			if (c == '*') {
				/*
				 * "$*" -> "$1 $2 ..."
				 */
				for (c = vac; c > 1; ) {
					fspushstr(os, vav[--c]);
					if (c > 1)
						fspushstr(os, " ");
					begina(FALSE);
				}
				break;
			} else if (c == '@') {
				/*
				 * "$@" -> "$1" "$2" ...
				 */
				if (xqlevel > 0 && vac <= 1 &&
				    *is->fstr_bp == '"') {
					begina(FALSE);
					/*
					 * Signal our caller that this argument
					 * needs to be copletely wiped out.
					 */
					return (-2);
				}
				for (c = vac; c > 1; ) {
					fspushstr(os, vav[--c]);
					if (c > 1)
						fspushstr(os, dqlevel > 0?
								"\" \"":" ");
					begina(FALSE);
				}
				break;
			} else if (c == 'r') {
				vectype = c;
				c = fsgetc(is);
				if (!isdigit(c)) {
					fspushcha(is, c);
					fspushstr(os, "$r");
					break;
				}
			}
			itmp = 0;
			{
				BOOL	overflow = FALSE;
				int	maxmult = INT_MAX / 10;

				while (isdigit(c)) {
					c -= '0';
					if (itmp > maxmult)
						overflow = TRUE;
					itmp *= 10;
					if (INT_MAX - itmp < c)
						overflow = TRUE;
					itmp += c;
					c = fsgetc(is);
				}
				if (overflow)
					itmp = INT_MAX;
			}
			fspushcha(is, c);
			if (vectype == 'r') {
				/*
				 * "$r2" -> "$2" "$3" ...
				 */
				for (c = vac; c > itmp; ) {
					fspushstr(os, vav[--c]);
					if (c > itmp)
						fspushstr(os, dqlevel > 0?
								"\" \"":" ");
					begina(FALSE);
				}
				break;
			} else if (itmp >= 0 && itmp < vac) {
				fspushstr(os, vav[itmp]);
				begina(FALSE);
				break;
			}
#endif	/* DOL */
		} else {
			fspushcha(os, c);
			if (!iswhite(c))
				begina(FALSE);
			break;
		}
	}
	return (0);
}
