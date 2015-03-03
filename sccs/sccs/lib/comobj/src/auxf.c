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
 * Copyright 2000 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2009-2015 J. Schilling
 *
 * @(#)auxf.c	1.5 15/02/28 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)auxf.c 1.5 15/02/28 J. Schilling"
#endif
/*
 * @(#)auxf.c 1.5 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)auxf.c"
#pragma ident	"@(#)sccs:lib/comobj/auxf.c"
#endif
# include	<defines.h>


/*
	Figures out names for g-file, l-file, x-file, etc.

	File	Module	g-file	l-file	x-file & rest

	a/s.m	m	m	l.m	a/x.m

	Second argument is letter; 0 means module name is wanted.
*/

char *
auxf(sfile,ch)
register char *sfile;
register char ch;
{
	static char auxfile[FILESIZE];
	register char *snp;

	auxfile[0] = '\0';
	if(sfile[0] == '\0')
		return(auxfile);
	
	snp = sname(sfile);

	switch(ch) {

	case 0:
	case 'g':	/* Basename from g-file derived from sfile */
			strlcpy(auxfile, &snp[2], sizeof (auxfile));
			break;

	case 'A':	/* "g." + Basename from g-file derived from sfile */
			strlcpy(auxfile, "g.", sizeof (auxfile));
			strlcat(auxfile, &snp[2], sizeof (auxfile));
			break;

	case 'l':	/* "l." + Basename from g-file derived from sfile */
			strlcpy(auxfile, snp, sizeof (auxfile));
			auxfile[0] = 'l';
			break;

	case 'G':	/*
			 * Note: called with gfile. not with sfile.
			 * Pathname from gfile with "g." inserted after last '/'
			 */
			strlcpy(auxfile, sfile, sizeof (auxfile));
			if ((snp-sfile) >= sizeof (auxfile))
				auxfile[0] = '\0';
			else
				auxfile[snp-sfile] = '\0';
			strlcat(auxfile, "g.", sizeof (auxfile));
			strlcat(auxfile, snp, sizeof (auxfile));
			break;

	case 'I':	/*
			 * Note: called with sfile.
			 * Pathname from sfile with "s." removed after last '/'
			 */
			strlcpy(auxfile, sfile, sizeof (auxfile));
			if ((snp-sfile) >= sizeof (auxfile))
				auxfile[0] = '\0';
			else
				auxfile[snp-sfile] = '\0';
			strlcat(auxfile, &snp[2], sizeof (auxfile));
			break;

	default:	/* Pathname from sfile with "s." replaced by "<ch>." */
			strlcpy(auxfile, sfile, sizeof (auxfile));
			if ((snp-sfile) >= sizeof (auxfile))
				auxfile[0] = '\0';
			else
				auxfile[snp-sfile] = ch;
	}
	return(auxfile);
}
