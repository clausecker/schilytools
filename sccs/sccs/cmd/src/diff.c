/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * University Copyright- Copyright (c) 1982, 1986, 1988
 * The Regents of the University of California
 * All Rights Reserved
 *
 * University Acknowledgment- Portions of this document are derived from
 * software developed by the University of California, Berkeley, and its
 * contributors.
 */
/*
 * Copyright 2006-2017 J. Schilling
 *
 * @(#)diff.c	1.71 17/02/03 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)diff.c 1.71 17/02/03 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)diff.c	1.55	05/07/22 SMI"
#endif

/*
 *	diff - differential file comparison
 *
 *	Uses an algorithm due to Harold Stone, which finds
 *	a pair of longest identical subsequences in the two
 *	files.
 *
 *	The major goal is to generate the match vector J.
 *	J[i] is the index of the line in file1 corresponding
 *	to line i file0. J[i] = 0 if there is no
 *	such line in file1.
 *
 *	Lines are hashed so as to work in core. All potential
 *	matches are located by sorting the lines of each file
 *	on the hash (called value). In particular, this
 *	collects the equivalence classes in file1 together.
 *	Subroutine equiv  replaces the value of each line in
 *	file0 by the index of the first element of its
 *	matching equivalence in (the reordered) file1.
 *	To save space equiv squeezes file1 into a single
 *	array member in which the equivalence classes
 *	are simply concatenated, except that their first
 *	members are flagged by changing sign.
 *
 *	Next the indices that point into member are unsorted into
 *	array class according to the original order of file0.
 *
 *	The cleverness lies in routine stone. This marches
 *	through the lines of file0, developing a vector klist
 *	of "k-candidates". At step i a k-candidate is a matched
 *	pair of lines x,y (x in file0 y in file1) such that
 *	there is a common subsequence of lenght k
 *	between the first i lines of file0 and the first y
 *	lines of file1, but there is no such subsequence for
 *	any smaller y. x is the earliest possible mate to y
 *	that occurs in such a subsequence.
 *
 *	Whenever any of the members of the equivalence class of
 *	lines in file1 matable to a line in file0 has serial number
 *	less than the y of some k-candidate, that k-candidate
 *	with the smallest such y is replaced. The new
 *	k-candidate is chained (via pred) to the current
 *	k-1 candidate so that the actual subsequence can
 *	be recovered. When a member has serial number greater
 *	that the y of all k-candidates, the klist is extended.
 *	At the end, the longest subsequence is pulled out
 *	and placed in the array J by unravel.
 *
 *	With J in hand, the matches there recorded are
 *	checked against reality to assure that no spurious
 *	matches have crept in due to hashing. If they have,
 *	they are broken, and "jackpot " is recorded--a harmless
 *	matter except that a true match for a spuriously
 *	mated line may now be unnecessarily reported as a change.
 *
 *	Much of the complexity of the program comes simply
 *	from trying to minimize core utilization and
 *	maximize the range of doable problems by dynamically
 *	allocating what is needed and reusing what is not.
 *	The core requirements for problems larger than somewhat
 *	are (in words) 2*length(file0) + length(file1) +
 *	3*(number of k-candidates installed),  typically about
 *	6n words for files of length n.
 *
 *	JS remarks:
 *
 *	The amount of memory "in use" largely depends on the allocation
 *	algorithm as the code intensively uses realloc().
 *
 *	If the files differ, we always allocate
 *	sizeof (off_t) * (lines(file0) + lines(file1))
 *	In largefile mode we typically allocate less than 20% more
 *	than in non-largefile mode. It seems that in largefile mode, the
 *	amount of space need is typically ~ 5 * sizeof (int) *
 *	(lines(file0) + lines(file1)).
 *
 *	Largefile mode is neeeded in order to be able to deal with files
 *	in the 64 bit inode # range on ZFS. Sun used to use a partial
 *	largefile mode but this is not portable, so we switched to a full
 *	large file mode.
 *
 *	The code currently uses "int" for line numbers but should use "long".
 *	If the code is changed to use "long", it would be possible to have
 *	real large file mode in case that diff is compiled in 64 bit mode.
 *	In 32 bit mode, there is no need to check for an integer overflow
 *	as the process will run "out of memory" before line numbers overflow.
 */
#ifdef	SCHILY_BUILD

#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/wchar.h>
#include <schily/wctype.h>
#include <schily/ctype.h>
#include <schily/stdlib.h>
#include <schily/limits.h>
#include <schily/types.h>
#include <schily/stat.h>
#include <schily/wait.h>
#include <schily/unistd.h>
#include <schily/signal.h>
#include <schily/fcntl.h>
#include <schily/dirent.h>
#include <schily/maxpath.h>
#include <schily/nlsdefs.h>
#include <schily/varargs.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/time.h>
#include <version.h>
#include <schily/sysexits.h>
#define	VMS_VFORK_OK
#include <schily/vfork.h>
#include <schily/libport.h>

#else	/* non-portable SunOS -only definitions BEGIN */

#define	SCCS_DIFF		0
#define	_FILE_OFFSET_BITS	64
#define	_LARGEFILE_SOURCE
#include <stdio.h>
#include <wchar.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <locale.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sysexits.h>

#ifndef	PROVIDER
#define	PROVIDER	"Schily"
#endif
#ifndef	VERSION
#define	VERSION		"5.08"
#endif
#ifndef	HOST_OS
#define	HOST_OS		"SunOS"
#endif
#ifndef	VDATE
#define	VDATE		""
#endif

#ifdef	USE_VERSION_H
#include <version.h>
#endif

#ifdef	__sparc
#define	HOST_CPU	"sparc"
#define	HOST_VENDOR	"Sun"
#endif
#if defined(__i386) || defined(__amd64)
#define	HOST_CPU	"i386"
#define	HOST_VENDOR	"pc"
#endif
#ifndef	HOST_CPU
#define	HOST_CPU	"unknown"
#endif
#ifndef	HOST_VENDOR
#define	HOST_VENDOR	"unknown"
#endif
#ifndef	HOST_OS
#define	HOST_OS		"unknown"
#endif

#define	PROTOTYPES
#define	__PR(a)	a
#define	EXPORT
#define	LOCAL	static

#define	HAVE_LARGEFILES
#define	HAVE_CFTIME
#define	HAVE_GETEXECNAME
#define	HAVE_MEMCMP
#define	HAVE_VFORK
#define	_FOUND_STAT_NSECS_
#define	HAVE_REALLOC_NULL

/*
 * Found e.g. on SunOS-5.x
 */
#define	stat_ansecs(s)		((s)->st_atim.tv_nsec)
#define	stat_mnsecs(s)		((s)->st_mtim.tv_nsec)
#define	stat_cnsecs(s)		((s)->st_ctim.tv_nsec)

#endif	/* non-portable SunOS -only definitions END */


#ifdef	HAVE_LARGEFILES
#undef	fseek
#define	fseek		fseeko
#undef	ftell
#define	ftell		ftello
#endif

#include "diff.h"

#ifndef	PATH_MAX
#ifdef	MAXPATHNAME
#define	PATH_MAX	MAXPATHNAME
#endif
#endif
#ifndef	PATH_MAX
#define	PATH_MAX	1024
#endif

#ifdef	pdp11
#define	D_BUFSIZ	BUFSIZ
#else
#define	D_BUFSIZ	(32*1024)
#endif

/*
 * In case this is missing in /usr/include/ (e.g. SunOS-4.x)
 */
extern char	*optarg;
extern int	optind, opterr, optopt;

#define	CHRTRAN(x)	(iflag ? (iswupper(x) ? towlower(x) : (x)) : (x))
#define	NCCHRTRAN(x)	(iswupper(x) ? towlower(x) : (x))
#undef	max
#define	max(a, b)	((a) < (b) ? (b) : (a))
#undef	min
#define	min(a, b)	((a) > (b) ? (b) : (a))

static int pref, suff;		/* length of prefix and suffix */
static int *class;		/* will be overlaid on file[0] */
static int *member;		/* will be overlaid on file[1] */
static int *klist;		/* will be overlaid on file[0] after class */
static struct cand *clist;	/* merely a free storage pot for candidates */
static int clen = 0;
static int camt = 0;
static int *J;			/* will be overlaid on class */
static off_t *ixold;		/* will be overlaid on klist */
static off_t *ixnew;		/* will be overlaid on file[1] */

#define	FUNCTION_CONTEXT_SIZE	55
static char	lastbuf[FUNCTION_CONTEXT_SIZE];
static int	lastline;
static int	lastmatchline;

static int	didvfork;
static int	mbcurmax;

static char	nulldev[] = "/dev/null";

static void	*talloc __PR((size_t n));
static void	*ralloc __PR((void *p, size_t n));
	int	main __PR((int argc, char **argv));
static void	error __PR((const char *));
static void	unravel __PR((int));
static void	check __PR((void));
static void	output __PR((void));
static void	change __PR((int, int, int, int));
static void	range __PR((int, int, char *));
static void	fetch __PR((off_t *, int, int, int, char *, int));
static void	dump_context_vec __PR((void));
static void	diffdir __PR((char **, struct pdirs *));
static void	calldiffdir __PR((char **, struct pdirs *));
static void	setfile __PR((char **, char **, char *));
static void	scanpr __PR((struct dir *, int, char *, char *,
	char *, char *, char *));
static void	only __PR((struct dir *, int));
static void	sort __PR((struct line *, int));
static void	unsort __PR((struct line *, int, int *));
static void	filename __PR((char **, char **, struct stat *, char **));
static void	prepare __PR((int, char *));
static void	prune __PR((void));
static void	equiv __PR((struct line *, int, struct line *, int, int *));
static void	done __PR((void));
static void	noroom __PR((void));
static void	usage __PR((void));
static void	initbuf __PR((FILE *, int, off_t));
static void	resetbuf __PR((int));

static int	diffreg __PR((void));
static void	print_dstatus __PR((void));
#ifndef	DO_SPAWN_DIFF
static int	calldiffreg __PR((int, int));
#endif
static int	stone __PR((int *, int, int *, int *));
static int	newcand __PR((int, int, int));
static int	search __PR((int *, int, int));
static int	skipline __PR((int));
static int	readhash __PR((FILE *, int, char *));
static int	entcmp __PR((struct dir *, struct dir *));
static int	compare __PR((struct dir *));
static int	calldiff __PR((char *));
static int	binary __PR((int));
static int	filebinary __PR((FILE *));
static int	isbinary __PR((char *, int));
static int	useless __PR((char *));
static char	*copytemp __PR((char *));
static char	*pfiletype __PR((mode_t));
static struct dir *setupdir __PR((char *));
static off_t	ftellbuf __PR((int));
static wint_t	wcput	__PR((wint_t));
static wint_t	getbufwchar __PR((int, int *));
#if !defined(HAVE_CFTIME) && defined(HAVE_STRFTIME)
static time_t	gmtoff __PR((const time_t *clk));
#endif
static void	cf_time	__PR((char *s, size_t maxsize,
				char *fmt, const time_t *clk));

static char	*match_function __PR((const off_t *, int, int));


/*
 * error message string constants
 */
#define	BAD_MB_ERR	"invalid multibyte character encountered"
#define	NO_PROCS_ERR	"no more processes"
#define	NO_MEM_ERR	"out of memory"

static void *
talloc(n)
	size_t	n;
{
	void *p;
	p = malloc(n);
	if (p == NULL)
		noroom();
	return (p);
}

static void *
ralloc(p, n)			/* compacting reallocation */
	void	*p;
	size_t	n;
{
	void	*q;
#if 0
	free(p);
#endif
#ifndef	HAVE_REALLOC_NULL
	if (p == NULL)
		q = malloc(n);
	else
#endif
	q = realloc(p, n);
	if (q == NULL)
		noroom();
	return (q);
}

#ifdef	DEBUG
extern	char	*_end;
long	oe = (long)&_end;
#endif

int
main(argc, argv)
	int	argc;
	char	**argv;
{
	char *argp;
	int flag;			/* option flag read by getopt() */
	int i, j;
	char buf1[D_BUFSIZ], buf2[D_BUFSIZ];


	(void) setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)		/* Should be defined by cc -D */
#define	TEXT_DOMAIN	"SYS_TEST"	/* Use this only if it weren't */
#endif
	(void) textdomain(TEXT_DOMAIN);

	mbcurmax = MB_CUR_MAX;

	diffargv = argv;
	whichtemp = 0;
	while ((flag =
	    getopt(argc, argv,
		    "()abBdiptwcuefhnqlrsNC:D:S:U:V(version)")) != EOF) {
		switch (flag) {
		case 'D':
			opt = D_IFDEF;
			wantelses = 1;
			ifdef1 = "";
			ifdef2 = optarg;
			break;

		case 'a':
			aflag = 1;
			break;

		case 'b':
			bflag = 1;
			break;

		case 'B':
			Bflag = 1;
			break;

		case 'd':		/* -d is a no-op for GNU diff compat */
			break;

		case 'C':
		case 'U':
			opt = D_CONTEXT;
			argp = optarg;
			context = 0;
			while (*argp >= '0' && *argp <= '9')
				context *= 10, context += *argp++ - '0';
			if (*argp)
				error(gettext("use [ -C num | -U num ]"));
			if (flag == 'U')
				uflag++;
			else
				uflag = 0;
			break;

		case 'c':
		case 'u':
			opt = D_CONTEXT;
			context = 3;
			if (flag == 'u')
				uflag++;
			else
				uflag = 0;
			break;

		case 'N':
			Nflag = 1;
			break;

		case 'e':
			opt = D_EDIT;
			break;

		case 'f':
			opt = D_REVERSE;
			break;

		case 'h':
			hflag++;
			break;

		case 'i':
			iflag = 1;
			break;

		case 'l':
			lflag = 1;
			break;

		case 'n':
			opt = D_NREVERSE;
			break;

		case 'p':
			pflag = 1;
			if (opt == D_NORMAL) {
				opt = D_CONTEXT;
				context = 3;
				uflag = 0;
			}
			break;

		case 'q':
			opt = D_BRIEF;
			break;

		case 'r':
			rflag = 1;
			break;

		case 'S':
			start = optarg;
			break;

		case 's':
			sflag = 1;
			break;

		case 't':
			tflag = 1;
			break;

		case 'w':
			wflag = 1;
			break;

		case '?':
			usage();
			break;

		case 'V':		/* version */
			printf("diff %s-%s version %s%s%s (%s-%s-%s)\n",
				PROVIDER,
				SCCS_DIFF ? "SCCS":"diff",
				VERSION,
				*VDATE ? " ":"",
				VDATE,
				HOST_CPU, HOST_VENDOR, HOST_OS);
			exit(EX_OK);

		default:
			/* Not sure how it would get here, but just in case */
			(void) fprintf(stderr, "diff: ");
			(void) fprintf(stderr,
				gettext("invalid option -%c\n"), flag);
			usage();
		}
	}

	argc -= optind;
	argv = &argv[optind];

	if (opt != D_CONTEXT && uflag)
		uflag = 0;

	if (argc != 2)
		error(gettext("two filename arguments required"));

	file1 = argv[0];
	file2 = argv[1];
	file1ok = file2ok = 1;
	status = 0;

	if (hflag) {
		if (opt) {
			error(
gettext("-h doesn't support -e, -f, -n, -c, -q, -u, or -D"));
		} else {
			diffargv[0] = "diffh";
			(void) execv(diffh, diffargv);
			(void) fprintf(stderr, "diffh: ");
			perror(diffh);
			status = 2;
			done();
		}

	}

	if (Nflag) {
		int	fail = 0;

		if (stat(file1, &stb1) < 0) {
			if (errno == ENOENT) {		/* file1 nonexisting */
				file1ok = 0;
				stb1.st_mode = S_IFREG;	/* assume plain file */
				stb1.st_size = 0;
				input[0] = fopen(nulldev, "r");
				fail += 1;
			}
		}
		if (stat(file2, &stb2) < 0) {
			if (errno == ENOENT) {
				file2ok = 0;
				if (!fail)
					stb2.st_mode = stb1.st_mode;
				else
					stb2.st_mode = S_IFREG;
				stb2.st_size = 0;
				input[1] = fopen(nulldev, "r");
				fail += 1;
			}
		} else if (fail == 1) {			/* file2 exists	    */
			stb1.st_mode = stb2.st_mode;	/* file1 like file2 */
		}
		if (fail > 1) {				/* both files missing */
			if (input[0] != NULL)
				fclose(input[0]);
			if (input[1] != NULL)
				fclose(input[1]);
			input[0] = NULL;
			input[1] = NULL;
		}
	}

	if (strcmp(file1, "-") == 0) {
		if (fstat(fileno(stdin), &stb1) == 0) {
			stb1.st_mode = S_IFREG;
		} else {
			(void) fprintf(stderr, "diff: ");
			perror("stdin");
			status = 2;
			done();
		}
	} else if (input[0] == NULL && stat(file1, &stb1) < 0) {
		(void) fprintf(stderr, "diff: ");
		perror(file1);
		status = 2;
		done();
	}

	if (strcmp(file2, "-") == 0) {
		if (strcmp(file1, "-") == 0) {
			error(gettext("cannot specify - -"));
		} else {
			if (fstat(fileno(stdin), &stb2) == 0) {
				stb2.st_mode = S_IFREG;
			} else {
				(void) fprintf(stderr, "diff: ");
				perror("stdin");
				status = 2;
				done();
			}
		}
	} else if (input[1] == NULL && stat(file2, &stb2) < 0) {
		(void) fprintf(stderr, "diff: ");
		perror(file2);
		status = 2;
		done();
	}

	if ((stb1.st_mode & S_IFMT) == S_IFDIR &&
	    (stb2.st_mode & S_IFMT) == S_IFDIR) {
		diffdir(argv, (struct pdirs *)NULL);
		done();
	    }

	filename(&file1, &file2, &stb1, &input_file1);
	filename(&file2, &file1, &stb2, &input_file2);
	if (input[0] == NULL && (input[0] = fopen(file1, "r")) == NULL) {
		(void) fprintf(stderr, "diff: ");
		perror(file1);
		status = 2;
		done();
	}
	initbuf(input[0], 0, (off_t)0);

	if (input[1] == NULL && (input[1] = fopen(file2, "r")) == NULL) {
		(void) fprintf(stderr, "diff: ");
		perror(file2);
		status = 2;
		done();
	}
	initbuf(input[1], 1, (off_t)0);

#ifdef	HAVE_SETVBUF
	setvbuf(input[0], NULL, _IOFBF, 32*1024);
	setvbuf(input[1], NULL, _IOFBF, 32*1024);
#endif

	if (stb1.st_size != stb2.st_size)
		goto notsame;

	for (;;) {
		i = fread(buf1, 1, D_BUFSIZ, input[0]);
		j = fread(buf2, 1, D_BUFSIZ, input[1]);
		if (ferror(input[0]) || ferror(input[1])) {
			(void) fprintf(stderr, "diff: ");
			(void) fprintf(stderr, gettext("Error reading "));
			perror(ferror(input[0])? file1:file2);
			(void) fclose(input[0]);
			(void) fclose(input[1]);
			status = 2;
			done();
		}
		if (i != j)
			goto notsame;
		if (i == 0 && j == 0) {
			/* files are the same; diff -D needs to print one */
			if (opt == D_IFDEF) {
				rewind(input[0]);
				while ((i =
				    fread(buf1, 1, D_BUFSIZ, input[0])) > 0) {
					(void) fwrite(buf1, 1, i, stdout);
				}
			}
			(void) fclose(input[0]);
			(void) fclose(input[1]);
			status = 0;
			goto same;		/* files don't differ */
		}
#ifdef	HAVE_MEMCMP
		if (memcmp(buf1, buf2, i))
			goto notsame;
#else
		for (j = 0; j < i; j++)
			if (buf1[j] != buf2[j])
				goto notsame;
#endif
	}

notsame:
	status = 1;
	if (!aflag &&
	    (filebinary(input[0]) || filebinary(input[1]))) {
		if (ferror(input[0]) || ferror(input[1])) {
			(void) fprintf(stderr, "diff: ");
			(void) fprintf(stderr, gettext("Error reading "));
			perror(ferror(input[0])? file1:file2);
			(void) fclose(input[0]);
			(void) fclose(input[1]);
			status = 2;
			done();
		}
		(void) printf(gettext("Binary files %s and %s differ\n"),
		    file1, file2);
		(void) fclose(input[0]);
		(void) fclose(input[1]);
		done();
	}
	anychange = diffreg();
	status = anychange;

same:
	print_dstatus();

#ifdef	DEBUG
	fprintf(stderr, "Allocates space: %ld Bytes\n", (long)sbrk(0) - oe);
#endif
	done();
	/*NOTREACHED*/
	return (0);
}

static int
diffreg()
{
	int	k;

	anychange = 0;
	lastline = 0;
	lastmatchline = 0;

	prepare(0, file1);
	prepare(1, file2);
	prune();
	sort(sfile[0], slen[0]);
	sort(sfile[1], slen[1]);

	member = (int *)file[1];
	equiv(sfile[0], slen[0], sfile[1], slen[1], member);
	member = (int *)ralloc((void *)member, (slen[1] + 2) * sizeof (int));

	class = (int *)file[0];
	unsort(sfile[0], slen[0], class);
	class = (int *)ralloc((void *)class, (slen[0] + 2) * sizeof (int));

	klist = (int *)talloc((slen[0] + 2) * sizeof (int));
	clist = (struct cand *)talloc(sizeof (cand));
	clen = 0;
	camt = 0;
	k = stone(class, slen[0], member, klist);
	free((void *)member);
	free((void *)class);

	J = (int *)ralloc(J, (len[0] + 2) * sizeof (int));
	unravel(klist[k]);
	free((char *)clist);
	free((char *)klist);

	ixold = (off_t *)ralloc(ixold, (len[0] + 2) * sizeof (off_t));
	ixnew = (off_t *)ralloc(ixnew, (len[1] + 2) * sizeof (off_t));
	check();
	output();

	return (anychange);
}

static void
print_dstatus()
{
	if (opt == D_CONTEXT && anychange == 0)
		(void) printf(gettext("No differences encountered\n"));
	else if (opt == D_BRIEF && anychange != 0)
		(void) printf(gettext("Files %s and %s differ\n"),
		    file1, file2);
}

#ifndef	DO_SPAWN_DIFF
static int
calldiffreg(f1, f2)
	int	f1;
	int	f2;
{
	int	result = anychange;
	int	ret;
	char	*in1 = input_file1;
	char	*in2 = input_file2;

	input_file1 = file1;
	input_file2 = file2;

	if ((input[0] = fdopen(f1, "r")) == NULL) {
		(void) fprintf(stderr, "diff: ");
		perror(file1);
		status = 2;
		done();
	}
	if ((input[1] = fdopen(f2, "r")) == NULL) {
		(void) fprintf(stderr, "diff: ");
		perror(file1);
		status = 2;
		done();
	}

	initbuf(input[0], 0, (off_t)0);
	initbuf(input[1], 1, (off_t)0);

#ifdef	HAVE_SETVBUF
	setvbuf(input[0], NULL, _IOFBF, 32*1024);
	setvbuf(input[1], NULL, _IOFBF, 32*1024);
#endif
	rewind(input[0]);
	rewind(input[1]);

	ret = diffreg();
	print_dstatus();
	fclose(input[0]);
	fclose(input[1]);

	input_file1 = in1;
	input_file2 = in2;
	if (ret > result)
		result = ret;
	return (result);
}
#endif

static int
stone(a, n, b, c)
	int	*a;
	int	n;
	int	*b;
	int	*c;
{
	int i, k, y;
	int j, l;
	int oldc, tc;
	int oldl;

	k = 0;
	c[0] = newcand(0, 0, 0);
	for (i = 1; i <= n; i++) {
		j = a[i];
		if (j == 0)
			continue;
		y = -b[j];
		oldl = 0;
		oldc = c[0];
		do {
			if (y <= clist[oldc].y)
				continue;
			l = search(c, k, y);
			if (l != oldl+1)
				oldc = c[l-1];
			if (l <= k) {
				if (clist[c[l]].y <= y)
					continue;
				tc = c[l];
				c[l] = newcand(i, y, oldc);
				oldc = tc;
				oldl = l;
			} else {
				c[l] = newcand(i, y, oldc);
				k++;
				break;
			}
		} while ((y = b[++j]) > 0);
	}
	return (k);
}

static int
newcand(x, y, pred)
	int	x;
	int	y;
	int	pred;
{
	struct cand *q;

	if (clen >= camt) {
		camt += 64;
		clist = (struct cand *)ralloc((void *)clist,
		    camt * sizeof (cand));
	}
	q = clist + clen;
	q->x = x;
	q->y = y;
	q->pred = pred;
	return (clen++);
}

static int
search(c, k, y)
	int	*c;
	int	k;
	int	y;
{
	int i, j, l;
	int t;

	if (clist[c[k]].y < y)	/* quick look for typical case */
		return (k + 1);
	i = 0;
	j = k+1;
	while ((l = (i + j) / 2) > i) {
		t = clist[c[l]].y;
		if (t > y)
			j = l;
		else if (t < y)
			i = l;
		else
			return (l);
	}
	return (l + 1);
}

static void
unravel(p)
	int	p;
{
	int i;
	struct cand *q;

	for (i = 0; i <= len[0]; i++)
		J[i] = i <= pref ? i :
			i > len[0] - suff ? i + len[1] - len[0]:
			0;
	for (q = clist + p; q->y != 0; q = clist + q->pred)
		J[q->x + pref] = q->y + pref;
}

/*
 * check does double duty:
 * 1. ferret out any fortuitous correspondences due to confounding by
 * hashing (which result in "jackpot")
 * 2. collect random access indexes to the two files
 */

static void
check()
{
	wint_t	c, d;
	int i, j;
	/* int jackpot; */
	int	mlen;
	off_t ctold, ctnew;

	resetbuf(0);
	resetbuf(1);

	j = 1;
	ixold[0] = ixnew[0] = 0;
	/* jackpot = 0; */

	/*
	 * ctold and ctnew are byte positions within the file (suitable for
	 * lseek()).  After we get a character with getwc(), instead of
	 * just incrementing the byte position by 1, we have to determine
	 * how many bytes the character actually is.  This is the reason for
	 * the wctomb() calls here and in skipline().
	 */
	ctold = ctnew = 0;
	for (i = 1; i <= len[0]; i++) {
		if (J[i] == 0) {
			ixold[i] = ctold += skipline(0);
			continue;
		}
		while (j < J[i]) {
			ixnew[j] = ctnew += skipline(1);
			j++;
		}
		if (bflag || wflag || iflag) {
			for (;;) {
				c = getbufwchar(0, &mlen);
				ctold += mlen;
				d = getbufwchar(1, &mlen);
				ctnew += mlen;

				if (bflag && iswspace(c) && iswspace(d)) {
					while (iswspace(c)) {
						if (c == '\n' || c == WEOF)
							break;

						c = getbufwchar(0, &mlen);
						ctold += mlen;
					}
					while (iswspace(d)) {
						if (d == '\n' || d == WEOF)
							break;

						d = getbufwchar(1, &mlen);
						ctnew += mlen;
					}
				} else if (wflag) {
					while (iswspace(c) && c != '\n') {
						c = getbufwchar(0, &mlen);
						ctold += mlen;
					}
					while (iswspace(d) && d != '\n') {
						d = getbufwchar(1, &mlen);
						ctnew += mlen;
					}
				}
				if (c == WEOF || d == WEOF) {
					if (c != d) {
						/* jackpot++; */
						J[i] = 0;
						if (c != '\n' && c != WEOF)
							ctold += skipline(0);
						if (d != '\n' && d != WEOF)
							ctnew += skipline(1);
						break;
					}
					break;
				} else {
					if (CHRTRAN(c) != CHRTRAN(d)) {
						/* jackpot++; */
						J[i] = 0;
						if (c != '\n')
							ctold += skipline(0);
						if (d != '\n')
							ctnew += skipline(1);
						break;
					}
					if (c == '\n')
						break;
				}
			}
		} else {
			for (;;) {
				c = getbufwchar(0, &mlen);
				ctold += mlen;
				d = getbufwchar(1, &mlen);
				ctnew += mlen;
				if (c != d) {
					/* jackpot++; */
					J[i] = 0;
					if (c != '\n' && c != WEOF)
						ctold += skipline(0);
					if (d != '\n' && d != WEOF)
						ctnew += skipline(1);
					break;
				}
				if (c == '\n' || c == WEOF)
					break;
			}
		}
		ixold[i] = ctold;
		ixnew[j] = ctnew;
		j++;
	}
	for (; j <= len[1]; j++) {
		ixnew[j] = ctnew += skipline(1);
	}

/*	if(jackpot)			*/
/*		fprintf(stderr, "diff: jackpot\n");	*/
}

static int
skipline(f)
	int	f;
{
	int i;
	wint_t c;
	int	mlen;

	for (i = 1; (c = getbufwchar(f, &mlen)) != '\n' && c != WEOF; ) {
		i += mlen;
	}
	return (i);
}

static void
output()
{
	int m;
	wint_t	wc;
	int i0, i1, j1;
	int j0;
	int	mlen;

	resetbuf(0);
	resetbuf(1);

	m = len[0];
	J[0] = 0;
	J[m + 1] = len[1] + 1;
	if (opt != D_EDIT) {
		for (i0 = 1; i0 <= m; i0 = i1 + 1) {
			while (i0 <= m && J[i0] == J[i0 - 1] + 1)
				i0++;
			j0 = J[i0 - 1] + 1;
			i1 = i0 - 1;
			while (i1 < m && J[i1 + 1] == 0)
				i1++;
			j1 = J[i1 + 1] - 1;
			J[i1] = j1;
			change(i0, i1, j0, j1);
		}
	} else {
		for (i0 = m; i0 >= 1; i0 = i1 - 1) {
			while (i0 >= 1 && J[i0] == J[i0 + 1] - 1 && J[i0] != 0)
				i0--;
			j0 = J[i0 + 1] - 1;
			i1 = i0 + 1;
			while (i1 > 1 && J[i1 - 1] == 0)
				i1--;
			j1 = J[i1 - 1] + 1;
			J[i1] = j1;
			change(i1, i0, j1, j0);
		}
	}
	if (m == 0)
		change(1, 0, 1, len[1]);
	if (opt == D_IFDEF) {
		for (;;) {
			wc = getbufwchar(0, &mlen);
			if (wc == WEOF)
				return;
			(void) wcput(wc);
		}
	}
	if (anychange && opt == D_CONTEXT)
		dump_context_vec();
}


/*
 * indicate that there is a difference between lines a and b of the from file
 * to get to lines c to d of the to file.
 * If a is greater then b then there are no lines in the from file involved
 * and this means that there were lines appended (beginning at b).
 * If c is greater than d then there are lines missing from the to file.
 */
static void
change(a, b, c, d)
	int	a;
	int	b;
	int	c;
	int	d;
{
	char	time_buf[BUFSIZ];
	char	*dcmsg;
	char	*p;

#if	BUFSIZ < 40	/* 36 should be sufficient */
	error time buffer too small
#endif
	if (opt != D_IFDEF && a > b && c > d)
		return;
	if (Bflag) {
		int	i;

		/*
		 * On a UNIX system, enpty lines only contain a single "\n" and
		 * thus cause a file offset difference of 1 to the next line.
		 */
		if (a > b) {	/* Inserted */
			for (i = c; i <= d; i++) {
				if ((ixnew[i] - ixnew[i - 1]) != 1)
					goto show;
			}
		}
		if (c > d) {	/* Deleted */
			for (i = a; i <= b; i++) {
				if ((ixold[i] - ixold[i - 1]) != 1)
					goto show;
			}
		}
		return;
	}
show:
	if (anychange == 0) {
		anychange = 1;
		if (opt == D_CONTEXT) {
			/*
			 * TRANSLATION_NOTE_FOR_DC
			 * This message is the format of file
			 * timestamps written with the -C and
			 * -c options.
			 * %a -- locale's abbreviated weekday name
			 * %b -- locale's abbreviated month name
			 * %e -- day of month [1,31]
			 * %T -- Time as %H:%M:%S
			 * %Y -- Year, including the century
			 */
			if (uflag) {
#if	!defined(_FOUND_STAT_NSECS_)
				dcmsg = "%Y-%m-%d %H:%M:%S %z";
#else
				dcmsg = "%Y-%m-%d %H:%M:%S.000000000 %z";
#endif
			} else {
				dcmsg = dcgettext(NULL, "%a %b %e %T %Y",
								LC_TIME);
			}
			cf_time(time_buf, sizeof (time_buf),
						dcmsg, &stb1.st_mtime);

#if	defined(_FOUND_STAT_NSECS_)
			/*
			 * Be careful here: in the German locale, the string
			 * contains "So. " for "Sonntag".
			 */
			if ((p = strchr(time_buf, '.')) != NULL &&
			    p[1] == '0') {
				long	ns = stat_mnsecs(&stb1);
				if (ns < 0)
					ns = 0;
				sprintf(++p, "%9.9ld", ns);
				p[9] = ' ';	/* '\0' from sprintf() */
			}
#endif
			if (uflag)
				(void) printf("--- %s	%s\n", input_file1,
				    time_buf);
			else
				(void) printf("*** %s	%s\n", input_file1,
				    time_buf);
			cf_time(time_buf, sizeof (time_buf),
						dcmsg, &stb2.st_mtime);

#if	defined(_FOUND_STAT_NSECS_)
			/*
			 * Be careful here: in the German locale, the string
			 * contains "So. " for "Sonntag".
			 */
			if ((p = strchr(time_buf, '.')) != NULL &&
			    p[1] == '0') {
				long	ns = stat_mnsecs(&stb2);
				if (ns < 0)
					ns = 0;
				sprintf(++p, "%9.9ld", ns);
				p[9] = ' ';	/* '\0' from sprintf() */
			}
#endif
			if (uflag)
				(void) printf("+++ %s	%s\n", input_file2,
				    time_buf);
			else
				(void) printf("--- %s	%s\n", input_file2,
				    time_buf);

			if (context_vec_start == NULL) {
				context_vec_start = (struct context_vec *)
					    malloc(MAX_CONTEXT *
					    sizeof (struct context_vec));
			}
			if (context_vec_start == NULL)
				error(gettext(NO_MEM_ERR));

			context_vec_end = context_vec_start + (MAX_CONTEXT - 1);
			context_vec_ptr = context_vec_start - 1;
		}
	}

	if (opt == D_CONTEXT) {
		/*
		 * if this new change is within 'context' lines of
		 * the previous change, just add it to the change
		 * record.  If the record is full or if this
		 * change is more than 'context' lines from the previous
		 * change, dump the record, reset it & add the new change.
		 */
		if (context_vec_ptr >= context_vec_end ||
		    (context_vec_ptr >= context_vec_start &&
		    a > (context_vec_ptr->b + 2 * context) &&
		    c > (context_vec_ptr->d + 2 * context)))
			dump_context_vec();

		context_vec_ptr++;
		context_vec_ptr->a = a;
		context_vec_ptr->b = b;
		context_vec_ptr->c = c;
		context_vec_ptr->d = d;
		return;
	}

	switch (opt) {
	case D_BRIEF:
		return;
	case D_NORMAL:
	case D_EDIT:
		range(a, b, ",");
		(void) putchar(a > b ? 'a' : c > d ? 'd' : 'c');
		if (opt == D_NORMAL) range(c, d, ",");
		(void) printf("\n");
		break;
	case D_REVERSE:
		(void) putchar(a > b ? 'a' : c > d ? 'd' : 'c');
		range(a, b, " ");
		(void) printf("\n");
		break;
	case D_NREVERSE:
		if (a > b) {
			(void) printf("a%d %d\n", b, d - c + 1);
		} else {
			(void) printf("d%d %d\n", a, b - a + 1);
			if (!(c > d))
				/* add changed lines */
				(void) printf("a%d %d\n", b, d - c + 1);
		}
		break;
	}
	if (opt == D_NORMAL || opt == D_IFDEF) {
		fetch(ixold, a, b, 0, "< ", 1);
		if (a <= b && c <= d && opt == D_NORMAL)
			(void) prints("---\n");
	}
	fetch(ixnew, c, d, 1, opt == D_NORMAL?"> ":empty, 0);
	if ((opt == D_EDIT || opt == D_REVERSE) && c <= d)
		(void) prints(".\n");
	if (inifdef) {
		(void) fprintf(stdout, "#endif /* %s */\n", endifname);
		inifdef = 0;
	}
}

static void
range(a, b, separator)
	int	a;
	int	b;
	char	*separator;
{
	(void) printf("%d", a > b ? b : a);
	if (a < b) {
		(void) printf("%s%d", separator, b);
	}
}

static void
fetch(f, a, b, filen, s, oldfile)
	off_t	*f;
	int	a;
	int	b;
	int	filen;
	char	*s;
	int	oldfile;
{
	int i;
	int col;
	int nc;
	int mlen = 0;
	wint_t	ch;
	FILE	*lb;

	lb = input[filen];
	/*
	 * When doing #ifdef's, copy down to current line
	 * if this is the first file, so that stuff makes it to output.
	 */
	if (opt == D_IFDEF && oldfile) {
		off_t curpos = ftellbuf(filen);
		/* print through if append (a>b), else to (nb: 0 vs 1 orig) */
		nc = f[(a > b) ? b : (a - 1) ] - curpos;
		for (i = 0; i < nc; i += mlen) {
			ch = getbufwchar(filen, &mlen);
			if (ch == WEOF) {
				(void) putchar('\n');
				break;
			} else {
				(void) wcput(ch);
			}
		}
	}
	if (a > b)
		return;
	if (opt == D_IFDEF) {
		int oneflag = (*ifdef1 != '\0') != (*ifdef2 != '\0');
		if (inifdef) {
			(void) fprintf(stdout, "#else /* %s%s */\n",
			    oneflag && oldfile == 1 ? "!" : "", ifdef2);
		} else {
			if (oneflag) {
				/* There was only one ifdef given */
				endifname = ifdef2;
				if (oldfile)
					(void) fprintf(stdout,
					    "#ifndef %s\n", endifname);
				else
					(void) fprintf(stdout,
					    "#ifdef %s\n", endifname);
			} else {
				endifname = oldfile ? ifdef1 : ifdef2;
				(void) fprintf(stdout,
					"#ifdef %s\n", endifname);
			}
		}
		inifdef = 1 + oldfile;
	}

	for (i = a; i <= b; i++) {
		wint_t	lastch;

		if (ftellbuf(filen) != f[i - 1]) {
			/*
			 * Only seek in case we are not at the expected offset.
			 * This is marginally slower with small files but 50%
			 * faster with huge files.
			 */
			(void) fseek(lb, f[i - 1], SEEK_SET);
			initbuf(lb, filen, f[i - 1]);
		}
		if (opt != D_IFDEF)
			(void) prints(s);
		col = 0;
		lastch = '\0';
		while ((ch = getbufwchar(filen, &mlen)) != '\n' && ch != WEOF) {
			if (ch == '\t' && tflag) {
				do
					(void) putchar(' ');
				while (++col & 7);
			} else {
				(void) wcput(ch);
				if (col++ == 0)
					lastch = ch;
			}
		}
		/*
		 * When creating an "ed" script, we cannot directly append a
		 * line that contains only ".\n". We rather enter "..\n", leave
		 * append mode, substitute ".." by "." and re-enter append mode.
		 */
		if (opt == D_EDIT && col == 1 && lastch == '.' && ch == '\n') {
			char	*cp = ".\n.\ns/.//\na";

			while (*cp)
				(void) wcput(*cp++);
		}
		(void) putchar('\n');
	}
}

/*
 * hashing has the effect of
 * arranging line in 7-bit bytes and then
 * summing 1-s complement in 16-bit hunks
 */

static int
readhash(f, filen, str)
	FILE	*f;
	int	filen;
	char	*str;
{
	long sum;
	unsigned int	shift;
	int space;
	int t;
	wint_t	wt;
	int	mlen;

	sum = 1;
	space = 0;
	if (!bflag && !wflag) {
		if (iflag) {
			if (mbcurmax == 1) {
				/* In this case, diff doesn't have to take */
				/* care of multibyte characters. */
				for (shift = 0; (t = getc(f)) != '\n';
					shift += 7) {
					if (t == EOF) {
						if (shift) {
							(void) fprintf(stderr,
	gettext("Warning: missing newline at end of file %s\n"), str);
							break;
						} else {
							return (0);
						}
					}
					sum += (isupper(t) ? tolower(t) : t) <<
						(shift &= HALFMASK);
				}
			} else {
				/* In this case, diff needs to take care of */
				/* multibyte characters. */
				for (shift = 0;
				(wt = getbufwchar(filen, &mlen)) != '\n';
					shift += 7) {
					if (wt == WEOF) {
						if (shift) {
							(void) fprintf(stderr,
	gettext("Warning: missing newline at end of file %s\n"), str);
							break;
						} else {
							return (0);
						}
					}
					sum += NCCHRTRAN(wt) <<
						(shift &= HALFMASK);
				}
			}
		} else {
			/* In this case, diff doesn't have to take care of */
			/* multibyte characters. */
			for (shift = 0; (t = getc(f)) != '\n'; shift += 7) {
				if (t == EOF) {
					if (shift) {
						(void) fprintf(stderr,
	gettext("Warning: missing newline at end of file %s\n"), str);
						break;
					} else {
						return (0);
					}
				}
				sum += (long)t << (shift &= HALFMASK);
			}
		}
	} else {
		/* In this case, diff needs to take care of */
		/* multibyte characters. */
		for (shift = 0; ; ) {
			wt = getbufwchar(filen, &mlen);

			if (wt != '\n' && iswspace(wt)) {
				space++;
				continue;
			} else {
				switch (wt) {
				case WEOF:
					if (shift) {
						(void) fprintf(stderr,
	gettext("Warning: missing newline at end of file %s\n"), str);
						break;
					} else {
						return (0);
					}
				default:
					if (space && !wflag) {
						shift += 7;
						space = 0;
					}
					sum += CHRTRAN(wt) <<
						(shift &= HALFMASK);
					shift += 7;
					continue;
				case '\n':
					break;
				}
			}
			break;
		}
	}
	return (sum);
}


/* dump accumulated "context" diff changes */
static void
dump_context_vec()
{
	int	a, b, c, d;
	char	ch;
	struct	context_vec *cvp = context_vec_start;
	int	lowa, upb, lowc, upd;
	int	do_output;

	if (cvp > context_vec_ptr)
		return;

	/*
	 * Make GCC quiet, we don't need to initialize "b" and "d" as from here
	 * there is a grant for cvp <= context_vec_ptr and thus the two loop
	 * are always entered at least once.
	 */
	b = d = 0;

	lowa = max(1, cvp->a - context);
	upb  = min(len[0], context_vec_ptr->b + context);
	lowc = max(1, cvp->c - context);
	upd  = min(len[1], context_vec_ptr->d + context);

	if (uflag) {
		/*
		 * The POSIX standard likes to see 0,0 for an empty range at
		 * the beginning of a file. We use
		 * 	1,0 for "diff -Nu file /dev/null"
		 * and
		 * 	0,0 for "diff -Nu file non-existent"
		 * which matches the behavior of patch(1) that removes files
		 * only if the range was specified by 0,0.
		 */
		a = b = 0;
		if (file1ok == 0)
			a = 1;
		if (file2ok == 0)
			b = 1;
		(void) printf("@@ -%d,%d +%d,%d @@",
		    lowa > upb ? upb : 		/* Needed for -U0 */
		    lowa - a, upb - lowa + 1,
#ifdef	__symmetric_low__			/* othogonal but wrong */
		    lowc > upd ? upd :
#endif
		    lowc - b, upd - lowc + 1);
		if (pflag) {
			char	*f;

			f = match_function(ixold, lowa-1, 0);
			if (f != NULL)
				(void) printf(" %s", f);
		}
		(void) printf("\n");
	} else {
		(void) printf("***************");
		if (pflag) {
			char	*f;

			f = match_function(ixold, lowa-1, 0);
			if (f != NULL)
				(void) printf(" %s", f);
		}
		(void) printf("\n*** ");
		range(lowa, upb, ",");
		(void) printf(" ****\n");
	}

	/*
	 * output changes to the "old" file.  The first loop suppresses
	 * output if there were no changes to the "old" file (we'll see
	 * the "old" lines as context in the "new" list).
	 */
	if (uflag) {
		do_output = 1;
	} else {
		for (do_output = 0; cvp <= context_vec_ptr; cvp++) {
			if (cvp->a <= cvp->b) {
				cvp = context_vec_start;
				do_output++;
				break;
			}
		}
	}

	if (do_output) {
		while (cvp <= context_vec_ptr) {
			a = cvp->a; b = cvp->b; c = cvp->c; d = cvp->d;

			if (a <= b && c <= d)
				ch = 'c';
			else
				ch = (a <= b) ? 'd' : 'a';

			if (ch == 'a') {
				/* The last argument should not affect */
				/* the behavior of fetch() */
				fetch(ixold, lowa, b, 0, uflag ? " " : "  ", 1);
				if (uflag)
					fetch(ixnew, c, d, 1, "+", 0);
			} else if (ch == 'd') {
				fetch(ixold, lowa, a - 1, 0, uflag ? " " :
				    "  ", 1);
				fetch(ixold, a, b, 0, uflag ? "-" : "- ", 1);
			} else {
				/* The last argument should not affect */
				/* the behavior of fetch() */
				fetch(ixold, lowa, a-1, 0, uflag ? " " : "  ",
				    1);
				if (uflag) {
					fetch(ixold, a, b, 0, "-", 1);
					fetch(ixnew, c, d, 1, "+", 0);
				} else {
					fetch(ixold, a, b, 0, "! ", 1);
				}
			}
			lowa = b + 1;
			cvp++;
		}
		/* The last argument should not affect the behavior */
		/* of fetch() */
		fetch(ixold, b+1, upb, 0, uflag ? " " : "  ", 1);
	}

	if (uflag) {
		context_vec_ptr = context_vec_start - 1;
		return;
	}

	/* output changes to the "new" file */
	(void) printf("--- ");
	range(lowc, upd, ",");
	(void) printf(" ----\n");

	do_output = 0;
	for (cvp = context_vec_start; cvp <= context_vec_ptr; cvp++) {
		if (cvp->c <= cvp->d) {
			cvp = context_vec_start;
			do_output++;
			break;
		}
	}

	if (do_output) {
		while (cvp <= context_vec_ptr) {
			a = cvp->a; b = cvp->b; c = cvp->c; d = cvp->d;

			if (a <= b && c <= d)
				ch = 'c';
			else
				ch = (a <= b) ? 'd' : 'a';

			if (ch == 'd') {
				/* The last argument should not affect */
				/* the behavior of fetch() */
				fetch(ixnew, lowc, d, 1, "  ", 0);
			} else {
				/* The last argument should not affect */
				/* the behavior of fetch() */
				fetch(ixnew, lowc, c - 1, 1, "  ", 0);
				fetch(ixnew, c, d, 1,
				    ch == 'c' ? "! " : "+ ", 0);
			}
			lowc = d + 1;
			cvp++;
		}
		/* The last argument should not affect the behavior */
		/* of fetch() */
		fetch(ixnew, d + 1, upd, 1, "  ", 0);
	}
	context_vec_ptr = context_vec_start - 1;
}



/*
 * diff - directory comparison
 */
int	header;
char	title[2 * BUFSIZ], *etitle;

static void
diffdir(argv, pdirs)
	char		**argv;
	struct pdirs	*pdirs;
{
	struct dir *d1, *d2;
	struct dir *dir1, *dir2;
	int i;
	int cmp;
	int result, dirstatus;

	if (opt == D_IFDEF)
		error(gettext("cannot specify -D with directories"));

	if (opt == D_EDIT && (sflag || lflag)) {
		(void) fprintf(stderr, "diff: ");
		(void) fprintf(stderr, gettext(
			"warning: should not give -s or -l with -e\n"));
	}
	dirstatus = 0;
	title[0] = 0;
	(void) strlcpy(title, "diff ", sizeof (title));
	for (i = 1; diffargv[i + 2]; i++) {
		if (strcmp(diffargv[i], "-") == 0) {
			continue;	/* Skip -S and its argument */
		}
		(void) strlcat(title, diffargv[i], sizeof (title));
		(void) strlcat(title, " ", sizeof (title));
	}
	for (etitle = title; *etitle; etitle++)
		;
	setfile(&file1, &efile1, file1);
	setfile(&file2, &efile2, file2);
	argv[0] = file1;
	argv[1] = file2;
	dir1 = setupdir(file1);
	dir2 = setupdir(file2);
	d1 = dir1; d2 = dir2;
	while (d1->d_entry != 0 || d2->d_entry != 0) {
		if (d1->d_entry && useless(d1->d_entry)) {
			d1++;
			continue;
		}
		if (d2->d_entry && useless(d2->d_entry)) {
			d2++;
			continue;
		}
		if (d1->d_entry == 0)
			cmp = 1;
		else if (d2->d_entry == 0)
			cmp = -1;
		else
			cmp = strcmp(d1->d_entry, d2->d_entry);

		file1ok = file2ok = 1;
		if (cmp < 0) {
			if (Nflag) {
				result = compare(d1);
				if (result > dirstatus)
					dirstatus = result;
			} else if (lflag) {
				d1->d_flags |= ONLY;
			} else if (opt == D_NORMAL || opt == D_CONTEXT ||
			    opt == D_BRIEF) {
				only(d1, 1);
			}
			if (d1->d_entry)
				d1++;
			if (!Nflag && dirstatus == 0)
				dirstatus = 1;
		} else if (cmp == 0) {
			result = compare(d1);
			if (result > dirstatus)
				dirstatus = result;
			if (d1->d_entry)
				d1++;
			if (d2->d_entry)
				d2++;
		} else {
			if (Nflag) {
				result = compare(d2);
				if (result > dirstatus)
					dirstatus = result;
				if (d2->d_flags & DIRECT)
					d2->d_flags |= XDIRECT;
			} else if (lflag) {
				d2->d_flags |= ONLY;
			} else if (opt == D_NORMAL || opt == D_CONTEXT) {
				only(d2, 2);
			}
			if (d2->d_entry)
				d2++;
			if (!Nflag && dirstatus == 0)
				dirstatus = 1;
		}
	}
	if (lflag) {
		scanpr(dir1, ONLY,
			gettext("Only in %.*s"), file1, efile1, 0, 0);
		scanpr(dir2, ONLY,
			gettext("Only in %.*s"), file2, efile2, 0, 0);
		scanpr(dir1, SAME,
		    gettext("Common identical files in %.*s and %.*s"),
		    file1, efile1, file2, efile2);
		scanpr(dir1, DIFFER,
		    gettext("Binary files which differ in %.*s and %.*s"),
		    file1, efile1, file2, efile2);
		scanpr(dir1, DIRECT,
		    gettext("Common subdirectories of %.*s and %.*s"),
		    file1, efile1, file2, efile2);
	}
	if (rflag) {
		struct pdirs	pdir;

		pdir.p_last = pdirs;

		if (header && lflag)
			(void) printf("\f");
		for (d1 = dir1; d1->d_entry; d1++) {
			if ((d1->d_flags & DIRECT) == 0)
				continue;
			(void) strcpy(efile1, d1->d_entry);
			(void) strcpy(efile2, d1->d_entry);
			pdir.p_dev1 = d1->d_dev1;
			pdir.p_dev2 = d1->d_dev2;
			pdir.p_ino1 = d1->d_ino1;
			pdir.p_ino2 = d1->d_ino2;
			calldiffdir(argv, &pdir);
		}
		if (Nflag) {
			for (d2 = dir2; d2->d_entry; d2++) {
				if ((d2->d_flags & XDIRECT) == 0)
						continue;
				(void) strcpy(efile1, d2->d_entry);
				(void) strcpy(efile2, d2->d_entry);
				pdir.p_dev1 = d2->d_dev1;
				pdir.p_dev2 = d2->d_dev2;
				pdir.p_ino1 = d2->d_ino1;
				pdir.p_ino2 = d2->d_ino2;
				calldiffdir(argv, &pdir);
			}
		}
	}
	for (d1 = dir1; d1->d_entry; d1++)
		free(d1->d_entry);
	for (d2 = dir2; d2->d_entry; d2++)
		free(d2->d_entry);
	free(dir1);
	free(dir2);
	if (dirstatus > status)
		status = dirstatus;
}

static void
calldiffdir(argv, pdirs)
	char		**argv;
	struct pdirs	*pdirs;
{
	char	*f1 = file1;
	char	*f2 = file2;
	char	*ef1 = efile1;
	char	*ef2 = efile2;
	struct pdirs	*pd = pdirs->p_last;

#ifndef	DIRCMP_DEBUG
	/*
	 * We do not need to compare identical directories.
	 */
	if (pdirs->p_dev1 == pdirs->p_dev2 &&
	    pdirs->p_ino1 == pdirs->p_ino2)
		return;
#endif
	while (pd) {
		if (pdirs->p_dev1 == pd->p_dev1 &&
		    pdirs->p_dev2 == pd->p_dev2 &&
		    pdirs->p_ino1 == pd->p_ino1 &&
		    pdirs->p_ino2 == pd->p_ino2)
			return;
		pd = pd->p_last;
	}

	diffdir(argv, pdirs);
	free(file1);
	free(file2);
	file1 = f1;
	file2 = f2;
	efile1 = ef1;
	efile2 = ef2;
	argv[0] = file1;
	argv[1] = file2;
}

static void
setfile(fpp, epp, filen)
	char	**fpp;
	char	**epp;
	char	*filen;
{
	char *cp;

	*fpp = (char *)malloc(BUFSIZ);
	if (*fpp == 0) {
		(void) fprintf(stderr, "diff: ");
		(void) fprintf(stderr, gettext("out of memory\n"));
		exit(1);
	}
	(void) strcpy(*fpp, filen);
	for (cp = *fpp; *cp; cp++)
		continue;
	*cp++ = '/';
	*cp = 0;
	*epp = cp;
}

static void
scanpr(dp, test, titlen, file1n, efile1n, file2n, efile2n)
	struct dir	*dp;
	int		test;
	char		*titlen;
	char		*file1n;
	char		*efile1n;
	char		*file2n;
	char		*efile2n;
{
	int titled = 0;

	for (; dp->d_entry; dp++) {
		if ((dp->d_flags & test) == 0)
			continue;
		if (titled == 0) {
			if (header == 0)
				header = 1;
			else
				(void) printf("\n");
			(void) printf(titlen,
			    efile1n - file1n - 1, file1n,
			    efile2n - file2n - 1, file2n);
			(void) printf(":\n");
			titled = 1;
		}
		(void) printf("\t%s\n", dp->d_entry);
	}
}

static void
only(dp, which)
	struct dir	*dp;
	int		which;
{
	char *filen = which == 1 ? file1 : file2;
	char *efilen = which == 1 ? efile1 : efile2;

	(void) printf(gettext("Only in %.*s: %s\n"),
	    (int)(efilen - filen - 1), filen,
	    dp->d_entry);
}

static struct dir *
setupdir(cp)
	char	*cp;
{
	struct dir *dp = 0, *ep;
	struct dirent *rp;
	int nitems;
	int	dplen;
	int size;
	DIR *dirp;

	dirp = opendir(cp);
	if (Nflag && dirp == NULL) {
		dp = (struct dir *)malloc(sizeof (struct dir));
		if (dp == 0)
			error(gettext(NO_MEM_ERR));
		dp[0].d_entry = 0;		/* delimiter */
		return (dp);
	}
	if (dirp == NULL) {
		(void) fprintf(stderr, "diff: ");
		perror(cp);
		status = 2;
		done();
	}
	nitems = 0;
	dplen = 0;
	dp = (struct dir *)malloc(sizeof (struct dir));
	if (dp == 0)
		error(gettext(NO_MEM_ERR));

	while ((rp = readdir(dirp)) != NULL) {
		ep = &dp[nitems++];
		ep->d_entry = 0;
		ep->d_flags = 0;
		ep->d_dev1 = 0;
		ep->d_dev2 = 0;
		ep->d_ino1 = 0;
		ep->d_ino2 = 0;
		size = strlen(rp->d_name);
		if (size > 0) {
			ep->d_entry = (char *)malloc(size + 1);
			if (ep->d_entry == 0)
				error(gettext(NO_MEM_ERR));

			(void) strcpy(ep->d_entry, rp->d_name);
		}
		if (nitems >= dplen) {
			dplen += 64;
			dp = (struct dir *)realloc((char *)dp,
				(dplen + 1) * sizeof (struct dir));
		}
		if (dp == 0)
			error(gettext(NO_MEM_ERR));
	}
	dp[nitems].d_entry = 0;		/* delimiter */
	(void) closedir(dirp);
	qsort(dp, nitems, sizeof (struct dir),
		(int (*) __PR((const void *, const void *)))entcmp);
	return (dp);
}

static int
entcmp(d1, d2)
	struct dir	*d1;
	struct dir	*d2;
{
	return (strcmp(d1->d_entry, d2->d_entry));
}

static int
compare(dp)
	struct dir	*dp;
{
	int i, j;
	int f1 = -1, f2 = -1;
	mode_t fmt1, fmt2;
	struct stat statb1, statb2;
	char buf1[D_BUFSIZ], buf2[D_BUFSIZ];
	int result = 0;

	(void) strcpy(efile1, dp->d_entry);
	(void) strcpy(efile2, dp->d_entry);

	if (stat(file1, &statb1) == -1) {
		if (errno == ENOENT && Nflag) {
			statb1.st_mode = 0;
			statb1.st_dev = 0;
			statb1.st_ino = 0;
			result = 1;
			file1ok = 0;
		} else {
			(void) fprintf(stderr, "diff: ");
			perror(file1);
			return (2);
		}
	}
	if (stat(file2, &statb2) == -1) {
		if (errno == ENOENT && Nflag) {
			statb2.st_mode = 0;
			statb2.st_dev = 0;
			statb2.st_ino = 0;
			result = 1;
			file2ok = 0;
		} else {
			(void) fprintf(stderr, "diff: ");
			perror(file2);
			return (2);
		}
	}

	fmt1 = statb1.st_mode & S_IFMT;
	fmt2 = statb2.st_mode & S_IFMT;

	if (fmt1 == S_IFREG) {
		f1 = open(file1, O_RDONLY);
	} else if (file1ok == 0) {
		f1 = open(nulldev, O_RDONLY);
	}
	if ((fmt1 == S_IFREG || file1ok == 0) && f1 < 0) {
		(void) fprintf(stderr, "diff: ");
		perror(file1);
		return (2);
	}

	if (fmt2 == S_IFREG) {
		f2 = open(file2, O_RDONLY);
	} else if (file2ok == 0) {
		f2 = open(nulldev, O_RDONLY);
	}
	if ((fmt2 == S_IFREG || file2ok == 0) && f2 < 0) {
		(void) fprintf(stderr, "diff: ");
		perror(file2);
		(void) close(f1);
		return (2);
	}

	if (result) {
		if (fmt1 == S_IFDIR || fmt2 == S_IFDIR)
			fmt1 = fmt2 = S_IFDIR;
		if (fmt1 == S_IFREG || fmt2 == S_IFREG)
			goto notsame;
	}

	if (fmt1 != S_IFREG || fmt2 != S_IFREG) {
		if (fmt1 == fmt2) {
			switch (fmt1) {

			case S_IFDIR:
				dp->d_flags = DIRECT;
				dp->d_dev1 = statb1.st_dev;
				dp->d_dev2 = statb2.st_dev;
				dp->d_ino1 = statb1.st_ino;
				dp->d_ino2 = statb2.st_ino;
				if (lflag || opt == D_EDIT)
					goto closem;
				if (Nflag && rflag)
					goto closem;
				(void) printf(gettext(
				    "Common subdirectories: %s and %s\n"),
				    file1, file2);
				goto closem;

#if	defined(S_IFCHR) || defined(S_IFBLK)
			case S_IFCHR:
#ifdef	S_IFBLK
			case S_IFBLK:
#endif
				if (statb1.st_rdev == statb2.st_rdev)
					goto same;
				(void) printf(gettext(
				    "Special files %s and %s differ\n"),
				    file1, file2);
				break;
#endif

#ifdef	S_IFLNK
			case S_IFLNK:
				if ((i = readlink(file1, buf1,
							D_BUFSIZ)) == -1) {
					(void) fprintf(stderr, gettext(
					    "diff: cannot read link\n"));
					return (2);
				}

				if ((j = readlink(file2, buf2,
							D_BUFSIZ)) == -1) {
					(void) fprintf(stderr, gettext(
					    "diff: cannot read link\n"));
					return (2);
				}

				if (i == j) {
					if (strncmp(buf1, buf2, i) == 0)
						goto same;
				}

				(void) printf(gettext(
				    "Symbolic links %s and %s differ\n"),
				    file1, file2);
				break;
#endif

#ifdef	S_IFIFO
			case S_IFIFO:
				if (statb1.st_ino == statb2.st_ino)
					goto same;
				(void) printf(gettext(
				    "Named pipes %s and %s differ\n"),
				    file1, file2);
				break;
#endif
			}
		} else {
			if (lflag) {
				dp->d_flags |= DIFFER;
			} else if (opt == D_NORMAL || opt == D_CONTEXT) {
/*
 * TRANSLATION_NOTE
 * The second and fourth parameters will take the gettext'ed string
 * of one of the following:
 * a directory
 * a character special file
 * a block special file
 * a plain file
 * a named pipe
 * a socket
 * a door
 * an event port
 * an unknown type
 */
				(void) printf(
gettext("File %s is %s while file %s is %s\n"),
					file1, pfiletype(fmt1),
					file2, pfiletype(fmt2));
			}
		}
		(void) close(f1); (void) close(f2);
		return (1);
	}
	if (statb1.st_size != statb2.st_size)
		goto notsame;
	for (;;) {
		i = read(f1, buf1, D_BUFSIZ);
		j = read(f2, buf2, D_BUFSIZ);
		if (i < 0 || j < 0) {
			(void) fprintf(stderr, "diff: ");
			(void) fprintf(stderr, gettext("Error reading "));
			perror(i < 0 ? file1: file2);
			(void) close(f1); (void) close(f2);
			return (2);
		}
		if (i != j)
			goto notsame;
		if (i == 0 && j == 0)
			goto same;
#ifdef	HAVE_MEMCMP
		if (memcmp(buf1, buf2, i))
			goto notsame;
#else
		for (j = 0; j < i; j++)
			if (buf1[j] != buf2[j])
				goto notsame;
#endif
	}
same:
	if (sflag == 0)
		goto closem;
	if (lflag)
		dp->d_flags = SAME;
	else
		(void) printf(gettext("Files %s and %s are identical\n"),
			file1, file2);

closem:
	(void) close(f1); (void) close(f2);
	return (0);

notsame:
	if (!aflag &&
	    (binary(f1) || binary(f2))) {
		if (lflag)
			dp->d_flags |= DIFFER;
		else if (opt == D_NORMAL || opt == D_CONTEXT)
			(void) printf(
				gettext("Binary files %s and %s differ\n"),
			    file1, file2);
		(void) close(f1); (void) close(f2);
		return (1);
	}
#ifdef	DO_SPAWN_DIFF
	(void) close(f1); (void) close(f2);
#endif
	anychange = 1;
	if (lflag) {
#ifndef	DO_SPAWN_DIFF
		(void) close(f1); (void) close(f2);
#endif
		result = calldiff(title);
	} else {
		if (opt == D_EDIT)
			(void) printf("ed - %s << '-*-END-*-'\n", dp->d_entry);
		else
			(void) printf("%s%s %s\n", title, file1, file2);
#ifdef	DO_SPAWN_DIFF
		result = calldiff((char *)0);
#else
		stb1 = statb1;
		stb2 = statb2;
		result = calldiffreg(f1, f2);
#endif
		if (opt == D_EDIT)
			(void) printf("w\nq\n-*-END-*-\n");
	}
	return (result);
}

char	*prargs[] = { "pr", "-h", 0, 0, 0 };

static int
calldiff(wantpr)
	char	*wantpr;
{
	pid_t pid;
	int diffstatus, pv[2];

	prargs[2] = wantpr;
	(void) fflush(stdout);
	if (wantpr) {
		(void) sprintf(etitle, "%s %s", file1, file2);
		(void) pipe(pv);
		pid = vfork();
		if (pid == (pid_t)-1)
			error(gettext(NO_PROCS_ERR));

		if (pid == 0) {
#ifdef	set_child_standard_fds	/* VMS */
			set_child_standard_fds(pv[0],
						STDOUT_FILENO,
						STDERR_FILENO);
#ifdef	F_SETFD
			fcntl(pv[1], F_SETFD, 1);
#endif
#else			/* ! VMS, below is the code for UNIX */
			(void) dup2(pv[0], STDIN_FILENO);
			(void) close(pv[0]);
			(void) close(pv[1]);
#endif
			(void) execv(pr+4, prargs);
			(void) execv(pr, prargs);
			perror(pr);
#ifdef	HAVE_VFORK
			didvfork = 1;
#endif
			status = 2;
			done();
		}
	}
	pid = vfork();
	if (pid == (pid_t)-1)
		error(gettext(NO_PROCS_ERR));

	if (pid == 0) {
		if (wantpr) {
			(void) dup2(pv[1], STDOUT_FILENO);
			(void) close(pv[0]);
			(void) close(pv[1]);
		}
#ifdef	HAVE_GETEXECNAME
		(void) execv(getexecname(), diffargv);
#endif
		(void) execv(diff+4, diffargv);
		(void) execv(diff, diffargv);
		perror(diff);
#ifdef	HAVE_VFORK
		didvfork = 1;
#endif
		status = 2;
		done();
	}
	if (wantpr) {
		(void) close(pv[0]);
		(void) close(pv[1]);
	}
	while (wait(&diffstatus) != pid)
		continue;
	while (wait((int *)0) != (pid_t)-1)
		continue;
	if (WIFEXITED(diffstatus))
		return (WEXITSTATUS(diffstatus));
	else
		return (2);
}

#ifdef	PROTOTYPES
static char *
pfiletype(mode_t fmt)
#else
static char *
pfiletype(fmt)
	mode_t	fmt;
#endif
{
/*
 * TRANSLATION_NOTE
 * The following 9 messages will be used in the second and
 * the fourth parameters of the message
 * "File %s is %s while file %s is %s\n"
 */
	switch (fmt) {

	case S_IFDIR:
		return (gettext("a directory"));

#ifdef	S_IFCHR
	case S_IFCHR:
		return (gettext("a character special file"));
#endif

#ifdef	S_IFBLK
	case S_IFBLK:
		return (gettext("a block special file"));
#endif

	case S_IFREG:
		return (gettext("a plain file"));

#ifdef	S_IFIFO
	case S_IFIFO:
		return (gettext("a named pipe"));
#endif

#ifdef	S_IFSOCK
	case S_IFSOCK:
		return (gettext("a socket"));
#endif

#ifdef	S_IFDOOR
	case S_IFDOOR:
		return (gettext("a door"));
#endif

#ifdef	S_IFPORT
	case S_IFPORT:
		return (gettext("an event port"));
#endif

	default:
		return (gettext("an unknown type"));
	}
}

static int
binary(f)
	int	f;
{
	char buf[D_BUFSIZ];
	int cnt;

	if (f < 0)
		return (0);
	(void) lseek(f, (off_t)0, SEEK_SET);
	cnt = read(f, buf, D_BUFSIZ);
	if (cnt < 0)
		return (1);
	return (isbinary(buf, cnt));
}

static int
filebinary(f)
	FILE	*f;
{
	char buf[D_BUFSIZ];
	int cnt;

	(void) fseek(f, (off_t)0, SEEK_SET);
	cnt = fread(buf, 1, D_BUFSIZ, f);
	if (ferror(f))
		return (1);
	return (isbinary(buf, cnt));
}


/*
 * We consider a "binary" file to be one that:
 * contains a null character ("diff" doesn't handle them correctly, and
 *    neither do many other UNIX text-processing commands).
 * Characters with their 8th bit set do NOT make a file binary; they may be
 * legitimate text characters, or parts of same.
 */
static int
isbinary(buf, cnt)
	char	*buf;
	int	cnt;
{
	char *cp;

	cp = buf;
	while (--cnt >= 0)
		if (*cp++ == '\0')
			return (1);
	return (0);
}


/*
 * THIS IS CRUDE.
 */
static int
useless(cp)
	char	*cp;
{

	if (cp[0] == '.') {
		if (cp[1] == '\0')
			return (1);	/* directory "." */
		if (cp[1] == '.' && cp[2] == '\0')
			return (1);	/* directory ".." */
	}
	if (start && strcmp(start, cp) > 0)
		return (1);
	return (0);
}


static void
sort(a, n)				/* shellsort CACM #201 */
	struct line	*a;
	int		n;
{
	struct line w;
	int j, m;
	struct line *ai;
	struct line *aim;
	int k;

	for (j = 1, m = 0; j <= n; j *= 2)
		m = 2 * j - 1;
	for (m /= 2; m != 0; m /= 2) {
		k = n - m;
		for (j = 1; j <= k; j++) {
			for (ai = &a[j]; ai > a; ai -= m) {
				aim = &ai[m];
				if (aim < ai)
					break;	/* wraparound */
				if (aim->value > ai[0].value ||
				    (aim->value == ai[0].value &&
				    aim->serial > ai[0].serial))
					break;
				w.value = ai[0].value;
				ai[0].value = aim->value;
				aim->value = w.value;
				w.serial = ai[0].serial;
				ai[0].serial = aim->serial;
				aim->serial = w.serial;
			}
		}
	}
}

static void
unsort(f, l, b)
	struct line	*f;
	int		l;
	int		*b;
{
	int *a;
	int i;

	a = (int *)talloc((l + 1) * sizeof (int));
	for (i = 1; i <= l; i++)
		a[f[i].serial] = f[i].value;
	for (i = 1; i <= l; i++)
		b[i] = a[i];
	free((char *)a);
}

static void
filename(pa1, pa2, st, ifile)
	char		**pa1;
	char		**pa2;
	struct stat	*st;
	char		**ifile;
{
	char *a1, *b1, *a2;

	a1 = *pa1;
	a2 = *pa2;

	if (*ifile)
		free(*ifile);

	if (strcmp(*pa1, "-") == 0)
		*ifile = strdup("-");
	else
		*ifile = strdup(*pa1);

	if (*ifile == (char *)NULL) {
		(void) fprintf(stderr, gettext(
			"no more memory - try again later\n"));
		status = 2;
		done();
	}

	if ((st->st_mode & S_IFMT) == S_IFDIR) {
		b1 = *pa1 = (char *)malloc(PATH_MAX);
		if (b1 == (char *)NULL) {
			(void) fprintf(stderr, gettext(
				"no more memory - try again later\n"));
			status = 2;
			done();
		}
		while ((*b1++ = *a1++) != '\0')
			;
		b1[-1] = '/';
		a1 = b1;
		while ((*a1++ = *a2++) != '\0')
			if (*a2 && *a2 != '/' && a2[-1] == '/')
				a1 = b1;
		free(*ifile);
		*ifile = strdup(*pa1);

		if (*ifile == (char *)NULL) {
			(void) fprintf(stderr, gettext(
				"no more memory - try again later\n"));
			status = 2;
			done();
		}

		if (stat(*pa1, st) < 0) {
			(void) fprintf(stderr, "diff: ");
			perror(*pa1);
			status = 2;
			done();
		}
	} else if ((st->st_mode & S_IFMT) == S_IFCHR) {
		*pa1 = copytemp(a1);
	} else if (a1[0] == '-' && a1[1] == 0) {
		*pa1 = copytemp(a1);	/* hack! */
		if (stat(*pa1, st) < 0) {
			(void) fprintf(stderr, "diff: ");
			perror(*pa1);
			status = 2;
			done();
		}
	}
}

static char *
copytemp(fn)
	char	*fn;
{
	int ifd, ofd;	/* input and output file descriptors */
	int i;
	char template[13];	/* template for temp file name */
	char buf[D_BUFSIZ];

	/*
	 * a "-" file is interpreted as fd 0 for pre-/dev/fd systems
	 * ... let's hope this goes away soon!
	 */
	if ((ifd = (strcmp(fn, "-") ? open(fn, 0) : 0)) < 0) {
		(void) fprintf(stderr, "diff: ");
		(void) fprintf(stderr, gettext("cannot open %s\n"), fn);
		done();
	}
#ifdef	SIGHUP
	(void) signal(SIGHUP, (void (*) __PR((int)))done);
#endif
#ifdef	SIGINT
	(void) signal(SIGINT, (void (*) __PR((int)))done);
#endif
#ifdef	SIGPIPE
	(void) signal(SIGPIPE, (void (*) __PR((int)))done);
#endif
#ifdef	SIGTERM
	(void) signal(SIGTERM, (void (*) __PR((int)))done);
#endif
	(void) strcpy(template, "/tmp/dXXXXXX");
	if ((ofd = mkstemp(template)) < 0) {
		(void) fprintf(stderr, "diff: ");
		(void) fprintf(stderr, gettext("cannot create %s\n"), template);
		done();
	}
	(void) strcpy(tempfile[whichtemp++], template);
	while ((i = read(ifd, buf, D_BUFSIZ)) > 0)
		if (write(ofd, buf, i) != i) {
			(void) fprintf(stderr, "diff: ");
			(void) fprintf(stderr,
				gettext("write failed %s\n"), template);
			done();
		}
	(void) close(ifd); (void) close(ofd);
	return (tempfile[whichtemp-1]);
}

static void
prepare(i, arg)
	int	i;
	char	*arg;
{
	struct line *p;
	int j, h;
	size_t	isize;

	/*
	 * Average line length is aprox. 35 chars.
	 */
	isize = (i == 0 ? stb1.st_size : stb2.st_size) / 35;
	if (isize > 1000000)
		isize = 1000000;
	if (isize < 64)
		isize = 64;

	(void) fseek(input[i], (off_t)0, SEEK_SET);
	p = (struct line *)talloc((isize + 3) * sizeof (line));
	for (j = 0; (h = readhash(input[i], i, arg)) != 0; ) {
		if (j >= isize) {
			isize += 64;
			p = (struct line *)ralloc((void *)p,
					(isize + 3) * sizeof (line));
		}
		p[++j].value = h;
	}
	len[i] = j;
	file[i] = p;
}

static void
prune()
{
	int i, j;

	for (pref = 0; pref < len[0] && pref < len[1] &&
	    file[0][pref + 1].value == file[1][pref + 1].value;
	    pref++)
		;
	for (suff = 0; (suff < len[0] - pref) &&
	    (suff < len[1] - pref) &&
	    (file[0][len[0] - suff].value ==
	    file[1][len[1] - suff].value);
	    suff++)
		;

	/* decremnt suff by 2 iff suff >= 2, ensure that suff is never < 0 */
	if (suff >= 2)
		suff -= 2;

	for (j = 0; j < 2; j++) {
		sfile[j] = file[j] + pref;
		slen[j] = len[j] - pref - suff;
		for (i = 0; i <= slen[j]; i++)
			sfile[j][i].serial = i;
	}
}

static void
equiv(a, n, b, m, c)
	struct line	*a;
	int		n;
	struct line	*b;
	int		m;
	int		*c;
{
	int i, j;
	i = j = 1;
	while (i <= n && j <= m) {
		if (a[i].value < b[j].value)
			a[i++].value = 0;
		else if (a[i].value == b[j].value)
			a[i++].value = j;
		else
			j++;
	}
	while (i <= n)
		a[i++].value = 0;
	b[m+1].value = 0;	j = 0;
	while (++j <= m) {
		c[j] = -b[j].serial;
		while (b[j + 1].value == b[j].value) {
			j++;
			c[j] = b[j].serial;
		}
	}
	c[j] = -1;
}

static void
done()
{
	if (whichtemp) (void) unlink(tempfile[0]);
	if (whichtemp == 2) (void) unlink(tempfile[1]);
	if (didvfork)
		_exit(status);
	exit(status);
}

static void
noroom()
{
	(void) fprintf(stderr, "diff: ");
	(void) fprintf(stderr, gettext("files too big, try -h\n"));
	done();
}

static void
error(s)
	const char *s;
{
	(void) fprintf(stderr, "diff: ");
	(void) fprintf(stderr, "%s", s);
	(void) fprintf(stderr, "\n");
	done();
}

static void
usage()
{
	(void) fprintf(stderr, gettext(
	"usage: diff [-abBiNptw] [-c | -e | -f | -h | -n | -q | -u] \
file1 file2\n\
       diff [-abBiNptw] [-C number | -U number] file1 file2\n\
       diff [-abBiNptw] [-D string] file1 file2\n\
       diff [-abBiNptw] [-c | -e | -f | -h | -n | -q | -u] [-l] [-r] \
[-s] [-S name] directory1 directory2\n"));
	status = 2;
	done();
}

#define	NW	1024
struct buff	{
	FILE	*iop;			/* I/O stream */
	char	buf[NW + MB_LEN_MAX];	/* buffer */
	char	*ptr;			/* current pointer in the buffer */
	int	buffered;		/* if non-zero, buffer has data */
	off_t	offset;			/* offset in the file */
};

static struct buff bufwchar[2];

/*
 *	Initializes the buff structure for specified
 *	I/O stream.  Also sets the specified offset
 */
static void
initbuf(iop, filen, offset)
	FILE	*iop;
	int	filen;
	off_t	offset;
{
	bufwchar[filen].iop = iop;
	bufwchar[filen].ptr = NULL;
	bufwchar[filen].buffered = 0;
	bufwchar[filen].offset = offset;
}

/*
 * 	Reset a buff structure, and rewind the associated file.
 */
static void
resetbuf(filen)
	int	filen;
{
	bufwchar[filen].ptr = NULL;
	bufwchar[filen].buffered = bufwchar[filen].offset = 0;
	rewind(bufwchar[filen].iop);
}


/*
 *	Returns the current offset in the file
 */
static off_t
ftellbuf(filen)
	int	filen;
{
	return (bufwchar[filen].offset);
}

static wint_t
wcput(wc)
	wint_t	wc;
{
	char	mbs[MB_LEN_MAX];
	unsigned char	*p;
	int	n;

	n = wctomb(mbs, (wchar_t)wc);
	if (n > 0) {
		p = (unsigned char *)mbs;
		while (n--) {
			(void) putc((*p++), stdout);
		}
		return (wc);
	} else if (n < 0) {
		(void) putc((int)(wc & 0xff), stdout);
		return (wc & 0xff);
	} else {
		/* this should not happen */
		return (WEOF);
	}
}

/*
 *	Reads one wide-character from the file associated with filen.
 *	If multibyte locales, the input is buffered.
 *
 *	Input:	filen	the file number (0 or 1)
 *	Output:	*blen	number of bytes to make wide-character
 *	Return:			wide-character
 */
static wint_t
getbufwchar(filen, blen)
	int	filen;
	int	*blen;
{

	int	i, num, chlen;
	wchar_t	wc;
	size_t	mxlen;

	if (mbcurmax == 1) {
		/* If sigle byte locale, use getc() */
		int	ch;

		ch = getc(bufwchar[filen].iop);
		bufwchar[filen].offset++;
		*blen = 1;

		if (isascii(ch) || (ch == EOF)) {
			return ((wint_t)ch);
		} else {
			wchar_t	wch;
			char	str[2];

			str[0] = (char)ch;
			str[1] = '\0';
			if (mbtowc(&wch, str, 1) > 0) {
				return ((wint_t)wch);
			} else {
				return ((wint_t)ch);
			}
		}
	} else {
		mxlen = mbcurmax;
	}

	if (bufwchar[filen].buffered == 0) {
		/* Not buffered */
		bufwchar[filen].ptr = &(bufwchar[filen].buf[MB_LEN_MAX]);
		num = fread((void *)bufwchar[filen].ptr,
			sizeof (char), NW, bufwchar[filen].iop);
		if (ferror(bufwchar[filen].iop)) {
			(void) fprintf(stderr, "diff: ");
			(void) fprintf(stderr, gettext("Error reading "));
			perror((filen == 0) ? file1 : file2);
			status = 2;
			done();
		}
		if (num == 0)
			return (WEOF);
		bufwchar[filen].buffered = num;
	}

	if (bufwchar[filen].buffered < mbcurmax) {
		for (i = 0; i < bufwchar[filen].buffered; i++) {
			bufwchar[filen].buf[MB_LEN_MAX -
				(bufwchar[filen].buffered - i)] =
				*(bufwchar[filen].ptr + i);
		}
		bufwchar[filen].ptr = &(bufwchar[filen].buf[MB_LEN_MAX]);
		num = fread((void *)bufwchar[filen].ptr,
			sizeof (char), NW, bufwchar[filen].iop);
		if (ferror(bufwchar[filen].iop)) {
			(void) fprintf(stderr, "diff: ");
			(void) fprintf(stderr, gettext("Error reading "));
			perror((filen == 0) ? file1 : file2);
			status = 2;
			done();
		}
		bufwchar[filen].ptr = &(bufwchar[filen].buf[MB_LEN_MAX -
				bufwchar[filen].buffered]);
		bufwchar[filen].buffered += num;
		if (bufwchar[filen].buffered < mbcurmax) {
			mxlen = bufwchar[filen].buffered;
		}
	}

	chlen = mbtowc(&wc, bufwchar[filen].ptr, mxlen);
	if (chlen <= 0) {
		(bufwchar[filen].buffered)--;
		*blen = 1;
		(bufwchar[filen].offset)++;
		wc = (wchar_t)((unsigned char)*bufwchar[filen].ptr++);
		return ((wint_t)wc);
	} else {
		bufwchar[filen].buffered -= chlen;
		bufwchar[filen].ptr += chlen;
		bufwchar[filen].offset += chlen;
		*blen = chlen;
		return ((wint_t)wc);
	}
}

#if !defined(HAVE_CFTIME) && defined(HAVE_STRFTIME)
static time_t
gmtoff(clk)
	const time_t	*clk;
{
	struct tm	local;
	struct tm	gmt;
	time_t		crtime;

	local = *localtime(clk);
	gmt   = *gmtime(clk);

	local.tm_sec  -= gmt.tm_sec;
	local.tm_min  -= gmt.tm_min;
	local.tm_hour -= gmt.tm_hour;
	local.tm_yday -= gmt.tm_yday;
	local.tm_year -= gmt.tm_year;
	if (local.tm_year)		/* Hit new-year limit	*/
		local.tm_yday = local.tm_year;	/* yday = +-1	*/

	crtime = local.tm_sec + 60 *
		    (local.tm_min + 60 *
			(local.tm_hour + 24 * local.tm_yday));

	return (crtime);
}
#endif

#define	DO2(p, n, c)	*p++ = ((char)((n)/10) + '0'); *p++ = \
					((char)((n)%10) + '0'); *p++ = c;

#define	DO2_(p, n)	*p++ = ((char)((n)/10) + '0'); *p++ = \
					((char)((n)%10) + '0');

static void
cf_time(s, maxsize, fmt, clk)
	char		*s;
	size_t		maxsize;
	char		*fmt;
	const time_t	*clk;
{
#ifdef	HAVE_CFTIME
	cftime(s, fmt, clk);
#else
#ifdef	HAVE_STRFTIME
	struct	tm	*tp = localtime(clk);
	char		*p;

	strftime(s, maxsize, fmt, tp);
	/*
	 * HP/UX implements %z as %Z and we need to correct this...
	 */
	p = fmt + strlen(fmt);
	if (*--p != 'z')
		return;
	p = strrchr(s, ' ');
	if (p++) {
		if (*p != '+' && *p != '-') {
			register int	z;
			register int	n;

			z = gmtoff(clk) / 60;	/* seconds -> minutes */
			if (z < 0) {
				*p++ = '-';
				z = -z;
			} else {
				*p++ = '+';
			}
			n = z / 60;
			DO2_(p, n);
			n = z % 60;
			DO2(p, n, 0);
		}
	}
#else
	/*
	 * This is not the correct time format, but we are not on a POSIX
	 * platform and need to do the best we can.
	 */
	strlcpy(s, ctime(clk), maxsize);
	if (maxsize > 24)
		s[24] = '\0';
#endif
#endif
}

/*
 * The next function has been imported from OpenBSD.
 * Original author is: Otto Moerbeek <otto@drijf.net>
 */
#define	begins_with(s, pre)	(strncmp(s, pre, sizeof (pre)-1) == 0)
#define	C			(char *)

static char *
match_function(f, pos, filen)
	const off_t	*f;
	int		pos;
	int		filen;
{
	unsigned char	buf[FUNCTION_CONTEXT_SIZE];
	size_t		nc;
	int		last = lastline;
	char		*state = NULL;
	FILE		*fp = bufwchar[filen].iop;
	off_t		off;

	off = ftellbuf(filen);
	lastline = pos;
	while (pos > last) {
		fseek(fp, f[pos - 1], SEEK_SET);
		nc = f[pos] - f[pos - 1];
		if (nc >= sizeof (buf))
			nc = sizeof (buf) - 1;
		nc = fread(buf, 1, nc, fp);
		if (nc > 0) {
			buf[nc] = '\0';
			buf[strcspn(C buf, "\n")] = '\0';
			if (isalpha(buf[0]) || buf[0] == '_' || buf[0] == '$') {
				if (begins_with(C buf, "private:")) {
					if (!state)
						state = " (private)";
				} else if (begins_with(C buf, "protected:")) {
					if (!state)
						state = " (protected)";
				} else if (begins_with(C buf, "public:")) {
					if (!state)
						state = " (public)";
				} else {
					strlcpy(lastbuf, C buf,
					    sizeof (lastbuf));

					if (state)
						strlcat(lastbuf, state,
						    sizeof (lastbuf));
					lastmatchline = pos;
					initbuf(fp, filen, off);
					return (lastbuf);
				}
			}
		}
		pos--;
	}
	initbuf(fp, filen, off);
	return (lastmatchline > 0 ? lastbuf : NULL);
}
