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
 * @(#)lockchset.c	1.1 20/06/18 Copyright 2020 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)lockchset.c	1.1 20/06/18 Copyright 2020 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)lockchset.c"
#pragma ident	"@(#)sccs:lib/comobj/lockchset.c"
#endif
#include	<defines.h>

int
lockchset(ppid, pid, uuname)
	pid_t	ppid;			/* The pid of the parent process.    */
	pid_t	pid;			/* The pid of the holding process.   */
	char	*uuname;		/* The hostname for this machine.    */
{
	char	*cs;
	int	r = 0;

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
	return (unlockit(auxf(changesetfile, 'z'), pid, uuname));
}

int
refreshchsetlock()
{
	return (lockrefresh(auxf(changesetfile, 'z')));
}
