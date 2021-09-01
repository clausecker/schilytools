/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
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
/*
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)lock.cc 1.17 06/12/12
 */

#pragma	ident	"@(#)lock.cc	1.17	06/12/12"

/*
 * Copyright 2017-2021 J. Schilling
 *
 * @(#)lock.cc	1.10 21/08/15 2017-2021 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)lock.cc	1.10 21/08/15 2017-2021 J. Schilling";
#endif

#include <avo/intl.h>	/* for NOCATGETS */

#if defined(SCHILY_BUILD) || defined(SCHILY_INCLUDES)
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/param.h>
#include <schily/stat.h>
#include <schily/types.h>
#include <schily/unistd.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <vroot/vroot.h>
#include <mksh/defs.h>	/* for libintl */

#if defined(SCHILY_BUILD) || defined(SCHILY_INCLUDES)
#include <schily/signal.h>
#include <schily/errno.h>		/* errno */
#else
#include <signal.h>
#include <errno.h>			/* errno */
#endif

#if	!defined(HAVE_STRERROR)
extern	char		*sys_errlist[];
extern	int		sys_nerr;
#endif

static	void		file_lock_error(char *msg, char *file, const char *str, const char *arg1, const char *arg2);

#define BLOCK_INTERUPTS sigfillset(&newset) ; \
	sigprocmask(SIG_SETMASK, &newset, &oldset)

#define UNBLOCK_INTERUPTS \
	sigprocmask(SIG_SETMASK, &oldset, &newset)

/*
 * This code stolen from the NSE library and changed to not depend
 * upon any NSE routines or header files.
 *
 * Simple file locking.
 * Create a symlink to a file.  The "test and set" will be
 * atomic as creating the symlink provides both functions.
 *
 * The timeout value specifies how long to wait for stale locks
 * to disappear.  If the lock is more than 'timeout' seconds old
 * then it is ok to blow it away.  This part has a small window
 * of vunerability as the operations of testing the time,
 * removing the lock and creating a new one are not atomic.
 * It would be possible for two processes to both decide to blow
 * away the lock and then have process A remove the lock and establish
 * its own, and then then have process B remove the lock which accidentily
 * removes A's lock rather than the stale one.
 *
 * A further complication is with the NFS.  If the file in question is
 * being served by an NFS server, then its time is set by that server.
 * We can not use the time on the client machine to check for a stale
 * lock.  Therefore, a temp file on the server is created to get
 * the servers current time.
 *
 * Returns an error message.  NULL return means the lock was obtained.
 *
 * 12/6/91 Added the parameter "file_locked".  Before this parameter 
 * was added, the calling procedure would have to wait for file_lock() 
 * to return before it sets the flag. If the user interrupted "make"
 * between the time the lock was acquired and the time file_lock()
 * returns, make wouldn't know that the file has been locked, and therefore
 * it wouldn' remove the lock. Setting the flag right after locking the file
 * makes this window much smaller.
 */

int
file_lock(char *name, char *lockname, int *file_locked, int timeout)
{
	int		counter = 0;
	static char	msg[MAXPATHLEN+1];
	int		printed_warning = 0;
	int		r;
	struct stat	statb;
	sigset_t newset;
	sigset_t oldset;

	*file_locked = 0;	
	if (timeout <= 0) {
		timeout = 120;
	}
	for (;;) {
		BLOCK_INTERUPTS;
		r = symlink(name, lockname);
		if (r == 0) {
			*file_locked = 1;
			UNBLOCK_INTERUPTS;
			return 0; /* success */
		}
		UNBLOCK_INTERUPTS;

		if (errno != EEXIST) {
			file_lock_error(msg, name, NOCATGETS("symlink(%s, %s)"),
			    name, lockname);
			fprintf(stderr, "%s", msg);
			return errno;
		}

		counter = 0;
		for (;;) {
			sleep(1); 
			r = lstat(lockname, &statb);
			if (r == -1) {
				/*
				 * The lock must have just gone away - try 
				 * again.
				 */
				break;
			}

			if ((counter > 5) && (!printed_warning)) {
				/* Print waiting message after 5 secs */
				if (getcwd(msg, MAXPATHLEN) == NULL)
					msg[0] = nul_char;
				fprintf(stderr,
					gettext("file_lock: file %s is already locked.\n"),
					name);
				fprintf(stderr,
					gettext("file_lock: will periodically check the lockfile %s for two minutes.\n"),
					lockname);
				fprintf(stderr,
					gettext("Current working directory %s\n"),
					msg);

				printed_warning = 1;
			}

			if (++counter > timeout ) {
				/*
				 * Waited enough - return an error..
				 */
				return EEXIST;
			}
		}
	}
	/* NOTREACHED */
}

/*
 * Format a message telling why the lock could not be created.
 */
static	void
file_lock_error(char *msg, char *file, const char *str, const char *arg1, const char *arg2)
{
	int		len;

	sprintf(msg, gettext("Could not lock file `%s'; "), file);
	len = strlen(msg);
	sprintf(&msg[len], str, arg1, arg2);
	strcat(msg, gettext(" failed - "));
#ifdef	HAVE_STRERROR
	char	*emsg;

	emsg = strerror(errno);
	if (emsg) {
		strcat(msg, strerror(errno));
	} else {
		len = strlen(msg);
		sprintf(&msg[len], NOCATGETS("errno %d"), errno);
	}
#else
	if (errno < sys_nerr) {
		strcat(msg, sys_errlist[errno]);
	} else {
		len = strlen(msg);
		sprintf(&msg[len], NOCATGETS("errno %d"), errno);
	}
#endif
}
