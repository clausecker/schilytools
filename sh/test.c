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
#pragma ident	"@(#)test.c	1.17	06/06/20 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2018 J. Schilling
 *
 * @(#)test.c	1.41 18/07/01 2008-2018 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)test.c	1.41 18/07/01 2008-2018 J. Schilling";
#endif


/*
 *      test expression
 *      [ expression ]
 */

#ifdef	SCHILY_INCLUDES
#include	<schily/types.h>
#include	<schily/fcntl.h>
#include	<schily/stat.h>
#else
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

#ifndef	HAVE_LSTAT
#define	lstat	stat
#undef	AT_SYMLINK_NOFOLLOW
#define	AT_SYMLINK_NOFOLLOW	0
#endif
#ifndef	HAVE_DECL_STAT
extern int stat	__PR((const char *, struct stat *));
#endif
#ifndef	HAVE_DECL_LSTAT
extern int lstat __PR((const char *, struct stat *));
#endif

#define	exp	_exp	/* Some compilers do not like exp() */

	int	test		__PR((int argn, unsigned char *com[]));
#ifdef	DO_SYSATEXPR
	void	expr		__PR((int argn, unsigned char *com[]));
static	long	aexpr		__PR((struct namnod *n,
						unsigned char *op, long y));
#endif
static	unsigned char *nxtarg	__PR((int mt));
static	int	exp		__PR((void));
static	int	e1		__PR((void));
static	int	e2		__PR((void));
static	int	e3		__PR((void));
static	int	test_unary	__PR((int op, unsigned char *arg));
static	int	test_binary	__PR((unsigned char *arg1, int op,
					unsigned char *arg2));
static	int	ftype		__PR((unsigned char *f, int field));
static	int	filtyp		__PR((unsigned char *f, int field));
#ifdef	DO_EXT_TEST
static	int	fsame		__PR((unsigned char *f1, unsigned char *f2));
static	int	ftime		__PR((unsigned char *f1, unsigned char *f2));
static	int	fnew		__PR((unsigned char *));
static	int	fowner		__PR((unsigned char *f, uid_t owner));
static	int	fgroup		__PR((unsigned char *f, uid_t owner));
#endif
static	int	fsizep		__PR((unsigned char *f));
static	Intmax_t str2imax	__PR((unsigned char *a));

#undef	failed
#undef	bfailed
#define		failed(s1, s2)		bfailed(s1, s2, NULL)
static	void	bfailed		__PR((unsigned char *s1,
					const char *s2,
					unsigned char *s3));

#ifdef	DO_POSIX_TEST
/*
 * In POSIX mode, we need to overwrite the standard error code with a
 * value > 1, this is ETEST (2).
 */
#undef	ERROR
#define	ERROR	ETEST
#endif

#ifdef	DO_SYSATEXPR
static int	isexpr;
#endif
static int	ap, ac;
static unsigned char **av;
static jmp_buf	testsyntax;

#define	nargs()	(ac - ap)

/*
 * Set up test_unops[] to avoid calling test_unary() with non-unary
 * operators. The following #defines influence the number os unary
 * operators:
 *
 * DO_POSIX_TEST				-e -S
 * DO_EXT_TEST					-C -D -G -N -O -P
 * defined(DO_EXT_TEST) && defined(DO_SET_O)	-o
 * normal unaries from old Bourne Shell		Lbcdfghknprstuwxz
 */
#ifdef	PROTOTYPES
const char	test_unops[] = "Lbcdfghknprstuwxz"
#ifdef	DO_POSIX_TEST
		"eS"
#endif
#ifdef	DO_EXT_TEST
		"CDGNOP"
#endif
#if defined(DO_EXT_TEST) && defined(DO_SET_O)
		"o"
#endif
		"";
#else	/* !PROTOTYPES */
#if defined(DO_POSIX_TEST) || defined(DO_EXT_TEST)
const char	test_unops[] = "LbcdfghknprstuwxzeSCDGNOPo";
#else
const char	test_unops[] = "Lbcdfghknprstuwxz";
#endif
#endif	/* !PROTOTYPES */

int
test(argn, com)
	int		argn;
	unsigned char	*com[];
{
#ifdef	DO_POSIX_TEST
	int	not;
	int	inv = 0;
	int	op;
#endif
	ac = argn;
	av = com;
	ap = 1;
#ifdef	DO_SYSATEXPR
	isexpr = 0;
#endif

	if (eq(com[0], "[")) {
		if (!eq(com[--ac], "]")) {
			Failure((unsigned char *)"test", nobracket);
			return (SYNBAD);
		}
	}
	com[ac] = 0;
	if (ac <= 1)				/* POSIX case: 0 args	*/
		return (1);			/* test exits false	*/
	if (setjmp(testsyntax))
		return (ETEST);

#ifdef	DO_POSIX_TEST
	not = com[1][0] == '!' && com[1][1] == '\0';
	com++;
	switch (ac) {

	default:				/* POSIX >4 args unspec	*/
		break;

	case 5:
		if (not) {
			inv = 1;
			com++;
			not = com[0][0] == '!' && com[0][1] == '\0';

		} else if ((com[0][0] == '(' && com[0][1] == '\0') &&
			    (com[3][0] == ')' && com[3][1] == '\0')) {
			com++;
			not = com[0][0] == '!' && com[0][1] == '\0';
			goto two;
		} else {
			break;			/* Unspecified by POSIX	*/
		}
		/* FALLTHROUGH */

	case 4:
		op = syslook(com[1], test_ops, no_test_ops);
		if (op)
			return (inv ^ !test_binary(com[0], op, com[2]));

		if (not) {
			inv = 1;
			com++;
			not = com[0][0] == '!' && com[0][1] == '\0';

		} else if ((com[0][0] == '(' && com[0][1] == '\0') &&
			    (com[2][0] == ')' && com[2][1] == '\0')) {
			com++;
			not = com[0][0] == '!' && com[0][1] == '\0';
			goto one;
		} else {
			break;			/* Unspecified by POSIX	*/
		}
		/* FALLTHROUGH */
	case 3:
	two:
		if (not)
			return (inv ^ (com[1][0] != 0));

		if (com[0][0] == '-' &&
		    com[0][1] != '\0' && com[0][2] == '\0' &&
		    strchr(test_unops, com[0][1]))
			return (inv ^ !test_unary(com[0][1], com[1]));

		break;				/* Unspecified by POSIX	*/
	case 2:
	one:
		/*
		 * Compatibility for UNIX -t without parameter
		 */
		if (!(flags2 & posixflg) && eq(com[0], "-t"))
			break;

		return (inv ^ (com[0][0] == 0));
	}
#endif

	return (exp() ? 0 : 1);
}

#ifdef	DO_SYSATEXPR
void
expr(argn, com)
	int		argn;
	unsigned char	*com[];
{
	int		incr = 0;
	struct namnod	*n = NULL;		/* Make GCC happy */
	char		buf[40];

	ac = argn;
	av = com;
	ap = optskip(argn, com, "@ expr");
	if (ap < 0)
		return;
	isexpr = 1;

	if (ac == 2) {				/* @ var++ */
		int len = length(av[ap]);

		if (len > 3) {
			unsigned char	*p = av[ap] + len - 3;

			if (eq(p, "++"))
				incr = 1;
			else if (eq(p, "--"))
				incr = -1;
			if (incr) {
				*p = '\0';
				n = lookup(av[ap]);
			}
		}
		if (incr == 0) {
			Failure((unsigned char *)"@", noarg);
			return;
		}
	} else if (ac < 4) {
		Failure((unsigned char *)"@", noarg);
		return;
	}
	if (setjmp(testsyntax))
		return;
	if (incr) {
		snprintf(buf, sizeof (buf), "%ld", aexpr(n, UC "+=", incr));
	} else {
		n = lookup(av[ap++]);
		snprintf(buf, sizeof (buf), "%ld", aexpr(n, av[ap++], exp(0)));
	}
	assign(n, UC buf);
}

static long
aexpr(n, op, y)
	struct namnod	*n;
	unsigned char	*op;
	long		y;
{
	long		x;
	char		c = *op;

	if (eq(op, "="))
		return (y);

	if (c == 0 || op[1] != '=' || op[2])
		bfailed((unsigned char *)"@", badop, op);

	if (n->namval == NULL)
		failed((unsigned char *)"@", unset);
	x = str2imax(n->namval);

	switch (c) {

	case '+':	return (x + y);
	case '-':	return (x - y);
	case '*':	return (x * y);
	case '/':
			if (y == 0)
				failed((unsigned char *)"@", divzero);
			return (x / y);
	case '%':
			if (y == 0)
				failed((unsigned char *)"@", divzero);
			return (x % y);

	default:
		bfailed((unsigned char *)"@", badop, op);
	}

	return (-1);
}
#endif

static unsigned char *
nxtarg(mt)
	int	mt;
{
	if (ap >= ac) {
		if (mt) {
			ap++;
			return (0);
		}
		failed((unsigned char *)"test", noarg);
	}
	return (av[ap++]);
}

/*
 * The main test expression evaluator.
 * Returns	0 -> FALSE
 *		!= 0 -> TRUE
 */
/* ARGSUSED */
static int
exp()
{
	int	p1;
	unsigned char	*p2;

	p1 = e1();
	p2 = nxtarg(1);
	if (p2 != 0) {
		if (eq(p2, "-o"))
			return (p1 | exp());

#ifdef	__nono__
		if (!eq(p2, ")"))
			failed((unsigned char *)"test", synmsg);
#endif
	}
	ap--;
	return (p1);
}

static int
e1()
{
	int	p1;
	unsigned char	*p2;

	p1 = e2();
	p2 = nxtarg(1);

	if ((p2 != 0) && eq(p2, "-a"))
		return (p1 & e1());
	ap--;
	return (p1);
}

static int
e2()
{
	if (eq(nxtarg(0), "!"))
		return (!e3());
	ap--;
	return (e3());
}

static int
e3()
{
	int	p1;
	unsigned char	*a;
	unsigned char	*p2;

	a = nxtarg(0);
	if (eq(a, "(")) {
		p1 = exp();
		if (!eq(nxtarg(0), ")"))
			failed((unsigned char *)"test", noparen);
		return (p1);
	}
	p2 = nxtarg(1);
	ap--;
	if ((p2 == 0) || (!eq(p2, "=") && !eq(p2, "!="))) {
		if (eq(a, "-t")) {
			unsigned char	*na;

			if (ap >= ac)		/* no args */
				return (isatty(STDOUT_FILENO));
			na = nxtarg(0);
			ap--;
			if (eq(na, "-a") || eq(na, "-o"))
				return (isatty(STDOUT_FILENO));
		}
		if (a[0] == '-' && a[1] != '\0' && a[2] == '\0' &&
		    strchr(test_unops, a[1]))
			return (test_unary(a[1], nxtarg(0)));
	}

	p2 = nxtarg(1);
	if (p2 == 0) {
#ifdef	DO_SYSATEXPR
		if (isexpr && digit(*a)) {
			ll_1 = str2imax(a);
			return (ll_1);
		}
#endif
		return (!eq(a, ""));
	}

	p1 = syslook(p2, test_ops, no_test_ops);
	if (p1 == TEST_AND || p1 == TEST_OR) {
		ap--;
		return (!eq(a, ""));
	}
	if (p1) {
		return (test_binary(a, p1, nxtarg(0)));
	}

#ifdef	DO_SYSATEXPR
	if (isexpr) {
		char	c = *p2;

		if (c && p2[1] == '\0') {
			if (c == '+')
				return (ll_1 + ll_2);
			if (c == '-')
				return (ll_1 - ll_2);
			if (c == '*')
				return (ll_1 * ll_2);
			if (c == '/') {
				if (ll_2 == 0)
					failed((unsigned char *)"@", divzero);
				return (ll_1 / ll_2);
			}
			if (c == '%') {
				if (ll_2 == 0)
					failed((unsigned char *)"@", divzero);
				return (ll_1 % ll_2);
			}
			if (c == '&')
				return (ll_1 & ll_2);
			if (c == '|')
				return (ll_1 | ll_2);
			if (c == '>')
				return (ll_1 > ll_2);
			if (c == '<')
				return (ll_1 < ll_2);
		}
		if (eq(p2, "&&"))
			return (ll_1 && ll_2);
		if (eq(p2, "||"))
			return (ll_1 || ll_2);
		if (eq(p2, ">="))
			return (ll_1 >= ll_2);
		if (eq(p2, "<="))
			return (ll_1 <= ll_2);
		if (eq(p2, ">>"))
			return (ll_1 >> ll_2);
		if (eq(p2, "<<"))
			return (ll_1 << ll_2);
	}
#endif

	bfailed((unsigned char *)btest, badop, p2);
	/* NOTREACHED */

	return (0);		/* Not reached, but keeps GCC happy */
}

static int
test_unary(op, arg)
	int		op;
	unsigned char	*arg;
{
	switch (op) {
#ifdef	DO_POSIX_TEST
	case 'e':
			return (chk_access(arg, F_OK, 0) == 0);
#endif
	case 'r':
			return (chk_access(arg, S_IREAD, 0) == 0);
	case 'w':
			return (chk_access(arg, S_IWRITE, 0) == 0);
	case 'x':
			return (chk_access(arg, S_IEXEC, 0) == 0);
	case 'd':
			return (filtyp(arg, S_IFDIR));
#ifdef	DO_EXT_TEST
	case 'D':
#ifdef	S_IFDOOR
			return (filtyp(arg, S_IFDOOR));
#else
			return (0);
#endif
	case 'C':
#ifdef	S_IFCTG
			return (filtyp(arg, S_IFCTG));
#else
			return (0);
#endif
#endif
	case 'c':
			return (filtyp(arg, S_IFCHR));
	case 'b':
			return (filtyp(arg, S_IFBLK));
	case 'f':
			if (ucb_builtins) {
				struct stat statb;

				return (lstatat((char *)arg, &statb, 0) >= 0 &&
					(statb.st_mode & S_IFMT) != S_IFDIR);
			} else {
				return (filtyp(arg, S_IFREG));
			}
	case 'u':
			return (ftype(arg, S_ISUID));
	case 'g':
			return (ftype(arg, S_ISGID));
	case 'k':
#ifdef	S_ISVTX
			return (ftype(arg, S_ISVTX));
#else
			return (0);
#endif
#if	defined(DO_EXT_TEST) && defined(DO_SET_O)
	case 'o':
			if (arg && *arg == '?') {
				return (lookopt(++arg) != NULL);
			}
			return (optval(lookopt(arg)));
#endif
	case 'p':
			return (filtyp(arg, S_IFIFO));
	case 'h':
	case 'L':
			return (filtyp(arg, S_IFLNK));
#ifdef	DO_EXT_TEST
	case 'P':
#ifdef	S_IFPORT
			return (filtyp(arg, S_IFPORT));
#else
			return (0);
#endif
#endif
#ifdef	DO_POSIX_TEST
	case 'S':
#ifdef	S_IFSOCK
			return (filtyp(arg, S_IFSOCK));
#else
			return (0);
#endif
#endif
	case 's':
			return (fsizep(arg));
	case 't':
			return (isatty(atoi((char *)arg)));
#ifdef	DO_EXT_TEST
	case 'O':
			return (fowner(arg, geteuid()));
	case 'G':
			return (fgroup(arg, getegid()));
	case 'N':
			return (fnew(arg));
#endif
	case 'n':
			return (!eq(arg, ""));
	case 'z':
			return (eq(arg, ""));

	default:
			failed((unsigned char *)"test", noarg);
			return (SYNBAD);
	}
}

static int
test_binary(arg1, op, arg2)
	unsigned char	*arg1;
	int		op;
	unsigned char	*arg2;
{
	Intmax_t	ll_1 = 0;	/* Avoid warning from silly GCC */
	Intmax_t	ll_2 = 0;	/* Avoid warning from silly GCC */

	if (op >= TEST_EQ) {
		ll_1 = str2imax(arg1);
		ll_2 = str2imax(arg2);
	}
	switch (op) {

	case TEST_AND:	return (*arg1 && *arg2);
	case TEST_OR:	return (*arg1 || *arg2);

#ifdef	DO_EXT_TEST
	case TEST_EF:	return (fsame(arg1, arg2));
	case TEST_NT:	return (ftime(arg1, arg2) > 0);
	case TEST_OT:	return (ftime(arg1, arg2) < 0);
#endif
	case TEST_SEQ:	return (eq(arg1, arg2));
	case TEST_SNEQ:	return (!eq(arg1, arg2));

	case TEST_EQ:	return (ll_1 == ll_2);
	case TEST_NE:	return (ll_1 != ll_2);
	case TEST_GT:	return (ll_1 > ll_2);
	case TEST_LT:	return (ll_1 < ll_2);
	case TEST_GE:	return (ll_1 >= ll_2);
	case TEST_LE:	return (ll_1 <= ll_2);

	default:	return (SYNBAD);
	}
}

static int
ftype(f, field)
	unsigned char	*f;
	int		field;
{
	struct stat statb;

	if (lstatat((char *)f, &statb, 0) < 0)
		return (0);
	if ((statb.st_mode & field) == field)
		return (1);
	return (0);
}

static int
filtyp(f, field)
	unsigned char	*f;
	int		field;
{
	struct stat statb;
	int	flag = (field == S_IFLNK) ? AT_SYMLINK_NOFOLLOW : 0;

	if (lstatat((char *)f, &statb, flag) < 0)
		return (0);
	if ((statb.st_mode & S_IFMT) == field)
		return (1);
	else
		return (0);
}

#ifdef	DO_EXT_TEST
static int
fsame(f1, f2)
	unsigned char	*f1;
	unsigned char	*f2;
{
	struct	stat	statb1;
	struct	stat	statb2;

	if (lstatat((char *)f1, &statb1, 0) < 0)	/* lstat() ??? */
		return (FALSE);
	if (lstatat((char *)f2, &statb2, 0) < 0)	/* lstat() ??? */
		return (FALSE);
	if (statb1.st_ino == statb2.st_ino && statb1.st_dev == statb2.st_dev)
		return (1);
	return (FALSE);
}

static int					/* mod time f1 - mod time f2 */
ftime(f1, f2)
	unsigned char	*f1;
	unsigned char	*f2;
{
	struct	stat	statb1;
	struct	stat	statb2;
	int		ret;

	ret = lstatat((char *)f1, &statb1, 0);	/* lstat() ??? */
	if (lstatat((char *)f2, &statb2, 0) < 0) /* lstat() ??? */
		return (ret < 0 ? 0:1);
	if (ret < 0)
		return (-1);
	if (statb1.st_mtime > statb2.st_mtime)
		return (1);
	if (statb1.st_mtime == statb2.st_mtime &&
	    stat_mnsecs(&statb1) > stat_mnsecs(&statb2))
		return (1);
	if (statb1.st_mtime < statb2.st_mtime)
		return (-1);
	if (statb1.st_mtime == statb2.st_mtime &&
	    stat_mnsecs(&statb1) < stat_mnsecs(&statb2))
		return (-1);
	return (0);
}

static int
fnew(f)
	unsigned char	*f;
{
	struct	stat	statb;

	if (lstatat((char *)f, &statb, 0) < 0)	/* lstat() ??? */
		return (0);

	if (statb.st_mtime > statb.st_atime)
		return (1);
	if (statb.st_mtime == statb.st_atime &&
	    stat_mnsecs(&statb) > stat_ansecs(&statb))
		return (1);
	return (0);
}

static int
fowner(f, owner)
	unsigned char	*f;
	uid_t		owner;
{
	struct	stat	statb;

	if (lstatat((char *)f, &statb, 0) < 0)	/* lstat() ??? */
		return (FALSE);
	return (statb.st_uid == owner);
}

static int
fgroup(f, group)
	unsigned char	*f;
	gid_t		group;
{
	struct	stat	statb;

	if (lstatat((char *)f, &statb, 0) < 0)	/* lstat() ??? */
		return (FALSE);
	return (statb.st_gid == group);
}
#endif

static int
fsizep(f)
	unsigned char	*f;
{
	struct stat statb;

	if (lstatat((char *)f, &statb, 0) < 0)
		return (0);
	return (statb.st_size > 0);
}

static Intmax_t
str2imax(a)
	unsigned char	*a;
{
	Intmax_t	i;
	char		*ep;

#ifdef	HAVE_STRTOLL
	i = strtoll((char *)a, &ep, 10);
#else
	i = strtol((char *)a, &ep, 10);
#endif
#ifdef	DO_POSIX_TEST
	if ((char *)a == ep || *ep != '\0')
		bfailed((unsigned char *)ep, badnum, NULL);
#endif
	return (i);
}

static void
bfailed(s1, s2, s3)
	unsigned char	*s1;
	const char	*s2;
	unsigned char	*s3;
{
#ifdef	DO_POSIX_FAILURE
	failure_real(ETEST, s1, s2, s3, 0);
#else
	failed_real(ETEST, s1, s2, s3);
#endif
	longjmp(testsyntax, 1);
}
