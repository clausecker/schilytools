/* @(#)movearch.h	1.3 20/07/08 Copyright 1993, 1995, 2001-2020 J. Schilling */
/*
 *	Handle non-file type data that needs to be moved from/to the archive.
 *
 *	Copyright (c) 1993, 1995, 2001-2020 J. Schilling
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

typedef struct {
	char	*m_data;	/* Pointer to data to be moved ftom/to arch */
	size_t	m_size;		/* Size of data to be moved from/to arch    */
	int	m_flags;	/* Flags holding different move options	    */
} move_t;

#define	MF_ADDSLASH	0x01	/* Add a slash to the data on archive	    */


extern	ssize_t	move_from_arch	__PR((move_t *move, char *p, size_t amount));
extern	ssize_t	move_to_arch	__PR((move_t *move, char *p, size_t amount));

#define	vp_move_from_arch ((ssize_t(*)__PR((void *, char *, size_t)))move_from_arch)
#define	vp_move_to_arch	((ssize_t(*)__PR((void *, char *, size_t)))move_to_arch)
