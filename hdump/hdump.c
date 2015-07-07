/* @(#)hdump.c	1.36 15/06/25 Copyright 1986-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)hdump.c	1.36 15/06/25 Copyright 1986-2015 J. Schilling";
#endif
/*
 *	hex dump for files
 *	od "octal" dump
 *
 *	This is an attempt to implement POSIX behavior as well as traditional
 *	Solaris od(1) behavior.
 *
 *	Note that we do not implement bugs and we do not implement apparent
 *	behavior from other implementations but documented behavior for the
 *	programs /usr/bin/od and /usr/xpg4/bin/od on Solaris and we of course
 *	follow the POSIX standard.
 *
 *	As we use getexecname() to distinct between the POSIX interface and the
 *	traditional Solaris interface, /usr/bin/od and /usr/xpg4/bin/od must be
 *	hard linked. Symlinks would be followed by getexecname().
 *
 *	Copyright (c) 1986-2015 J. Schilling
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
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>	/* O_BINARY */
#include <schily/types.h>	/* To make off_t available */
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/getargs.h>
#include <schily/align.h>
#include <schily/nlsdefs.h>
#include <schily/io.h>		/* for setmode() prototype */
#include <schily/limits.h>	/* for  MB_LEN_MAX	*/
#include <schily/ctype.h>	/* For isprint()	*/
#include <schily/wchar.h>	/* wchar_t		*/
#include <schily/wctype.h>	/* For iswprint()	*/

/*
 * K&R cpp does not permit us to use spaces in the K&R concat method below,
 * so we are forced to use the special CSTYLED comment in order to keep the
 * indentation lint program "cstyle" quiet.
 */
#if	defined(PROTOTYPES)
#define	CONCAT(a, b)	a##b
#else
			/* CSTYLED */
#define	CONCAT(a, b)	a/**/b
#endif

#define	octdig(x)	(x >= '0' && x <= '7')
#ifndef	TRUE
#define	TRUE		1
#define	FALSE		0
#endif

/*
 * The current display state
 */
typedef struct dstate {
	FILE	*f;		/* The current input file		*/
	int	imcnt;		/* Count from intermediate buffer	*/
	int	rest;		/* Unprinted octets + 1			*/
	BOOL	printable;	/* Partial character was printable	*/
	int	blocksize;	/* The display buffer size		*/
	const char *inname;	/* The current input file name		*/
	int	argc;		/* argc for the rest of the files	*/
	char	*const *argv;	/* argv[0] points to current file	*/
	int	excode;		/* The exit code for delayed errors	*/
} dst_t;

typedef	int	(*pfun)	__PR((const char *fmt, void *valp));
typedef	int	(*pbfun)__PR((int cnt, char *buf, dst_t *dstp));

typedef struct pr pr_t;
struct pr {
	size_t	pr_size;	/* Object size for one element		*/
	int	pr_fieldw;	/* Output-fieldwidth for one element	*/
	char	*pr_fmt;	/* Printf() format for pr_out		*/
	pfun	pr_out;		/* Element output function		*/
	pbfun	pr_block;	/* Line-block output function		*/
	int	pr_fill;	/* Number of ' ' fill characters	*/
	pr_t	*pr_next;
};

/*
 * The K&R CONCAT does not permit us to put a space after the after the comma
 * in a CONCAT() call, so we are forced to use the special CSTYLED comment in
 * order to keep the indentation lint program "cstyle" quiet.
 */
#define	DEF_PR(name, width, type, format)				\
			/* CSTYLED */					\
		LOCAL int CONCAT(pr_,name) __PR((const char *fmt, void *vp));\
		LOCAL int						\
			/* CSTYLED */					\
		CONCAT(pr_,name) (fmt, vp)				\
			const char *fmt;				\
			void	*vp;					\
		{							\
			return (printf(fmt, *(type *)vp));		\
		}							\
				/* CSTYLED */				\
		LOCAL	pr_t	CONCAT(d_,name) = { sizeof (type),	\
					width,				\
					format,				\
					/* CSTYLED */			\
					CONCAT(pr_,name),		\
					0,				\
					0, 0 }


LOCAL BOOL	is_od = FALSE;		/* This is "od" and not "hdump"	   */
LOCAL BOOL	is_xpg4 = FALSE;	/* Called as /usr/xpg4/bin/od	   */
LOCAL BOOL	is_posix = FALSE;	/* Used POSIX option (-A/-j/-N/-t) */
LOCAL BOOL	is_pipe;		/* Whether stdout is a pipe	   */
LOCAL int	curradix;		/* Radix found by myatoll()	   */
LOCAL int	maxline = 0;		/* Max with in output formats	   */
LOCAL char	*addrfmt;		/* Address printf() format	   */
LOCAL char	*samefmt;		/* printf() format for same data   */
LOCAL off_t	pos = (off_t)0;		/* Position used for address label */

LOCAL BOOL	dflag = FALSE;	/* -d od -tu2 / hdump -d (decimal)	   */
LOCAL BOOL	oflag = FALSE;	/* -o hdump: switch to octal		   */
LOCAL BOOL	lflag = FALSE;	/* -l hdump: switch to long		   */
LOCAL BOOL	uflag = FALSE;	/* -u hdump: switch to unsigned		   */
LOCAL BOOL	tflag = FALSE;	/* -t was seen, POSIX interface requested  */
LOCAL BOOL	lenflag = FALSE; /* A bytecount limiting argument was seen */
LOCAL BOOL	vflag = FALSE;	/* -v show all input data		   */

LOCAL	void	usage	__PR((int exitcode));
LOCAL	const char *filename __PR((const char *name));
EXPORT	int	main	__PR((int ac, char **av));
LOCAL	void	checkfill __PR((dst_t *dstp));
LOCAL	int	gettype	__PR((const char *arg, void *valp));
LOCAL	void	add_dout __PR((pr_t *this));
LOCAL	int	add_out	__PR((const char *arg, long *valp));
LOCAL	int	add_fmt	__PR((const char *arg, long *valp));
LOCAL	int	opt_b	__PR((const char *arg, long *valp));
LOCAL	int	opt_c	__PR((const char *arg, long *valp));
LOCAL	int	opt_d	__PR((const char *arg, long *valp));
LOCAL	int	opt_l	__PR((const char *arg, long *valp));
LOCAL	int	opt_o	__PR((const char *arg, long *valp));
LOCAL	int	opt_u	__PR((const char *arg, long *valp));
LOCAL	void	dump	__PR((off_t len, dst_t *dstp));
LOCAL	void	prbuf	__PR((int cnt, char *obuf, dst_t *dstp));
LOCAL	int	prasc	__PR((int cnt, char *buf, dst_t *dstp));
LOCAL	int	prch	__PR((int cnt, char *buf, dst_t *dstp));
LOCAL	int	prmbch	__PR((int cnt, char *buf, dst_t *dstp));
LOCAL	int	prcbytes __PR((int cnt, char *buf, dst_t *dstp));
LOCAL	int	prbbytes __PR((int cnt, char *buf, dst_t *dstp));
LOCAL	int	prascii	__PR((int cnt, char *buf, dst_t *dstp));
LOCAL	int	prlong	__PR((int cnt, char *buf, dst_t *dstp));
LOCAL	int	prshort	__PR((int cnt, char *buf, dst_t *dstp));
LOCAL	BOOL	bufeql	__PR((long *b1, long *b2, int cnt));
LOCAL	Llong	myatoll	__PR((char *s));
LOCAL	int	read_input __PR((dst_t *dstp, char *bp, int cnt));
LOCAL	BOOL	open_next __PR((dst_t *dstp));
LOCAL	off_t	advance	__PR((dst_t *dstp, off_t xpos));
LOCAL	off_t	doadvance __PR((FILE *f, off_t xpos, BOOL is_last));
LOCAL	off_t	doskip	__PR((FILE *f, off_t xpos));
LOCAL	BOOL	out_ispipe __PR((void));
LOCAL	int	ptype	__PR((const char *arg));
LOCAL	int	illsize	__PR((int size));
LOCAL	int	illfsize __PR((int size));
LOCAL	int	illtype	__PR((int type));
LOCAL	void	setaddrfmt __PR((int Aflag, int lradix));

/*
 * These output descriptors are object oriented.
 * They are called once per element.
 */

/*
 * Decimal formats
 */
DEF_PR(c_d,  4, char,  " %3hhd");
DEF_PR(s_d,  7, short, " %6.5hd");
DEF_PR(i_d, 12, int,   " %11.10d");
#if	SIZEOF_LONG_INT > SIZEOF_INT
DEF_PR(l_d, 21, long,  " %20.18ld");
#else
#define	d_l_d	d_i_d
#endif
#if	SIZEOF_LONG_LONG > SIZEOF_LONG_INT
DEF_PR(ll_d, 21, Llong, " %20.18lld");
#endif

/*
 * Octal formats
 */
DEF_PR(c_o,  4, char,  " %3.3hho");
DEF_PR(s_o,  7, short, " %6.6ho");
DEF_PR(i_o, 12, int,   " %11.11o");
#if	SIZEOF_LONG_INT > SIZEOF_INT
DEF_PR(l_o, 23, long,  " %22.22lo");
#else
#define	d_l_o	d_i_o
#endif
#if	SIZEOF_LONG_LONG > SIZEOF_LONG_INT
DEF_PR(ll_o, 23, Llong, " %22.22llo");
#endif

/*
 * Unsigned decimal formats
 */
DEF_PR(c_u,  4, char,  " %3.3hhu");
DEF_PR(s_u,  6, short, " %5.5hu");
DEF_PR(i_u, 11, int,   " %10.10u");
#if	SIZEOF_LONG_INT > SIZEOF_INT
DEF_PR(l_u, 21, long,   " %20.20lu");
#else
#define	d_l_u	d_i_u
#endif
#if	SIZEOF_LONG_LONG > SIZEOF_LONG_INT
DEF_PR(ll_u, 21, Llong, " %20.20llu");
#endif

/*
 * Hexadecimal formats
 */
DEF_PR(c_x,  3, char,  " %2.2hhx");
DEF_PR(s_x,  5, short, " %4.4hx");
DEF_PR(i_x,  9, int,   " %8.8x");
#if	SIZEOF_LONG_INT > SIZEOF_INT
DEF_PR(l_x, 17, long,  " %16.16lx");
#else
#define	d_l_x	d_i_x
#endif
#if	SIZEOF_LONG_LONG > SIZEOF_LONG_INT
DEF_PR(ll_x, 17, Llong, " %16.16llx");
#endif

/*
 * Floatingpoint formats
 */
#ifndef	NO_FLOATINGPOINT
#define	FLOAT_AS_ON_SUNOS
#ifdef	FLOAT_AS_ON_SUNOS
DEF_PR(f_f, 15, float, " %14.7e");
#else
DEF_PR(f_f, 14, float, " %13.6e");
#endif
DEF_PR(d_f, 23, double, " %22.14e");
#ifdef	HAVE_LONGDOUBLE
DEF_PR(ld_f, 24, long double, " %23.14Le");
#endif
#endif

/*
 * These output descriptors are block oriented.
 * They are called once per line.
 */
LOCAL	pr_t	d_asc = { sizeof (char), 4, "", 0, prasc };	/* od -ta   */
LOCAL	pr_t	d_ch  = { sizeof (char), 4, "", 0, prch };	/* od -c    */
LOCAL	pr_t	d_mbch = { sizeof (char), 4, "", 0, prmbch };	/* od -C/-tc */

LOCAL	pr_t	d_ascii = { sizeof (char), 3, "", 0, prascii };	/* hd -a    */
LOCAL	pr_t	d_bbytes = { sizeof (char), 4, "", 0, prbbytes }; /* hd -b  */
LOCAL	pr_t	d_cbytes = { sizeof (char), 4, "", 0, prcbytes }; /* hd -c  */
LOCAL	pr_t	d_short = { sizeof (short), 4, "", 0, prshort }; /* hd	    */
LOCAL	pr_t	d_long = { sizeof (long), 4, "", 0, prlong };	/* hd -l    */

LOCAL	pr_t	*pr_root;
LOCAL	pr_t	**pr_tail = &pr_root;


LOCAL void
usage(exitcode)
	int	exitcode;
{
	error(_(
	"Usage:	%s [options] [file] [[+]starting address[.][b|B]%s]\n"),
	is_od?"od":"hdump", is_od?"":_(" [count]]"));
	error(_(
"Usage:	%s [options] [-t type]... [-A base] [-j skip] [-N count] [file...]\n"),
	is_od?"od":"hdump");
	error(_("Options:\n"));
	error(_("\t-A c\tSet address base c ('d', 'o', 'n' or 'x')\n"));
	error(_("\t-j skip\tSkip input for the files\n"));
	error(_("\t-N n\tOnly process n bytes\n"));
	error(_("\t-t type\tSpecify output format type\n"));
	error(_("\t-a\tDisplay content also in characters\n"));
	error(_("\t-b\tDisplay content in bytes\n"));
	error(_("\t-c\tDisplay content as %s quoted characters\n"),
			is_xpg4 ? _("single or multi byte") : _("single byte"));
	error(_("\t-C\tDisplay content as %s quoted characters\n"),
				_("single or multi byte"));
	error(_("\t-d\tDisplay content in decimal%s\n"),
			is_od ? " -tu2" : "");
	error(_("\t-D\tDisplay content in decimal -tu4\n"));
#ifndef	NO_FLOATINGPOINT
	error(_("\t-f\tDisplay content as floats\n"));
	error(_("\t-F\tDisplay content as doubles\n"));
#endif
	if (!is_od)
	error(_("\t-l\tDisplay content as longs\n"));
	error(_("\t-o\tDisplay content in octal%s\n"),
			is_od ? " -to2" : "");
	error(_("\t-O\tDisplay content in octal -to4\n"));
	error(_("\t-s\tDisplay content in decimal -td2\n"));
	error(_("\t-S\tDisplay content in decimal -td4\n"));
	if (!is_od)
	error(_("\t-u\tDisplay content as unsigned\n"));
	error(_("\t-v\tShow all data even if it is identical\n"));
	error(_("\t-x\tDisplay content in hexadecimal -tx2\n"));
	error(_("\t-X\tDisplay content in hexadecimal -tx4\n"));
	error(_("\t-help\tPrint this help.\n"));
	error(_("\t-version\tPrint version number.\n"));
	error(_("'b' after starting address multiplies with 512\n"));
	exit(exitcode);
}

#include <schily/string.h>
LOCAL const char *
filename(name)
	const char	*name;
{
	char	*p;

	if ((p = strrchr(name, '/')) == NULL)
		return (name);
	return (++p);
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	off_t	len = (off_t)0;
	char	*options =
"A?,j&,N&,t&,a~,b~,c~,C~,d~,D~,f~,F~,l~,o~,O~,s~,S~,u~,v,x~,X~,help,version";
	BOOL	help = FALSE;
	BOOL	prversion = FALSE;
	BOOL	didoffset = FALSE;
	char	Aflag = '\0';
	Llong	skip = 0;
	Llong	nbytes = 0;
	int	lradix = 16;
	int	cac;
	char	* const * cav;
#if	defined(USE_NLS)
	char	*dir;
#endif
	dst_t	dst;
	struct ga_props ga_props;

	save_args(ac, av);

	(void) setlocale(LC_ALL, "");

#if	defined(USE_NLS)
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "hdump"	/* Use this only if it weren't */
#endif
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#ifdef	PROTOTYPES
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
#endif

	if (streql(filename(av[0]), "od")) {		/* "od" interface? */
		is_od = TRUE;
		lradix = 8;
#ifdef	HAVE_GETEXECNAME
		if (strstr(getexecname(), "/xpg4")) {	/* X-Open interface? */
			is_xpg4 = TRUE;
		}
#endif
	}

	cac = --ac;
	cav = ++av;

	getarginit(&ga_props, GAF_NO_PLUS);	/* POSIX doesn't permit +opt */
	if (getlargs(&cac, &cav, &ga_props, options,
			&Aflag,
			getllnum, &skip,	/* -j skip	*/
			getllnum, &nbytes,	/* -N bytes	*/
			gettype, NULL,		/* -t type	*/
			add_out, &d_ascii,	/* -a		*/
			opt_b, NULL,		/* -b		*/
			opt_c, NULL,		/* -c		*/
			add_out, &d_mbch,	/* -C		*/
			opt_d, NULL,		/* -d		*/
			add_fmt, "u4",		/* -D		*/
			add_fmt, "f4",		/* -f		*/
			add_fmt, "f8",		/* -F		*/
			opt_l, NULL,		/* -l (hdump)	*/
			opt_o, NULL,		/* -o		*/
			add_fmt, "o4",		/* -O		*/
			add_fmt, "d2",		/* -s		*/
			add_fmt, "d4",		/* -S		*/
			opt_u, NULL,		/* -u (hdump)	*/
			&vflag,			/* -v		*/
			add_fmt, "x2",		/* -x		*/
			add_fmt, "x4",		/* -X		*/
						&help, &prversion) < 0) {
		error(_("Bad flag: '%s'.\n"), cav[0]);
		usage(1);
	}
	if (help)
		usage(0);
	if (prversion) {
		printf(
		_("%s release %s (%s-%s-%s) Copyright (C) 1986-2015 %s\n"),
				is_od ? "Od":"Hdump",
				"1.36",
				HOST_CPU, HOST_VENDOR, HOST_OS,
				_("Joerg Schilling"));
		exit(0);
	}
	is_pipe = out_ispipe();

	if (skip < 0)
		comerrno(EX_BAD, _("Invalid offset %lld.\n"), skip);
	if (skip > 0) {
		is_posix = TRUE;
		pos = skip;
		if (pos != skip) {
			comerrno(EX_BAD,
			_("Offset %lld is too large for type 'off_t'.\n"),
				skip);
		}
	}
	if (nbytes < 0)
		comerrno(EX_BAD, _("Invalid number of bytes.\n"));
	if (nbytes > 0) {
		is_posix = TRUE;
		lenflag = TRUE;
		len = nbytes;
		if (len != nbytes) {
			comerrno(EX_BAD,
		_("Number of bytes %lld is too large for type 'off_t'.\n"),
				nbytes);
		}
	}
	if (Aflag || tflag)
		is_posix = TRUE;

	cac = ac;
	cav = av;
	dst.excode = 0;
	dst.inname = NULL;
	dst.f = (FILE *)NULL;
	if (getlfiles(&cac, &cav, &ga_props, options) <= 0) {	/* Skip opts */
		dst.argc = -2;
		dst.argv = cav;
	} else {
		if (cac == 1 && cav[0][0] == '+')	/* If called od +off */
			dst.argc = -2;			/* mark to use stdin */
		else
			dst.argc = cac;
		dst.argv = cav;
	}
	open_next(&dst);				/* Open first file */
	if (dst.f == (FILE *)NULL)
		return (dst.excode);

	/*
	 * Permit to call "od +offset" (use old skip syntax and dump stdin).
	 */
	if (!(cac == 1 && cav[0][0] == '+')) {
		cac--, cav++;
	}
	if (!is_posix && ((is_od && cac == 1) || (!is_od && cac > 0))) {
		char	*arg = cav[0];

		if ((is_xpg4 &&		/* This is for /usr/xpg4/bin/od */
		    strchr("+0123456789", *arg)) ||
		    (!is_xpg4 &&	/* This is for /usr/bin/od and hdump */
		    (strchr("+0123456789", *arg) ||
		    (arg[0] == 'x' && arg[1] == '\0') ||
		    (arg[0] == 'x' && strchr("0123456789abcdef", arg[1])) ||
		    (arg[0] == '.' && arg[1] == '\0')))) {
			if (*arg == '+')
				arg++;
			pos = (off_t)myatoll(arg);
			if (!is_od)
				pos &= ~((off_t)1);
			lradix = curradix;
			didoffset = TRUE;
			cac--, cav++;
			dst.argc--;
		}
	}
	if (didoffset && cac > 0) {
		len = (off_t)myatoll(cav[0]);
		lenflag = TRUE;
		cac--; cav++;
		dst.argc--;
	}

	if (didoffset && cac > 0) {
		errmsgno(EX_BAD, _("Unexpected argument '%s'.\n"), cav[0]);
		usage(1);
	}

	setaddrfmt(Aflag, lradix);	/* Set format for address labels */

	if (pos > 0) {
		off_t	newpos = advance(&dst, pos);

		if (newpos > 0)
			comerrno(-2, _("Cannot skip past EOF.\n"));
		/*
		 * In case of a POSIX -j offset spec, start with address
		 * label 0, otherwise use the address label that matches pos.
		 */
		if (skip > 0)
			pos = 0;
	}
	if (pr_root == NULL) {				/* No pr format yet? */
		if (is_od) {
			add_dout(&d_s_o);		/* od default: -toS  */
		} else {
			if (lflag)
				add_dout(&d_long);
			else
				add_dout(&d_short);
		}
	}
	checkfill(&dst);
	dump(len, &dst);
	return (dst.excode);
}

/*
 * Check how many fill characters are needed for each format in order to
 * align the output according to the POSIX standard.
 */
LOCAL void
checkfill(dstp)
	dst_t	*dstp;
{
	pr_t	*dp;
	int	kgv = 1;
	int	n;

	for (dp = pr_root; dp != NULL; dp = dp->pr_next) {
		if (kgv % dp->pr_size) {
			n = kgv;
			while (n % dp->pr_size)
				n += kgv;
			kgv = n;
		}
	}
	while (kgv < 12)
		kgv *= 2;
	dstp->blocksize = kgv;

	for (dp = pr_root, n = 0; dp != NULL; dp = dp->pr_next) {
		int	lmax;
		int	nmax;

		lmax = dstp->blocksize / dp->pr_size * dp->pr_fieldw;
		if (lmax > maxline)
			maxline = lmax;
		nmax = dstp->blocksize / dp->pr_size;
		if (nmax > n)
			n = nmax;
	}
	/*
	 * Round up to the next value that is dividable by the highest
	 * number of objects per line.
	 */
	while (maxline > (maxline / n * n))
		maxline++;
	for (dp = pr_root; dp != NULL; dp = dp->pr_next) {
		dp->pr_fill =
			maxline/(dstp->blocksize/dp->pr_size)-dp->pr_fieldw;
	}
}

/*
 * Get the type for the -t type option.
 * Mark that we did call the type parser along with -t.
 */
/* ARGSUSED */
LOCAL int
gettype(arg, valp)
	const	char	*arg;
		void	*valp;
{
	tflag = TRUE;
	return (ptype(arg));
}

/*
 * Append an output definition to our current list.
 */
LOCAL void
add_dout(this)
	pr_t	*this;
{
	pr_t	*new = malloc(sizeof (*new));

	if (new == NULL)
		comerr(_("No memory.\n"));

	*new = *this;
	new->pr_next = (pr_t *)0;
	*pr_tail = new;
	pr_tail = &new->pr_next;
}

/*
 * Append an output definition to our current list.
 * This variant is called as getargs() callback.
 */
/* ARGSUSED */
LOCAL int
add_out(arg, valp)
	const	char	*arg;
		long	*valp;
{
	add_dout((pr_t *)valp);
	return (1);
}

/*
 * Append an output definition from type string.
 * This variant is called as getargs() callback.
 */
/* ARGSUSED */
LOCAL int
add_fmt(arg, valp)
	const	char	*arg;
		long	*valp;
{
	return (ptype((char *)valp));
}

/*
 * getargs() callback to implement -b for bin/hdump, bin/od and xpg4/bin/od.
 */
/* ARGSUSED */
LOCAL int
opt_b(arg, valp)
	const	char	*arg;
		long	*valp;
{
	if (is_od || tflag)
		return (ptype("o1"));
	add_dout(&d_bbytes);
	return (1);
}

/*
 * getargs() callback to implement -c for bin/hdump, bin/od and xpg4/bin/od.
 */
/* ARGSUSED */
LOCAL int
opt_c(arg, valp)
	const	char	*arg;
		long	*valp;
{
	if (is_od || tflag) {
		if (is_xpg4)
			add_dout(&d_mbch);
		else
			add_dout(&d_ch);
		return (1);
	}
	add_dout(&d_cbytes);
	return (1);
}

/*
 * getargs() callback to implement -d for bin/hdump, bin/od and xpg4/bin/od.
 */
/* ARGSUSED */
LOCAL int
opt_d(arg, valp)
	const	char	*arg;
		long	*valp;
{
	if (is_od || tflag)
		return (ptype("u2"));
	dflag = TRUE;
	return (1);
}

/*
 * getargs() callback to implement -l for bin/hdump, bin/od and xpg4/bin/od.
 */
/* ARGSUSED */
LOCAL int
opt_l(arg, valp)
	const	char	*arg;
		long	*valp;
{
	if (is_od)
		return (-1);	/* -l is illegal for od(1) */
	lflag = TRUE;
	return (1);
}

/*
 * getargs() callback to implement -o for bin/hdump, bin/od and xpg4/bin/od.
 */
/* ARGSUSED */
LOCAL int
opt_o(arg, valp)
	const	char	*arg;
		long	*valp;
{
	if (is_od || tflag)
		return (ptype("o2"));
	oflag = TRUE;
	return (1);
}

/*
 * getargs() callback to implement -u for bin/hdump, bin/od and xpg4/bin/od.
 */
/* ARGSUSED */
LOCAL int
opt_u(arg, valp)
	const	char	*arg;
		long	*valp;
{
	if (is_od)
		return (-1);	/* -u is illegal for od(1) */
	uflag = TRUE;
	return (1);
}

LOCAL void
dump(len, dstp)
	register off_t	len;
		dst_t	*dstp;
{
		char	obuf[4 * 24];	/* 1x align 2x main buffer 1x ahead */
	register char	*buf;
	register char	*oldbuf;
	register char	*temp;
	register int	cnt;

	/*
	 * Align for the worst case (This is a long double in LP64 mode)
	 */
	buf	= xalign((obuf), 16, 15);
	oldbuf	= buf + dstp->blocksize;

	dstp->imcnt = 0;
	dstp->rest = 0;
	dstp->printable = FALSE;

	do {
		if (lenflag) {
			if (len <= 0)
				break;
			cnt = len > dstp->blocksize ? dstp->blocksize:(int)len;
		} else {
			cnt = dstp->blocksize;
		}

		if (dstp->imcnt > 0) {
			movebytes(oldbuf+dstp->blocksize, buf, dstp->imcnt);
			cnt = cnt < dstp->imcnt ? cnt:dstp->imcnt;
			dstp->imcnt = 0;
		} else {
			if ((cnt = read_input(dstp, buf, cnt)) == 0)
				break;
		}
		if (vflag ||
		    !bufeql((long *)buf, (long *)oldbuf, dstp->blocksize)) {
			if (cnt < dstp->blocksize)
				fillbytes(buf+cnt, dstp->blocksize-cnt, '\0');
			prbuf(cnt, buf, dstp);
		}
		pos += cnt;
		len -= cnt;

		temp = oldbuf;
		oldbuf = buf;
		buf = temp;
	} while (!feof(dstp->f));
	printf(addrfmt, pos);
	printf("\n");
}

LOCAL void
prbuf(cnt, obuf, dstp)
		int	cnt;
	register char	*obuf;
		dst_t	*dstp;
{
	register	pr_t	*dp;

	/*
	 * cnt > 0 is granted by dump()
	 */
	printf(addrfmt, pos);

	for (dp = pr_root; dp; dp = dp->pr_next) {
		if (dp->pr_block) {
			(*dp->pr_block)(cnt, obuf, dstp);
		} else {
			register	int	i;
			register	char	*cp = obuf;

			for (i = 0; i < cnt; ) {
				register int k = dp->pr_fill;

				while (--k >= 0)
					(void) putchar(' ');

				(*dp->pr_out)(dp->pr_fmt, cp);
				i += dp->pr_size;
				cp += dp->pr_size;
			}
		}
		if (dp->pr_next)
			printf("\n       ");
		else
			printf("\n");
	}
}

/*
 * POSIX names for -ta (named characters).
 */
LOCAL char *ascii_names[] = {
	"nul", "soh", "stx", "etx", "eot", "enq", "ack", "bel",
	" bs", " ht", " lf", " vt", " ff", " cr", " so", " si",
	"dle", "dc1", "dc2", "dc3", "dc4", "nak", "syn", "etb",
	"can", " em", "sub", "esc", " fs", " gs", " rs", " us",
	" sp", "  !", "  \"", "  #", "  $", "  %", "  &", "  '",
	"  (", "  )", "  *", "  +", "  ,", "  -", "  .", "  /",
	"  0", "  1", "  2", "  3", "  4", "  5", "  6", "  7",
	"  8", "  9", "  :", "  ;", "  <", "  =", "  >", "  ?",
	"  @", "  A", "  B", "  C", "  D", "  E", "  F", "  G",
	"  H", "  I", "  J", "  K", "  L", "  M", "  N", "  O",
	"  P", "  Q", "  R", "  S", "  T", "  U", "  V", "  W",
	"  X", "  Y", "  Z", "  [", "  \\", "  ]", "  ^", "  _",
	"  `", "  a", "  b", "  c", "  d", "  e", "  f", "  g",
	"  h", "  i", "  j", "  k", "  l", "  m", "  n", "  o",
	"  p", "  q", "  r", "  s", "  t", "  u", "  v", "  w",
	"  x", "  y", "  z", "  {", "  |", "  }", "  ~", "del"
};

/*
 * Print named 7-bit ASCII characters
 */
/* ARGSUSED */
LOCAL int
prasc(cnt, buf, dstp)
	register int	cnt;
	register char	*buf;
		dst_t	*dstp;
{
	register short	i;

	for (i = 0; i < cnt; i++) {
		(void) putchar(' ');
		(void) fputs(ascii_names[buf[i]&0177], stdout);
	}
	return (0);
}

/*
 * Print single byte characters, quote non-printable chars.
 */
/* ARGSUSED */
LOCAL int
prch(cnt, buf, dstp)
	register int	cnt;
	register char	*buf;
		dst_t	*dstp;
{
	register short	i;
	register Uchar	c;

	for (i = 0; i < cnt; i++) {
		c = buf[i];
		if (isprint(c))
			printf(" %3c", c);
		else switch (c) {

		case '\0':	printf("  \\0"); break;
		case '\a':	printf("  \\a"); break;
		case '\b':	printf("  \\b"); break;
		case '\f':	printf("  \\f"); break;
		case '\n':	printf("  \\n"); break;
		case '\r':	printf("  \\r"); break;
		case '\t':	printf("  \\t"); break;
		case '\v':	printf("  \\v"); break;
		default:
				printf(" %3.3hho", c);
		}
	}
	return (0);
}

/*
 * Print single byte or multi-byte characters, quote non-printable chars.
 * The character type is selected from LC_CTYPE.
 */
LOCAL int
prmbch(cnt, buf, dstp)
	register int	cnt;
	register char	*buf;
		dst_t	*dstp;
{
	register short	i = 0;
	register int	n;
		wchar_t	wc = 0;

	n = dstp->rest;
	dstp->rest = 0;

	while (--n >= 0 && i < cnt) {
		if (dstp->printable)
			printf("  **");
		else
			printf(" %3.3hho", buf[i]);
		i++;
	}

	for (; i < cnt; i++) {
again:
		n = mbtowc(&wc, &buf[i], cnt - i + dstp->imcnt);
		if (n < 0) {
			(void) mbtowc(NULL, NULL, 0);
			/*
			 * Be careful here: Solaris is buggy and sets
			 * MB_CUR_MAX to 3 for UTF-8 although mbtowc() may
			 * return larger numbers.
			 */
			if (dstp->imcnt == 0 && (cnt - i) < MB_LEN_MAX) {
				dstp->imcnt = read_input(dstp,
							buf+dstp->blocksize,
							dstp->blocksize);
				if (dstp->imcnt > 0)
					goto again; /* Don't increment i */
			}
		}
		if (n > 0 && iswprint(wc)) {
			printf("%*c%.*s", 4-wcwidth(wc), ' ', n, &buf[i]);
			while (--n > 0 && ++i < cnt)
				printf("  **");
			dstp->printable = TRUE;
		} else {
			dstp->printable = FALSE;
			n--;
			switch (wc) {

			case '\0':	printf("  \\0"); break;
			case '\a':	printf("  \\a"); break;
			case '\b':	printf("  \\b"); break;
			case '\f':	printf("  \\f"); break;
			case '\n':	printf("  \\n"); break;
			case '\r':	printf("  \\r"); break;
			case '\t':	printf("  \\t"); break;
			case '\v':	printf("  \\v"); break;
			default:
				do {
					printf(" %3.3hho", buf[i]);
					if (++i >= cnt)
						break;
				} while (--n >= 0);
				i--;
			}
		}
	}
	dstp->rest = n;
	return (0);
}

/*
 * Implement hdump -c output.
 */
/* ARGSUSED */
LOCAL int
prcbytes(cnt, buf, dstp)
	register int	cnt;
	register char	*buf;
		dst_t	*dstp;
{
	register short i;

	for (i = 0; i < cnt; i++) {
		if (buf[i] < ' ' || buf[i] >= '\177')
			printf("  \\%02X", 0377&buf[i]);
		else
			printf("    %c", buf[i]);
		if (i == 7 && cnt > 8)
			printf("\n       ");
	}
	return (0);
}

/*
 * Implement hdump -b output.
 */
/* ARGSUSED */
LOCAL int
prbbytes(cnt, buf, dstp)
	register int	cnt;
	register char	*buf;
		dst_t	*dstp;
{
	register short i;

	for (i = 0; i < cnt; i++) {
		if (dflag) {
			if (uflag)
				printf("  %4d", 0377&buf[i]);
			else
				printf("  %4d", buf[i]);
		} else if (oflag) {
			printf(" %04o", 0377&buf[i]);
		} else {
			printf("   %02X", 0377&buf[i]);
		}
		if (i == 7 && cnt > 8)
			printf("\n       ");
	}
	return (0);
}

/*
 * Implement hdump -a output.
 */
/* ARGSUSED */
LOCAL int
prascii(cnt, buf, dstp)
	register int	cnt;
	register char	*buf;
		dst_t	*dstp;
{
	register short	i;
	register short	n = dstp->blocksize;
	register char	c;

	for (i = 0; i < n; i++) {
		if (i >= cnt)
			printf("   ");
		else if (dflag) {
			if (uflag)
				printf("%4u", 0377&buf[i]);
			else
				printf("%4d", buf[i]);
		} else if (oflag) {
			printf(" %03o", 0377&buf[i]);
		} else {
			printf(" %02X", 0377&buf[i]);
		}
		if (i == 7)
			printf("  ");
	}
	if (dflag || oflag)
		printf("\n         ");
	else
		printf("   ");
	for (i = 0; i < cnt; i++) {
		c = buf[i];
		(void) putchar(c < ' ' || c >= 0177 ? '.' : c);
	}
	return (0);
}

/*
 * Implement hdump -l output.
 */
/* ARGSUSED */
LOCAL int
prlong(cnt, buf, dstp)
	int	cnt;
	char	*buf;
	dst_t	*dstp;
{
			/* LINTED */
	register long	*obuf = (long *)buf;
	register short	i;
	register int	n = cnt;

	n /= sizeof (long);
	for (i = 0; i < n; i++) {
last:
		if (dflag) {
			if (uflag)
				printf("%12lu", obuf[i]);
			else
				printf("%12ld", obuf[i]);
		} else if (oflag) {
			printf(" %012lo", obuf[i]);
		} else {
			printf("   %08lX", obuf[i]);
		}
	}
	if ((i = (cnt % sizeof (long))) != 0) {
		fillbytes(&((char *)obuf)[cnt], sizeof (long)-i, '\0');
		cnt -= i;
		i = cnt / sizeof (long);
		goto last;
	}
	return (0);
}

/*
 * Implement hdump default size output.
 */
/* ARGSUSED */
LOCAL int
prshort(cnt, buf, dstp)
	int	cnt;
	char	*buf;
	dst_t	*dstp;
{
			/* LINTED */
	register short	*obuf = (short *)buf;
	register short	i;
	register int	n = cnt;

	n /= sizeof (short);
	for (i = 0; i < n; i++) {
last:
		if (dflag) {
			if (uflag)
				printf("%7hu", obuf[i]);
			else
				printf("%7hd", obuf[i]);
		} else if (oflag) {
			printf(" %06ho", obuf[i]);
		} else {
			printf("   %04hX", obuf[i]);
		}
	}
	if ((i = (cnt % sizeof (short))) != 0) {
		fillbytes(&((char *)obuf)[cnt], sizeof (short)-i, '\0');
		cnt -= i;
		i = cnt / sizeof (short);
		goto last;
	}
	return (0);
}

LOCAL BOOL
bufeql(b1, b2, cnt)
	register long	*b1;
	register long	*b2;
		int	cnt;
{
	register int	i;
	static	int	dont_print = -1;

	if (dont_print < 0)
		return (dont_print = FALSE);
	for (i = cnt / sizeof (long); --i >= 0; )
		if (*b1++ != *b2++)
			return (dont_print = FALSE);
	if (!dont_print) {
		printf("%s", samefmt);
		if (is_pipe)	/* Make it immediately visible in $PAGER */
			(void) fflush(stdout);
	}
	return (dont_print = TRUE);
}

LOCAL Llong
myatoll(s)
	char	*s;
{
	char	*p;
	Llong	val = 0;

	if (s[0] == 'x' && s[1] == '\0') {
		curradix = 16;
		return (val);
	} else if (s[0] == 'x' &&
		    strchr("0123456789abcdefABCDEF", s[1])) {
		curradix = 16;
		s++;
	} else if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
		curradix = 16;
	} else if (s[0] == '0' && !streql("0", s)) {
		curradix = 8;
	} else if (((p = strchr(s, '.')) != NULL) &&
		    (p[1] == '\0' ||
		    ((p[1] == 'b' || p[1] == 'B') && p[2] == '\0'))) {
		curradix = 10;
	} else {
		curradix = 8;
	}

	p = astollb(s, &val, curradix);

	if (*p != '\0') {
		if (*p == '.') {
			p++;
			curradix = 10;
		}
		if (*p && (streql(p, "b") || streql(p, "B"))) {
			if (*p == 'b') val *= 512;
			if (*p == 'B') val *= 512;
		} else if (*p)
			comerrno(EX_BAD, _("Bad numeric argument '%s'.\n"), s);
	}

	return (val);
}

LOCAL int
read_input(dstp, bp, cnt)
	register dst_t	*dstp;
	register char	*bp;
	register int	cnt;
{
	register int	amt;
	register int	total = 0;

	while (cnt > 0) {
		amt = fileread(dstp->f, bp, cnt);
		if (amt <= 0) {
			if (amt < 0) {
				errmsg(_("Error reading '%s'.\n"),
					dstp->inname);
			}
			if (!open_next(dstp))
				return (total);
			continue;
		}
		if (feof(dstp->f) && !open_next(dstp))
			return (total+amt);

		total += amt;
		cnt   -= amt;
		bp    += amt;
	};
	return (total);
}

LOCAL BOOL
open_next(dstp)
	dst_t	*dstp;
{
	FILE	*f;
	char	*inname;
	BOOL	stdinflag;

	do {
		stdinflag = FALSE;
		if (dstp->argc == -2) {		/* Once only decrement later */
			inname = "/dev/stdin";
			dstp->inname = "stdin";
			stdinflag = TRUE;
		} else if (dstp->argc <= 0) {
			return (FALSE);
		} else {
			dstp->inname = inname = dstp->argv[0];
			dstp->argv++;
		}
		/*
		 * POSIX.1-2008 optionally permits the notation "-" for stdin
		 * but /usr/xpg4/bin/od on Solaris does not. We currently
		 * follow /usr/xpg4/bin/od.
		 */
		if (!is_xpg4 && inname[0] == '-' && inname[1] == '\0') {
			inname = "/dev/stdin";
			dstp->inname = "stdin";
			stdinflag = TRUE;
		}
#ifndef	HAVE__DEV_STDIN
		if (stdinflag) {
			if (dstp->f != (FILE *)NULL) {
				fclose(dstp->f);
				dstp->f = (FILE *)NULL;
			}
			f = fileluopen(dup(STDIN_FILENO), "rb");
		} else
#endif
		if (dstp->f == (FILE *)NULL) {
			f = fileopen(inname, "rb");
		} else {
			f = filereopen(inname, "rb", dstp->f);
		}
		/* LINTED */
		if (stdinflag && f != (FILE *)NULL)
			setmode(fileno(f), O_BINARY);
		if (f == (FILE *)NULL) {
			errmsg(_("Can't open '%s'.\n"), dstp->inname);
			dstp->excode = geterrno();
		}
		dstp->argc--;
	} while (f == (FILE *)NULL);

	if (f == (FILE *)NULL)
		return (FALSE);

	dstp->f = f;
	file_raise(dstp->f, FALSE);
#ifdef	_FASCII		/* Mark Williams C */
	dsp->f->_ff &= ~_FASCII;
#endif
	return (TRUE);
}

/*
 * Seek/skip bytes and return the number of unskipped bytes
 */
LOCAL off_t
advance(dstp, xpos)
	dst_t	*dstp;
	off_t	xpos;
{
	off_t	newpos = xpos;

	do {
		newpos = doadvance(dstp->f, newpos, dstp->argc <= 0);
	} while (newpos > 0 && open_next(dstp));
	return (newpos);
}

#include <schily/stat.h>

/*
 * Seek/skip bytes and return the number of unskipped bytes
 */
LOCAL off_t
doadvance(f, xpos, is_last)
	FILE	*f;
	off_t	xpos;
	BOOL	is_last;
{
	struct stat sb;

	if (fstat(fdown(f), &sb) < 0)
		return (doskip(f, xpos));
	if (isatty(fdown(f)))
		return (doskip(f, xpos));
	/*
	 * Skip fast in case this is a plain file smaller than the skip value.
	 */
	if (S_ISREG(sb.st_mode) && sb.st_size < xpos)
		return (xpos - sb.st_size);
	/*
	 * Seekable is any plain file
	 * and the last block or character device in the file name list.
	 */
	if (S_ISREG(sb.st_mode) ||
	    (is_last &&
	    (S_ISBLK(sb.st_mode) ||
	    S_ISCHR(sb.st_mode)))) {
		off_t	newpos = xpos;

		/*
		 * On raw devices, we need to be aware of the max. sector size.
		 * Make sure that we still need to read something to be able to
		 * verify our position in case we are on a device.
		 */
		if (S_ISBLK(sb.st_mode) || S_ISCHR(sb.st_mode)) {
			newpos = xpos / 4096 * 4096;
			if (newpos == xpos && newpos > 0)
				newpos -= 4096;
		}
		(void) fileseek(f, newpos);
		newpos = filepos(f);
		if (newpos > 0)
			xpos -= newpos;
	}
	return (doskip(f, xpos));
}

/*
 * Skip bytes and return the number of unskipped bytes
 */
LOCAL off_t
doskip(f, xpos)
	FILE	*f;
	off_t	xpos;
{
	char	sbuf[BUFSIZ];
	int	amt;

	while (xpos > 0) {
		amt = sizeof (sbuf);
		if (xpos < amt)
			amt = (int)xpos;
		amt = fileread(f, sbuf, amt);
		if (amt <= 0)
			break;
		xpos -= amt;
	}
	return (xpos);
}

LOCAL BOOL
out_ispipe()
{
	struct stat sb;

	if (fstat(fdown(stdout), &sb) < 0)
		return (TRUE);
	return (S_ISFIFO(sb.st_mode) || S_ISSOCK(sb.st_mode));
}

/*
 * Fill POSIX size description strings for -t type according to the parameters
 * of the current platform.
 *
 * The typical ILP32 and LP64 implementations do not cause problems as all
 * sizes 1 2 4 8 are present as basic types. Tru64 and older Linux versions for
 * DEC Alpha are implemented as ILP64 and cause a problem from missing an
 * official basic type with 32 bits. There is only __int32.
 */
LOCAL char	chartype[3]	= { 'C', sizeof (char) + '0', '\0' };
LOCAL char	shorttype[3]	= { 'S', sizeof (short) + '0', '\0' };
LOCAL char	inttype[3]	= { 'I', sizeof (int) + '0', '\0' };
LOCAL char	longtype[3]	= { 'L', sizeof (long) + '0', '\0' };
#if	SIZEOF_LONG_LONG > SIZEOF_LONG_INT
LOCAL char	longlongtype[3]	= { sizeof (Llong) + '0', '\0' };
#endif
LOCAL char	floattype[3]	= { 'F', sizeof (float) + '0', '\0' };
LOCAL char	doubletype[3]	= { 'D', sizeof (double) + '0', '\0' };
#ifdef	HAVE_LONGDOUBLE
LOCAL char	ldoubletype[3]	= { 'L', '\0' };
#endif

/*
 * POSIX type string to output conversion.
 */
LOCAL int
ptype(arg)
	const	char	*arg;
{
	int	type;
	int	size;

#define	error(a)

	while ((type = *arg++) != '\0') {

		switch (type) {

		case 'a':
			error("CHAR Named character\n");
			add_dout(&d_asc);
			break;
		case 'c':
			error("CHAR Character\n");
			add_dout(&d_mbch);
			break;

			/* Int size */
		case 'd':	/* C S I L 1 2 4 8 */
			size = *arg++;

			if (size == '\0') {
				error("INT Dezimal\n");
				add_dout(&d_i_d);
				arg--;
			} else if (strchr(chartype, size)) {
				error("CHAR Dezimal\n");
				add_dout(&d_c_d);
			} else if (strchr(shorttype, size)) {
				error("SHORT Dezimal\n");
				add_dout(&d_s_d);
			} else if (strchr(inttype, size)) {
				error("INT Dezimal\n");
				add_dout(&d_i_d);
			} else if (strchr(longtype, size)) {
				error("LONG Dezimal\n");
				add_dout(&d_l_d);
#if	SIZEOF_LONG_LONG > SIZEOF_LONG_INT
			} else if (strchr(longlongtype, size)) {
				error("LONG LONG Dezimal\n");
				add_dout(&d_ll_d);
#endif
			} else {
				if (isupper(size) || isdigit(size))
					return (illsize(size));
				error("INT Dezimal\n");
				add_dout(&d_i_d);
				arg--;
			}
			break;

#ifndef	NO_FLOATINGPOINT
		case 'f':	/* F D L 4 8 */
			size = *arg++;

			if (size == '\0') {
				error("DOUBLE\n");
				add_dout(&d_d_f);
				arg--;
			} else if (strchr(floattype, size)) {
				error("FLOAT\n");
				add_dout(&d_f_f);
			} else if (strchr(doubletype, size)) {
				error("DOUBLE\n");
				add_dout(&d_d_f);
#ifdef	HAVE_LONGDOUBLE
			} else if (strchr(ldoubletype, size)) {
				error("LONG DOUBLE\n");
				add_dout(&d_ld_f);
#endif
			} else {
				if (isupper(size) || isdigit(size))
					return (illfsize(size));
				error("DOUBLE\n");
				add_dout(&d_d_f);
				arg--;
			}
			break;
#endif

		case 'o':	/* C S I L 1 2 4 8 */
			size = *arg++;

			if (size == '\0') {
				error("INT Octal\n");
				add_dout(&d_i_o);
				arg--;
			} else if (strchr(chartype, size)) {
				error("CHAR Octal\n");
				add_dout(&d_c_o);
			} else if (strchr(shorttype, size)) {
				error("SHORT Octal\n");
				add_dout(&d_s_o);
			} else if (strchr(inttype, size)) {
				error("INT Octal\n");
				add_dout(&d_i_o);
			} else if (strchr(longtype, size)) {
				error("LONG Octal\n");
				add_dout(&d_l_o);
#if	SIZEOF_LONG_LONG > SIZEOF_LONG_INT
			} else if (strchr(longlongtype, size)) {
				error("LONG LONG Octal\n");
				add_dout(&d_ll_o);
#endif
			} else {
				if (isupper(size) || isdigit(size))
					return (illsize(size));
				error("INT Octal\n");
				add_dout(&d_i_o);
				arg--;
			}
			break;

		case 'u':	/* C S I L 1 2 4 8 */
			size = *arg++;

			if (size == '\0') {
				error("INT Unsigned\n");
				add_dout(&d_i_u);
				arg--;
			} else if (strchr(chartype, size)) {
				error("CHAR Unsigned\n");
				add_dout(&d_c_u);
			} else if (strchr(shorttype, size)) {
				error("SHORT Unsigned\n");
				add_dout(&d_s_u);
			} else if (strchr(inttype, size)) {
				error("INT Unsigned\n");
				add_dout(&d_i_u);
			} else if (strchr(longtype, size)) {
				error("LONG Unsigned\n");
				add_dout(&d_l_u);
#if	SIZEOF_LONG_LONG > SIZEOF_LONG_INT
			} else if (strchr(longlongtype, size)) {
				error("LONG LONG Unsigned\n");
				add_dout(&d_ll_u);
#endif
			} else {
				if (isupper(size) || isdigit(size))
					return (illsize(size));
				error("INT Unsigned\n");
				add_dout(&d_i_u);
				arg--;
			}
			break;

		case 'x':	/* C S I L 1 2 4 8 */
			size = *arg++;

			if (size == '\0') {
				error("INT Hex\n");
				add_dout(&d_i_x);
				arg--;
			} else if (strchr(chartype, size)) {
				error("CHAR Hex\n");
				add_dout(&d_c_x);
			} else if (strchr(shorttype, size)) {
				error("SHORT Hex\n");
				add_dout(&d_s_x);
			} else if (strchr(inttype, size)) {
				error("INT Hex\n");
				add_dout(&d_i_x);
			} else if (strchr(longtype, size)) {
				error("LONG Hex\n");
				add_dout(&d_l_x);
#if	SIZEOF_LONG_LONG > SIZEOF_LONG_INT
			} else if (strchr(longlongtype, size)) {
				error("LONG LONG Hex\n");
				add_dout(&d_ll_x);
#endif
			} else {
				if (isupper(size) || isdigit(size))
					return (illsize(size));
				error("INT hex\n");
				add_dout(&d_i_x);
				arg--;
			}
			break;

		default:
			return (illtype(type));
		}
	}
	return (1);
}

LOCAL int
illsize(size)
	int	size;
{
	errmsgno(EX_BAD,
		_("Illegal size '%c', use C, S, I, L, 1, 2, 4 or 8.\n"),
		size);
	return (-1);
}

LOCAL int
illfsize(size)
	int	size;
{
	errmsgno(EX_BAD,
		_("Illegal size '%c', use F, D L, 4 or 8.\n"),
		size);
	return (-1);
}

LOCAL int
illtype(type)
	int	type;
{
	errmsgno(EX_BAD,
#ifndef	NO_FLOATINGPOINT
	_("Illegal type '%c', use 'a', 'c', 'd', 'f', 'o', 'u' or 'x'.\n"),
#else
	_("Illegal type '%c', use 'a', 'c', 'd', 'o', 'u' or 'x'.\n"),
#endif
		type);
	return (-1);
}

LOCAL void
setaddrfmt(Aflag, lradix)
	int	Aflag;
	int	lradix;
{
	char	*llfmt;			/* Address format off_t -> Llong   */
	char	*lfmt;			/* Address format off_t -> long	   */

	if (is_od)
		samefmt = "*\n";
	else
		samefmt = "     *\n";

	switch (Aflag) {

	case 'x':
		if (is_od) {
			lfmt = "%7.7lx";
			llfmt = "%7.7llx";
		} else {
			lfmt = "%6lx: ";
			llfmt = "%6llx: ";
		}
		break;

	case 'd':
		if (is_od) {
			lfmt = "%7.7ld";
			llfmt = "%7.7lld";
		} else {
			lfmt = "%6ld: ";
			llfmt = "%6lld: ";
		}
		break;

	case 'o':
		if (is_od) {
			lfmt = "%7.7lo";
			llfmt = "%7.7llo";
		} else {
			lfmt = "%6.6lo: ";
			llfmt = "%6.6llo: ";
		}
		break;

	case 'n':
		if (is_od) {
			llfmt = lfmt = "\t";
		} else {
			llfmt = lfmt = "        ";
		}
		samefmt = "*\n";
		break;

	case 0:				/* No -Ac format specified */
		if (is_od) {
			llfmt = lradix == 16 ? "%07.7llx" :
					(lradix == 10 ? "%7.7lld" : "%7.7llo");
			lfmt  = lradix == 16 ? "%07.7lx" :
					(lradix == 10 ? "%7.7ld" : "%7.7lo");
		} else {
			llfmt = lradix == 8 ? "%06llo:" :
					(lradix == 10 ? "%6lld:" : "%6llx:");
			lfmt  = lradix == 8 ? "%06lo:" :
					(lradix == 10 ? "%6ld:" : "%6lx:");
		}
		break;
	default:
		llfmt = lfmt = "";	/* Make GCC and lint happy */

		/* NOTREACHED */
		comerrno(EX_BAD,
		_("-A option only accepts the following:  d, o, n, and x.\n"));
	}
	if (sizeof (pos) > sizeof (long))
		addrfmt = llfmt;
	else
		addrfmt = lfmt;
}
