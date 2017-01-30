/* @(#)bsh.h	1.69 17/01/18 Copyright 1985-2017 J. Schilling */
/*
 *	Bsh general definitions
 *
 *	Copyright (c) 1985-2017 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/ccomdefs.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Include sys/types.h for off_t */
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/errno.h>
#include "bshconf.h"
#ifdef	SHORT_NAMES
#include "shortnames.h"
#endif


#define	F_NULL	(int (*) ()) 0

#define	TEST			/* include test code */

/*
 * Our internal argc/argv structure.
 */
typedef struct {
	int	av_ac;
	char	*av_av[1];
} Argvec;

/*
 * Node for parsed command tree
 */
typedef struct Tnode {
	union ptr {
		struct Tnode	*tn_node;
		Argvec		*tn_avec;
		char		*tn_str;
	} tn_left, tn_right;
	long	tn_type;
} Tnode;

/*
 * Possible types for a Tnode
 */
/* XXX da das im switch vorkommt, sollte es ein int sein. d.h. < 0xFFFF */
#define	TYPMASK		0xFFFFL

#define	xntype(type)	((long)((type) & TYPMASK))
#define	ntype(np)	((long)((np)->tn_type & TYPMASK))
#define	quotetype(np)	((long)((np)->tn_type & ~TYPMASK))
#define	nlen(np)	((int)(((np)->tn_type & ~TYPMASK) >> 16))
#define	nsetlen(np, len)((np)->tn_type = LSTRING | (((int)(len)) << 16))
#define	sxnlen(len)	(LSTRING | (((int)(len)) << 16))

#define	STRING	0x0000L		/* simple non quoted string */
#define	LSTRING	0x0001L		/* non quoted string tagged with len arg */
#define	LIST	0x0002L		/* not used */
#define	VECTOR	0x0003L		/* not used */
#define	CMD	0x0004L		/* non parentheses'ed cmd list */
#define	NOQUOTE	0x00000L	/* non quoted string (same as STRING) */
#define	SQUOTE	0x10000L	/* sinlgle quoted string (\') */
#define	DQUOTE	0x20000L	/* double quoted string (") */
#define	BQUOTE	0x40000L	/* back quoted string (`) */
#define	NENV	0x100000L	/* This is a "env" setting Tnode */

#define	ERRPIPE	(('|'<<8)|'%')	/* pipe on stderr |% */
#define	ANDP	(('&'<<8)|'&')	/* conditional AND execution */
#define	ORP	(('|'<<8)|'|')	/* conditional OR  execution */
#define	OUTAPP	(('>'<<8)|'>')	/* redirect stdout (append mode) >> */
#define	ERRAPP	(('%'<<8)|'%')	/* redirect stderr (append mode) %% */
#define	DOCIN	(('<'<<8)|'<')	/* here document stdin redirection << */
#define	IDUP	(('<'<<8)|'&')	/* duplicate output fd n<& */
#define	ODUP	(('>'<<8)|'&')	/* duplicate output fd n>& */

/*
 * Possible flags for command execution
 */
#define	BGRND	0x0001		/* execute this command in background */
#define	ASYNC	0x0002		/* this command not to be waited for */
#define	PRPID	0x0004		/* print pid on fork() for this command */
#define	NOSIG	0x0008		/* ignore signals for this command */
#define	LOPRI	0x0010		/* execute this command with lower pri */
#define	NULIN	0x0020		/* set up /dev/null for stdin */
#define	SUBSH	0x2000		/* this process is a subshell */
#define	NOPGRP	0x4000		/* do no process group management at all */
#define	PGRP	0x8000		/* do no further process group setup */
#define	WALL	0x10000		/* wait for all childs */
#define	NOTMS	0x20000		/* do not print TIME info */
#define	ENV	0x40000		/* use builtin "env" for name=value */
#define	DIDFORK	0x80000		/* did fork already */
#define	NOVFORK	0x100000	/* do not vfork(), use shfork() instead */
#define	IGNENV	0x200000	/* this is a command from env -i ... */

/*
 * Global data
 */
extern int	ctlc;		/* SIGINT received, abort parsing/builtins  */
extern int	parseflg;	/* Shell is in parser			    */
extern int	vflg;		/* Verbose command execution		    */
extern int	iflg;		/* Intercative command execution (-i / tty) */
extern int	no_histflg;	/* Don't use ~/.history file.		    */
extern int	pfshell;	/* Be a pfexec shell			    */
extern int	mailcheck;	/* Mail check interval			    */
extern int	vac;		/* The arg count in global arg Argvec	$#  */
extern char	**vav;		/* The global arg Argvec		$*  */
extern sigtype	osig2;		/* Old Signal #2  (SIGINT)		    */
extern sigtype	osig3;		/* Old Signal #3  (SIGQUIT)		    */
extern sigtype	osig15;		/* Old Signal #15 (SIGTERM)		    */
extern sigtype	osig18;		/* Old Signal #18 (SIGTSTP)		    */
extern sigtype	osig21;		/* Old Signal #21 (SIGTTIN)		    */
extern sigtype	osig22;		/* Old Signal #22 (SIGTTOU)		    */
extern FILE	*gstd[];	/* The global stdio Argvec (std{in!out!err} */
extern FILE	*cmdfp;		/* File pointer to the current shell script */
extern int	ex_status;	/* The global exit status ($?)		    */
extern char	**evarray;	/* The environment array managed by us	    */
extern char	*prompts[];	/* A list of prompts used by the input editor */
extern FILE	*protfile;	/* File pointer used for the cmd protocol   */
#ifdef	VFORK
extern	char	*Vlist;		/* To free things allocated by vfork() child */
extern	char	*Vtmp;		/* To free things allocated by vfork() child */
extern	char	**Vav;		/* To free things allocated by vfork() child */
#endif

#define	MOREPROMPT	"> "
#define	EDITPROMPT	"edit> "

/*
 * bsh.c
 */
extern	sigret	intr		__PR((int sig));
extern	int	main		__PR((int ac, char **av, char **ev));
extern	BOOL	dofile		__PR((char *s, int tab, int flag,
						FILE **std, BOOL  jump));
extern	void	doopen		__PR((FILE *fd, char *s, int tab, int flag,
						FILE **std, BOOL  jump));
extern	void	process		__PR((FILE *f, int flag,
						FILE **std, BOOL  jump));
extern	int	berror		__PR((const char *s, ...)) __printflike__(1, 2);
extern	char	*errstr		__PR((int err));
extern	void	close_other_files	__PR((FILE **std));
extern	char	*getuname	__PR((int uid));
extern	char	*getpwdir	__PR((char *name));
extern	char	*mypwhome		__PR((void));
extern	char	*myhome		__PR((void));
extern	void	exitbsh		__PR((int excode));

/*
 * parse.c
 */
extern	Tnode	*cmdline	__PR((int flag, FILE **std, int buildcmd));
extern	Tnode	*pword		__PR((void));
extern	char	*pstring	__PR((char *terms, int quotec));
extern	BOOL	argend		__PR((char *term, int quotec));
extern	int	skipwhite	__PR((void));
extern	void	eatline		__PR((void));
extern	void	syntax		__PR((char *s, ...)) __printflike__(1, 2);

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
EXPORT	int	sig2str		__PR((int sig, char *s));
#endif

/*
 * hashcmd.c
 */
extern	void	hashcmd		__PR((FILE **std));

/*
 * exec.c
 */
extern	int	execute		__PR((Tnode *cmd, int flag, FILE **std));
extern	int	execcmd		__PR((Argvec *vp, FILE **std, int flag));
extern	Argvec*	scan		__PR((Tnode *cmd));

/*
 * sys.c
 */
extern	void	start		__PR((Argvec * vp, FILE ** std, int flag));
extern	pid_t	shfork		__PR((int flag));
extern	void	pset		__PR((pid_t child, int flag));
extern	void	block_sigs	__PR((void));
extern	void	unblock_sigs	__PR((void));
extern	int	ewait		__PR((pid_t child, int flag));
extern	int	fexec		__PR((char **path, char *name,
						FILE *in, FILE *out, FILE *err,
						char **av, char **env));
extern	char	*_findinpath	__PR((char *name, int mode, BOOL plain_file));

/*
 * builtin.c
 */
extern	void	wrong_args	__PR((Argvec *vp, FILE **std));
extern	void	unimplemented	__PR((Argvec *vp, FILE **std));
extern	BOOL	busage		__PR((Argvec *vp, FILE **std));
extern	BOOL	toint		__PR((FILE **std, char *s, int *i));
extern	BOOL	tolong		__PR((FILE **std, char *s, long *l));
#ifdef	_SCHILY_UTYPES_H
extern	BOOL	tollong		__PR((FILE **std, char *s, Llong *ll));
#endif
extern	BOOL	builtin		__PR((Argvec *vp, FILE **std, int flag));

/*
 * call.c
 */
extern	void	bsignal		__PR((Argvec *vp, FILE **std, int flag));
extern	void	esigs		__PR((void));
extern	void	bfunc		__PR((Argvec *vp, FILE **std, int flag));
extern	void	breturn		__PR((Argvec *vp, FILE **std, int flag));
extern	BOOL	func_call	__PR((Argvec *vp, FILE **std, int flag));
extern	char	*map_func	__PR((char *name));

/*
 * call.c
 */
extern	void	push		__PR((int id));
extern	void	freestack	__PR((void));
extern	char	*cons_args	__PR((Argvec *vp, int n));
extern	void	bif		__PR((Argvec *vp, FILE **std, int flag));
extern	void	bfor		__PR((Argvec *vp, FILE **std, int flag));
extern	void	bloop		__PR((Argvec *vp, FILE **std, int flag));
extern	void	bread		__PR((Argvec *vp, FILE **std, int flag));
extern	void	bswitch		__PR((Argvec *vp, FILE **std, int flag));

/*
 * dirs.c
 */
extern	void	update_cwd	__PR((void));
extern	void	bpwd		__PR((Argvec *vp, FILE **std, int flag));
extern	void	bdirs		__PR((Argvec *vp, FILE **std, int flag));
extern	void	bcd		__PR((Argvec *vp, FILE **std, int flag));

/*
 * test.c
 */
extern	void	bcompute	__PR((Argvec *vp, FILE **std, int flag));
extern	void	btest		__PR((Argvec *vp, FILE **std, int flag));
extern	void	bexpr		__PR((Argvec *vp, FILE **std, int flag));
extern	BOOL	test		__PR((Argvec *vp, FILE **std));
extern	BOOL	is_dir		__PR((char *name));

/*
 * input.c
 */
extern	int	nextch		__PR((void));
extern	int	peekch		__PR((void));
extern	void	ungetch		__PR((int c));
extern	char	*nextline	__PR((void));
extern	FILE	*setinput	__PR((FILE *f));
extern	void	sclearerr	__PR((void));
extern	void	pushline	__PR((char *s));
extern	void	quote		__PR((void));
extern	void	unquote		__PR((void));
extern	int	quoting		__PR((void));
extern	void	dquote		__PR((void));
extern	void	undquote	__PR((void));
extern	int	dquoting	__PR((void));
extern	int	begina		__PR((int flg));
extern	int	getbegina	__PR((void));
extern	int	setbegina	__PR((void));

/*
 * limit.c
 */
extern	void	blimit		__PR((Argvec *vp, FILE **std, int flag));
extern	void	prtime		__PR((FILE **std, long sec, long usec));
extern	void	btime		__PR((Argvec *vp, FILE **std, int flag));
extern	void	inittime	__PR((void));
extern	void	setstime	__PR((void));
#ifdef	RUSAGE_SELF
extern	void	getpruself	__PR((struct rusage *prusage));
extern	void	getpruchld	__PR((struct rusage *prusage));
extern	void	prtimes		__PR((FILE **std,
					struct rusage *prusage));
extern	void	rusagesub	__PR((struct rusage *pru1,
					struct rusage *pru2));
extern	void	rusageadd	__PR((struct rusage *pru1,
					struct rusage *pru2));
#endif

/*
 * expand.c
 */
extern	BOOL	any_match	__PR((char *s));
extern	Tnode	*expand		__PR((char *s));

/*
 * inputc.c
 */
extern	FILE	*getinfile	__PR((void));
extern	int	get_histlen	__PR((void));
extern	void	chghistory	__PR((char *cp));
extern	void	histrange	__PR((unsigned *firstp,
					unsigned *lastp,
					unsigned *nextp));
extern	void	init_input	__PR((void));
extern	int	getnextc	__PR((void));
extern	int	nextc		__PR((void));
extern	void	space		__PR((int n));
extern	void	append_line	__PR((char *linep, unsigned int len,
							unsigned int pos));
extern	char	*match_hist	__PR((char *pattern));
extern	char	*make_line	__PR((int (*f)(FILE *), FILE *arg));
extern	char	*get_line	__PR((int n, FILE *f));

/*
 * Keep #defines in sync with include/schily/shedit.h
 */
#define	HI_NOINTR	0	/* History traversal noninterruptable	*/
#define	HI_INTR		1	/* History traversal is interruptable	*/
#define	HI_NONUM	2	/* Do not print numbers			*/
#define	HI_TAB		4	/* Print TABs				*/
#define	HI_REVERSE	8	/* Print in reverse order		*/
#define	HI_PRETTYP	16	/* Pretty Type non-printable chars	*/
#define	HI_ANSI_NL	32	/* Convert ASCII newlines to ANSI nl	*/
extern	int	put_history	__PR((FILE *f, int flg,
					int _first, int _last, char *_subst));
extern	int	search_history	__PR((int flg,
					int _first, char *_pat));
extern	int	remove_history	__PR((int flg,
					int _first, char *_pat));
extern	void	save_history	__PR((int flg));
extern	void	read_init_history	__PR((void));
extern	void	readhistory	__PR((FILE *f));

/*
 * ttymodes.c
 */
extern	void	reset_line_disc	__PR((void));
extern	void	reset_tty_pgrp	__PR((void));
extern	void	reset_tty_modes	__PR((void));
extern	void	set_append_modes	__PR((FILE *f));
extern	void	set_insert_modes	__PR((FILE *f));
extern	void	get_tty_modes	__PR((FILE *f));
extern	pid_t	tty_getpgrp	__PR((int f));
extern	int	tty_setpgrp	__PR((int f, pid_t pgrp));

/*
 * testmail.c
 */
extern	void	testmail	__PR((void));

/*
 * oldhistory.c
 */
#ifndef	INTERACTIVE
extern	int	high_hist	__PR((void));
extern	BOOL	treeequal	__PR((Tnode *cmd, Tnode *ocmd));
extern	void	h_append	__PR((Tnode *cmd));
extern	void	lr_used		__PR((int nr));
extern	void	printtree	__PR((FILE *f, Tnode *cmd));
extern	void	phistory	__PR((BOOL));
extern	void	hi_list		__PR((FILE *));
extern	BOOL	sethistory	__PR((char *));
extern	void	inithistory	__PR((void));
#endif

/*
 * evops.c
 */
extern	char	*getcurenv	__PR((char *name));
extern	void	ev_init		__PR((char **evp));
extern	void	ev_insert	__PR((char *val));
extern	void	ev_ins		__PR((char *val));
extern	void	ev_delete	__PR((char *name));
extern	void	ev_inc		__PR((void));
extern	void	ev_list		__PR((FILE *fp));
extern	BOOL	ev_eql		__PR((char *name, char *tval));
extern	void	inituser	__PR((void));
extern	void	inithostname	__PR((void));
extern	void	initprompt	__PR((void));
extern	BOOL	ev_set_locked	__PR((char *val));

/*
 * alloc.c
 */
extern	void	*Jfree		__PR((void *t, void *chain));
#ifdef	D_MALLOC
extern	void	*dbg_malloc		__PR((size_t size,
							char *file, int line));
extern	void	*dbg_calloc		__PR((size_t nelem, size_t elsize,
							char *file, int line));
extern	void	*dbg_realloc		__PR((void *t, size_t size,
							char *file, int line));
#define	malloc(s)			dbg_malloc(s, __FILE__, __LINE__)
#define	calloc(n, s)			dbg_calloc(n, s, __FILE__, __LINE__)
#define	realloc(t, s)			dbg_realloc(t, s, __FILE__, __LINE__)
#endif
extern	int	psize		__PR((char *t));
extern	void	freechecking	__PR((BOOL val));
extern	void	nomemraising	__PR((BOOL val));
extern	void	aprintfree	__PR((FILE *f));
extern	void	aprintlist	__PR((FILE *f, long l));
extern	void	aprintchunk	__PR((FILE *f, long l));

extern	void	balloc		__PR((Argvec *vp, FILE **std, int flag));

extern	void	*get_heapbeg	__PR((void));
extern	void	*get_heapend	__PR((void));

/*
 * wait3.c
 */
#ifndef	_LFS64_ASYNCHRONOUS_IO		/* Hack for Solaris >= 2.6 */
#ifndef	HAVE_WAIT3
#ifdef	HAVE_WAITID
#ifdef	RUSAGE_SELF
#ifdef	_SCHILY_WAIT_H			/* Needed for WAIT_T */

/*
 * XXX if this no longer compiles on UnixWare, change back pid_t to int
 * XXX here and in wait3.c
 */
extern	pid_t	wait3	__PR((WAIT_T *status, int options,
						struct rusage *rusage));
#endif
#endif
#endif
#endif
#endif

/*
 * pfexec.c
 */
extern	void	pfinit		__PR((void));
extern	void	pfend		__PR((void));
extern	int	pfexec		__PR((char **path, char *name,
						FILE *in, FILE *out, FILE *err,
						char **av, char **env));
