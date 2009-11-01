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
 * @(#)fatal.h	1.5 09/11/01 J. Schilling
 */
#if defined(sun)
#ident "@(#)fatal.h 1.5 09/11/01 J. Schilling"
#endif
/*
 * @(#)fatal.h 1.3 06/12/12
 */

#if defined(sun)
#ident	"@(#)fatal.h	1.4	89/10/19"	/* SVr4.0 1.4.1.1	*/
#endif

#include <schily/standard.h>
#include <setjmp.h>

extern	int	Fflags;
extern	char	*Ffile;
extern	int	Fvalue;
extern	int	(*Ffunc) __PR((char *));
extern	jmp_buf	Fjmp;
extern  char    *nsedelim;

# define FTLMSG		0100000
# define FTLCLN		 040000
# define FTLFUNC	 020000
# define FTLACT		    077
# define FTLJMP		     02
# define FTLEXIT	     01
# define FTLRET		      0

# define FSAVE(val)	SAVE(Fflags,old_Fflags); Fflags = val;
# define FRSTR()	RSTR(Fflags,old_Fflags);

extern  char    *nse_file_trim __PR((char *, int));
