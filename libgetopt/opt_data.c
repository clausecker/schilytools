/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2019 J. Schilling
 *
 * @(#)opt_data.c	1.4 19/10/23 J. Schilling
 */

#if defined(sun)
#pragma ident	"@(#)opt_data.c	1.11	05/06/08 SMI"
#endif

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

#include <schily/getopt.h>

/*
 * Global variables
 * used in getopt
 */

/*#include "synonyms.h"*/

int	opterr = 1;
int	optind = 1;
int	optopt = 0;
int	optflags = 0;
char	*optarg = 0;
