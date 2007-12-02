/* @(#)dumpdate.h	1.13 07/10/28 Copyright 2003-2007 J. Schilling */
/*
 *	Copyright (c) 2003-2007 J. Schilling
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
	struct timeval	date;
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
							struct timeval *date));
extern	char	*dumpdate	__PR((struct timeval *date));
extern	BOOL	getdumptime	__PR((char *p, struct timeval *tvp));
extern	dumpd_t *checkdumpdates	__PR((const char *name, int level, int dflags));
extern	void	adddumpdates	__PR((const char *name, int level, int dflags,
							struct timeval *date,
								BOOL useold));

#endif
