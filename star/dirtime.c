/* @(#)dirtime.c	1.33 18/06/20 Copyright 1988-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)dirtime.c	1.33 18/06/20 Copyright 1988-2018 J. Schilling";
#endif
/*
 *	Copyright (c) 1988-2018 J. Schilling
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

/*
 * Save directories and its times on a stack and set the times, if the new name
 * will not increase the depth of the directory stack.
 * The final flush of the stack is caused by a zero length filename.
 *
 * A string will be sufficient for the names of the directory stack because
 * all directories in a tree have a common prefix.  A counter for each
 * occurence of a slash '/' is the index into the array of times for the
 * directory stack. Directories with unknown times have atime.tv_nsec == -1.
 *
 * If the order of the files on tape is not in an order that find(1) will
 * produce, this algorithm is not guaranteed to work. This is the case with
 * tapes that have been created with the -r option or with the list= option.
 *
 * The only alternative would be saving all directory times and setting them
 * at the end of an extract.
 *
 * NOTE: I am not shure if degenerate filenames will fool this algorithm.
 */
#include <schily/types.h>	/* includes <sys/types.h> needed for mode_t */
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/string.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include "star.h"
#include "xutimes.h"
#include "checkerr.h"
#include "dirtime.h"
#include "starsubs.h"
#include "pathname.h"

#ifdef DEBUG
extern	BOOL	debug;
#define	EDBG(a)	if (debug) error a
#else
#define	EDBG(a)
#endif

/*
 * Maximum depth of directory nesting depends on availabe memory.
 */
LOCAL	pathstore_t	dirstack;

#ifdef	SET_CTIME
#define	NT	3
LOCAL	struct timespec dottimes[NT] = { {-1, -1}, {-1, -1}, {-1, -1}};
#else
#define	NT	2
LOCAL	struct timespec dottimes[NT] = { -1, -1, -1, -1};
#endif
LOCAL	struct timespec badtime = { -1, -1};

typedef	struct timespec	timev[NT];

LOCAL	pathstore_t	dtps;
LOCAL	int		ndtimes;
LOCAL	timev		*dtimes;

LOCAL	mode_t	dotmodes = _BAD_MODE;

LOCAL	pathstore_t	dmps;
LOCAL	int		ndmodes;
LOCAL	mode_t		*dmodes;


LOCAL	BOOL	init_dirtimes	__PR((void));
LOCAL	BOOL	grow_dtimes	__PR((int n));
LOCAL	BOOL	grow_dmodes	__PR((int n));
EXPORT	void	sdirtimes	__PR((char *name, FINFO *info,
						BOOL do_times, BOOL do_mode));
EXPORT	void	sdirmode	__PR((char *name, mode_t mode));
LOCAL	void	dirtimes	__PR((char *name, struct timespec *tp,
								mode_t mode));
EXPORT	void	flushdirtimes	__PR((void));
LOCAL	void	flushdirstack	__PR((char *, char *, int));
LOCAL	void	setdirtime	__PR((char *, struct timespec *));

LOCAL BOOL
init_dirtimes()
{
	/*
	 * First the path string
	 */
	if (init_pspace(PS_STDERR, &dirstack) < 0)
		return (FALSE);
	/*
	 * Now the time array
	 */
	if (!grow_dtimes(1))
		return (FALSE);

	/*
	 * Now the mode array
	 */
	if (!grow_dmodes(1))
		return (FALSE);

	return (TRUE);
}

LOCAL BOOL
grow_dtimes(n)
	int	n;
{
	if ((n * sizeof (timev)) < ndtimes)
		return (TRUE);

	/*
	 * Add 1 since "ndtimes" is the size of the array while "n" is the
	 * next array index to use.
	 */
	if (grow_pspace(PS_STDERR, &dtps, (n+1) * sizeof (timev)) < 0) {
		return (FALSE);
	}

	dtimes = (timev *)dtps.ps_path;
	ndtimes = dtps.ps_size / sizeof (timev);

	return (TRUE);
}

LOCAL BOOL
grow_dmodes(n)
	int	n;
{
	if ((n * sizeof (mode_t)) < ndmodes)
		return (TRUE);

	/*
	 * Add 1 since "ndmodes" is the size of the array while "n" is the
	 * next array index to use.
	 */
	if (grow_pspace(PS_STDERR, &dmps, (n+1) * sizeof (mode_t)) < 0) {
		return (FALSE);
	}

	dmodes = (mode_t *)dmps.ps_path;
	ndmodes = dmps.ps_size / sizeof (mode_t);

	return (TRUE);
}

/*
 * This is the standard method to enter time and mode values
 * into the directory stack.
 */
EXPORT void
sdirtimes(name, info, do_times, do_mode)
	char	*name;
	FINFO	*info;
	BOOL	do_times;
	BOOL	do_mode;
{
	struct timespec	tp[NT];
	mode_t		mode = _BAD_MODE;

	if (do_times) {
		tp[0].tv_sec = info->f_atime;
		tp[0].tv_nsec = info->f_ansec;

		tp[1].tv_sec = info->f_mtime;
		tp[1].tv_nsec = info->f_mnsec;
#ifdef	SET_CTIME
		tp[2].tv_sec = info->f_ctime;
		tp[2].tv_nsec = info->f_cnsec;
#endif
	} else {
		tp[0] = badtime;
		tp[1] = badtime;
#ifdef	SET_CTIME
		tp[2] = badtime;
#endif
	}
	if (do_mode) {
		mode = info->f_mode;
	}
	dirtimes(name, tp, mode);
}

/*
 * This is the method to enter mode values into the directory stack.
 * It is only used to puch the umask value from _create_dirs().
 */
EXPORT void
#ifdef	PROTOTYPES
sdirmode(char *name, mode_t mode)
#else
sdirmode(name, mode)
	char	*name;
	mode_t	mode;
#endif
{
	struct timespec	tp[NT];

	tp[0] = badtime;
	tp[1] = badtime;
#ifdef	SET_CTIME
	tp[2] = badtime;
#endif
	dirtimes(name, tp, mode);
}

LOCAL void
#ifdef	PROTOTYPES
dirtimes(char *name, struct timespec tp[NT], mode_t mode)
#else
dirtimes(name, tp, mode)
	char		*name;
	struct timespec	tp[NT];
	mode_t		mode;
#endif
{
	register char	*dp = dirstack.ps_path;
	register char	*np = name;
	register int	idx = -1;

	if (dp == NULL) {
		if (!init_dirtimes())
			return;
		dp = dirstack.ps_path;
	}
	EDBG(("dirtimes('%s', %s", name, tp ? ctime(&tp[1].tv_sec):"NULL\n"));

	if (np[0] == '\0') {				/* final flush */
		if (dotmodes != _BAD_MODE) {
			EDBG(("setmode: '.' to 0%o\n", dotmodes));
			setdirmodes(".", dotmodes);
		}
		if (dottimes[0].tv_nsec != badtime.tv_nsec)
			setdirtime(".", dottimes);
		flushdirstack(dp, dp, -1);
		return;
	}

	if ((np[0] == '.' && np[1] == '/' && np[2] == '\0') ||
				(np[0] == '.' && np[1] == '\0')) {
		dottimes[0] = tp[0];
		dottimes[1] = tp[1];
#ifdef	SET_CTIME
		dottimes[2] = tp[2];
#endif
		dotmodes = mode;
	} else {
		size_t	nlen;

		nlen = strlen(np);
		if (nlen >= dirstack.ps_size) {
			if (set_pspace(PS_STDERR, &dirstack, nlen+1) < 0) {
				return;
			}
			dp = dirstack.ps_path;
		}

		/*
		 * Find end of common part
		 */
		while (*dp == *np) {
			if (*dp == '\0')
				break;
			if (*dp++ == '/')
				++idx;
			np++;
		}
		/*
		 * Make sure that the ending '/' always stays in dirstack.
		 */
		if (*dp == '/' && *np == '\0') {
			dp++;
			++idx;
		}
		EDBG(("DIR: '%.*s' DP: '%s' NP: '%s' idx: %d\n",
				/* XXX Should not be > int */
				(int)(dp - dirstack.ps_path),
				dirstack.ps_path, dp, np, idx));

		if (*dp) {
			/*
			 * New directory does not increase the depth of the
			 * directory stack. Flush all dirs below idx.
			 */
			flushdirstack(dirstack.ps_path, dp, idx);
		}

		/*
		 * Put the new dir on the directory stack.
		 * First append the name component, then
		 * store times of "this" dir.
		 */
		while ((*dp = *np++) != '\0') {
			if (*dp++ == '/') {
				/*
				 * Disable times of unknown dirs.
				 */
				if (!grow_dtimes(++idx))
					return;
				if (!grow_dmodes(idx))
					return;
				EDBG(("zapping idx: %d\n", idx));
				dtimes[idx][0] = badtime;
				dmodes[idx] = _BAD_MODE;
			} else if (*np == '\0') {
				/*
				 * Make sure the dirname always ends with '/'.
				 */
				*dp++ = '/';
				*dp = '\0';
				idx++;
			}
		}
		if (tp) {
			if (!grow_dtimes(idx))
				return;
			if (!grow_dmodes(idx))
				return;
			EDBG(("set idx %d '%s'\n", idx, name));
			dtimes[idx][0] = tp[0];	/* overwrite last atime */
			dtimes[idx][1] = tp[1];	/* overwrite last mtime */
#ifdef	SET_CTIME
			dtimes[idx][2] = tp[2];	/* overwrite last ctime */
#endif
			dmodes[idx] = mode;
		}
	}
}

/*
 * Needed for on_comerr() as we cannot pass the needed parameters to dirtimes().
 */
EXPORT void
flushdirtimes()
{
	dirtimes("", (struct timespec *)0, (mode_t)0);
}

LOCAL void
flushdirstack(dirbase, dp, depth)
		char	*dirbase;
	register char	*dp;
	register int	depth;
{
	if (depth == -1 && dp == dirbase && dp[0] == '/') {
		/*
		 * Flush the root dir, avoid flushing "".
		 */
		while (*dp == '/')
			dp++;
		if (dmodes[++depth] != _BAD_MODE) {
			EDBG(("depth: %d setmode: '/' to 0%o\n",
							depth, dmodes[depth]));
			setdirmodes("/", dmodes[depth]);
		}
		if (dtimes[depth][0].tv_nsec != badtime.tv_nsec) {
			EDBG(("depth: %d ", depth));
			setdirtime("/", dtimes[depth]);
		}
	}
	/*
	 * The dirname always ends with a '/' (see above).
	 */
	while (*dp) {
		if (*dp++ == '/') {
			*--dp = '\0';	/* temporarily delete '/' */
			if (dmodes[++depth] != _BAD_MODE) {
				EDBG(("depth: %d setmode: '%s' to 0%o\n",
						depth, dirbase, dmodes[depth]));
				setdirmodes(dirbase, dmodes[depth]);
			}
			if (dtimes[depth][0].tv_nsec != badtime.tv_nsec) {
				EDBG(("depth: %d ", depth));
				setdirtime(dirbase, dtimes[depth]);
			}
			*dp++ = '/';	/* restore '/' */
		}
	}
}

LOCAL void
setdirtime(name, tp)
	char	*name;
	struct timespec	tp[NT];
{
	EDBG(("settime: '%s' to %s", name, ctime(&tp[1].tv_sec)));
	if (xutimes(name, tp) < 0) {
		if (!errhidden(E_SETTIME, name)) {
			if (!errwarnonly(E_SETTIME, name))
				xstats.s_settime++;
			errmsg("Can't set time on '%s'.\n", name);
			(void) errabort(E_SETTIME, name, TRUE);
		}
	}
}
