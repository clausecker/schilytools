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
 * This file contains modifications Copyright 2009 J. Schilling
 *
 * @(#)had.h	1.3 09/11/08 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)had.h 1.3 09/11/08 J. Schilling"
#endif
/*
 * @(#)had.h 1.4 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)had.h"
#pragma ident	"@(#)sccs:hdr/had.h"
#endif
/*
 */
#define	HAD_SIZE	27
extern char 	had[HAD_SIZE];	/* one extre for luch (val clears had[26] ??? */

# define HADA had[('a'-'a')]
# define HADB had[('b'-'a')]
# define HADC had[('c'-'a')]
# define HADD had[('d'-'a')]
# define HADE had[('e'-'a')]
# define HADF had[('f'-'a')]
# define HADG had[('g'-'a')]
# define HADH had[('h'-'a')]
# define HADI had[('i'-'a')]
# define HADJ had[('j'-'a')]
# define HADK had[('k'-'a')]
# define HADL had[('l'-'a')]
# define HADM had[('m'-'a')]
# define HADN had[('n'-'a')]
# define HADO had[('o'-'a')]
# define HADP had[('p'-'a')]
# define HADQ had[('q'-'a')]
# define HADR had[('r'-'a')]
# define HADS had[('s'-'a')]
# define HADT had[('t'-'a')]
# define HADU had[('u'-'a')]
# define HADV had[('v'-'a')]
# define HADW had[('w'-'a')]
# define HADX had[('x'-'a')]
# define HADY had[('y'-'a')]
# define HADZ had[('z'-'a')]
