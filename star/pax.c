/* @(#)pax.c	1.36 18/05/21 Copyright 1989, 2003-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char _p_sccsid[] =
	"@(#)pax.c	1.36 18/05/21 Copyright 1989, 2003-2018 J. Schilling";
#endif
/*
 *	PAX specific routines for star main program.
 *
 *	Copyright (c) 1989, 2003-2018 J. Schilling
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

#ifdef	STAR_MAIN
/*
 * We have been included from star.c
 */
#define	opts		pax_opts
#define	_opts		_pax_opts
#define	gargs		pax_gargs
#define	susage		pax_susage
#define	usage		pax_usage
#define	xusage		pax_xusage
#else
#define	NO_STAR_MAIN
#define	PAX_MAIN
#define	PTYPE_DEFAULT	P_PAX

#include "star.c"
#endif

LOCAL	void	gargs		__PR((int ac, char *const *av));
LOCAL	void	susage		__PR((int ret));
LOCAL	void	usage		__PR((int ret));
LOCAL	void	xusage		__PR((int ret));
#ifdef	STAR_MAIN
LOCAL	void	pax_setopts	__PR((char *o));
#endif

/*
 *	-r	Read					tar x
 *	-w	Write					tar c
 *	-a	Append -w -a				tar r
 *		Update -w -a -u				tar u
 *
 *	-b	Blocksize in Bytes
 *	-c	Invert match for pattern and files
 *	-d	Do not descend dirs
 *	-f	Archive File
 *	-i	Interactive Rename
 *	-k	Noklobber
 *	-l	Create hard links in copy mode
 *	-n	Select first archive member per pattern only
 *	-o	Future Options
 *	-p	Privilleges:
 *		e	everything
 *		a	Do NOT preserve access	time
 *		m	Do NOT preserve mod	time
 *		o	Preserve UID/GID
 *		p	Preserve perm
 *	-s	Substitution
 *	-t	Reset access time
 *	-u	Gegenteil von -U
 *	-v	List Verbose
 *	-x	Archive Format
 *	-X	Wie star -M
 *
 *	-H	SUSv3
 *	-L	SUSv3
 */

/*
 * PAX related options
 *
 * The official POSIX options start after the -bz/-lzo/-7z/-xz/-lzip option.
 */
/* BEGIN CSTYLED */
char	_opts[] = "help,xhelp,version,debug,xdebug#,xd#,time,no-statistics,do-statistics,fifostats,numeric,no-fifo,no-fsync,bs&,fs&,/,..,secure-links,acl,xfflags,z,bz,lzo,7z,xz,lzip,r,w,a,b&,c,d,f&,H,i,k,L,l,n,o*,p&,s&,t,u,v+,x&,artype&,X";
/* END CSTYLED */
char	*opts = _opts;
#ifdef	NO_STAR_MAIN
struct ga_props	gaprops;
#endif

LOCAL	void	pax_info	__PR((void));

LOCAL void
gargs(ac, av)
	int		ac;
	char	*const *av;
{
	int	files	 = 0;
	BOOL	help	 = FALSE;
	BOOL	xhelp	 = FALSE;
	BOOL	prvers	 = FALSE;
	BOOL	no_fifo	 = FALSE;
	BOOL	paxrflag = FALSE;
	BOOL	paxwflag = FALSE;
	BOOL	paxaflag = FALSE;
	BOOL	paxtflag = FALSE;
	BOOL	paxdflag = FALSE;
	BOOL	paxiflag = FALSE;
	BOOL	paxlflag = FALSE;
	char	*paxopts = NULL;
	BOOL	paxuflag = FALSE;
	BOOL	do_stats = FALSE;

	/*
	 * Current default archive format in all other cases is USTAR.
	 * We may change this to PAX in the future.
	 */
	hdrtype = H_USTAR;
#ifdef	STAR_MAIN
	pax_setopts(opts);			/* set up opts for getfiles */
#endif
	getarginit(&gaprops, GAF_SINGLEARG);	/* POSIX combined args	  */

	iftype		= I_PAX;		/* command line interface */
	ptype		= P_PAX;		/* program interface type */
	paxls		= TRUE;
	paxmatch	= TRUE;
	nopflag		= TRUE;			/* pax default */
	no_stats	= TRUE;			/* -no-statitstics	   */
	nochown		= TRUE;			/* chown only with -po / -pe */

	--ac, ++av;
	files = getfilecount(ac, av, opts);
	if (getlallargs(&ac, &av, &gaprops, opts,
				&help, &xhelp, &prvers, &debug, &xdebug, &xdebug,
#ifndef	__old__lint
				&showtime, &no_stats, & do_stats, &do_fifostats,
				&numeric,  &no_fifo, &no_fsync,
				getenum, &bs,
				getenum, &fs,
				&abs_path, &allow_dotdot, &secure_links,
				&doacl, &dofflags,
				&zflag, &bzflag, &lzoflag,
				&p7zflag, &xzflag, &lzipflag,
				&paxrflag,
				&paxwflag,
				&paxaflag,
				getenum, &bs,
				&notarg,		/* -c */
				&paxdflag,		/* -d */
				addtarfile, NULL,	/* -f */
				&paxHflag,		/* -H */
				&paxiflag,		/* -i */
				&keep_old,		/* -k */
				&paxfollow,		/* -L */
				&paxlflag,		/* -l */
				&paxnflag,		/* -n */
				&paxopts,		/* -o */
				getpriv, NULL,		/* -p */
				parsesubst, &do_subst,	/* -s */
				&paxtflag,		/* -t */
				&paxuflag,		/* -u */
				&verbose,		/* -v */
				gethdr, &chdrtype,	/* -x */
				gethdr, &chdrtype,	/* artype= */
#endif /* __old__lint */
				&nomount) < 0) {
		errmsgno(EX_BAD, "Bad Option: %s.\n", av[0]);
		susage(EX_BAD);
	}
	star_helpvers("spax", help, xhelp, prvers);

	if (!paxrflag && !paxwflag) {
		tflag = TRUE;
	} else if (paxrflag && paxwflag) {
		copyflag = TRUE;
		if (files == 1)
			listfile = "-";
	} else if (paxwflag && paxaflag) {
		if (paxuflag)
			uflag = TRUE;
		else
			rflag = TRUE;
	} else if (paxwflag) {
		cflag = TRUE;
		if (files == 0)
			listfile = "-";
	} else {
		xflag = TRUE;
	}

	if ((cflag || uflag || rflag || copyflag) && paxtflag)
		acctime = TRUE;

	if (paxdflag) {
		/* XXX nodesc aber mit pattern match bei extract */
		nodesc = TRUE;
	}
	if (paxiflag) {
		interactive = TRUE;
		paxinteract = TRUE;
	}
	if (paxlflag) {
		/* link bei -copy */
		errmsgno(EX_BAD, "Unsupported option -l.\n");
		susage(EX_BAD);
	}
	uncond = !paxuflag;

	if (do_stats)
		no_stats = FALSE;

	star_checkopts(/* oldtar */ FALSE, /* dodesc */ TRUE,
				/* usetape */ FALSE,
				/* archive */ -1, no_fifo,
				paxopts,
				/* llbs */ 0);

	nolinkerr = FALSE;
}

LOCAL void
pax_info()
{
	error("\nFor a more complete user interface use the tar type command interface.\n");
	error("See 'man star'. The %s command is more or less limited to the\n", get_progname());
	error("POSIX standard pax command line interface.\n");
}

/*
 * Short usage
 */
LOCAL void
susage(ret)
	int	ret;
{
	error("Usage:\t%s cmd [options] file1 ... filen\n", get_progname());
	error("\nUse\t%s -help\n", get_progname());
	error("and\t%s -xhelp\n", get_progname());
	error("to get a list of valid cmds and options.\n");
	error("\nUse\t%s -x help\n", get_progname());
	error("to get a list of valid archive header formats.\n");
	pax_info();
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\t%s cmd [options] file1 ... filen\n", get_progname());
	error("Cmd:\n");
	error("\t<none>\t\tlist named files from tape\n");
	error("\t-r\t\textract named files from tape\n");
	error("\t-w\t\twrite archive with named files to tape\n");
	error("\t-w -a\t\tupdate/replace named files to tape\n");
	error("\t-r -w\t\tcopy named files to destination directory\n");
	error("Options:\n");
	error("\t-help\t\t(*) print this help\n");
	error("\t-xhelp\t\t(*) print extended help\n");
	error("\t-version\t(*) print version information and exit\n");
	error("\t-b #\t\tset blocking factor to # Bytes (default 10240)\n");
	error("\t-c\t\tinvert matching rules\n");
	error("\t-d\t\tdo not descend directories\n");
	error("\t-f nm\t\tuse 'nm' as tape instead of stdin/stdout\n");
	error("\t-H\t\tfollow symbolic links from cmdline as if they were files\n");
	error("\t-i\t\tdo interactive creation/extraction/renaming\n");
	error("\t-k\t\tkeep existing files\n");
	error("\t-l\t\tlink files rather than copying them\n");
	error("\t-L\t\tfollow symbolic links as if they were files\n");
	error("\t-n\t\tone match per pattern only\n");
	error("\t-o\t\toptions (none specified with SUSv2 / UNIX-98)\n");
	error("\t-p string\tset privileges\n");
	error("\t-s replstr\tApply ed like pattern substitution -s /old/new/gp on filenames\n");
	error("\t-t\t\trestore atime after reading files\n");
	error("\t-u\t\treplace/restore files only if they are newer\n");
	error("\t-v\t\tincrement verbose level\n");
	error("\t-x header\tgenerate 'header' type archive (see -x help)\n");
	error("\tartype=header\t(*) generate 'header' type archive (see artype=help)\n");
	error("\t-X\t\tdo not descend mounting points\n");
	error("\t-z\t\t(*) pipe input/output through gzip, does not work on tapes\n");
	error("\t-bz\t\t(*) pipe input/output through bzip2, does not work on tapes\n");
	error("\t-lzo\t\t(*) pipe input/output through lzop, does not work on tapes\n");
	error("\t-7z\t\t(*) pipe input/output through p7zip, does not work on tapes\n");
	error("\t-xz\t\t(*) pipe input/output through xz, does not work on tapes\n");
	error("\t-lzip\t\t(*) pipe input/output through lzip, does not work on tapes\n");
#ifdef	FIFO
	error("\t-no-fifo\t(*) don't use a fifo to optimize data flow from/to tape\n");
#endif
	error("\nAll options marked with (*) are not defined by POSIX.\n");
	pax_info();
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
xusage(ret)
	int	ret;
{
	error("Usage:\t%s cmd [options] file1 ... filen\n", get_progname());
	error("Extended options:\n");
	error("\t-debug\t\tprint additional debug messages\n");
	error("\txdebug=#,xd=#\tset extended debug level\n");
	error("\t-/\t\tdon't strip leading '/'s from file names\n");
	error("\t-..\t\tdon't skip filenames that contain '..' in non-interactive extract\n");
	error("\t-secure-links\tdon't extract links that start with '/' or contain '..'\n");
	error("\t-acl\t\thandle access control lists\n");
	error("\t-xfflags\thandle extended file flags\n");
	error("\tbs=#\t\tset (output) block size to #\n");
#ifdef	FIFO
	error("\tfs=#\t\tset fifo size to #\n");
#endif
	error("\t-no-fsync\tdo not call fsync() for each extracted file (may be dangerous)\n");
	error("\t-time\t\tprint timing info\n");
	error("\t-no-statistics\tdo not print statistics\n");
	error("\t-do-statistics\tprint statistics\n");
#ifdef	FIFO
	error("\t-fifostats\tprint fifo statistics\n");
#endif
	error("\t-numeric\tdon't use user/group name from tape\n");
	error("\nAll options above are not defined by POSIX.\n");
	exit(ret);
	/* NOTREACHED */
}

#ifdef	STAR_MAIN
#undef	opts
#undef	_opts
#undef	gargs
#undef	susage
#undef	usage
#undef	xusage

LOCAL void
pax_setopts(o)
	char	*o;
{
extern	char	*opts;
	opts = o;
}
#endif
