/* @(#)dsklabel.h	1.6 06/09/13 Copyright 1997 J. Schilling */
/*
 * 	Definitions for disk labels
 *
 *	Copyright (c) 1997 J. Schilling
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

#ifndef	_DSKLABEL_H
#define	_DSKLABEL_H

#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#if	(defined(HAVE_SYS_DKIO_H) && defined(HAVE_SYS_DKLABEL_H)) || \
	(defined(HAVE_SUN_DKIO_H) && defined(HAVE_SUN_DKLABEL_H))
#define	HAVE_DKIO
#endif

#if (defined(sparc) || defined(mc68000)) && defined(sun)
#else
/*
 * Quick hack to disable DKIO on Solais x86
 */
#undef	HAVE_DKIO
#endif

#ifdef	HAVE_DKIO

#	ifdef	HAVE_SYS_DKIO_H
#		include <sys/dkio.h>
#	endif
#	ifdef	HAVE_SYS_DKLABEL_H
#		include <sys/dklabel.h>
#	endif
#	ifdef	HAVE_SUN_DKIO_H
#		include <sun/dkio.h>
#	endif
#	ifdef	HAVE_SUN_DKLABEL_H
#		include <sun/dklabel.h>
#	endif

#else
#	include "sun_dkio.h"
#	include "sun_dklabel.h"
#	undef	DKIOCGCONF
#	define	DKIOCGCONF	12345678
#	undef	DKIOCGAPART
#	define	DKIOCGAPART	12345679
#	undef	DKIOCSAPART
#	define	DKIOCSAPART	12345670
#	undef	DKIOCSGEOM
#	define	DKIOCSGEOM	12345671
#	undef	DKIOCINFO
#	define	DKIOCINFO	12345672
#endif

#endif	/* _DSKLABEL_H */
