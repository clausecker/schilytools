/* @(#)test.c	1.37 18/07/02 Copyright 1986,1995-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)test.c	1.37 18/07/02 Copyright 1986,1995-2018 J. Schilling";
#endif
/*
 *	Test routine (the test builtin command)
 *
 *	Copyright (c) 1986,1995-2018 J. Schilling
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

#include <schily/fcntl.h>
#include <schily/stdio.h>
#include <schily/unistd.h>
#include <schily/setjmp.h>
#include <schily/jmpdefs.h>
#include <schily/varargs.h>
#include <schily/stat.h>
#include "bsh.h"
#include "str.h"
#include "strsubs.h"
#include <schily/string.h>

#ifndef	R_OK
#define	R_OK		4
#endif
#ifndef	W_OK
#define	W_OK		2
#endif
#ifndef	X_OK
#define	X_OK		1
#endif
#ifndef	F_OK
#define	F_OK		0
#endif

#ifndef	HAVE_LSTAT
#	define	lstat	stat
#undef	AT_SYMLINK_NOFOLLOW
#define	AT_SYMLINK_NOFOLLOW	0
#endif

/* Kostet mehr Code den ganzen Kram auf den Stack zu tun */

LOCAL	jmps_t	*tjmp	= 0;	/* jmp_buf steht auf dem Stack */
LOCAL	int	tac	= 0;
LOCAL	char	**tav	= 0;
LOCAL	FILE	**tstd	= 0;
LOCAL	Argvec	*tvp	= 0;

#define	exp2	_exp2	/* Some compilers do not like exp2() */

EXPORT	void	bcompute	__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	btest		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bexpr		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	BOOL	test		__PR((Argvec * vp, FILE ** std));
LOCAL	char	*getarg		__PR((BOOL flg));
LOCAL	int	getiarg		__PR((BOOL flg));
LOCAL	void	ungetarg	__PR((void));
LOCAL	int	expr		__PR((void));
LOCAL	int	exp0		__PR((void));
LOCAL	int	exp1		__PR((void));
LOCAL	int	exp2		__PR((void));
LOCAL	int	expn		__PR((void));
LOCAL	int	ass_expr	__PR((char *name, char *op, int y));
LOCAL	BOOL	access_ok	__PR((char *name, int mode));
LOCAL	BOOL	fattr		__PR((char *name, int type));
LOCAL	BOOL	ftype		__PR((char *name, int type));
EXPORT	BOOL	is_dir		__PR((char *name));
LOCAL	BOOL	fowner		__PR((char *name, int owner));
LOCAL	BOOL	fgroup		__PR((char *name, int group));
LOCAL	off_t	fsize		__PR((char *name));
LOCAL	BOOL	isttyf		__PR((int i));
LOCAL	int	lstatat		__PR((char *name, struct stat *buf, int flag));
LOCAL	void	expr_syntax	__PR((char *fmt, ...));

/* ARGSUSED */
EXPORT void
bcompute(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	fprintf(std[2], "compute obsolete use 'test' or '@'.\n");
	test(vp, std);
}

/* ARGSUSED */
EXPORT void
btest(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	test(vp, std);
}

/* ARGSUSED */
EXPORT void
bexpr(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
		char	buf[12];
		jmps_t	exprjmp;
	register int	ac;
	register char	**av;

	tjmp = &exprjmp;
	if (!setjmp(exprjmp.jb)) {
		ac	= vp->av_ac;
		av	= vp->av_av;
		tvp = vp;
		tstd = std;
		if ((tac = ac - 3) <= 0)
			expr_syntax(expected, argument);
		tav = &av[3];
		sprintf(buf, "%d", ass_expr(av[1], av[2], expr()));
		ev_insert(concat(av[1], eql, buf, (char *)NULL));
	}
}

EXPORT BOOL
test(vp, std)
	Argvec	*vp;
	FILE	*std[];
{
		jmps_t	testjmp;
	register int	ac;
	register char	**av;

	tjmp = &testjmp;
	if (!setjmp(testjmp.jb)) {
		ac	= vp->av_ac;
		av	= vp->av_av;
		tvp = vp;
		tstd = std;
		if (streql(av[0], "[") && !streql(av[--ac], "]"))
			expr_syntax("%s ']'", emissing);
		tac = --ac;
		tav = &av[1];
		ex_status = expr() ? 0 : 1;
		return (TRUE);
	}
	return (FALSE);		/* fuer bif() */
}

LOCAL char *
getarg(flg)
	BOOL	flg;
{
	if (tac-- <= 0) {
		if (flg) {
			tac++;
			return (NULL);
		}
		expr_syntax(expected, argument);
	}
	return (*tav++);
}

/* ARGSUSED */
LOCAL int
getiarg(flg)
	BOOL	flg;
{
	int	i;

	if (!toint(tstd, getarg(0), &i))
		expr_syntax(expected, number);
	return (i);
}


LOCAL void
ungetarg()
{
	tac++, tav--;
}

LOCAL int
expr()
{
	int	x;
	char	*op;

	x = exp0();
	if ((op = getarg(1)) != NULL)
		expr_syntax(unexpected, op);
	return (x);
}

LOCAL int
exp0()
{
	int	x;
	char	*op;

	x = exp1();
	if ((op = getarg(1)) != NULL) {
		if (streql(op, "-o"))
			return (x | exp0());
		if (streql(op, "-or"))
			return (x || exp0());
		ungetarg();
	}
	return (x);
}

LOCAL int
exp1()
{
	int	x;
	char	*op;

	x = exp2();
	if ((op = getarg(1)) != NULL) {
		if (streql(op, "-a"))
			return (x & exp1());
		if (streql(op, "-and"))
			return (x && exp1());
		ungetarg();
	}
	return (x);
}

LOCAL int
exp2()
{

	if (streql(getarg(0), "!"))
		return (!expn());
	ungetarg();
	return (expn());
}

LOCAL int
expn()
{
	register char	*a;
	register char	*op;
		int	x;
		int	y;

	a = getarg(0);
	if (streql(a, lpar)) {
		x = exp0();
		if (!(a = getarg(1)) || !streql(a, rpar))
			expr_syntax("%s %s", emissing, rpar);
		return (x);
	}
	if (streql(a, "-r"))
		return (access_ok(getarg(0), R_OK));
	if (streql(a, "-w"))
		return (access_ok(getarg(0), W_OK));
	if (streql(a, "-x"))
		return (access_ok(getarg(0), X_OK));
	if (streql(a, "-e"))			/* ksh uses -a */
		return (access_ok(getarg(0), F_OK));
	if (streql(a, "-O"))			/* non-POSIX */
		return (fowner(getarg(0), geteuid()));
	if (streql(a, "-G"))			/* non-POSIX */
		return (fgroup(getarg(0), getegid()));
	if (streql(a, "-s"))
		return (fsize(getarg(0)) > (off_t)0);
	if (streql(a, "-S"))
#ifdef	S_IFSOCK
		return (ftype(getarg(0), S_IFSOCK));
#else
		return (FALSE);
#endif
	if (streql(a, "-d"))
		return (ftype(getarg(0), S_IFDIR));
	if (streql(a, "-D"))
#ifdef	S_IFDOOR
		return (ftype(getarg(0), S_IFDOOR));
#else
		return (FALSE);
#endif
	if (streql(a, "-c"))
#ifdef	S_IFCHR
		return (ftype(getarg(0), S_IFCHR));
#else
		return (FALSE);
#endif
	if (streql(a, "-b"))
#ifdef	S_IFBLK
		return (ftype(getarg(0), S_IFBLK));
#else
		return (FALSE);
#endif
	if (streql(a, "-f"))
		return (ftype(getarg(0), S_IFREG));
	if (streql(a, "-h") || streql(a, "-L"))	/* -L is for ksh compat */
#ifdef	S_IFLNK
		return (ftype(getarg(0), S_IFLNK));
#else
		return (FALSE);
#endif
	if (streql(a, "-C"))			/* non-POSIX aber genannt */
#ifdef	S_IFCTG
		return (ftype(getarg(0), S_IFCTG));
#else
		return (FALSE);
#endif
	if (streql(a, "-p"))
#ifdef	S_IFIFO
		return (ftype(getarg(0), S_IFIFO));
#else
		return (FALSE);
#endif
	if (streql(a, "-P"))
#ifdef	S_IFPORT
		return (ftype(getarg(0), S_IFPORT));
#else
		return (FALSE);
#endif
	if (streql(a, "-u"))
		return (fattr(getarg(0), S_ISUID));
	if (streql(a, "-g"))
		return (fattr(getarg(0), S_ISGID));
	if (streql(a, "-k"))			/* non-POSIX aber genannt */
		return (fattr(getarg(0), S_ISVTX));
	if (streql(a, "-t"))
		return (isatty(getiarg(0)));
	if (streql(a, "-T"))			/* non-POSIX */
		return (isttyf(getiarg(0)));
	if (streql(a, "-l"))			/* non-POSIX aber genannt */
		return (strlen(getarg(0)));
	if (streql(a, "-n"))
		return (!streql(getarg(0), nullstr));
	if (streql(a, "-z"))
		return (streql(getarg(0), nullstr));
	op = getarg(1);
	if (op == NULL) {
		if (*astoi(a, &x) == '\0')
			return (x);
		return (!streql(a, nullstr));
	}
	if (streql(op, "-a") || streql(op, "-o") ||
		streql(op, "-and") || streql(op, "-or")) {
		ungetarg();
		return (!streql(a, nullstr));	/* TRUE if not "" */
	}
	if (streql(op, eql) || streql(op, "=="))
		return (streql(getarg(0), a));
	if (streql(op, "!="))
		return (!streql(getarg(0), a));
	if (!toint(tstd, a, &x) || !toint(tstd, getarg(0), &y))
		expr_syntax(expected, number);
	if (streql(op, "+"))
		return (x + y);
	if (streql(op, "-"))
		return (x - y);
	if (streql(op, "*"))
		return (x * y);
	if (streql(op, slash)) {
		if (y == 0)
			expr_syntax(divzero);	/* never returns */
		return (x / y);
	}
	if (streql(op, "%")) {
		if (y == 0)
			expr_syntax(divzero);	/* never returns */
		return (x % y);
	}
	if (streql(op, "&"))
		return (x & y);
	if (streql(op, "|"))
		return (x | y);
	if (streql(op, "&&"))
		return (x && y);
	if (streql(op, "||"))
		return (x || y);
	if (streql(op, "-eq"))
		return (x == y);
	if (streql(op, "-ne"))
		return (x != y);
	if (streql(op, ">") || streql(op, "-gt"))
		return (x > y);
	if (streql(op, "<") || streql(op, "-lt"))
		return (x < y);
	if (streql(op, ">=") || streql(op, "-ge"))
		return (x >= y);
	if (streql(op, "<=") || streql(op, "-le"))
		return (x <= y);
	if (streql(op, "<<"))
		return (x << y);
	if (streql(op, ">>"))
		return (x >> y);
	expr_syntax(ebadop, op);	/* never returns */
	return (0);			/* Keep lint happy */
}

LOCAL int
ass_expr(name, op, y)
		char	*name;
	register char	*op;
		int	y;
{
		char	*val;
		int	x;

	if (streql(op, eql))
		return (y);
	if (strlen(op) != 2 || op[1] != '=')
		expr_syntax(ebadop, op);
	if (!(val = getcurenv(name)))
		expr_syntax("Undefined Variable '%s'", name);
	else if (!toint(tstd, val, &x))
		expr_syntax(expected, number);		/* never returns */

	switch (op[0]) {

	case '+' :	return (x + y);
	case '-' :	return (x - y);
	case '/' :	if (y == 0)
				expr_syntax(divzero);	/* never returns */
			return (x / y);
	case '%' :	if (y == 0)
				expr_syntax(divzero);	/* never returns */
			return (x % y);
	case '*' :	return (x * y);

	default  :	expr_syntax(ebadop, op);	/* never returns */

	}
	return (0);			/* Keep lint happy */
}

LOCAL BOOL
access_ok(name, mode)
	char	*name;
	int	mode;
{
	return (access(name, mode) == 0 ? TRUE : FALSE);
}

LOCAL BOOL
fattr(name, type)
	char	*name;
	int	type;
{
	struct	stat	buf;

	if (lstatat(name, &buf, AT_SYMLINK_NOFOLLOW) < 0)
		return (FALSE);
	return ((buf.st_mode & type) == type);
}

LOCAL BOOL
ftype(name, type)
	char	*name;
	int	type;
{
	struct	stat	buf;

	if (lstatat(name, &buf, AT_SYMLINK_NOFOLLOW) < 0)
		return (FALSE);
	return ((buf.st_mode & S_IFMT) == type);
}

EXPORT BOOL
is_dir(name)
	char	*name;
{
/*	return (ftype(name, S_IFDIR));*/

	struct	stat	buf;

	if (lstatat(name, &buf, 0) < 0)
		return (FALSE);
	return ((buf.st_mode & S_IFMT) == S_IFDIR);
}

LOCAL BOOL
fowner(name, owner)
	char	*name;
	int	owner;
{
	struct	stat	buf;

	if (lstatat(name, &buf, AT_SYMLINK_NOFOLLOW) < 0)
		return (FALSE);
	return (buf.st_uid == owner);
}

LOCAL BOOL
fgroup(name, group)
	char	*name;
	int	group;
{
	struct	stat	buf;

	if (lstatat(name, &buf, AT_SYMLINK_NOFOLLOW) < 0)
		return (FALSE);
	return (buf.st_gid == group);
}

LOCAL off_t
fsize(name)
	char	*name;
{
	struct	stat	buf;

	if (lstatat(name, &buf, AT_SYMLINK_NOFOLLOW) < 0)
		return (-1);
	return (buf.st_size);
}

LOCAL BOOL
isttyf(i)
	int	i;
{
	return ((i < 0 || i > 2) ? FALSE : isatty(fdown(tstd[i])));
}

LOCAL int
lstatat(name, buf, flag)
	char		*name;
	struct stat	*buf;
	int		flag;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	char	*p2;
	int	fd;
	int	dfd;
	int	err;
#endif
	int	ret;

	if ((ret = fstatat(AT_FDCWD, name, buf, flag)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	p = name;
	fd = AT_FDCWD;
	while (*p) {
		if ((p2 = strchr(p, '/')) != NULL)
			*p2 = '\0';
		else
			break;
		if ((dfd = openat(fd, p, O_RDONLY|O_DIRECTORY|O_NDELAY)) < 0) {
			err = geterrno();

			close(fd);
			if (err == EMFILE)
				seterrno(err);
			else
				seterrno(ENAMETOOLONG);
			*p2 = '/';
			return (dfd);
		}
		close(fd);
		fd = dfd;
		if (p2 == NULL)
			break;
		*p2++ = '/';
		p = p2;
	}
	ret = fstatat(fd, p, buf, flag);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL void
expr_syntax(char *fmt, ...)
#else
LOCAL void
expr_syntax(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	fprintf(tstd[2], "%s: %r\n", tvp->av_av[0], fmt, args);
	va_end(args);
	busage(tvp, tstd);
	ex_status = -1;
	longjmp(tjmp->jb, TRUE);
}
