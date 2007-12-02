/* @(#)unistd.h	1.19 07/01/16 Copyright 1996-2007 J. Schilling */
/*
 *	Definitions for unix system interface
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

#ifndef _SCHILY_UNISTD_H
#define	_SCHILY_UNISTD_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * unistd.h grants things like off_t to be typedef'd.
 */
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifdef	HAVE_UNISTD_H

#ifndef	_INCL_UNISTD_H
#include <unistd.h>
#define	_INCL_UNISTD_H
#endif

#ifndef	_SC_PAGESIZE
#ifdef	_SC_PAGE_SIZE	/* HP/UX & OSF */
#define	_SC_PAGESIZE	_SC_PAGE_SIZE
#endif
#endif

#endif	/* HAVE_UNISTD_H */

/*
 * MSVC has getcwd()/chdir()/mkdir()/rmdir() in direct.h
 */
#if defined(_MSC_VER) && defined(HAVE_DIRECT_H)
#ifndef	_INCL_DIRECT_H
#include <direct.h>
#define	_INCL_DIRECT_H
#endif
#endif

/*
 * MSVC has size_t in stddef.h
 */
#ifdef HAVE_STDDEF_H
#ifndef	_INCL_STDDEF_H
#include <stddef.h>
#define	_INCL_STDDEF_H
#endif
#endif

#ifndef	STDIN_FILENO
#	ifdef	JOS
#		ifndef	_SCHILY_JOS_IO_H
#		include <schily/jos_io.h>
#		endif
#	else
#		define	STDIN_FILENO	0
#		define	STDOUT_FILENO	1
#		define	STDERR_FILENO	2
#	endif
#endif

#ifndef	R_OK
/* Symbolic constants for the "access" routine: */
#define	R_OK	4	/* Test for Read permission */
#define	W_OK	2	/* Test for Write permission */
#define	X_OK	1	/* Test for eXecute permission */
#define	F_OK	0	/* Test for existence of File */
#endif
#ifndef	E_OK
#ifdef	HAVE_ACCESS_E_OK
#ifdef	EFF_ONLY_OK
#define	E_OK	EFF_ONLY_OK /* Irix */
#else
#ifdef	EUID_OK
#define	E_OK	EUID_OK	/* UNICOS (0400) */
#else
#define	E_OK	010	/* Test effective uids */
#endif	/* EUID_OK */
#endif	/* EFF_ONLY_OK */
#else
#define	E_OK	0
#endif	/* HAVE_ACCESS_E_OK */
#endif	/* !E_OK */

/* Symbolic constants for the "lseek" routine: */
#ifndef	SEEK_SET
#define	SEEK_SET	0	/* Set file pointer to "offset" */
#endif
#ifndef	SEEK_CUR
#define	SEEK_CUR	1	/* Set file pointer to current plus "offset" */
#endif
#ifndef	SEEK_END
#define	SEEK_END	2	/* Set file pointer to EOF plus "offset" */
#endif

#if	!defined(HAVE_UNISTD_H) || !defined(_POSIX_VERSION)
/*
 * Maybe we need a lot more definitions here...
 * It is not clear whether we should have prototyped definitions.
 */
extern	int	access	__PR((const char *, int));
extern	int	close	__PR((int));
extern	int	dup	__PR((int));
extern	int	dup2	__PR((int, int));
#ifndef	_MSC_VER
/*
 * MS C comes with broken prototypes in wrong header files (in our case, the
 * wrong prototype is in stdlib.h). Avoid to redefine the broken MS stuff with
 * correct prototypes.
 */
extern	void	_exit	__PR((int));
#endif
extern	int	link	__PR((const char *, const char *));
extern	int	read	__PR((int, void *, size_t));
extern	int	unlink	__PR((const char *));
extern	int	write	__PR((int, const void *, size_t));
#endif

#endif	/* _SCHILY_UNISTD_H */
