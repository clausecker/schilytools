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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2008 J. Schilling
 *
 * @(#)defines.h	1.13 08/09/04 J. Schilling
 */
#if defined(sun) || defined(__GNUC__)

#ident "@(#)defines.h 1.13 08/09/04 J. Schilling"
#endif
/*
 * @(#)defines.h 1.21 06/12/12
 */

#ident	"@(#)defines.h"
#ident	"@(#)sccs:hdr/defines.h"
# include	<schily/mconfig.h>
# include	<schily/types.h>
# include	<schily/utypes.h>
# include	<schily/param.h>
# include	<schily/stat.h>
# include	<schily/errno.h>
# include	<schily/fcntl.h>
# include	<stdio.h>
# include	<schily/stdlib.h>
# include	<schily/unistd.h>
# include	<schily/string.h>
# include	<schily/standard.h>	/* define signed */
# include	<schily/nlsdefs.h>
# include	<macros.h>
# undef		abs
# include	<fatal.h>
# include	<schily/time.h>

#ifdef	HAVE_VAR_TIMEZONE
extern long timezone;
#else
#define	timezone	xtimezone
long	timezone;
#endif
#define	tzset		xtzset

extern int optind, opterr, optopt;
extern char *optarg;

# include	<schily/maxpath.h>
#ifndef PATH_MAX
#ifdef	FILENAME_MAX
#define PATH_MAX	FILENAME_MAX
#endif
#endif
#ifndef	PATH_MAX
#ifdef	MAXPATHNAME
#define	PATH_MAX	MAXPATHNAME
#endif
#endif
#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif

#ifndef	MAXPATHLEN
#define	MAXPATHLEN	PATH_MAX
#endif

#if	defined(NEED_SCHILY) || defined(NEED_SCHILY_PRINT)
#ifdef	NEED_SCHILY_PRINT
#define	SCHILY_PRINT
#endif
# define error	__js_error__		/* SCCS error differs from schily.h */
# include	<schily/schily.h>
# undef	error
#endif

/*
 * SCCS was written in 1972. It supports 2 digit year strings from 1969..2068.
 * Since the Y2000 fix from 1999, it supports to read 4 digit year strings.
 * We start to create 4 digit year strings in Y2038 when 32 bit SCCS
 * implementations will stop working.
 */
#define	Y2038		0x7FFFFFFE
#ifdef	FOUR_DIGIT_YEAR_TEST
#define	Y2038		0x47800000	/* Jan 6th 2008 for tests */
#endif

#define ALIGNMENT  	(sizeof(long long))
#define ROUND(x,base)   (((x) + (base-1)) & ~(base-1))

# define CTLSTR		"%c%c\n"

# define CTLCHAR	1	/* ^A control character sccs control prelude */
# define HEAD		'h'	/* ^Ah sccs magic number and checksum line   */

# define STATS		's'	/* ^A sccs stats inserted/deleted/unchanged  */

# define BDELTAB	'd'	/* ^Ad sccs delta type line		    */
# define INCLUDE	'i'	/* ^Ai list if include serial numbers	    */
# define EXCLUDE	'x'	/* ^Ax list of exclude serial numbers	    */
# define IGNORE		'g'	/* ^Ag list of ignore serial numbers	    */
# define MRNUM		'm'	/* ^Am list of mr-numbers		    */
# define COMMENTS	'c'	/* ^Ac a sccs comment line		    */
# define EDELTAB	'e'	/* ^Ae the end of a delta table		    */

# define BUSERNAM	'u'	/* ^Au begin list of allowed delta users    */
# define EUSERNAM	'U'	/* ^AU end list of allowed delta users      */

# define NFLAGS	28

#if	defined(IS_MACOS_X)
/*
 * Quick and dirty hack to work around a bug in the Mac OS X linker
 */
char	*Sflags[NFLAGS];	/* sync with lib/comobj/src/permiss.c */
char	saveid[50];		/* sync with lib/comobj/src/logname.c */
#endif

# define FLAG		'f'	/* ^Af	the begin of a flag line	    */
# define NULLFLAG	'n'	/* ^Af n create null deltas for skipped rel */
# define JOINTFLAG	'j'	/* ^Af j allow multiple concurrent updates  */
# define DEFTFLAG	'd'	/* ^Af d def-SID to use for get		    */
# define TYPEFLAG	't'	/* ^Af t mod-type used for %Y % keyword	    */
# define VALFLAG	'v'	/* ^Af v val-prog used or mr-flags	    */
# define CMFFLAG	'z'	/* ^Af z CMFFLAG ????			    */
# define BRCHFLAG	'b'	/* ^Af b enables branch deltas		    */
# define IDFLAG		'i'	/* ^Af i "No id keywords (ge6)" is an error */
# define MODFLAG	'm'	/* ^Af m mod-name used for %M % keyword	    */
# define FLORFLAG	'f'	/* ^Af f floor-rel allowed to check out	    */
# define CEILFLAG	'c'	/* ^Af c ceiling-rel allowed to check out   */
# define QSECTFLAG	'q'	/* ^Af m mod-name used for %Q % keyword	    */
# define LOCKFLAG	'l'	/* ^Af l ll relase list locked for get -e   */
# define ENCODEFLAG	'e'	/* ^Af e The content is UUencoded	    */
# define SCANFLAG	's'	/* ^Af s # of lines to be scanned f. keyw.  */
# define EXTENSFLAG	'x'	/* ^Af x enables sccs e'x'tensions	    */
# define EXPANDFLAG	'y'	/* ^Af y list of sccs keywords to be exp.   */

# define BUSERTXT	't'	/* ^At sccs file specific comment start	    */
# define EUSERTXT	'T'	/* ^AT sccs file specific comment end	    */

# define INS		'I'	/* ^AI release Insert block start	    */
# define DEL		'D'	/* ^AD release Delete block start	    */
# define END		'E'	/* ^AE release Insert/Delete block end	    */

# define MINR		1		/* minimum release number */
# define MAXR		9999		/* maximum release number */
# define FILESIZE	MAXPATHLEN
# define MAXLINE	BUFSIZ
# define DEF_LINE_SIZE	128
# undef	MAX
# define MAX		9999
# define DELIVER	'*'
# define LOGSIZE	(33)		/* TWCP SCCS compatibility */
# define MAXERRORLEN	(1024+MAXPATHLEN)	/* max length of SccsError buffer */

# define FAILPUT    fatal("fputs could not write to file (ut13)")
# define SCCS_LOCK_ATTEMPTS	4       /* maximum number of lock   attempts  */
# define SCCS_CREAT_ATTEMPTS	4       /* maximum number of create attempts  */

/*
	SCCS Internal Structures.
*/

struct apply {
	char	a_inline;	/* in the line of normally applied deltas */
	int	a_code;		/* APPLY, NOAPPLY or SX_EMPTY */
	int	a_reason;
};

#define SX_EMPTY	(0)
#define APPLY		(1)
#define NOAPPLY		(2)

# define IGNR		0100
# define USER		040
# define INCL		1
# define EXCL		2
# define CUTOFF		4
# define INCLUSER	(USER | INCL)
# define EXCLUSER	(USER | EXCL)
# define IGNRUSER	(USER | IGNR)

struct queue {
	struct queue *q_next;
	int	q_sernum;	/* serial number */
	char    q_keep;		/* keep switch setting */
	char	q_iord;		/* INS or DEL */
	char	q_ixmsg;	/* caused inex msg */
	char	q_user;		/* inex'ed by user */
};

#define YES	(1)
#define NO	(2)

struct	sid {
	int	s_rel;
	int	s_lev;
	int	s_br;
	int	s_seq;
};

struct	deltab {
	struct	sid	d_sid;
	int	d_serial;
	int	d_pred;
	time_t	d_datetime;
	char	d_pgmr[LOGSIZE];
	char	d_type;
};

struct	ixg {
	struct	ixg	*i_next;
	char	i_type;
	char	i_cnt;
	int	i_ser[1];
};

struct	idel {
	struct	sid	i_sid;
	struct	ixg	*i_ixg;
	int	i_pred;
	time_t	i_datetime;
};

# define maxser(pkt)	((pkt)->p_idel->i_pred)
# define sccsfile(f)	imatch("s.", sname(f))

struct packet {
	char	p_file[FILESIZE]; /* file name containing module */
	struct	sid p_reqsid;	/* requested SID, then new SID */
	struct	sid p_gotsid;	/* gotten SID */
	struct	sid p_inssid;	/* SID which inserted current line */
	char	p_verbose;	/* verbose flags (see #define's below) */
	char	p_upd;		/* update flag (!0 = update mode) */
	time_t	p_cutoff;	/* specified cutoff date-time */
	int	p_ihash;	/* initial (input) hash */
	int	p_chash;	/* current (input) hash */
	int	p_uchash;	/* current unsigned (input) hash */
	int	p_nhash;	/* new (output) hash */
	int	p_glnno;	/* line number of current gfile line */
	int	p_slnno;	/* line number of current input line */
	char	p_wrttn;	/* written flag (!0 = written) */
	char	p_keep;		/* keep switch for readmod() */
	struct	apply *p_apply;	/* ptr to apply array */
	struct	queue *p_q;	/* ptr to control queue */
	FILE	*p_iop;		/* input file */
	char	p_buf[BUFSIZ];	/* input file buffer */
	char	*p_line;	/* buffer for getline() */
	size_t	p_line_size;	/* size of the buffer for getline() */
	time_t	p_cdt;		/* date/time of newest applied delta */
	char	*p_lfile;	/* 0 = no l-file; else ptr to l arg */
	struct	idel *p_idel;	/* ptr to internal delta table */
	FILE	*p_stdout;	/* standard output for warnings and messages */
	FILE	*p_gout;	/* g-file output file */
	char	p_user;		/* !0 = user on user list */
	char	p_chkeof;	/* 0 = eof generates error */
	int	p_maxr;		/* largest release number */
	int	p_ixmsg;	/* inex msg counter */
	int	p_reopen;	/* reopen flag used by getline on eof */
	int	p_ixuser;	/* HADI | HADX (in get) */
	int	do_chksum;	/* for getline(), 1 = do check sum */
};

struct	stats {
	int	s_ins;
	int	s_del;
	int	s_unc;
};

struct	pfile	{
	struct	sid	pf_gsid;
	struct	sid	pf_nsid;
	char	pf_user[LOGSIZE];
	time_t	pf_date;
	char	*pf_ilist;
	char	*pf_elist;
	char 	*pf_cmrlist;
};

/*
 Declares for external functions in lib/cassi
*/
extern	char*	gf	__PR((char *));
extern	int	sweep	__PR((int, char *, char *, int, int, int, char **, char *, char **, int (*)(char *, int, char **), int (*)(char **, char **, int)));
extern	int	cmrcheck __PR((char *, char *));
extern	int	deltack __PR((char [], char *, char *, char *));
extern	void	error	__PR((const char *));

/*
 Declares for external functions in lib/comobj
*/

extern	char*	auxf	__PR((char *, int));
extern	void	sinit	__PR((struct packet *, char *, int));
extern	void	setup	__PR((struct packet *, int));
extern	void	finduser __PR((struct packet *));
extern	void	permiss	__PR((struct packet *));
extern	char*	sid_ab	__PR((char *, struct sid *));
extern	char*	sid_ba	__PR((struct sid *, char *));
extern	char*	omit_sid __PR((char *));
extern	int	date_ab	__PR((char *, time_t *));
extern	char*	date_ba	__PR((time_t *, char *));
extern	char*	date_bal __PR((time_t *, char *));
extern	char	del_ab	__PR((char *, struct deltab *, struct packet *));
extern	char*	del_ba	__PR((struct deltab *, char *));
extern	void	stats_ab __PR((struct packet *, struct stats *));
extern	void	pf_ab	__PR((char *, struct pfile *, int));
extern	int	getser	__PR((struct packet *));
extern	int	sidtoser __PR((struct sid *, struct packet *));
extern	int	eqsid	__PR((struct sid *, struct sid *));
extern	void	chksid	__PR((char *, struct sid *));
extern	void	newsid	__PR((struct packet *, int));
extern	void	newstats __PR((struct packet *, char *, char *)); 
extern	void	condset	__PR((struct apply *, int, int));
extern	void	dolist	__PR((struct packet *, char *, int));
extern	void	dohist	__PR((char *));
extern	void	doie	__PR((struct packet *, char *, char *, char *));
extern	void	doflags	__PR((struct packet *));
extern	struct idel *dodelt __PR((struct packet *, struct stats *, struct sid *, int));
extern	void	do_file __PR((char *, void (*func)(char *), int));
extern	void	fmterr	__PR((struct packet *));
/*
 * Deal with unfriendly and non POSIX compliant glibc that defines getline()
 */
#undef	getline
#define	getline	comgetline
extern	char	*getline __PR((struct packet *));
extern	void	putline	__PR((struct packet *, char *));
extern	char*	logname	__PR((void));
extern	int	mystrptime __PR((char *, struct tm *, int));
extern	char*	savecmt	__PR((char *));
extern	void	mrfixup	__PR((void));
extern	void	xrm	__PR((void));
extern	void	flushto	__PR((struct packet *, int, int));
extern	void	flushline __PR((struct packet *, struct stats *));
extern	int 	chkid	__PR((char *, char *));
extern	int	valmrs	__PR((struct packet *, char *));
extern	void	encode	__PR((FILE *, FILE *));
extern	void	decode	__PR((char *, FILE *));
extern	int	readmod	__PR((struct packet *));
extern	int	parse_date __PR((char *, time_t *));
extern	int	cmpdate	__PR((struct tm *, struct tm *));
extern	void	addq	__PR((struct packet *, int, int, int, int));
extern	void	remq	__PR((struct packet *, int));
extern	void	setkeep	__PR((struct packet *));
extern	void	get_Del_Date_time __PR((char *, struct deltab *, struct packet *, struct tm *));
extern	char*	stalloc	__PR((unsigned int));
extern	int	mosize	__PR((int y, int t));
extern	int	gN	__PR((char *str, char **next, int num, int *digits, int *chars));
extern	void	xtzset	__PR((void));

/*
 Declares for external functions in lib/mpwlib
*/

extern	int	any	__PR((int, char *));
extern	char	*abspath __PR((char *));
extern	char	*sname	__PR((char *));
extern	char	*cat	__PR((char *dest, ...));
extern	char	*dname	__PR((char *));
extern	char	*satoi	__PR((char *, int *));
extern	int	patoi	__PR((char *));
extern	char	*repl	__PR((char *, char, char));
extern	char	*strend	__PR((char *));
extern	char	*trnslat __PR((char *, char *, char *, char *));
extern	char	*zero	__PR((char *, int));
extern	void	*fmalloc __PR((unsigned));
extern	void	ffree	__PR((void *));
extern	void	ffreeall __PR((void));
extern	int	fatal	__PR((char *));
extern	int	lockit	__PR((char *, int, pid_t, char *));
extern	int	unlockit __PR((char *, pid_t, char *));
extern	int	mylock	__PR((char *, pid_t, char *));
extern	int	sccs_index __PR((char *, char *));
extern	int	imatch	__PR((char *, char *));
extern	int	xmsg	__PR((const char *, const char *));
extern	FILE*	fdfopen	__PR((int, int));
extern	int	xcreat	__PR((char *, mode_t));
extern	int	xopen	__PR((char [], int));
extern	int	xlink	__PR((const char *, const char *));
extern	int	xunlink	__PR((const char *));
extern	int	xpipe	__PR((int *));
extern	void	setsig	__PR((void));
extern	int	check_permission_SccsDir __PR((char *));
extern  char*	get_Sccs_Comments __PR((void));
extern	int	userexit __PR((int code));

/*
 * Exported from vaious programs in /usr/ccs/bin
 */
extern	void    clean_up __PR((void));
extern	void	escdodelt __PR((struct packet *pkt));
extern	void	fredck	__PR((struct packet *pkt));
extern	void	enter	__PR((struct packet *pkt, int ch, int n, struct sid *sidp));

# define RESPSIZE	1024
# define NVARGS		64
# define VSTART		3

/*
**	The following five definitions (copy, xfopen, xfcreat, remove,
**	USXALLOC) are taken from macros.h 1.1
*/

# define copy(srce,dest)	cat(dest, srce, (char *)0)
# define xfopen(file,mode)	fdfopen(xopen(file,mode),mode)
# define xfcreat(file,mode)	fdfopen(xcreat(file,mode), O_WRONLY|O_BINARY)
# define remove(file)		xunlink(file)
# define USXALLOC() \
		char *alloc(n) {return((char *)xalloc((unsigned)n));} \
		free(n) char *n; {xfree(n);} \
		char *malloc(n) unsigned n; {int p; p=xalloc(n); \
			return((char *)(p != -1?p:0));}

