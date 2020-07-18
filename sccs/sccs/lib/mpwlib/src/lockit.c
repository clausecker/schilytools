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
 * This file contains modifications Copyright 2006-2020 J. Schilling
 *
 * @(#)lockit.c	1.22 20/07/12 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)lockit.c 1.22 20/07/12 J. Schilling"
#endif
/*
 * @(#)lockit.c 1.20 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)lockit.c"
#pragma ident	"@(#)sccs:lib/mpwlib/lockit.c"
#endif

/*
 *	Process semaphore.
 *	Try repeatedly (`count' times) to create `lockfile' mode 444.
 *	Sleep 10 seconds between tries.
 *	If `lockfile' is successfully created, write the process ID
 *	`pid' in `lockfile' (in binary)and return 0.
 *
 *	If `lockfile' exists and it hasn't been modified within the last
 *	minute, and either the file is empty or the process ID contained
 *	in the file is not the process ID of any existing process,
 *	`lockfile' is removed and it tries again to make `lockfile'.
 *	After `count' tries, or if the reason for the create failing
 *	is something other than EACCES, return xmsg().
 *
 *	Unlockit will return 0 if the named lock exists, contains
 *	the given pid, and is successfully removed; -1 otherwise.
 */

#define	NEED_PRINTF_J		/* Need defines for js_snprintf()? */
#include	<defines.h>
#include	<i18n.h>
#include	<schily/utsname.h>
#include	<ccstypes.h>
#include	<signal.h>
#include	<limits.h>

#ifndef	SYS_NMLN
#define	SYS_NMLN	257
#endif
#ifndef	O_DSYNC
#define	O_DSYNC	0
#endif

#define	nodenamelength	SYS_NMLN

static int onelock __PR((pid_t pid, char *uuname, char *lockfile));

int
lockit(lockfile, count, pid, uuname)
	char	*lockfile;		/* The z.file path (e.g. SCCS/z.foo) */
	int	count;			/* # of retries to get lock (e.g. 4) */
	pid_t	pid;			/* The pid of the holding process.   */
	char	*uuname;		/* The hostname for this machine.    */
{
	int	fd;
	int	dosleep = 0;
	pid_t	opid;
	char	ouuname[nodenamelength];
	long	ltime, omtime, onsecs;
#ifdef	needed
	char	uniqfilename[PATH_MAX+48];
	char	tempfile[PATH_MAX];
	int	uniq_nmbr;

	copy(lockfile, tempfile);
	snprintf(uniqfilename, sizeof (uniqfilename),
	    "%s/%ju.%s%jd", dname(tempfile),
	    (UIntmax_t)pid, uuname, (Intmax_t)time((time_t *)0));
	if (length(uniqfilename) >= PATH_MAX) {
		uniq_nmbr = (int)pid + (int)time((time_t *)0);
		copy(lockfile, tempfile);
		sprintf(uniqfilename, "%s/%X", dname(tempfile),
		    uniq_nmbr&0xffffffff);
		uniqfilename[PATH_MAX-1] = '\0';
	}
	/*
	 * Former SCCS implementations later renamed this file to the lock file
	 * using link() and unlink(), but we like to live without link().
	 * We here only check whether we are able to create a file in the
	 * desired directory.
	 */
	fd = open(uniqfilename, O_WRONLY|O_CREAT|O_BINARY, 0666);
	if (fd < 0) {
		return (-1);
	} else {
		(void) close(fd);
		(void) unlink(uniqfilename);
	}
#endif
	if (lockfile == NULL)
		return (-1);

	for (++count; --count; dosleep ? sleep(10) : 0) {
		dosleep = 1;
		if (onelock(pid, uuname, lockfile) == 0)
			return (0);
		if (errno == EACCES)
			return (-1);
		if (!exists(lockfile))
			continue;
		omtime = Statbuf.st_mtime;
		onsecs = stat_mnsecs(&Statbuf);
		if ((fd = open(lockfile, O_RDONLY|O_BINARY)) < 0)
			continue;
		opid = pid;		/* In case file is empty */
		ouuname[0] = '\0';
		(void) read(fd, (char *)&opid, sizeof (opid));
		(void) read(fd, ouuname, nodenamelength);
		(void) close(fd);
		/*
		 * If lockfile is from this host, check for pid.
		 * If lockfile is empty, ouuname and uuname are not equal.
		 */
		if (equal(ouuname, uuname)) {
			if (opid == pid)	/* Recursive lock attempt */
				return (-2);
			if (opid == getppid())	/* Lock held by our parent */
				return (-3);
			if (kill((int) opid, 0) == -1 && errno == ESRCH) {
				if ((exists(lockfile)) &&
				    (omtime == Statbuf.st_mtime) &&
				    (onsecs == stat_mnsecs(&Statbuf))) {
					(void) unlink(lockfile);
					dosleep = 0;
					continue;
				}
			}
		}
		/*
		 * Lock file is empty, hold by other host or hold by an
		 * existing process on this host.
		 * Wait in case that file is younger than 60 seconds.
		 */
		if ((ltime = time((time_t *)0) - Statbuf.st_mtime) < 60L) {
			if (ltime >= 0 && ltime < 60) {
				(void) sleep((unsigned) (60 - ltime));
			} else {
				(void) sleep(60);
			}
		}
		/*
		 * Lock file is at least 60 seconds old.
		 * If lock file is still empty and did not change, we may
		 * remove it as we check for long delays between creation
		 * and writing the content in onelock().
		 * Since we only unlink() the file in case it is empty, we
		 * do not affect lock files from different NFS participants.
		 * If an operation takes more than 60 seconds, it is thus
		 * important to refresh the timestamps from the lock file
		 * after less than 60 seconds, to make NFS collaboration work.
		 */
		if (exists(lockfile) &&
		    Statbuf.st_size == 0 &&
		    (omtime == Statbuf.st_mtime) &&
		    (onsecs == stat_mnsecs(&Statbuf))) {
			(void) unlink(lockfile);
			dosleep = 0;
		}
	}
	errno = EEXIST;	/* We failed due to exhausted retry count */
	return (-1);
}

int
lockrefresh(lockfile)
	char	*lockfile;		/* The z.file path (e.g. SCCS/z.foo) */
{
	/*
	 * Do we need to check whether this is still our old lock?
	 */
	return (utimens(lockfile, NULL));
}

int
unlockit(lockfile, pid, uuname)
char	*lockfile;
pid_t	pid;
char	*uuname;
{
	/*
	 * Do nothing if just called from a clean up handler that does
	 * not yet have a lockfile name.
	 */
	if (lockfile == NULL || lockfile[0] == '\0')
		return (-1);

	if (mylock(lockfile, pid, uuname))
		return (unlink(lockfile));
	else
		return (-1);
}

/*
 * Create a lockfile using O_CREAT|O_EXC and write our identification
 * pid/uuname into the file.
 */
static int
onelock(pid, uuname, lockfile)
pid_t	pid;
char	*uuname;
char	*lockfile;
{
	int	fd;
	char	lock[sizeof (pid_t) + nodenamelength];
	char	*p;
	int	i;
	time_t	otime;

	/*
	 * Copy pid and nodename together and write it in a single write() call
	 * to make sure we are not interrupted with a partially written lock
	 * file.
	 * A previous implementation wrote into the file under a temporary
	 * name and then linked it to lockname, but not all platforms support
	 * hard links.
	 */
	for (i = 0, p = (char *)&pid; i < sizeof (pid_t); i++)
		lock[i] = *p++;
	strncpy(&lock[sizeof (pid_t)], uuname, nodenamelength);

	/*
	 * Old SunOS versions from this file used O_DSYNC as open() flag.
	 * If we do this, the write() call below becomes expensive.
	 * Approx. 20ms on Solaris and approx. 50ms on Linux. Since with the
	 * project mode, we need two locks, this would be too much. The only
	 * advantage, O_DSYNC gives is on-disk consistence and since a reboot
	 * makes the process ID in the lock file void, it seems that the in
	 * core view of the file is sufficient for the integrity of the lock.
	 */
	otime = time((time_t *)0);
	if ((fd = open(lockfile,
		    O_WRONLY|O_CREAT|O_EXCL|O_BINARY, 0444)) >= 0) {
		/*
		 * One write, so the file is either empty or written completely
		 */
		if (write(fd, lock, sizeof (lock)) != sizeof (lock)) {
			(void) close(fd);
			(void) unlink(lockfile);
			return (xmsg(lockfile, NOGETTEXT("lockit")));
		}
		close(fd);
		/*
		 * In case of a long sleep, check whether the current lock file
		 * is ours or whether it has been replaced by a faster process.
		 * If the current lock file is not from us, allow to try again
		 * to create the lock.
		 */
		if ((time((time_t *)0) - otime) > 2)
			if (!mylock(lockfile, pid, uuname))
				return (-1);
		return (0);
	}
	if ((errno == ENFILE) || (errno == EACCES) || (errno == EEXIST)) {
		/*
		 * This is a retryable error.
		 */
		return (-1);
	} else {
		/*
		 * Give up for other problems.
		 */
		return (xmsg(lockfile, NOGETTEXT("lockit")));
	}
}

/*
 * Check whether a lock in lockfile exists and whether pid/uuname are equal
 * to what's in the file.
 *
 * Returns:
 *
 *	-2	Lock hold by a currently active process on this machine.
 *
 *	-1	No lock file is present (ENOENT).
 *
 *	0	Cannot open lock file, empty file or pid/uuname do not match.
 *		This includes a currently active lock on a different machine.
 *
 *	1	Lock file is present and pid/uuname match with the file.
 */
int
ismylock(lockfile, pid, uuname)
	char	*lockfile;
	pid_t	pid;
	char	*uuname;
{
	int	fd;
	int	n;
	pid_t	opid;
	char	ouuname[nodenamelength];

	if ((fd = open(lockfile, O_RDONLY|O_BINARY)) < 0) {
		if (errno == ENOENT)		/* There was no lock file */
			return (-1);
		return (0);
	}
	n = read(fd, (char *)&opid, sizeof (opid));
	(void) read(fd, ouuname, nodenamelength);
	(void) close(fd);
	if (n == sizeof (opid) && opid == pid && (equal(ouuname, uuname))) {
		return (1);
	} else {
		if (n == sizeof (opid) && (equal(ouuname, uuname))) {
			/*
			 * The case of a legally long lasting lock is finally
			 * handled in lockit().
			 */
			if (kill(opid, 0) == 0) {
				return (-2);	/* Lock is still active */
			}
		}
		return (0);
	}
}

/*
 * Check whether a lock in lockfile exists and can be opened and whether
 * pid/uuname are equal to what's in the file.
 *
 * Returns:
 *
 *	FALSE	Cannot open lock file, or pid/uuname do not match.
 *
 *	TRUE	Lock file is present and pid/uuname match with the file.
 */
int
mylock(lockfile, pid, uuname)
	char	*lockfile;
	pid_t	pid;
	char	*uuname;
{
	return (ismylock(lockfile, pid, uuname) > 0);
}

/*
 * Generic lock file error routine.
 */
void
lockfatal(lockfile, pid, uuname)
	char	*lockfile;
	pid_t	pid;
	char	*uuname;
{
	if (ismylock(lockfile, pid, uuname) > 0)
		fatal(gettext("recursive dead lock attempt (cm23)"));

	if (ismylock(lockfile, getppid(), uuname) > 0)
		fatal(gettext("parent dead lock attempt (cm24)"));

	efatal(gettext("cannot create lock file (cm4)"));
}
