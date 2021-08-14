#ifndef _MKSH_MACRO_H
#define _MKSH_MACRO_H
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
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)macro.h 1.3 06/12/12
 */

#pragma	ident	"@(#)macro.h	1.3	06/12/12"

/*
 * Copyright 2021 J. Schilling
 *
 * @(#)macro.h	1.3 21/08/10 2021 J. Schilling
 */

#include <mksh/defs.h>

/*
 * The expansion type for expand_macro() and expand_value().
 * 
 */
typedef enum {
	deflt_expand =	0,	/* The default macro expansion behavior	    */
	no_expand =	1,	/* Do not expand GNU type ::= macros	    */
	keep_ddollar =	2	/* Keep $$ if doing :::= or +:= assignment  */
} Expand_Type; 

extern void	expand_macro(register Source source, register String destination, wchar_t *current_string, Boolean cmd, Expand_Type exp_type = deflt_expand);
extern void	expand_value(Name value, register String destination, Boolean cmd, Expand_Type exp_type = deflt_expand);
extern Name	getvar(register Name name);

extern Property	setvar_daemon(register Name name, register Name value, Boolean append, Daemon daemon, Boolean strip_trailing_spaces, short debug_level);

#endif
