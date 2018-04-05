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
 * This file contains modifications Copyright 2006-2018 J. Schilling
 *
 * @(#)fatal.h	1.10 18/04/02 J. Schilling
 */
#ifndef	_COMMON_FATAL_H
#define	_COMMON_FATAL_H

#if defined(sun)
#pragma ident "@(#)fatal.h 1.10 18/04/02 J. Schilling"
#endif
/*
 * @(#)fatal.h 1.3 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)fatal.h	1.4	89/10/19"	/* SVr4.0 1.4.1.1	*/
#endif

#include <schily/standard.h>
#include <setjmp.h>

extern	int	Fflags;		/* Flags, see below			  */
extern	char	*Ffile;		/* Filename, (Fflags & FTLMSG) != 0	  */
extern	int	Fvalue;		/* Return value, (Fflags & FTLACT) == FTLRET */
extern	int	(*Ffunc) __PR((char *)); /* Function for Fflags & FTLFUNC */
extern	jmp_buf	Fjmp;		/* Jump buffer for Fflags & FTLJMP	  */
extern  char    *nsedelim;

/*
 * Definitions for Fflags:
 */
# define FTLRECURSE	0200000	/* We have been called recursively	  */
# define FTLMSG		0100000	/* Print "ERROR [filename]: " first	  */
# define FTLCLN		 040000	/* Call clean up function		  */
# define FTLFUNC	 020000	/* Call (*Ffunc)(msg)			  */
# define FTLVFORK	 010000	/* A vfork() child that must call _exit() */
# define FTLACT		    077	/* Mask for fatal() return behavior:	  */
# define FTLJMP		     02	/* Call longjmp(Fjmp, 1);		  */
# define FTLEXIT	     01	/* Call exit();				  */
# define FTLRET		      0	/* Call return(Fvalue);			  */

# define FSAVE(val)	SAVE(Fflags,old_Fflags); Fflags = val;
# define FRSTR()	RSTR(Fflags,old_Fflags);

extern  char    *nse_file_trim __PR((char *, int));

#endif	/* _COMMON_FATAL_H */
