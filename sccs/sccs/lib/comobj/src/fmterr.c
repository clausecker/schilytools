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
 * Copyright 1998 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2011 J. Schilling
 *
 * @(#)fmterr.c	1.6 11/09/02 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)fmterr.c 1.6 11/09/02 J. Schilling"
#endif
/*
 * @(#)fmterr.c 1.5 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)fmterr.c"
#pragma ident	"@(#)sccs:lib/comobj/fmterr.c"
#endif
# include	<defines.h>
# include       <locale.h>

void
fmterr(pkt)
register struct packet *pkt;
{
	extern char SccsError[];

	if (pkt->p_iop)
		(void) fclose(pkt->p_iop);
	pkt->p_iop = NULL;
	if (pkt->p_file[0] != '\0')
		sprintf(SccsError, gettext("%s: format error at line %d (co4)"),
			pkt->p_file,
			pkt->p_slnno);
	else
		sprintf(SccsError, gettext("format error at line %d (co4)"),
			pkt->p_slnno);
	fatal(SccsError);
}
