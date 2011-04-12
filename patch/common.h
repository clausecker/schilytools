/* @(#)common.h	1.14 11/01/30 2011 J. Schilling */
/*
 *	Copyright (c) 1986, 1988 Larry Wall
 *	Copyright (c) 2011 J. Schilling
 *
 *	This program may be copied as long as you don't try to make any
 *	money off of it, or pretend that you wrote it.
 */

#define	DEBUGGING

#include <schily/stdio.h>
#include <schily/types.h>
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
#include <schily/nlsdefs.h>
#ifndef	NO_SCHILY_PRINT
#include <schily/schily.h>
#endif

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
#define	Mktemp		(void)mktemp
#define	Strcpy		(void)strcpy
#define	Strcat		(void)strcat

/* constants */

#define	MAXHUNKSIZE	100000		/* is this enough lines? */
#define	INITHUNKMAX	125		/* initial dynamic allocation size */
#define	MAXLINELEN	8192
#define	BUFFERSIZE	8192
#define	SCCSPREFIX	"s."
#define	GET		"get -e %s"
#define	RCSSUFFIX	",v"
#define	CHECKOUT	"co -l %s"

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

EXT char buf[MAXLINELEN];		/* general purpose buffer */
EXT FILE *ofp;				/* output file pointer */
EXT FILE *rejfp;			/* reject file pointer */

EXT bool using_plan_a;			/* try to keep everything in memory */
EXT bool out_of_mem;			/* ran out of memory in plan a */

#define	MAXFILEC 2
EXT int filec;				/* how many file arguments? */
EXT char *filearg[MAXFILEC];
EXT bool ok_to_create_file;
EXT char *bestguess;			/* guess at correct filename */

EXT char *outname;			/*   -o outname		*/
EXT char rejname[128];			/*   -r rejname		*/

EXT char *origext;			/*   -b ext		*/
EXT char *origprae;

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
EXT int strippath;			/*   -p#		*/
EXT bool canonicalize;			/*   -l			*/

EXT bool do_posix;			/* POSIX vers. old patch behavior */
EXT bool wall_plus;			/* Permit enhancements from old patch */
EXT bool do_backup;			/* Create backup from original file */

#define	CONTEXT_DIFF		1
#define	NORMAL_DIFF		2
#define	ED_DIFF			3
#define	NEW_CONTEXT_DIFF	4
#define	UNI_DIFF		5

EXT int diff_type;			/*   -c/-e/-n/-u	*/

EXT char *revision;			/* prerequisite revision, if any */

extern	void	my_exit __PR((int status));
