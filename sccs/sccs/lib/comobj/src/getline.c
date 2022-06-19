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
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)getline.c	1.22 20/09/07 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)getline.c 1.22 20/09/07 J. Schilling"
#endif
/*
 * @(#)getline.c 1.10 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)getline.c"
#pragma ident	"@(#)sccs:lib/comobj/getline.c"
#endif
#include	<defines.h>
#include	<i18n.h>

char 	*getline	__PR((struct packet *pkt));
void	grewind		__PR((struct packet *pkt));

/*
 *	Routine to read a line into the packet.  The main reason for
 *	it is to make sure that pkt->p_wrttn gets turned off,
 *	and to increment pkt->p_slnno.
 */

char *
getline(pkt)
register struct packet *pkt;
{
	int	eof = 0;
#ifndef	NO_GETDELIM
	register ssize_t used = 0;
		size_t line_size;
		char	*line;
#else
	register size_t used = 0;
	register size_t line_size;
	register char	*line;
#endif

	if (pkt->p_wrttn == 0)
		putlline(pkt, (char *) 0, 0);

#ifdef	HAVE_MMAP
	if (pkt->p_mmbase) {
		char	*p;
		ssize_t	siz;

		if (pkt->p_savec) {
			*pkt->p_mmnext = pkt->p_savec;
			pkt->p_savec = '\0';
		}
		siz = pkt->p_mmend - pkt->p_mmnext;
		pkt->p_line = pkt->p_mmnext;
		p = findbytes(pkt->p_mmnext, siz, '\n');
		if (p == NULL) {
			used = siz;
			pkt->p_mmnext = pkt->p_mmend;
		} else {
			used = ++p - pkt->p_line;
			pkt->p_mmnext = p;
			if (pkt->p_mmnext < pkt->p_mmend) {
				/*
				 * We cannot nul terminate the string
				 * if this is past the last byte in the file.
				 * We typically get a SIGSEGV if the last
				 * byte is the last byte in the last MMU page.
				 * XXX: should we check if we are at pageend?
				 */
				pkt->p_savec = *pkt->p_mmnext;
				*pkt->p_mmnext = '\0';
			}
		}
		pkt->p_line_length = used;
		if (used == 0)
			pkt->p_line = "";
		eof = pkt->p_mmnext >= pkt->p_mmend;
	} else
#endif
	{					/* pkt->p_mmbase == NULL */
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
#ifndef	NO_GETDELIM
	/*
	 * getdelim() allows one to read lines that have embedded nul bytes
	 * and we don't need to call strlen().
	 */
	errno = 0;
	used = getdelim(&line, &line_size, '\n', pkt->p_iop);
	if (used == -1) {
		if (errno == ENOMEM)
			fatal(gettext("OUT OF SPACE (ut9)"));
		if (ferror(pkt->p_iop)) {
			xmsg(pkt->p_file, NOGETTEXT("getline"));
		} else if (feof(pkt->p_iop)) {
			used = 0;
			eof = 1;
		}
	}
#else
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
#endif
	pkt->p_line = line;
	pkt->p_linebase = line;
	pkt->p_line_size = line_size;
	pkt->p_line_length = used;
	}					/* pkt->p_mmbase == NULL */

	/* check end of file condition */
	if (eof && (used == 0)) {
		if (!pkt->p_reopen) {
			sclose(pkt);
		}
		if (!pkt->p_chkeof)
			fatal(gettext("premature eof (co5)"));
		if ((pkt->do_chksum && (pkt->p_chash ^ pkt->p_ihash) & 0xFFFF))
		    if (pkt->do_chksum && (pkt->p_uchash ^ pkt->p_ihash) & 0xFFFF)
			fatal(gettext("Corrupted file (co6)"));
		if (pkt->p_reopen) {
			grewind(pkt);
		}
		return (NULL);
	}

	pkt->p_wrttn = 0;
	pkt->p_slnno++;

	if (pkt->no_chksum)
		return (pkt->p_line);

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

void
grewind(pkt)
	register struct packet *pkt;
{
	srewind(pkt);
	pkt->p_onhash = pkt->p_nhash;	/* Remember value, delta(1) needs it */

	pkt->p_reopen = 0;
	pkt->p_slnno = 0;
	pkt->p_ihash = 0;
	pkt->p_chash = 0;
	pkt->p_uchash = 0;
	pkt->p_nhash = 0;
	pkt->p_keep = 0;
	pkt->do_chksum = 0;
	pkt->no_chksum = 0;
}
