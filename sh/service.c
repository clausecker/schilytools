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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)service.c	1.27	08/01/29 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2016 J. Schilling
 *
 * @(#)service.c	1.39 16/01/04 2008-2016 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)service.c	1.39 16/01/04 2008-2016 J. Schilling";
#endif

/*
 * UNIX shell
 */
#ifdef	SCHILY_INCLUDES
#include	<schily/types.h>
#include	<schily/stat.h>
#include	<schily/errno.h>
#include	<schily/fcntl.h>
#include	"sh_policy.h"
#else
#include	<errno.h>
#include	<fcntl.h>
#include	"sh_policy.h"
#endif

#define	ARGMK	01

	short		initio	__PR((struct ionod *iop, int save));
	unsigned char *simple	__PR((unsigned char *s));
	unsigned char *getpath	__PR((unsigned char *s));
	int		pathopen __PR((unsigned char *path,
						unsigned char *name));
	unsigned char *catpath	__PR((unsigned char *path,
						unsigned char *name));
	unsigned char *nextpath	__PR((unsigned char *path));
	void		execa	__PR((unsigned char *at[], short pos,
						int isvfork,
						unsigned char *av0));
static unsigned char *execs	__PR((unsigned char *ap, unsigned char *t[],
						int isvfork,
						unsigned char *av0));
	void		trim	__PR((unsigned char *at));
	void		trims	__PR((unsigned char *at));
	unsigned char *mactrim	__PR((unsigned char *at));
	unsigned char **scan	__PR((int argn));
static void		gsort	__PR((unsigned char *from[],
						unsigned char *to[]));
	int		getarg	__PR((struct comnod *ac));
static int		split	__PR((unsigned char *s));

extern short topfd;


/*
 * service routines for `execute'
 */
short
initio(iop, save)
	struct ionod	*iop;
	int		save;
{
	unsigned char	*ion;
	int	iof, fd = -1;
	int		ioufd;
	short	lastfd;
	int	newmode;

	lastfd = topfd;
	while (iop) {
		iof = iop->iofile;
		ion = mactrim((unsigned char *)iop->ioname);
		ioufd = iof & IOUFD;

		if (*ion && (flags&noexec) == 0) {
			if (save) {
				fdmap[topfd].org_fd = ioufd;
				fdmap[topfd++].dup_fd = savefd(ioufd);
			}

			if (iof & IODOC_SUBST) {
				struct tempblk tb;

				subst(chkopen(ion, O_RDONLY),
					    (fd = tmpfil(&tb)));

				/*
				 * pushed in tmpfil() --
				 * bug fix for problem with
				 * in-line scripts
				 */
				poptemp();

				fd = chkopen(tmpout, O_RDONLY);
				unlink((const char *)tmpout);
			} else if (iof & IOMOV) {
				if (eq(minus, ion)) {
					fd = -1;
					close(ioufd);
				} else if ((fd = stoi(ion)) >= USERIO) {
					failed(ion, badfile);
				}
				else
					fd = dup(fd);
			} else if (((iof & IOPUT) == 0) && ((iof & IORDW) == 0))
				fd = chkopen(ion, O_RDONLY);
			else if (iof & IORDW) /* For <> */ {
				newmode = O_RDWR|O_CREAT;
				fd = chkopen(ion, newmode);
			} else if (flags & rshflg) {
				failed(ion, restricted);
			} else if (iof & IOAPP &&
#if defined(DO_O_APPEND) && defined(O_APPEND)
			    (fd = open((char *)ion, O_WRONLY|O_APPEND)) >= 0) {
#else
			    (fd = open((char *)ion, O_WRONLY)) >= 0) {
#endif
				lseek(fd, (off_t)0, SEEK_END);
			} else {
				fd = create(ion, iof);
			}
			if (fd >= 0)
				renamef(fd, ioufd);
		}

		iop = iop->ionxt;
	}
	return (lastfd);
}

unsigned char *
simple(s)
unsigned char	*s;
{
	unsigned char	*sname;

	sname = s;
	/* CONSTCOND */
	while (1) {
		if (any('/', sname))
			while (*sname++ != '/')
				/* LINTED */
				;
		else
			return (sname);
	}
	/* NOTREACHED */
}

unsigned char *
getpath(s)
	unsigned char	*s;
{
	unsigned char	*path, *newpath;
	int pathlen;

	if (any('/', s))
	{
		if (flags & rshflg) {
			failed(s, restricted);
			/* NOTREACHED */
		} else
			return ((unsigned char *)nullstr);
#ifdef	DO_SYSCOMMAND
	} else if (flags & ppath) {
		return ((unsigned char *)defppath);
#endif
	} else if ((path = pathnod.namval) == 0)
		return ((unsigned char *)defpath);
	else {
		pathlen = length(path)-1;
		/* Add extra ':' if PATH variable ends in ':' */
		if (pathlen > 2 && path[pathlen - 1] == ':' &&
				path[pathlen - 2] != ':') {
			newpath = locstak();
			newpath = memcpystak(newpath, path, pathlen);
			if (&newpath[pathlen+1] >= brkend) {
				newpath = growstak(&newpath[pathlen+1]);
				newpath -= pathlen + 1;
			}

			newpath[pathlen] = ':';
			newpath = endstak(newpath + pathlen + 1);
			return (newpath);
		} else
			return (cpystak(path));
	}
	return (NULL);		/* Not reached, but keeps GCC happy */
}

int
pathopen(path, name)
	unsigned char	*path;
	unsigned char	*name;
{
	int	f;

	do {
		do {
			path = catpath(path, name);
		} while ((f = open((char *)curstak(), O_RDONLY)) < 0 && path);
		if (f >= 0) {
			struct stat sb;

			if (fstat(f, &sb) < 0 || S_ISDIR(sb.st_mode)) {
				close(f);
				f = -1;
#ifdef	EISDIR
				errno = EISDIR;
#endif
			}
		}
	} while (f < 0 && path);
	return (f);
}

unsigned char *
catpath(path, name)
	unsigned char	*path;
	unsigned char	*name;
{
	/*
	 * leaves result on top of stack
	 */
	unsigned char	*scanp = path;
	unsigned char	*argp = locstak();

	while (*scanp && *scanp != COLON) {
		GROWSTAK(argp);
		*argp++ = *scanp++;
	}
	if (scanp != path) {
		GROWSTAK(argp);
		*argp++ = '/';
	}
	if (*scanp == COLON)
		scanp++;
	path = (*scanp ? scanp : 0);
	scanp = name;
	do {
		GROWSTAK(argp);
	} while ((*argp++ = *scanp++) != '\0');
	return (path);
}

unsigned char *
nextpath(path)
	unsigned char	*path;
{
	unsigned char	*scanp = path;

	while (*scanp && *scanp != COLON)
		scanp++;

	if (*scanp == COLON)
		scanp++;

	return (*scanp ? scanp : 0);
}

static const char	*xecmsg;
static unsigned char	**xecenv;

#ifdef	PROTOTYPES
void
execa(unsigned char *at[], short pos, int isvfork, unsigned char *av0)
#else
void
execa(at, pos, isvfork, av0)
	unsigned char	*at[];
	short		pos;
	int		isvfork;
	unsigned char	*av0;
#endif
{
	unsigned char	*path;
	unsigned char	**t = at;
	int		cnt;

	if ((flags & noexec) == 0) {
		xecmsg = notfound;
		path = getpath(*t);
		xecenv = local_setenv(isvfork?ENV_NOFREE:0);

		if (pos > 0) {
			cnt = 1;
			while (cnt != pos) {
				++cnt;
				path = nextpath(path);
			}
			execs(path, t, isvfork, av0);
			path = getpath(*t);
		}
		while ((path = execs(path, t, isvfork, av0)) != NULL)
			/* LINTED */
			;
		failedx(errno == ENOENT ? ERR_NOTFOUND : ERR_NOEXEC,
			*t, xecmsg);
	}
}

static unsigned char *
execs(ap, t, isvfork, av0)
	unsigned char	*ap;
	unsigned char	*t[];
	int		isvfork;
	unsigned char	*av0;
{
	int		pfstatus = NOATTRS;
	int		oexflag = exflag;
	struct excode	oex;
	int		e = ERR_NOEXEC;
	unsigned char	*p, *prefix;
#ifdef	EXECATTR_FILENAME
	unsigned char	*savptr;
	struct ionod	*iosav;
#endif

	prefix = catpath(ap, t[0]);
	trim(p = curstak());
	sigchk();

#ifdef	EXECATTR_FILENAME
	if (flags & pfshflg) {
		/*
		 * Need to save the stack information, or the
		 * first memory allocation in secpolicy_profile_lookup()
		 * will clobber it.
		 */
		savptr = endstak(p + strlen((const char *)p) + 1);
		iosav = iotemp;

		pfstatus = secpolicy_pfexec((const char *)p,
		    (char **)t, (const char **)xecenv);

		if (pfstatus != NOATTRS) {
			errno = pfstatus;
		}

		tdystak(savptr, iosav);
	}
#endif

	if (pfstatus == NOATTRS) {
		unsigned char	*a0 = t[0];

		if (av0)
			t[0] = av0;
		execve((const char *)p, (char *const *)&t[0],
		    (char *const *)xecenv);
		if (av0)
			t[0] = a0;
	}
	oex = ex;
	ex.ex_status =
	ex.ex_code = ERR_NOEXEC;
	ex.ex_pid = mypid;

	if (isvfork)
		exflag = TRUE;	/* Call _exit() instead of exit() */
	switch (errno) {
	case ENOEXEC:		/* could be a shell script */
		ex = oex;
		if (isvfork) {
			exflag = 2;
			_exit(126);
		}
		funcnt = 0;	/* Reset to global argc/argv */
		flags = 0;	/* Clear flags		*/
		flags2 = 0;	/* Clear flags2		*/
		*flagadr = 0;	/* Clear $-		*/
		comdiv = 0;	/* Clear sh -c arg	*/
		ioset = 0;	/* Clear /dev/null redirect */
		clearup();	/* remove open files and for loop junk */
		if (input)
			close(input);
		input = chkopen(p, O_RDONLY);	/* Open script as stdin	*/

#ifdef ACCT
		preacct(p);	/* reset accounting */
#endif
		exflag = oexflag;

		/*
		 * set up new args
		 */

		setargs(t);
		longjmp(subshell, 1);
		/* NOTREACHED */

	case ENOMEM:
		failedx(e, p, toobig);
		/* NOTREACHED */

	case E2BIG:
		failedx(e, p, arglist);
		/* NOTREACHED */

	case ETXTBSY:
		failedx(e, p, txtbsy);
		/* NOTREACHED */

#ifdef	ELIBACC
	case ELIBACC:
		failedx(e, p, libacc);
		/* NOTREACHED */
#endif

#ifdef	ELIBBAD
	case ELIBBAD:
		failedx(e, p, libbad);
		/* NOTREACHED */
#endif

#ifdef	ELIBSCN
	case ELIBSCN:
		failedx(e, p, libscn);
		/* NOTREACHED */
#endif

#ifdef	ELIBMAX
	case ELIBMAX:
		failedx(e, p, libmax);
		/* NOTREACHED */
#endif

	default:
		xecmsg = badexec;
		/* FALLTHROUGH */

	case ENOENT:
		if (errno == ENOENT)
			ex.ex_status =
			ex.ex_code = ERR_NOTFOUND;
		return (prefix);
	}
}

BOOL		nosubst;

void
trim(at)
	unsigned char	*at;
{
	unsigned char	*last;
	unsigned char	*current;
	unsigned char	c;
	int	len;
	wchar_t	wc;

	nosubst = 0;
	if ((current = at) != NULL) {
		last = at;
		(void) mbtowc(NULL, NULL, 0);
		while ((c = *current) != 0) {
			if ((len = mbtowc(&wc, (char *)current,
					MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				*last++ = c;
				current++;
				continue;
			}

			if (wc != '\\') {
				memmove(last, current, len);
				last += len;
				current += len;
				continue;
			}

			/* remove \ and quoted nulls */
			nosubst = 1;
			current++;
			if ((c = *current) != 0) {
				if ((len = mbtowc(&wc, (char *)current,
						MB_LEN_MAX)) <= 0) {
					(void) mbtowc(NULL, NULL, 0);
					*last++ = c;
					current++;
					continue;
				}
				memmove(last, current, len);
				last += len;
				current += len;
			} else
				current++;
		}

		*last = 0;
	}
}

/* Same as trim, but only removes backlashes before slashes */
void
trims(at)
unsigned char	*at;
{
	unsigned char	*last;
	unsigned char	*current;
	unsigned char	c;
	int	len;
	wchar_t	wc;

	if ((current = at) != NULL)
	{
		last = at;
		(void) mbtowc(NULL, NULL, 0);
		while ((c = *current) != 0) {
			if ((len = mbtowc(&wc, (char *)current,
					MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				*last++ = c;
				current++;
				continue;
			}

			if (wc != '\\') {
				memmove(last, current, len);
				last += len; current += len;
				continue;
			}

			/* remove \ and quoted nulls */
			current++;
			if ((c = *current) == '\0') {
				current++;
				continue;
			}

			if (c == '/') {
				*last++ = c;
				current++;
				continue;
			}

			*last++ = '\\';
			if ((len = mbtowc(&wc, (char *)current,
					MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				*last++ = c;
				current++;
				continue;
			}
			memmove(last, current, len);
			last += len; current += len;
		}
		*last = 0;
	}
}

unsigned char *
mactrim(s)
unsigned char	*s;
{
	unsigned char	*t = macro(s);

	trim(t);
	return (t);
}

unsigned char **
scan(argn)
int	argn;
{
	struct argnod *argp =
			(struct argnod *)(Rcheat(gchain) & ~ARGMK);
	unsigned char **comargn, **comargm;

	comargn = (unsigned char **)getstak((Intptr_t)BYTESPERWORD * argn +
							BYTESPERWORD);
	comargm = comargn += argn;
	*comargn = ENDARGS;
	while (argp)
	{
		*--comargn = argp->argval;

		trim(*comargn);
		argp = argp->argnxt;

		if (argp == 0 || Rcheat(argp) & ARGMK)
		{
			gsort(comargn, comargm);
			comargm = comargn;
		}
		argp = (struct argnod *)(Rcheat(argp) & ~ARGMK);
	}
	return (comargn);
}

static void
gsort(from, to)
unsigned char	*from[], *to[];
{
	int	k, m, n;
	int	i, j;

	if ((n = to - from) <= 1)
		return;
	for (j = 1; j <= n; j *= 2)
		/* LINTED */
		;
	for (m = 2 * j - 1; m /= 2; )
	{
		k = n - m;
		for (j = 0; j < k; j++)
		{
			for (i = j; i >= 0; i -= m)
			{
				unsigned char **fromi;

				fromi = &from[i];
				if (cf(fromi[m], fromi[0]) > 0)
				{
					break;
				}
				else
				{
					unsigned char *s;

					s = fromi[m];
					fromi[m] = fromi[0];
					fromi[0] = s;
				}
			}
		}
	}
}

/*
 * Argument list generation
 */
int
getarg(ac)
struct comnod	*ac;
{
	struct argnod	*argp;
	int		count = 0;
	struct comnod	*c;

	if ((c = ac) != NULL)
	{
		argp = c->comarg;
		while (argp)
		{
			count += split(macro(argp->argval));
			argp = argp->argnxt;
		}
	}
	return (count);
}

static int
split(s)		/* blank interpretation routine */
unsigned char	*s;
{
	unsigned char	*argp;
	int		c;
	int		count = 0;

	(void) mbtowc(NULL, NULL, 0);
	for (;;)
	{
		int clength;
		sigchk();
		argp = locstak() + BYTESPERWORD;
		while ((c = *s) != 0) {
			wchar_t wc;
			if ((clength = mbtowc(&wc, (char *)s,
					MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				wc = (unsigned char)*s;
				clength = 1;
			}

			if (c == '\\') { /* skip over quoted characters */
				GROWSTAK(argp);
				*argp++ = (char)c;
				s++;
				/* get rest of multibyte character */
				if ((clength = mbtowc(&wc, (char *)s,
						MB_LEN_MAX)) <= 0) {
					(void) mbtowc(NULL, NULL, 0);
					wc = (unsigned char)*s;
					clength = 1;
				}
				GROWSTAK(argp);
				*argp++ = *s++;
				while (--clength > 0) {
					GROWSTAK(argp);
					*argp++ = *s++;
				}
				continue;
			}

			if (anys(s, ifsnod.namval)) {
				/* skip to next character position */
				s += clength;
				break;
			}

			GROWSTAK(argp);
			*argp++ = c;
			s++;
			while (--clength > 0) {
				GROWSTAK(argp);
				*argp++ = *s++;
			}
		}
		if (argp == staktop + BYTESPERWORD)
		{
			if (c)
			{
				continue;
			}
			else
			{
				return (count);
			}
		}
		/*
		 * file name generation
		 */

		argp = endstak(argp);
		trims(((struct argnod *)argp)->argval);
		if ((flags & nofngflg) == 0 &&
			(c = expand(((struct argnod *)argp)->argval, 0)))
			count += c;
		else
		{
			makearg((struct argnod *)argp);
			count++;
		}
		gchain = (struct argnod *)((Intptr_t)gchain | ARGMK);
	}
}

#ifdef ACCT
#include	<sys/types.h>
#include	<sys/acct.h>
#include	<sys/times.h>

#ifdef	AHZV1		/* Newer FreeBSD versions */
struct acctv1 sabuf;
#else
struct acct sabuf;
#endif
struct tms buffer;
static clock_t before;
static int shaccton;	/* 0 implies do not write record on exit */
			/* 1 implies write acct record on exit */
	void suspacct	__PR((void));
	void preacct	__PR((unsigned char *cmdadrp));
	void doacct	__PR((void));
static comp_t compress	__PR((clock_t));


/*
 *	suspend accounting until turned on by preacct()
 */
void
suspacct()
{
	shaccton = 0;
}

void
preacct(cmdadrp)
	unsigned char	*cmdadrp;
{
	if (acctnod.namval && *acctnod.namval) {
		sabuf.ac_btime = time((time_t *)0);
		before = times(&buffer);
		sabuf.ac_uid = getuid();
		sabuf.ac_gid = getgid();
		movstrn(simple(cmdadrp), (unsigned char *)sabuf.ac_comm,
			sizeof (sabuf.ac_comm));
		shaccton = 1;
	}
}

void
doacct()
{
	int fd;
	clock_t after;

	if (shaccton) {
		after = times(&buffer);
		sabuf.ac_utime = compress(buffer.tms_utime + buffer.tms_cutime);
		sabuf.ac_stime = compress(buffer.tms_stime + buffer.tms_cstime);
		sabuf.ac_etime = compress(after - before);

		if ((fd = open((char *)acctnod.namval,
				O_WRONLY | O_APPEND | O_CREAT, 0666)) != -1) {
			write(fd, &sabuf, sizeof (sabuf));
			close(fd);
		}
	}
}

/*
 *	Produce a pseudo-floating point representation
 *	with 3 bits base-8 exponent, 13 bits fraction
 */

static comp_t
compress(t)
	clock_t	t;
{
	int exp = 0;
	int rund = 0;

	while (t >= 8192) {
		exp++;
		rund = t & 04;
		t >>= 3;
	}

	if (rund) {
		t++;
		if (t >= 8192) {
			t >>= 3;
			exp++;
		}
	}
	return ((exp << 13) + t);
}
#endif
