/* @(#)checkerr.h	1.17 19/02/28 Copyright 2003-2019 J. Schilling */
/*
 *	Generic error control for programs that do file i/o.
 *	The error control is usually used by archiving programs.
 *
 *	The current version does not provide a stable interface.
 *	It does not support multi-threaded programs and it may call
 *	comerr() from the parser. If you use the code before there is
 *	an official stable and "library-compliant" interface, be careful
 *	and watch for changes.
 *
 *	Copyright (c) 2003-2019 J. Schilling
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

#ifndef _SCHILY_CHECKERR_H
#define	_SCHILY_CHECKERR_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_STANDARD_H
#include <schily/standard.h>
#endif
#ifndef	_SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Error conditions handled by error control.
 */
#define	E_STAT		0x0001		/* Could not stat(2) file	   */
#define	E_GETACL	0x0002		/* Could not retrieve ACL info	   */
#define	E_OPEN		0x0004		/* Could not open file		   */
#define	E_READ		0x0008		/* Could not read file		   */
#define	E_WRITE		0x0010		/* Could not write file		   */
#define	E_GROW		0x0020		/* File did grow during backup	   */
#define	E_SHRINK	0x0040		/* File did shrink during backup   */
#define	E_MISSLINK	0x0080		/* Missing hard link(s) for file   */
#define	E_NAMETOOLONG	0x0100		/* File name too long for archive  */
#define	E_FILETOOBIG	0x0200		/* File too big for archive	   */
#define	E_SPECIALFILE	0x0400		/* Improper file type for archive  */
#define	E_READLINK	0x0800		/* Could not read symbolic link	   */
#define	E_GETXATTR	0x1000		/* Could not get xattr		   */
#define	E_CHDIR		0x2000		/* Could not chdir()		   */
#define	E_ICONV		0x4000		/* Could not convert via iconv()   */
#define	E_ID		0x8000		/* Could not write numeric uid/gid */
#define	E_TIME		(0x0001|E_FL1)	/* Could not convert time_t	   */

/*
 * Currently unused: none
 */

#define	E_SETTIME	0x10000		/* Could not set file times	   */
#define	E_SETMODE	0x20000		/* Could not set access modes	   */
#define	E_SECURITY	0x40000		/* Skipped for security reasons	   */
#define	E_LSECURITY	0x80000		/* Link skipped for security	   */
#define	E_SAMEFILE	0x100000	/* Skipped from/to identical	   */
#define	E_BADACL	0x200000	/* ACL string conversion error	   */
#define	E_SETACL	0x400000	/* Could not set ACL for file	   */
#define	E_SETXATTR	0x800000	/* Could not set xattr		   */

/*
 * Unused for flag array index: 0x1000000 .. 0x8000000
 */
#define	E_FL1		0x1000000	/* 1st enhancement bit		   */
#define	E_FL2		0x2000000	/* 2nd enhancement bit		   */
#define	E_FL3		0x3000000	/* 3rd enhancement index	   */
#define	E_FL4		0x4000000	/* 3rd enhancement bit		   */
#define	E_FL5		0x5000000	/* 5th enhancement index	   */
#define	E_FL6		0x6000000	/* 6th enhancement index	   */
#define	E_FL7		0x7000000	/* 7th enhancement index	   */
#define	E_FL8		0x8000000	/* 4th enhancement bit		   */
#define	E_FL9		0x9000000	/* 9th enhancement index	   */
#define	E_FL10		0xA000000	/* 10th enhancement index	   */
#define	E_FL11		0xB000000	/* 11th enhancement index	   */
#define	E_FL12		0xC000000	/* 12th enhancement index	   */
#define	E_FL13		0xD000000	/* 13th enhancement index	   */
#define	E_FL14		0xE000000	/* 14th enhancement index	   */
#define	E_FL15		0xF000000	/* 15th enhancement index	   */
#define	E_EMASK		0xF000000	/* Enhancement mask		   */
#define	E_NFL		16		/* Number of flag array elements   */
#define	E_SHIFT		24		/* Sift to get index		   */

#define	E_DIFF		0x10000000	/* Diffs encountered		   */
#define	E_WARN		0x20000000	/* Print this error but do exit(0) */
#define	E_ABORT		0x40000000	/* Abort on this error		   */
#define	E_EXT		0x80000000	/* Extended (TBD later)		   */

#define	E_ALL		(~(UInt32_t)(E_DIFF|E_ABORT))

extern	int	errconfig	__PR((char *name));
extern	BOOL	errhidden	__PR((int etype, const char *fname));
extern	BOOL	errwarnonly	__PR((int etype, const char *fname));
extern	BOOL	errabort	__PR((int etype, const char *fname,
					BOOL doexit));

#ifdef	__cplusplus
}
#endif

#endif /* _SCHILY_CHECKERR_H */
