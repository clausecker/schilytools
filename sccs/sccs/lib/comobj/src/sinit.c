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
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2011 J. Schilling
 *
 * @(#)sinit.c	1.8 11/08/22 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sinit.c 1.8 11/08/22 J. Schilling"
#endif
/*
 * @(#)sinit.c 1.7 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)sinit.c"
#pragma ident	"@(#)sccs:lib/comobj/sinit.c"
#endif
# include	<defines.h>

/*
	Does initialization for sccs files and packet.
*/

void
sinit(pkt,file,openflag)
register struct packet *pkt;
register char *file;
int openflag;
{
	register char *p;

	zero((char *)pkt, sizeof(*pkt));
	if (size(file) > FILESIZE)
		fatal(gettext("too long (co7)"));
	if (!sccsfile(file))
		fatal(gettext("not an SCCS file (co1)"));
	copy(file,pkt->p_file);
	pkt->p_wrttn = 1;
	pkt->do_chksum = 1;	/* turn on checksum check for getline */
	if (openflag) {
		pkt->p_iop = xfopen(file, O_RDONLY|O_BINARY);
#ifdef	USE_SETVBUF
		setvbuf(pkt->p_iop, NULL, _IOFBF, VBUF_SIZE);
#else
		setbuf(pkt->p_iop, pkt->p_buf);
#endif
		fstat((int)fileno(pkt->p_iop),&Statbuf);
		if (Statbuf.st_nlink > 1)
			fatal(gettext("more than one link (co3)"));
		p = getline(pkt);
		if (p == NULL || (p = checkmagic(pkt, p)) == NULL) {
			if (pkt->p_iop)
				(void) fclose(pkt->p_iop);
			pkt->p_iop = NULL;
			fmterr(pkt);
		}
		p = satoi(p, &pkt->p_ihash);
	}
	pkt->p_chash = 0;
	pkt->p_uchash = 0;
}

char *
checkmagic(pkt, p)
	register struct packet	*pkt;
	register char		*p;
{
	register int	i;
	register int	c;

	if (*p++ != CTLCHAR || *p++ != HEAD)
		return (NULL);

	if (strncmp(p, "V6,sum=", 7) == 0) {
		pkt->p_flags |= PF_V6;
		p += 7;
	}
	for (i = 5; --i >= 0; ) {
		c = *p++ - '0';
		if (c < 0 || c > 9)
			return (NULL);		
	}
	if (*p == '\n')
		return (&p[-5]);
	if ((pkt->p_flags & PF_V6) && *p == ',')
		return (&p[-5]);
	return (NULL);
}
