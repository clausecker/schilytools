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
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2008 J. Schilling
 *
 * @(#)date_bal.c	1.2 08/01/05 J. Schilling
 *
 * From Sun: @(#)sccs:lib/comobj/date_ba.c @(#)date_ba.c 1.5 06/12/12
 */
#if defined(sun) || defined(__GNUC__)

#ident "@(#)date_bal.c 1.2 08/01/05 J. Schilling"
#endif
# include	<defines.h>

# define DO2(p,n,c)	*p++ = ((char) ((n)/10) + '0'); *p++ = ( (char) ((n)%10) + '0'); *p++ = c;
# define DO4(p,n,c)	*p++ = ((char) ((n)/1000) + '0'); *p++ = ( (char) ((n)%1000/100) + '0'); \
			*p++ = ((char) ((n)%100/10) + '0'); *p++ = ( (char) ((n)%10) + '0'); *p++ = c;


char *
date_bal(bdt, adt)
time_t	*bdt;
char	*adt;
{
	register struct tm *lcltm;
	register char *p;

#if defined(BUG_1205145) || defined(GMT_TIME)
	lcltm = gmtime(bdt);
#else
	lcltm = localtime(bdt);
#endif
	p = adt;
	lcltm->tm_year += 1900;

	DO4(p,lcltm->tm_year,'/');
	DO2(p,lcltm->tm_mon + 1,'/');
	DO2(p,lcltm->tm_mday,' ');
	DO2(p,lcltm->tm_hour,':');
	DO2(p,lcltm->tm_min,':');
	DO2(p,lcltm->tm_sec,0);
	return(adt);
}
