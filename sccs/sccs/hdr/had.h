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
 * This file contains modifications Copyright 2009-2011 J. Schilling
 *
 * @(#)had.h	1.6 11/06/06 J. Schilling
 */
#ifndef	_HDR_HAD_H
#define	_HDR_HAD_H

#if defined(sun)
#pragma ident "@(#)had.h 1.6 11/06/06 J. Schilling"
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
#ifndef	NLOWER		/* Copy from defines.h */
#define	NLOWER		('z'-'a'+1)
#define	NUPPER		('Z'-'A'+1)
#define	NALPHA		(NLOWER + NUPPER)
#define	LOWER(c)	((c) >= 'a' && (c) <= 'z')
#define	UPPER(c)	((c) >= 'A' && (c) <= 'Z')
#define	ALPHA(c)	(LOWER(c) || UPPER(c))
#endif

#define	HAD_SIZE	(NALPHA + 1)	/* Add one for safety */

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

# define HADUCA had[(NLOWER+'A'-'A')]
# define HADUCB had[(NLOWER+'B'-'A')]
# define HADUCC had[(NLOWER+'C'-'A')]
# define HADUCD had[(NLOWER+'D'-'A')]
# define HADUCE had[(NLOWER+'E'-'A')]
# define HADUCF had[(NLOWER+'F'-'A')]
# define HADUCG had[(NLOWER+'G'-'A')]
# define HADUCH had[(NLOWER+'H'-'A')]
# define HADUCI had[(NLOWER+'I'-'A')]
# define HADUCJ had[(NLOWER+'J'-'A')]
# define HADUCK had[(NLOWER+'K'-'A')]
# define HADUCL had[(NLOWER+'L'-'A')]
# define HADUCM had[(NLOWER+'M'-'A')]
# define HADUCN had[(NLOWER+'N'-'A')]
# define HADUCO had[(NLOWER+'O'-'A')]
# define HADUCP had[(NLOWER+'P'-'A')]
# define HADUCQ had[(NLOWER+'Q'-'A')]
# define HADUCR had[(NLOWER+'R'-'A')]
# define HADUCS had[(NLOWER+'S'-'A')]
# define HADUCT had[(NLOWER+'T'-'A')]
# define HADUCU had[(NLOWER+'U'-'A')]
# define HADUCV had[(NLOWER+'V'-'A')]
# define HADUCW had[(NLOWER+'W'-'A')]
# define HADUCX had[(NLOWER+'X'-'A')]
# define HADUCY had[(NLOWER+'Y'-'A')]
# define HADUCZ had[(NLOWER+'Z'-'A')]


#endif	/* _HDR_HAD_H */
