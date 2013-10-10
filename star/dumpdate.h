/* @(#)dumpdate.h	1.14 13/10/05 Copyright 2003-2013 J. Schilling */
/*
 *	Copyright (c) 2003-2013 J. Schilling
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

#ifndef	DUMPDATE_H
#define	DUMPDATE_H

#ifndef	_SCHILY_TIME_H
#include <schily/time.h>
#endif
#ifndef	_SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

typedef struct dumpdates dumpd_t;

struct dumpdates {
	dumpd_t		*next;
	char		*name;
	int		level;
	struct timespec	date;
	Uchar		flags;
};

/*
 * Definitions for the dumpdates 'flags'.
 */
#define	DD_PARTIAL	0x01	/* Dump is partial */
#define	DD_CUMULATIVE	0x02	/* Dump is cumulative to same level */


extern	void	initdumpdates	__PR((char *fname, BOOL doupdate));
extern	void	writedumpdates	__PR((char *fname, const char *name,
							int level, int dflags,
							struct timespec *date));
extern	char	*dumpdate	__PR((struct timespec *date));
extern	BOOL	getdumptime	__PR((char *p, struct timespec *tvp));
extern	dumpd_t *checkdumpdates	__PR((const char *name, int level, int dflags));
extern	void	adddumpdates	__PR((const char *name, int level, int dflags,
							struct timespec *date,
								BOOL useold));

#endif
