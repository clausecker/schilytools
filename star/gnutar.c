/* @(#)gnutar.c	1.28 17/09/20 Copyright 1989, 2003-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char _g_sccsid[] =
	"@(#)gnutar.c	1.28 17/09/20 Copyright 1989, 2003-2017 J. Schilling";
#endif
/*
 *	GNU TAR specific routines for star main program.
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

static char *gnu_copt = "HcrtuxdAWkUSOGgmpsfMLFbiBVojzZCTXPhlKNvRw01234567";

#ifdef	STAR_MAIN
/*
 * We have been included from star.c
 */
#define	opts		gnutar_opts
#define	_opts		_gnutar_opts
#define	gargs		gnutar_gargs
#define	susage		gnutar_susage
#define	usage		gnutar_usage
#define	xusage		gnutar_xusage
#else
#define	NO_STAR_MAIN
#define	GNUTAR_MAIN
#define	PTYPE_DEFAULT	P_GNUTAR

#include "star.c"
#endif

#define	GNU_TAR

LOCAL	void	gargs		__PR((int ac, char *const *av));
LOCAL	void	susage		__PR((int ret));
LOCAL	void	usage		__PR((int ret));
LOCAL	void	xusage		__PR((int ret));
#ifdef	STAR_MAIN
LOCAL	void	gnutar_setopts	__PR((char *o));
#endif

/*
 * Main operation mode:
 *   -t, --list              list the contents of an archive
 *   -x, --extract, --get    extract files from an archive
 *   -c, --create            create a new archive
 *   -d, --diff, --compare   find differences between archive and file system
 *   -r, --append            append files to the end of an archive
 *   -u, --update            only append files newer than copy in archive
 *   -A, --catenate          append tar files to an archive
 *       --concatenate       same as -A
 *       --delete            delete from the archive (not on mag tapes!)
 *
 * Operation modifiers:
 *   -W, --verify               attempt to verify the archive after writing it
 *       --remove-files         remove files after adding them to the archive
 *   -k, --keep-old-files       don't replace existing files when extracting
 *       --overwrite            overwrite existing files when extracting
 *       --overwrite-dir        overwrite directory metadata when extracting
 *   -U, --unlink-first         remove each file prior to extracting over it
 *       --recursive-unlink     empty hierarchies prior to extracting directory
 *   -S, --sparse               handle sparse files efficiently
 *   -O, --to-stdout            extract files to standard output
 *   -G, --incremental          handle old GNU-format incremental backup
 *   -g, --listed-incremental=FILE
 *                              handle new GNU-format incremental backup
 *       --ignore-failed-read   do not exit with nonzero on unreadable files
 *
 * Handling of file attributes:
 *       --owner=NAME             force NAME as owner for added files
 *       --group=NAME             force NAME as group for added files
 *       --mode=CHANGES           force (symbolic) mode CHANGES for added files
 *       --atime-preserve         don't change access times on dumped files
 *   -m, --modification-time      don't extract file modified time
 *       --same-owner             try extracting files with the same ownership
 *       --no-same-owner          extract files as yourself
 *       --numeric-owner          always use numbers for user/group names
 *   -p, --same-permissions       extract permissions information
 *       --no-same-permissions    do not extract permissions information
 *       --preserve-permissions   same as -p
 *   -s, --same-order             sort names to extract to match archive
 *       --preserve-order         same as -s
 *       --preserve               same as both -p and -s
 *
 * Device selection and switching:
 *   -f, --file=ARCHIVE             use archive file or device ARCHIVE
 *       --force-local              archive file is local even if has a colon
 *       --rsh-command=COMMAND      use remote COMMAND instead of rsh
 *   -[0-7][lmh]                    specify drive and density
 *   -M, --multi-volume             create/list/extract multi-volume archive
 *   -L, --tape-length=NUM          change tape after writing NUM x 1024 bytes
 *   -F, --info-script=FILE         run script at end of each tape (implies -M)
 *       --new-volume-script=FILE   same as -F FILE
 *       --volno-file=FILE          use/update the volume number in FILE
 *
 * Device blocking:
 *   -b, --blocking-factor=BLOCKS   BLOCKS x 512 bytes per record
 *       --record-size=SIZE         SIZE bytes per record, multiple of 512
 *   -i, --ignore-zeros             ignore zeroed blocks in archive (means EOF)
 *   -B, --read-full-records        reblock as we read (for 4.2BSD pipes)
 *
 * Archive format selection:
 *
 *   -H, --format=FORMAT                create archive of the given format
 *
 *   -V, --label=NAME                   create archive with volume name NAME
 *               PATTERN                at list/extract time, a globbing PATTERN
 *   -o, --old-archive, --portability   write a V7 format archive
 *       --posix                        write a POSIX format archive
 *   -j, --bzip2                        filter the archive through bzip2
 *   -z, --gzip, --ungzip               filter the archive through gzip
 *   -Z, --compress, --uncompress       filter the archive through compress
 *       --use-compress-program=PROG    filter through PROG (must accept -d)
 *
 * Local file selection:
 *   -C, --directory=DIR          change to directory DIR
 *   -T, --files-from=NAME        get names to extract or create from file NAME
 *       --null                   -T reads null-terminated names, disable -C
 *       --exclude=PATTERN        exclude files, given as a PATTERN
 *   -X, --exclude-from=FILE      exclude patterns listed in FILE
 *       --anchored               exclude patterns match file name start (default)
 *       --no-anchored            exclude patterns match after any /
 *       --ignore-case            exclusion ignores case
 *       --no-ignore-case         exclusion is case sensitive (default)
 *       --wildcards              exclude patterns use wildcards (default)
 *       --no-wildcards           exclude patterns are plain strings
 *       --wildcards-match-slash  exclude pattern wildcards match '/' (default)
 *       --no-wildcards-match-slash exclude pattern wildcards do not match '/'
 *   -P, --absolute-names         don't strip leading `/'s from file names
 *   -h, --dereference            dump instead the files symlinks point to
 *       --no-recursion           avoid descending automatically in directories
 *   -l, --one-file-system        stay in local file system when creating archive
 *   -K, --starting-file=NAME     begin at file NAME in the archive
 *   -N, --newer=DATE             only store files newer than DATE
 *       --newer-mtime=DATE       compare date and time when data changed only
 *       --after-date=DATE        same as -N
 *       --backup[=CONTROL]       backup before removal, choose version control
 *       --suffix=SUFFIX          backup before removal, override usual suffix
 *
 * Informative output:
 *       --help            print this help, then exit
 *       --version         print tar program version number, then exit
 *   -v, --verbose         verbosely list files processed
 *       --checkpoint      print directory names while reading the archive
 *       --totals          print total bytes written while creating archive
 *   -R, --block-number    show block number within archive with each message
 *   -w, --interactive     ask for confirmation for every action
 *       --confirmation    same as -w
 */

/*
 * GNU TAR related options
 */
/* BEGIN CSTYLED */
char	_opts[] = "C*,help,xhelp,version,debug,xdebug#,xd#,time,no-statistics,do-statistics,fifostats,numeric,no-fifo,no-fsync,bs&,fs&,/,..,secure-links,acl,xfflags,copy,diff,H&,format&,z,bz,create,c,append,r,list,t,update,u,extract,get,x,compare,d,catenate,concatenate,A,delete,verify,W,remove-files,keep-old-files,k,unlink-first,U,recursive-unlink,sparse,S,to-stdout,O,incremental,G,listed-incremental*,g*,ignore-failed-read,owner*,group*,mode*,atime-preserve,modification-time,m,same-owner,no-same-owner,numeric-owner,same-permissions,preserve-permissions,p,no-same-permissions,same-order,preserve,preserve-order,s,force-local,file&,f&,rsh-command*,multi-volume,M,tape-length&,L&,new-volume-script*,info-script*,F*,volno-file*,blocking-factor&,b&,record-size&,ignore-zeros,i,read-full-records,B,label*,V*,old-archive,portability,o,posix,bzip2,j,gzip,ungzip,compress,uncompress,Z,use-compress-program*,files-from*,T*,null,exclude&,exclude-from&,X&,anchored,no-anchored,ignore-case,no-ignore-case,wildcards,no-wildcards,wildcards-match-slash,no-wildcards-match-slash,absolute-names,P,dereference,h,no-recursion,one-file-system,l,starting-file*,K*,newer*,after-date*,N*,newer-mtime*,backup*,suffix*,v+,verbose+,checkpoint,totals,block-number,R,interactive,confirmation,w,?";
/* END CSTYLED */
char	*opts = _opts;
struct ga_props	gaprops;

LOCAL	void	gnutar_info	__PR((void));

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
	BOOL	gnuconcat = FALSE;
	BOOL	gnudel	  = FALSE;
	BOOL	gnuveri	  = FALSE;
	BOOL	gnuremove = FALSE;
	BOOL	gnuincrem = FALSE;
	char	*gnulinc  = NULL;
	BOOL	gnuignfail = FALSE;
	char	*gnuowner  = NULL;
	char	*gnugroup  = NULL;
	char	*gnumode   = NULL;
	BOOL	gnuchown = FALSE;
	BOOL	gnuorder = FALSE;
	char	*rsh_command = NULL;
	char	*gnuvolno_file = NULL;
	BOOL	gnuposix = FALSE;
	BOOL	gnuanchored = FALSE;
	BOOL	gnuno_anchored = FALSE;
	BOOL	gnuignorecase = FALSE;
	BOOL	gnuno_ignorecase = FALSE;
	BOOL	gnuwildchards = FALSE;
	BOOL	gnuno_wildchards = FALSE;
	BOOL	gnumatchslash = FALSE;
	BOOL	gnuno_matchslash = FALSE;
	char	*gnustartf = NULL;
	char	*gnunewer = NULL;
	char	*gnunewermt = NULL;
	char	*gnubackup = NULL;
	char	*gnusuffix = NULL;
	BOOL	gnucheckpoint = FALSE;
	BOOL	gnutotals = FALSE;
	BOOL	do_stats = FALSE;
	Llong	llbs	 = 0;
signed	char	archive	 = -1;		/* On IRIX, we have unsigned chars by default */

	/*
	 * Current default archive format in all other cases is USTAR.
	 * We may change this to PAX in the future.
	 */
	hdrtype = H_USTAR;
#ifdef	STAR_MAIN
	gnutar_setopts(opts);			/* set up opts for getfiles */
#endif
	getarginit(&gaprops, GAF_DEFAULT);	/* Set default behavior	  */

	iftype		= I_TAR;		/* command line interface */
	ptype		= P_GNUTAR;		/* program interface type */
	bsdchdir	= TRUE;
	no_dirslash	= FALSE;
	no_stats	= TRUE;			/* and -no-statitstics	   */

	--ac, ++av;
	files = getfilecount(ac, av, opts);
	if (getlallargs(&ac, &av, &gaprops, opts,
				&dir_flags,
				&help, &xhelp, &prvers, &debug, &xdebug, &xdebug,
#ifndef	__old__lint
				&showtime, &no_stats, &do_stats, &do_fifostats,
				&numeric,  &no_fifo, &no_fsync,
				getnum, &bs,
				getnum, &fs,
				&abs_path, &allow_dotdot, &secure_links,
				&doacl, &dofflags,
				&copyflag, &diff_flag,
				gethdr, &chdrtype, gethdr, &chdrtype,
				&zflag, &bzflag,

				&cflag, &cflag,
				&rflag, &rflag,
				&tflag, &tflag,
				&uflag, &uflag,
				&xflag, &xflag, &xflag,
				&diff_flag, &diff_flag,
				&gnuconcat, &gnuconcat, &gnuconcat,
				&gnudel,

				&gnuveri, &gnuveri,
				&gnuremove,
				&keep_old, &keep_old,
/*				--overwrite */
/*				--overwrite-dir / --nooverwrite-dir */
				&remove_first, &remove_first,
				&remove_recursive,
				&sparse, &sparse,
				&to_stdout, &to_stdout,
				&gnuincrem, &gnuincrem,
				&gnulinc, &gnulinc,
				&gnuignfail,
				&gnuowner, &gnugroup, &gnumode,
				&acctime,
				&nomtime, &nomtime,
				&gnuchown,
				&nochown,
				&numeric,
				&pflag, &pflag, &pflag,
				&nopflag,
				&gnuorder, &gnuorder, &gnuorder, &gnuorder,

				&force_noremote,
				addtarfile, NULL,	/* -file archive */
				addtarfile, NULL,	/* -f archive */
				&rsh_command,

				&multivol, &multivol,
				getknum, &tsize,
				getknum, &tsize,
				&newvol_script, &newvol_script, &newvol_script,
				&gnuvolno_file,

				getnum, &bs,		/* -b blocks */
				getnum, &bs,		/* -b blocks */
				getllnum, &llbs,
				&ignoreerr, &ignoreerr,
				&multblk, &multblk,
				&volhdr, &volhdr,
				&oldtar, &oldtar, &oldtar,
				&gnuposix,
				&bzflag, &bzflag,
				&zflag, &zflag,
				&Zflag, &Zflag, &Zflag,
				&compress_prg,
				&listfile, &listfile,
				&readnull,
				addpattern, NULL,
				getexclude, NULL,
				getexclude, NULL,
				&gnuanchored,
				&gnuno_anchored,
				&gnuignorecase,
				&gnuno_ignorecase,
				&gnuwildchards,
				&gnuno_wildchards,
				&gnumatchslash,
				&gnuno_matchslash,
				&abs_path, &abs_path,
				&follow, &follow,
				&nodesc,
				&nomount, &nomount,
				&gnustartf, &gnustartf,
				&gnunewer, &gnunewer, &gnunewer,
				&gnunewermt,
				&gnubackup,
				&gnusuffix,

				&verbose, &verbose,
				&gnucheckpoint,
				&gnutotals,
				&prblockno, &prblockno,
				&interactive, &interactive, &interactive,
#endif /* __old__lint */
				&archive) < 0) {
		errmsgno(EX_BAD, "Bad Option: %s.\n", av[0]);
		susage(EX_BAD);
	}
	if (archive != -1 && !(archive >= '0' && archive <= '7')) {
		errmsgno(EX_BAD, "Bad Option: -%c.\n", archive);
		susage(EX_BAD);
	}
	star_helpvers("gnutar", help, xhelp, prvers);

	if (gnuconcat) {
		errmsgno(EX_BAD, "The --concatenate option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnudel) {
		errmsgno(EX_BAD, "The --delete option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuveri) {
		errmsgno(EX_BAD, "The --verify option is not implemented.\n");
		errmsgno(EX_BAD, "Use the rewind and -diff from star instead.\n");
		susage(EX_BAD);
	}
	if (gnuremove) {
		errmsgno(EX_BAD, "The --remove-files option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuincrem) {
		errmsgno(EX_BAD, "The --incremental option is not implemented.\n");
		errmsgno(EX_BAD, "Use the true incremental dump feature from star instead.\n");
		susage(EX_BAD);
	}
	if (gnulinc) {
		errmsgno(EX_BAD, "The --listed-incremental option is not implemented.\n");
		errmsgno(EX_BAD, "Use the true incremental dump feature from star instead.\n");
		susage(EX_BAD);
	}
	if (gnuignfail) {
		errmsgno(EX_BAD, "The --ignore-failed-read option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuowner) {
		errmsgno(EX_BAD, "The --owner option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnugroup) {
		errmsgno(EX_BAD, "The --group option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnumode) {
		errmsgno(EX_BAD, "The --mode option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuchown)
		nochown = FALSE;
	if (gnuorder) {
		errmsgno(EX_BAD, "The --same-order option is completely useless.\n");
		errmsgno(EX_BAD, "It is not needed as star uses hash tables.\n");
		susage(EX_BAD);
	}
	if (rsh_command) {
		int	len = strlen(rsh_command) + 4;	/* + "RSH=" */
		char	*p;

		p = ___malloc(len, "putenv");
		strcatl(p, "RSH=", rsh_command, (char *)NULL);
#ifdef	HAVE_PUTENV
		putenv(p);
#else
		comerrno(EX_BAD, "No putenv() on this platform, cannot use --rsh-command.\n");
#endif
	}
	if (gnuvolno_file) {
		errmsgno(EX_BAD, "The --volno-file option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuposix) {
		errmsgno(EX_BAD, "The --posix option is outdated - ignored.\n");
		errmsgno(EX_BAD, "Star is posix by default.\n");
		errmsgno(EX_BAD, "If you like to create other archive types, use -Htype from star instead.\n");
	}
	if (gnuanchored) {
		errmsgno(EX_BAD, "The --anchored option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuno_anchored) {
		errmsgno(EX_BAD, "The --no-anchored option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuignorecase) {
		errmsgno(EX_BAD, "The --ignore-case option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuno_ignorecase) {
		errmsgno(EX_BAD, "The --no-ignore-case option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuwildchards) {
		errmsgno(EX_BAD, "The --wildcards option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuno_wildchards) {
		errmsgno(EX_BAD, "The --no-wildcards option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnumatchslash) {
		errmsgno(EX_BAD, "The --wildcards-match-slash option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnuno_matchslash) {
		errmsgno(EX_BAD, "The --no-wildcards-match-slash option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnustartf) {
		errmsgno(EX_BAD, "The --starting-file option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnunewer) {
		errmsgno(EX_BAD, "The --newer option is not implemented.\n");
		errmsgno(EX_BAD, "Use the -newer option from star with -ctime instead.\n");
		susage(EX_BAD);
	}
	if (gnunewermt) {
		errmsgno(EX_BAD, "The --newer-mtime option is not implemented.\n");
		errmsgno(EX_BAD, "Use the -newer option from star without -ctime instead.\n");
		susage(EX_BAD);
	}
	if (gnubackup) {
		errmsgno(EX_BAD, "The --backup option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnusuffix) {
		errmsgno(EX_BAD, "The --suffix option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnucheckpoint) {
		errmsgno(EX_BAD, "The --checkpoint option is not yet implemented.\n");
		susage(EX_BAD);
	}
	if (gnutotals)
		showtime = TRUE;
	if (do_stats)
		no_stats = FALSE;

	notpat = TRUE;	/* GNU tar only supports exclude patterns */

	star_checkopts(oldtar, /* dodesc */ FALSE, /* usetape */ TRUE,
					archive, no_fifo, llbs);
	star_nfiles(files, minfiles);

	if ((cflag || xflag) && verbose == 1)
		tpath = TRUE;
	/*
	 * Strange GNU tar semantics:
	 * 	tar -cf /dev/null . equals star -cf -nullout .
	 */
	if (cflag && archisnull(tarfiles[0]))
		nullout = TRUE;
}

LOCAL void
gnutar_info()
{
	error("\nFor a more complete user interface use the star type command interface.\n");
	error("See 'man star'. The %s command is more or less limited to the\n", get_progname());
	error("GNU tar command line interface.\n");
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
	error("\nUse\t%s -H help\n", get_progname());
	error("to get a list of valid archive header formats.\n");
	gnutar_info();
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
	error("\t--diff\t\tdiff archive against file system (see -xhelp)\n");
	error("Options:\n");
	error("\t--help\t\t(*) print this help\n");
	error("\t--xhelp\t\tprint extended help\n");
	error("\t--version\tprint version information and exit\n");

	error("\tH=header\tgenerate 'header' type archive (see H=help)\n");
	error("\t-z\t\tpipe input/output through gzip, does not work on tapes\n");
	error("\t--bz\t\t(*) pipe input/output through bzip2, does not work on tapes\n");
#ifdef	FIFO
	error("\t--no-fifo\t(*) don't use a fifo to optimize data flow from/to tape\n");
#endif
	error("\nAll options marked with (*) are not defined by GNU tar.\n");

	/* BEGIN CSTYLED */
	error("\n\
GNU tar help:\n\
Main operation mode:\n\
  -t, --list              list the contents of an archive\n\
  -x, --extract, --get    extract files from an archive\n\
  -c, --create            create a new archive\n\
  -d, --diff, --compare   find differences between archive and file system\n\
  -r, --append            append files to the end of an archive\n\
  -u, --update            only append files newer than copy in archive\n\
  -A, --catenate          append tar files to an archive\n\
      --concatenate       same as -A\n\
      --delete            delete from the archive (not on mag tapes!)\n");

	error("\n\
Operation modifiers:\n\
  -W, --verify               attempt to verify the archive after writing it\n\
      --remove-files         remove files after adding them to the archive\n\
  -k, --keep-old-files       don't replace existing files when extracting\n\
      --overwrite            overwrite existing files when extracting\n\
      --overwrite-dir        overwrite directory metadata when extracting\n\
  -U, --unlink-first         remove each file prior to extracting over it\n\
      --recursive-unlink     empty hierarchies prior to extracting directory\n\
  -S, --sparse               handle sparse files efficiently\n\
  -O, --to-stdout            extract files to standard output\n\
  -G, --incremental          handle old GNU-format incremental backup\n\
  -g, --listed-incremental=FILE\n\
                             handle new GNU-format incremental backup\n\
      --ignore-failed-read   do not exit with nonzero on unreadable files\n");

	error("\n\
Handling of file attributes:\n\
      --owner=NAME             force NAME as owner for added files\n\
      --group=NAME             force NAME as group for added files\n\
      --mode=CHANGES           force (symbolic) mode CHANGES for added files\n\
      --atime-preserve         don't change access times on dumped files\n\
  -m, --modification-time      don't extract file modified time\n\
      --same-owner             try extracting files with the same ownership\n\
      --no-same-owner          extract files as yourself\n\
      --numeric-owner          always use numbers for user/group names\n\
  -p, --same-permissions       extract permissions information\n\
      --no-same-permissions    do not extract permissions information\n\
      --preserve-permissions   same as -p\n\
  -s, --same-order             sort names to extract to match archive\n\
      --preserve-order         same as -s\n\
      --preserve               same as both -p and -s\n");

	error("\n\
Device selection and switching:\n\
  -f, --file=ARCHIVE             use archive file or device ARCHIVE\n\
      --force-local              archive file is local even if has a colon\n\
      --rsh-command=COMMAND      use remote COMMAND instead of rsh\n\
  -[0-7][lmh]                    specify drive and density\n\
  -M, --multi-volume             create/list/extract multi-volume archive\n\
  -L, --tape-length=NUM          change tape after writing NUM x 1024 bytes\n\
  -F, --info-script=FILE         run script at end of each tape (implies -M)\n\
      --new-volume-script=FILE   same as -F FILE\n\
      --volno-file=FILE          use/update the volume number in FILE\n");

	error("\n\
Device blocking:\n\
  -b, --blocking-factor=BLOCKS   BLOCKS x 512 bytes per record\n\
      --record-size=SIZE         SIZE bytes per record, multiple of 512\n\
  -i, --ignore-zeros             ignore zeroed blocks in archive (means EOF)\n\
  -B, --read-full-records        reblock as we read (for 4.2BSD pipes)\n");

	error("\n\
Archive format selection:\n\
  -H, --format=FORMAT                create archive of the given format\n\
  -V, --label=NAME                   create archive with volume name NAME\n\
              PATTERN                at list/extract time, a globbing PATTERN\n\
  -o, --old-archive, --portability   write a V7 format archive\n\
      --posix                        write a POSIX format archive\n\
  -j, --bzip2                        filter the archive through bzip2\n\
  -z, --gzip, --ungzip               filter the archive through gzip\n\
  -Z, --compress, --uncompress       filter the archive through compress\n\
      --use-compress-program=PROG    filter through PROG (must accept -d)\n");

	error("\n\
Local file selection:\n\
  -C, --directory=DIR          change to directory DIR\n\
  -T, --files-from=NAME        get names to extract or create from file NAME\n\
      --null                   -T reads null-terminated names, disable -C\n\
      --exclude=PATTERN        exclude files, given as a PATTERN\n\
  -X, --exclude-from=FILE      exclude patterns listed in FILE\n\
      --anchored               exclude patterns match file name start (default)\n\
      --no-anchored            exclude patterns match after any /\n\
      --ignore-case            exclusion ignores case\n\
      --no-ignore-case         exclusion is case sensitive (default)\n\
      --wildcards              exclude patterns use wildcards (default)\n\
      --no-wildcards           exclude patterns are plain strings\n\
      --wildcards-match-slash  exclude pattern wildcards match '/' (default)\n\
      --no-wildcards-match-slash exclude pattern wildcards do not match '/'\n\
  -P, --absolute-names         don't strip leading `/'s from file names\n\
  -h, --dereference            dump instead the files symlinks point to\n\
      --no-recursion           avoid descending automatically in directories\n\
  -l, --one-file-system        stay in local file system when creating archive\n\
  -K, --starting-file=NAME     begin at file NAME in the archive\n");

	error("\
  -N, --newer=DATE             only store files newer than DATE\n\
      --newer-mtime=DATE       compare date and time when data changed only\n\
      --after-date=DATE        same as -N\n");

	error("\
      --backup[=CONTROL]       backup before removal, choose version control\n\
      --suffix=SUFFIX          backup before removal, override usual suffix\n");

	error("\n\
Informative output:\n\
      --help            print this help, then exit\n\
      --version         print tar program version number, then exit\n\
  -v, --verbose         verbosely list files processed\n\
      --checkpoint      print directory names while reading the archive\n\
      --totals          print total bytes written while creating archive\n\
  -R, --block-number    show block number within archive with each message\n\
  -w, --interactive     ask for confirmation for every action\n\
      --confirmation    same as -w\n");
	/* END CSTYLED */

	gnutar_info();
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
	error("\t-..\t\tdon't skip filenames that contain '..' in non-interactive extract\n");
	error("\t--secure-links\tdon't extract links that start with '/' or contain '..'\n");
	error("\t--acl\t\thandle access control lists\n");
	error("\t--xfflags\thandle extended file flags\n");
	error("\tbs=#\t\tset (output) block size to #\n");
#ifdef	FIFO
	error("\tfs=#\t\tset fifo size to #\n");
#endif
	error("\t--no-fsync\tdo not call fsync() for each extracted file (may be dangerous)\n");
	error("\t--time\t\tprint timing info\n");
	error("\t--no-statistics\tdo not print statistics\n");
	error("\t--do-statistics\tprint statistics\n");
#ifdef	FIFO
	error("\t--fifostats\tprint fifo statistics\n");
#endif
	error("\t--numeric\tdon't use user/group name from tape\n");
	error("\nAll options above are not defined by GNU tar.\n");
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
gnutar_setopts(o)
	char	*o;
{
extern	char	*opts;
	opts = o;
}
#endif
