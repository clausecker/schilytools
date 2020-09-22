/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 1990 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)error.c	1.7 20/09/06 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)error.c 1.7 20/09/06 J. Schilling"
#endif
/*
 * @(#)error.c 1.2 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)error.c"
#pragma ident	"@(#)sccs:lib/cassi/error.c"
#endif

#include <defines.h>

void cmrerror(dummy)
	const char *dummy;
{
	printf("%s\n",dummy);
}
