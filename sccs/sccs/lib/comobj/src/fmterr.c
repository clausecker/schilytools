/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
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
 * Copyright 2006-2018 J. Schilling
 *
 * @(#)fmterr.c	1.7 18/11/13 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)fmterr.c 1.7 18/11/13 J. Schilling"
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

	sclose(pkt);
	if (pkt->p_file[0] != '\0')
		sprintf(SccsError, gettext("%s: format error at line %d (co4)"),
			pkt->p_file,
			pkt->p_slnno);
	else
		sprintf(SccsError, gettext("format error at line %d (co4)"),
			pkt->p_slnno);
	fatal(SccsError);
}
