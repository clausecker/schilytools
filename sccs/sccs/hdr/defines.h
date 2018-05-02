/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2018 J. Schilling
 *
 * @(#)defines.h	1.98 18/04/30 J. Schilling
 */
#ifndef	_HDR_DEFINES_H
#define	_HDR_DEFINES_H
#if defined(sun)
#pragma ident "@(#)defines.h 1.98 18/04/30 J. Schilling"
#endif
/*
 * @(#)defines.h 1.21 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)defines.h"
#pragma ident	"@(#)sccs:hdr/defines.h"
#endif
# include	<schily/mconfig.h>
# include	<schily/types.h>
# include	<schily/utypes.h>
# include	<schily/param.h>
# include	<schily/stat.h>
# include	<schily/errno.h>
# include	<schily/fcntl.h>
# include	<schily/stdio.h>
# include	<schily/stdlib.h>
# include	<schily/varargs.h>	/* needed for vfprintf() prototype */
# include	<schily/unistd.h>
# include	<schily/string.h>
# include	<schily/standard.h>	/* define signed */
# include	<schily/nlsdefs.h>
# include	<schily/io.h>		/* for setmode() prototype */
# include	<macros.h>
# undef		abs
# include	<fatal.h>
# include	<schily/time.h>

#ifdef	HAVE_LONG_LONG
typedef	unsigned long long urand_t;
#else
typedef struct urand {
	unsigned int	high;
	unsigned int	low;
} urand_t;
#endif

#ifdef	HAVE_VAR_TIMEZONE
#ifndef	HAVE_VAR_TIMEZONE_DEF		/* IRIX has extern time_t timezone */
extern long timezone;
#endif
#else
#define	timezone	xtimezone
long	timezone;
#endif
#define	tzset		xtzset

/*
 * Note that the #define above will rename the struct timeb member "timezone"
 * to "xtimezone" as well as the name of the variable.
 */
# include	<schily/timeb.h>

extern int optind, opterr, optopt;
extern char *optarg;

# include	<schily/maxpath.h>
#ifndef PATH_MAX
#ifdef	FILENAME_MAX
#define	PATH_MAX	FILENAME_MAX
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

#ifdef	NO_SCCS_FATALHELP
#undef	SCCS_FATALHELP
#endif

#if	(!defined(HAVE_SNPRINTF) && !defined(NO_SNPRINTF)) || \
	defined(NEED_PRINTF_J) && !defined(HAVE_PRINTF_J)
#define	NEED_SCHILY_PRINT	/* We need defines for js_snprintf() */
#endif

#if	(!defined(HAVE_SNPRINTF) && !defined(NO_SNPRINTF)) || \
	defined(NEED_PRINTF_Z) && !defined(HAVE_PRINTF_Z)
#define	NEED_SCHILY_PRINT	/* We need defines for js_snprintf() */
#endif

/*
 * Always include schily/schily.h to allow POSIX bug workarounds to be
 * implemented in schily/schily.h.
 */
#ifdef	NEED_SCHILY_PRINT
#define	SCHILY_PRINT
#endif
#include	<schily/schily.h>

#if SIZEOF_TIME_T == 4

#define	mktime		xmktime
#define	localtime	xlocaltime
#define	gmtime		xgmtime

extern time_t	xmktime		__PR((struct tm *));
extern struct tm *xlocaltime	__PR((time_t *));
extern struct tm *xgmtime	__PR((time_t *));
#endif

/*
 * SCCS was written in 1972. It supports 2 digit year strings from 1969..2068.
 * Since the Y2000 fix from 1999, it supports to read 4 digit year strings.
 * We start to create 4 digit year strings in Y2038 when 32 bit SCCS
 * implementations will stop working.
 */
#define	_YM9999		0x3AFFF4417FL	/* Dec 31 9999 23:59:59 GMT */
#define	_Y2069		0xBA37E000	/* Jan 1  2069 00:00:00 GMT */
#define	_YM2038		0x7FFFFFFE	/* Jan 19 2038 03:14:07 GMT */
#define	_Y2038		0x7FE81780	/* Jan 1  2038 00:00:00 GMT */
#ifdef	FOUR_DIGIT_YEAR_TEST
#define	_Y2038		0x47800000	/* Jan 6th 2008 for tests */
#endif
#define	_Y1969		(-31536000)	/* Jan 1  1969 00:00:00 GMT */

extern time_t	Y2069;			/* defined in lib/comobj/src/xtzset.c */
extern time_t	Y2038;			/* defined in lib/comobj/src/xtzset.c */
extern time_t	Y1969;			/* defined in lib/comobj/src/xtzset.c */

#if SIZEOF_TIME_T == 4			/* a 32 bit program: */
#define	MAX_TIME	(time_t)_YM2038	/* max positive long */
#else					/* a 64 bit or larger program: */
#define	MAX_TIME	(time_t)_YM9999	/* We currently support 4 digit years */
#endif

/*
 * The epoche start for urandom() time -> rand computation:
 */
#define	URAND_BASE	0x50000000	/* 2012 Jul 13 11:01:20 UTC */

#define	ALIGNMENT	(sizeof (long long))
#define	ROUND(x, base)	(((x) + (base-1)) & ~(base-1))

# define CTLSTR		"%c%c\n"

# define CTLCHAR	1	/* ^A control character sccs control prelude */
# define HEAD		'h'	/* ^Ah sccs magic number and checksum line   */

# define STATS		's'	/* ^A sccs stats inserted/deleted/unchanged  */

# define BDELTAB	'd'	/* ^Ad sccs delta type line		    */
# define INCLUDE	'i'	/* ^Ai list if include serial numbers	    */
# define EXCLUDE	'x'	/* ^Ax list of exclude serial numbers	    */
# define IGNORE		'g'	/* ^Ag list of ignore serial numbers	    */
# define MRNUM		'm'	/* ^Am list of mr-numbers		    */
# define SIDEXTENS	'S'	/* ^AS SID specific extensions V6	    */
# define COMMENTS	'c'	/* ^Ac a sccs comment line		    */
# define EDELTAB	'e'	/* ^Ae the end of a delta table		    */

# define BUSERNAM	'u'	/* ^Au begin list of allowed delta users    */
# define EUSERNAM	'U'	/* ^AU end list of allowed delta users	    */

#define	NFLAGS	28
#define	NLOWER		('z'-'a'+1)
#define	NUPPER		('Z'-'A'+1)
#define	NALPHA		(NLOWER + NUPPER)
#define	LOWER(c)	((c) >= 'a' && (c) <= 'z')
#define	UPPER(c)	((c) >= 'A' && (c) <= 'Z')
#define	ALPHA(c)	(LOWER(c) || UPPER(c))
#define	fdx(c)	((c)-'a')	/* Flag array index (e.g. for p_sflags)	    */

/*
 * Definition kept in lib to work around a bug in the Mac OS X linker
 */
extern	char	saveid[50];	/* defined in lib/comobj/src/logname.c */

# define FLAG		'f'	/* ^Af	the begin of a flag line	    */
# define NULLFLAG	'n'	/* ^Af n create null deltas for skipped rel */
# define JOINTFLAG	'j'	/* ^Af j allow multiple concurrent updates  */
# define DEFTFLAG	'd'	/* ^Af d def-SID to use for get		    */
# define TYPEFLAG	't'	/* ^Af t mod-type used for %Y % keyword	    */
# define VALFLAG	'v'	/* ^Af v val-prog used or mr-flags	    */
# define CMFFLAG	'z'	/* ^Af z CMFFLAG ????			    */
# define BRCHFLAG	'b'	/* ^Af b enables branch deltas		    */
# define IDFLAG		'i'	/* ^Af i "No id keywords (cm7)" is an error */
# define MODFLAG	'm'	/* ^Af m mod-name used for %M % keyword	    */
# define FLORFLAG	'f'	/* ^Af f floor-rel allowed to check out	    */
# define CEILFLAG	'c'	/* ^Af c ceiling-rel allowed to check out   */
# define QSECTFLAG	'q'	/* ^Af m mod-name used for %Q % keyword	    */
# define LOCKFLAG	'l'	/* ^Af l ll relase list locked for get -e   */
# define ENCODEFLAG	'e'	/* ^Af e The content is UUencoded	    */
# define SCANFLAG	's'	/* ^Af s # of lines to be scanned f. keyw.  */
# define EXTENSFLAG	'x'	/* ^Af x enables sccs e'x'tensions	    */
# define EXPANDFLAG	'y'	/* ^Af y list of sccs keywords to be exp.   */

# define NAMEDFLAG	'F'	/* ^AF	the begin of a named flag line V6   */
# define GLOBALEXTENS	'G'	/* ^AG	the begin of a global ext. line V6  */

# define BUSERTXT	't'	/* ^At sccs file specific comment start	    */
# define EUSERTXT	'T'	/* ^AT sccs file specific comment end	    */

# define INS		'I'	/* ^AI release Insert block start	    */
# define DEL		'D'	/* ^AD release Delete block start	    */
# define END		'E'	/* ^AE release Insert/Delete block end	    */

# define NONL		'N'	/* ^AN escaped text line with no newline V6 */

# define MINR		1		/* minimum release number */
# define MAXR		9999		/* maximum release number */
# define MAXL		9999		/* max level for dolist() -> maxint? */
#ifndef	MINFBFSZ
# define MINFBFSZ	2048	/* Mininum File Buffer Size */
#endif
# define FILESIZE	max(MAXPATHLEN, MINFBFSZ) /* Space for path names */
#ifndef	MINLINESZ
# define MINLINESZ	8192	/* Minimum Line Buffer Size */
#endif
# define MAXLINE	max(BUFSIZ, MINLINESZ)	/* Buffer size for various */
# define DEF_LINE_SIZE	128		/* start size for alocated lines */
# define DELIVER	'*'		/* MR number was "delivered" no rmdel */
# define LOGSIZE	(33)		/* TWCP SCCS compatibility */
# define MAXERRORLEN	(1024+FILESIZE)	/* max length of SccsError buffer */

# define FAILPUT    fatal("fputs could not write to file (ut13)")
# define SCCS_LOCK_ATTEMPTS	4	/* maximum number of lock   attempts  */
# define SCCS_CREAT_ATTEMPTS	4	/* maximum number of create attempts  */

/*
 * The third argument for the function flushto() controls whether the data
 * is only read or whether is is copied. FLUSH_COPY copies all lines including
 * the matching final line. FLUSH_COPY_UNTIL does not copy the matching final
 * line.
 */
# define FLUSH_COPY		0	/* Copy all read lines until match */
# define FLUSH_NOCOPY		1	/* Skip all read lines until match */
# define FLUSH_COPY_UNTIL	2	/* Copy read lines until before match */

/*
 * The third argument for the function sinit() is a flag that controls the
 * behavior. Unless SI_FORCE is used, the filename is checked to be either
 * "s.name" or "xxx/s.name". SI_FORCE allows us to open the "x.file" for
 * reading.
 */
# define SI_INIT		0	/* Check filename and init pkt	    */
# define SI_OPEN		1	/* Check filename init pkt open file */
# define SI_FORCE		2	/* Init pkt & open any file name    */

# define SI_FORCEOPEN		(SI_OPEN|SI_FORCE)


/*
 *	SCCS Internal Structures.
 */

/*
 * Definitions for date+time
 */
typedef struct dtime {
	time_t	dt_sec;		/* Seconds since Jan 1 1970 GMT		*/
	int	dt_nsec;	/* Nanoseconds (must be positive)	*/
	int	dt_zone;	/* Timezone (seconds east to GMT)	*/
} dtime_t;

#define	DT_NO_ZONE	1	/* Impossible timezone - no zone found	*/
#define	DT_MIN_ZONE	(-89940) /* Minimum zone (-24:59)		*/
#define	DT_MAX_ZONE	93540	/* Minimum zone (+25:59)		*/

/*
 * String space needed for various date formats:
 *
 *	DT_STRSIZE	Classical SCCS date format
 *	DT_LSTRSIZE	Long date format with 4-digit year
 *	DT_ZSTRSIZE	Long date format with nanoseconds and time zone
 */
#define	DT_STRSIZE	18	/* "11/08/18 10:54:30"			*/
#define	DT_LSTRSIZE	20	/* "2011/08/18 10:54:30"		*/
#define	DT_ZSTRSIZE	35	/* "2011/08/18 10:54:30.123456789+0200"	*/

/*
 * An array of struct apply[maxser+1] is allocated by dodelt() and assigned to
 * pkt->p_apply
 */
struct apply {
	char	a_inline;	/* in the line of normally applied deltas */
	char	a_code;		/* APPLY, NOAPPLY or SX_EMPTY */
	char	a_reason;
};

/*
 * Definitions for a_code
 */
#define	SX_EMPTY	(0)
#define	APPLY		(1)
#define	NOAPPLY		(2)

/*
 * Definitions for a_reason
 */
#define	IGNR		0100
#define	USER		040
#define	INCL		1
#define	EXCL		2
#define	CUTOFF		4
#define	INCLUSER	(USER | INCL)
#define	EXCLUSER	(USER | EXCL)
#define	IGNRUSER	(USER | IGNR)

struct queue {
	struct queue *q_next;
	int	q_sernum;	/* serial number */
	char    q_keep;		/* keep switch setting */
	char	q_iord;		/* INS or DEL */
	char	q_ixmsg;	/* caused inex msg */
	char	q_user;		/* inex'ed by user */
};

#define	YES	(1)
#define	NO	(2)

struct	sid {
	int	s_rel;		/* Release			*/
	int	s_lev;		/* Level			*/
	int	s_br;		/* Branch			*/
	int	s_seq;		/* Sequence (if branch != 0)	*/
};

/*
 * String space needed for the SID.
 *
 * Currently s_rel is limited to the range MINR .. MAXR and as long
 * as MAXR stays 1ith 9999, 38 chars would be enough. It is however
 * safer to permit the whole range for SID -> string conversion.
 */
#define	SID_STRSIZE	45	/* 4 * 10 + 3 * '.' + '\0' = 44 */

/*
 * Parameters for the -N option:
 */
typedef struct Nparms {
	char	n_comma;		/* Found -N,*			*/
	char	n_unlink;		/* Found -N-*			*/
	char	n_get;			/* Found -N+* or -N++*		*/
	char	n_sdot;			/* Found -N*s.			*/
	char	n_subd;			/* Found -NSCCS			*/
	char	*n_prefix;		/* "s." /  "* /s." / "* /SCCS"	*/
	char	*n_parm;		/* -N argument			*/
	char	*n_ifile;		/* The plain g-file name	*/
	char	*n_dir_name;		/* directory for -N		*/
	struct sid	n_sid;		/* SID if n_get == 2		*/
	struct timespec	n_mtime;	/* Timestamp for -i -o		*/
} Nparms;


struct	deltab {
	struct	sid	d_sid;
	int	d_serial;
	int	d_pred;
	dtime_t	d_dtime;
	char	d_pgmr[LOGSIZE];
	char	d_type;
};

struct	ixg {
	struct	ixg	*i_next;
	char	i_type;
	char	i_cnt;
	int	i_ser[1];
};

/*
 * An array of struct idel[maxser+1] is allocated by dodelt() and assigned to
 * pkt->p_idel
 */
struct	idel {
	struct	sid	i_sid;
	struct	ixg	*i_ixg;
	int	i_pred;
	struct timespec	i_datetime;
};

# define maxser(pkt)	((pkt)->p_idel->i_pred)
# define sccsfile(f)	imatch("s.", sname(f))

/*
 * SCCS used to use setbuf() with packet.p_buf as buffer. Since at least
 * Solaris uses parts of such a buffer for multi-byte putback features,
 * calling setbuf() is counter-productive as it results in unaligned reads
 * with BUFSIZE-8 as read(2) buffer size.
 * We try to avoid this problem by calling setvbuf(f, NULL, _IOFBF, size)
 * and telling stdio to allocate the buffer for us instead. Thus, if we have
 * setvbuf, we no longer need packet.p_buf.
 */
#ifdef	HAVE_SETVBUF
#define	VBUF_SIZE	(32*1024)
#define	USE_SETVBUF
#else
#define	VBUF_SIZE	BUFSIZ
#endif
#ifdef	pdp11
#undef	VBUF_SIZE
#define	VBUF_SIZE	512
#endif
#ifdef	VMS
#define	RECORD_IO
#endif
#if !defined(SCHILY_BUILD) && !defined(RECORD_IO)
#define	RECORD_IO
#endif

struct packet {
	char	p_file[FILESIZE]; /* file name containing module */
	struct	sid p_reqsid;	/* requested SID, then new SID */
	struct	sid p_gotsid;	/* gotten SID */
	struct	sid p_inssid;	/* SID which inserted current line */
	char	*p_sflags[NFLAGS]; /* SCCS s.file flags */
	char	p_verbose;	/* verbose flags (see #define's below) */
	char	p_upd;		/* update flag (!0 = update mode) */
	char	p_flags;	/* general flags see below */
	char	p_props;	/* file properties see below */
	time_t	p_cutoff;	/* specified cutoff date-time */
	int	p_ihash;	/* initial (input) hash */
	int	p_chash;	/* current (input) hash */
	int	p_uchash;	/* current unsigned (input) hash */
	int	p_clhash;	/* current (input) line hash */
	int	p_uclhash;	/* current unsigned (input) line hash */
	int	p_nhash;	/* new (output) hash */
	int	p_ghash;	/* current gfile hash */
	int	p_glines;	/* number of lines in current gfile */
	int	p_glnno;	/* line number of current gfile line */
	int	p_slnno;	/* line number of current input line */
	int	p_insser;	/* serial number which inserted current line */
	char	p_wrttn;	/* written flag (!0 = written) */
	char	p_keep;		/* keep switch for readmod() */
	char	p_encoding;	/* encoding flags for data */
	char	p_xcreate;	/* output file created, first line written */
	uint16_t *p_hash;	/* ptr to array of checksums */
	struct	apply *p_apply;	/* ptr to apply array */
	struct	queue *p_q;	/* ptr to control queue */
	FILE	*p_iop;		/* input file */
	FILE	*p_xiop;	/* output file */
#ifndef	HAVE_SETVBUF
	char	p_buf[BUFSIZ];	/* input file buffer */
#endif
	char	*p_linebase;	/* base addr of malloc()ed line */
	char	*p_lineptr;	/* begin of line past escape process */
	char	*p_line;	/* buffer for getline() */
	size_t	p_line_length;	/* actual line length for getline() */
	size_t	p_line_size;	/* size of the buffer for getline() */
	time_t	p_cdt;		/* date/time of newest applied delta */
	char	*p_lfile;	/* 0 = no l-file; else ptr to l arg */
	struct	idel *p_idel;	/* ptr to internal delta table */
	char	**p_pgmrs;	/* ptr to table of delta users */
	FILE	*p_stdout;	/* standard output for warnings and messages */
	FILE	*p_gout;	/* g-file output file */
	char	p_user;		/* !0 = user on user list */
	char	p_chkeof;	/* 0 = eof generates error */
	int	p_maxr;		/* largest release number */
	int	p_ixmsg;	/* inex msg counter */
	int	p_reopen;	/* reopen flag used by getline on eof */
	int	p_ixuser;	/* HADI | HADX (in get) */
	int	do_chksum;	/* for getline(), 1 = do check sum */
	/*
	 * Global meta data
	 */
	char	*p_init_path;	/* Initial path relative to project set home */
	urand_t	p_rand;		/* Unified random number for file */
	/*
	 * Various other s-file specific data
	 */
	void	(*p_enter) __PR((struct packet *pkt,
					int ch, int n,
					struct sid *sidp));
	void	(*p_escdodelt) __PR((struct packet *pkt));
	void	(*p_fredck) __PR((struct packet *pkt));
	char	p_cdid_mrs;	/* for cdc to check MRs */
	char	p_did_id;	/* whether a keyword was found */
	char	p_first_cmt;	/* first comment encountered */
	char	p_first_esc;	/* cdc, first time */
	char    p_pgmr[LOGSIZE]; /* for rmdel & chghist (rmchg) */
};

/*
 * General flags (p_flags)
 *
 * The PF_GMT flag is used to avoid calling mktime(3) in get(1) and delta(1)
 * as this is an expensive operation on some systems like SunOS-4.x and Linux.
 * If we set the flag, we need to carefully compensate the systematic error
 * introduced by this hack.
 */
#define	PF_GMT		1	/* Use GMT conversion			  */
#define	PF_V6		2	/* Support SCCS V6 features		  */
#define	PF_V6TAGS	4	/* SCCS V6 features found in hist file	  */
#define	PF_NONL		8	/* This line has no newline		  */
#define	PF_SCOX		0x10	/* SCO eXecutable flag found s.file	  */

/*
 * Encoding flags (p_encoding)
 *
 * The uuencoding takes place before the interleaved delta is created,
 * the gzip compression takes place on the interleaved delta block.
 */
#define	EF_TEXT		0	/* Pure text in interleaved delta	  */
#define	EF_UUENCODE	1	/* UUencoded binary in interleaved delta  */
#define	EF_GZIP		4	/* Gzipped interleaved delta		  */

/*
 * Flags to collect reasons for non-Text files also used in p_props
 */
#define	CK_NONL		1	/* No newline at end of file	*/
#define	CK_CTLCHAR	2	/* CTLCHAR ar beginning of line	*/
#define	CK_NULL		4	/* NUL character found in file	*/

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
 * Declares for external functions in lib/cassi
 */
extern	char	*gf	__PR((char *));
extern	int	sweep	__PR((int, char *, char *, int, int, int, char **,
				char *, char **,
				int (*)(char *, int, char **),
				int (*)(char **, char **,
				int)));
extern	int	cmrcheck __PR((char *, char *));
extern	int	deltack __PR((char [], char *, char *, char *, char *[]));
extern	void	cmrerror __PR((const char *));

/*
 * Declares for external variables in lib/comobj
 */
extern	char	*sccs_insbase;	/* Installation base dir or "/usr"	  */
extern	char	*Comments;	/* Comments from -y option		  */
extern	char	*Mrs;		/* List of Mr numbers from -m option	  */
extern	int	Domrs;		/* 'v' flag was found in the histroy file */

				/*
				 * If it is not possible to retrieve "setahome",
				 * it may be a NULL pointer. The variables
				 * "setrhome" and "cwdprefix" are always set
				 * from sethome().
				 */
extern	char	*setphome;	/* Best path to the project set home dir */
extern	char	*setrhome;	/* Relative path to the project set home dir */
extern	char	*setahome;	/* Absolute path to the project set home dir */
extern	char	*cwdprefix;	/* Prefix from project set home dir to cwd */
extern	int	homedist;	/* The # of dirs to the project set home dir */
extern	int	setrhomelen;	/* strlen(setrhome) */
extern	int	setahomelen;	/* strlen(setahome) */
extern	int	cwdprefixlen;	/* strlen(cwdprefix) */
extern	int	sethomestat;	/* sethome() status flags */

/*
 * sethome() status flags:
 */
#define	SETHOME_NONE	0	/* $SET_HOME/.sccs could not be found	*/
#define	SETHOME_OK	1	/* $SET_HOME/.sccs was found		*/
#define	SETHOME_ABS	2	/* Using setahome is preferred		*/
#define	SETHOME_INTREE	4	/* $SET_HOME/.sccs/data missing, SCCS in tree */
#define	SETHOME_OFFTREE	8	/* $SET_HOME/.sccs/data found, SCCS off tree */
#define	SETHOME_DELS_OK 16	/* $SET_HOME/.sccs/dels was found	*/

#define	SETHOME_ALL_OK	(SETHOME_OK|SETHOME_DELS_OK)

#define	SETHOME_INIT()	((sethomestat & SETHOME_ALL_OK) == SETHOME_ALL_OK)

/*
 * Declares for external functions in lib/comobj
 */

extern	char	*auxf	__PR((char *, int));
extern	void	sinit	__PR((struct packet *, char *, int));
extern	char	*checkmagic __PR((struct packet *, char *));
extern	void	setup	__PR((struct packet *, int));
extern	void	finduser __PR((struct packet *));
extern	void	permiss	__PR((struct packet *));
extern	char	*sid_ab	__PR((char *, struct sid *));
extern	char	*sid_ba	__PR((struct sid *, char *));
extern	void	sidext_ab __PR((struct packet *, struct deltab *, char *));
extern	void	sidext_v4compat_ab __PR((struct packet *, struct deltab *));
extern	void	sidext_ba __PR((struct packet *, struct deltab *));
extern	char	*omit_sid __PR((char *));
extern	int	date_ab	__PR((char *, time_t *, int flags));
extern	int	date_abz __PR((char *, dtime_t *, int flags));
extern	char	*date_ba __PR((time_t *, char *, int flags));
extern	char	*date_bal __PR((time_t *, char *, int flags));
extern	char	*date_bazl __PR((dtime_t *, char *, int flags));
extern	char	del_ab	__PR((char *, struct deltab *, struct packet *));
extern	char	*del_ba	__PR((struct deltab *, char *, int flags));
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
extern	void	dometa	__PR((struct packet *));
extern	struct idel *dodelt __PR((struct packet *,
				struct stats *, struct sid *, int));
extern	void	do_file __PR((char *, void (*func)(char *), int, int));
extern	void	fmterr	__PR((struct packet *));
/*
 * Deal with unfriendly and non POSIX compliant glibc that defines getline()
 */
#undef	getline
#define	getline	comgetline
extern	char	*getline __PR((struct packet *));
extern	void	putline	__PR((struct packet *, char *));
extern	void	putchr	__PR((struct packet *, int c));
extern	void	putctl	__PR((struct packet *));
extern	void	putctlnnl __PR((struct packet *));
extern	void	putmagic __PR((struct packet *, char *));
extern	void	putmeta	__PR((struct packet *));
extern	char	*logname	__PR((void));
extern	char	*sccs_user	__PR((char *));
extern	int	mystrptime __PR((char *, struct tm *, int));
extern	char	*savecmt	__PR((char *));
extern	void	mrfixup	__PR((void));
extern	void	xrm	__PR((struct packet *));
extern	void	flushto	__PR((struct packet *, int, int));
extern	void	flushline __PR((struct packet *, struct stats *));
extern	int 	chkid	__PR((char *, char *, char *[]));
extern	int	valmrs	__PR((struct packet *, char *));
extern	void	encode	__PR((FILE *, FILE *));
extern	void	decode	__PR((char *, FILE *));
extern	int	readmod	__PR((struct packet *));
extern	int	parse_date __PR((char *, time_t *, int flags));
extern	int	cmpdate	__PR((struct tm *, struct tm *));
extern	void	addq	__PR((struct packet *, int, int, int, int));
extern	void	remq	__PR((struct packet *, int));
extern	void	setkeep	__PR((struct packet *));
extern	void	get_Del_Date_time __PR((char *, struct deltab *,
					struct packet *, struct tm *));
extern	char	*stalloc	__PR((unsigned int));
extern	int	mosize	__PR((int y, int t));
extern	int	gN	__PR((char *str, char **next, int num,
					int *digits, int *chars));
extern	int	gNp	__PR((char *str, char **next, int num,
					int *digits, int *chars));
extern	int	gns	__PR((char *str, char **next));
extern	int	gtz	__PR((char *str, char **next));
extern	void	xtzset	__PR((void));
extern	void	dtime	__PR((dtime_t *));
extern	void	time2dt	__PR((dtime_t *, time_t, int));
extern	time_t	gmtoff	__PR((time_t));
extern	int		ssum __PR((char *, int));
extern	unsigned int	usum __PR((char *, int));
extern  void	set_init_path __PR((struct packet *, char *file, char *dir));
extern	void	sccs_setinsbase __PR((char *));
extern	int	sccsfatalhelp __PR((char *));
extern	int	sccshelp __PR((FILE *, char *));
extern	size_t	lhash_size	__PR((size_t size));
extern	void	lhash_destroy	__PR((void));
extern	char	*lhash_add	__PR((char *str));
extern	char	*lhash_lookup	__PR((char *str));
extern	void	sccs_xpg4 __PR((int val));
extern	void	whatsetup __PR((char *w));
extern	void	idsetup __PR((struct packet *pkt, char *gname));
extern	char *	idsubst __PR((struct packet *pkt, char *line));
extern	char *	getmodname __PR((void));
extern	char *	urand_ab __PR((char *aurand, urand_t *urandp));
extern	char *	urand_ba __PR((urand_t *urandp,
					char *aurand, size_t aurandsize));
extern	void	change_ba __PR((struct packet *pkt,
					char *cbuf, size_t cbuflen));
extern	char *	change_ab __PR((char *cbuf, struct packet *pkt));
extern	void	initN	__PR((Nparms *N));
extern	void	parseN	__PR((Nparms *N));
extern	char *	bulkprepare __PR((Nparms *N, char *afile));

/*
 * Declares for external variables in lib/mpwlib
 */
extern	char	SccsError[MAXERRORLEN];

/*
 * Declares for external functions in lib/mpwlib
 */

extern	int	any	__PR((int, char *));
#ifdef	HAVE_STRCHR
#define	any(c, s)	strchr(s, c)
#endif
extern	char	*fixpath __PR((char *));
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
extern	int	efatal	__PR((char *));
extern	int	fatal	__PR((char *));
extern	int	lockit	__PR((char *, int, pid_t, char *));
extern	int	unlockit __PR((char *, pid_t, char *));
extern	int	mylock	__PR((char *, pid_t, char *));
extern	int	sccs_index __PR((char *, char *));
extern	int	imatch	__PR((char *, char *));
extern	int	xmsg	__PR((const char *, const char *));
extern	FILE	*fdfopen	__PR((int, int));
extern	int	xcreat	__PR((char *, mode_t));
extern	int	xmkdir	__PR((char *, mode_t));
extern	int	xmkdirs	__PR((char *, mode_t));
extern	int	xopen	__PR((char [], int));
extern	int	xlink	__PR((const char *, const char *));
extern	int	xunlink	__PR((const char *));
extern	int	xpipe	__PR((int *));
extern	void	setsig	__PR((void));
extern	int	check_permission_SccsDir __PR((char *));
extern  char	*get_Sccs_Comments __PR((void));
extern	int	userexit __PR((int code));
extern	void	*zrealloc __PR((void *ptr, size_t amt));
extern	int	urandom	__PR((urand_t *urp));
extern	void	tv2urand __PR((struct timeval *tvp, urand_t *urp));
extern	void	urand2tv __PR((urand_t *urp, struct timeval *tvp));
extern	int	checkhome __PR((char *path));
extern	int	sethome	__PR((char *path));
extern	void	unsethome __PR((void));
extern	int	xsethome __PR((char *path));

#ifdef	DBG_MALLOC
extern	void	*dbg_fmalloc __PR((unsigned, char *, int));

#define	fmalloc(s)	dbg_fmalloc(s, __FILE__, __LINE__)
#endif


#ifndef	HAVE_REALLOC_NULL
#define	realloc	zrealloc
#endif

/*
 * Setup a callback for fatal() from libmpw
 */
extern	void	set_clean_up	__PR((void (*f)(void)));


# define RESPSIZE	1024
# define NVARGS		64
# define VSTART		3

/*
 *	The following five definitions (copy, xfopen, xfcreat, remove,
 *	USXALLOC) are taken from macros.h 1.1
 */

# define copy(srce, dest)	cat(dest, srce, (char *)0)
# define xfopen(file, mode)	fdfopen(xopen(file, mode), mode)
# define xfcreat(file, mode)	fdfopen(xcreat(file, mode), O_WRONLY|O_BINARY)
# define remove(file)		xunlink(file)
# define USXALLOC() \
		char *alloc(n) {return ((char *)xalloc((unsigned)n)); } \
		free(n) char *n; {xfree(n); } \
		char *malloc(n) unsigned n; {int p; p = xalloc(n); \
			return ((char *)(p != -1?p:0)); }


#if defined(linux) && !defined(NO_LINUX_SETVBUF_HACK) && \
    defined(HAVE_FILE__FLAGS) && defined(HAVE_FILE__IO_BUF_BASE) && \
    defined(_IO_USER_BUF)
/*
 * Work around a performance problem in the baroque stdio implementaion on
 * Linux.
 */
#define	setvbuf(f, buf, type, sz)	setvbuf(f, malloc(sz), type, sz)

static inline int xfclose __PR((FILE *f));
static inline int xfclose(f)
	FILE	*f;
{	int	ret;
	int	flags = f->_flags;
	char	*buf = f->_IO_buf_base;

	ret = fclose(f);
	if (ret == 0 && (flags & _IO_USER_BUF)) {
		free(buf);
	}
	return (ret);
}
#define	fclose(f)	xfclose(f)
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

#endif	/* _HDR_DEFINES_H */
