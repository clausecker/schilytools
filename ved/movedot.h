/* @(#)movedot.h	1.7 00/12/13 Copyright 1984 J. Schilling */
/*
 *	Definitions for users of the dot moving functions.
 *
 *	Copyright (c) 1984 J. Schilling
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

extern	epos_t	forwword	__PR((ewin_t *wp, epos_t start, ecnt_t n));
extern	epos_t	revword		__PR((ewin_t *wp, epos_t start, ecnt_t n));
extern	epos_t	forwline	__PR((ewin_t *wp, epos_t start, ecnt_t n));
extern	epos_t	revline		__PR((ewin_t *wp, epos_t start, ecnt_t n));
extern	epos_t	forwpara	__PR((ewin_t *wp, epos_t start, ecnt_t n));
extern	epos_t	revpara		__PR((ewin_t *wp, epos_t start, ecnt_t n));
