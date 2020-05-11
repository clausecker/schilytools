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

#ifndef	_DEFS_H
#define	_DEFS_H

#if defined(sun)
#pragma ident	"@(#)defs.h	1.28	08/01/29 SMI"
#endif

/*
 * Copyright 2008-2020 J. Schilling
 *
 * @(#)defs.h	1.209 20/04/21 2008-2020 J. Schilling
 */

/*
 * Some compilers may not support enough command line arguments.
 * This include file permits to put the configuration inti conf.h
 * instead of having it in the Makefile.
 */
#ifdef	DO_CONF_H
#include	"conf.h"
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 *	UNIX shell
 */

/* execute flags */
#define		XEC_EXECED	01	/* Forked cmd with recursive execute */
#define		XEC_LINKED	02	/* Forked lhs of "|" or "&"	    */
#define		XEC_NOSTOP	04	/* Do no jobcontrol for this cmd    */
#define		XEC_NOBLTIN	010	/* Do not execute functions / bltins */
#define		XEC_STDINSAV	020	/* STDIN_FILENO was moved away	    */
#define		XEC_ALLOCJOB	040	/* A job slot was already allocated */
#define		XEC_HASARGV	0100	/* t->comarg holds expanded argv[]  */
#define		XEC_INFUNC	01000	/* We are inside a function call    */

/* endjobs flags */
#define		JOB_STOPPED	01
#define		JOB_RUNNING	02

/* Environ flags */
#define		ENV_NOFREE	01

/* error exits from various parts of shell */
#define		ERROR		1	/* Standard shell error/exit code */
#define		SYNBAD		2	/* Shell error/exit for bad syntax */
#ifdef	DO_POSIX_TEST
#define		ETEST		2	/* POSIX test(1) error exit code  */
#else
#define		ETEST		ERROR	/* historical test(1) error exit code */
#endif
#define		SIGFAIL 	2000	/* Default shell exit code on signal */
#define		SIGFLG		0200	/* $? == SIGFLG + signo */
#define		C_NOEXEC	126	/* Shell error/exit for exec error */
#define		C_NOTFOUND	127	/* Shell error/exit for exec notfound */
#ifdef	DO_POSIX_EXIT
#define		ERR_NOEXEC	C_NOEXEC
#define		ERR_NOTFOUND	C_NOTFOUND
#else
#define		ERR_NOEXEC	ERROR
#define		ERR_NOTFOUND	ERROR
#endif

/* command tree */
#define		FPIN		0x0100	/* PIPE from stdin		*/
#define		FPOU		0x0200	/* PIPE to stdout		*/
#define		FAMP		0x0400	/* Forked because of "cmd &"	*/
#define		COMMSK		0x10F0	/* Node type mask, see below	*/
#define		CNTMSK		0x000F	/* Count mask - no longer used	*/
#define		IOFMSK		0x000F	/* I/O fd# mask for pipes	*/

#define		TCOM		0x0000	/* some kind of command node 	*/
#define		TPAR		0x0010	/* "()" parentized cmd node	*/
#define		TFIL		0x0020	/* PIPE "|" filter node		*/
#define		TLST		0x0030	/* ";" separated command list	*/
#define		TIF		0x0040	/* if ... then ... node		*/
#define		TWH		0x0050	/* "while" loop node		*/
#define		TUN		0x0060	/* "until" loop node		*/
#define		TSW		0x0070	/* "case command node		*/
#define		TAND		0x0080	/* "&&" command node		*/
#define		TORF		0x0090	/* "||" command node		*/
#define		TFORK		0x00A0	/* node running forked cmd	*/
#define		TNOFORK		0x10A0	/* node running avoid fork cmd	*/
#define		TFOR		0x00B0	/* for ... do .. done node	*/
#define		TSELECT		0x10B0	/* select ... do .. done node	*/
#define		TFND		0x00C0	/* function definition node	*/
#define		TTIME		0x00D0	/* "time" node			*/
#define		TNOT		0x10D0	/* "!" node			*/
#define		TSETIO		0x00E0	/* node to redirect io		*/

/*
 * execute table
 *
 * Numbers for builtin commands
 */
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
#define		SYSSHFT		7
#define		SYSWAIT		8
#define		SYSCONT		9
#define		SYSBREAK	10
#define		SYSEVAL		11
#define		SYSDOT		12
#define		SYSRDONLY	13
#define		SYSTIMES	14
#define		SYSXPORT	15
#define		SYSNULL		16
#define		SYSREAD		17
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
#define		SYSSAVEHIST	36
#define		SYSMAP		37
#define		SYSREPEAT	38

#define		SYSDIRS		39
#define		SYSPOPD		40
#define		SYSPUSHD	41

#define		SYSDOSH		42
#define		SYSALIAS	43
#define		SYSUNALIAS	44

#define		SYSTRUE		45
#define		SYSFALSE	46

#define		SYSALLOC	47

#define		SYSBUILTIN	48
#define		SYSFIND		49
#define		SYSEXPR		50
#define		SYSSYNC		51
#define		SYSPGRP		52
#define		SYSERRSTR	53
#define		SYSPRINTF	54
#define		SYSCOMMAND	55
#define		SYSLOCAL	56
#define		SYSFC		57

#define		SYSLOADABLE	255	/* For all dynamically loaded cmds   */
#define		SYSMAX		255	/* Must fit in low 8 ENTRY.data bits */

/*
 * Sysnode flags for builtin commands:
 */
#define		BLT_SPC		1	/* A special builtin */
#define		BLT_INT		2	/* A shell intrinsic */
#define		BLT_DEL		8	/* Builtin was deleted */

/*
 * Flags for cd/pwd reladed builtin commands:
 */
#define		CHDIR_L		1	/* Use logical mode (symlinks kept)   */
#define		CHDIR_P		2	/* Use physcal mode (expand symlinks) */

/*
 * Binary operators in "test"
 */
#define	TEST_SNEQ	1		/* !=  string not equal */
#define	TEST_AND	2		/* -a  and */
#define	TEST_EF		3		/* -ef file exist + equal */
#define	TEST_NT		4		/* -nt file newer than */
#define	TEST_OR		5		/* -o  or */
#define	TEST_OT		6		/* -ot file older than */
#define	TEST_SEQ	7		/* =   string equal */

#define	TEST_EQ		10		/* -eq integer equal		== */
#define	TEST_GE		11		/* -ge integer greater or equal >= */
#define	TEST_GT		12		/* -gt integer greater		>  */
#define	TEST_LE		13		/* -le integer less or equal	<= */
#define	TEST_LT		14		/* -lt integer less than	<  */
#define	TEST_NE		15		/* -ne integer not equal	!= */

/* used for input and output of shell */
#define		INIO 		19

/* io nodes */
#define		USERIO		10	/* User definable fds in range 0..9 */
#define		IOUFD		15	/* mask for UNIX file descriptor #  */
#define		IODOC		0x0010	/* << here document		    */
#define		IOPUT		0x0020	/* >  redirection		    */
#define		IOAPP		0x0040	/* >> redirection		    */
#define		IOMOV		0x0080	/* <& or >& redirection		    */
#define		IORDW		0x0100	/* <> redirection open with O_RDWR  */
#define		IOSTRIP		0x0200	/* <<-word: strip leading tabs	    */
#define		IODOC_SUBST	0x0400	/* <<\word: no substitution	    */
#define		IOCLOB		0x1000	/* >| clobber files		    */
#define		IOBARRIER	0x2000	/* tmpfile link/unlink barrier	    */

#define		INPIPE		0	/* Input fd index in pipe fd array  */
#define		OTPIPE		1	/* Output fd index in pipe fd array */

/* arg list terminator */
#define		ENDARGS		0

#ifdef	SCHILY_INCLUDES
#include	<schily/mconfig.h>
#include	<schily/unistd.h>
#include	<schily/getopt.h>
#include	<schily/stdlib.h>	/* malloc()/free()... */
#include	<schily/limits.h>
#include	<schily/maxpath.h>
#include	<schily/signal.h>
#include	<schily/types.h>
#include	<schily/utypes.h>
#include	<schily/time.h>
#include	<schily/string.h>
#undef	index

/* locale support */
#include	"ctype.h"
#include	<schily/ctype.h>
#include	<schily/nlsdefs.h>
#include	<schily/wchar.h>	/* includes stdio.h */
#include	<schily/wctype.h>	/* needed before we use wchar_t */
#undef	feof				/* to make mode.h compile with K&R */

#include	<schily/setjmp.h>
#include	<schily/jmpdefs.h>

#ifdef DO_SYSBUILTIN
/*
 * Note that if we do not #define DO_SYSBUILTIN, we will never have
 * a HAVE_LOADABLE_LIBS #define
 */
#include	<schily/dlfcn.h>	/* to #define HAVE_LOADABLE_LIBS */
#endif

#include 	"mac.h"
#include	"mode.h"
#include	"name.h"
#include	"bosh.h"

#ifndef	HAVE_SYS_ACCT_H
#undef	ACCT
#endif

#else	/* SCHILY_INCLUDES */

#include	<unistd.h>
#include 	"mac.h"
#include	"mode.h"
#include	"name.h"
#include	"bosh.h"
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

#include	<setjmp.h>

#define	PROTOTYPES
#define	__PR(a)	a
#define	EXPORT
#define	LOCAL	static

/*
 * This is a static configuration for SunOS-5.11.
 * For other platforms, use the dynamic SCHILY_INCLUDES environment.
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

#endif	/* ! SCHILY_INCLUDES */

#define	CH	(char)
#define	UCH	(unsigned char)
#define	C	(char *)
#define	UC	(unsigned char *)
#define	CP	(char **)
#define	UCP	(unsigned char **)
#define	CPP	(char ***)
#define	UCPP	(unsigned char ***)

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

/*
 * Global data structure
 */
extern bosh_t	bosh;

/* id's */
extern pid_t	mypid;
extern pid_t	mypgid;
extern pid_t	mysid;

/* getopt */
#define	_sp		opt_sp	/* Use variable name from new getopt() */
#ifndef	__CYGWIN__
/*
 *	Cygwin uses a nonstandard:
 *
 *		__declspec(dllexport)
 *	or
 *		__declspec(dllimport)
 *
 *	that is in conflict with our standard definition.
 */
extern int		optind;
extern int		opterr;
extern int		optopt;
extern char 		*optarg;
#endif

#ifdef	STAK_DEBUG
#ifndef	DO_SYSALLOC
#define	DO_SYSALLOC
#endif
#endif	/* !STAK_DEBUG */

#ifdef	NO_INTERACTIVE
#undef	INTERACTIVE
#undef	DO_SYSFC
#endif

#ifdef	NO_SYSATEXPR
#undef	DO_SYSATEXPR
#endif

#ifdef	NO_SYSFIND
#undef	DO_SYSFIND
#endif

#ifdef	NO_SYSPRINTF
#undef	DO_SYSPRINTF
#endif

#ifdef	NO_VFORK
#undef	HAVE_VFORK
#endif

#ifdef	NO_PIPE_PARENT
#undef	DO_PIPE_PARENT
#endif

/* Function prototypes */

/*
 * args.c
 */
extern	void		prversion	__PR((void));
extern	int		options		__PR((int argc, unsigned char **argv));
extern	void		setopts		__PR((void));
extern	void		setargs		__PR((unsigned char *argi[]));
extern	struct dolnod	*freeargs	__PR((struct dolnod *blk));
extern	void		clearup		__PR((void));
extern	struct dolnod	*savargs	__PR((int funcnt));
extern	void 		restorargs	__PR((struct dolnod *olddolh,
							int funcnt));
extern	struct dolnod	*useargs	__PR((void));
extern	int		optval		__PR((unsigned char *flagc));
extern	unsigned char	*lookopt	__PR((unsigned char *name));

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
extern	void	builtin		__PR((int type, int argc, unsigned char **argv,
						struct trenod *t, int xflags));

/*
 * cmd.c
 */
extern	struct trenod *makefork	__PR((int flgs, struct trenod *i));
extern	struct trenod *cmd	__PR((int sym, int flg));

/*
 * echo.c
 */
extern	int	echo		__PR((int argc, unsigned char **argv));
extern unsigned char *escape_char __PR((unsigned char *cp, unsigned char *res,
					int echomode));


/*
 * error.c
 *
 * error() and failed_real() used to have the __NORETURN tag, but since we
 * support the "command" builtin, we need to be able to prevent the exit
 * on error (or longjmp to prompt) behavior via (flags & noexit) != 0.
 */
extern	void	error		__PR((const char *s));
extern	void	failed_real	__PR((int err, unsigned char *s1,
						const char *s2,
						unsigned char *s3));
extern	void	failure_real	__PR((int err, unsigned char *s1,
						const char *s2,
						unsigned char *s3,
						int gflag));

extern	void	exvalsh		__PR((int xno));
extern	void	exitsh		__PR((int xno)) __NORETURN;
#ifdef	DO_DOT_SH_PARAMS
extern	void	exval_clear	__PR((void));
extern	void	exval_sig	__PR((void));
extern	void	exval_set	__PR((int xno));
#else
#define	exval_clear()
#define	exval_sig()
#define	exval_set(a)
#endif
extern	void	rmtemp		__PR((struct ionod *base));
extern	void	rmfunctmp	__PR((struct ionod *base));


/*
 * expand.c
 */
extern	int	expand		__PR((unsigned char *as, int rcnt));
extern	void	makearg		__PR((struct argnod *));

/*
 * fault.c
 */
extern	void	done		__PR((int sig)) __NORETURN;
extern	void	fault		__PR((int sig));
extern	int	handle		__PR((int sig, sigtype func));
extern	void	stdsigs		__PR((void));
extern	void	oldsigs		__PR((int dofree));
#ifdef	HAVE_VFORK
extern	void	restoresigs	__PR((void));
#endif
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
extern	void	prtree		__PR((struct trenod *t, char *label));
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
extern	short	pathlook	__PR((unsigned char *com, int flg,
						struct argnod *arg));
extern	void	zaphash		__PR((void));
extern	void	zapcd		__PR((void));
extern	void	hashpr		__PR((void));
extern	void	set_dotpath	__PR((void));
extern	void	hash_func	__PR((unsigned char *name));
extern	void	func_unhash	__PR((unsigned char *name));
extern	short	hash_cmd	__PR((unsigned char *name));
extern	int	what_is_path	__PR((unsigned char *name, int verbose));
extern	int	chk_access	__PR((unsigned char *name,
						mode_t mode, int regflag));

/*
 * hashcmd.c
 */
extern	void	hashcmd		__PR((void));

/*
 * io.c
 */
extern	void	initf		__PR((int fd));
extern	int	estabf		__PR((unsigned char *s));
extern	void	push		__PR((struct fileblk *af));
extern	int	pop		__PR((void));
extern	int	poptemp		__PR((void));
extern	int	gpoptemp	__PR((void));
extern	void	chkpipe		__PR((int *pv));
extern	int	chkopen		__PR((unsigned char *idf, int mode));
extern	void	renamef		__PR((int f1, int f2));
extern	int	create		__PR((unsigned char *s, int iof));
extern	int	tmpfil		__PR((struct tempblk *tb));
extern	void	copy		__PR((struct ionod *ioparg));
extern	int	link_iodocs	__PR((struct ionod *i));
extern	void	swap_iodoc_nm	__PR((struct ionod *i));
extern	int	savefd		__PR((int fd));
extern	void	restore		__PR((int last));

/*
 * jobs.c
 */
extern	char	*code2str	__PR((int code));
extern	void	collect_fg_job	__PR((void));
extern	void	freejobs	__PR((void));
extern	int	settgid		__PR((pid_t new, pid_t expexted));
extern	void	startjobs	__PR((void));
extern	int	endjobs		__PR((int check_if));
extern	void	allocjob	__PR((char *_cmd, unsigned char *cwd,
							int monitor));
extern	void	clearjobs	__PR((void));
extern	void	makejob		__PR((int monitor, int fg));
/*
 * This is a partial type declaration for struct job. After this line,
 * we may use struct job * in a parameter list.
 */
extern	struct job *
		postjob		__PR((pid_t pid, int fg, int blt));
extern	void	deallocjob	__PR((struct job *jp));
extern	void	*curjob		__PR((void));
extern	pid_t	curpgid		__PR((void));
extern	void	setjobpgid	__PR((pid_t pgid));
extern	void	setjobfd	__PR((int fd, int sfd));
extern	void	resetjobfd	__PR((void));
extern	void	sysjobs		__PR((int argc, unsigned char *argv[]));
extern	void	sysfgbg		__PR((int argc, char *argv[]));
extern	void	syswait		__PR((int argc, char *argv[]));
extern	void	sysstop		__PR((int argc, char *argv[]));
extern	void	syskill		__PR((int argc, char *argv[]));
extern	void	syssusp		__PR((int argc, char *argv[]));
extern	void	syspgrp		__PR((int argc, char *argv[]));
extern	int	sysprintf	__PR((int argc, unsigned char *argv[]));
extern	void	hupforegnd	__PR((void));
extern	pid_t	wait_status	__PR((pid_t id,
					int *codep, int *statusp, int opts));
extern	void	prtime		__PR((struct job *jp));
#ifndef	HAVE_GETRUSAGE
extern	int	getrusage	__PR((int who, struct rusage *r_usage));
#endif
extern	void	shmcreate	__PR((void));

/*
 * macro.c
 */
#define	M_PARM		1	/* Normal parameter expansion	*/
#define	M_COMMAND	2	/* Command substitution		*/
#define	M_DOLAT		4	/* $@ was expanded		*/
#define	M_ARITH		8	/* $(()) was expanded		*/
#define	M_SPLIT		(M_PARM|M_COMMAND|M_DOLAT|M_ARITH)
#define	M_NOCOMSUBST	0x100	/* Do not expand ` ` and $()	*/
extern	unsigned char *macro	__PR((unsigned char *as));
extern	unsigned char *_macro	__PR((unsigned char *as));
extern	void	subst		__PR((int in, int ot));

/*
 * main.c
 */
extern	void	chkpr		__PR((void));
extern	void	settmp		__PR((void));
extern	void	chkmail		__PR((void));
extern	void	setmail		__PR((unsigned char *));
#define	setmode		set_imode	/* Conflicts w. FreeBSD libc function */
extern	void	setmode		__PR((int prof));
extern	void	secpolicy_print __PR((int level, const char *msg));

/*
 * name.c
 */
extern	int	syslook		__PR((unsigned char *w,
						const struct sysnod syswds[],
						int n));
extern	const struct sysnod *
		sysnlook	__PR((unsigned char *w,
					const struct sysnod syswds[], int n));
extern	void	setlist		__PR((struct argnod *arg, int xp));
extern	void	setname		__PR((unsigned char *nam, int xp));
extern	void	replace		__PR((unsigned char **a, unsigned char *v));
extern	void	dfault		__PR((struct namnod *n, unsigned char *v));
extern	void	assign		__PR((struct namnod *n, unsigned char *v));
extern	int	readvar		__PR((int namec, unsigned char **names));
extern	void	assnum		__PR((unsigned char **p, long i));
extern	unsigned char *make	__PR((unsigned char *v));
extern	struct namnod *lookup	__PR((unsigned char *nam));
extern	void	namscan		__PR((void (*fn)(struct namnod *n)));
extern	void	printfunc	__PR((struct namnod *n));
extern	void	printnam	__PR((struct namnod *n));
#ifdef	DO_LINENO
extern	unsigned char *linenoval __PR((void));
#endif
extern	void	printro		__PR((struct namnod *n));
extern	void	printpro	__PR((struct namnod *n));
extern	void	printexp	__PR((struct namnod *n));
extern	void	printpexp	__PR((struct namnod *n));
extern	void	printlocal	__PR((struct namnod *n));
extern	void	exportenv	__PR((struct namnod *n));
extern	void	deexportenv	__PR((struct namnod *n));
extern	void	pushval		__PR((struct namnod *n, void *t));
extern	void	popvars		__PR((void));
extern	void	poplvars	__PR((void));
extern	void	popval		__PR((struct namnod *n));
extern	void	setup_env	__PR((void));
extern	unsigned char **local_setenv __PR((int flg));
extern	unsigned char **get_envptr __PR((void));
extern	struct namnod *findnam	__PR((unsigned char *nam));

#define	UNSET_FUNC	1
#define	UNSET_VAR	2
extern	void	unset_name	__PR((unsigned char *name, int uflg));
#ifdef INTERACTIVE
extern	char	*getcurenv	__PR((char *name));
extern	void	ev_insert	__PR((char *name));
#endif

/*
 * print.c
 */
extern	void	prp		__PR((void));
extern	void	prs		__PR((unsigned char *as));
extern	void	prc		__PR((unsigned char c));
extern	void	prwc		__PR((wchar_t c));
extern	void	clock2tv	__PR((clock_t t, struct timeval	*tp));
extern	void	prt		__PR((long t));
extern	void	prtv		__PR((struct timeval *tp, int digs, int lf));
extern	void	prn		__PR((int n));
extern	void	itos		__PR((int n));
extern	void	sitos		__PR((int n));
extern	int	stoi		__PR((unsigned char *icp));
extern	int	stosi		__PR((unsigned char *icp));
extern	int	sltos		__PR((long n));
extern	int	slltos		__PR((Intmax_t n));
extern	int	ltos		__PR((long n));

extern	void	flushb		__PR((void));
extern	void	unprs_buff	__PR((int));
extern	void	prc_buff	__PR((unsigned char c));
extern	int	prs_buff	__PR((unsigned char *s));
extern	void	prs_cntl	__PR((unsigned char *s));
extern	void	qprs_buff	__PR((unsigned char *s));
extern	void	prull_buff	__PR((UIntmax_t lc));
extern	void	prl_buff	__PR((long l));
extern	void	prn_buff	__PR((int n));
extern	int	setb		__PR((int fd));
extern	unsigned char *endb	__PR((void));


/*
 * pwd.c
 */
extern	int	lchdir		__PR((char *path));
extern	int	sh_hop_dirs	__PR((char *name, char **np));
extern	int	lstatat		__PR((char *name, struct stat *buf, int flag));
extern	void	cwd		__PR((unsigned char *dir,
					unsigned char *cwdbase));
extern	int	cwdrel2abs	__PR((void));
extern	unsigned char *cwdget	__PR((int cdflg));
extern	unsigned char *cwdset	__PR((void));
extern	void	cwdprint	__PR((int cdflg));
extern	void	ocwdnod		__PR((void));
extern	struct argnod *push_dir	__PR((unsigned char *name));
extern	struct argnod *pop_dir	__PR((int offset));
extern	void	init_dirs	__PR((void));
extern	int	pr_dirs		__PR((int minlen, int cdflg));

/*
 * service.c
 */
extern	short		initio	__PR((struct ionod *iop, int save));
extern	unsigned char *simple	__PR((unsigned char *s));
extern	unsigned char *getpath	__PR((unsigned char *s));
extern	int		pathopen __PR((unsigned char *path,
						unsigned char *name));
extern	unsigned char *catpath	__PR((unsigned char *path,
						unsigned char *name));
extern	unsigned char *nextpath	__PR((unsigned char *path));
extern	void		execa	__PR((unsigned char *at[], short pos,
						int isvfork,
						unsigned char *av0));
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

#define	GROWSTAK(a)	if ((a) >= brkend) \
				(a) = growstak(a);
/*
 * "a" needs to be a char * object
 */
#define	GROWSTAKL(a, l)	do { if (((a) + (l)) >= brkend) {	\
				(a) += (l);			\
				(a) = growstak(a);		\
				(a) -= (l);			\
			} } while (0);
#define	GROWSTAK2(a, o)	do { if ((a) >= brkend) {		\
				char *oa = (char *)(a);		\
				(a) = growstak(a);		\
				(o) += (char *)(a) - oa;	\
			} } while (0);
#define	GROWSTAKTOP()	do { if (staktop >= brkend)		\
				(void) growstak(staktop);	\
			} while (0);
#define	GROWSTAKTOPL(l)	do { if ((staktop + (l)) >= brkend) {	\
				staktop += (l);			\
				(void) growstak(staktop);	\
				staktop -= (l);			\
			} } while (0);

extern	void		*alloc		__PR((size_t));
extern	void		free		__PR((void *ap));
extern	void		libc_free	__PR((void *ap));
extern	unsigned char *getstak		__PR((Intptr_t asize));
extern	unsigned char *locstak		__PR((void));
extern	unsigned char *growstak		__PR((unsigned char *newtop));
extern	unsigned char *savstak		__PR((void));
extern	unsigned char *endstak		__PR((unsigned char *argp));
extern	void		tdystak		__PR((unsigned char *x,
							struct ionod *iosav));
extern	void		stakchk		__PR((void));
extern	unsigned char *cpystak		__PR((unsigned char *));
extern	unsigned char *movstrstak	__PR((unsigned char *a,
							unsigned char *b));
extern	unsigned char *memcpystak	__PR((unsigned char *s1,
							unsigned char *s2,
							int n));

/*
 * strexpr.c
 */
extern	Intmax_t	strexpr	__PR((unsigned char *arg, int *errp));

/*
 * string.c
 */
extern	unsigned char *movstr	__PR((unsigned char *a, unsigned char *b));
extern	int		any	__PR((wchar_t c, unsigned char *s));
extern	int		anys	__PR((unsigned char *c, unsigned char *s));
extern	int		clen	__PR((unsigned char *c));
extern	int		cf	__PR((unsigned char *s1, unsigned char *s2));
extern	int		length	__PR((unsigned char *as));
extern	unsigned char *movstrn	__PR((unsigned char *a,
						unsigned char *b, int n));

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
#ifndef	SIG2STR_MAX		/* From Solaris signal.h */
#define	SIG2STR_MAX	32
#endif

/*
 * test.c
 */
extern	int	test		__PR((int argn, unsigned char *com[]));
#ifdef	DO_SYSATEXPR
extern	void	expr		__PR((int argn, unsigned char *com[]));
#endif

/*
 * ulimit.c
 */
extern	void	sysulimit	__PR((int argc, unsigned char **argv));

/*
 * umask.c
 */
extern	void	sysumask	__PR((int argc, char **argv));

/*
 * alias.c
 */
#ifdef	DO_SYSALIAS
extern	void	sysalias	__PR((int argc, unsigned char **argv));
extern	void	sysunalias	__PR((int argc, unsigned char **argv));
#endif

/*
 * builtin.c
 */
#ifdef	DO_SYSBUILTIN
extern	void	sysbuiltin	__PR((int argc, unsigned char **argv));
extern	struct sysnod2 *sh_findbuiltin	__PR((unsigned char *name));
#endif

/*
 * find.c
 */
#ifdef	DO_SYSFIND
extern	int	sysfind		__PR((int argc, Uchar **argv, bosh_t *));
#endif

/*
 * optget.c
 */
extern	void	optinit		__PR((struct optv *optv));
extern	int	optget		__PR((int argc, unsigned char **argv,
					struct optv *optv,
					const char *optstring));
extern	void	optbad		__PR((int argc, unsigned char **argv,
					struct optv *optv));
extern	int	optnext		__PR((int argc, unsigned char **argv,
					struct optv *optv,
					const char *optstring,
					const char *use));
extern	int	optskip		__PR((int argc, unsigned char **argv,
					const char *use));

/*
 * word.c
 */
extern	int		word	__PR((void));
extern	unsigned char	*match_arith	__PR((unsigned char *argp));
extern	unsigned int	skipwc	__PR((void));
extern	unsigned int	nextwc	__PR((void));
extern	unsigned char	*readw	__PR((wchar_t d));
extern	unsigned int	readwc	__PR((void));
extern	int		isbinary __PR((struct fileblk *f));
extern	unsigned char	*do_tilde __PR((unsigned char *arg));

/*
 * xec.c
 */
extern	int	execute		__PR((struct trenod *argt, int xflags,
						int errorflg,
						int *pf1, int *pf2));
extern	unsigned char *ps_macro	__PR((unsigned char *as, int perm));
extern	void	execexp		__PR((unsigned char *s, Intptr_t f,
						int xflags));
extern	int	callsh		__PR((int ac, char **av));


#define		_cf(a, b)	cf((unsigned char *)(a), (unsigned char *)(b))
#define		attrib(n, f)	((n)->namflg |= f)
#define		round(a, b)	(((Intptr_t)(((char *)(a)+b)-1))&~((b)-1))
#define		closepipe(x)	(close(x[INPIPE]), close(x[OTPIPE]))
#define		eq(a, b)	(_cf(a, b) == 0)
#undef		max
#define		max(a, b)	((a) > (b)?(a):(b))
#define		assert(x)
#define		_gettext(s)	(unsigned char *)gettext(s)

/*
 * Exit shell or longjmp before next prompt.
 *
 * macros using failed_real(). Only s2 is gettext'd with both functions.
 *
 * Called from "fatal errors", from locations where either a real exit() is
 * expected or from an interactive command where a longjmp() to the next prompt
 * is expected.
 */
#define		failed(s1, s2)		failed_real(ERROR, s1, s2, NULL)
#define		failedx(e, s1, s2)	failed_real(e, s1, s2, NULL)
#define		bfailed(s1, s2, s3)	failed_real(ERROR, s1, s2, s3)
#define		bfailedx(e, s1, s2, s3)	failed_real(e, s1, s2, s3)

/*
 * Prepare non-zero $? for this command.
 *
 * macros using failure_real(). s1 and s2 is gettext'd with gfailure(), but
 * only s2 is gettext'd with failure().
 *
 * From a normal error that usually does not cause an exit() of the shell.
 * Except when "set -e" has been issued, we just set up $? and return.
 */
#define		failure(s1, s2)		failure_real(ERROR, s1, s2, NULL, 0)
#define		failurex(e, s1, s2)	failure_real(e, s1, s2, NULL, 0)
#define		bfailure(s1, s2, s3)	failure_real(ERROR, s1, s2, s3, 0)
#define		bfailurex(e, s1, s2, s3) failure_real(e, s1, s2, s3, 0)
#define		gfailure(s1, s2)	failure_real(ERROR, s1, s2, NULL, 1)
#define		gfailurex(e, s1, s2)	failure_real(e, s1, s2, NULL, 1)
#define		gbfailure(s1, s2, s3)	failure_real(ERROR, s1, s2, s3, 1)
#define		gbfailurex(e, s1, s2, s3) failure_real(e, s1, s2, s3, 1)

/*
 * Failure macros for builtin commands that historically aborted scripts
 * in case of syntax errors or "fatal errors".
 *
 * Should we make this runtime settable?
 */
#ifdef	DO_POSIX_FAILURE
#define		Failure(s1, s2)		failure(s1, s2)
#define		Failurex(e, s1, s2)	failurex(e, s1, s2)
#define		BFailure(s1, s2, s3)	bfailure(s1, s2, s3)
#define		BFailurex(e, s1, s2, s3) bfailurex(e, s1, s2, s3)
#define		Error(s1)		gfailure(UC s1, NULL)
#else
#define		Failure(s1, s2)		failed(s1, s2)
#define		Failurex(e, s1, s2)	failedx(e, s1, s2)
#define		BFailure(s1, s2, s3)	bfailed(s1, s2, s3)
#define		BFailurex(e, s1, s2, s3) bfailedx(e, s1, s2, s3)
#define		Error(s1)		error(s1)
#endif

/* temp files and io */
extern int		output;
extern int		ioset;
extern struct ionod	*iotemp; /* files to be deleted sometime */
extern struct ionod	*xiotemp; /* limit for files to be deleted sometime */
extern struct ionod	*fiotemp; /* function files to be deleted sometime */
extern struct ionod	*iopend; /* documents waiting to be read at NL */
extern struct fdsave	fdmap[];
extern int savpipe;

/* substitution */
extern int		dolc;
extern unsigned char	**dolv;
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
extern const char		atline[];
extern const char		readmsg[];
extern const char		selectmsg[];
extern const char		colon[];
extern const char		minus[];
extern const char		nullstr[];
extern const char		sptbnl[];
extern const char		unexpected[];
extern const char		endoffile[];
extern const char		synmsg[];

/* name tree and words */
extern const struct sysnod	reserved[];
extern const int		no_reserved;
extern const struct sysnod	commands[];
extern const int		no_commands;
extern const struct sysnod	test_ops[];
extern const int		no_test_ops;

extern int			wdval;
extern int			wdnum;
extern int			fndef;
extern int			nohash;
extern struct argnod		*wdarg;
extern int			wdset;
extern BOOL			reserv;

/* prompting */
extern const char		stdprompt[];
extern const char		supprompt[];
extern const char		profile[];
extern const char		sysprofile[];
extern const char		rcfile[];
extern const char		sysrcfile[];
extern const char		globalname[];
extern const char		localname[];
extern const char		fcedit[];

/* locale testing */
extern const char		localedir[];
extern int			localedir_exists;

/* built in names */
extern struct namnod		fngnod;
extern struct namnod		cdpnod;
extern struct namnod		envnod;
extern struct namnod		fcenod;
extern struct namnod		ifsnod;
extern struct namnod		homenod;
extern struct namnod		pwdnod;
extern struct namnod		opwdnod;
extern struct namnod		linenonod;
extern struct namnod		mailnod;
extern struct namnod		pathnod;
extern struct namnod		ppidnod;
extern struct namnod		ps1nod;
extern struct namnod		ps2nod;
extern struct namnod		ps3nod;
extern struct namnod		ps4nod;
extern struct namnod		mchknod;
extern struct namnod		repnod;
extern struct namnod		acctnod;
extern struct namnod		mailpnod;
extern struct namnod		timefmtnod;
extern struct namnod		*optindnodep;

/* special names */
extern unsigned char		flagadr[];
extern unsigned char		*pcsadr;
extern unsigned char		*pidadr;
extern unsigned char		*cmdadr;

/* names always present */
extern const char		defpath[];
extern const char		defppath[];
extern const char		linenoname[];
extern const char		mailname[];
extern const char		homename[];
extern const char		pathname[];
extern const char		ppidname[];
extern const char		cdpname[];
extern const char		envname[];
extern const char		fcename[];
extern const char		ifsname[];
extern const char		ps1name[];
extern const char		ps2name[];
extern const char		ps3name[];
extern const char		ps4name[];
extern const char		mchkname[];
extern const char		opwdname[];
extern const char		pwdname[];
extern const char		repname[];
extern const char		acctname[];
extern const char		mailpname[];
extern const char		timefmtname[];

/* transput */
extern unsigned char		tmpout[];
extern int 			tmpout_offset;
extern unsigned int		serial;

/*
 * allow plenty of room for size for temp file name:
 * "/tmp/sh"(7) + <pid> (<=6) + <unsigned int #> (<=10) + \0 (1)
 */
#define		TMPOUTSZ	32

extern struct fileblk	*standin;

#define		input		(standin->fdes)
#define		eof		(standin->feof)

#define	peekc	peekc_		/* AIX has a hidden peekc() in libc */
extern int			peekc;
extern int			peekn;
extern unsigned char		*comdiv;
extern const char		devnull[];

/*
 * flags
 */
#define		noexec		01		/* set -n noexec */
#define		intflg		02		/* set -i interactive */
#define		prompt		04
#define		setflg		010		/* set -u nounset */
#define		errflg		020		/* set -e errexit */
#define		ttyflg		040		/* in + out is a tty */
#define		forked		0100		/* *forked child */
#define		oneflg		0200		/* set -t onecmd */
#define		rshflg		0400		/* set -r restrictive */
#define		subsh		01000		/* Is a subshell */
#define		stdflg		02000		/* set -s stdin */
#define		STDFLG		's'
#define		execpr		04000		/* set -x xtrace */
#define		readpr		010000		/* set -v verbose */
#define		keyflg		020000		/* set -k keyword */
#define		hashflg		040000		/* set -h hashall */
#define		vforked		0100000		/* A vforked child */
#define		nofngflg	0200000		/* set -f noglob */
#define		exportflg	0400000		/* set -a allexport */
#define		monitorflg	01000000	/* set -m monitor */
#define		jcflg		02000000	/* Do jobcontrol */
#define		privflg		04000000	/* set -p Keep privs */
#define		forcexit	010000000	/* Terminate shell */
#define		jcoff		020000000	/* Tmp jobcontrol off */
#define		pfshflg		040000000	/* set -P pfexec() support */
#define		ppath		0100000000	/* Use POSIX default PATH */
#define		noexit		0200000000	/* Inhibit exit from builtins */
#define		nofuncs		0400000000	/* Inhibit functions */

/*
 * We currently support up to 4x 30 flag bits, but we currently only use
 * 2x 30 flag bits.
 * If we start to use 'fl3' and 'fl4', we need to ass support to args.c
 */
#define		fl2		010000000000	/* If in flagval: see flags2 */
#define		fl3		020000000000	/* If in flagval: see flags3 */
#define		fl4		(fl2 | fl3)	/* If in flagval: see flags4 */

/*
 * flags2
 */
#define		fdpipeflg	01		/* e.g. 2| pipe from stderr */
#define		timeflg		02		/* set -o time		*/
#define		systime		04		/* "time pipeline"	*/
#define		fullexitcodeflg	010		/* set -o fullexitcode	*/
#define		hashcmdsflg	020		/* set -o hashcmds	*/
#define		hostpromptflg	040		/* set -o hostprompt	*/
#define		noclobberflg	0100		/* set -o noclobber	*/
#define		bgniceflg	0200		/* set -o bgnice	*/
#define		ignoreeofflg	0400		/* set -o ignoreeof	*/
#define		notifyflg	01000		/* set -o notify / -b	*/
#define		versflg		02000		/* set -V (print version) */
#define		globalaliasflg	04000		/* set -o globalaliases */
#define		localaliasflg	010000		/* set -o localaliases  */
#define		aliasownerflg	020000		/* set -o aliasowner=   */
#define		viflg		040000		/* set -o vi VI cmdln. edit  */
#define		vedflg		0100000		/* set -o ved VED cmdln. edit */
#define		posixflg	0200000		/* set -o posix		*/
#define		promptcmdsubst	0400000		/* set -o promptcmdsubst */
#define		globskipdot	01000000	/* set -o globskipdot	*/

extern unsigned long		flags;		/* Flags for set(1) and more */
extern unsigned long		flags2;		/* Second set of flags */
extern int			exflag;		/* Use _exit(), not exit() */
extern int			rwait;		/* flags read waiting */
#ifdef	DO_POSIX_SET
extern int			dashdash;	/* flags set -- encountered */
#endif

/* error exits from various parts of shell */
extern jmp_buf			subshell;
extern jmp_buf			errshell;
extern jmps_t			*dotshell;

/* fault handling */
extern unsigned			brkincr;
#define		MINTRAP		0
#ifdef	DO_ERR_TRAP
#define		MAXTRAP		(NSIG+1)
#define		ERRTRAP		(MAXTRAP-1)
#else
#define		MAXTRAP		NSIG
#endif
#define		MAX_SIG		NSIG

#define		TRAPSET		2		/* Mark incoming signal/fault */
#define		SIGSET		4		/* Mark fault w. no trapcmd */
#define		SIGMOD		8		/* Signal with trap(1) set */
#define		SIGIGN		16		/* Signal is ignored	*/
#define		SIGCLR		32		/* From oldsigs() w. vfork() */
#define		SIGINP		64		/* Signal in line editor inp */

extern BOOL			trapnote;
extern int			traprecurse;
extern int			trapsig;
extern unsigned char		*trapcom[];

/* name tree and words */
#ifdef	HAVE_ENVIRON_DEF
/*
 * Warning: HP-UX and Linux have "extern char **environ;" in unistd.h
 * We should not use our own definitions here.
 */
#else
extern char			**environ;
#endif

#define		NUMBUFLEN	21	/* big enough for 64 bits */
#if	NUMBUFLEN < SIG2STR_MAX
#undef		NUMBUFLEN
#define		NUMBUFLEN	(SIG2STR_MAX-1)
#endif
extern unsigned char		numbuf[];
extern const char		export[];
extern const char		duperr[];
extern const char		readonly[];

/* execflgs */
extern struct excode		ex;
extern struct excode		retex;
#ifdef	DO_DOL_SLASH
extern	int			*excausep;
#endif
extern int			exitval;
extern int			retval;
extern BOOL			execbrk;
extern BOOL			dotbrk;
extern int			loopcnt;
extern int			breakcnt;
extern int			funcnt;
extern int			dotcnt;
extern void			*localp;
extern int			localcnt;
extern int			tried_to_exit;

/* fault */
extern int			*intrptr;

/* messages */
extern const char		mailmsg[];
extern const char		coredump[];
extern const char		badopt[];
extern const char		emptystack[];
extern const char		badparam[];
extern const char		unset[];
extern const char		badsub[];
extern const char		nospace[];
extern const char		nostack[];
extern const char		notfound[];
extern const char		badtrap[];
extern const char		baddir[];
extern const char		badoff[];
extern const char		badshift[];
extern const char		restricted[];
extern const char		execpmsg[];
extern const char		notid[];
extern const char 		badulimit[];
extern const char 		ulimit[];
extern const char		wtfailed[];
extern const char		badcreate[];
extern const char		eclobber[];
extern const char		nofork[];
extern const char		noswap[];
extern const char		piperr[];
extern const char		badopen[];
extern const char		badnum[];
extern const char		badsig[];
extern const char		badid[];
extern const char		arglist[];
extern const char		txtbsy[];
extern const char		toobig[];
extern const char		badexec[];
extern const char		badfile[];
extern const char		badreturn[];
extern const char		badexport[];
extern const char		badunset[];
extern const char		nohome[];
extern const char		badperm[];
extern const char		mssgargn[];
extern const char		toomanyargs[];
extern const char		libacc[];
extern const char		libbad[];
extern const char		libscn[];
extern const char		libmax[];
extern const char		emultihop[];
extern const char		nulldir[];
extern const char		enotdir[];
extern const char		eisdir[];
extern const char		enoent[];
extern const char		eacces[];
extern const char		enolink[];
extern const char		exited[];
extern const char		running[];
extern const char		ambiguous[];
extern const char		nosuchjob[];
extern const char		nosuchpid[];
extern const char		nosuchpgid[];
extern const char		usage[];
extern const char		nojc[];
extern const char		killuse[];
extern const char		jobsuse[];
extern const char		aliasuse[];
extern const char		unaliasuse[];
extern const char		repuse[];
extern const char		builtinuse[];
extern const char		stopuse[];
extern const char		trapuse[];
extern const char		ulimuse[];
extern const char		limuse[];
extern const char		nocurjob[];
extern const char		loginsh[];
extern const char		jobsstopped[];
extern const char		jobsrunning[];
extern const char		nlorsemi[];
extern const char		signalnum[];
extern const char		badpwd[];
extern const char		badlocale[];
extern const char		nobracket[];
extern const char		noparen[];
extern const char		noarg[];
extern const char		unimplemented[];
extern const char		divzero[];

/*
 * 'builtin' error messages
 */
extern const char		btest[];
extern const char		badop[];
extern const char		badumask[];

/*
 * Shell name
 */
extern const unsigned char	shname[];
extern	    unsigned char	*shvers;

/*	fork constant	*/
#define		FORKLIM 	32

extern address			end[];

extern int			eflag;
extern int			ucb_builtins;

/*
 * Find out if it is time to go away.
 * `trapnote' is set to SIGSET when fault is seen and
 * no trap has been set.
 */
#define		sigchk()	if (trapnote & SIGSET)	{		\
					exval_sig();			\
					if (!traprecurse) {		\
						exitsh(exitval ?	\
						    exitval : SIGFAIL); \
					}				\
				}

#define		exitset()	(retex = ex, retval = exitval)

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
#define	POSIXJOBS
#endif

#if	!defined(HAVE_GETPGID) && defined(HAVE_BSD_GETPGRP)
#define	getpgid	getpgrp
#define	getsid	getpgrp
#endif
#if	!defined(HAVE_GETSID) && defined(HAVE_BSD_GETPGRP)
#define	getsid	getpgrp
#endif
#if	!defined(HAVE_SETPGID) && defined(HAVE_BSD_SETPGRP)
#define	setpgid	setpgrp
#endif
#if	!defined(HAVE_SETSID) && defined(HAVE_BSD_GETPGRP)
#define	setsid	setpgrp(getpid())
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
#define	getpgid(a)	getpgrp()
#endif

#if	!defined(HAVE_GETSID) && !defined(HAVE_BSD_GETPGRP)
#define	getsid	getpgid
#endif

#ifdef	SCHILY_INCLUDES

#ifdef	USE_BYTES
#define	memset(s, c, n)		fillbytes(s, n, c)
#define	memchr(s, c, n)		findbytes(s, n, c)
#define	memcpy(s1, s2, n)	movebytes(s2, s1, n)
#define	memmove(s1, s2, n)	movebytes(s2, s1, n)
#endif

#if !defined(HAVE_MEMSET) && !defined(memset)
#define	memset(s, c, n)		fillbytes(s, n, c)
#endif
#if !defined(HAVE_MEMCHR) && !defined(memchr)
#define	memchr(s, c, n)		findbytes(s, n, c)
#endif
#if !defined(HAVE_MEMCPY) && !defined(memcpy)
#define	memcpy(s1, s2, n)	movebytes(s2, s1, n)
#endif
#if !defined(HAVE_MEMMOVE) && !defined(memmove)
#define	memmove(s1, s2, n)	movebytes(s2, s1, n)
#endif

/*
 * <schily/libport.h> is needed for various stuff that may be missing on the
 * current platform and that is implemented in libschily for portability.
 * If we are missing memset(), memchr(), memcpy() or memmove(), we use the
 * *bytes() functions from libschily that have prototypes in <schily/schily.h>.
 * There are several historic name conflicts in libschily and the Bourne Shell
 * that we need to take care of.
 */
#define	BOOL	JS_BOOL			/* Bourne Shell uses other BOOL size */
#undef	peekc				/* First remove AIX cludge	*/
#define	peekc	js_peekc		/* The Bourne Shell has int peekc */
#define	error	js_error		/* Bourne Shell has own error()	*/
#define	flush	js_flush		/* Bourne Shell has own flush()	*/
#define	getperm	js_getperm		/* Bourne Shell modified getperm() */
#define	eaccess	__no_eaccess__		/* libgen.h / -lgen contain eaccess() */
#include	<schily/schily.h>	/* Includes <schily/libport.h>	*/
#undef	eaccess				/* No longer needed		*/
#undef	BOOL				/* Back to BOOL from Bourne Shell */
#undef	peekc				/* Remove schily/schily.h cludge */
#define	peekc	peekc_			/* AIX has a hidden peekc() in libc */
#undef	error				/* Reestablish Bourne Shell error() */
#undef	flush				/* Reestablish Bourne Shell flush() */
#undef	getperm				/* Reestablish Bourne Shell getperm() */

#endif	/* SCHILY_INCLUDES */

#ifdef	__cplusplus
}
#endif

#endif	/* _DEFS_H */
