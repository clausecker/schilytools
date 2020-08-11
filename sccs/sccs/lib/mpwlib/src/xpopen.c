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
 *	Spawn new process with arbitrary pipes
 *
 * @(#)xpopen.c	1.1 20/08/06 Copyright 2006-2020 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)xpopen.c	1.1 20/08/06 Copyright 2006-2020 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)xmkdir.c"
#pragma ident	"@(#)sccs:lib/mpwlib/xmkdir.c"
#endif
#include	<defines.h>

#include <schily/unistd.h>
#include <schily/errno.h>
#include <schily/vfork.h>
#include <schily/signal.h>
#include <schily/wait.h>
#include <schily/stdio.h>

#ifndef	HAVE_VFORK
#undef	vfork
#undef	vforkx
#define	vfork	fork
#define	vforkx	forkx
#endif  /* HAVE_VFORK */


pid_t
xpopen(pfpin, pfpout, pfperr, cmd, argv)
	FILE		**pfpin;
	FILE		**pfpout;
	FILE		**pfperr;
	const char	*cmd;
	char *const	argv[];
{
	int	in[2];
	int	out[2];
	int	err[2];
	pid_t	pid;

	in[0]  = STDIN_FILENO;
	out[1] = STDOUT_FILENO;
	err[1] = STDERR_FILENO;
	if (pfpin) {
		if (pipe(in) < 0)
			return ((pid_t)-1);
	}
	if (pfpout) {
		if (pipe(out) < 0) {
			close(in[0]);
			close(in[1]);
			return ((pid_t)-1);
		}
	}
	if (pfperr) {
		if (pipe(err) < 0) {
			close(in[0]);
			close(in[1]);
			close(out[0]);
			close(out[1]);
			return ((pid_t)-1);
		}
	}

#ifdef	FORK_NOSIGCHLD
	pid = vforkx(FORK_NOSIGCHLD|FORK_WAITPID);
#else
	pid = vfork();	/* Need to find a way to deal with SIGCHLD and wait */
#endif
	if (pid == (pid_t)-1) {
		return ((pid_t)-1);
	} else if (pid > 0) {
		if (pfpin) {
			close(in[0]);
			*pfpin = fdopen(in[1], "w");
			if (*pfpin == NULL) {
				close(in[1]);
				close(out[0]);
				close(out[1]);
				close(err[0]);
				close(err[1]);
				kill(pid, SIGKILL);
				xpclose(NULL, NULL, NULL, pid);
				return ((pid_t)-1);
			}
		}
		if (pfpout) {
			close(out[1]);
			*pfpout = fdopen(out[0], "r");
			if (*pfpout == NULL) {
				close(out[0]);
				close(err[0]);
				close(err[1]);
				kill(pid, SIGKILL);
				xpclose(*pfpin, NULL, NULL, pid);
				return ((pid_t)-1);
			}
		}
		if (pfperr) {
			close(err[1]);
			*pfperr = fdopen(err[0], "r");
			if (*pfperr == NULL) {
				close(err[0]);
				kill(pid, SIGKILL);
				xpclose(*pfpin, *pfpout, NULL, pid);
				return ((pid_t)-1);
			}
		}
	} else {
		if (pfpin)
			close(in[1]);
		if (pfpout)
			close(out[0]);
		if (pfperr)
			close(err[0]);
		if (in[0] != STDIN_FILENO) {
			dup2(in[0], STDIN_FILENO);
			close(in[0]);
		}
		if (out[1] != STDOUT_FILENO) {
			dup2(out[1], STDOUT_FILENO);
			close(out[1]);
		}
		if (err[1] != STDERR_FILENO) {
			dup2(err[1], STDERR_FILENO);
			close(err[1]);
		}
		execvp(cmd, argv);
		_exit(1);
	}
	return (pid);
}

int
xpclose(fpin, fpout, fperr, child)
	FILE		*fpin;
	FILE		*fpout;
	FILE		*fperr;
	pid_t		child;
{
	int	status = 0;

	if (fpin)
		fclose(fpin);
	if (fpout)
		fclose(fpout);
	if (fperr)
		fclose(fperr);

	if (child <= 0) {
		errno = ECHILD;
		return (-1);
	}
	if (waitpid(child, &status, WNOHANG) == child)
		return (status);
	while (waitpid(child, &status, 0) < 0) {
		if (errno != EINTR) {
			status = -1;
			break;
		}
	}
	return (status);
}
