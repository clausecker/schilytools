/* @(#)exitcodes.h	1.2 06/05/13 Copyright 2002 Heiko Eissfeldt */
/*
 * header file for system wide exit codes.
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

#ifndef	_EXITCODES_H
#define	_EXITCODES_H

#define NO_ERROR	0
#define SYNTAX_ERROR	1
#define PERM_ERROR	2
#define READ_ERROR	3
#define WRITE_ERROR	4
#define SOUND_ERROR	5
#define STAT_ERROR	6
#define SIGPIPE_ERROR	7
#define SETSIG_ERROR	8
#define SHMMEM_ERROR	9
#define NOMEM_ERROR	10
#define MEDIA_ERROR	11
#define DEVICEOPEN_ERROR	12
#define RACE_ERROR	13
#define DEVICE_ERROR	14
#define INTERNAL_ERROR	15
#define SEMAPHORE_ERROR	16
#define SETUPSCSI_ERROR	17
#define PIPE_ERROR	18
#endif
