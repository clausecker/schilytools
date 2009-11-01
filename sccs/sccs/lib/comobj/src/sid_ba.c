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
 * @(#)sid_ba.c	1.2 09/11/01 J. Schilling
 */
#if defined(sun)
#ident "@(#)sid_ba.c 1.2 09/11/01 J. Schilling"
#endif
/*
 * @(#)sid_ba.c 1.3 06/12/12
 */

#if defined(sun)
#ident	"@(#)sid_ba.c"
#ident	"@(#)sccs:lib/comobj/sid_ba.c"
#endif
# include	<defines.h>


char *
sid_ba(sp,p)
register struct sid *sp;
register char *p;
{
	sprintf(p,"%d.%d",sp->s_rel,sp->s_lev);
	while (*p++)
		;
	--p;
	if (sp->s_br) {
		sprintf(p,".%d.%d",sp->s_br,sp->s_seq);
		while (*p++)
			;
		--p;
	}
	return(p);
}
