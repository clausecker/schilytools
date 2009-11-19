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
 * Copyright 1995 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2009 J. Schilling
 *
 * @(#)repl.c	1.5 09/11/08 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)repl.c 1.5 09/11/08 J. Schilling"
#endif
/*
 * @(#)repl.c 1.4 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)repl.c"
#pragma ident	"@(#)sccs:lib/mpwlib/repl.c"
#endif
/*
	Replace each occurrence of `old' with `new' in `str'.
	Return `str'.
*/

#include <defines.h>

#ifdef	PROTOTYPES
char *
repl(char *str, char old, char new)
#else
char *
repl(str,old,new)
char *str;
char old,new;
#endif
{
	char		old_str[2];
	
	old_str[0] = old;
	old_str[1] = '\0';
	return(trnslat(str, old_str, &new, str));
}
