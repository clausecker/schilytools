/* @(#)movearch.h	1.2 03/06/12 Copyright 1993, 1995, 2001-2003 J. Schilling */
/*
 *	Handle non-file type data that needs to be moved from/to the archive.
 *
 *	Copyright (c) 1993, 1995, 2001-2003 J. Schilling
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

typedef struct {
	char	*m_data;	/* Pointer to data to be moved ftom/to arch */
	int	m_size;		/* Size of data to be moved from/to arch    */
	int	m_flags;	/* Flags holding different move options	    */
} move_t;

#define	MF_ADDSLASH	0x01	/* Add a slash to the data on archive	    */


extern	int	move_from_arch	__PR((move_t *move, char *p, int amount));
extern	int	move_to_arch	__PR((move_t *move, char *p, int amount));

#define	vp_move_from_arch ((int(*)__PR((void *, char *, int)))move_from_arch)
#define	vp_move_to_arch	((int(*)__PR((void *, char *, int)))move_to_arch)
