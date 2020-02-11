/* @(#)star.c	1.403 20/02/05 Copyright 1985, 88-90, 92-96, 98, 99, 2000-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)star.c	1.403 20/02/05 Copyright 1985, 88-90, 92-96, 98, 99, 2000-2020 J. Schilling";
#endif
/*
 *	Copyright (c) 1985, 88-90, 92-96, 98, 99, 2000-2020 J. Schilling
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

#define	STAR_MAIN

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/signal.h>
#include <schily/string.h>
#include "star.h"
#include "props.h"
#include "diff.h"
#include <schily/wait.h>
#include <schily/standard.h>
#define	__XDEV__		/* Needed to activate _dev_init() */
#include <schily/device.h>
#include <schily/fcntl.h>	/* Needed for O_XATTR */
#include <schily/stat.h>	/* Needed for umask(2) */
#include <schily/getargs.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/idcache.h>
#include "fifo.h"		/* Needed for #undef FIFO */
#include "dumpdate.h"
#ifdef	USE_FIND
#include <schily/walk.h>
#include <schily/find.h>
#endif

#include <schily/nlsdefs.h>

#include "starsubs.h"
#include "dirtime.h"
#include "checkerr.h"

EXPORT	int	main		__PR((int ac, char **av));
LOCAL	void	star_create	__PR((int ac, char *const *av));
LOCAL	void	checkdumptype	__PR((GINFO *gp));
LOCAL	void	init_ddate	__PR((char *name));
EXPORT	void	copy_create	__PR((int ac, char *const *av));
LOCAL	int	getfilecount	__PR((int ac, char *const *av, const char *fmt));
LOCAL	void	getdir		__PR((int *acp, char *const **avp,
						const char **dirp));
LOCAL	void	openlist	__PR((void));
LOCAL	void	check_stdin	__PR((char *name));
LOCAL	void	susage		__PR((int ret));
LOCAL	void	usage		__PR((int ret));
LOCAL	void	xusage		__PR((int ret));
LOCAL	void	dusage		__PR((int ret));
LOCAL	void	husage		__PR((int ret));
LOCAL	void	gargs		__PR((int ac, char *const *av));
LOCAL	void	star_mkvers	__PR((void));
LOCAL	void	star_helpvers	__PR((char *name, BOOL help, BOOL xhelp, BOOL prvers));
LOCAL	void	star_checkopts	__PR((BOOL oldtar, BOOL dodesc, BOOL usetape,
					int archive, BOOL no_fifo,
					const char *paxopts,
					Llong llbs));
EXPORT	void	star_verifyopts	__PR((void));
LOCAL	void	star_nfiles	__PR((int files, int minfiles));
LOCAL	int	getpaxH		__PR((char *arg, long *valp, int *pac, char *const **pav));
LOCAL	int	getpaxL		__PR((char *arg, long *valp, int *pac, char *const **pav));
LOCAL	int	getpaxP		__PR((char *arg, long *valp, int *pac, char *const **pav));
LOCAL	int	getfind		__PR((char *arg, long *valp, int *pac, char *const **pav));
LOCAL	int	getpaxpriv	__PR((char *arg, long *valp));
LOCAL	int	getlldefault	__PR((char *arg, Llong *valp, int mult));
EXPORT	int	getbnum		__PR((char *arg, Llong *valp));
EXPORT	int	getknum		__PR((char *arg, Llong *valp));
EXPORT	int	getknum		__PR((char *arg, Llong *valp));
LOCAL	int	getenum		__PR((char *arg, long *valp));
LOCAL	int	addtarfile	__PR((const char *tarfile));
LOCAL	int	add_diffopt	__PR((char *optstr, long *flagp));
LOCAL	int	gethdr		__PR((char *optstr, long *typep));
LOCAL	int	getexclude	__PR((char *arg, long *valp, int *pac, char *const **pav));
#ifdef	USED
LOCAL	int	addfile		__PR((char *optstr, long *dummy));
#endif
EXPORT	void	set_signal	__PR((int sig, RETSIGTYPE (*handler)(int)));
LOCAL	void	exsig		__PR((int sig));
LOCAL	void	sighup		__PR((int sig));
LOCAL	void	sigintr		__PR((int sig));
LOCAL	void	sigquit		__PR((int sig));
LOCAL	void	getstamp	__PR((void));
LOCAL	const char *has_cli	__PR((int ac, char *const *av));
LOCAL	int	get_ptype	__PR((const char *p));
LOCAL	void	set_ptype	__PR((int *pac, char *const **pav));
LOCAL	void	docompat	__PR((int *pac, char *const **pav));
EXPORT	BOOL	ttyerr		__PR((FILE *f));

#if	defined(SIGDEFER) || defined(SVR4)
#define	signal	sigset
#endif

#define	QIC_24_TSIZE	122880		/*  61440 kBytes */
#define	QIC_120_TSIZE	256000		/* 128000 kBytes */
#define	QIC_150_TSIZE	307200		/* 153600 kBytes */
#define	QIC_250_TSIZE	512000		/* 256000 kBytes (XXX not verified) */
#define	QIC_525_TSIZE	1025000		/* 512500 kBytes */
#define	TSIZE(s)	((s)*TBLOCK)

char	*vers;				/* the full version string	*/

struct star_stats	xstats;		/* for printing statistics	*/

extern	BOOL		havepat;	/* Pattern matching in use	*/

#define	NTARFILE	100		/* Max # of archive files	*/

FILE	*tarf;				/* The current archive		*/
FILE	*listf;				/* File for list= option	*/
FILE	*tty;				/* Open /dev/tty for questions	*/
FILE	*vpr;				/* File for verbose printing	*/
BOOL	did_stdin = FALSE;		/* Did use stdin for any option	*/
const	char	*tarfiles[NTARFILE];	/* Cycle list of all archives	*/
int	ntarfiles;			/* Number of entries in list	*/
int	tarfindex;			/* Current index in list	*/
char	*newvol_script;			/* -new-volume-script name	*/
BOOL	multivol = FALSE;		/* -multivol specified		*/
BOOL	force_noremote = FALSE;		/* -force-local specified	*/
char	*rmt;				/* -rmt specify remote server	*/
char	*rsh;				/* -rsh specify rsh command	*/
char	*listfile;			/* File name for list=		*/
BOOL	pkglist = FALSE;		/* pkglist= specified		*/
char	*stampfile;			/* Time stamp file for -newer	*/
BOOL	errflag;			/* -e for abort on error	*/
const	char	*wdir;			/* current working dir name	*/
const	char	*currdir;		/* current -C dir argument	*/
const	char	*dir_flags = NULL;	/* One/more -C options present	*/
BOOL	bsdchdir = FALSE;		/* -C only valid for next arg	*/
char	*volhdr;			/* VOLHDR= argument		*/
char	*fs_name;			/* fs-name= for snapshot fs	*/
char	*dd_name;			/* dumpdate= for snapshots	*/
dev_t	tape_dev;			/* st_dev for current archive	*/
ino_t	tape_ino;			/* st_ino for current archive	*/
BOOL	tape_isreg = FALSE;		/* Tape is a regular file	*/
#ifdef	FIFO
BOOL	use_fifo = TRUE;		/* Whether to use a FIFO or not	*/
#else
BOOL	use_fifo = FALSE;		/* Whether to use a FIFO or not	*/
#endif
BOOL	shmflag	= FALSE;		/* Whether to use shmem f. FIFO	*/
long	fs;				/* FIFO size			*/
long	bs;				/* TAPE block size (bytes)	*/
int	nblocks = 20;			/* TAPE blocks (512 byte units)	*/
BOOL	not_tape = FALSE;		/* -sun-n not a Tape		*/
uid_t	dir_uid = _BAD_UID;		/* -dir-owner			*/
gid_t	dir_gid = _BAD_GID;		/* -dir-group			*/
uid_t	my_uid;				/* Current euid			*/
dev_t	curfs = NODEV;			/* Current st_dev for -M option	*/
struct timespec	ddate;			/* The current dump date	*/
time_t	sixmonth;			/* 6 months before limit (ls)	*/
time_t	now;				/* now limit (ls)		*/
/*
 * Change default header format into XUSTAR in 2004 (see below in gargs())
 */
long	hdrtype	  = H_XSTAR;		/* default header format	*/
long	chdrtype  = H_UNDEF;		/* command line hdrtype		*/
int	cmptype	  = C_NONE;		/* compression type		*/
int	iftype	  = I_TAR;		/* command line interface type	*/
int	ptype	  = P_STAR;		/* program interface type	*/
const char *pname = NULL;		/* program name with cli=	*/
BOOL	paxls	  = FALSE;		/* create PAX type listing	*/
int	version	  = 0;			/* Version from POSIX TAR  hdr	*/
int	swapflg	  = -1;			/* Whether to swap input	*/
BOOL	debug	  = FALSE;		/* -debug has been specified	*/
int	xdebug	  = 0;			/* eXtended debug level		*/
int	dumplevel = -1;			/* level for incremental dumps	*/
int	oldlevel  = 0;			/* dumpleve this dump refers to	*/
BOOL	dump_partial = FALSE;		/* Dump is not a full dump	*/
BOOL	dump_cumulative = FALSE;	/* -cumulative has b. specified	*/
char	*dumpdates = "/etc/tardumps";	/* Database for increment. dump	*/
BOOL	wtardumps = FALSE;		/* Should update above file	*/
BOOL	print_artype = FALSE;
BOOL	showtime  = FALSE;		/* -time has been specified	*/
BOOL	no_stats  = FALSE;		/* -no-statistics specified	*/
BOOL	cpio_stats = FALSE;		/* -cpio-statistics specified	*/
BOOL	do_fifostats = FALSE;		/* -fifostats specified		*/
BOOL	numeric	  = FALSE;		/* -numeric user ids		*/
int	verbose   = 0;			/* -v has been specified	*/
BOOL	silent    = FALSE;		/* -silent no informal msg	*/
BOOL	prblockno = FALSE;		/* -block-number for all files	*/
BOOL	no_xheader = FALSE;		/* -no-xheader ignore P.2001	*/
BOOL	no_fsync  = -1;			/* -no-fsync on extract		*/
BOOL	readnull  = FALSE;		/* -read0 on with list=		*/
BOOL	tpath	  = FALSE;		/* -tpath print path only	*/
BOOL	cflag	  = FALSE;		/* -c has been specified	*/
BOOL	uflag	  = FALSE;		/* -u has been specified	*/
BOOL	rflag	  = FALSE;		/* -r has been specified	*/
BOOL	xflag	  = FALSE;		/* -x has been specified	*/
BOOL	tflag	  = FALSE;		/* -t has been specified	*/
BOOL	copyflag  = FALSE;		/* -copy has been specified	*/
BOOL	binflag   = FALSE;		/* -o binary has been specified	*/
BOOL	nflag	  = FALSE;		/* -n dummy extract mode	*/
BOOL	diff_flag = FALSE;		/* -diff has been specified	*/
BOOL	Zflag	  = FALSE;		/* -Z has been specified	*/
BOOL	zflag	  = FALSE;		/* -z has been specified	*/
BOOL	bzflag	  = FALSE;		/* -bz has been specified	*/
BOOL	lzoflag	  = FALSE;		/* -lzo has been specified	*/
BOOL	p7zflag	  = FALSE;		/* -7z has been specified	*/
BOOL	xzflag	  = FALSE;		/* -xz has been specified	*/
BOOL	lzipflag  = FALSE;		/* -lzip has been specified	*/
BOOL	zstdflag  = FALSE;		/* -zstd has been specified	*/
BOOL	lzmaflag  = FALSE;		/* -lzma has been specified	*/
BOOL	freezeflag  = FALSE;		/* -freeze has been specified	*/
char	*compress_prg = NULL;		/* -compress-program specified	*/
BOOL	multblk	  = FALSE;		/* -B has been specified	*/
BOOL	ignoreerr = FALSE;		/* -i has been specified	*/
BOOL	nodir	  = FALSE;		/* -d do not store dirs		*/
BOOL	noxdir	  = FALSE;		/* -d do not create dirs	*/
BOOL	noatime	  = FALSE;		/* -p a pax do not restore atime */
BOOL	nomtime	  = FALSE;		/* -m do not restore times	*/
BOOL	nochown	  = FALSE;		/* -o do not restore owner	*/
BOOL	acctime	  = FALSE;		/* -atime has been specified	*/
BOOL	pflag	  = FALSE;		/* -p restore permissions	*/
BOOL	nopflag	  = FALSE;		/* -no-p don't restore perms	*/
BOOL	dirmode	  = FALSE;		/* -dirmode wr. dirs past files	*/
BOOL	nolinkerr = FALSE;		/* pr. link # err depends on -l	*/
BOOL	follow	  = FALSE;		/* -h follow symbolic links	*/
BOOL	paxfollow = FALSE;		/* PAX -L follow symbolic links	*/
BOOL	paxHflag  = FALSE;		/* PAX -H follow symbolic links	*/
BOOL	nodesc	  = FALSE;		/* -D do not descenc dirs	*/
BOOL	nomount	  = FALSE;		/* -M do not cross mount points	*/
BOOL	interactive = FALSE;		/* -w has been specified	*/
BOOL	paxinteract = FALSE;		/* PAX -i has been specified	*/
BOOL	signedcksum = FALSE;		/* -signed-checksum		*/
BOOL	partial	  = FALSE;		/* -P write partial last record	*/
BOOL	nospec	  = FALSE;		/* -S no special files		*/
int	Fflag	  = 0;			/* -F,-FF,... no SCCS/RCS/...	*/
BOOL	uncond	  = FALSE;		/* -U unconditional extract	*/
BOOL	uncond_rename = FALSE;		/* -uncond-rename - ask always	*/
BOOL	xdir	  = FALSE;		/* -xdir uncond. dir extract	*/
BOOL	xdot	  = FALSE;		/* -xdot uncond '.' dir extract	*/
BOOL	keep_old  = FALSE;		/* -k do not overwrite files	*/
BOOL	refresh_old = FALSE;		/* -refresh existing only	*/
BOOL	abs_path  = FALSE;		/* -/ absolute path allowed	*/
BOOL	allow_dotdot = FALSE;		/* -.. '..' in path allowed	*/
BOOL	secure_links = -1;		/* -secure-links (no .. & /)	*/
BOOL	no_dirslash = FALSE;		/* -no-dirslash option		*/
BOOL	notpat	  = FALSE;		/* -not invert pattern matcher	*/
BOOL	match_tree = FALSE;		/* -match-tree match dir -> tree */
BOOL	notarg	  = FALSE;		/* PAX -c invert match		*/
BOOL	paxmatch  = FALSE;		/* Do PAX like matching		*/
BOOL	paxnflag  = FALSE;		/* PAX -n one match only	*/
BOOL	force_hole = FALSE;		/* -force-hole on extract	*/
BOOL	sparse	  = FALSE;		/* -sparse has been specified	*/
BOOL	to_stdout = FALSE;		/* -to-stdout extraction	*/
BOOL	wready    = FALSE;		/* -wready wait for ready tape	*/
BOOL	force_remove = FALSE;		/* -force-remove on extraction	*/
BOOL	ask_remove = FALSE;		/* -ask-remove on extraction	*/
BOOL	remove_first = FALSE;		/* -remove-first on extraction	*/
BOOL	remove_recursive = FALSE;	/* -remove-recursive on extract	*/
BOOL	keep_nonempty_dirs = FALSE;	/* -keep-nonempty-dirs on extract */
BOOL	do_install = FALSE;		/* -install on extract		*/
BOOL	nullout   = FALSE;		/* -onull - simulation write	*/
BOOL	prinodes  = FALSE;		/* -prinodes print ino # w. -tv */

Ullong	maxsize	  = 0;			/* max file size for create	*/
struct timespec	Newer = {0, 0};		/* Time stamp to compare with	*/
Ullong	tsize	  = 0;			/* Max tape size in tar blocks	*/
long	diffopts  = 0L;			/* diffopts= bit mask		*/
BOOL	nowarn	  = FALSE;		/* -nowarn has been specified	*/
BOOL	Ctime	  = FALSE;		/* -ctime has been specified	*/
BOOL	nodump	  = FALSE;		/* -nodump has been specified	*/

BOOL	listnew	  = FALSE;		/* -newest list newest only	*/
BOOL	listnewf  = FALSE;		/* -newest-file list n. plain f	*/
BOOL	hpdev	  = FALSE;		/* -hpdev non POSIX dev #	*/
BOOL	modebits  = FALSE;		/* -modebits more than 12 bits	*/
BOOL	copylinks = FALSE;		/* -copylinks rather than link	*/
BOOL	copyhardlinks = FALSE;		/* -copyhardlinks rather than link */
BOOL	copysymlinks = FALSE;		/* -copysymlinks rather than link */
BOOL	copydlinks = FALSE;		/* copy content of linked dirs	*/
BOOL	hardlinks = FALSE;		/* -hardlinks ext. sym as hard	*/
BOOL	symlinks  = FALSE;		/* -symlinks ext. hard as syml	*/
BOOL	linkdata  = FALSE;		/* -link-data data in hardlinks	*/
BOOL	doacl	  = FALSE;		/* -acl handle ACLs		*/
BOOL	doxattr	  = FALSE;		/* -xattr handle extended fattr	*/
BOOL	dolxattr  = FALSE;		/* -xattr-linux extended fattr	*/
BOOL	dofflags  = FALSE;		/* -xfflags handle extended ffl	*/
BOOL	link_dirs = FALSE;		/* -link-dirs hard linked dirs	*/
BOOL	dodump	  = FALSE;		/* -dump mode with all ino prop	*/
BOOL	dorestore = FALSE;		/* -restore in incremental mode	*/
BOOL	dopartial = FALSE;		/* -partial in incremental mode	*/
BOOL	forcerestore = FALSE;		/* -force-restore in incremental mode	*/
BOOL	dometa	  = FALSE;		/* -meta ino metadata only	*/
BOOL	dumpmeta  = FALSE;		/* -dumpmeta metadata for ctime	*/
BOOL	xmeta	  = FALSE;		/* -xmeta extract meta files	*/
BOOL	lowmem	  = FALSE;		/* -lowmem use less memory	*/
#ifdef	USE_FIND
BOOL	dofind	  = FALSE;		/* -find option found		*/
int	find_ac	  = 0;			/* ac past -find option		*/
char	*const *find_av = NULL;		/* av past -find option		*/
int	find_pac  = 0;			/* ac for first find primary	*/
char	*const *find_pav = NULL;	/* av for first find primary	*/
findn_t	*find_node;			/* syntaxtree from find_parse()	*/
void	*plusp;				/* residual for -exec ...{} +	*/
int	find_patlen;			/* len for -find pattern state	*/
char	*codeset = "ISO8859-1";
#ifdef	USE_SELINUX
BOOL	selinux_enabled;
#endif


LOCAL 	int		walkflags = WALK_CHDIR | WALK_PHYS | WALK_NOEXIT |
				    WALK_STRIPLDOT;
LOCAL	int		maxdepth = -1;
LOCAL	int		mindepth = -1;
EXPORT	struct WALK	walkstate;
#endif

BOOL	tcompat	  = FALSE;	/* Tar compatibility (av[0] is tar/ustar)   */
BOOL	fcompat	  = FALSE;	/* Archive file compatibility was requested */

int	intr	  = 0;		/* Did catch a ^C	*/

BOOL	do_subst;

/*
 * _grinfo is only used to read the information.
 */
GINFO	_ginfo;				/* Global (volhdr) information	*/
GINFO	_grinfo;			/* Global read information	*/
GINFO	*gip  = &_ginfo;		/* Global information pointer	*/
GINFO	*grip = &_grinfo;		/* Global read info pointer	*/

struct ga_props	gaprops;

#ifdef	STAR_FAT
#include "suntar.c"
#include "gnutar.c"
#include "cpio.c"
#include "pax.c"
#endif

#ifndef	NO_STAR_MAIN
#define	PTYPE_DEFAULT	P_STAR
/*
 * Achtung: Optionen wie f= sind problematisch denn dadurch dass -ffilename geht,
 * werden wird bei Falschschreibung von -fifo evt. eine Datei angelegt wird.
 */
/* BEGIN CSTYLED */
char	_opts[] = "C*,find~,help,xhelp,version,debug,xdebug#,xd#,bsdchdir,pax-ls,level#,tardumps*,wtardumps,time,no_statistics,no-statistics,cpio-statistics,fifostats,numeric,v+,block-number,tpath,c,u,r,x,t,copy,xcopy,n,diff,diffopts&,H&,artype&,print-artype,fs-name*,force_hole,force-hole,sparse,to_stdout,to-stdout,wready,force_remove,force-remove,ask_remove,ask-remove,remove_first,remove-first,remove_recursive,remove-recursive,keep-nonempty-dirs,install,nullout,onull,fifo,no_fifo,no-fifo,shm,fs&,VOLHDR*,list*,pkglist*,multivol,new-volume-script*,force-local,restore,partial,force-restore,freeze,file&,f&,T,Z,z,bz,j,lzo,7z,xz,lzip,zstd,lzma,compress-program*,rmt*,rsh*,bs&,blocks&,b&,B,pattern&,pat&,i,d,m,o,nochown,pax-o*,pax-p&,a,atime,p,no-p,dirmode,l,h,L,pax-L~,pax-H~,pax-P~,D,dodesc,M,xdev,w,pax-i,I,X&,exclude-from&,O,signed_checksum,signed-checksum,P,S,F+,U,uncond-rename,xdir,xdot,k,keep_old_files,keep-old-files,refresh_old_files,refresh-old-files,refresh,/,..,secure-links,no-secure-links%0,no-dirslash,not,V,match-tree,pax-match,pax-n,pax-c,notarg,maxsize&,newer*,ctime,nodump,tsize&,qic24,qic120,qic150,qic250,qic525,nowarn,newest_file,newest-file,newest,hpdev,modebits,copylinks,copyhardlinks,copysymlinks,copydlinks,hardlinks,symlinks,link-data,acl,xattr,xattr-linux,xfflags,link-dirs,dumpdate*,dump,dump\\+%2,cumulative,dump-cumulative,meta,dumpmeta,xmeta,silent,lowmem,no-xheader,no-fsync%1,do-fsync%0,read0,errctl&,e,data-change-warn,prinodes,dir-owner*,dir-group*,umask*,s&,?";
/* END CSTYLED */
char	*opts = _opts;
#else
extern	char	*opts;
#endif	/* NO_STAR_MAIN */

EXPORT int
main(ac, av)
	int	ac;
	char	**av;
{
	int		cac  = ac;
	char *const	*cav = av;
	int		oac;
	char *const	*oav;
	int		excode = 0;
	char		*tgt_dir = NULL;

	save_args(ac, av);

#ifdef  USE_NLS
	if (setlocale(LC_ALL, "") != NULL) {
#ifdef	CODESET
		codeset = nl_langinfo(CODESET);
#endif
	}

#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "star"	/* Use this only if it weren't */
#endif
	{ char	*dir;
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#if defined(PROTOTYPES) && defined(INS_BASE)
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
	}

#endif 	/* USE_NLS */

	my_uid = geteuid();
	my_uid = getuid();

	docompat(&cac, &cav);

	gargs(cac, cav);
	if (pname) {			/* cli=xxx seen as argv[1] */
		--cac, cav++;
	}
	--cac, cav++;
	oac = cac;
	oav = cav;

#ifdef	SIGHUP
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		set_signal(SIGHUP, sighup);
#endif
#ifdef	SIGINT
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		set_signal(SIGINT, sigintr);
#endif
#ifdef	SIGQUIT
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		set_signal(SIGQUIT, sigquit);
#endif
#ifdef	SIGINFO
	/*
	 * Be polite to *BSD users.
	 * They copied our idea and implemented intermediate status
	 * printing in 'dd' in 1990.
	 */
	if (signal(SIGINFO, SIG_IGN) != SIG_IGN)
		set_signal(SIGINFO, sigquit);
#endif

	file_raise((FILE *)NULL, FALSE);

	initbuf(nblocks);		/* Calls initfifo() if needed	*/

	(void) openremote();		/* This needs super user privilleges */

	if (geteuid() != getuid()) {	/* AIX does not like to do this */
					/* If we are not root		*/
#ifdef	HAVE_SETREUID
		if (setreuid(-1, getuid()) < 0)
#else
#ifdef	HAVE_SETEUID
		if (seteuid(getuid()) < 0)
#else
		if (setuid(getuid()) < 0)
#endif
#endif
			comerr("Panic cannot set back effective uid.\n");
	}
	my_uid = geteuid();
	/*
	 * WARNING: We now are no more able to open a new remote connection
	 * unless we have been called by root.
	 * It you like to do a remote multi-tape backup to different hosts
	 * and do not call star from root, you are lost.
	 */

#ifdef	USE_SELINUX
	selinux_enabled = is_selinux_enabled() > 0;
#endif

	opentape();

	if (stampfile)
		getstamp();

	star_mkvers();		/* Create version string */
	setprops(chdrtype);	/* Set up properties for archive format */
	/*
	 * If the archive format contains extended headers, we
	 * need to set up iconv().
	 */
	if (cflag && props.pr_flags & PR_XHDR)
		utf8_init(S_CREATE); /* Init. iconv() setup for xheader */

	if (!(rflag || uflag) || chdrtype != H_UNDEF)
		star_verifyopts(); /* Chk if options are valid for chdrtype */

	if (dumplevel >= 0)
		initdumpdates(dumpdates, wtardumps);
	dev_init(debug);	/* Init device macro handling */
	xbinit();		/* Initialize buffer for extended headers */

	if (dir_flags && (!tflag || copyflag))
		wdir = dogetwdir(TRUE);		/* Exit on failure */
	else if (xflag)
		wdir = dogetwdir(FALSE);	/* Return NULL on failure */

	getnstimeofday(&ddate);
	now	 = ddate.tv_sec + 60;
	sixmonth = ddate.tv_sec - 6L*30L*24L*60L*60L;
#ifdef	USE_FIND
	find_timeinit(ddate.tv_sec);
	walkinitstate(&walkstate);
#endif
	if (dd_name)
		init_ddate(dd_name);

	ginit();		/* Initialize global (volhdr) info */

	if (copyflag) {
		int		lac = cac;
		char *const	*lav = cav;

		if (tflag) {
			/*
			 * Flag no args at 'extract' side in -c -list mode.
			 */
			cav = &oav[oac];
			cac = 0;
		} else {
			/*
			 * Find last file type argument.
			 */
			for (; ; --cac, cav++) {
				if (getlfiles(&cac, &cav, &gaprops, opts) == 0)
					break;
				lac = cac;
				lav = cav;
			}
			tgt_dir = lav[0];
			cav = &lav[1];
			cac = lac-1;
			if (cac > 0) {
				errmsgno(EX_BAD,
				"Badly placed option after target directory.\n");
				susage(EX_BAD);
			}
		}
	}

	/*
	 * These callbacks are only called in case we leave star via comexit().
	 * We do this only in case of a severe error.
	 * These functions are called in the inverse set up oder, so checkerrs()
	 * called last.
	 */
	on_comerr((void(*)__PR((int, void *)))checkerrs, (void *)0);
	on_comerr((void(*)__PR((int, void *)))prstats, (void *)0);
	if (xflag)
		on_comerr((void(*)__PR((int, void *)))flushdirtimes, (void *)0);
#ifdef	FIFO
	if (use_fifo) {
		runfifo(oac, oav);	/* Run FIFO, fork() is called here  */
		on_comerr(fifo_onexit,	/* For foreground FIFO process only */
			(void *)0);
	}
#endif

	if (copyflag) {
		do_subst = FALSE;	/* Substitution only at create side */
		havepat = FALSE;	/* Patterns only at create side */
		listfile = NULL;	/* Listfile only at create side */
		swapflg = 0;		/* Don't try to find out the hdrtype */
		dir_flags = tgt_dir;	/* Target directory only at extract */
#ifdef	USE_FIND
		dofind = FALSE;		/* -find expr only at create side */
#endif
	}

	if (xflag || tflag || diff_flag) {
		/*
		 * cflag will never be TRUE in this case
		 */
		if (listfile) {
			openlist();
			hash_build(listf);
			if ((currdir = dir_flags) != NULL)
				dochdir(currdir, TRUE);
		} else {
#ifdef	USE_FIND
			if (!dofind) {
#endif
			for (; ; --cac, cav++) {
				if (dir_flags)
					getdir(&cac, &cav, &currdir);
				if (getlfiles(&cac, &cav, &gaprops, opts) == 0)
					break;
				addarg(cav[0]);
			}
#ifdef	USE_FIND
			}
#endif
			closepattern();
		}
		if (tflag) {
			list();
		} else {
			/*
			 * xflag || diff_flag
			 * First change dir to the one or last -C arg
			 * in case there is no pattern in list.
			 */
			if ((currdir = dir_flags) != NULL)
				dochdir(currdir, TRUE);
			if (xflag)
				extract(volhdr);
			else
				diff();
		}
	}
	closepattern();
	if (uflag || rflag) {
		/*
		 * cflag will also be TRUE in this case
		 */
		skipall();
		syncbuf();
		backtape();
	}
	if (cflag) {
		/*
		 * xflag, tflag, diff_flag will never be TRUE in this case
		 */
		star_create(cac, cav);
	}

#ifdef	USE_FIND
	find_plusflush(plusp, &walkstate);
#endif
	fflush(vpr);	/* Avoid output mix with checklinks() from 2>&1 | tee */
	if (!nolinkerr)
		checklinks();
	if (!use_fifo) {
		extern m_stats	*stats;

		closetape();
		runnewvolscript(stats->volno+1, tarfindex+1);
	}
#ifdef	FIFO
	if (use_fifo)
		fifo_exit(0);
#endif

#ifdef	HAVE_FORK
	while (wait(0) >= 0) {
		;
		/* LINTED */
	}
#endif
	if (!no_stats)
		prpatstats();
	prstats();
	if (checkerrs()) {
		if (!nowarn && !no_stats) {
			errmsgno(EX_BAD,
			"Processed all possible files, despite earlier errors.\n");
		}
		excode = -2;
	}
	if (intr) {
		/*
		 * This happens when we are in create mode and the interrupt is
		 * delayed in order to prevent inconsistent archives.
		 */
		if (excode == 0)
			excode = -4;
	}
	if (!isatty(fdown(stderr))) {
		char	*p;

		/*
		 * Try to avoid that the verbose or diagnostic messages are
		 * sometimes lost if called on Linux via "ssh". Unfortunately
		 * this does not always help. If you like to make sure that
		 * nothing gets lost, call: ssh host "star .... ; sleep 10"
		 */
		fflush(vpr);
		fflush(stderr);
#ifdef	HAVE_FSYNC
		if (!no_fsync) {
			fsync(fdown(vpr));
			fsync(fdown(stderr));
		}
#endif
		/*
		 * Use the sleep only in case that the environment is set, but
		 * keep the fflush() as stderr may be buffered.
		 */
		if ((p = getenv("STAR_WORKAROUNDS")) != NULL &&
		    strstr(p, "ssh-tcpip") != NULL)
			usleep(100000);
	}
#ifdef	FIFO
	/*
	 * Fetch errno from FIFO if available.
	 */
	if (fifo_errno())
		excode = fifo_errno();
#endif
	if (dumplevel >= 0 && wtardumps) {
		if (excode != 0 || intr) {
			errmsgno(EX_BAD, "'%s' not written due to problems during backup.\n",
				dumpdates);
		} else {
			int	dflags = 0;

			if (gip->dumptype != DT_FULL)
				dflags |= DD_PARTIAL;
			if (dump_cumulative)
				dflags |= DD_CUMULATIVE;

			writedumpdates(dumpdates, gip->filesys, dumplevel, dflags, &ddate);
		}
	}
#ifdef	DBG_MALLOC
	aprintlist(stdout, 1);
#endif

	exit(excode);
	/* NOTREACHED */
	return (excode);	/* keep lint happy */
}

LOCAL void
star_create(ac, av)
	int		ac;
	char	*const *av;
{
	/*
	 * xflag, tflag, diff_flag will never be TRUE in this case
	 */
	put_release();		/* Pax 'g' vendor unique */
	put_archtype();		/* Pax 'g' vendor unique */
	if (dumplevel < 0)	/* In dump mode we first collect the data */
		put_volhdr(volhdr, TRUE);
#ifdef	USE_FIND
	if (dumplevel >= 0 && (listfile || dofind))
		comerrno(EX_BAD,
			"Cannot do incremental dumps with list= or -find.\n");
#else
	if (dumplevel >= 0 && listfile)
		comerrno(EX_BAD, "Cannot do incremental dumps with list=.\n");
#endif
#ifdef	USE_FIND
	if (dofind) {
		if (listfile)
			walkopen(&walkstate);

		if (find_patlen > 0) {
			walkstate.patstate = ___malloc(sizeof (int) * find_patlen,
						"space for pattern state");
		}

		walkstate.walkflags	= walkflags;
		walkstate.maxdepth	= maxdepth;
		walkstate.mindepth	= mindepth;
		walkstate.lname		= NULL;
		walkstate.tree		= find_node;
		walkstate.err		= 0;
		walkstate.pflags	= 0;

		if (listfile)
			openlist();

		if ((currdir = dir_flags) != NULL)
			dochdir(currdir, TRUE);

		if (listfile) {
			if (find_pav > find_av)
				comerrno(EX_BAD, "Too many args for list= option.\n");
			createlist(&walkstate);
		} else {
			nodesc = TRUE;
			for (av = find_av; av != find_pav; av++) {
				treewalk(*av, walkfunc, &walkstate);
			}
		}
	} else
#endif
	if (listfile) {
		openlist();
		if ((currdir = dir_flags) != NULL)
			dochdir(currdir, TRUE);
		/*
		 * We do not allow file type args together with list=
		 * Note that Sun tar allows a mix.
		 */
		if (getlfiles(&ac, &av, &gaprops, opts) > 0)
			comerrno(EX_BAD, "Too many args for list= option.\n");
		createlist(NULL);
	} else {
		const char	*cdir = NULL;

		for (; ; --ac, av++) {
			if (dir_flags)
				getdir(&ac, &av, &currdir);
			if (currdir && cdir != currdir) {
				if (!(dochdir(wdir, FALSE) &&
				    dochdir(currdir, FALSE)))
					break;
				cdir = currdir;
			}

			if (getlfiles(&ac, &av, &gaprops, opts) == 0)
				break;
			if (dumplevel >= 0) {
				dumpd_t	*dp;
				int	dflags = 0;

				/*
				 * The next message is only for debugging
				 * purposes to find problems related to option
				 * parsing.
				 */
				if (ac > 1)
					errmsgno(EX_BAD, "INFO: ac %c av[0] '%s'\n", ac, av[0]);

				/*
				 * We cannot have more than one file type
				 * argument in dump mode if we like to grant
				 * the consistency of dumps. In theory, it would
				 * be possible to allow it, but then we will not
				 * be able to deal with renames from outside the
				 * scope to inside the scope.
				 */
				if (ac > 1)
					comerrno(EX_BAD,
					"Only one file type arg allowed in dump mode.\n");
				if (cdir == NULL) {
					comerrno(EX_BAD,
					"Need '-C dir' in dump mode.\n");
					/* NOTREACHED */
				}
				if (cdir[0] != '/')
					comerrno(EX_BAD,
					"Need absolute path with '-C dir' in dump mode.\n");
				if (!streql(av[0], "."))
					comerrno(EX_BAD,
					"File type arg must be '.' in dump mode.\n");

				gip->filesys = (char *)cdir;
				gip->gflags |= GF_FILESYS;
				if (fs_name) {
					gip->filesys = fs_name;
					gip->cwd    = (char *)cdir;
					gip->gflags |= GF_CWD;
				}
				/*
				 * Set dump type to full/partial.
				 */
				checkdumptype(gip);
				if (gip->dumptype != DT_FULL)
					dflags |= DD_PARTIAL;
				if (dump_cumulative)
					dflags |= DD_CUMULATIVE;
				dp = checkdumpdates(gip->filesys, dumplevel, dflags);
				if (dp == NULL && dumplevel > 0 && gip->dumptype != DT_FULL)
					dp = checkdumpdates(gip->filesys, dumplevel, 0);
				if (dp == NULL && dumplevel > 0) {
					errmsgno(EX_BAD,
					"No level 0 dump entry found in '%s'.\n",
					dumpdates);
					comerrno(EX_BAD, "Perform a level 0 dump first.\n");
				}
				if (dp) {
					oldlevel = dp->dd_level;
					Newer = dp->dd_date;
					gip->reflevel = dp->dd_level;
					gip->refdate = dp->dd_date;
					gip->gflags |= (GF_REFLEVEL|GF_REFDATE);
				}

				adddumpdates(gip->filesys, dumplevel, dflags,
								&ddate, TRUE);

				error("Type of this level %d%s dump: %s\n",
					dumplevel,
					(dflags & DD_PARTIAL) ? "P":" ",
					dt_name(gip->dumptype));
				error("Date of this level %d%s dump: %s\n",
					dumplevel,
					(dflags & DD_PARTIAL) ? "P":" ",
					dumpdate(&ddate));
				error("Date of last level %d%s dump: %s\n",
					oldlevel,
					(dp && (dp->dd_flags & DD_PARTIAL)) ?
						"P":" ",
					dumpdate(&Newer));

				put_volhdr(volhdr, TRUE);
			}
			if (intr)
				break;
			curfs = NODEV;
			/*
			 * To avoid empty incremental dumps, make sure that
			 * av[0] is always in the archive in with dumplevel >= 0
			 */
			create(av[0], paxHflag, dumplevel >= 0);
			if (bsdchdir && wdir && !dochdir(wdir, FALSE))
				break;
		}
	}
	flushlinks();
	weof();
	buf_drain();
}

LOCAL void
checkdumptype(gp)
	GINFO	*gp;
{
	FINFO	dinfo;
	FINFO	ddinfo;
	BOOL	full = FALSE;

	if (!_getinfo(".", &dinfo))
		return;
	if (!_getinfo("..", &ddinfo))
		return;

	if (dinfo.f_ino == ddinfo.f_ino && dinfo.f_dev == ddinfo.f_dev)
		full = TRUE;

	if (dinfo.f_dev != ddinfo.f_dev)
		full = TRUE;
	if (full && !dump_partial && !havepat) {
		gp->gflags |= GF_DUMPTYPE;
		gp->dumptype = DT_FULL;
	} else {
		gp->gflags |= GF_DUMPTYPE;
		gp->dumptype = DT_PARTIAL;
	}
}

LOCAL void
init_ddate(name)
	char	*name;
{
	FINFO	ddinfo;

	if (!_getinfo(name, &ddinfo))
		comerr("Cannot stat '%s'.\n", name);

	ddate.tv_sec  = ddinfo.f_mtime;
	ddate.tv_nsec = ddinfo.f_mnsec;
}

EXPORT void
copy_create(ac, av)
	int		ac;
	char	*const *av;
{
	int		oac = ac;
	char *const	*oav = av;
	int		lac = ac;
#ifdef	__needed__
	char *const	*lav = av;
#endif

	verbose = 0;		/* Verbose not at create side */
	interactive = FALSE;	/* Interactive not at create side */

	if (!tflag) {
		/*
		 * Cut off beginning at last file type arg.
		 */
		for (; ; --ac, av++) {
			if (getlfiles(&ac, &av, &gaprops, opts) == 0)
				break;
			lac = ac;
#ifdef	__needed__
			lav = av;
#endif
		}
		ac = oac-lac;
		av = oav;
	}

	star_create(ac, av);
	/*
	 * XXX Fehlerzusammenfassung fuer die -c reate Seite?
	 */
}

LOCAL int
getfilecount(ac, av, fmt)
	int		ac;
	char	*const *av;
	const char	*fmt;
{
	int	files = 0;

	for (; ; --ac, av++) {
		if (getlfiles(&ac, &av, &gaprops, fmt) == 0)
			break;
		files++;
	}
	return (files);
}

LOCAL void
getdir(acp, avp, dirp)
	int		*acp;
	char *const	**avp;
	const char	**dirp;
{
	int	len = strlen(opts);
	char	*dir = NULL;

	if (iftype == I_CPIO || iftype == I_PAX)
		return;
	/*
	 * Skip all other flags.
	 * Note that we need to patch away "...,?" at the end of the
	 * option string so this will not interfere with a -C dir
	 * option in the command line.
	 */
	if (opts[len-1] == '?' && opts[len-2] == ',')
		opts[len-2] = '\0';
	getlfiles(acp, avp, &gaprops, &opts[3]);

	if (debug) /* temporary */
		errmsgno(EX_BAD, "Flag/File: '%s'.\n", (*avp)[0]);

again:
	/*
	 * Get next '-C dir' option
	 */
	if (getlargs(acp, avp, &gaprops, "C*", &dir) < 0) {
		int	cac = *acp;
		/*
		 * Skip all other flags that are known to star.
		 */
		if (getlfiles(acp, avp, &gaprops, &opts[3]) < 0) {
			/*
			 * If we did find other legal flags, try again.
			 */
			if (cac > *acp)
				goto again;

			errmsgno(EX_BAD, "Badly placed Option: %s.\n",
						(*avp)[0]);
			if ((*avp)[1] != NULL)
				errmsgno(EX_BAD, "Next arg is '%s'.\n",
						(*avp)[1]);
			susage(EX_BAD);
		}
	}
	if (opts[len-2] == '\0')
		opts[len-2] = ',';
	if (dir)
		*dirp = dir;
	if (debug) /* temporary */
		errmsgno(EX_BAD, "Dirp: '%s' Dir: %s.\n", *dirp, dir);
}

LOCAL void
openlist()
{
	if (streql(listfile, "-")) {
		check_stdin("list=");
		listf = stdin;
		listfile = "stdin";
	} else if ((listf = lfilemopen(listfile, "r", S_IRWALL)) == (FILE *)NULL)
		comerr("Cannot open '%s'.\n", listfile);
}

LOCAL void
check_stdin(name)
	char	*name;
{
	if (did_stdin) {
		comerrno(EX_BAD,
		"Did already use stdin, cannot use stdin for '%s' option.\n",
		name);
	}
	did_stdin = TRUE;
}

#ifndef	NO_STAR_MAIN
/*
 * Short usage
 */
LOCAL void
susage(ret)
	int	ret;
{
#ifdef	STAR_FAT
	switch (ptype) {

	case P_SUNTAR:
		suntar_susage(ret); exit(ret);
	case P_GNUTAR:
		gnutar_susage(ret); exit(ret);
	case P_PAX:
		pax_susage(ret); exit(ret);
	case P_CPIO:
		cpio_susage(ret); exit(ret);
	}
#endif
#ifdef	USE_FIND
	error("Usage:\t%s cmd [options] [-find] file1 ... filen [find expression]\n", get_progname());
#else
	error("Usage:\t%s cmd [options] file1 ... filen\n", get_progname());
#endif
	error("\t%s cli=name ...\n", get_progname());
	error("\nUse\t%s -help\n", get_progname());
	error("and\t%s -xhelp\n", get_progname());
	error("to get a list of valid cmds and options.\n");
	error("\nUse\t%s H=help\n", get_progname());
	error("to get a list of valid archive header formats.\n");
	error("\nUse\t%s diffopts=help\n", get_progname());
	error("to get a list of valid diff options.\n");
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
usage(ret)
	int	ret;
{
#ifdef	STAR_FAT
	switch (ptype) {

	case P_SUNTAR:
		suntar_usage(ret); exit(ret);
	case P_GNUTAR:
		gnutar_usage(ret); exit(ret);
	case P_PAX:
		pax_usage(ret); exit(ret);
	case P_CPIO:
		cpio_usage(ret); exit(ret);
	}
#endif
#ifdef	USE_FIND
	error("Usage:\t%s cmd [options] [-find] file1 ... filen [find expression]\n", get_progname());
#else
	error("Usage:\t%s cmd [options] file1 ... filen\n", get_progname());
#endif
	error("\t%s cli=name ...\n", get_progname());
	error("Cmd:\n");
	error("\t-c/-u/-r\tcreate/update/replace archive with named files to tape\n");
	error("\t-x/-t/-n\textract/list/trace named files from tape\n");
	error("\t-copy\t\tcopy named files to destination directory\n");
	error("\t-diff\t\tdiff archive against file system (see -xhelp)\n");
	error("Options:\n");
	error("\t-help\t\tprint this help\n");
	error("\t-xhelp\t\tprint extended help\n");
	error("\t-version\tprint version information and exit\n");
	error("\t-xcopy\t\talias for -copy -sparse -acl\n");
	error("\tblocks=#,b=#\tset blocking factor to #x512 Bytes (default 20)\n");
	error("\tfile=nm,f=nm\tuse 'nm' as tape instead of stdin/stdout\n");
	error("\t-T\t\tuse $TAPE as tape instead of stdin/stdout\n");
	error("\t-[0-7]\t\tSelect an alternative tape drive\n");
#ifdef	FIFO
	error("\t-fifo/-no-fifo\tuse/don't use a fifo to optimize data flow from/to tape\n");
#if defined(USE_MMAP) && defined(USE_USGSHM)
	error("\t-shm\t\tuse SysV shared memory for fifo\n");
#endif
#endif
	error("\t-v\t\tincrement verbose level\n");
	error("\t-block-number\tprint the block numbers where the TAR headers start\n");
	error("\t-tpath\t\tuse with -t, -cv or -diff to list path names only\n");
	error("\tH=header\tgenerate 'header' type archive (see H=help)\n");
	error("\tartype=header\tgenerate 'header' type archive (see artype=help)\n");
	error("\t-print-artype\tcheck and print archive and compression type on one line and exit.\n");
	error("\tC=dir\t\tperform a chdir to 'dir' before storing/extracting next file\n");
	error("\t-bsdchdir\tdo BSD style C= (only related to the next file type arg)\n");
#ifdef	USE_FIND
	error("\t-find\t\tOption separator: Use find command line to the right.\n");
#endif
	error("\t-Z\t\tpipe input/output through compress, does not work on tapes\n");
	error("\t-z\t\tpipe input/output through gzip, does not work on tapes\n");
	error("\t-j,-bz\t\tpipe input/output through bzip2, does not work on tapes\n");
	error("\t-lzo\t\tpipe input/output through lzop, does not work on tapes\n");
	error("\t-7z\t\tpipe input/output through p7zip, does not work on tapes\n");
	error("\t-xz\t\tpipe input/output through xz, does not work on tapes\n");
	error("\t-lzip\t\tpipe input/output through lzip, does not work on tapes\n");
	error("\t-zstd\t\tpipe input/output through zstd, does not work on tapes\n");
	error("\t-lzma\t\tpipe input/output through lzma, does not work on tapes\n");
	error("\t-freeze\t\tpipe input/output through freeze, does not work on tapes\n");
	error("\tcompress-program=name\tpipe input/output through program 'name', does not work on tapes\n");
	error("\t-B\t\tperform multiple reads (needed on pipes)\n");
	error("\t-i\t\tignore checksum errors\n");
	error("\t-d\t\tdo not store/create directories\n");
	error("\t-m\t\tdo not restore access and modification time\n");
	error("\t-o,-nochown\tdo not restore owner and group\n");
	error("\t-pax-o string\tPAX options (none specified with SUSv2 / UNIX-98)\n");
	error("\t-pax-p string\tuse PAX like privileges set up\n");
	error("\t-a,-atime\treset access time after storing file\n");
	error("\t-p\t\trestore file permissions\n");
	error("\t-no-p\t\tdo not restore file permissions\n");
	error("\t-l\t\tdo not print a message if not all links are dumped\n");
	error("\t-h,-L\t\tfollow symbolic links as if they were files\n");
	error("\t-pax-L\t\tfollow symbolic links as if they were files (PAX style)\n");
	error("\t-pax-H\t\tfollow symbolic links from cmdline as if they were files (PAX style)\n");
	error("\t-D\t\tdo not descend directories\n");
	error("\t-M,-xdev\tdo not descend mounting points\n");
	error("\t-w\t\tdo interactive creation/extraction/renaming\n");
	error("\t-pax-i\t\tdo interactive creation/extraction/renaming (PAX style)\n");
	error("\t-O\t\tbe compatible to old tar (except for checksum bug)\n");
	error("\t-P\t\tlast record may be partial (useful on cartridge tapes)\n");
	error("\t-S\t\tdo not store/create special files\n");
	error("\t-F,-FF,-FFF,...\tdo not store/create SCCS/RCS, core and object files\n");
	error("\t-U\t\trestore files unconditionally\n");
	error("\t-uncond-rename\twith interactive restore unconditionally ask for name\n");
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
xusage(ret)
	int	ret;
{
#ifdef	STAR_FAT
	switch (ptype) {

	case P_SUNTAR:
		suntar_xusage(ret); exit(ret);
	case P_GNUTAR:
		gnutar_xusage(ret); exit(ret);
	case P_PAX:
		pax_xusage(ret); exit(ret);
	case P_CPIO:
		cpio_xusage(ret); exit(ret);
	}
#endif
#ifdef	USE_FIND
	error("Usage:\t%s cmd [options] [-find] file1 ... filen [find expression]\n", get_progname());
#else
	error("Usage:\t%s cmd [options] file1 ... filen\n", get_progname());
#endif
	error("\t%s cli=name ...\n", get_progname());
	error("Extended options:\n");
	error("\tdiffopts=optlst\tcomma separated list of diffopts (see diffopts=help)\n");
	error("\t-debug\t\tprint additional debug messages\n");
	error("\txdebug=#,xd=#\tset extended debug level\n");
	error("\t-pax-ls\t\tprint a PAX type file listing\n");
	error("\t-silent\t\tno not print informational messages\n");
	error("\t-lowmem\t\ttry to use less memory for operation\n");
	error("\t-not,-V\t\tuse those files which do not match pat= pattern\n");
	error("\t-pax-match\tuse PAX like pattern matching\n");
	error("\t-pax-n\t\tonly one match per pattern allowed\n");
	error("\t-notarg,-pax-c\tuse those files which do not match file type pattern\n");
	error("\tVOLHDR=name\tuse name to generate a volume header\n");
	error("\t-xdir\t\textract dir even if the current is never\n");
	error("\t-xdot\t\textract first '.' or './' dir even if the current is never\n");
	error("\t-dirmode\twrite directories after the files they contain\n");
	error("\t-link-dirs\tlook for hard linked directories in create mode\n");
	error("\t-dump\t\tarchive more ino metadata (needed for incremental dumps)\n");
	error("\t-dump+\t\tlike -dump but with more global meta data\n");
	error("\t-restore\trestore incremental dumps\n");
	error("\t-partial\tpermit to restore partial incremental dumps\n");
	error("\t-force-restore\tforce to restore partial incremental dumps\n");
	error("\t-no-xheader\tdo not read or write extended headers regardless of format\n");
	error("\t-meta\t\tuse inode metadata only (omit file content)\n");
	error("\t-xmeta\t\textract meta files\n");
	error("\t-dumpmeta\tuse inode metadata in dump mode if only ctime is newer\n");
	error("\t-keep-old-files,-k\tkeep existing files\n");
	error("\t-refresh-old-files\trefresh existing files, don't create new files\n");
	error("\t-refresh\trefresh existing files, don't create new files\n");
	error("\t-/\t\tdon't strip leading '/'s from file names\n");
	error("\t-..\t\tdon't skip filenames that contain '..' in non-interactive extract\n");
	error("\t-secure-links\tdon't extract links that start with '/' or contain '..' (default)\n");
	error("\t-no-secure-links\textract links that start with '/' or contain '..'\n");
	error("\t-no-dirslash\tdon't append a slash to directory names\n");
	error("\tlist=name\tread filenames from named file\n");
	error("\t-X name\t\texclude filenames from named file\n");
	error("\t-exclude-from name\texclude filenames from named file\n");
	error("\tpkglist=name\tread filenames from named file (unstable interface for sps)\n");
	error("\t-read0\t\tread null terminated filenames with list=\n");
	error("\t-data-change-warn\ttreat data/size changes in create more as warning only\n");
	error("\t-e\t\tabort on all error conditions undefined by errctl=\n");
	error("\terrctl=name\tread error contrl definitions from named file\n");
	error("\t-dodesc\t\tdo descend directories found in a list= file\n");
	error("\tpattern=p,pat=p\tset matching pattern\n");
	error("\t-match-tree\tdo not scan the content of non matching dirs in create mode\n");
	error("\ts=replstr\tApply ed like pattern substitution -s /old/new/gp on filenames\n");
	error("\tlevel=dumplevel\tset current incremental dump level\n");
	error("\t-cumulative\tmake a cumulative incremental dump (relative to same level)\n");
	error("\ttardumps=name\tset file name for tar dump dates, default is %s\n", dumpdates);
	error("\t-wtardumps\tupdate file for tar dump dates if in dump mode\n");
	error("\tdumpdate=name\tuse timestamp from name instead of current time for %s\n", dumpdates);
	error("\tfs-name=name\tuse name instead of mount point for %s\n", dumpdates);
	error("\tmaxsize=#\tdo not store file if bigger than # (default mult is kB)\n");
	error("\tnewer=name\tstore only files which are newer than 'name'\n");
	error("\t-multivol\tread/write/list a multi volume archive\n");
	error("\tnew-volume-script=script\tcall 'script' at end of each volume\n");
	error("\t-ctime\t\tuse ctime for newer= option\n");
	error("\t-nodump\t\tdo not dump files that have the nodump flag set\n");
	error("\t-acl\t\thandle access control lists\n");
	error("\t-xattr\t\thandle extended file attributes\n");
	error("\t-xattr-linux\t\thandle extended file attributes (Linux variant)\n");
	error("\t-xfflags\thandle extended file flags\n");
	error("\t-prinodes\tif archive contains inode number, print them in list mode\n");
	error("\tbs=#\t\tset (output) block size to #\n");
#ifdef	FIFO
	error("\tfs=#\t\tset fifo size to #\n");
#endif
	error("\ttsize=#\t\tset tape volume size to # (default multiplier is 512)\n");
	error("\t-qic24\t\tset tape volume size to %d kBytes\n",
						TSIZE(QIC_24_TSIZE)/1024);
	error("\t-qic120\t\tset tape volume size to %d kBytes\n",
						TSIZE(QIC_120_TSIZE)/1024);
	error("\t-qic150\t\tset tape volume size to %d kBytes\n",
						TSIZE(QIC_150_TSIZE)/1024);
	error("\t-qic250\t\tset tape volume size to %d kBytes\n",
						TSIZE(QIC_250_TSIZE)/1024);
	error("\t-qic525\t\tset tape volume size to %d kBytes\n",
						TSIZE(QIC_525_TSIZE)/1024);
	error("\t-no-fsync\tdo not call fsync() for each extracted file (may be dangerous)\n");
	error("\t-nowarn\t\tdo not print warning messages\n");
	error("\t-time\t\tprint timing info\n");
	error("\t-no-statistics\tdo not print statistics\n");
	error("\t-cpio-statistics\tprint cpio style statistics\n");
#ifdef	FIFO
	error("\t-fifostats\tprint fifo statistics\n");
#endif
	error("\t-numeric\tdon't use user/group name from tape\n");
	error("\t-newest\t\tfind newest file on tape\n");
	error("\t-newest-file\tfind newest regular file on tape\n");
	error("\t-hpdev\t\tuse HP's non POSIX compliant method to store dev numbers\n");
	error("\t-modebits\tinclude all 16 bits from stat.st_mode, this violates POSIX-1003.1\n");
	error("\t-copylinks\tCopy hard and symlinks rather than linking\n");
	error("\t-copyhardlinks\tCopy hardlink source files rather than linking\n");
	error("\t-copysymlinks\tCopy symlink source files rather than linking\n");
	error("\t-copydlinks\tCopy the content of linked dirs\n");
	error("\t-hardlinks\tExtract symlinks as hardlinks\n");
	error("\t-link-data\tInclude data for hard linked files\n");
	error("\t-symlinks\tExtract hardlinks as symlinks\n");
	error("\t-signed-checksum\tuse signed chars to calculate checksum\n");
	error("\trmt=path\tSpecify remote path to remote tape server program\n");
	error("\trsh=path\tSpecify path to remote login program\n");
	error("\t-sparse\t\thandle file with holes effectively on store/create\n");
	error("\t-force-hole\ttry to extract all files with holes\n");
	error("\t-to-stdout\textract files to stdout\n");
	error("\t-wready\t\twait for tape drive to become ready\n");
	error("\t-force-remove\tforce to remove non writable files on extraction\n");
	error("\t-ask-remove\task to remove non writable files on extraction\n");
	error("\t-remove-first\tremove files before extraction\n");
	error("\t-remove-recursive\tremove files recursive\n");
	error("\t-keep-nonempty-dirs\tdo not complain about non-empty dirs\n");
	error("\t-install\tcarefully replace old files with -x, similar to install(1)\n");
	error("\tdir-owner=user\tIntermediate created directories will be owned by 'user'.\n");
	error("\tdir-group=user\tIntermediate created directories will be owned by 'group'.\n");
	error("\tumask=mask\tSet star's umask to 'mask'.\n");
	error("\t-onull,-nullout\tsimulate creating an achive to compute the size\n");
	exit(ret);
	/* NOTREACHED */
}
#endif	/* NO_STAR_MAIN */

LOCAL void
dusage(ret)
	int	ret;
{
	error("Diff options:\n");
	error("\tnot\t\tif this option is present, exclude listed options\n");
	error("\t!\t\tif this option is present, exclude listed options\n");
	error("\tall\t\tcompare everything\n");
	error("\tperm\t\tcompare file permissions\n");
	error("\tmode\t\tcompare file permissions\n");
	error("\tsymperm\t\tcompare symlink permissions\n");
	error("\ttype\t\tcompare file type\n");
	error("\tnlink\t\tcompare all linkcounts (star dump mode only)\n");
	error("\tdnlink\t\tcompare directory linkcounts (star dump mode only)\n");
	error("\tuid\t\tcompare owner of file\n");
	error("\tgid\t\tcompare group of file\n");
	error("\tuname\t\tcompare name of owner of file\n");
	error("\tgname\t\tcompare name of group of file\n");
	error("\tid\t\tcompare owner, group, ownername and groupname of file\n");
	error("\tsize\t\tcompare file size\n");
	error("\tdata\t\tcompare content of file\n");
	error("\tcont\t\tcompare content of file\n");
	error("\trdev\t\tcompare rdev of device node\n");
	error("\thardlink\tcompare target of hardlink\n");
	error("\tsymlink\t\tcompare target of symlink\n");
	error("\tsympath\t\tcompare target pathname of symlink\n");
	error("\tsparse\t\tcompare if both files are sparse or not\n");
	error("\tatime\t\tcompare access time of file (only star)\n");
	error("\tmtime\t\tcompare modification time of file\n");
	error("\tctime\t\tcompare creation time of file (only star)\n");
	error("\ttimes\t\tcompare all times of file\n");
	error("\tlmtime\t\tcompare modification time of symlinks\n");
	error("\txtimes\t\tcompare all times and lmtime\n");
	error("\tnsecs\t\tcompare nanoseconds in times\n");
	error("\tdir\t\tcompare directory content (star dump mode only)\n");
#ifdef USE_ACL
	error("\tacl\t\tcompare access control lists (specify -acl also)\n");
#endif
#ifdef USE_XATTR
	error("\txattr\t\tcompare extended attributes (specify -xattr also)\n");
#endif
#ifdef USE_FFLAGS
	error("\tfflags\t\tcompare extended file flags (specify -xfflags also)\n");
#endif
	error("\n");
	error("Default is to compare everything except atime.\n");
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
husage(ret)
	int	ret;
{
	error("Header types (default marked with '*'):\n");
	hdr_usage();
	exit(ret);
	/* NOTREACHED */
}

#ifndef	NO_STAR_MAIN
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
	BOOL	xcopy	 = FALSE;
	BOOL	oldtar	 = FALSE;
	BOOL	no_fifo	 = FALSE;
	BOOL	usetape	 = FALSE;
	BOOL	dodesc	 = FALSE;
	BOOL	qic24	 = FALSE;
	BOOL	qic120	 = FALSE;
	BOOL	qic150	 = FALSE;
	BOOL	qic250	 = FALSE;
	BOOL	qic525	 = FALSE;
	BOOL	dchangeflag = FALSE;
	char	*pkglistfile = NULL;
	char	*diruid	 = NULL;
	char	*dirgid	 = NULL;
	char	*u_mask	 = NULL;
	char	*paxopts = NULL;
	const	char	*p;
	Llong	llbs	 = 0;
signed	char	archive	 = -1;		/* On IRIX, we have unsigned chars by default */
BOOL	Ointeractive	 = FALSE;

/* BEGIN CSTYLED */
/*char	_opts[] = "C*,find~,help,xhelp,version,debug,xdebug#,xd#,bsdchdir,pax-ls,level#,tardumps*,wtardumps,time,no_statistics,no-statistics,cpio-statistics,fifostats,numeric,v+,block-number,tpath,c,u,r,x,t,copy,xcopy,n,diff,diffopts&,H&,artype&,print-artype,fs-name*,force_hole,force-hole,sparse,to_stdout,to-stdout,wready,force_remove,force-remove,ask_remove,ask-remove,remove_first,remove-first,remove_recursive,remove-recursive,keep-nonempty-dirs,install,nullout,onull,fifo,no_fifo,no-fifo,shm,fs&,VOLHDR*,list*,pkglist*,multivol,new-volume-script*,force-local,restore,partial,force-restore,freeze,file&,f&,T,Z,z,bz,j,lzo,7z,xz,lzip,zstd,lzma,compress-program*,rmt*,rsh*,bs&,blocks&,b&,B,pattern&,pat&,i,d,m,o,nochown,pax-o*,pax-p&,a,atime,p,no-p,dirmode,l,h,L,pax-L~,pax-H~,pax-P~,D,dodesc,M,xdev,w,pax-i,I,X&,exclude-from&,O,signed_checksum,signed-checksum,P,S,F+,U,uncond-rename,xdir,xdot,k,keep_old_files,keep-old-files,refresh_old_files,refresh-old-files,refresh,/,..,secure-links,no-secure-links%0,no-dirslash,not,V,match-tree,pax-match,pax-n,pax-c,notarg,maxsize&,newer*,ctime,nodump,tsize&,qic24,qic120,qic150,qic250,qic525,nowarn,newest_file,newest-file,newest,hpdev,modebits,copylinks,copyhardlinks,copysymlinks,copydlinks,hardlinks,symlinks,link-data,acl,xattr,xattr-linux,xfflags,link-dirs,dumpdate*,dump,dump\\+%2,cumulative,dump-cumulative,meta,dumpmeta,xmeta,silent,lowmem,no-xheader,no-fsync%1,do-fsync%0,read0,errctl&,e,data-change-warn,prinodes,dir-owner*,dir-group*,umask*,s&,?";*/
/* END CSTYLED */

	getarginit(&gaprops, GAF_DEFAULT);	/* Set default behavior	  */
#ifdef	STAR_FAT
	switch (ptype) {

	case P_SUNTAR:
		suntar_gargs(ac, av);
		return;
	case P_GNUTAR:
		gnutar_gargs(ac, av);
		return;
	case P_PAX:
		pax_gargs(ac, av);
		return;
	case P_CPIO:
		cpio_gargs(ac, av);
		return;
	}
#endif

	p = filename(av[0]);
	if (streql(p, "ustar")) {
		/*
		 * If we are called as "ustar" we are as POSIX-1003.1-1988
		 * compliant as possible. There are no enhancements at all.
		 */
		hdrtype = H_USTAR;
	} else if (streql(p, "tar")) {
		/*
		 * If we are called as "tar" we are mostly POSIX compliant
		 * and use POSIX-1003.1-2001 extensions. The differences of the
		 * base format compared to POSIX-1003.1-1988 can only be
		 * regocnised by star. Even the checsum bug of the "pax"
		 * reference implementation is not hit by the fingerprint
		 * used to allow star to discriminate XUSTAR from USTAR.
		 */
		hdrtype = H_XUSTAR;
	}
	/*
	 * Current default archive format in all other cases is XSTAR (see
	 * above). This will not change until 2004 (then the new XUSTAR format
	 * is recognised by star for at least 5 years and we may asume that
	 * all star installations will properly handle it.
	 * XSTAR is USTAR with extensions similar to GNU tar.
	 */

	iftype = I_TAR;		/* command line interface */
	ptype  = P_STAR;	/* program interface type */

	if (pname) {		/* cli=xxx seen as argv[1] */
		--ac, av++;
	}
	--ac, ++av;
	files = getfilecount(ac, av, opts);
	if (getlallargs(&ac, &av, &gaprops, opts,
				&dir_flags,
				getfind, NULL,
				&help, &xhelp, &prvers, &debug, &xdebug, &xdebug,
				&bsdchdir, &paxls,
				&dumplevel, &dumpdates, &wtardumps,
				&showtime, &no_stats, &no_stats, &cpio_stats,
				&do_fifostats,
				&numeric, &verbose, &prblockno, &tpath,
#ifndef	__old__lint
				&cflag,
				&uflag,
				&rflag,
				&xflag,
				&tflag,
				&copyflag, &xcopy,
				&nflag,
				&diff_flag, add_diffopt, &diffopts,
				gethdr, &chdrtype, gethdr, &chdrtype,
				&print_artype,
				&fs_name,
				&force_hole, &force_hole, &sparse, &to_stdout, &to_stdout, &wready,
				&force_remove, &force_remove, &ask_remove, &ask_remove,
				&remove_first, &remove_first, &remove_recursive, &remove_recursive,
				&keep_nonempty_dirs, &do_install,
				&nullout, &nullout,
				&use_fifo, &no_fifo, &no_fifo, &shmflag,
				getenum, &fs,
				&volhdr,
				&listfile, &pkglistfile,
				&multivol, &newvol_script,
				&force_noremote,
				&dorestore, &dopartial, &forcerestore,
				&freezeflag,
				/*
				 * All options starting with -f need to appear
				 * before this line.
				 */
				addtarfile, NULL,
				addtarfile, NULL,
				&usetape,
				&Zflag, &zflag, &bzflag, &bzflag, &lzoflag,
				&p7zflag, &xzflag, &lzipflag, &zstdflag, &lzmaflag,
				&compress_prg,
				&rmt, &rsh,
				getenum, &bs,
				getbnum, &llbs,
				getbnum, &llbs,
				&multblk,
				addpattern, NULL,
				addpattern, NULL,
				&ignoreerr,
				&nodir,
				&nomtime, &nochown, &nochown,
				&paxopts,
				getpaxpriv, NULL,
				&acctime, &acctime,
				&pflag, &nopflag, &dirmode,
				&nolinkerr,
				&follow, &follow,
#ifdef	USE_FIND
				getpaxL, &walkflags,
				getpaxH, &walkflags,
				getpaxP, &walkflags,
#else
				getpaxL, NULL,
				getpaxH, NULL,
				getpaxP, NULL,
#endif
				&nodesc,
				&dodesc,
				&nomount, &nomount,
				&interactive, &paxinteract,
				&Ointeractive,
				getexclude, NULL,
				getexclude, NULL,
				&oldtar, &signedcksum, &signedcksum,
				&partial,
				&nospec, &Fflag,
				&uncond, &uncond_rename,
				&xdir, &xdot,
				&keep_old, &keep_old, &keep_old,
				&refresh_old, &refresh_old, &refresh_old,
				&abs_path, &allow_dotdot,
				&secure_links, &secure_links,
				&no_dirslash,
				&notpat, &notpat, &match_tree,
				&paxmatch, &paxnflag, &notarg, &notarg,
				getknum, &maxsize,
				&stampfile,
				&Ctime,
				&nodump,
				getbnum, &tsize,
				&qic24,
				&qic120,
				&qic150,
				&qic250,
				&qic525,
				&nowarn,
#endif /* __old__lint */
				&listnewf, &listnewf,
				&listnew,
				&hpdev, &modebits,
				&copylinks, &copyhardlinks, &copysymlinks,
				&copydlinks,
				&hardlinks, &symlinks, &linkdata,
				&doacl, &doxattr, &dolxattr, &dofflags,
				&link_dirs,
				&dd_name,
				&dodump, &dodump,
				&dump_cumulative, &dump_cumulative,
				&dometa, &dumpmeta, &xmeta,
				&silent, &lowmem, &no_xheader,
				&no_fsync, &no_fsync,
				&readnull,
				errconfig, NULL,
				&errflag, &dchangeflag,
				&prinodes,
				&diruid, &dirgid, &u_mask,
				parsesubst, &do_subst, &archive) < 0) {
		errmsgno(EX_BAD, "Bad Option: %s.\n", av[0]);
		susage(EX_BAD);
	}

	if (archive != -1 && !(archive >= '0' && archive <= '7')) {
		errmsgno(EX_BAD, "Bad Option: -%c.\n", archive);
		susage(EX_BAD);
	}
	star_helpvers("star", help, xhelp, prvers);

	if (Ointeractive) {
		comerrno(EX_BAD, "Option -I is obsolete and will get a different meaning in next release, use -w instead.\n");
	}
	if (xcopy) {
		copyflag = TRUE;
		sparse	 = TRUE;
		doacl	 = TRUE;
		xdot	 = TRUE;
	}
	if (tsize == 0) {
		if (qic24)  tsize = QIC_24_TSIZE;
		if (qic120) tsize = QIC_120_TSIZE;
		if (qic150) tsize = QIC_150_TSIZE;
		if (qic250) tsize = QIC_250_TSIZE;
		if (qic525) tsize = QIC_525_TSIZE;
	}
	if (pkglistfile != NULL) {
		listfile = pkglistfile;
		pkglist = TRUE;
	}
	if (u_mask) {
		long	l;

		if (*astolb(u_mask, &l, 8))
			comerrno(EX_BAD, "Bad umask '%s'.\n", u_mask);
		umask((mode_t)l);
	}
	if (diruid) {
		Llong	ll;
		uid_t	uid;

		if (!ic_uidname(diruid, strlen(diruid), &uid)) {
			if (*astollb(diruid, &ll, 10))
				comerrno(EX_BAD, "Bad uid '%s'.\n", diruid);
			dir_uid = ll;
		} else {
			dir_uid = uid;
		}
	}
	if (dirgid) {
		Llong	ll;
		gid_t	gid;

		if (!ic_gidname(dirgid, strlen(dirgid), &gid)) {
			if (*astollb(dirgid, &ll, 10))
				comerrno(EX_BAD, "Bad gid '%s'.\n", diruid);
			dir_gid = ll;
		} else {
			dir_gid = gid;
		}
	}

	if (dchangeflag)
		errconfig("WARN|GROW|SHRINK *");

	star_checkopts(oldtar, dodesc, usetape, archive, no_fifo,
			paxopts, llbs);
#ifdef	USE_FIND
	if (dofind && find_ac > 0) {
		int	cac = find_ac;
		char *const * cav = find_av;
		finda_t	fa;

		if (copyflag)
			cac--;
		find_firstprim(&cac, &cav);
		find_pac = cac;
		find_pav = cav;
		files = find_ac - cac;
		if (!copyflag && !cflag && files > 0)
			comerrno(EX_BAD, "Path arguments not yet supported in extract mode.\n");

		if (cac > 0) {
			BOOL	did_stdout = FALSE;
			int	i;

			now = time(0);
			now = now +60;
			find_argsinit(&fa);
			fa.walkflags = walkflags;
			fa.Argc = cac;
			fa.Argv = (char **)cav;
			find_node = find_parse(&fa);
			if (fa.primtype == FIND_ERRARG)
				comexit(fa.error);
			if (fa.primtype != FIND_ENDARGS)
				comerrno(EX_BAD, "Incomplete expression.\n");
			plusp = fa.plusp;
			find_patlen = fa.patlen;
			walkflags = fa.walkflags;
			maxdepth = fa.maxdepth;
			mindepth = fa.mindepth;

			for (i = 0; i < ntarfiles; i++) {
				if (tarfiles[i][0] == '-' && tarfiles[i][1] == '\0')
					did_stdout = TRUE;
			}
			if (ntarfiles == 1 && nullout)
				did_stdout = FALSE;

			if (find_node && (did_stdin || did_stdout)) {
				if (find_pname(find_node, "-exec") ||
				    find_pname(find_node, "-execdir") ||
				    find_pname(find_node, "-exec+") ||
				    find_pname(find_node, "-execdir+") ||
				    find_pname(find_node, "-ok") ||
				    find_pname(find_node, "-okdir"))
					comerrno(EX_BAD,
					"Cannot -exec with f=-.\n");
				if (cflag && did_stdout &&
				    (find_pname(find_node, "-print") ||
				    find_pname(find_node, "-print0") ||
				    find_pname(find_node, "-printnnl") ||
				    find_pname(find_node, "-ls")))
					comerrno(EX_BAD,
					"Cannot -print/-ls with f=-.\n");
			}
		}
	}
#endif
	star_nfiles(files, minfiles);
}
#endif	/* NO_STAR_MAIN */

LOCAL void
star_mkvers()
{
	char	buf[512];
extern	char	strvers[];
extern	char	dvers[];

	if (vers != NULL)
		return;

	js_snprintf(buf, sizeof (buf),
		"%s %s (%s-%s-%s) %s", "star", strvers,
			HOST_CPU, HOST_VENDOR, HOST_OS,
			dvers);

	vers = ___savestr(buf);
}

LOCAL void
star_helpvers(name, help, xhelp, prvers)
	char	*name;
	BOOL	help;
	BOOL	xhelp;
	BOOL	prvers;
{
	if (help)
		usage(0);
	if (xhelp)
		xusage(0);
	star_mkvers();
	if (prvers) {
		printf("%s: %s\n\n", name, vers);
		gtprintf("Options:");
#ifdef	USE_ACL
		opt_acl();
#endif
#ifdef	USE_FIND
		printf(" find");
#endif
#ifdef	USE_FFLAGS
		opt_fflags();
#endif
#ifdef	USE_REMOTE
		opt_remote();
#endif
#ifdef	USE_XATTR
		opt_xattr();
#endif
#ifdef	USE_SELINUX
		opt_selinux();
#endif
		gtprintf("\n\n");
		gtprintf("Copyright (C) 1985, 88-90, 92-96, 98, 99, 2000-2020 Jörg Schilling\n");
		gtprintf("This is free software; see the source for copying conditions.  There is NO\n");
		gtprintf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}
}

LOCAL void
star_checkopts(oldtar, dodesc, usetape, archive, no_fifo, paxopts, llbs)
	BOOL		oldtar;		/* -O oldtar option		*/
	BOOL		dodesc;		/* -dodesc descend dirs from listfile */
	BOOL		usetape;	/* -T usetape option		*/
	int		archive;	/* -0 .. -9 archive option	*/
	BOOL		no_fifo;	/* -no-fifo option		*/
	const char *	paxopts;	/* -pax-o / -o option		*/
	Llong		llbs;		/* blocks= option		*/
{
	int	n;

	if (print_artype) {
		tflag = TRUE;
		no_fifo = TRUE;
	}
	if ((n = xflag + cflag + uflag + rflag + tflag + copyflag + nflag + diff_flag) > 1) {
		if ((n == 2) && copyflag && (tflag || diff_flag)) {
			/*
			 * This is OK: star -copy -t or star -copy -diff
			 */
			/* EMPTY */
		} else if ((n == 2) && cflag && (tflag || diff_flag)) {
			copyflag = TRUE;
			cflag = FALSE;
		} else {
			errmsgno(EX_BAD,
			"Too many commands, only one of -x -c -u -r -t -copy -n or -diff is allowed.\n");
			susage(EX_BAD);
		}
	}
	if (!(xflag | cflag | uflag | rflag | tflag | copyflag | nflag | diff_flag)) {
		errmsgno(EX_BAD, "Missing command, must specify -x -c -u -r -t -copy -n or -diff.\n");
		susage(EX_BAD);
	}
	if (uflag || rflag) {
		cflag = TRUE;
		no_fifo = TRUE;	/* Until we are able to reverse the FIFO */
		dump_partial = TRUE;
	}
	if (nullout && !cflag) {
		errmsgno(EX_BAD, "-nullout only makes sense in create mode.\n");
		susage(EX_BAD);
	}
	if (no_fifo || nullout)
		use_fifo = FALSE;
#ifndef	FIFO
	if (use_fifo) {
		errmsgno(EX_BAD, "Fifo not configured in.\n");
		susage(EX_BAD);
	}
#endif

	if (ptype == P_SUNTAR)
		nolinkerr ^= tcompat;

	noxdir = nodir;
	if (xdir)		/* Extract all dirs uncond */
		xdot = FALSE;

	if (copylinks) {
		copyhardlinks = TRUE;
		copysymlinks = TRUE;
	}
	if (copyflag) {
		hdrtype = chdrtype = H_EXUSTAR;
		dodump = TRUE;
		partial = TRUE;	/* Important as we fiddle with FIFO obs */
		binflag = TRUE;
		nodir = FALSE;
		multivol = FALSE;
		linkdata = FALSE;
		if (secure_links < 0)
			secure_links = FALSE;

		if (!tflag && !diff_flag)
			xflag = TRUE;

		if (!use_fifo) {
			errmsgno(EX_BAD, "Need fifo for -copy mode.\n");
			susage(EX_BAD);
		}
	}
	if (cflag && linkdata && sparse)
		linkdata = FALSE;	/* XXX Cannot yet do sparse datalinks */

	if (dumplevel >= 0) {
		/*
		 * This is an articicial limitation, our code supports an
		 * unlimited number of dump levels.
		 */
		if (dumplevel > 99)
			comerrno(EX_BAD, "Illegal dump level, use 0..99\n");
		dodump = TRUE;
		if (!nomount)
			comerrno(EX_BAD, "A dump needs the -M/-xdev option.\n");
	}
	if (dodump) {
		chdrtype = H_EXUSTAR;
		if (lowmem)
			comerrno(EX_BAD, "Dump mode does not work with -lowmem.\n");
	}
	if (dump_cumulative) {
		if (dumplevel < 0)
			comerrno(EX_BAD, "With -cumulative, level= is needed.\n");
	}
	if (maxsize > 0 || Fflag > 0 || nospec || nodump)
		dump_partial = TRUE;

	if (dopartial)
		dorestore = TRUE;
	if (forcerestore)
		dorestore = TRUE;
	if (dorestore) {
		xdir = TRUE;
		if (secure_links < 0)
			secure_links = FALSE;
		if (!uncond)
			comerrno(EX_BAD, "A restore needs the -U option.\n");
		if (do_install)
			comerrno(EX_BAD, "-install not allowed in restore mode.\n");
	}
	if (oldtar)
		chdrtype = H_OTAR;
	if (chdrtype != H_UNDEF) {
		if (H_TYPE(chdrtype) == H_OTAR)
			oldtar = TRUE;	/* XXX hack */
	}
	/*
	 * We do not set chdrtype here in case it is H_UNDEF and -r or -u have
	 * been specified.
	 */
	if (cflag && (!(rflag || uflag) || chdrtype != H_UNDEF)) {
		if (chdrtype != H_UNDEF)
			hdrtype = chdrtype;
		chdrtype = hdrtype;	/* wegen setprops(chdrtype) in main() */

		/*
		 * hdrtype und chdrtype
		 * bei uflag, rflag sowie xflag, tflag, nflag, diff_flag
		 * in get_tcb vergleichen !
		 */
	}
	if (no_dirslash && chdrtype == H_OTAR) {
		errmsgno(EX_BAD, "-no-dirslash cannot be used with the old tar format\n");
		susage(EX_BAD);
	}
	if (diff_flag) {
		if (diffopts == 0)
			diffopts = D_DEFLT;
		if ((diffopts & D_ATIME) == 0)
			diffopts &= ~D_ANTIME;
		if ((diffopts & D_MTIME) == 0)
			diffopts &= ~D_MNTIME;
		if ((diffopts & D_CTIME) == 0)
			diffopts &= ~D_CNTIME;
	} else if (diffopts != 0) {
		errmsgno(EX_BAD, "diffopts= only makes sense with -diff\n");
		susage(EX_BAD);
	}
	if (fs == 0L) {
		char	*ep = getenv("STAR_FIFOSIZE");

		if (ep) {
			if (getnum(ep, &fs) != 1) {
				comerrno(EX_BAD,
					"Bad fifo size environment '%s'.\n",
									ep);
			}
		}
	}
	if (llbs != 0 && bs != 0) {
		errmsgno(EX_BAD, "Only one of blocks= b= bs=.\n");
		susage(EX_BAD);
	}
	if (llbs != 0) {
		bs = (long)llbs;
		if (bs != llbs) {
			errmsgno(EX_BAD, "Blocksize used with blocks= or b= too large.\n");
			susage(EX_BAD);
		}
	}
	if (bs % TBLOCK) {
		errmsgno(EX_BAD, "Invalid block size %ld.\n", bs);
		susage(EX_BAD);
	}
	if (bs)
		nblocks = bs / TBLOCK;
	if ((nblocks <= 0) ||
	    ((rflag || uflag) && nblocks < 2)) {
		errmsgno(EX_BAD, "Invalid block size %d blocks.\n", nblocks);
		susage(EX_BAD);
	}
	bs = nblocks * TBLOCK;
	if (debug) {
		errmsgno(EX_BAD, "Block size %d blocks (%ld bytes).\n", nblocks, bs);
	}
	if (tsize > 0) {
		if (tsize % TBLOCK) {
			errmsgno(EX_BAD, "Invalid tape size %llu.\n", tsize);
			susage(EX_BAD);
		}
		tsize /= TBLOCK;
	}

	if (pkglist) {
		dodesc = FALSE;
		readnull = FALSE;
		if (!cflag)
			comerrno(EX_BAD, "pkglist= option only works in create mode.\n");
	}
	if (listfile != NULL)
		dump_partial = TRUE;

	if (listfile != NULL && !dodesc)
		nodesc = TRUE;
	if (oldtar)
		nospec = TRUE;
	if (!tarfiles[0]) {
		if (usetape) {
			tarfiles[0] = getenv("TAPE");
		}
		if ((usetape || archive > 0) &&
		    !tarfiles[0]) {
			static char	arname[] = "archive0=";
				Ullong	otsize = tsize;
				char	*dfltfile = NULL;

#ifdef	DFLT_FILE
#define	DFILE	DFLT_FILE
#else
#define	DFILE	NULL
#endif
			/*
			 * If we got a digit option, check for an 'archive#='
			 * entry in /etc/default/[s!]tar. If there was no -f
			 * or digit option, look for 'archive0='.
			 */
			if (archive < '0' || archive > '9')
				archive = '0';
			arname[7] = (char)archive;
			if (ptype == P_SUNTAR)
				dfltfile = DFILE;
			if (!star_darchive(arname, dfltfile)) {
				errmsgno(EX_BAD,
					"Archive entry %c not found in %s. %s",
					archive,
					get_stardefaults(DFILE),
					"Using stdin/stdout as archive.\n");
				tarfiles[0] = NULL;
				tsize = otsize;
			}
		}
		if (!tarfiles[0])
			tarfiles[0] = "-";
		ntarfiles++;
	}
	if (!cflag && !copyflag) {
		for (n = 0; n < ntarfiles; n++) {
			if (tarfiles[n][0] == '-' && tarfiles[n][1] == '\0')
				check_stdin("-f");
		}
	}
	if (tsize % nblocks) {
		/*
		 * Silently round down to a multiple of the tape block size.
		 */
		tsize /= nblocks;
		tsize *= nblocks;
	}
	/*
	 * XXX This must be rethought with files split by multi volume and
	 * XXX with with volume headers and continuation headers.
	 */
	if (tsize > 0 && tsize < 3) {
		errmsgno(EX_BAD, "Tape size must be at least 3 blocks.\n");
		susage(EX_BAD);
	}
	/*
	 * XXX This is a place that should be checked every time, when
	 * XXX possible interactivity is modified.
	 */
	if (interactive || ask_remove ||
	    ((multivol || tsize > 0) && !newvol_script)) {
#ifdef	JOS
		tty = stderr;
#else
#ifdef	HAVE__DEV_TTY
		if ((tty = lfilemopen("/dev/tty", "r", S_IRWALL)) == (FILE *)NULL)
			comerr("Cannot open '/dev/tty'.\n");
#else
		tty = stderr;
#endif
#endif
	}
	if (nflag) {
		xflag = TRUE;
		interactive = TRUE;
		if (verbose == 0 && !tpath)
			verbose = 1;
	}
	if (to_stdout) {
		force_hole = FALSE;
	}
	if (keep_old && refresh_old) {
		errmsgno(EX_BAD, "Cannot use -keep-old-files and -refresh-old-files together.\n");
		susage(EX_BAD);
	}
	if ((copylinks + hardlinks + symlinks) > 1) {
		errmsgno(EX_BAD, "Only one of -copylinks -hardlinks -symlinks.\n");
		susage(EX_BAD);
	}

	if (my_uid == 0 && !nopflag)
		pflag = TRUE;

	/*
	 * -acl includes -p
	 */
	if (doacl)
		pflag = TRUE;

	if (doxattr) {
#ifndef	O_XATTR
		errmsgno(EX_BAD,
		"This platform does not support NFSv4 extended attribute files.\n");
		comerrno(EX_BAD,
		"-xattr is reserved for NFSv4 extended attributes, for Linux use -xattr-linux\n");
#else
		/*
		 * XXX: see getpaxpriv() "pax -pe": doxattr commented out.
		 */
		comerrno(EX_BAD,
		"NFSv4 extended attribute files are not yet supported.\n");
#endif
	}

	if (paxopts) {
		if (!ppaxopts(paxopts)) {
			errmsgno(EX_BAD, "Unsupported option '%s' for %s.\n",
					paxopts,
					iftype == I_PAX ? "-o" : "-pax-o");
			susage(EX_BAD);
		}
	}

	star_defaults(&fs, &no_fsync, &secure_links, NULL);
	if (no_fsync < 0)
		no_fsync = FALSE;
	if (secure_links < 0)
		secure_links = TRUE;
}

EXPORT void
star_verifyopts()
{
	if (cflag && (props.pr_flags & PR_LINK_DATA) == 0)
		linkdata = FALSE;
	if (cflag && multivol && (props.pr_flags & PR_MULTIVOL) == 0) {
		errmsgno(EX_BAD,
		"Multi volume archives are not supported with %s format.\n",
		hdr_name(chdrtype));
		susage(EX_BAD);
	}
	if (cflag && doacl) {
		/*
		 * Check properties for archive format.
		 */
		if ((props.pr_xhmask & (XF_ACL_ACCESS|XF_ACL_DEFAULT|XF_ACL_ACE)) == 0) {
			errmsgno(EX_BAD,
				"Archive format '%s' does not support -acl.\n",
							hdr_name(chdrtype));
			susage(EX_BAD);
		}
	}
}

LOCAL void
star_nfiles(files, minfiles)
	int	files;
	int	minfiles;
{
	if (cflag || copyflag) {
		if (copyflag && !tflag)
			minfiles++;
		if (listfile)
			minfiles--;
		if (files < minfiles) {
			errmsgno(EX_BAD, "Too few arguments; will not create an empty archive.\n");
			susage(EX_BAD);
		}
	}
}

/* ARGSUSED */
LOCAL int
getpaxH(arg, valp, pac, pav)
	char	*arg;
	long	*valp;
	int	*pac;
	char	*const	**pav;
{
#ifdef	GETARG_DEBUG
	error("paxH\n");
#endif
	paxfollow = FALSE;
	paxHflag = TRUE;
#ifdef	USE_FIND
	*(int *)valp |= WALK_ARGFOLLOW;
#endif
	return (1);
}

/* ARGSUSED */
LOCAL int
getpaxL(arg, valp, pac, pav)
	char	*arg;
	long	*valp;
	int	*pac;
	char	*const	**pav;
{
#ifdef	GETARG_DEBUG
	error("paxL\n");
#endif
	paxfollow = TRUE;
	paxHflag = FALSE;
#ifdef	USE_FIND
	*(int *)valp |= WALK_ALLFOLLOW;
#endif
	return (1);
}

/* ARGSUSED */
LOCAL int
getpaxP(arg, valp, pac, pav)
	char	*arg;
	long	*valp;
	int	*pac;
	char	*const	**pav;
{
#ifdef	GETARG_DEBUG
	error("paxP\n");
#endif
	paxfollow = FALSE;
	paxHflag = FALSE;
#ifdef	USE_FIND
	*(int *)valp &= ~(WALK_ARGFOLLOW | WALK_ALLFOLLOW);
#endif
	return (1);
}

/* ARGSUSED */
LOCAL int
getfind(arg, valp, pac, pav)
	char	*arg;
	long	*valp;	/* Not used until we introduce a ptr to opt struct */
	int	*pac;
	char	*const	**pav;
{
#ifdef	USE_FIND
	dofind = TRUE;
	find_ac = *pac;
	find_av = *pav;
	find_ac--, find_av++;
	return (NOARGS);
#else
	return (BADFLAG);
#endif
}

/* ARGSUSED */
LOCAL int
getpaxpriv(arg, valp)
	char	*arg;
	long	*valp;	/* Not used until we introduce a ptr to opt struct */
{
	register char	*p = arg;
	register char	c;

	while ((c = *p++) != '\0') {
		switch (c) {

		case 'a':	/* do not preserve access time */
			noatime = TRUE;
			break;

		case 'e':	/* preserve everything */
			pflag = TRUE;
			doacl = TRUE;
#if 0
			/*
			 * XXX: see doxattr check above.
			 * XXX: We disable this to pass a solaris-ON compilation
			 * XXX: "NFSv4 extended attribute files are not yet..."
			 */
			doxattr = TRUE;
#endif
			dolxattr = TRUE;
			dofflags = TRUE;
			noatime = FALSE;
			nomtime = FALSE;
			nochown = FALSE;
			break;

		case 'm':	/* do not preserve modification time */
			nomtime = TRUE;
			break;

		case 'o':	/* preserve userid/grupid & SUID/SGID */
			nochown = FALSE;
			break;

		case 'p':	/* preserve file mode bits (permissions) */
			pflag = TRUE;
			break;

		default:
			errmsgno(EX_BAD,
				"Bad character '%c' in option '-p %s'.\n",
				c, arg);
			return (-1);
		}
	}
	return (1);
}

LOCAL int
getlldefault(arg, valp, mult)
	char	*arg;
	Llong	*valp;
	int	mult;
{
	int	ret = 1;
	int	len = strlen(arg);

	if (len > 0) {
		len = (Uchar)arg[len-1];
		if (!isdigit(len))
			mult = 1;
	}
	ret = getllnum(arg, valp);
	if (ret == 1)
		*valp *= mult;
	else
		errmsgno(EX_BAD, "Badly formed number '%s'.\n", arg);
	return (ret);
}

EXPORT int
getbnum(arg, valp)
	char	*arg;
	Llong	*valp;
{
	return (getlldefault(arg, valp, 512));
}

EXPORT int
getknum(arg, valp)
	char	*arg;
	Llong	*valp;
{
	return (getlldefault(arg, valp, 1024));
}

LOCAL int
getenum(arg, valp)
	char	*arg;
	long	*valp;
{
	int ret = getnum(arg, valp);

	if (ret != 1)
		errmsgno(EX_BAD, "Badly formed number '%s'.\n", arg);
	return (ret);
}

LOCAL int
addtarfile(tarfile)
	const char	*tarfile;
{
#ifdef	ADDARG_DEBUG
	if (debug)
		error("Add tar file '%s'.\n", tarfile);
#endif

	if (ntarfiles >= NTARFILE)
		comerrno(EX_BAD, "Too many tar files (max is %d).\n", NTARFILE);

	if (ntarfiles > 0 && (streql(tarfile, "-") || streql(tarfiles[0], "-")))
		comerrno(EX_BAD, "Cannot handle multi volume archives from/to stdin/stdout.\n");

	tarfiles[ntarfiles] = tarfile;
	ntarfiles++;
	return (TRUE);
}

LOCAL int
add_diffopt(optstr, flagp)
	char	*optstr;
	long	*flagp;
{
	char	*ep;
	char	*np;
	int	optlen;
	long	optflags = 0;
	BOOL	not = FALSE;

	while (*optstr) {
		if ((ep = strchr(optstr, ',')) != NULL) {
			Intptr_t	pdiff = ep - optstr;

			optlen = (int)pdiff;
			if (optlen != pdiff)	/* lint paranoia */
				return (-1);
			np = &ep[1];
		} else {
			optlen = strlen(optstr);
			np = &optstr[optlen];
		}
		if (optstr[0] == '!') {
			optstr++;
			optlen--;
			not = TRUE;
		}
		if (strncmp(optstr, "not", optlen) == 0 ||
				strncmp(optstr, "!", optlen) == 0) {
			not = TRUE;
		} else if (strncmp(optstr, "all", optlen) == 0) {
			optflags |= D_ALL;
		} else if (strncmp(optstr, "perm", optlen) == 0) {
			optflags |= D_PERM;
		} else if (strncmp(optstr, "mode", optlen) == 0) {
			optflags |= D_PERM;
		} else if (strncmp(optstr, "symperm", optlen) == 0) {
			optflags |= D_SYMPERM;
		} else if (strncmp(optstr, "type", optlen) == 0) {
			optflags |= D_TYPE;
		} else if (strncmp(optstr, "nlink", optlen) == 0) {
			optflags |= D_NLINK|D_DNLINK;
		} else if (strncmp(optstr, "dnlink", optlen) == 0) {
			optflags |= D_DNLINK;
		} else if (strncmp(optstr, "uid", optlen) == 0) {
			optflags |= D_UID;
		} else if (strncmp(optstr, "gid", optlen) == 0) {
			optflags |= D_GID;
		} else if (strncmp(optstr, "uname", optlen) == 0) {
			optflags |= D_UNAME;
		} else if (strncmp(optstr, "gname", optlen) == 0) {
			optflags |= D_GNAME;
		} else if (strncmp(optstr, "id", optlen) == 0) {
			optflags |= D_ID;
		} else if (strncmp(optstr, "size", optlen) == 0) {
			optflags |= D_SIZE;
		} else if (strncmp(optstr, "data", optlen) == 0) {
			optflags |= D_DATA;
		} else if (strncmp(optstr, "cont", optlen) == 0) {
			optflags |= D_DATA;
		} else if (strncmp(optstr, "rdev", optlen) == 0) {
			optflags |= D_RDEV;
		} else if (strncmp(optstr, "hardlink", optlen) == 0) {
			optflags |= D_HLINK;
		} else if (strncmp(optstr, "symlink", optlen) == 0) {
			optflags |= D_SLINK;
		} else if (strncmp(optstr, "sympath", optlen) == 0) {
			optflags |= D_SLPATH;
		} else if (strncmp(optstr, "sparse", optlen) == 0) {
			optflags |= D_SPARS;
		} else if (strncmp(optstr, "atime", optlen) == 0) {
			optflags |= D_ATIME;
		} else if (strncmp(optstr, "mtime", optlen) == 0) {
			optflags |= D_MTIME;
		} else if (strncmp(optstr, "ctime", optlen) == 0) {
			optflags |= D_CTIME;
		} else if (strncmp(optstr, "lmtime", optlen) == 0) {
			optflags |= D_LMTIME;
		} else if (strncmp(optstr, "times", optlen) == 0) {
			optflags |= D_TIMES;
		} else if (strncmp(optstr, "xtimes", optlen) == 0) {
			optflags |= D_XTIMES;
		} else if (strncmp(optstr, "nsecs", optlen) == 0) {
			optflags |= D_ANTIME|D_MNTIME|D_CNTIME;
		} else if (strncmp(optstr, "dir", optlen) == 0) {
			optflags |= D_DIR;
#ifdef USE_ACL
		} else if (strncmp(optstr, "acl", optlen) == 0) {
			optflags |= D_ACL;
#endif
#ifdef USE_XATTR
		} else if (strncmp(optstr, "xattr", optlen) == 0) {
			optflags |= D_XATTR;
#endif
#ifdef USE_FFLAGS
		} else if (strncmp(optstr, "fflags", optlen) == 0) {
			optflags |= D_FFLAGS;
#endif
		} else if (strncmp(optstr, "help", optlen) == 0) {
			dusage(0);
		} else {
			error("Illegal diffopt.\n");
			dusage(EX_BAD);
			return (-1);
		}
		optstr = np;
	}
	if (not)
		optflags = ~optflags;

	if ((optflags & D_MTIME) == 0)
		optflags &= ~D_LMTIME;

	if ((optflags & D_SLINK) == 0)
		optflags &= ~D_SLPATH;

	*flagp = optflags;

	return (TRUE);
}

LOCAL int
gethdr(optstr, typep)
	char	*optstr;
	long	*typep;
{
	BOOL	swapped = FALSE;
	long	type	= H_UNDEF;

	if (*optstr == 'S') {
		swapped = TRUE;
		optstr++;
	}
	if (streql(optstr, "help")) {
		husage(0);
	} else if ((type = hdr_type(optstr)) < 0) {
		error("Illegal header type '%s'.\n", optstr);
		husage(EX_BAD);
		return (-1);
	}
	if (swapped)
		*typep = H_SWAPPED(type);
	else
		*typep = type;
	return (TRUE);
}

/* ARGSUSED */
LOCAL int
getexclude(arg, valp, pac, pav)
	char	*arg;
	long	*valp;
	int	*pac;
	char	*const	**pav;
{
	FILE	*xf;

	if (streql(arg, "-")) {
		check_stdin("-X");
		xf = stdin;
	} else if ((xf = lfilemopen(arg, "r", S_IRWALL)) == (FILE *)NULL)
		comerr("Cannot open '%s'.\n", arg);
	hash_xbuild(xf);
	fclose(xf);
	return (1);
}

#ifdef	USED
/*
 * Add archive file.
 * May currently not be activated:
 *	If the option string ends with ",&", the -C option will not work
 *	anymore.
 */
LOCAL int
addfile(optstr, dummy)
	char	*optstr;
	long	*dummy;
{
	char	*p;

#ifdef	ADDARG_DEBUG
	error("got_it: %s\n", optstr);
#endif

	if (!strchr("01234567", optstr[0]))
		return (NOTAFILE); /* Tell getargs that this may be a flag */

	for (p = &optstr[1]; *p; p++) {
		if (*p != 'l' && *p != 'm' && *p != 'h')
			return (BADFLAG);
	}
#ifdef	ADDARG_DEBUG
	error("is_tape: %s\n", optstr);
#endif

	comerrno(EX_BAD, "Options [0-7][lmh] currently not supported.\n");
	/*
	 * The tape device should be determined from the defaults file
	 * in the near future.
	 * Search for /etc/opt/schily/star, /etc/default/star, /etc/default/tar
	 */

	return (1);		/* Success */
}
#endif

EXPORT void
set_signal(sig, handler)
	int		sig;
	RETSIGTYPE	(*handler)	__PR((int));
{
#if	defined(HAVE_SIGPROCMASK) && defined(SA_RESTART)
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	(void) sigaction(sig, &sa, (struct sigaction *)0);
#else
#ifdef	HAVE_SIGSETMASK
	struct sigvec	sv;

	sv.sv_mask = 0;
	sv.sv_handler = handler;
	sv.sv_flags = 0;
	(void) sigvec(sig, &sv, (struct sigvec *)0);
#else
	(void) signal(sig, handler);
#endif
#endif
}

LOCAL void
exsig(sig)
	int	sig;
{
	(void) signal(sig, SIG_DFL);
	kill(getpid(), sig);
}

/* ARGSUSED */
LOCAL void
sighup(sig)
	int	sig;
{
#ifdef	SIGHUP
	set_signal(SIGHUP, sighup);
#endif
	prstats();
	intr++;
	if (!cflag)
		exsig(sig);
}

/* ARGSUSED */
LOCAL void
sigintr(sig)
	int	sig;
{
#ifdef	SIGINT
	set_signal(SIGINT, sigintr);
#endif
	prstats();
	intr++;
	if (!cflag)
		exsig(sig);
}

/* ARGSUSED */
LOCAL void
sigquit(sig)
	int	sig;
{
	/*
	 * sig may be either SIGQUIT or SIGINFO (*BSD only).
	 */
	set_signal(sig, sigquit);
	prstats();
}

LOCAL void
getstamp()
{
	FINFO	finfo;
	BOOL	ofollow = follow;

	follow = TRUE;
	if (!_getinfo(stampfile, &finfo))
		comerr("Cannot stat '%s'.\n", stampfile);
	follow = ofollow;

	Newer.tv_sec = finfo.f_mtime;
	Newer.tv_nsec = finfo.f_mnsec;
}

LOCAL const char *
has_cli(ac, av)
	int	ac;
	char	*const *av;
{
	if (ac > 1) {
		const char	*p = av[1];

		if (p[0] == 'c' && p[1] == 'l' && p[2] == 'i' && p[3] == '=')
			return (&p[4]);
	}
	return (NULL);
}

LOCAL struct clis {
	char	*name;
	int	type;
} clis[] = {
	{ "star",	P_STAR },
	{ "suntar",	P_SUNTAR },
	{ "tar",	P_SUNTAR },
	{ "gnutar",	P_GNUTAR },
	{ "gtar",	P_GNUTAR },
	{ "pax",	P_PAX },
	{ "cpio",	P_CPIO },
	{ NULL,		C_NONE },
};

LOCAL int
get_ptype(p)
	const char	*p;
{
	struct	clis	*clp = clis;

	while (clp->name) {
		if (streql(clp->name, p))
			return (clp->type);
		clp++;
	}

	return (C_NONE);
}

LOCAL void
set_ptype(pac, pav)
	int	*pac;
	char	*const **pav;
{
	int	ac		= *pac;
	char	*const *av	= *pav;
const	char	*p;

	if ((p = has_cli(ac, av)) != NULL) {
		ptype = get_ptype(p);
		if (ptype == C_NONE) {
			struct	clis	*clp = clis;

			errmsgno(EX_BAD, "Illegal cli name '%s'.\n", p);
			errmsgno(EX_BAD, "Use one of:");
			while (clp->name) {
				error(" %s", (clp++)->name);
			}
			error(".\n\n");
			susage(EX_BAD);
		}
		set_progname((pname = p));
		return;
	}

	p = filename(av[0]);

	/*
	 * If you like different behavior, you need to insert exceptional
	 * code before the switch statement.
	 *
	 * These are the names we support:
	 *
	 *	cpio gnutar pax star suntar ustar
	 */
	switch (p[0]) {

	case 'c':			/* 'c'pio */
		ptype = P_CPIO;
		return;

	case 'g':			/* 'g'*tar */
		ptype = P_GNUTAR;
		return;

	case 'p':			/* 'p'ax */
		ptype = P_PAX;
		return;

	case 't':			/* 't'ar */
		/*
		 * If we put something different here (e.g. P_STAR), we may
		 * set the default behavior to be the behavor of 'star'.
		 */
		ptype = P_SUNTAR;
		return;
	case 's':
		if (streql(p, "suntar")) {
			ptype = P_SUNTAR;
			return;
		}
		if (streql(p, "scpio")) {
			ptype = P_CPIO;
			return;
		}
		if (streql(p, "spax")) {
			ptype = P_PAX;
			return;
		}
		/* FALLTHRU */

	case 'u':			/* 'u'star */
		ptype = P_STAR;
		return;
	default:
		ptype = PTYPE_DEFAULT;
		return;
	}
}

/* BEGIN CSTYLED */
/*
 * Convert old tar type syntax into the new UNIX option syntax.
 * Allow only a limited subset of the single character options to avoid
 * collisions between interpretation of options in different
 * tar implementations. The old syntax has a risk to damage files
 * which is avoided with the 'fcompat' flag (see opentape()).
 *
 * The UNIX-98 documentation lists the following tar options:
 *	Function Key:	crtux
 *			c	Create
 *			r	Append
 *			t	List
 *			u	Update
 *			x	Extract
 *	Additional Key:	vwfblmo
 *			v	Verbose
 *			w	Wait for confirmation
 *			f	Archive file
 *			b	Blocking factor
 *			l	Report missing links
 *			m	Do not restore mtime from archive
 *			o	Do not restore owner/group from archive
 *
 *	Incompatibilities with UNIX-98 tar:
 *			l	works the oposite way round as with star, but
 *				if TAR_COMPAT is defined, star will behave
 * 				as documented in UNIX-98 if av[0] is either
 *				"tar" or "ustar".
 *
 *	Additional functions from historic UNIX tar versions:
 *			0..7	magtape_0 .. magtape_7
 *
 *	Additional functions from historic BSD tar versions:
 *			p	Extract dir permissions too
 *			h	Follow symbolic links
 *			i	ignore directory checksum errors
 *			B	do multiple reads to reblock pipes
 *			F	Ommit unwanted files (e.g. core)
 *
 *	Additional functions from historic Star versions:
 *			T	Use $TAPE environment as archive
 *			L	Follow symbolic links
 *			d	do not archive/extract directories
 *			k	keep old files
 *			n	do not extract but tell what to do
 *
 *	Additional functions from historic SVr4 tar versions:
 *			e	Exit on unexpected errors
 *			X	Arg is File with unwanted filenames
 *
 *	Additional functions from historic GNU tar versions:
 *			z	do inline compression
 *
 *	Missing in star (from SVr4/Solaris tar):
 *			E	Extended headers
 *			P	Supress '/' at beginning of filenames
 *			q	quit after extracting first file
 *	Incompatibilities with SVr4/Solaris tar:
 *			I	Arg is File with filenames to be included
 *				The -I option is not handled as option.
 *			P	SVr4/solaris: Supress '/', star: last partial
 *			k	set tape size for multi volume archives
 *			n	non tape device (do seeks)
 *
 *	Incompatibilities with GNU tar:
 *		There are many. GNU programs in many cases make smooth
 *		coexistence hard.
 *
 * Problems:
 *	The 'e' and 'X' option are currently not implemented.
 *	There is a collision between the BSD -I (include) and
 *	star's -I (interactive) which may be solved by using -w instead.
 */
/* END CSTYLED */
LOCAL void
docompat(pac, pav)
	int	*pac;
	char	*const **pav;
{
	int	ac		= *pac;
	char	*const *av	= *pav;
	int	nac;
	char	**nav;
	char	nopt[3];
	char	*_copt = "crtuxbfXBFTLdehijklmnopvwz01234567";
	char	*copt = _copt;
const	char	*p;
	char	c;
	char	*const *oa;
	char	**na;
	int	coff = 1;

	set_ptype(pac, pav);
	switch (ptype) {

	case P_SUNTAR:
		iftype = I_TAR;
#ifdef	SUN_TAR
		copt = sun_copt;
#endif
		break;
	case P_GNUTAR:
		iftype = I_TAR;
#ifdef	GNU_TAR
		copt = gnu_copt;
#endif
		break;
	case P_PAX:
		iftype = I_PAX;
		break;
	case P_CPIO:
		iftype = I_CPIO;
		break;
	case P_STAR:
	default:
		iftype = I_TAR;
		copt = _copt;
	}

	/*
	 * We must check this first to be able to set 'tcompat'.
	 */
	p = filename(av[0]);
	if (streql(p, "tar") || streql(p, "ustar") ||
	    streql(p, "suntar") || streql(p, "gnutar") || streql(p, "gtar"))
		tcompat = TRUE;

	if (pname)				/* cli=xxx seen as argv[1] */
		coff++;

	if (ac <= coff)
		return;
	if (iftype != I_TAR)
		return;


	/*
	 * If we come here, this is for a tar type CLI.
	 */
	if (ptype != P_SUNTAR && ptype != P_GNUTAR) {
		/*
		 * For "suntar" & "gnutar" the first option string may start
		 * with '-', and still may need a conversion, else only convert
		 * to new syntax if the first arg is a non '-' arg.
		 */
		if (av[1][0] == '-')		/* Do not convert new syntax */
			return;
	}
	if (av[coff][0] == '-' && av[coff][1] == '-')	/* Do not convert new syntax */
		return;

	if (strchr(av[coff], '=') != NULL)		/* Do not try to convert bs= */
		return;

	nac = ac + strlen(av[coff]);
	nav = ___malloc(nac-- * sizeof (char *), /* keep space for NULL ptr */
				"compat argv");
	oa = av;				/* remember old arg pointer */
	na = nav;				/* set up new arg pointer */
	*na++ = *oa++;				/* copy over av[0] */
	if (coff > 1)				/* If we have a cli= argument */
		*na++ = *oa++;			/* copy over av[1] (cli=...) */
	oa++;					/* Skip over av[coff] */

	nopt[0] = '-';
	nopt[2] = '\0';

	for (p = av[coff]; (c = *p) != '\0'; p++) {
		if (c == '-') {
			nac--;
			continue;
		}
		if (strchr(copt, c) == NULL) {
			if (ptype == P_SUNTAR || ptype == P_GNUTAR)
				errmsgno(EX_BAD, "Illegal option '%c'.\n", c);
			else
				errmsgno(EX_BAD, "Illegal option '%c' for compat mode.\n", c);

			susage(EX_BAD);
		}
		nopt[1] = c;
		*na++ = ___savestr(nopt);

		if (c == 'f' || c == 'b' || (ptype == P_SUNTAR && c == 'k') || c == 'X') {
			if ((av + ac) <= oa) {
				comerrno(EX_BAD,
					"Missing arg for '%s' option.\n",
					nopt);
			}
			*na++ = *oa++;
			/*
			 * The old syntax has a high risk of corrupting
			 * files if the user disorders the args.
			 */
			if (c == 'f')
				fcompat = TRUE;
		}
	}

	/*
	 * Now copy over the rest...
	 */
	while ((av + ac) > oa)
		*na++ = *oa++;
	*na = NULL;

	*pac = nac;
	*pav = nav;

#ifdef	COMPAT_DEBUG
	{ int	i;
		printf("agrc: %d\n", nac);
		for (i = 0; i < nac; i++)
			printf("%i: '%s'\n", i, nav[i]);
	}
#endif
}

EXPORT BOOL
ttyerr(f)
	FILE	*f;
{
	/*
	 * We may get EIO in case that we received an ignored SIGTTIN.
	 */
	if (ferror(f)) {
		errmsgno(EX_BAD, "No access to tty.\n");
		return (TRUE);
	}
	if (feof(f)) {
		errmsgno(EX_BAD, "EOF on tty.\n");
		return (TRUE);
	}
	return (FALSE);
}
