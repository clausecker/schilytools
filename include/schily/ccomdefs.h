/* @(#)ccomdefs.h	1.4 06/09/13 Copyright 2000 J. Schilling */
/*
 *	Various compiler dependant macros.
 *
 *	Copyright (c) 2000 J. Schilling
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

#ifndef _SCHILY_CCOMDEFS_H
#define	_SCHILY_CCOMDEFS_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Compiler-dependent macros to declare that functions take printf-like
 * or scanf-like arguments. They are defined to nothing for versions of gcc
 * that are not known to support the features properly (old versions of gcc-2
 * didn't permit keeping the keywords out of the application namespace).
 */
#if __GNUC__ < 2 || __GNUC__ == 2 && __GNUC_MINOR__ < 7

#define	__printflike__(fmtarg, firstvararg)
#define	__printf0like__(fmtarg, firstvararg)
#define	__scanflike__(fmtarg, firstvararg)

#else /* We found GCC that supports __attribute__ */

#define	__printflike__(fmtarg, firstvararg) \
		__attribute__((__format__(__printf__, fmtarg, firstvararg)))
#define	__printf0like__(fmtarg, firstvararg) \
		__attribute__((__format__(__printf0__, fmtarg, firstvararg)))

/*
 * FreeBSD GCC implements printf0 that allows the format string to
 * be a NULL pointer.
 */
#if	__FreeBSD_cc_version < 300001
#undef	__printf0like__
#define	__printf0like__	__printflike__
#endif

#define	__scanflike__(fmtarg, firstvararg) \
		__attribute__((__format__(__scanf__, fmtarg, firstvararg)))

#endif /* GNUC */

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_CCOMDEFS_H */
