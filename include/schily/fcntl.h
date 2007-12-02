/* @(#)fcntl.h	1.16 07/01/16 Copyright 1996-2007 J. Schilling */
/*
 *	Generic header for users of open(), creat() and chmod()
 *
 *	Copyright (c) 1996-2007 J. Schilling
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

#ifndef _SCHILY_FCNTL_H
#define	_SCHILY_FCNTL_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>	/* Needed for fcntl.h			*/
#endif

#ifndef	_SCHILY_STAT_H
#include <schily/stat.h>	/* For 3rd arg of open() and chmod()	*/
#endif

#ifdef	HAVE_SYS_FILE_H
/*
 * Historical systems with flock() only need sys/file.h
 */
#	ifndef	_INCL_SYS_FILE_H
#	include <sys/file.h>
#	define	_INCL_SYS_FILE_H
#	endif
#endif
#ifdef	HAVE_FCNTL_H
#	ifndef	_INCL_FCNTL_H
#	include <fcntl.h>
#	define	_INCL_FCNTL_H
#endif
#endif

/*
 * Do not define more than O_RDONLY / O_WRONLY / O_RDWR / O_BINARY
 * The values may differ.
 *
 * O_BINARY is defined here to allow all applications to compile on a non DOS
 * environment without repeating this definition.
 */
#ifndef	O_RDONLY
#	define	O_RDONLY	0
#endif
#ifndef	O_WRONLY
#	define	O_WRONLY	1
#endif
#ifndef	O_RDWR
#	define	O_RDWR		2
#endif
#ifndef	O_BINARY			/* Only present on DOS or similar */
#	define	O_BINARY	0
#endif
#ifndef	O_NDELAY			/* This is undefined on BeOS :-( */
#	define	O_NDELAY	0
#endif

#ifndef	O_ACCMODE
#define	O_ACCMODE		(O_RDONLY|O_WRONLY|O_RDWR)
#endif

#endif	/* _SCHILY_FCNTL_H */
