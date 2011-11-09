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
 * @(#)getline.c	1.15 11/09/14 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)getline.c 1.15 11/09/14 J. Schilling"
#endif
/*
 * @(#)getline.c 1.10 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)getline.c"
#pragma ident	"@(#)sccs:lib/comobj/getline.c"
#endif
#include	<defines.h>

/*
 *	Routine to read a line into the packet.  The main reason for
 *	it is to make sure that pkt->p_wrttn gets turned off,
 *	and to increment pkt->p_slnno.
 */

char *
getline(pkt)
register struct packet *pkt;
{
	int	eof;
	register size_t used = 0;
	register size_t line_size;
	register char	*line;

	if (pkt->p_wrttn == 0)
		putline(pkt, (char *) 0);

	line_size = pkt->p_line_size;
	if (line_size == 0) {
		line_size = DEF_LINE_SIZE;
		pkt->p_line = (char *) malloc(line_size);
		if (pkt->p_line == NULL)
			fatal(gettext("OUT OF SPACE (ut9)"));
	}
	/* read until EOF or newline encountered */
	line = pkt->p_line;
	line[0] = '\0';
	do {
		line[line_size - 1] = '\t';	/* arbitrary non-zero char */
		line[line_size - 2] = ' ';	/* arbitrary non-newline char */
		if (!(eof = (fgets(line+used,
				    line_size-used,
				    pkt->p_iop) == NULL))) {

			if (line[line_size - 1] != '\0' ||
			    line[line_size - 2] == '\n')
				break;

			used = line_size - 1;

			line_size += DEF_LINE_SIZE;
			line = (char *) realloc(line, line_size);
			if (line == NULL) {
				fatal(gettext("OUT OF SPACE (ut9)"));
			}
		}
	} while (!eof);
	used += strlen(&line[used]);
	pkt->p_line = line;
	pkt->p_linebase = line;
	pkt->p_line_size = line_size;
	pkt->p_line_length = used;

	/* check end of file condition */
	if (eof && (used == 0)) {
		if (!pkt->p_reopen) {
			if (pkt->p_iop)
				(void) fclose(pkt->p_iop);
			pkt->p_iop = 0;
		}
		if (!pkt->p_chkeof)
			fatal(gettext("premature eof (co5)"));
		if ((pkt->do_chksum && (pkt->p_chash ^ pkt->p_ihash) & 0xFFFF))
		    if (pkt->do_chksum && (pkt->p_uchash ^ pkt->p_ihash) & 0xFFFF)
			fatal(gettext("Corrupted file (co6)"));
		if (pkt->p_reopen) {
			rewind(pkt->p_iop);
			pkt->p_reopen = 0;
			pkt->p_slnno = 0;
			pkt->p_ihash = 0;
			pkt->p_chash = 0;
			pkt->p_uchash = 0;
			pkt->p_nhash = 0;
			pkt->p_keep = 0;
			pkt->do_chksum = 0;
		}
		return (NULL);
	}

	pkt->p_wrttn = 0;
	pkt->p_slnno++;

	/* update check sum */
	{
		register signed char	*p;
		register int		c;
		register int		ch = 0;
		register unsigned	uch = 0;
		register int		len = used;

#define	DO8(a)	a; a; a; a; a; a; a; a;

		for (p = (signed char *)pkt->p_line; len >= 8; len -= 8) {
			DO8(c = *p++; ch += c; uch += (unsigned char)c);
		}

		switch (len) {

		case 7:	c = *p++; ch += c; uch += (unsigned char)c;
		case 6:	c = *p++; ch += c; uch += (unsigned char)c;
		case 5:	c = *p++; ch += c; uch += (unsigned char)c;
		case 4:	c = *p++; ch += c; uch += (unsigned char)c;
		case 3:	c = *p++; ch += c; uch += (unsigned char)c;
		case 2:	c = *p++; ch += c; uch += (unsigned char)c;
		case 1:	c = *p++; ch += c; uch += (unsigned char)c;
		}

		pkt->p_clhash = ch;
		pkt->p_uclhash = uch;
		pkt->p_chash += ch;
		pkt->p_uchash += uch;
	}

	return (pkt->p_line);
}
