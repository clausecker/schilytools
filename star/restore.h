/* @(#)restore.h	1.3 04/09/06 Copyright 2004 J. Schilling */
/*
 *	Data structures used to map old to new inode numbers
 *	when in incremental restore mode.
 *
 *	Copyright (c) 2004 J. Schilling
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

#ifndef	_RESTORE_H
#define	_RESTORE_H

/*
 * Inode mapping from archive and /star-symtable
 */
typedef struct imap imap_t;

struct imap {
/* ? */	imap_t	*i_next;	/* Next in list			*/
	imap_t	*i_hnext;	/* Next in name hash list	*/
	imap_t	*i_honext;	/* Next in old inode hash list	*/
	imap_t	*i_hnnext;	/* Next in new inode hash list	*/
/* ? */	imap_t	*i_dnext;	/* Next Directory in cwd list	*/
	imap_t	*i_dparent;	/* Parent Directory		*/
	imap_t	*i_dir;		/* Directory content		*/
	imap_t	*i_dxnext;	/* Next entry in Directory cont	*/
	char	*i_name;	/* File name			*/
	int	i_hash;		/* File name hash value		*/
	ino_t	i_oino;		/* Old inode number		*/
	ino_t	i_nino;		/* New inode number		*/
	Int32_t	i_flags;	/* Flags (see below)		*/
};

/*
 * Flags for i_flags:
 */
#define	I_DIR		0x01	/* This is a directory		*/
#define	I_NOARCHIVE	0x02	/* Name in archive, file is not	*/
#define	I_DID_RENAME	0x04	/* Entry (dir) has been renamed	*/
#define	I_DELETE	0x80	/* Entry has been deleted	*/

#define	I_NO_INO	(ino_t)-1

#endif	/* _RESTORE_H */
