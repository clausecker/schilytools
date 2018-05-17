/* @(#)limit.h	1.8 11/07/16 Copyright 1995-2008 J. Schilling */
/*
 * limit.c
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

extern	void	blimit		__PR((Argvec *vp, FILE **std, int flag));
extern	void	prtime		__PR((FILE **std, long sec, long usec));
extern	void	getpruself	__PR((struct rusage *prusage));
extern	void	getpruchld	__PR((struct rusage *prusage));
extern	void	btime		__PR((Argvec *vp, FILE **std, int flag));
extern	void	inittime	__PR((void));
extern	void	setstime	__PR((void));
extern	void	prtimes		__PR((FILE **std, struct rusage *prusage));
extern	void	rusagesub	__PR((struct rusage *pru1, struct rusage *pru2));
extern	void	rusageadd	__PR((struct rusage *pru1, struct rusage *pru2));

/*
 * in getrusage.c
 */
#ifndef	HAVE_GETRUSAGE
extern	int	getrusage	__PR((int who, struct rusage *rusage));
#endif
