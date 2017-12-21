/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2000 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)word.c	1.22	05/09/13 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2017 J. Schilling
 *
 * @(#)word.c	1.87 17/12/09 2008-2017 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)word.c	1.87 17/12/09 2008-2017 J. Schilling";
#endif

/*
 * UNIX shell
 */

#include	"sym.h"
#ifdef	DO_SYSALIAS
#include	"abbrev.h"
#endif
#ifdef	SCHILY_INCLUDES
#include	<schily/errno.h>
#include	<schily/fcntl.h>
#ifdef	INTERACTIVE
#include	<schily/shedit.h>
#endif
#else
#include	<errno.h>
#include	<fcntl.h>
#endif
#ifdef	DO_TILDE
#include	<schily/pwd.h>
#endif

	int		word	__PR((void));
static	unsigned char	*match_word __PR((unsigned char *argp,
						unsigned int c,
						unsigned int d,
						unsigned int *wordcp));
#ifdef	DO_DOL_PAREN
static	unsigned char	*dolparen	__PR((unsigned char *argp));
static	unsigned char	*match_cmd	__PR((unsigned char *argp));
	unsigned char	*match_arith	__PR((unsigned char *argp));
#endif
static	unsigned char	*match_literal	__PR((unsigned char *argp));
static	unsigned char	*match_block __PR((unsigned char *argp,
						unsigned int c,
						unsigned int d));
	unsigned int	skipwc	__PR((void));
	unsigned int	nextwc	__PR((void));
	unsigned char	*readw	__PR((wchar_t d));
	unsigned int	readwc	__PR((void));
static int		readb	__PR((struct fileblk *, int, int));
#ifdef	INTERACTIVE
static	BOOL		chk_igneof __PR((void));
static	int		xread	__PR((int f, char *buf, int n));
#endif
#ifdef	DO_TILDE
static unsigned char	*do_tilde __PR((unsigned char *arg));
#endif

/* ========	character handling for command lines	======== */

#ifdef	DO_SYSALIAS
static	void		*seen;	/* Structure to track recursive alias calls */
#endif

int
word()
{
	unsigned int	c, d;

	wdnum = 0;
	wdset &= ~KEYFLAG;

	/*
	 * We first call readwc() in order to make sure that the history editor
	 * was called already and malloc() will not be called while we are
	 * working on a "local stack". We asume that after readwc() was called,
	 * no further edit related malloc() call will happen and it is safe to
	 * call locstak() to create a local stack.
	 */
	/* CONSTCOND */
	while (1) {
		while (c = nextwc(), space(c))		/* skipc() */
			/* LINTED */
			;

		if (c == COMCHAR) {			/* Skip comment */
			while ((c = readwc()) != NL && c != EOF)
				/* LINTED */
				;
			peekc = c;			/* NL or EOF */
		} else {
			break;	/* out of comment - white space loop */
		}
	}
	if (!eofmeta(c) || (c == '^' && (flags2 & posixflg))) {
		struct argnod	*arg = (struct argnod *)locstak();
		unsigned char	*argp = arg->argval;
		unsigned int	wordc;	/* To restore c from  match_word() */

		/*
		 * As eofmeta(c) includes NL and EOF, we will not be here in
		 * case that peekc was set.
		 */
		argp = match_word(argp, c, MARK, &wordc);
		arg = (struct argnod *)endstak(argp);
		if (!letter(arg->argval[0]))
			wdset &= ~KEYFLAG;

		c = wordc;		/* Last c from inside match_word() */
		if (arg->argval[1] == 0 &&
		    (d = arg->argval[0], digit(d)) &&
#ifdef	DO_FDPIPE
		    (c == '>' || c == '<' ||
		    ((flags2 & fdpipeflg) && c == '|'))) {
#else
		    /* CSTYLED */
		    (c == '>' || c == '<')) {
#endif
			word();
			/*
			 * wdnum is cleared when entering word() but may
			 * already contain IOSTRIP here. Keep the bug for
			 * the old Bourne Shell variant.
			 */
#ifndef	DO_IOSTRIP_FIX
			wdnum = d - '0';
#else
			wdnum |= d - '0';
#endif
		} else { /* check for reserved words */
			if (reserv == FALSE ||
			    (wdset & IN_CASE) ||
			    (wdval = syslook(arg->argval,
			    reserved, no_reserved)) == 0) {
				wdval = 0;
			}
#ifdef	DO_TIME
			else if (wdval == TIMSYM) {
				/*
				 * POSIX requires to support "time -p command",
				 * so check for "time -" and disable the
				 * "time" reserved word if needed.
				 */
				while (c = nextwc(), space(c))	/* skipc() */
					/* LINTED */
					;
				if (c)
					peekn = c;
				if (c == '-')
					wdval = 0;
			}
#endif
			/* set arg for reserved words too */
			wdarg = arg;
		}
	} else if (dipchar(c)) {
		if ((d = nextwc()) == c) {
			wdval = c | SYMREP;
			if (c == '<') {
				if ((d = nextwc()) == '-')
					wdnum |= IOSTRIP;
				else
					peekn = d | MARK;
			}
		} else {
			peekn = d | MARK;
			wdval = c;
		}
	} else {
		if ((wdval = c) == EOF)
			wdval = EOFSYM;
		if (iopend && eolchar(c)) {
			struct ionod *tmp_iopend;
			tmp_iopend = iopend;
			iopend = 0;
			copy(tmp_iopend);
		}
	}

#ifdef	DO_SYSALIAS
	/*
	 * We previously did not expand aliases while in an eval(1) call.
	 * Since all other shells expand aliases inside an eval call, we
	 * now do it as well.
	 */
	if (wdval == 0) {
			char	*val;
		extern	int	abegin;
			int	aflags = abegin > 0 ? AB_BEGIN:0;

		if ((val = ab_value(LOCAL_AB, (char *)wdarg->argval,
		    &seen, aflags)) == NULL) {
			val = ab_value(GLOBAL_AB, (char *)wdarg->argval,
			    &seen, aflags);
		}
		if (val) {
			struct filehdr *fb = alloc(sizeof (struct filehdr));

			push((struct fileblk *)fb);	/* Push tmp filehdr */
			estabf(UC val);			/* Install value    */
			standin->fdes = -2;		/* Make it auto-pop */
			standin->peekn = peekn;		/* Remember peekn   */
			peekn = 0;			/* for later use    */

			if (abegin > 0) {		/* Was a begin alias */
				size_t	len = strlen(val);

				if (len > 0 &&
				    (val[len-1] == ' ' || val[len-1] == '\t'))
					standin->fdes = -3; /* begin alias */
			}

			return (word());		/* Parse replacement */
		}
	}
	seen = NULL;
#endif
	reserv = FALSE;
	return (wdval);
}

/*
 * Match and copy the next word from the input stream.
 */
static unsigned char *
match_word(argp, c, d, wordcp)
	unsigned char	*argp;		/* Output pointer	*/
	unsigned int	c;		/* Last read character	*/
	unsigned int	d;		/* Delimiter or MARK	*/
	unsigned int	*wordcp;	/* Pointer to return c	*/
{
	unsigned int	cc;
	unsigned char	*pc;
	int		alpha = 1;
	int		parm = 0;
#ifdef	DO_TILDE
	int		iskey = 0;
	int		tilde = -1;
extern	int		abegin;

	if (c == '~')
		tilde = argp - stakbot;
#endif

	do {
		if (c == LITERAL) {	/* '\'' */
			argp = match_literal(argp);
		} else {
			if (c == 0) {
				GROWSTAK(argp);
				*argp++ = 0;
				parm = 0;	/* EOF -> abort ${..} scan */
			} else {
				pc = readw(c);
				while (*pc) {
					GROWSTAK(argp);
					*argp++ = *pc++;
				}
			}
			if (d != MARK) {
				if (c == 0 || c == d)
					break;
				if (c == NL)
					chkpr();
			}
			if (c == '\\') {
				if ((cc = readwc()) == 0) {
					GROWSTAK(argp);
					*argp++ = 0;
				} else {
					pc = readw(cc);
					while (*pc) {
						GROWSTAK(argp);
						*argp++ = *pc++;
					}
				}
			}
			if (d == MARK) {
				if (c == '=') {
					wdset |= alpha;
#ifdef	DO_TILDE
					if (abegin > 0 || flags & keyflg) {
						tilde = argp - stakbot;
						iskey++;
					}
#endif
				}
				if (!alphanum(c))
					alpha = 0;
			}
#ifdef	DO_DOL_PAREN
			if (c == DOLLAR) {
				argp = dolparen(argp);
				if (peekn == ('{' | MARK))
					parm++;
			} else if (c == '}' && parm) {
				parm--;
			} else
#endif
			if (qotchar(c)) {	/* '`' or '"' */
				argp = match_block(argp, c, c);
			}

#ifdef	DO_TILDE
			if (tilde >= 0) {
				c = readwc();
				peekc = c | MARK;
				if (c == '/' || c == ':' || eofmeta(c)) {
					unsigned char	*val;

					GROWSTAK(argp);
					*argp = '\0';
					val = do_tilde(stakbot+tilde);
					if (val)
						argp = movstrstak(val,
								stakbot+tilde);
					tilde = -1;
				}
			} else if (c == ':' && iskey)
				tilde = argp - stakbot;
#endif
		}
	} while ((c = nextwc(), d != MARK || !eofmeta(c)) || parm > 0 ||
				(c == '^' && (flags2 & posixflg)));

	if (d == MARK) {
		/*
		 * We need to remember c for word() as c may have MARK set.
		 */
		*wordcp = c;
		peekn = c | MARK;
	}
	return (argp);
}

#ifdef	DO_DOL_PAREN
static unsigned char *
dolparen(argp)
	unsigned char	*argp;		/* Output pointer	*/
{
	unsigned int	c;		/* Last read character	*/

	/*
	 * Check for '$('
	 */
	if ((c = nextwc()) == '(') {
		/*
		 * Check for '$(('
		 */
		if ((c = nextwc()) == '(') {
			argp = match_arith(argp);
		} else {
			peekn = c | MARK;
			argp = match_cmd(argp);
		}
	} else {
		peekn = c | MARK;
	}
	return (argp);
}

static unsigned char *
match_cmd(argp)
	unsigned char	*argp;		/* Output pointer	*/
{
	struct argnod	*arg;
	struct trenod	*tc;
	int		save_fd;
	struct ionod	*oiopend = iopend;
	int		owdnum = wdnum;
	int		owdset = wdset;

	/*
	 * Add "( " and make the string null terminated semi permanent.
	 * Note that the space is needed to avoid confusion with "$((".
	 */
	argp += 3;
	GROWSTAK(argp);
	argp -= 3;
	*argp++ = '(';
	*argp++ = ' ';
	*argp++ = 0;
	arg = (struct argnod *)endstak(argp);

	iopend = 0;
	tc = cmd(')', MTFLG | NLFLG | SEMIFLG);	/* Tell parser to stop at ) */
	iopend = oiopend;
	wdset = owdset;
	wdnum = owdnum;

	/*
	 * Convert the syntax tree back into a command line.
	 * Use prf() to get a command line with new-lines instead of ';'
	 * since our current parser does not walways grok ';' where a
	 * new line is ok.
	 */
	save_fd = setb(-1);
	prs_buff(arg->argval);		/* Copy begin of argument */
	prf(tc);			/* Convert cmd to string  */
	prs_buff(UC ")");		/* Add closing ) from $() */
	argp = stakbot;
	(void) setb(save_fd);
	argp = endb();
#ifdef	DOL_PAREN_DEBUG
	fprintf(stderr, "DO_PAREN parse->prf() '%s'\n", argp); fflush(stderr);
#endif

	/*
	 * Create new growable local stack and copy over the current text.
	 */
	arg = (struct argnod *)locstak();
	argp = movstrstak(argp, arg->argval);
	return (argp);
}

unsigned char *
match_arith(argp)
	unsigned char	*argp;		/* Output pointer	*/
{
	int		nest = 2;
	unsigned int	c;
	unsigned char	*pc;
	UIntptr_t	p = relstakp(argp);

	/*
	 * Add the "((".
	 */
	argp += 3;
	GROWSTAK(argp);
	argp -= 3;
	*argp++ = '(';
	*argp++ = '(';
	*argp = 0;
	while ((c = nextwc()) != '\0') {
		/*
		 * quote each character within
		 * single quotes
		 */
		pc = readw(c);
		while (*pc) {
			GROWSTAK(argp);
			*argp++ = *pc++;
		}
		if (c == '`') {
			argp = match_block(argp, c, c);
			continue;
		}
		if (c == NL) {
			chkpr();
		} else if (c == '(') {
			nest++;
		} else if (c == ')') {
			if (--nest == 0)
				break;
		} else if (c == DOLLAR) {
			argp = dolparen(argp);
			continue;
		}
	}
	GROWSTAK(argp);
	*argp = 0;
	if (nest != 0)		/* Need a generalized syntax error function */
		failed(absstak(p), synmsg); /* instead if calling failed()  */
	return (argp);
}
#endif

/*
 * Match and copy the next literal block (surrounded by '\'') from the
 * input stream.
 */
static unsigned char *
match_literal(argp)
	unsigned char	*argp;		/* Output pointer	*/
{
	unsigned int	c;
	unsigned char	*pc;
	unsigned char	*oldargp = argp;

	while ((c = readwc()) != '\0' && c != LITERAL) {
		/*
		 * quote each character within
		 * single quotes.
		 * If we implement $(), the strings need to pass the parser
		 * more than once, so we need to surround \n by "".
		 */
		pc = readw(c);
		GROWSTAK(argp);
#ifdef	DO_DOL_PAREN
		if (c == NL)
			*argp++ = '"';
		else
#endif
			*argp++ = '\\';
		/* Pick up rest of multibyte character */
		while (*pc != 0) {
			GROWSTAK(argp);
			*argp++ = *pc++;
		}
		if (c == NL) {
#ifdef	DO_DOL_PAREN
			GROWSTAK(argp);
			*argp++ = '"';
#endif
			chkpr();
		}
	}
	if (argp == oldargp) { /* null argument - '' */
		/*
		 * Word will be represented by quoted null
		 * in macro.c if necessary
		 */
		GROWSTAK(argp);
		*argp++ = '"';
		GROWSTAK(argp);
		*argp++ = '"';
	}
	GROWSTAK(argp);
	*argp = '\0';
	return (argp);
}

/*
 * Match and copy the next quoted block (surrounded by e.g. '`' or '"') from the
 * input stream.
 */
static unsigned char *
match_block(argp, c, d)
	unsigned char	*argp;		/* Output pointer	*/
	unsigned int	c;		/* Last read character	*/
	unsigned int	d;		/* Delimiter		*/
{
	unsigned int	cc;
	unsigned char	*pc;
#ifdef	MATCH_BLOCK_DEBUG
	UIntptr_t	p = relstakp(argp);
#endif

	for (;;) {
		if ((c = nextwc()) == 0) {
			GROWSTAK(argp);
			*argp++ = 0;
		} else {
			pc = readw(c);
			while (*pc) {
				GROWSTAK(argp);
				*argp++ = *pc++;
			}
		}
		if (c == 0 || c == d)
			break;
		if (c == NL)
			chkpr();
		/*
		 * don't interpret quoted
		 * characters
		 */
		if (c == '\\') {
			/*
			 * This is the quoted character:
			 */
			if ((cc = readwc()) == 0) {
				GROWSTAK(argp);
				*argp++ = 0;
			} else {
				pc = readw(cc);
				while (*pc) {
					GROWSTAK(argp);
					*argp++ = *pc++;
				}
			}
		}
#ifdef	DO_DOL_PAREN
		if (c == DOLLAR) {
			argp = dolparen(argp);
		}
#endif
	}
	GROWSTAK(argp);
	*argp = '\0';
#ifdef	MATCH_BLOCK_DEBUG
	fprintf(stderr, "match_block(%c) '%s'\n", d, absstak(p));
	fflush(stderr);
#endif
	return (argp);
}

unsigned int
skipwc()
{
	unsigned int c;

	while (c = nextwc(), space(c))
		/* LINTED */
		;
	return (c);
}

unsigned int
nextwc()
{
	unsigned int	c, d;

retry:
	if ((d = readwc()) == ESCAPE) {
		if ((c = readwc()) == NL) {
			chkpr();
			goto retry;
		}
		peekc = c | MARK;
	}
	return (d);
}

unsigned char *
readw(d)
	wchar_t	d;
{
	static unsigned char c[MULTI_BYTE_MAX + 1];
	int clength;

	if (isascii(d)) {
		c[0] = d;
		c[1] = '\0';
		return (c);
	}

	clength = wctomb((char *)c, d);
	if (clength <= 0) {
		c[0] = (unsigned char)d;
		clength = 1;
	}
	c[clength] = '\0';
	return (c);
}

unsigned int
readwc()
{
	wchar_t	c;
	int	len;
	struct fileblk	*f;
	int	mbmax;
	int	i, mlen;

top:
	if (peekn) {
		c = peekn & 0x7fffffff;
		peekn = 0;
		return (c);
	}
	if (peekc) {
		c = peekc & 0x7fffffff;
		peekc = 0;
		return (c);
	}
	f = standin;

retry:
	if (f->fend > f->fnxt) {
		/*
		 * something in buffer
		 */
		c = (unsigned char)*f->fnxt;
		if (c == 0) {
			f->fnxt++;
			f->nxtoff++;
			if (f->feval == 0)
				goto retry;	/* = c = readc(); */
			if (estabf(*f->feval++))
				c = EOF;
			else
				c = SPACE;
			if ((flags & readpr) && standin->fstak == 0)
				prc(c);
			if (c == NL)
				f->flin++;
			return (c);
		}

		if (isascii(c)) {
			f->fnxt++;
			f->nxtoff++;
			if ((flags & readpr) && standin->fstak == 0)
				prc(c);
			if (c == NL)
				f->flin++;
			return (c);
		}

		(void) mbtowc(NULL, NULL, 0);
		mbmax = MB_CUR_MAX;
		mlen = 0;
		for (i = 1; i <= mbmax; i++) {
			int	rest;
			if ((rest = f->fend - f->fnxt) < i) {
				/*
				 * not enough bytes available
				 * f->fsiz could be BUFFERSIZE or 1 since
				 * mbmax is enough smaller than BUFFERSIZE,
				 * this loop won't overrun the f->fbuf buffer.
				 */
				len = readb(f,
				    (f->fsiz == 1) ? 1 : (f->fsiz - rest),
				    rest);
				if (len == 0)
					break;
			}
			mlen = mbtowc(&c, (char *)f->fnxt, i);
			if (mlen > 0)
				break;
			(void) mbtowc(NULL, NULL, 0);
		}

		if (i > mbmax) {
			/*
			 * enough bytes available but cannot be converted to
			 * a valid wchar.
			 */
			c = (unsigned char)*f->fnxt;
			mlen = 1;
		}

		f->fnxt += mlen;
		f->nxtoff += mlen;
		if ((flags & readpr) && standin->fstak == 0)
			prwc(c);
		if (c == NL)
			f->flin++;
		return (c);
	}

	if (f->feof || f->fdes < 0) {
		if (f->fdes <= -2) {	/* Auto-pop() fileblk to remove */
		extern	int	abegin;

			if (f->fdes == -3) /* Continue with begin alias */
				abegin++;
			peekn = f->peekn;
			pop();
			free(f);
			f = standin;
			if (peekn)
				goto top;
			goto retry;
		}
		c = EOF;
		f->feof++;
		return (c);
	}

	if (readb(f, f->fsiz, 0) <= 0) {
		if (f->fdes != input || !isatty(input)) {
			close(f->fdes);
			f->fdes = -1;
		}
		f->feof++;
		c = EOF;
		return (c);
	}
	goto retry;
}

static int
readb(f, toread, rest)
	struct fileblk	*f;
	int		toread;
	int		rest;
{
	int	len;
	int	fflags;

	if (rest) {
		/*
		 * copies the remaining 'rest' bytes from f->fnxt
		 * to f->fbuf
		 */
		(void) memmove(f->fbuf, f->fnxt, rest);
		f->fnxt = f->fbuf;
		f->fend = f->fnxt + rest;
		f->nxtoff = 0;
		f->endoff = rest;
		if (f->fbuf[rest - 1] == '\n') {
			/*
			 * if '\n' found, it should be
			 * a bondary of multibyte char.
			 */
			return (rest);
		}
	}

retry:
	do {
		if (trapnote & SIGSET) {
			newline();
			sigchk();
		} else if ((trapnote & TRAPSET) && (rwait > 0)) {
			newline();
			chktrap();
			clearup();
		}
#ifdef	INTERACTIVE
	} while ((len = xread(f->fdes,
			    (char *)f->fbuf + rest, toread)) < 0 && trapnote);
#else
	} while ((len = read(f->fdes,
			    (char *)f->fbuf + rest, toread)) < 0 && trapnote);
#endif

	/*
	 * if child sets O_NDELAY or O_NONBLOCK on stdin
	 * and exited then turn the modes off and retry
	 */
	if (len == 0) {
		if (((flags & intflg) ||
		    ((flags & oneflg) == 0 && isatty(input) &&
		    (flags & stdflg))) &&
		    ((fflags = fcntl(f->fdes, F_GETFL, 0)) & O_NDELAY)) {
			fflags &= ~O_NDELAY;
			fcntl(f->fdes, F_SETFL, fflags);
			goto retry;
		}
	} else if (len < 0) {
		if (errno == EAGAIN) {
			fflags = fcntl(f->fdes, F_GETFL, 0);
			fflags &= ~O_NONBLOCK;
			fcntl(f->fdes, F_SETFL, fflags);
			goto retry;
		}
		len = 0;
	}
	f->fnxt = f->fbuf;
	f->fend = f->fnxt + (len + rest);
	f->nxtoff = 0;
	f->endoff = len + rest;
	return (len + rest);
}

#ifdef	INTERACTIVE
static BOOL
chk_igneof()
{
	return (flags2 & ignoreeofflg);
}

static int
xread(f, buf, n)
	int	f;
	char	*buf;
	int	n;
{
	/*
	 * If we like to call shedit_egetc() for files other than STDIN_FILENO
	 * we would need to set up the file descriptor for libshedit.
	 */
	if ((f == STDIN_FILENO) &&
	    (flags2 & vedflg)) {
		static	int	init = 0;
			int	c;

		if (!init) {
			init = 1;
			shedit_getenv(getcurenv);
			shedit_putenv(ev_insert);
			shedit_igneof(chk_igneof);
		}
		c = shedit_egetc();
		*buf = c;
		if (c == -1 && shedit_getdelim() == -1) {	/* EOF */
			shedit_treset();
			return (0);
		}
		if (c == CTLC && shedit_getdelim() == CTLC) {
			trapnote |= SIGSET;
			return (-1);
		}
		return (1);
	}
	return (read(f, buf, n));
}
#endif

#ifdef	DO_TILDE
static unsigned char *
do_tilde(arg)
	unsigned char	*arg;
{
	unsigned char	*val = NULL;
	unsigned char	*u = arg;
	unsigned char	*p;

	if (*u++ != '~')
		return (NULL);
	for (p = u; *p && *p != '/'; p++)
		;
	if (p == u) {
		val = homenod.namval;
	} else if ((p - u) == 1) {
		if (*u == '+')
			val = pwdnod.namval;
		else if (*u == '-')
			val = opwdnod.namval;
	}
	if (val == NULL) {
		struct passwd	*pw;
		unsigned int	c;

		c = *p;
		*p = '\0';
		pw = getpwnam((char *)u);
		endpwent();
		*p = c;
		if (pw)
			val = UC pw->pw_dir;
	}
	return (val);
}
#endif
