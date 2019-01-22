/* @(#)cpio.c	1.33 19/01/05 Copyright 1989, 2005-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char _c_sccsid[] =
	"@(#)cpio.c	1.33 19/01/05 Copyright 1989, 2005-2019 J. Schilling";
#endif
/*
 *	CPIO specific routines for star main program.
 *
 *	Copyright (c) 1989, 2005-2019 J. Schilling
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
#define	opts		cpio_opts
#define	_opts		_cpio_opts
#define	gargs		cpio_gargs
#define	susage		cpio_susage
#define	usage		cpio_usage
#define	xusage		cpio_xusage
#else
#define	NO_STAR_MAIN
#define	CPIO_MAIN
#define	PTYPE_DEFAULT	P_CPIO

#include "star.c"
#endif

LOCAL	void	gargs		__PR((int ac, char *const *av));
LOCAL	void	susage		__PR((int ret));
LOCAL	void	usage		__PR((int ret));
LOCAL	void	xusage		__PR((int ret));
#ifdef	STAR_MAIN
LOCAL	void	cpio_setopts	__PR((char *o));
#endif

/*
 * cpio -o[aBcv]
 *
 * cpio -i[Bcdmrtuvf] [pattern ...]
 *
 * cpio -p[adlmuv] directory
 *
 *	-o	Write					tar c
 *	-i	Read					tar x
 *	-i	Read -i -t				tar t
 *	-p	Pass					tar -copy
 *
 *	-a	Reset access times of input files after they have been copied
 *	-B	Block input/output to 5120 bytes records
 *	-c	Write/read header information in character form for portability
 *	-d	Create directories as needed
 *	-f	Copy in all files except those in patterns
 *	-l	Whenever possible, link files rather than copying them
 *	-m	Retain previous file modification time
 *	-r	Interactively rename files
 *	-t	Write a table of contents of the input
 *	-u	Copy unconditionally
 *	-v	Verbose: print the names of the affected files
 *		With the t option, provides a detailed listing
 *
 *	SVr4:
 *	-A	Append
 *	-b	swap bytes and halfwords
 *	-C	Bufsize
 *	-E	Solaris wie star list=
 *	-H	header type
 *	-I file
 *	-k	Skip corrupt
 *	-L	Follow symlinks
 *	-M msg	Message when switching media
 *	-O file
 *	-P	Preserve ACLs
 *	-R id	Reassign ids
 *	-s	Swaps bytes within each half word
 *	-S	Swaps halfwords within each word
 *	-V	Special verbose
 *	-6	ignored
 *	-@	Extended File Attributes
 */

/*
 * CPIO related options
 *
 * The official POSIX options start after the -bz/-lzop/-7z/-xz/-lzip/zstd option.
 */
/* BEGIN CSTYLED */
char	_opts[] = "help,xhelp,version,debug,xdebug#,xd#,time,no-statistics,fifostats,numeric,no-fifo,no-fsync,do-fsync%0,bs&,fs&,/,..,secure-links,no-secure-links%0,acl,xfflags,z,bz,lzo,7z,xz,lzip,zstd,i,o,p,a,A,b,B,c,C&,d,E*,f,H&,artype&,I&,O&,k,l,L,m,M*,P,r,R,s,S,t,u,6,@,V,v";
/* END CSTYLED */
char	*opts = _opts;
#ifdef	NO_STAR_MAIN
struct ga_props	gaprops;
#endif

LOCAL	void	cpio_info	__PR((void));

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
	BOOL	cpioiflag = FALSE;
	BOOL	cpiooflag = FALSE;
	BOOL	cpiobflag = FALSE;
	BOOL	cpioBflag = FALSE;
	BOOL	cpiocflag = FALSE;
	BOOL	cpiolflag = FALSE;
	BOOL	cpiomflag = FALSE;
	char	*cpioM	  = NULL;
	char	*cpioR	  = NULL;
	BOOL	cpiosflag = FALSE;
	BOOL	cpioSflag = FALSE;
	BOOL	cpio6flag = FALSE;
	BOOL	sunxattr  = FALSE;
	BOOL	cpioVflag = FALSE;
	Llong	llbs	 = 0;


	/*
	 * It does not really make sense to follow the SVr4 rules and by
	 * default create a UNIX-V7 binary CPIO archive. UNIX-V7 binary
	 * CPIO archives are not byte order independent and other (non scpio
	 * based) cpio implementations are unable to deal with byte order
	 * problems without manual interventions.
	 *
	 * The POSIX 1003.1-1988 default archive format is H_CPIO_CHR. We by
	 * default limit the file name length to 256 bytes with H_CPIO_ODC.
	 *
	 * POSIX 1003.1-1988 does not specifiy what archive format to use with
	 * the -c option.
	 */
#ifdef	__notdef__
	hdrtype = H_CPIO_BIN;			/* The SYSv cpio default format */
#endif
	hdrtype = H_CPIO_ODC;			/* POSIX 1003.1-1988 with SYSv compat */
#ifdef	STAR_MAIN
	cpio_setopts(opts);			/* set up opts for getfiles */
#endif
	getarginit(&gaprops, GAF_DEFAULT);	/* Set default behavior	  */

	iftype		= I_CPIO;		/* command line interface */
	ptype		= P_CPIO;		/* program interface type */
#ifdef	__never__
	paxls		= TRUE;			/* If ever, implement cpiols */
#endif
	cpio_stats	= TRUE;
	paxmatch	= TRUE;
	pflag		= TRUE;			/* cpio always restores permissions */
	xdir		= TRUE;			/* cpio restores dirs unconditionally */
	do_install	= TRUE;			/* cpio always uses install(1) mode */
	force_remove	= TRUE;			/* cpio always removes old files */
	remove_recursive = TRUE;		/* cpio always removes recursively */
	nblocks		= 1;			/* cpio standard blocksize is 512 */
	no_fsync	= TRUE;			/* cpio by default does -no-fsync */
	/*
	 * NOTE: star by default writes into existing files if possible.
	 *	If we like to emulate the Sun cpio behavior, we need to call
	 *	star -x -install -force-remove -remove-recursive
	 *	The only remaining difference to Sun cpio is that Sun cpio does
	 * 	rename the old files before extracting the new files and star
	 *	extracts the new file into a temporary name and renames the
	 *	result to the original name. This is better for updating a life
	 *	system without the risk of problems with old binaries.
	 */

	--ac, ++av;
	files = getfilecount(ac, av, opts);
	if (getlallargs(&ac, &av, &gaprops, opts,
				&help, &xhelp, &prvers, &debug,
				&xdebug, &xdebug,
#ifndef	__old__lint
				&showtime, &no_stats, &do_fifostats,
				&numeric,  &no_fifo, &no_fsync, &no_fsync,
				getenum, &bs,
				getenum, &fs,
				&abs_path, &allow_dotdot,
				&secure_links, &secure_links,
				&doacl, &dofflags,
				&zflag, &bzflag, &lzoflag,
				&p7zflag, &xzflag, &lzipflag, &zstdflag,

				&cpioiflag,		/* -i */
				&cpiooflag,		/* -o */
				&copyflag,		/* -p */
				&acctime,		/* -a */
				&rflag,			/* -A */
				&cpiobflag,		/* -b */
				&cpioBflag,		/* -B */
				&cpiocflag,		/* -c */
				getllnum, &llbs,	/* -C */
				&noxdir,		/* -d */
				&listfile,		/* -E */
				&notarg,		/* -f */
				gethdr, &chdrtype,	/* -H */
				gethdr, &chdrtype,	/* artype= */
				addtarfile, NULL,	/* -I */
				addtarfile, NULL,	/* -O */
				&ignoreerr,		/* -k */
				&cpiolflag,		/* -l */
				&paxfollow,		/* -L */
				&cpiomflag,		/* -m */
				&cpioM,			/* -M */
				&doacl,			/* -P */
				&paxinteract,		/* -r */
				&cpioR,			/* -R */
				&cpiosflag,		/* -s */
				&cpioSflag,		/* -S */
				&tflag,			/* -t */
				&uncond,		/* -u */
				&cpio6flag,		/* -6 */
				&sunxattr,		/* -@ */
				&cpioVflag,		/* -V */
#endif /* __old__lint */
				&verbose) < 0) {
		errmsgno(EX_BAD, "Bad Option: %s.\n", av[0]);
		susage(EX_BAD);
	}
	star_helpvers("scpio", help, xhelp, prvers);

	if ((cpioiflag + cpiooflag + copyflag) > 1) {
		errmsgno(EX_BAD,
		"Too many commands, only one of -i -o or -p is allowed.\n");
		susage(EX_BAD);
	}
	if (!(cpioiflag | cpiooflag | copyflag)) {
		errmsgno(EX_BAD,
		"Missing command, one of -i, -o or -p must be specified.\n");
		susage(EX_BAD);
	}
	if (cpioiflag && !tflag) {
		xflag = TRUE;
	} else if (cpioiflag) {
		tflag = TRUE;
		acctime = FALSE;
	} else if (cpiooflag) {
		cflag = TRUE;
	}
	if ((cflag || copyflag) && listfile == NULL)
		listfile = "-";		/* This is always true for cpio. */


	if (uflag || rflag) {
		cflag = TRUE;
		no_fifo = TRUE;	/* Until we are able to reverse the FIFO */
	}

	if (cpiobflag) {
		/* swap all */
		errmsgno(EX_BAD, "Unsupported option -b.\n");
		susage(EX_BAD);
	}
	if (cpioBflag) {
		if (llbs != 0 || bs != 0) {
			errmsgno(EX_BAD, "Only one of -B -C bs=.\n");
			susage(EX_BAD);
		}
		nblocks = 10;
		bs = nblocks * TBLOCK;
	}
	if (llbs > 0) {
		if (bs != 0) {
			errmsgno(EX_BAD, "Only one of -B -C bs=.\n");
			susage(EX_BAD);
		}
		nblocks = llbs / TBLOCK;
		bs = (long)llbs;
		if (bs != llbs) {
			errmsgno(EX_BAD, "Blocksize used with -C too large.\n");
			susage(EX_BAD);
		}
		bs = 0;
	}
	if (copyflag) {
		if (nblocks == 1)
			nblocks = 20;
	}
	if (cpiocflag) {
		if (chdrtype != H_UNDEF) {
			errmsgno(EX_BAD, "Only one of -c -H.\n");
			susage(EX_BAD);
		}
		chdrtype = H_CPIO_ASC;
	}
	if (cpiolflag) {
		/* link bei -copy */
		errmsgno(EX_BAD, "Unsupported option -l.\n");
		susage(EX_BAD);
	}
	if (cpioM) {
		/* Message to use when switching media */
		errmsgno(EX_BAD, "Unsupported option -M.\n");
		susage(EX_BAD);
	}
	if (cpioR) {
		/* Reassign ownership and group */
		errmsgno(EX_BAD, "Unsupported option -R.\n");
		susage(EX_BAD);
	}
	if (cpiosflag) {
		/* Swap bytes */
		errmsgno(EX_BAD, "Unsupported option -s.\n");
		susage(EX_BAD);
	}
	if (cpioSflag) {
		/* Swap halfwaords */
		errmsgno(EX_BAD, "Unsupported option -S.\n");
		susage(EX_BAD);
	}
	if (cpio6flag) {
		/* UNIX V6 compat */
		errmsgno(EX_BAD, "Unsupported option -6.\n");
		susage(EX_BAD);
	}
	if (sunxattr) {
		/* Extended File Attributes */
		errmsgno(EX_BAD, "Unsupported option -@.\n");
		susage(EX_BAD);
	}
	if (cpioVflag) {
		/* Special Verbose */
		errmsgno(EX_BAD, "Unsupported option -V.\n");
		susage(EX_BAD);
	}
	if (paxinteract)
		interactive = TRUE;

	nomtime = !cpiomflag;

	star_checkopts(/* oldtar */ FALSE, /* dodesc */ FALSE,
				/* usetape */ FALSE,
				/* archive */ -1, no_fifo,
				/* paxopts */ NULL,
				llbs);

	nolinkerr = FALSE;
}

LOCAL void
cpio_info()
{
	error("\nFor a more complete user interface use the tar type command interface.\n");
	error("See 'man star'. The %s command is more or less limited to the\n", get_progname());
	error("SUSv2 standard cpio command line interface.\n");
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
	error("\nUse\t%s -H help\n", get_progname());
	error("to get a list of valid archive header formats.\n");
	cpio_info();
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\t%s cmd [options] file1 ... filen\n", get_progname());
	error("Cmd:\n");
	error("\t-o\t\tCopy out (write files to an archive)\n");
	error("\t-i\t\tCopy in (extract files from archive)\n");
	error("\t-it\t\tList (files from archive)\n");
	error("\t-p\t\tPass (copy files to different location)\n");
	error("Options:\n");
	error("\t-help\t\t(*) print this help\n");
	error("\t-xhelp\t\t(*) print extended help\n");
	error("\t-version\t(*) print version information and exit\n");
	error("\t-a\t\treset access time after storing file\n");
	error("\t-A\t\t(+) append to an existing archive\n");
	error("\t-b\t\t(+) swap bytes and halfwords\n");
	error("\t-B\t\tblock input/output to 5120 bytes records\n");
	error("\t-c\t\twrite/read header information in character form\n");
	error("\t-C #\t\t(+) set (input/output) block size to #\n");
	error("\t-d\t\tcreate directories as needed\n");
	error("\t-E name\t\t(+) read filenames from named file\n");
	error("\t-f\t\tinvert matching rules\n");
	error("\t-H header\t(+) generate 'header' type archive (see -H help)\n");
	error("\tartype=header\t(*) generate 'header' type archive (see artype=help)\n");
	error("\t-I nm\t\t(+) use 'nm' as tape instead of stdin/stdout\n");
	error("\t-k\t\t(+) inore errors - skip corrupt data\n");
	error("\t-l\t\tlink files rather than copying them\n");
	error("\t-L\t\t(+) follow symbolic links as if they were files\n");
	error("\t-m\t\trestore access and modification time\n");
	error("\t-M message\t(+) message to use when switching media\n");
	error("\t-O nm\t\t(+) use 'nm' as tape instead of stdin/stdout\n");
	error("\t-P\t\t(+) handle access control lists\n");
	error("\t-r\t\tdo interactive creation/extraction/renaming\n");
	error("\t-R nm\t\t(+) reassign ownership and group information\n");
	error("\t-s\t\t(+) swap bytes\n");
	error("\t-S\t\t(+) swap halfwords\n");
	error("\t-t\t\twrite a table of contents\n");
	error("\t-u\t\trestore files unconditionally\n");
	error("\t-v\t\tincrement verbose level\n");
	error("\t-V\t\t(+) special verbose\n");
	error("\t-6\t\t(+) UNIX V6 binary cpio compatibility\n");
	error("\t-@\t\t(+) manage extended file attributes\n");
	error("\t-z\t\t(*) pipe input/output through gzip, does not work on tapes\n");
	error("\t-bz\t\t(*) pipe input/output through bzip2, does not work on tapes\n");
	error("\t-lzo\t\t(*) pipe input/output through lzop, does not work on tapes\n");
	error("\t-7z\t\t(*) pipe input/output through p7zip, does not work on tapes\n");
	error("\t-xz\t\t(*) pipe input/output through xp, does not work on tapes\n");
	error("\t-lzip\t\t(*) pipe input/output through lzip, does not work on tapes\n");
	error("\t-zstd\t\t(*) pipe input/output through zstd, does not work on tapes\n");
#ifdef	FIFO
	error("\t-no-fifo\t(*) don't use a fifo to optimize data flow from/to tape\n");
#endif
	error("\nAll options marked with (*) are neither defined by SUSv2 nor by SVr4.\n");
	error("All options marked with (+) are SVr4 extensions.\n");
	error("\nThe -c option is compatible to SVr4 cpio and thus is equivalent to -Hasc.\n");
	error("If you like the archive type from SUSv2 cpio -c, use -Hcpio instead.\n");
	cpio_info();
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
	error("\t-secure-links\tdon't extract links that start with '/' or contain '..' (default)\n");
	error("\t-no-secure-links\textract links that start with '/' or contain '..'\n");
	error("\t-acl\t\thandle access control lists\n");
	error("\t-xfflags\thandle extended file flags\n");
	error("\tbs=#\t\tset (output) block size to #\n");
#ifdef	FIFO
	error("\tfs=#\t\tset fifo size to #\n");
#endif
	error("\t-no-fsync\tdo not call fsync() for each extracted file (may be dangerous)\n");
	error("\t-do-fsync\tcall fsync() for each extracted file\n");
	error("\t-time\t\tprint timing info\n");
	error("\t-no-statistics\tdo not print statistics\n");
#ifdef	FIFO
	error("\t-fifostats\tprint fifo statistics\n");
#endif
	error("\t-numeric\tdon't use user/group name from tape\n");
	error("\nAll options above are not defined by SUSv2 nor by SVr4.\n");
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
cpio_setopts(o)
	char	*o;
{
extern	char	*opts;
	opts = o;
}
#endif
