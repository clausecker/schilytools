/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * A copy of the CDDL is also available via the Internet at
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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)jobs.c	1.28	07/05/14 SMI"
#endif

#ifndef _JOBS_H
#define	_JOBS_H

/*
 * Copyright 2008-2020 J. Schilling
 *
 * @(#)jobs.h	1.2 20/10/06 2008-2020 J. Schilling
 */

/*
 * Job control for UNIX Shell
 */

#ifdef	SCHILY_INCLUDES
#include	<schily/ioctl.h>	/* Must be before termios.h BSD botch */
#include	<schily/termios.h>
#include	<schily/types.h>
#include	<schily/utypes.h>
#ifdef	DO_TIME
#include	<schily/time.h>
#include	<schily/resource.h>
#endif
#else
#include	<sys/termio.h>
#include	<sys/types.h>
#ifdef	DO_TIME
#include	<sys/resource.h>
#endif
#endif

/*
 * one of these for each active job
 */
struct job
{
	struct job *j_nxtp;	/* next job in job ID order */
	struct job *j_curp;	/* next job in job currency order */
	struct termios j_stty;	/* termio save area when job stops */
#ifdef	DO_TIME
	struct timeval j_start;	/* job start time */
	struct rusage j_rustart; /* resource usage at start of job */
#endif
	pid_t	j_pid;		/* job leader's process ID */
	pid_t	j_pgid;		/* job's process group ID */
	pid_t	j_tgid;		/* job's foreground process group ID */
	UInt32_t j_jid;		/* job ID */
	Int32_t j_xval;		/* exit code, or exit or stop signal */
	Int16_t j_xcode;	/* exit or stop reason */
	Int16_t j_xsig;		/* exit signal, typicalle SIGCHLD */
	UInt16_t j_flag;	/* various status flags defined below */
	char	*j_pwd;		/* job's working directory */
	char	*j_cmd;		/* cmd used to invoke this job */
};

/*
 * defines for j_flag
 */
#define	J_DUMPED	0001	/* job has core dumped */
#define	J_NOTIFY	0002	/* job has changed status */
#define	J_SAVETTY	0004	/* job was stopped in foreground, and its */
				/*   termio settings were saved */
#define	J_STOPPED	0010	/* job has been stopped */
#define	J_SIGNALED	0020	/* job has received signal; j_xval has it */
#define	J_DONE		0040	/* job has finished */
#define	J_RUNNING	0100	/* job is currently running */
#define	J_FOREGND	0200	/* job was put in foreground by shell */
#define	J_BLTIN		0400	/* job was a shell builtin */
#define	J_REPORTED	01000	/* job was "reported" via wait(1) */

/*
 * From jobs.c:
 */
#ifdef	DO_TIME
extern	void	ruget		__PR((struct rusage *rup));
#endif

#endif /* _JOBS_H */
