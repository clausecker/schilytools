/* @(#)nlsdefs.h	1.2 06/12/28 Copyright 2004 J. Schilling */
/*
 *	Copyright (c) 2004 J. Schilling
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

#ifndef	_NLSDEFS_H
#define	_NLSDEFS_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_LIBINTL_H
#ifndef	_INCL_LIBINTL_H
#include <libintl.h>		/* gettext() */
#define	_INCL_LIBINTL_H
#endif
#else
#undef	USE_NLS
#endif

#ifdef	HAVE_LOCALE_H
#ifndef	_INCL_LOCALE_H
#include <locale.h>		/* LC_* definitions */
#define	_INCL_LOCALE_H
#endif
#else
#undef	USE_NLS
#endif

#ifdef	HAVE_LANGINFO_H
#ifndef	_INCL_LAGINFO_H
#include <langinfo.h>		/* YESSTR amnd similar */
#define	_INCL_LANGINFO_H
#endif
#else
#undef	USE_NLS
#endif

#ifndef	HAVE_GETTEXT
#undef	USE_NLS
#endif

#ifdef	NO_NLS
#undef	USE_NLS
#endif


#ifndef	USE_NLS
#undef	gettext
#define	gettext(s)			s
#undef	dgettext
#define	dgettext(d, s)			s
#undef	dcgettext
#define	dcgettext(d, s, c)		s

#undef	textdomain
#define	textdomain(a)			((char *)0)
#undef	bindtextdomain
#define	bindtextdomain(d, dir)		((char *)0)
#undef	bind_textdomain_codeset
#define	bind_textdomain_codeset(d, c)	((char *)0)
#endif

#endif	/* _NLSDEFS_H */
