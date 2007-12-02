/* @(#)xconfig.h	1.6 07/07/01 Copyright 1995-2007 J. Schilling */
/*
 *	This file either includes the dynamic or manual autoconf stuff.
 *
 *	Copyright (c) 1995-2007 J. Schilling
 *
 *	This file is included from <schily/mconfig.h> and usually
 *	includes $(SRCROOT)/incs/$(OARCH)/xconfig.h via
 *	-I$(SRCROOT)/incs/$(OARCH)/
 *
 *	Use only cpp instructions.
 *
 *	NOTE: SING: (Schily Is Not Gnu)
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

#ifndef _SCHILY_XCONFIG_H
#define	_SCHILY_XCONFIG_H

/*
 * This hack that is needed as long as VMS has no POSIX shell.
 * It will go away soon. VMS users: in future you need to specify:
 * cc -DUSE_STATIC_CONF
 */
#ifdef	VMS
#	define	USE_STATIC_CONF
#endif

#ifdef	USE_STATIC_CONF
#	include <schily/xmconfig.h>	/* The static autoconf stuff */
#else	/* USE_STATIC_CONF */


#ifdef	SCHILY_BUILD	/* #defined by Schily makefile system */
	/*
	 * Include $(SRCROOT)/incs/$(OARCH)/xconfig.h via
	 * -I$(SRCROOT)/incs/$(OARCH)/
	 */
#	include <xconfig.h>	/* The current dynamic autoconf stuff */
#else	/* !SCHILY_BUILD */
/*
 * The stuff for static compilation. Include files from a previous
 * dynamic autoconfiguration.
 */
#ifdef	__SUNOS5_SPARC_CC32
#include <schily/sparc-sunos5-cc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_SPARC_CC64
#include <schily/sparc-sunos5-cc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_SPARC_GCC32
#include <schily/sparc-sunos5-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_SPARC_GCC64
#include <schily/sparc-sunos5-gcc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_X86_CC32
#include <schily/i386-sunos5-cc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_X86_CC64
#include <schily/i386-sunos5-cc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_X86_GCC32
#include <schily/i386-sunos5-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__SUNOS5_X86_GCC64
#include <schily/i386-sunos5-gcc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif

#ifdef	__HPUX_HPPA_CC32
#include <schily/hppa-hp-ux-cc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__HPUX_HPPA_CC64
#include <schily/hppa-hp-ux-cc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__HPUX_HPPA_GCC32
#include <schily/hppa-hp-ux-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif
#ifdef	__HPUX_HPPA_GCC64
#include <schily/hppa-hp-ux-gcc64/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif

#ifdef	__MSWIN_X86_CL32
#include <schily/i686-cygwin32_nt-cl/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif

#ifdef	__CYGWIN_X86_GCC
#include <schily/i686-cygwin32_nt-gcc/xconfig.h>
#define	__JS_ARCH_CONF_INCL
#endif

#ifndef	__JS_ARCH_CONF_INCL
Error unconfigured architecture
#endif

#endif	/* SCHILY_BUILD */

#endif	/* USE_STATIC_CONF */

#endif /* _SCHILY_XCONFIG_H */
