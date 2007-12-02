/* @(#)wm_track.c	1.5 06/09/13 Copyright 1995, 1997, 2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)wm_track.c	1.5 06/09/13 Copyright 1995, 1997, 2004 J. Schilling";
#endif
/*
 *	CDR write method abtraction layer
 *	track at once writing intercace routines
 *
 *	Copyright (c) 1995, 1997, 2004 J. Schilling
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

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/utypes.h>

#include "cdrecord.h"

extern	int	debug;
extern	int	verbose;
extern	int	lverbose;

extern	char	*buf;			/* The transfer buffer */

EXPORT	int	write_track_data __PR((cdr_t *dp, int track, track_t *trackp));
