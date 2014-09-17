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
 * This file contains modifications Copyright 2006-2014 J. Schilling
 *
 * @(#)lockit.c	1.14 14/08/09 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)lockit.c 1.14 14/08/09 J. Schilling"
#endif
/*
 * @(#)lockit.c 1.20 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)lockit.c"
#pragma ident	"@(#)sccs:lib/mpwlib/lockit.c"
#endif

/*
	Process semaphore.
	Try repeatedly (`count' times) to create `lockfile' mode 444.
	Sleep 10 seconds between tries.
	If `tempfile' is successfully created, write the process ID
	`pid' in `tempfile' (in binary), link `tempfile' to `lockfile',
	and return 0.
	If `lockfile' exists and it hasn't been modified within the last
	minute, and either the file is empty or the process ID contained
	in the file is not the process ID of any existing process,
	`lockfile' is removed and it tries again to make `lockfile'.
	After `count' tries, or if the reason for the create failing
	is something other than EACCES, return xmsg().
 
	Unlockit will return 0 if the named lock exists, contains
	the given pid, and is successfully removed; -1 otherwise.
*/

# define	NEED_PRINTF_J		/* Need defines for js_snprintf()? */
# include	<defines.h>
# include	<i18n.h>
# include	<schily/utsname.h>
# include	<ccstypes.h>
# include	<signal.h>
# include	<limits.h>

#ifndef	SYS_NMLN
#define	SYS_NMLN	257
#endif
#ifndef	O_DSYNC
#define	O_DSYNC	0
#endif

# define	nodenamelength	SYS_NMLN

static int onelock __PR((pid_t pid, char *uuname, char *lockfile));

int
lockit(lockfile, count, pid, uuname)
	char	*lockfile;		/* The z.file path (e.g. SCCS/z.foo) */
	int	count;			/* # of retries to get lock (e.g. 4) */
	pid_t	pid;			/* The pid of the holding process.   */
	char	*uuname;		/* The hostname for this machine.    */
{  
	int	fd;
	pid_t	opid;
	char	ouuname[nodenamelength];
	long	ltime, omtime;
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
	   return(-1);
	} else {
	   (void)close(fd);
	   (void)unlink(uniqfilename);
	}
#endif
	for (++count; --count; (void)sleep(10)) {
		if (onelock(pid, uuname, lockfile) == 0)
		   return(0);
		if (errno == EACCES)
		   return(-1);
		if (!exists(lockfile))
		   continue;
		omtime = Statbuf.st_mtime;
		if ((fd = open(lockfile, O_RDONLY|O_BINARY)) < 0)
		   continue;
		opid = pid;		/* In case file is empty */
		ouuname[0] = '\0';
		(void)read(fd, (char *)&opid, sizeof(opid));
		(void)read(fd, ouuname, nodenamelength);
		(void)close(fd);
		/*
		 * If lockfile is from this host, check for pid.
		 * If lockfile is empty, ouuname and uuname are not equal.
		 */
		if (equal(ouuname, uuname)) {
		   if (kill((int) opid,0) == -1 && errno == ESRCH) {
		      if ((exists(lockfile)) &&
			  (omtime == Statbuf.st_mtime)) {
			 (void) unlink(lockfile);
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
		 */
		if (exists(lockfile) &&
		    Statbuf.st_size == 0 && omtime == Statbuf.st_mtime)
			(void) unlink(lockfile);
	}
	errno = EEXIST;	/* We failed due to exhausted retry count */
	return(-1);
}

int
unlockit(lockfile, pid, uuname)
char	*lockfile;
pid_t	pid;
char	*uuname;
{
	int	fd, n;
	pid_t	opid;
	char	ouuname[nodenamelength];

	/*
	 * Do nothing if just called from a clean up handler that does
	 * not yet have a lockfile name.
	 */
	if (lockfile[0] == '\0')
	   return (-1);

	if ((fd = open(lockfile, O_RDONLY|O_BINARY)) < 0) {
	   return(-1);
	}
	n = read(fd, (char *)&opid, sizeof(opid));
	(void)read(fd, ouuname, nodenamelength);
	(void)close(fd);
	if (n == sizeof(opid) && opid == pid && (equal(ouuname,uuname))) {
	   return(unlink(lockfile));
	} else {
	   return(-1);
	}
}

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

	otime = time((time_t *)0);
	if ((fd = open(lockfile, O_WRONLY|O_CREAT|O_EXCL|O_DSYNC|O_BINARY, 0444)) >= 0) {
		if (write(fd, lock, sizeof (lock)) != sizeof (lock)) {
			(void)close(fd);
			(void)unlink(lockfile);
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
		return (-1);
	} else {
		return (xmsg(lockfile, NOGETTEXT("lockit")));
	}
}

int
mylock(lockfile, pid, uuname)
char	*lockfile;
pid_t	pid;
char	*uuname;
{
	int	fd, n;
	pid_t	opid;
	char	ouuname[nodenamelength];

	if ((fd = open(lockfile, O_RDONLY|O_BINARY)) < 0)
	   return(0);
	n = read(fd, (char *)&opid, sizeof(opid));
	(void)read(fd, ouuname, nodenamelength);
	(void)close(fd);
	if (n == sizeof(opid) && opid == pid && (equal(ouuname, uuname))) {
	   return(1);
	} else {
	   return(0);
	}
}
