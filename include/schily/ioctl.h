/* @(#)ioctl.h	1.2 21/07/14 Copyright 2007-2021 J. Schilling */
/*
 *	Abstraction from sys/ioctl.h
 *
 *	Copyright (c) 2007-2021 J. Schilling
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

#ifndef _SCHILY_IOCTL_H
#define	_SCHILY_IOCTL_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#if	defined(OS390) || defined(__MVS__)
/*
 * z/OS is inconsistent:
 *	sys/types.h defines u_int only if _ALL_SOURCE is #defined,
 *	but sys/ioctl.h defines it unconditionally.
 */
#ifdef	u_int
#undef	u_int
#endif
#endif

#ifdef	HAVE_SYS_IOCTL_H
#ifndef	_INCL_SYS_IOCTL_H
#include <sys/ioctl.h>
#define	_INCL_SYS_IOCTL_H
#endif
#endif

#endif	/* _SCHILY_IOCTL_H */
