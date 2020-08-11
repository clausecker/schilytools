/* @(#)getexecpath.c	1.3 20/07/27 Copyright 2006-2020 J. Schilling */
/*
 *	Copyright (c) 2006-2020 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/types.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>

#if (defined(sun) || defined(__sun) || defined(__sun__)) && defined(__SVR4)
#define	PATH_IMPL
#define	METHOD_SYMLINK
#define	SYMLINK_PATH	"/proc/self/path/a.out"	/* Solaris 10 -> ... */
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#define	PATH_IMPL
#define	METHOD_SYMLINK
#define	SYMLINK_PATH	"/proc/curproc/file"	/* /proc may nor be mounted */
#endif

#if defined(__linux__) || defined(__linux)|| defined(linux)
#define	PATH_IMPL
#define	METHOD_SYMLINK
#define	SYMLINK_PATH	"/proc/self/exe"
#endif

#if defined(HAVE_PROC_PIDPATH)			/* Mac OS X */
#ifdef	HAVE_LIBPROC_H
#include <libproc.h>
#endif
#define	PATH_IMPL
#define	METHOD_PROC_PIDPATH
#endif

#if defined(HAVE_SYS_AUXV_H)
/*
 * Methods based on the ELF Aux Vector give the best results.
 */
#include <sys/auxv.h>

#ifdef	HAVE_GETEXECNAME			/* Solaris */
#define	PATH_IMPL
#define	METHOD_GETEXECNAME
#undef	METHOD_SYMLINK
#undef	METHOD_PROC_PIDPATH
#else
#if defined(HAVE_GETAUXVAL) && defined(AT_EXECFN) /* Linux */
#define	PATH_IMPL
#define	METHOD_GETAUXVAL
#undef	METHOD_SYMLINK
#undef	METHOD_PROC_PIDPATH
#else
#if defined(HAVE_ELF_AUX_INFO) && defined(AT_EXECPATH) /* FreeBSD */
#define	PATH_IMPL
#define	METHOD_ELF_AUX_INFO
#undef	METHOD_SYMLINK
#undef	METHOD_PROC_PIDPATH
#else
/*
 * No Solution yet
 */
#endif
#endif
#endif

#endif	/* HAVE_SYS_AUXV_H */



/*
 * TODO: AIX:	/proc/$$/object/a.out	-> plain file, match via st_dev/st_ino
 */


EXPORT char *
getexecpath()
{
#ifdef	PATH_IMPL
#ifdef	METHOD_GETEXECNAME			/* Solaris */
	const char	*en = getexecname();

	if (en == NULL)
		return (NULL);
	return (strdup(en));
#endif
#ifdef	METHOD_GETAUXVAL			/* Linux */
	char	*en = (char *)getauxval(AT_EXECFN);

	if (en == NULL)
		return (NULL);
	return (strdup(en));
#endif
#ifdef	METHOD_ELF_AUX_INFO			/* FreeBSD */
	char	buf[1024];
	int	ret;

	ret = elf_aux_info(AT_EXECPATH, buf, sizeof (buf));
	if (ret != 0)
		return (NULL);
	return (strdup(buf));
#endif
#ifdef	METHOD_SYMLINK
	char	buf[1024];
	ssize_t	len;

	len = readlink(SYMLINK_PATH, buf, sizeof (buf)-1);
	if (len == -1)
		return (NULL);
	buf[len] = '\0';
	return (strdup(buf));
#endif
#ifdef	METHOD_PROC_PIDPATH			/* Mac OS X */
	char	buf[1024];
	int	len;

	len = proc_pidpath(getpid(), buf, sizeof (buf));
	if (len == -1)
		return (NULL);
	return (strdup(buf));
#endif
#else
	return (NULL);
#endif
}
