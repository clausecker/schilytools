/* @(#)termios.h	1.31 07/05/24 Copyright 1984-2007 J. Schilling */
/*
 *	Terminal driver tty mode handling
 *
 *	Copyright (c) 1984-2007 J. Schilling
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


#ifndef	_SCHILY_TERMIOS_H
#define	_SCHILY_TERMIOS_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifdef	JOS
#	include <ttymodes.h>
#	include <spfcode.h>
#else
#ifdef	HAVE_TERMIOS_H
#	include <termios.h>
#	define	_INCL_TERMIOS_H
#	ifdef TIOCGETA				/* FreeBSD */
#		define	TCGETS	TIOCGETA
#		define	TCSETSW	TIOCSETAW
#	endif
#	ifdef TCGETATTR
#		define	TCGETS	TCGETATTR
#		define	TCSETSW	TCSETATTRD
#	endif
#else
#	ifdef	HAVE_TERMIO_H
#		include	<termio.h>
#		define	_INCL_TERMIO_H
#		ifndef	TCGETS
#		define	termios	termio
#		define	TCGETS	TCGETA
#		define	TCSETSW	TCSETAW
#		endif
#	else
#		define	USE_V7_TTY
#	endif
#endif
#endif

#if !defined(HAVE_TCGETATTR) || !defined(HAVE_TCSETATTR)
#	undef	TCSANOW
#endif

#ifndef	TCSANOW
#	if	!defined(TCGETS) || !defined(TCSETSW)
#		define	USE_V7_TTY
#	endif
#endif

#if !defined(_INCL_TERMIOS_H) && !defined(_INCL_TERMIO_H)
#	include	<schily/ioctl.h>
#endif

#ifdef	HAVE_SYS_BSDTTY_H
#include <sys/bsdtty.h>
#endif

#if	!defined(TIOCGWINSZ) && ! defined(TIOCGSIZE)
#	include	<schily/ioctl.h>
#endif

#ifndef	OXTABS					/* OS/2 EMX */
#define	OXTABS	0
#endif
#ifndef	XTABS
#	ifndef	TAB3				/* FreeBSD */
#	define	TABDLY	OXTABS
#	define	XTABS	OXTABS
#	else
#	define	XTABS	TAB3
#	endif
#endif
#ifndef	ONLCR					/* OS/2 EMX */
#define	ONLCR	0
#endif
#ifndef	OCRNL					/* FreeBSD */
#	define	OCRNL	0
#endif
#ifndef	ONLRET					/* FreeBSD */
#	define	ONLRET	0
#endif
#ifndef	_POSIX_VDISABLE
#	define	_POSIX_VDISABLE	0
#endif

#endif	/* _SCHILY_TERMIOS_H */
