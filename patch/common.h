/* @(#)common.h	1.29 18/04/10 2011-2018 J. Schilling */
/*
 *	Copyright (c) 1986, 1988 Larry Wall
 *	Copyright (c) 2011-2018 J. Schilling
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition is met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this condition and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define	DEBUGGING

#include <schily/stdio.h>
#include <schily/types.h>
#include <schily/time.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/stat.h>
#include <schily/fcntl.h>
#include <schily/assert.h>
#include <schily/ctype.h>
#include <schily/signal.h>
#include <schily/utypes.h>
#include <schily/errno.h>
#include <schily/nlsdefs.h>
#include <schily/schily.h>	/* e.g. ___malloc() */

#define	CH	(char)
#define	UCH	(unsigned char)
#define	C	(char *)
#define	UC	(unsigned char *)
#define	CP	(char **)
#define	UCP	(unsigned char **)
#define	CPP	(char ***)
#define	UCPP	(unsigned char ***)

#ifndef	NO_FLEXNAMES
/*
 * FLEXFILENAMES:
 *	This symbol, if defined, indicates that the system supports filenames
 *	longer than 14 characters.
 */
#define	FLEXFILENAMES
#endif

/* shut lint up about the following when return value ignored */

#define	Signal		(void)signal
#define	Unlink		(void)unlink
#define	Lseek		(void)lseek
#define	Fseek		(void)fseek
#define	Fstat		(void)fstat
#define	Pclose		(void)pclose
#define	Close		(void)close
#define	Fclose		(void)fclose
#define	Fflush		(void)fflush
#define	Sprintf		(void)sprintf
#define	Snprintf	(void)snprintf
#define	Mktemp		(void)mktemp
#define	Strcpy		(void)strcpy
#define	Strcat		(void)strcat

/* constants */

#define	INITHUNKMAX	125		/* initial dynamic allocation size */
#define	BUFFERSIZE	8192
#define	SCCSPREFIX	"s."
#define	GET		"get"		/* use: "get -e %s" */
#define	GETEDIT		"-e"
#define	RCSSUFFIX	",v"
#define	CHECKOUT	"co"		/* use: "co -l %s" */
#define	COEDIT		"-l"

#ifdef FLEXFILENAMES
#define	ORIGEXT		".orig"
#define	REJEXT		".rej"
#else
#define	ORIGEXT		"~"
#define	REJEXT		"#"
#endif

/* handy definitions */

#define	Null(t)		((t)0)
#define	Nullch		Null(char *)
#define	Nullfp		Null(FILE *)
#define	Nulline		Null(LINENUM)

#define	strNE(s1, s2)		(strcmp(s1, s2))
#define	strEQ(s1, s2)		(strcmp(s1, s2) == 0)
#define	strnNE(s1, s2, l)	(strncmp(s1, s2, l))
#define	strnEQ(s1, s2, l)	(strncmp(s1, s2, l) == 0)

/* typedefs */

typedef int	bool;
typedef off_t	LINENUM;		/* must be signed */
typedef unsigned MEM;			/* what to feed malloc */

/* globals */

EXT int Argc;				/* guess */
EXT char **Argv;
EXT int Argc_last;			/* for restarting plan_b */
EXT char **Argv_last;

EXT struct stat file_stat;		/* file statistics area */
EXT int filemode;
EXT struct timespec filetime;

EXT char *buf;				/* general purpose buffer */
EXT size_t bufsize;			/* current size of buf */
EXT FILE *ofp;				/* output file pointer */
EXT FILE *rejfp;			/* reject file pointer */

EXT bool using_plan_a;			/* try to keep everything in memory */
EXT bool out_of_mem;			/* ran out of memory in plan a */

#define	MAXFILEC 2
EXT int filec;				/* how many file arguments? */
EXT char *filearg[MAXFILEC];
EXT bool ok_to_create_file;
EXT bool is_null_time[2];
EXT char *bestguess;			/* guess at correct filename */
EXT time_t starttime;

EXT char *outname;			/*   -o outname		*/
EXT char rejname[128];			/*   -r rejname		*/

EXT char *origext;			/*   -b ext		*/
EXT char *origprae;

EXT int	TMPDLEN;
EXT char *TMPDIR;
EXT char *TMPOUTNAME;
EXT char *TMPINNAME;
EXT char *TMPREJNAME;
EXT char *TMPPATNAME;
EXT bool toutkeep;
EXT bool trejkeep;

EXT LINENUM last_offset;
#ifdef DEBUGGING
EXT int debug;				/*   -x#		*/
#endif
EXT bool force;				/*   -f			*/
EXT bool verbose;			/* ! -s			*/
EXT bool reverse;			/*   -R			*/
EXT bool noreverse;			/*   -N			*/
EXT bool skip_rest_of_patch;		/*   -S			*/
EXT bool touch_local;			/*   -T			*/
EXT bool touch_gmt;			/*   -Z			*/
EXT int strippath;			/*   -p#		*/
EXT bool canonicalize;			/*   -l			*/

EXT bool do_posix;			/* POSIX vers. old patch behavior */
EXT bool do_wall;			/* Old Larry patch-2.0 compatibility */
EXT bool wall_plus;			/* Permit enhancements from old patch */
EXT bool do_backup;			/* Create backup from original file */

#define	CONTEXT_DIFF		1
#define	NORMAL_DIFF		2
#define	ED_DIFF			3
#define	NEW_CONTEXT_DIFF	4
#define	UNI_DIFF		5

#define	EXIT_OK			0
#define	EXIT_REJECT		1
#define	EXIT_FAIL		2
#define	EXIT_SIGNAL		3

EXT int diff_type;			/*   -c/-e/-n/-u	*/

EXT char *revision;			/* prerequisite revision, if any */

EXT LINENUM p_repl_lines;		/* From pch.c # of replacement lines */

extern	void	my_exit __PR((int status));

#ifdef	HAVE_GETDELIM
#define	fgetaline(f, bufp, lenp)	getdelim(bufp, lenp, '\n', f)
#endif
