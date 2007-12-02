/* @(#)schilyp.h	1.1 07/02/13 Copyright 2007 J. Schilling */
/*
 *	Include definitions for libschily and define *printf() -> js_*printf()
 *
 *	Copyright (c) 2007 J. Schilling
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

#ifndef _SCHILY_SCHILYP_H
#define	_SCHILY_SCHILYP_H

#ifndef _SCHILY_SCHILY_H
#include <schily/schily.h>
#endif

/*
 * Always use js_*printf()
 */
#undef	fprintf
#define	fprintf		js_fprintf
#undef	printf
#define	printf		js_printf
#undef	snprintf
#define	snprintf	js_snprintf
#undef	sprintf
#define	sprintf		js_sprintf

#endif	/* _SCHILY_SCHILYP_H */
