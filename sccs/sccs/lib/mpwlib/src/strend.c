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
 * Copyright 1994 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2007 J. Schilling
 *
 * @(#)strend.c	1.3 07/01/11 J. Schilling
 */
#if defined(sun) || defined(__GNUC__)

#ident "@(#)strend.c 1.3 07/01/11 J. Schilling"
#endif
/*
 * @(#)strend.c 1.3 06/12/12
 */

#ident	"@(#)strend.c"
#ident	"@(#)sccs:lib/mpwlib/strend.c"

#include <defines.h>

char *strend(p)
register char *p;
{
	while (*p++)
		;
	return(--p);
}
