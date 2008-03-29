/* @(#)defaults.h	1.3 08/02/17 Copyright 1998-2008 J. Schilling */
/*
 *	The cdrecord defaults (/etc/default/cdrecord) interface
 *
 *	Copyright (c) 1998-2008 J. Schilling
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

#ifndef	_DEFAULTS_H
#define	_DEFAULTS_H
/*
 * defaults.c
 */
extern	void	cdr_defaults	__PR((char **devp, int *speedp, long *fsp, long *bufsizep, char **drvoptp));

#endif	/* _DEFAULTS_H */
