/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */
/*
 *	Lock the project via the changeset file in lock:
 *	$SET_HOME/.sccs/SCCS/z.changeset
 *
 * @(#)lockchset.c	1.6 20/08/10 Copyright 2020 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)lockchset.c	1.6 20/08/10 Copyright 2020 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)lockchset.c"
#pragma ident	"@(#)sccs:lib/comobj/lockchset.c"
#endif
#include	<defines.h>
#include	<schily/signal.h>

LOCAL	RETSIGTYPE	siglock	__PR((int));

int
lockchset(ppid, pid, uuname)
	pid_t	ppid;			/* The pid of the parent process.    */
	pid_t	pid;			/* The pid of the holding process.   */
	char	*uuname;		/* The hostname for this machine.    */
{
	char	*cs;
	int	r = 0;

	if (changesetfile == NULL)
		return (0);

	/*
	 * Apply for a global lock...
	 * First check whether there is a lock owned by our parent
	 * that may be sccs(1). If the lock is present and owned
	 * by our parent, everything is OK since we are called by
	 * sccs(1) for a larger work.
	 *
	 * If ppid = 0, this is sccs(1) and we do not check for a lock
	 * hold by the parent.
	 */
	cs = auxf(changesetfile, 'z');
	if (ppid != 0 && (r = ismylock(cs, ppid, uuname)) > 0)
		return (0);

	/*
	 * Since r is pre-initialized as 0, we assume a crash in case that the
	 * non-empty global lock file exists and we did not check for a parent
	 * owned lock because we are sccs(1).
	 */
	if (r == 0 && exists(cs) && Statbuf.st_size > 0) {
		/*
		 * A lock file exists that does not match our
		 * parent. A previous action may have crashed.
		 * So we need to run a recovery protocol. In
		 * order to learn what we need to do, we abort
		 * for now to find out when it happens.
		 */
		char	ebuf[MAXPATHLEN];

		snprintf(ebuf, sizeof (ebuf), gettext(
		    "Recovery needed? Otherwise remove '%s'"), cs);
		fatal(ebuf);
	}
	/*
	 * Lock out any other user who may be trying to
	 * process files from the same project.
	 */
	return (lockit(cs, SCCS_LOCK_ATTEMPTS, pid, uuname));
}

int
unlockchset(pid, uuname)
	pid_t	pid;			/* The pid of the holding process.   */
	char	*uuname;		/* The hostname for this machine.    */
{
	if (changesetfile == NULL)
		return (0);
	return (unlockit(auxf(changesetfile, 'z'), pid, uuname));
}

/*
 * Return whether a lockfile name is pointing to the active global lock which
 * is the lock file for the changeset file. We need to do this check in order
 * to avoid a deadlock from attempting to create the changeset lock a second
 * time while trying to modify the changeset file from one of the low level
 * programs when updating the global status.
 *
 * Returns:
 *
 *	0	A changeset lock is not active of "fname" is not thename for
 *		the changeset lock file.
 *
 *	1	"fname" points to the active changeset lock file.
 */
int
islockchset(fname)
	char	*fname;			/* The filename to check against    */
{
	char		*p;
	struct stat	sbchset;
	struct stat	sb;

	if (changesetfile == NULL)	/* Cannot be the changeset lock name */
		return (0);

	if (!SETHOME_CHSET())		/* Not in a changeset environment    */
		return (0);

	if ((p = strrchr(fname, '/')) == NULL)
		p = fname;
	else
		p++;
	if (strcmp(p, "z.changeset"))	/* Wrong name, cannot be changeset   */
		return (0);

	if (stat(auxf(changesetfile, 'z'), &sbchset) < 0)
		return (0);		/* Currently no changeset lock file */

	if (stat(fname, &sb) < 0)	/* Cannot be identical to chset lock */
		return (0);

	if (sbchset.st_dev != sb.st_dev)
		return (0);
	if (sbchset.st_ino != sb.st_ino)
		return (0);

	return (1);
}

int
refreshchsetlock()
{
	if (changesetfile == NULL)
		return (0);
	return (lockrefresh(auxf(changesetfile, 'z')));
}

LOCAL	char	*lockfile;		/* Name of current file lock */

#if	defined(HAVE_ALARM)
#if	(defined(HAVE_SIGPROCMASK) && defined(SA_RESTART)) || \
	defined(HAVE_SIGSETMASK)

LOCAL RETSIGTYPE
siglock(signo)
	int	signo;
{
	if (changesetfile)
		refreshchsetlock();	/* Refresh Changeset lock */
	if (lockfile && *lockfile)
		lockrefresh(lockfile);	/* Refresh lock for current file */

	alarm(30);
}
#endif
#endif

void
timersetlockfile(name)
	char	*name;
{
	lockfile = name;
}

/*
 * Set a repeated timer for every 30 seconds to refresh the lock files.
 * This avoids locks over NFS to expire in case that we are working on a
 * longer lasting task.
 *
 * We can do this only on platforms that support signals that do not
 * interrupt system calls. So we need either POSIX or BSD signals.
 */
void
timerchsetlock()
{
#if	defined(HAVE_ALARM)
#if	defined(HAVE_SIGPROCMASK) && defined(SA_RESTART)
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = siglock;
	sa.sa_flags = SA_RESTART;
	(void) sigaction(SIGALRM, &sa, (struct sigaction *)0);
	alarm(30);
#else
#ifdef	HAVE_SIGSETMASK
	struct sigvec	sv;

	sv.sv_mask = 0;
	sv.sv_handler = siglock;
	sv.sv_flags = 0;
	(void) sigvec(SIGALRM, &sv, (struct sigvec *)0);
	alarm(30);
#endif
#endif
#endif
}
