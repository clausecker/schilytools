/* @(#)suntar.c	1.36 17/09/20 Copyright 1989, 2003-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char _s_sccsid[] =
	"@(#)suntar.c	1.36 17/09/20 Copyright 1989, 2003-2017 J. Schilling";
#endif
/*
 *	Solaris TAR specific routines for star main program.
 *
 *	Copyright (c) 1989, 2003-2017 J. Schilling
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

/*
 * The options -C & -I are not supported without '-' with suntar.
 */
static	char	*sun_copt = "crtuxbBDeEfFhiklmnopPqTvwX@01234567";

#ifdef	STAR_MAIN
/*
 * We have been included from star.c
 */
#define	opts		suntar_opts
#define	_opts		_suntar_opts
#define	gargs		suntar_gargs
#define	susage		suntar_susage
#define	usage		suntar_usage
#define	xusage		suntar_xusage
#define	DFLT_FILE	"/etc/default/tar"
#else
#define	NO_STAR_MAIN
#define	SUNTAR_MAIN
#define	PTYPE_DEFAULT	P_SUNTAR
#define	DFLT_FILE	"/etc/default/tar"

#include "star.c"
#endif

#define	SUN_TAR

LOCAL	void	gargs		__PR((int ac, char *const *av));
LOCAL	void	susage		__PR((int ret));
LOCAL	void	usage		__PR((int ret));
LOCAL	void	xusage		__PR((int ret));
#ifdef	STAR_MAIN
LOCAL	void	suntar_setopts	__PR((char *o));
#endif

/*
 * tar {crtux}[bBDeEfFhiklmnopPqvwX@[0-7]] [-k size] [blocksize] [tapefile] [exclude-file] [-I include-file] files ...
 *
 *	-I fehlt noch
 */


/*
 * Solaris TAR related options
 */
/* BEGIN CSTYLED */
char	_opts[] = "C*,help,xhelp,version,debug,xdebug#,xd#,time,no-statistics,do-statistics,fifostats,numeric,no-fifo,no-fsync,do-fsync%0,sattr,bs&,fs&,/,..,secure-links,acl,xfflags,copy,diff,artype&,O,z,bz,lzo,7z,xz,lzip,c,r,t,u,x,b&,B,D,e,E,f&,F,h,I*,i,k&,l,m,n,o,p,P,q,v+,w,X&,@,T,?";
/* END CSTYLED */
char	*opts = _opts;
struct ga_props	gaprops;

LOCAL	void	suntar_info	__PR((void));

LOCAL void
gargs(ac, av)
	int		ac;
	char	*const *av;
{
	int	files	 = 0;
	int	minfiles = 1;
	BOOL	help	 = FALSE;
	BOOL	xhelp	 = FALSE;
	BOOL	prvers	 = FALSE;
	BOOL	no_fifo	 = FALSE;
	BOOL	oldtar   = FALSE;
	BOOL	sunDflag = FALSE;
	BOOL	sunEflag = FALSE;
	BOOL	sunpflag = FALSE;
	BOOL	sunqflag = FALSE;
	char	*sunI    = NULL;
	BOOL	sunxattr = FALSE;
	BOOL	sunTflag = FALSE;
	BOOL	do_stats = FALSE;
	BOOL	do_sattr = FALSE;
signed	char	archive	 = -1;		/* On IRIX, we have unsigned chars by default */

	/*
	 * Current default archive format in all other cases is USTAR.
	 * We may change this to PAX in the future.
	 */
	hdrtype = H_USTAR;
#ifdef	STAR_MAIN
	suntar_setopts(opts);
#endif
	getarginit(&gaprops, GAF_DEFAULT);	/* Set default behavior	  */

	iftype		= I_TAR;		/* command line interface */
	ptype		= P_SUNTAR;		/* program interface type */
	bsdchdir	= TRUE;
	uncond		= TRUE;			/* tar -x is star -xU ...  */
	force_remove	= TRUE;			/* and -force-remove  ...  */
	remove_first	= TRUE;			/* and -remove-first  ...  */
	keep_nonempty_dirs = TRUE;		/* and -keep-nonempty-dirs */
	no_fsync	= TRUE;			/* and -no-fsync	   */
	no_stats	= TRUE;			/* and -no-statitstics	   */
	/*
	 * NOTE: star by default writes into existing files if possible.
	 *	If we like to emulate the Sun tar behavior, we need to call
	 *	star -xU -force-remove -remove-first -keep-nonempty-dirs
	 *	The only remaining difference to Sun tar is that Sun tar does
	 * 	not try to remove non-empty directories in case that a
	 *	directory of the name name is to be made next.
	 *	One problem with -remove-first is that it slows down extraction
	 */

	--ac, ++av;
	files = getfilecount(ac, av, opts);
	if (getlallargs(&ac, &av, &gaprops, opts,
				&dir_flags,
				&help, &xhelp, &prvers, &debug, &xdebug, &xdebug,
#ifndef	__old__lint
				&showtime, &no_stats, &do_stats, &do_fifostats,
				&numeric,  &no_fifo, &no_fsync, &no_fsync,
				&do_sattr,		/* --sattr */
				getnum, &bs,
				getnum, &fs,
				&abs_path, &allow_dotdot, &secure_links,
				&doacl, &dofflags,
				&copyflag, &diff_flag,
				gethdr, &chdrtype,
				&oldtar,
				&zflag, &bzflag, &lzoflag,
				&p7zflag, &xzflag, &lzipflag,

				&cflag,
				&rflag,
				&tflag,
				&uflag,
				&xflag,

				getnum, &bs,		/* -b blocks */
				&multblk,		/* -B */
				&sunDflag,		/* -D */
				&errflag,		/* -e */
				&sunEflag,		/* -E */
				addtarfile, NULL,	/* -f archive */
				&Fflag,			/* -F */
				&paxfollow,		/* -h */
				&sunI,			/* -I */
				&ignoreerr,		/* -i */
				getknum, &tsize,	/* -k size */
				&nolinkerr,		/* -l */
				&nomtime,		/* -m */
				&not_tape,		/* -n */
				&nochown,		/* -o */
				&sunpflag,		/* -p */
				&no_dirslash,		/* -P */
				&sunqflag,		/* -q */
				&verbose,		/* -v */
				&interactive,		/* -w */
				getexclude, NULL,	/* -X */
				&sunxattr,		/* -@ */
				&sunTflag,		/* -T */
#endif /* __old__lint */
				&archive) < 0) {
		errmsgno(EX_BAD, "Bad Option: %s.\n", av[0]);
		susage(EX_BAD);
	}
	if (archive != -1 && !(archive >= '0' && archive <= '7')) {
		errmsgno(EX_BAD, "Bad Option: -%c.\n", archive);
		susage(EX_BAD);
	}
	star_helpvers("suntar", help, xhelp, prvers);

	if (sunDflag)
		errconfig("WARN|GROW|SHRINK *");

	if (sunEflag) {
		chdrtype = H_SUNTAR;
		no_dirslash = TRUE;
		if (cflag || rflag || uflag) {
			errmsgno(EX_BAD, "The -E option creates a deprecated archive type.\n");
			errmsgno(EX_BAD, "Use artype=exustar to create a POSIX extended archive.\n");
			if (sunpflag) {
				errmsgno(EX_BAD, "Switching to artype=exustar to support ACLs.\n");
				chdrtype = H_EXUSTAR;
			}
		}
	}
	if (sunI) {
		errmsgno(EX_BAD, "The -I option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (tsize) {
#ifdef	NO_SPLIT
		errmsgno(EX_BAD, "The -k option currently does not split files.\n");
#else
		multivol = TRUE;
#endif
	}
	if (not_tape) {
		errmsgno(EX_BAD, "The -n option is not yet implemented.\n");
	}
	if (sunpflag) {
		pflag = TRUE;
		doacl = TRUE;
	}
	if (sunqflag) {
		errmsgno(EX_BAD, "The -q option is not yet implemented (as in Sun tar).\n");
		susage(EX_BAD);
	}
	if (sunxattr) {
		errmsgno(EX_BAD, "The -@ option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (do_sattr) {
		errmsgno(EX_BAD, "The --sattr option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (sunTflag) {
		errmsgno(EX_BAD, "The -T option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (do_stats)
		no_stats = FALSE;

	star_checkopts(oldtar, /* dodesc */ FALSE, /* usetape */ TRUE,
					archive, no_fifo, /* llbs */ 0);
	star_nfiles(files, minfiles);

	star_defaults(&fs, DFLT_FILE);	/* Also check for Sun defaults */
}

LOCAL void
suntar_info()
{
	error("\nFor a more complete user interface use the tar type command interface.\n");
	error("See 'man star'. The %s command is more or less limited to the\n", get_progname());
	error("Solaris tar command line interface.\n");
}

/*
 * Short usage
 */
LOCAL void
susage(ret)
	int	ret;
{
	error("Usage:\t%s cmd [options] file1 ... filen\n", get_progname());
	error("\nUse\t%s --help\n", get_progname());
	error("and\t%s --xhelp\n", get_progname());
	error("to get a list of valid cmds and options.\n");
	suntar_info();
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\t%s cmd [options] file1 ... filen\n", get_progname());
	error("Cmd:\n");
	error("\t-c/-u/-r\tcreate/update/replace archive with named files to tape\n");
	error("\t-x/-t\t\textract/list named files from tape\n");
	error("\t--copy\t\t(*) copy named files to destination directory\n");
	error("\t--diff\t\t(*) diff archive against file system (see -xhelp)\n");
	error("\tartype=header\t(*) generate 'header' type archive (see artype=help)\n");
	error("Options:\n");
	error("\t--help\t\t(*) print this help\n");
	error("\t--xhelp\t\t(*) print extended help\n");
	error("\t--version\t(*) print version information and exit\n");
	error("\t-b #\t\tset blocking factor to #x512 Bytes (default 20)\n");
	error("\t-B\t\tperform multiple reads (needed on pipes)\n");
	error("\t-D\t\ttreat data change errors as warnings only\n");
	error("\t-e\t\texit immediately if unexpeted errors ocur\n");
	error("\t-E\t\tWrite a tarfile with extended headers\n");
	error("\t-f nm\t\tuse 'nm' as tape instead of stdin/stdout\n");
	error("\t-F,-FF,-FFF,...\tdo not store/create SCCS/RCS, core and object files\n");
	error("\t-h\t\tfollow symbolic links as if they were files\n");
	error("\t-I yy\t\tXXX Not implemented\n");
	error("\t-i\t\tignore checksum errors\n");
	error("\t-k yy\t\tset tape volume size to yy (default multiplier is 512)\n");
	error("\t-l\t\tprint a message if not all links are dumped\n");
	error("\t-m\t\tdo not restore access and modification time\n");
	error("\t-n\t\tXXX Not implemented\n");
	error("\t-o\t\tdo not restore owner and group\n");
	error("\t-O\t\t(*)be compatible to old tar (except for checksum bug)\n");
	error("\t-p\t\trestore filemodes of directories\n");
	error("\t-P\t\tdo not add a trailing '/' on directory archive entries\n");
	error("\t-q\t\tXXX Not implemented\n");
	error("\t-v\t\tincrement verbose level\n");
	error("\t-w\t\tdo interactive creation/extraction/renaming\n");
	error("\t-X yy\t\tExclude files from file yy containing a list of path names\n");
	error("\t-@\t\tXXX Not implemented\n");
	error("\t--sattr\t\tXXX Not implemented\n");
	error("\t-T\t\tXXX Not implemented\n");
	error("\t-[0-7]\t\tSelect an alternative tape drive\n");
	error("\t-z\t\t(*) pipe input/output through gzip, does not work on tapes\n");
	error("\t--bz\t\t(*) pipe input/output through bzip2, does not work on tapes\n");
	error("\t--lzo\t\t(*) pipe input/output through lzop, does not work on tapes\n");
	error("\t--7z\t\t(*) pipe input/output through p7zip, does not work on tapes\n");
	error("\t--xz\t\t(*) pipe input/output through xz, does not work on tapes\n");
	error("\t--lzip\t\t(*) pipe input/output through lzip, does not work on tapes\n");
#ifdef	FIFO
	error("\t--no-fifo\t(*) don't use a fifo to optimize data flow from/to tape\n");
#endif
	error("\nAll options marked with (*) are not defined by Solaris tar.\n");
	suntar_info();
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
xusage(ret)
	int	ret;
{
	error("Usage:\t%s cmd [options] file1 ... filen\n", get_progname());
	error("Extended options:\n");
	error("\t--debug\t\tprint additional debug messages\n");
	error("\txdebug=#,xd=#\tset extended debug level\n");
	error("\t-/\t\tdon't strip leading '/'s from file names\n");
	error("\t--..\t\tdon't skip filenames that contain '..' in non-interactive extract\n");
	error("\t--secure-links\tdon't extract links that start with '/' or contain '..'\n");
	error("\t--acl\t\thandle access control lists\n");
	error("\t--xfflags\thandle extended file flags\n");
	error("\tbs=#\t\tset (output) block size to #\n");
#ifdef	FIFO
	error("\tfs=#\t\tset fifo size to #\n");
#endif
	error("\t--no-fsync\tdo not call fsync() for each extracted file (may be dangerous)\n");
	error("\t--do-fsync\tcall fsync() for each extracted file\n");
	error("\t--time\t\tprint timing info\n");
	error("\t--no-statistics\tdo not print statistics\n");
	error("\t--do-statistics\tprint statistics\n");
#ifdef	FIFO
	error("\t--fifostats\tprint fifo statistics\n");
#endif
	error("\t--numeric\tdon't use user/group name from tape\n");
	error("\nAll options above are not defined by Solaris tar.\n");
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
suntar_setopts(o)
	char	*o;
{
extern	char	*opts;
	opts = o;
}
#endif
