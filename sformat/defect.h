/* @(#)defect.h	1.3 96/06/23 Copyright 1987 J. Schilling */
/*
 *	An old Sun defect list
 *
 *	Copyright (c) 1987 J. Schilling
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
	u_short	d_magic;
	u_short	d_size;
	struct scsi_def_bfi d_def[127];
}defect;

#define	DMAGIC	0xDEFE

#define	cyl_to_long(a)	((a)[2] | ((a)[1] << 8) | ((a)[0] << 16))
