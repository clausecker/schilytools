/* @(#)proc.c	1.3 08/12/20 Copyright 1995-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)proc.c	1.3 08/12/20 Copyright 1995-2008 J. Schilling";
#endif
/*
 *	Job control process lists
 *
 *	Copyright (c) 1995-2008 J. Schilling
 */
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

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/standard.h>
#include <schily/time.h>
#include <schily/stdlib.h>
#include "bsh.h"

struct proc {
	pid_t	pid;
	struct proc *next;
};

struct proc *procs;

EXPORT	struct proc * newproc	__PR((pid_t pid));
EXPORT	struct proc * findproc	__PR((pid_t pid));
EXPORT	void		exitproc __PR((pid_t pid));

#ifdef	PROTOTYPES
EXPORT struct proc *
newproc(pid_t pid)
#else
EXPORT struct proc *
newproc(pid)
	pid_t	pid;
#endif
{
	struct proc *pp = findproc(pid);

	if (pp)
		return (pp);

	pp = malloc(sizeof (*pp));
	if (pp == NULL)
		return ((struct proc *)0);
	pp->pid = pid;
	pp->next = procs;
	procs = pp;
	return (pp);
}

#ifdef	PROTOTYPES
EXPORT struct proc *
findproc(pid_t pid)
#else
EXPORT struct proc *
findproc(pid)
	pid_t	pid;
#endif
{
	struct proc *pp = procs;

	while (pp) {
		if (pp->pid == pid)
			break;
		pp = pp->next;
	}
	return (pp);
}

#ifdef	PROTOTYPES
EXPORT void
exitproc(pid_t pid)
#else
EXPORT void
exitproc(pid)
	pid_t	pid;
#endif
{
	struct proc *tp = findproc(pid);
	struct proc *pp = procs;

	if (pp == NULL)
		return;

	while (pp) {
		if (pp->next == tp) {
			pp->next = tp->next;
			free(tp);
			break;
		}
		pp = pp->next;
	}
}
