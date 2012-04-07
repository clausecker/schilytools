/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
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

#ifndef	_DEFS_H
#define	_DEFS_H

#if defined(sun)
#pragma ident	"@(#)defs.h	1.28	08/01/29 SMI"
#endif

/*
 * This file contains modifications Copyright 2008-2012 J. Schilling
 *
 * @(#)defs.h	1.34 12/04/07 2008-2012 J. Schilling
 */

#ifdef	__cplusplus
extern "C" {
#endif

/*
 *	UNIX shell
 */

/* execute flags */
#define		XEC_EXECED	01
#define		XEC_LINKED	02
#define		XEC_NOSTOP	04

/* endjobs flags */
#define		JOB_STOPPED	01
#define		JOB_RUNNING	02

/* error exits from various parts of shell */
#define		ERROR		1
#define		SYNBAD		2
#define		SIGFAIL 	2000
#define		SIGFLG		0200

/* command tree */
#define		FPIN		0x0100
#define		FPOU		0x0200
#define		FAMP		0x0400
#define		COMMSK		0x00F0
#define		CNTMSK		0x000F

#define		TCOM		0x0000
#define		TPAR		0x0010
#define		TFIL		0x0020
#define		TLST		0x0030
#define		TIF		0x0040
#define		TWH		0x0050
#define		TUN		0x0060
#define		TSW		0x0070
#define		TAND		0x0080
#define		TORF		0x0090
#define		TFORK		0x00A0
#define		TFOR		0x00B0
#define		TFND		0x00C0

/* execute table */
#define		SYSSET		1
#define		SYSCD		2
#define		SYSEXEC		3

#ifdef RES	/*	include login code	*/
#define		SYSLOGIN	4
#else
#define		SYSNEWGRP 	4
#endif

#define		SYSTRAP		5
#define		SYSEXIT		6
#define		SYSSHFT 	7
#define		SYSWAIT		8
#define		SYSCONT 	9
#define		SYSBREAK	10
#define		SYSEVAL 	11
#define		SYSDOT		12
#define		SYSRDONLY 	13
#define		SYSTIMES 	14
#define		SYSXPORT	15
#define		SYSNULL 	16
#define		SYSREAD 	17
#define		SYSTST		18

#ifndef RES	/*	exclude umask code	*/
#define		SYSUMASK 	20
#define		SYSULIMIT 	21
#endif

#define		SYSECHO		22
#define		SYSHASH		23
#define		SYSPWD		24
#define		SYSRETURN	25
#define		SYSUNS		26
#define		SYSMEM		27
#define		SYSTYPE  	28
#define		SYSGETOPT	29
#define		SYSJOBS		30
#define		SYSFGBG		31
#define		SYSKILL		32
#define		SYSSUSP		33
#define		SYSSTOP		34

#define		SYSHISTORY	35
#define		SYSALLOC	36

/* used for input and output of shell */
#define		INIO 		19

/* io nodes */
#define		USERIO		10
#define		IOUFD		15	/* mask for UNIX file descriptor number */
#define		IODOC		0x0010
#define		IOPUT		0x0020
#define		IOAPP		0x0040
#define		IOMOV		0x0080
#define		IORDW		0x0100
#define		IOSTRIP		0x0200
#define		IODOC_SUBST	0x0400
#define		INPIPE		0
#define		OTPIPE		1

/* arg list terminator */
#define		ENDARGS		0

#ifdef	SCHILY_BUILD
#include	<schily/mconfig.h>
#include	<schily/unistd.h>
#include	<schily/stdlib.h>	/* malloc()/free()... */
#include	<schily/limits.h>
#include	<schily/maxpath.h>
#include	<schily/signal.h>
#include	<schily/types.h>
#include	<schily/utypes.h>
#include	<schily/time.h>
#include	<schily/string.h>
#undef	index
#define	eaccess	__no_eaccess__
#include	<schily/libport.h>
#undef	eaccess

/* locale support */
#include	"ctype.h"
#include	<schily/ctype.h>
#include	<schily/nlsdefs.h>
#include	<schily/wchar.h>	/* includes stdio.h */
#include	<schily/wctype.h>	/* needed before we use wchar_t */
#undef	feof				/* to make mode.h compile with K&R */

#include 	"mac.h"
#include	"mode.h"
#include	"name.h"

#ifndef	HAVE_SYS_ACCT_H
#undef	ACCT
#endif

#else	/* SCHILY_BUILD */

#include	<unistd.h>
#include 	"mac.h"
#include	"mode.h"
#include	"name.h"
#include	<signal.h>
#include	<sys/types.h>
#include	<inttypes.h>
#define	UInt32_t	uint32_t
#define	UInt16_t	uint16_t
#define	UIntmax_t	uintmax_t
#define	Intmax_t	intmax_t
#define	UIntptr_t	uintptr_t
#define	Intptr_t	intptr_t
#include	<string.h>
#undef	index

/* locale support */
#include	"ctype.h"
#include	<ctype.h>
#include	<locale.h>

#include	<stdlib.h>
#include	<limits.h>

#define	PROTOTYPES
#define	__PR(a)	a
#define	EXPORT
#define	LOCAL	static

/*
 * This is a static configuration for SunOS-5.11.
 * For other platforms, use the dynamic SCHILY_BUILD environment.
 */
#define	HAVE_LIBGEN_H
#define	HAVE_GMATCH
#define	HAVE_UCONTEXT_H
#define	HAVE_ACCESS_E_OK
#define	HAVE_STRSIGNAL
#define	HAVE_STR2SIG
#define	HAVE_SIG2STR
#define	HAVE_MEMCHR
#define	HAVE_MEMSET
#define	HAVE_MEMCPY
#define	HAVE_MEMMOVE
#define	HAVE_SBRK
#define	HAVE_GETPGID
#define	HAVE_GETSID

#define	HAVE_ISASTREAM
#define	HAVE_SIGINFO_H
#define	HAVE_SIGINFO_T
#define	HAVE_SIGALTSTACK
#define	HAVE_STACK_T
#define	HAVE_STROPTS_H
#define	HAVE_STRTOLL
#define	HAVE_SYS_PROCSET_H
#define	HAVE_TCGETPGRP
#define	HAVE_TCSETPGRP

#endif	/* ! SCHILY_BUILD */

#ifndef	__NORETURN
#define	__NORETURN
#endif

#ifdef	RETSIGTYPE	/* From schily/mconfig.h */
typedef	RETSIGTYPE	(*sigtype) __PR((int));
typedef	RETSIGTYPE	sigret;

#else	/* RETSIGTYPE */
#define	VOID_SIGS
#ifdef	VOID_SIGS
typedef	void	(*sigtype) __PR((int));
typedef	void	sigret;
#else
typedef	int	(*sigtype) __PR((int));
typedef	int	sigret;
#endif	/* VOID_SIGS */
#endif	/* RETSIGTYPE */

/* id's */
extern pid_t	mypid;
extern pid_t	mypgid;
extern pid_t	mysid;

/* getopt */

extern int		optind;
extern int		opterr;
extern int 		_sp;
extern char 		*optarg;

#ifdef	STAK_DEBUG
#ifndef	DO_SYSALLOC
#define	DO_SYSALLOC
#endif
#endif	/* !STAK_DEBUG */

#ifdef	NO_INTERACTIVE
#undef	INTERACTIVE
#endif

/* Function prototypes */

/*
 * args.c
 */
extern	int		options		__PR((int argc, unsigned char **argv));
extern	void		setargs		__PR((unsigned char *argi[]));
extern	struct dolnod *	freeargs	__PR((struct dolnod *blk));
extern	void		clearup		__PR((void));
extern	struct dolnod *	savargs		__PR((int funcnt));
extern	void 		restorargs	__PR((struct dolnod *olddolh, int funcnt));
extern	struct dolnod *	useargs		__PR((void));

/*
 * blok.c
 */
extern	void	addblok		__PR((unsigned int));
#ifdef	DO_SYSALLOC
extern	void	chkmem		__PR((void));
#endif

/*
 * bltin.c
 */
extern	void	builtin		__PR((int type, int argc, unsigned char **argv, struct trenod *t));

/*
 * cmd.c
 */
extern	struct trenod *makefork	__PR((int flgs, struct trenod *i));
extern	struct trenod *cmd	__PR((int sym, int flg));

/*
 * echo.c
 */
extern	int	echo		__PR((int argc, unsigned char **argv));


/*
 * error.c
 */
extern	void	error		__PR((const char *s)) __NORETURN;
extern	void	failed_real	__PR((unsigned char *s1, const char *s2, unsigned char *s3)) __NORETURN;
extern	void	failure_real	__PR((unsigned char *s1, const char *s2, int gflag));
extern	void	exitsh		__PR((int xno)) __NORETURN;
extern	void	rmtemp		__PR((struct ionod *base));
extern	void	rmfunctmp	__PR((void));


/*
 * expand.c
 */
extern	int	expand		__PR((unsigned char *as, int rcnt));
extern	void	makearg		__PR((struct argnod *));

/*
 * fault.c
 */
extern	void	done		__PR((int sig)) __NORETURN;
extern	int	handle		__PR((int sig, sigtype func));
extern	void	stdsigs		__PR((void));
extern	void	oldsigs		__PR((void));
extern	void	chktrap		__PR((void));
extern	void	systrap		__PR((int argc, char **argv));
extern	void	sh_sleep	__PR((unsigned int ticks));
extern	void 	init_sigval	 __PR((void));


/*
 * func.c
 */
extern	void	freefunc	__PR((struct namnod  *n));
extern	void	freetree	__PR((struct trenod  *n));
extern	void	prcmd		__PR((struct trenod *t));
extern	void	prf		__PR((struct trenod *t));

#ifndef	HAVE_GMATCH
extern	int	gmatch		__PR((const char *, const char *));
#else
#ifdef	HAVE_LIBGEN_H
#include <libgen.h>
#endif
#endif

/*
 * hashserv.c
 */
extern	short	pathlook	__PR((unsigned char *com, int flg, struct argnod *arg));
extern	void	zaphash		__PR((void));
extern	void	zapcd		__PR((void));
extern	void	hashpr		__PR((void));
extern	void	set_dotpath	__PR((void));
extern	void	hash_func	__PR((unsigned char *name));
extern	void	func_unhash	__PR((unsigned char *name));
extern	short	hash_cmd	__PR((unsigned char *name));
extern	int	what_is_path	__PR((unsigned char *name));
extern	int	chk_access	__PR((unsigned char *name, mode_t mode, int regflag));

/*
 * io.c
 */
extern	void	initf		__PR((int fd));
extern	int	estabf		__PR((unsigned char *s));
extern	void	push		__PR((struct fileblk *af));
extern	int	pop		__PR((void));
extern	int	poptemp		__PR((void));
extern	void	chkpipe		__PR((int *pv));
extern	int	chkopen		__PR((unsigned char *idf, int mode));
extern	void	renamef		__PR((int f1, int f2));
extern	int	create		__PR((unsigned char *s));
extern	int	tmpfil		__PR((struct tempblk *tb));
extern	void	copy		__PR((struct ionod *ioparg));
extern	void	link_iodocs	__PR((struct ionod *i));
extern	void	swap_iodoc_nm	__PR((struct ionod *i));
extern	int	savefd		__PR((int fd));
extern	void	restore		__PR((int last));

/*
 * jobs.c
 */
extern	void	collect_fg_job	__PR((void));
extern	void	freejobs	__PR((void));
extern	void	startjobs	__PR((void));
extern	int	endjobs		__PR((int check_if));
extern	void	deallocjob	__PR((void));
extern	void	allocjob	__PR((char *_cmd, unsigned char *cwd, int monitor));
extern	void	clearjobs	__PR((void));
extern	void	makejob		__PR((int monitor, int fg));
extern	void	postjob		__PR((pid_t pid, int fg));
extern	void	sysjobs		__PR((int argc, char *argv[]));
extern	void	sysfgbg		__PR((int argc, char *argv[]));
extern	void	syswait		__PR((int argc, char *argv[]));
extern	void	sysstop		__PR((int argc, char *argv[]));
extern	void	syskill		__PR((int argc, char *argv[]));
extern	void	syssusp		__PR((int argc, char *argv[]));
extern	void	hupforegnd	__PR((void));

/*
 * macro.c
 */
extern	unsigned char *macro	__PR((unsigned char *as));
extern	void	subst		__PR((int in, int ot));

/*
 * main.c
 */
extern	void	chkpr		__PR((void));
extern	void	settmp		__PR((void));
extern	void	chkmail		__PR((void));
extern	void	setmail		__PR((unsigned char *));
#define	setmode		set_imode	/* Conflicts with FreeBSD libc function */
extern	void	setmode		__PR((int prof));
extern	void	secpolicy_print __PR((int level, const char *msg));



/*
 * name.c
 */
extern	int	syslook		__PR((unsigned char *w, const struct sysnod syswds[], int n));
extern	void	setlist		__PR((struct argnod *arg, int xp));
extern	void	replace		__PR((unsigned char **a, unsigned char *v));
extern	void	dfault		__PR((struct namnod *n, unsigned char *v));
extern	void	assign		__PR((struct namnod *n, unsigned char *v));
extern	int	readvar		__PR((unsigned char **names));
extern	void	assnum		__PR((unsigned char **p, long i));
extern	unsigned char *	make	__PR((unsigned char *v));
extern	struct namnod *	lookup	__PR((unsigned char *nam));
extern	void	namscan		__PR((void (*fn)(struct namnod *n)));
extern	void	printnam	__PR((struct namnod *n));
extern	void	printro		__PR((struct namnod *n));
extern	void	printexp	__PR((struct namnod *n));
extern	void	setup_env	__PR((void));
extern	unsigned char **local_setenv __PR((void));
extern	struct namnod *findnam	__PR((unsigned char *nam));
extern	void	unset_name	__PR((unsigned char *name));

/*
 * print.c
 */
extern	void	prp		__PR((void));
extern	void	prs		__PR((unsigned char *as));
extern	void	prc		__PR((unsigned char c));
extern	void	prwc		__PR((wchar_t c));
extern	void	prt		__PR((long t));
extern	void	prn		__PR((int n));
extern	void	itos		__PR((int n));
extern	int	stoi		__PR((unsigned char *icp));
extern	int	ltos		__PR((long n));

extern	void	flushb		__PR((void));
extern	void	prc_buff	__PR((unsigned char c));
extern	void	prs_buff	__PR((unsigned char *s));
extern	void	prs_cntl	__PR((unsigned char *s));
extern	void	prull_buff	__PR((UIntmax_t lc));
extern	void	prn_buff	__PR((int n));
extern	int	setb		__PR((int fd));

extern	void	prc_buff	__PR((unsigned char c));
extern	void	prs_buff	__PR((unsigned char *s));
extern	void	prn_buff	__PR((int n));
extern	void	prs_cntl	__PR((unsigned char *s));

/*
 * pwd.c
 */
extern	void	cwd		__PR((unsigned char *dir));
extern	unsigned char *cwdget	__PR((void));
extern	void	cwdprint	__PR((void));


/*
 * service.c
 */
extern	short		initio	__PR((struct ionod *iop, int save));
extern	unsigned char *simple	__PR((unsigned char *s));
extern	unsigned char *getpath	__PR((unsigned char *s));
extern	int		pathopen __PR((unsigned char *path, unsigned char *name));
extern	unsigned char *catpath	__PR((unsigned char *path, unsigned char *name));
extern	unsigned char *nextpath	__PR((unsigned char *path));
extern	void		execa	__PR((unsigned char *at[], short pos));
extern	void		trim	__PR((unsigned char *at));
extern	void		trims	__PR((unsigned char *at));
extern	unsigned char *mactrim	__PR((unsigned char *at));
extern	unsigned char **scan	__PR((int argn));
extern	int 		getarg	__PR((struct comnod *ac));

extern	void	suspacct	__PR((void));
extern	void	preacct		__PR((unsigned char *cmdadr));
extern	void	doacct		__PR((void));


/*
 * setbrk.c
 */
extern	unsigned char *setbrk	__PR((int));

/*
 * stak.c
 */
#define	free	shfree

extern	void		*alloc		__PR((size_t));
extern	void		free		__PR((void *ap));
extern	unsigned char *getstak		__PR((Intptr_t asize));
extern	unsigned char *locstak		__PR((void));
extern	unsigned char *growstak		__PR((unsigned char *newtop));
extern	unsigned char *savstak		__PR((void));
extern	unsigned char *endstak		__PR((unsigned char *argp));
extern	void		tdystak		__PR((unsigned char *x, struct ionod *iosav));
extern	void		stakchk		__PR((void));
extern	unsigned char *cpystak		__PR((unsigned char *));
extern	unsigned char *movstrstak	__PR((unsigned char *a, unsigned char *b));
extern	unsigned char *memcpystak	__PR((unsigned char *s1, unsigned char *s2, int n));


/*
 * string.c
 */
extern	unsigned char *movstr	__PR((unsigned char *a, unsigned char *b));
extern	int		any	__PR((wchar_t c, unsigned char *s));
extern	int		anys	__PR((unsigned char *c, unsigned char *s));
extern	int		cf	__PR((unsigned char *s1, unsigned char *s2));
extern	int		length	__PR((unsigned char *as));
extern	unsigned char *movstrn	__PR((unsigned char *a, unsigned char *b, int n));

/*
 * signames.c
 */
#ifndef	HAVE_STRSIGNAL
extern	char	*strsignal	__PR((int sig));
#endif
#ifndef	HAVE_STR2SIG
extern	int	str2sig		__PR((const char *s, int *sigp));
#endif
#ifndef	HAVE_SIG2STR
extern	int	sig2str		__PR((int sig, char *s));
#endif

/*
 * test.c
 */
extern	int	test		__PR((int argn, unsigned char *com[]));

/*
 * ulimit.c
 */
extern	void	sysulimit	__PR((int argc, char **argv));

/*
 * word.c
 */
extern	int		word	__PR((void));
extern	unsigned int	skipwc	__PR((void));
extern	unsigned int	nextwc	__PR((void));
extern	unsigned char *	readw	__PR((wchar_t d));
extern	unsigned int	readwc	__PR((void));

/*
 * xec.c
 */
extern	int	execute		__PR((struct trenod *argt, int xflags, int errorflg, int *pf1, int *pf2));
extern	void	execexp		__PR((unsigned char *s, Intptr_t f));

/*
 * libshedit
 */
extern	int	egetc		__PR((void));
extern	void	bsh_treset	__PR((void));
extern	void	bhist		__PR((void));

#define		attrib(n, f)		(n->namflg |= f)
#define		round(a, b)		(((Intptr_t)(((char *)(a)+b)-1))&~((b)-1))
#define		closepipe(x)		(close(x[INPIPE]), close(x[OTPIPE]))
#define		eq(a, b)		(cf((unsigned char *)(a), (unsigned char *)(b)) == 0)
#undef		max
#define		max(a, b)		((a) > (b)?(a):(b))
#define		assert(x)
#define		_gettext(s)		(unsigned char *)gettext(s)

/*
 * macros using failed_real(). Only s2 is gettext'd with both functions.
 */
#define		failed(s1, s2)		failed_real(s1, s2, NULL)
#define		bfailed(s1, s2, s3)	failed_real(s1, s2, s3)

/*
 * macros using failure_real(). s1 and s2 is gettext'd with gfailure(), but
 * only s2 is gettext'd with failure().
 */
#define		failure(s1, s2)		failure_real(s1, s2, 0)
#define		gfailure(s1, s2)	failure_real(s1, s2, 1)

/* temp files and io */
extern int				output;
extern int				ioset;
extern struct ionod	*iotemp; /* files to be deleted sometime */
extern struct ionod	*fiotemp; /* function files to be deleted sometime */
extern struct ionod	*iopend; /* documents waiting to be read at NL */
extern struct fdsave	fdmap[];
extern int savpipe;

/* substitution */
extern int				dolc;
extern unsigned char				**dolv;
extern struct dolnod	*argfor;
extern struct argnod	*gchain;

/* stak stuff */
#include		"stak.h"

/*
 * If non-ANSI C, make const go away.  We bring it back
 * at the end of the file to avoid side-effects.
 */
#ifndef __STDC__
#define	const
#endif

/* string constants */
extern const char				atline[];
extern const char				readmsg[];
extern const char				colon[];
extern const char				minus[];
extern const char				nullstr[];
extern const char				sptbnl[];
extern const char				unexpected[];
extern const char				endoffile[];
extern const char				synmsg[];

/* name tree and words */
extern const struct sysnod	reserved[];
extern const int				no_reserved;
extern const struct sysnod	commands[];
extern const int				no_commands;

extern int				wdval;
extern int				wdnum;
extern int				fndef;
extern int				nohash;
extern struct argnod	*wdarg;
extern int				wdset;
extern BOOL				reserv;

/* prompting */
extern const char				stdprompt[];
extern const char				supprompt[];
extern const char				profile[];
extern const char				sysprofile[];

/* locale testing */
extern const char			localedir[];
extern int				localedir_exists;

/* built in names */
extern struct namnod	fngnod;
extern struct namnod	cdpnod;
extern struct namnod	ifsnod;
extern struct namnod	homenod;
extern struct namnod	mailnod;
extern struct namnod	pathnod;
extern struct namnod	ps1nod;
extern struct namnod	ps2nod;
extern struct namnod	mchknod;
extern struct namnod	acctnod;
extern struct namnod	mailpnod;

/* special names */
extern unsigned char				flagadr[];
extern unsigned char				*pcsadr;
extern unsigned char				*pidadr;
extern unsigned char				*cmdadr;

/* names always present */
extern const char				defpath[];
extern const char				mailname[];
extern const char				homename[];
extern const char				pathname[];
extern const char				cdpname[];
extern const char				ifsname[];
extern const char				ps1name[];
extern const char				ps2name[];
extern const char				mchkname[];
extern const char				acctname[];
extern const char				mailpname[];

/* transput */
extern unsigned char				tmpout[];
extern int 					tmpout_offset;
extern unsigned int				serial;

/*
 * allow plenty of room for size for temp file name:
 * "/tmp/sh"(7) + <pid> (<=6) + <unsigned int #> (<=10) + \0 (1)
 */
#define			TMPOUTSZ 		32

extern struct fileblk	*standin;

#define		input		(standin->fdes)
#define		eof			(standin->feof)

#define	peekc	peekc_			/* AIX has a hidden peekc() in libc */
extern int				peekc;
extern int				peekn;
extern unsigned char			*comdiv;
extern const char			devnull[];

/* flags */
#define			noexec		01
#define			sysflg		01
#define			intflg		02
#define			prompt		04
#define			setflg		010
#define			errflg		020
#define			ttyflg		040
#define			forked		0100
#define			oneflg		0200
#define			rshflg		0400
#define			subsh		01000
#define			stdflg		02000
#define			STDFLG		's'
#define			execpr		04000
#define			readpr		010000
#define			keyflg		020000
#define			hashflg		040000
#define			nofngflg	0200000
#define			exportflg	0400000
#define			monitorflg	01000000
#define			jcflg		02000000
#define			privflg		04000000
#define			forcexit	010000000
#define			jcoff		020000000
#define			pfshflg		040000000
#define			versflg		0100000000

extern long				flags;
extern int				rwait;	/* flags read waiting */

/* error exits from various parts of shell */
#include	<setjmp.h>
extern jmp_buf			subshell;
extern jmp_buf			errshell;

/* fault handling */
#include	"brkincr.h"

extern unsigned			brkincr;
#define		MINTRAP		0
#define		MAXTRAP		NSIG

#define		TRAPSET		2
#define		SIGSET		4
#define			SIGMOD		8
#define			SIGIGN		16

extern BOOL				trapnote;

/* name tree and words */
#ifdef	HAVE_ENVIRON_DEF
/*
 * Warning: HP-UX and Linux have "extern char **environ;" in unistd.h
 * We should not use our own definitions here.
 */
#else
extern char					**environ;
#endif
extern unsigned char				numbuf[];
extern const char				export[];
extern const char				duperr[];
extern const char				readonly[];

/* execflgs */
extern int				exitval;
extern int				retval;
extern BOOL				execbrk;
extern int				loopcnt;
extern int				breakcnt;
extern int				funcnt;
extern int				tried_to_exit;

/* messages */
extern const char				mailmsg[];
extern const char				coredump[];
extern const char				badopt[];
extern const char				badparam[];
extern const char				unset[];
extern const char				badsub[];
extern const char				nospace[];
extern const char				nostack[];
extern const char				notfound[];
extern const char				badtrap[];
extern const char				baddir[];
extern const char				badshift[];
extern const char				restricted[];
extern const char				execpmsg[];
extern const char				notid[];
extern const char 				badulimit[];
extern const char 				ulimit[];
extern const char				wtfailed[];
extern const char				badcreate[];
extern const char				nofork[];
extern const char				noswap[];
extern const char				piperr[];
extern const char				badopen[];
extern const char				badnum[];
extern const char				badsig[];
extern const char				badid[];
extern const char				arglist[];
extern const char				txtbsy[];
extern const char				toobig[];
extern const char				badexec[];
extern const char				badfile[];
extern const char				badreturn[];
extern const char				badexport[];
extern const char				badunset[];
extern const char				nohome[];
extern const char				badperm[];
extern const char				mssgargn[];
extern const char				libacc[];
extern const char				libbad[];
extern const char				libscn[];
extern const char				libmax[];
extern const char				emultihop[];
extern const char				nulldir[];
extern const char				enotdir[];
extern const char				enoent[];
extern const char				eacces[];
extern const char				enolink[];
extern const char				exited[];
extern const char				running[];
extern const char				ambiguous[];
extern const char				nosuchjob[];
extern const char				nosuchpid[];
extern const char				nosuchpgid[];
extern const char				usage[];
extern const char				nojc[];
extern const char				killuse[];
extern const char				jobsuse[];
extern const char				stopuse[];
extern const char				ulimuse[];
extern const char				nocurjob[];
extern const char				loginsh[];
extern const char				jobsstopped[];
extern const char				jobsrunning[];
extern const char				nlorsemi[];
extern const char				signalnum[];
extern const char				badpwd[];
extern const char				badlocale[];
extern const char				nobracket[];
extern const char				noparen[];
extern const char				noarg[];

/*	'builtin' error messages	*/

extern const char				btest[];
extern const char				badop[];

/*	fork constant	*/

#define		FORKLIM 	32

extern address			end[];

extern int				eflag;
extern int				ucb_builtins;

/*
 * Find out if it is time to go away.
 * `trapnote' is set to SIGSET when fault is seen and
 * no trap has been set.
 */

#define		sigchk()	if (trapnote & SIGSET)	\
					exitsh(exitval ? exitval : SIGFAIL)

#define		exitset()	retval = exitval

/* Multibyte characters */
unsigned char *readw	__PR((wchar_t));
#define	MULTI_BYTE_MAX MB_LEN_MAX

/*
 * Make sure we have a definition for this.  If not, take a very conservative
 * guess.
 * POSIX requires the max pathname component lenght to be defined in limits.h
 * If variable, it may be undefined. If undefined, there should be
 * a definition for _POSIX_NAME_MAX in limits.h or in unistd.h
 * As _POSIX_NAME_MAX is defined to 14, we cannot use it.
 */
#ifndef NAME_MAX
#ifdef FILENAME_MAX
#define	NAME_MAX	FILENAME_MAX
#else
#define	NAME_MAX	256
#endif
#endif

#ifndef PATH_MAX
#ifdef FILENAME_MAX
#define	PATH_MAX	FILENAME_MAX
#else
#define	PATH_MAX	1024
#endif
#endif

#if	defined(HAVE_GETPGID) || defined(HAVE_SETPGID)
#	define	POSIXJOBS
#endif

#if	!defined(HAVE_GETPGID) && defined(HAVE_BSD_GETPGRP)
#	define	getpgid	getpgrp
#	define	getsid	getpgrp
#endif
#if	!defined(HAVE_GETSID) && defined(HAVE_BSD_GETPGRP)
#	define	getsid	getpgrp
#endif
#if	!defined(HAVE_SETPGID) && defined(HAVE_BSD_SETPGRP)
#	define	setpgid	setpgrp
#endif
#if	!defined(HAVE_SETSID) && defined(HAVE_BSD_GETPGRP)
#	define	setsid	setpgrp(getpid())
#endif

#if	!defined(HAVE_GETPGID) && !defined(HAVE_BSD_GETPGRP)
/*
 * FreeBSD (anything that is BSD-4.4 derived)
 * does not conform to POSIX and is not old BSD conforming either.
 * Note that this seems to have been changed between 1995 and 2000
 *
 * getpgrp()/setpgrp() are not POSIX on BSD-4.4
 *
 * setpgrp() is old BSD compliant,
 * getpgrp() is not BSD compliant but it is POSIX compliant
 *
 * setpgid() is OK (POSIX compliant)
 * getpgid() is missing!
 *
 * The builtin command 'pgrp' will not work correctly for this reason.
 *
 *	BSD:
 *
 *	getpgrp(pid)		-> pgid for pid
 *	setpgrp(pid, pgid)	-> set pgid of pid
 *
 *	POSIX:
 *
 *	getpgid(pid)		-> pgid for pid
 *	setpgid(pid, pgid)	-> set pgid of pid
 *	getpgrp(void)		-> pgid for $$
 *	setpgrp(void)		-> setpgid(0,0)
 *
 *	4.4-BSD:
 *
 *	getpgid(pid)		-> is missing!
 *	setpgid(pid, pgid)	-> set pgid of pid
 *	getpgrp(void)		-> ????
 *	setpgrp(pid, pgid)	-> set pgid of pid
 */
#	define	getpgid(a)	getpgrp()
#endif

#if	!defined(HAVE_GETSID) && !defined(HAVE_BSD_GETPGRP)
#	define	getsid	getpgid
#endif


#if !defined(HAVE_MEMSET) && !defined(memset)
#define	memset(s, c, n)		fillbytes(s, n, c)
#endif
#if !defined(HAVE_MEMCHR) && !defined(memchr)
#define	memchr(s, c, n)		findbytes(s, n, c)
#endif
#if !defined(HAVE_MEMCPY) && !defined(memcpy)
#define	memcpy(s1, s2, n)	movebytes(s2, s1, c)
#endif
#if !defined(HAVE_MEMMOVE) && !defined(memmove)
#define	memmove(s1, s2, n)	movebytes(s2, s1, c)
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _DEFS_H */
