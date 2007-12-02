/* @(#)rand_rw.h	1.3 96/06/23 Copyright 1993 J. Schilling */
/*
 *	Copyright (c) 1993 J. Schilling
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

/*
 * Return-CODES
 */
#define	BREAK			(0xFF)
#define	SYS_ERR			(0xF0)
#define	DRV_ERR			(0x0F)

#define	OK			(0x00)

#define	READ_FAULT		(0x01)
#define	WRITE_FAULT		(0x02)
#define	VERIFY_FAULT		(0x04)
#define	SEEK_FAULT		(0x08)

#define	NO_FAULT		(0x10)	/* not reproducable fault */
#define	SOFT_FAULT		(0x20)	/* partially reproducable fault */
#define	HARD_FAULT		(0x40)	/* constant fault */
#define	DATA_LOST		(0x80)

struct bb_list {
	int		count;
	u_long		block;
	u_char		code;
};
