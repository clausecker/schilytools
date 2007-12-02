/* @(#)types.h	1.2 07/01/16 Copyright 2006-2007 J. Schilling */
/*
 *	Abstraction from sys/types.h
 *
 *	Copyright (c) 2006-2007 J. Schilling
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

#ifndef _SCHILY_TYPES_H
#define	_SCHILY_TYPES_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_SYS_TYPES_H
#ifndef	_INCL_SYS_TYPES_H
#include <sys/types.h>
#define	_INCL_SYS_TYPES_H
#endif
#endif

#endif	/* _SCHILY_TYPES_H */
