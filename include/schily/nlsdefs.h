/* @(#)nlsdefs.h	1.7 20/10/09 Copyright 2004-2020 J. Schilling */
/*
 *	Native language support
 *
 *	Copyright (c) 2004-2020 J. Schilling
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

#ifndef	_SCHILY_NLSDEFS_H
#define	_SCHILY_NLSDEFS_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	NO_NLS
#ifndef	USE_NLS
#define	USE_NLS			/* Make nls the default */
#endif
#endif

#ifdef	HAVE_LIBINTL_H
#ifndef	_INCL_LIBINTL_H
#include <libintl.h>		/* gettext() */
#define	_INCL_LIBINTL_H
#endif
#else
#undef	USE_NLS
#endif

#ifndef	_SCHILY_LOCALE_H
#include <schily/locale.h>	/* LC_* definitions */
#endif
#ifndef	_INCL_LOCALE_H
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

#ifdef	NO_TEXT_DOMAIN
#undef	TEXT_DOMAIN
#endif

#ifndef	USE_NLS
#undef	gettext
#define	gettext(s)			((char *)s)
#undef	dgettext
#define	dgettext(d, s)			((char *)s)
#undef	dcgettext
#define	dcgettext(d, s, c)		((char *)s)

#undef	textdomain
#define	textdomain(a)			((char *)0)
#undef	bindtextdomain
#define	bindtextdomain(d, dir)		((char *)0)
#undef	bind_textdomain_codeset
#define	bind_textdomain_codeset(d, c)	((char *)0)
#endif

#ifdef	lint
/*
 * Allow lint to check printf() format strings.
 */
#define	_(s)				s
#else	/* lint */

#ifdef	USE_DGETTEXT			/* e.g. in a library */
#define	_(s)				dgettext(TEXT_DOMAIN, s)
#else
#define	_(s)				gettext(s)
#endif
#endif	/* lint */

#ifdef	NLS_XGETTEXT_MARKER
/*
 * Allow to mark strings for xgettext(1)
 * Since this definition is on conflict with a definition from newer
 * HP-UX versions that has the same purpose as out __PR() macro,
 * we cannot easily use this as long as this is mixed with system include
 * files.
 */
#define	__(s)				s
#endif

#endif	/* _SCHILY_NLSDEFS_H */
