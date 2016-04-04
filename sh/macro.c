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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)macro.c	1.16	06/10/09 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2016 J. Schilling
 *
 * @(#)macro.c	1.45 16/04/03 2008-2016 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)macro.c	1.45 16/04/03 2008-2016 J. Schilling";
#endif

/*
 * UNIX shell
 */

#ifdef	SCHILY_INCLUDES
#include	"sym.h"
#include	<schily/wait.h>
#else
#include	"sym.h"
#include	<wait.h>
#endif

static unsigned char	quote;	/* used locally */
static unsigned char	quoted;	/* used locally */

#define		COM_BACKQUOTE	0	/* `command`  type command substitution */
#define		COM_DOL_PAREN	1	/* $(command) type command substitution */

static void	copyto		__PR((unsigned char endch, int trimflag));
static void	skipto		__PR((unsigned char endch));
#ifdef	DO_DOT_SH_PARAMS
static unsigned char *shvar	__PR((unsigned char *v));
#endif
static unsigned int dolname	__PR((unsigned char **argpp,
					unsigned int c, unsigned int addc));
static int	getch		__PR((unsigned char endch, int trimflag));
	unsigned char *macro	__PR((unsigned char *as));
static void	comsubst	__PR((int, int));
	void	subst		__PR((int in, int ot));
static void	flush		__PR((int));

#ifdef	DO_SUBSTRING
static int	mbschars	__PR((unsigned char *v));
static unsigned char	*prefsubstr __PR((unsigned char *v, unsigned char *pat,
					int largest));
static unsigned char	*mbdecr	__PR((unsigned char *s, unsigned char *sp));
static int		suffsubstr __PR((unsigned char *v, unsigned char *pat,
					int largest));
#endif

#ifdef	PROTOTYPES
static void
copyto(unsigned char endch, int trimflag)
#else
static void
copyto(endch, trimflag)
	unsigned char	endch;
	int		trimflag;
#endif
/* trimflag - flag to check if argument will be trimmed */
{
	unsigned int	c;
	unsigned int	d;
	unsigned char *pc;

	while ((c = getch(endch, trimflag)) != endch && c) {
		if (quote) {
			if (c == '\\') { /* don't interpret next character */
				GROWSTAKTOP();
				pushstak(c);
				d = readwc();
				if (!escchar(d)) {
					/*
					 * both \ and following
					 * character are quoted if next
					 * character is not $, `, ", or \
					 */
					GROWSTAKTOP();
					pushstak('\\');
					GROWSTAKTOP();
					pushstak('\\');
					pc = readw(d);
					/* push entire multibyte char */
					while (*pc) {
						GROWSTAKTOP();
						pushstak(*pc++);
					}
				} else {
					pc = readw(d);
					/*
					 * d might be NULL
					 * Evenif d is NULL, we have to save it
					 */
					if (*pc) {
						while (*pc) {
							GROWSTAKTOP();
							pushstak(*pc++);
						}
					} else {
						GROWSTAKTOP();
						pushstak(*pc);
					}
				}
			} else { /* push escapes onto stack to quote chars */
				pc = readw(c);
				GROWSTAKTOP();
				pushstak('\\');
				while (*pc) {
					GROWSTAKTOP();
					pushstak(*pc++);
				}
			}
		} else if (c == '\\') {
			c = readwc(); /* get character to be escaped */
			GROWSTAKTOP();
			pushstak('\\');
			pc = readw(c);
			/* c might be NULL */
			/* Evenif c is NULL, we have to save it */
			if (*pc) {
				while (*pc) {
					GROWSTAKTOP();
					pushstak(*pc++);
				}
			} else {
				GROWSTAKTOP();
				pushstak(*pc);
			}
		} else {
			pc = readw(c);
			while (*pc) {
				GROWSTAKTOP();
				pushstak(*pc++);
			}
		}
	}
	GROWSTAKTOP();
	zerostak();	/* Add nul byte */
	if (c != endch)
		error(badsub);
}

#ifdef	PROTOTYPES
static void
skipto(unsigned char endch)
#else
static void
skipto(endch)
	unsigned char	endch;
#endif
{
	/*
	 * skip chars up to }
	 */
	unsigned int	c;

	while ((c = readwc()) != '\0' && c != endch)
	{
		switch (c)
		{
		case SQUOTE:
			skipto(SQUOTE);
			break;

		case DQUOTE:
			skipto(DQUOTE);
			break;

		case DOLLAR:
			if ((c = readwc()) == BRACE)
				skipto('}');
			else if (c == SQUOTE || c == DQUOTE)
				skipto(c);
			else if (c == 0)
				goto out;
			break;

		case ESCAPE:
			if (!(c = readwc()))
				goto out;
		}
	}
out:
	if (c != endch)
		error(badsub);
}

/*
 * Expand special shell variables ${.sh.xxx}.
 */
#ifdef	DO_DOT_SH_PARAMS
static unsigned char *
shvar(v)
	unsigned char	*v;
{
	if (eq(v, "status")) {			/* exit status */
		sitos(retex.ex_status);
		v = numbuf;
	} else if (eq(v, "termsig")) {		/* kill signame */
		numbuf[0] = '\0';
		sig2str(retex.ex_status, (char *)numbuf);
		v = numbuf;
		if (numbuf[0] == '\0')
			strcpy((char *)numbuf, "UNKNOWN");
	} else if (eq(v, "code")) {		/* exit code (reason) */
		itos(retex.ex_code);
		v = numbuf;
	} else if (eq(v, "codename")) {		/* text for above */
		v = UC code2str(retex.ex_code);
	} else if (eq(v, "pid")) {		/* exited pid */
		itos(retex.ex_pid);
		v = numbuf;
	} else if (eq(v, "signo")) {		/* SIGCHLD or trapsig */
		itos(retex.ex_signo);
		v = numbuf;
	} else if (eq(v, "signame")) {		/* text for above */
		sig2str(retex.ex_signo, (char *)numbuf);
		v = numbuf;
	} else if (eq(v, "shell")) {		/* Shell implementation name */
		v = UC shname;
	} else if (eq(v, "version")) {		/* Shell version */
		v = UC shvers;
	} else {
		return (NULL);
	}
	return (v);
}
#endif

/*
 * Collect the parameter name.
 * If "addc" is null, collect a normal parameter name,
 * else "addc" is an additional permitted character.
 * This is typically '.' for the ".sh.xxx" parameters.
 * Returns the first non-matching character to allow the rest
 * of the parser to recover.
 */
static unsigned int
dolname(argpp, c, addc)
	unsigned char	**argpp;
	unsigned int	c;
	unsigned int	addc;
{
	unsigned char	*argp;

	argp = (unsigned char *)relstak();
	while (alphanum(c) || (addc && c == addc)) {
		GROWSTAKTOP();
		pushstak(c);
		c = readwc();
	}
	GROWSTAKTOP();
	zerostak();
	*argpp = argp;
	return (c);
}

#ifdef	PROTOTYPES
static int
getch(unsigned char endch, int trimflag)
#else
static int
getch(endch, trimflag)
unsigned char	endch;
/*
 * flag to check if an argument is going to be trimmed, here document
 * output is never trimmed
 */
int trimflag;
#endif
{
	unsigned int	d;
	/*
	 * atflag to check if $@ has already been seen within double quotes
	 */
	int atflag = 0;
retry:
	d = readwc();
	if (!subchar(d))
		return (d);

	if (d == DOLLAR) {
		unsigned int c;

		if ((c = readwc(), dolchar(c))) {
			struct namnod *n = (struct namnod *)NIL;
			int		dolg = 0;
			int		vsize = -1;
			BOOL		bra;
			BOOL		nulflg;
			unsigned char	*argp, *v = NULL;
			unsigned char		idb[2];
			unsigned char		*id = idb;

			*id = '\0';

			if ((bra = (c == BRACE)) != FALSE)
				c = readwc();
#ifdef	DO_SUBSTRING
getname:
#endif
			if (letter(c)) {	/* valid parameter name */

				c = dolname(&argp, c, 0);
				n = lookup(absstak(argp));
				setstak(argp);
#ifndef	DO_POSIX_UNSET
				if (n->namflg & N_FUNCTN)
					error(badsub);
#endif
#ifdef	DO_LINENO
				if (n == &linenonod) {
					v = linenoval();
				} else
#endif
					v = n->namval;
				id = (unsigned char *)n->namid;
				peekc = c | MARK;
			} else if (digchar(c)) {	/* astchar or digits */
				*id = c;
				idb[1] = 0;
				if (astchar(c))	{	/* '*' or '@' */
					if (c == '@' && !atflag && quote) {
						quoted--;
						atflag = 1;
					}
					dolg = 1;
					c = 1;
				} else if (digit(c)) {
					c -= '0';
#ifdef	DO_POSIX_PARAM
					if (bra) {
						int	dd;

						while ((dd = readwc(),
							digit(dd)))
							c = 10 * c + (dd - '0');
						peekc = dd | MARK;
					}
#endif
				}
				/*
				 * Double cast is needed to work around a
				 * GCC bug.
				 */
				v = ((c == 0) ?
					cmdadr :
					((int)c <= dolc) ?
					dolv[c] :
					(unsigned char *)(Intptr_t)(dolg = 0));
			} else if (c == '$') {
				v = pidadr;
			} else if (c == '!') {
				v = pcsadr;
			} else if (c == '#') {
#ifdef	DO_SUBSTRING
				if (bra == 1) {
					c = readwc();
					if (c != '}') {
						bra++;
						goto getname;
					} else {
						bra = 0;
					}
				}
#endif
				itos(dolc);
				v = numbuf;
			} else if (c == '?') {
#ifdef	DO_SIGNED_EXIT
				sitos(retval);
#else
				itos(retval);
#endif
				v = numbuf;
			} else if (c == '-') {
				v = flagadr;
#ifdef	DO_DOT_SH_PARAMS
			} else if (bra && c == '.') {
				unsigned char	*shv;

				c = dolname(&argp, c, '.');
				v = absstak(argp);	/* Variable name */
				if (v[0] == '.' &&
				    v[1] == 's' &&
				    v[2] == 'h' &&
				    v[3] == '.' &&
					(shv = shvar(&v[4])) != NULL) {
					v = shv;
				} else {
					v = NULL;
				}
				setstak(argp);
				peekc = c | MARK;
#endif
			} else if (bra) {
				error(badsub);
			} else {
				goto retry;
			}
			c = readwc();
			if (c == ':' && bra == 1) { /* null and unset fix */
				nulflg = 1;
				c = readwc();	/* c now holds char past ':' */
			} else {
				nulflg = 0;
			}
			if (!defchar(c) && bra) {	/* check "}=-+?#%" */
				error(badsub);
			}
			argp = 0;
			if (bra) {		/* ${...} */

				if (c != '}') {	/* word follows parameter */
					/*
					 * Collect word or null string depending
					 * on parameter value and setchar(c).
					 */
					argp = (unsigned char *)relstak();
					if ((v == 0 ||
					    (nulflg && *v == 0)) ^ (setchar(c))) {
						int	ntrim = trimflag;
#ifdef	DO_SUBSTRING
						if (c == '#' || c == '%')
							ntrim = 0;
#endif
						copyto('}', ntrim);
					} else {
						skipto('}');
					}
					argp = absstak(argp);
				}
			} else {
				peekc = c | MARK;
				c = 0;
			}
#ifdef	DO_SUBSTRING
			if (bra > 1) {
				int	l = 0;

				if (c != '}' || argp) {
					error(badsub);
				}
				if (v)
					l = mbschars(v);
				itos(l);
				v = numbuf;
			}
			if ((c == '#' || c == '%')) {
				if (v) {
					int		largest = 0;
					unsigned char	*oargp = argp;
					UIntptr_t	a;

					if (dolg)
						error(badsub);
					if (*argp == c) {
						largest++;
						argp++;
					}
					staktop++;
					a = relstak();
					macro(argp);
					argp = absstak(a);
					if (c == '#') {
						v = prefsubstr(v, argp,
								largest);
					} else {
						vsize = suffsubstr(v, argp,
								largest);
					}
					staktop = oargp;
				} else {
					/*
					 * Clear to let it fail later with
					 * an unset error with set -u.
					 */
					argp = 0;
				}
			}
#endif
			if (v && (!nulflg || *v)) {
				/*
				 * Parameter is not unset and
				 * either non-null or the ':' is not present.
				 */

				if (c != '+') {
					/*
					 * Substitute parameter value
					 */
					(void) mbtowc(NULL, NULL, 0);
					for (;;) {
						if (*v == 0 && quote) {
							GROWSTAKTOP();
							pushstak('\\');
							GROWSTAKTOP();
							pushstak('\0');
						} else {
							while (vsize && (c = *v) != '\0') {
								wchar_t	wc;
								int	clength;
								if ((clength = mbtowc(&wc, (char *)v, MB_LEN_MAX)) <= 0) {
									(void) mbtowc(NULL, NULL, 0);
									clength = 1;
								}
								if (quote || (c == '\\' && trimflag)) {
									GROWSTAKTOP();
									pushstak('\\');
								}
								while (clength-- > 0) {
									GROWSTAKTOP();
									pushstak(*v++);
								}
								if (vsize > 0)
									vsize--;
							}
						}

						if (dolg == 0 ||
						    (++dolg > dolc))
							break;
						else /* $* and $@ expansion */
						{
							v = dolv[dolg];
							if (*id == '*' &&
							    quote) {
								unsigned char sep = ' ';

#ifdef	DO_IFS_SEP
								if (ifsnod.namval)
									sep = *ifsnod.namval;
#endif
				/*
				 * push quoted separator so that "$*" will
				 * not be broken into separate arguments.
				 */
								if (sep) {
									GROWSTAKTOP();
									pushstak('\\');
									GROWSTAKTOP();
									pushstak(sep);
								}
							} else {
								GROWSTAKTOP();
								pushstak(' ');
							}
						}
					}
				}
			} else if (argp) {
				if (c == '?') {
					if (trimflag)
						trim(argp);
					failed(id, *argp ? (const char *)argp :
					    badparam);
				} else if (c == '=') {
					if (n) {
						int strlngth = staktop - stakbot;
						unsigned char *savptr = fixstak();
						struct ionod *iosav = iotemp;
						unsigned char *newargp;

						/*
						 * The malloc()-based stak.c
						 * will relocate the last item
						 * if we call fixstak();
						 */
						argp = savptr;

					/*
					 * copy word onto stack, trim it, and
					 * then do assignment
					 */
						usestak();
						(void) mbtowc(NULL, NULL, 0);
						while ((c = *argp) != '\0') {
							wchar_t		wc;
							int		len;

							if ((len = mbtowc(&wc, (char *)argp, MB_LEN_MAX)) <= 0) {
								(void) mbtowc(NULL, NULL, 0);
								len = 1;
							}
							if (c == '\\' &&
							    trimflag) {
								argp++;
								if (*argp == 0) {
									argp++;
									continue;
								}
								if ((len = mbtowc(&wc, (char *)argp, MB_LEN_MAX)) <= 0) {
									(void) mbtowc(NULL, NULL, 0);
									len = 1;
								}
							}
							while (len-- > 0) {
								GROWSTAKTOP();
								pushstak(*argp++);
							}
						}
						newargp = fixstak();
						assign(n, newargp);
						tdystak(savptr, iosav);
						(void) memcpystak(stakbot, savptr, strlngth);
						staktop = stakbot + strlngth;
					} else {
						error(badsub);
					}
				}
			} else if (flags & setflg) {
				failed(id, unset);
			}
			goto retry;
#ifdef		DO_DOL_PAREN
		} else if (c == '(') {
			comsubst(trimflag, COM_DOL_PAREN);
			goto retry;
#endif
		} else {
			peekc = c | MARK;
		}
	} else if (d == endch) {
		return (d);
	} else if (d == SQUOTE) {
		comsubst(trimflag, COM_BACKQUOTE);
		goto retry;
	} else if (d == DQUOTE && trimflag) {
		if (!quote) {
			atflag = 0;
			quoted++;
		}
		quote ^= QUOTE;
		goto retry;
	}
	return (d);
}

unsigned char *
macro(as)
unsigned char	*as;
{
	/*
	 * Strip "" and do $ substitution
	 * Leaves result on top of stack
	 */
	BOOL	savqu = quoted;
	unsigned char	savq = quote;
	struct filehdr	fb;

	fb.fsiz = 1;	/* It's a filehdr not a fileblk */
	push((struct fileblk *)&fb);
	estabf(as);
	usestak();
	quote = 0;
	quoted = 0;
	copyto(0, 1);
	pop();
	if (quoted && (stakbot == staktop)) {
		GROWSTAKTOP();
		pushstak('\\');
		GROWSTAKTOP();
		pushstak('\0');
/*
 * above is the fix for *'.c' bug
 */
	}
	quote = savq;
	quoted = savqu;
	return (fixstak());
}
/* Save file descriptor for command substitution */
int savpipe = -1;

static void
comsubst(trimflag, type)
	int	trimflag;
	int	type;
/* trimflag - used to determine if argument will later be trimmed */
{
	/*
	 * command substn
	 */
	struct fileblk	cb;
	unsigned int	d;
	int strlngth = staktop - stakbot;
	unsigned char *oldstaktop;
	unsigned char *savptr = fixstak();
	struct ionod *iosav = iotemp;
	unsigned char	*pc;
	struct trenod	*tc = NULL;

	if (type == COM_BACKQUOTE) {  /* `command`  type command substitution */

	usestak();
	while ((d = readwc()) != SQUOTE && d) {
		if (d == '\\') {
			d = readwc();
			if (!escchar(d) || (d == '"' && !quote)) {
				/*
				 * trim quotes for `, \, or " if
				 * command substitution is within
				 * double quotes
				 */
				GROWSTAKTOP();
				pushstak('\\');
			}
		}
		pc = readw(d);
		/* d might be NULL */
		if (*pc) {
			while (*pc) {
				GROWSTAKTOP();
				pushstak(*pc++);
			}
		} else {
			GROWSTAKTOP();
			pushstak(*pc);
		}
	}
	{
		unsigned char	*argc;

		argc = fixstak();
		push(&cb);
		estabf(argc);  /* read from string */
		tc = cmd(EOFSYM, MTFLG | NLFLG | SEMIFLG);
	}
	}
#ifdef	DO_DOL_PAREN
	else {	/* $(command) type command substitution */
		tc = cmd(')', MTFLG | NLFLG | SEMIFLG);
	}
#endif
	{
		struct trenod	*t;
		int		pv[2];

		/*
		 * this is done like this so that the pipe
		 * is open only when needed
		 */
		t = makefork(FPOU|STDOUT_FILENO, tc);
		chkpipe(pv);
		savpipe = pv[OTPIPE];
#ifdef	DO_DOL_PAREN
		if (type != COM_BACKQUOTE) {
			push(&cb);
			estabf(NULL);
		}
#endif
		initf(pv[INPIPE]); /* read from pipe */
#ifdef	PARSE_DEBUG
		prtree(t, "Command Substitution: ");
#endif
		execute(t, XEC_NOSTOP, (int)(flags & errflg), 0, pv);
		close(pv[OTPIPE]);
		savpipe = -1;
	}
	tdystak(savptr, iosav);
	(void) memcpystak(stakbot, savptr, strlngth);
	oldstaktop = staktop = stakbot + strlngth;
	while ((d = readwc()) != '\0') {
		if (quote || (d == '\\' && trimflag)) {
			unsigned char *rest;
			/*
			 * quote output from command subst. if within double
			 * quotes or backslash part of output
			 */
			rest = readw(d);
			GROWSTAKTOP();
			pushstak('\\');
			while ((d = *rest++) != '\0') {
			/* Pick up all of multibyte character */
				GROWSTAKTOP();
				pushstak(d);
			}
		} else {
			pc = readw(d);
			while (*pc) {
				GROWSTAKTOP();
				pushstak(*pc++);
			}
		}
	}
	{
		extern pid_t parent;
		int	rc;
		int	ret = 0;
		int	wstatus;
		int	wcode;

		while ((ret = wait_status(parent,
				&wcode, &wstatus,
				(WEXITED|WTRAPPED))) != parent) {
			/* break out if waitid(2) has failed */
			if (ret == -1)
				break;
		}
		rc = wstatus;
		if ((flags2 & fullexitcodeflg) == 0)
			rc &= 0xFF; /* As dumb as with historic wait */
		if (wcode == CLD_KILLED || wcode == CLD_DUMPED)
			rc |= SIGFLG;
#ifdef	DO_EXIT_MODFIX
		else if (wstatus != 0 && rc == 0)
			rc = SIGFLG;	/* Use special value 128 */
#endif
		if (rc && (flags & errflg))
			exitsh(rc);
		exitval = rc;
		ex.ex_status = wstatus;
		ex.ex_code = wcode;
		ex.ex_pid = parent;
		flags |= eflag;
		exitset();
	}
	while (oldstaktop != staktop)
	{ /* strip off trailing newlines from command substitution only */
		if ((*--staktop) != NL)
		{
			++staktop;
			break;
		} else if (quote)
			staktop--; /* skip past backslashes if quoting */
	}
	pop();
}

#define	CPYSIZ	512

void
subst(in, ot)
	int	in;
	int	ot;
{
	unsigned int	c;
	struct fileblk	fb;
	int	count = CPYSIZ;
	unsigned char	*pc;

	push(&fb);
	initf(in);
	/*
	 * DQUOTE used to stop it from quoting
	 */
	while ((c = (getch(DQUOTE, 0))) != '\0') {
		/*
		 * read characters from here document and interpret them
		 */
		if (c == '\\') {
			c = readwc();
			/*
			 * check if character in here document is escaped
			 */
			if (!escchar(c) || c == '"') {
				GROWSTAKTOP();
				pushstak('\\');
			}
		}
		pc = readw(c);
		/* c might be NULL */
		if (*pc) {
			while (*pc) {
				GROWSTAKTOP();
				pushstak(*pc++);
			}
		} else {
			GROWSTAKTOP();
			pushstak(*pc);
		}
		if (--count == 0)
		{
			flush(ot);
			count = CPYSIZ;
		}
	}
	flush(ot);
	pop();
}

static void
flush(ot)
	int	ot;
{
	write(ot, stakbot, staktop - stakbot);
	if (flags & execpr)
		write(output, stakbot, staktop - stakbot);
	staktop = stakbot;
}

#ifdef	DO_SUBSTRING
static int
mbschars(v)
	unsigned char	*v;
{
	register unsigned char	*s = v;
		wchar_t		wc;
		int		len;
		int		chars = 0;

	while (*s) {
		if ((len = mbtowc(&wc, (char *)s, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			len = 1;
		}
		s += len;
		chars++;
	}
	return (chars);
}

static unsigned char *
prefsubstr(v, pat, largest)
	unsigned char	*v;
	unsigned char	*pat;
	int		largest;
{
	register unsigned char	*s = v;
	register unsigned char	*r = v;
	register unsigned int	c;
		wchar_t		wc;
		int		len;

	do {
		c = *s;
		*s = '\0';
		if (gmatch(C v, C pat)) {
			if (!largest) {
				*s = c;
				return (s);
			}
			r = s;
		}
		*s = c;

		if ((len = mbtowc(&wc, (char *)s, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			len = 1;
		}
		s += len;
	} while (c);
	return (r);
}

static unsigned char *
mbdecr(s, sp)
	unsigned char	*s;
	unsigned char	*sp;
{
	wchar_t		wc;
	int		len;

	while (s < sp) {
		if ((len = mbtowc(&wc, (char *)s, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			len = 1;
		}
		if ((s + len) >= sp)
			return (s);
		s += len;
	}
	return (sp-1);
}

static int
suffsubstr(v, pat, largest)
	unsigned char	*v;
	unsigned char	*pat;
	int		largest;
{
	register unsigned char	*s = v;
	register int		size = strlen(C v);

	s += size;
	while (s >= v) {
		if (gmatch(C s, C pat)) {
			size = s - v;
			if (!largest)
				break;
		}
		s = mbdecr(v, s);
	}
	return(size);
}
#endif
