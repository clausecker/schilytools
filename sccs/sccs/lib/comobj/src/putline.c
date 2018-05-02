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
 * Copyright 2006-2018 J. Schilling
 *
 * @(#)putline.c	1.16 18/04/29 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)putline.c 1.16 18/04/29 J. Schilling"
#endif
/*
 * @(#)putline.c 1.13 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)putline.c"
#pragma ident	"@(#)sccs:lib/comobj/putline.c"
#endif
#include	<defines.h>
#include	<i18n.h>

/*
 *	Routine to write out either the current line in the packet
 *	(if newline is zero) or the line specified by newline.
 *	A line is actually written (and the x-file is only
 *	opened) if pkt->p_upd is non-zero.  When the current line from
 *	the packet is written, pkt->p_wrttn is set non-zero, and
 *	further attempts to write it are ignored.  When a line is
 *	read into the packet, pkt->p_wrttn must be turned off.
 */

#define	MAX_LINES	99999	/* Max # of lines that can fit in the */
				/* stats field.  Larger #s will overflow */
				/* and corrupt the file */

static const int signed_chksum = 1;

void
putchr(pkt, c)
register struct packet *pkt;
	int		c;
{
	if (pkt->p_xiop) {
		if (fprintf(pkt->p_xiop, "%c", c) == EOF)
			FAILPUT;
		if (pkt->p_xcreate)
			pkt->p_nhash += c;
	}
}

void
putctl(pkt)
register struct packet *pkt;
{
	putchr(pkt, CTLCHAR);
}

void
putctlnnl(pkt)
register struct packet *pkt;
{
	putchr(pkt, CTLCHAR);
	putchr(pkt, NONL);
}

void
putline(pkt, newline)
register struct packet *pkt;
char *newline;
{
#ifndef	USE_SETVBUF
	static char obf[BUFSIZ];
#endif
	char *xf = (char *) NULL;
	register signed char *p;
	register unsigned char *u_p;

	if (pkt->p_upd == 0)
		return;

	if (!pkt->p_xcreate) {
		/*
		 * Stash away gid and uid from the stat,
		 * as Xfcreat will trash Statbuf.
		 */
		/*
		 * int	gid, uid;
		 */

		(void) stat(pkt->p_file, &Statbuf);
		/*
		 * gid = Statbuf.st_gid;
		 * uid = Statbuf.st_uid;
		 */
		xf = auxf(pkt->p_file, 'x');
		pkt->p_xiop = xfcreat(xf, Statbuf.st_mode);
#ifdef	USE_SETVBUF
		setvbuf(pkt->p_xiop, NULL, _IOFBF, VBUF_SIZE);
#else
		setbuf(pkt->p_xiop, obf);
#endif
	/*
	 * commenting it out as it doesn't do anything useful and creates problems
	 * in networked environment where some platforms allow chown for non root
	 * users.
	 */
#if 0
		chown(xf, uid, gid);
#endif
	}
	if (newline) {
		p = (signed char *)newline;
		u_p = (unsigned char *)newline;
	} else {
		if (pkt->p_wrttn == 0) {
			pkt->p_wrttn++;
			p = (signed char *)pkt->p_line;
			u_p = (unsigned char *)pkt->p_line;
		} else {
			p =  0;
			u_p = 0;
		}
	}
	if (p) {
		if (fputs((const char *)p, pkt->p_xiop) == EOF)
			FAILPUT;
		if (pkt->p_xcreate) {
			if (newline) {
				register int	hash = 0;

				if (signed_chksum) {
					while (*p)
						hash += *p++;
				} else {
					while (*p)
						hash += *u_p++;
				}
				pkt->p_nhash += hash;
			} else {
#ifdef	ALLOW_MODIFIED_LINE
				if (signed_chksum)
					pkt->p_nhash += ssum((char *)p,
							    pkt->p_line_length);
				else
					pkt->p_nhash += usum((char *)p,
							    pkt->p_line_length);
#else
				/*
				 * Use hash from getline()
				 */
				if (signed_chksum)
					pkt->p_nhash += pkt->p_clhash;
				else
					pkt->p_nhash += pkt->p_uclhash;
#endif
			}
		}
	}
	pkt->p_xcreate = 1;
}
int org_ihash;
int org_chash;
int org_uchash;

void
flushline(pkt, stats)
register struct packet *pkt;
register struct stats *stats;
{
	register signed char *p;
	register unsigned char *u_p;
	char *xf = (char *) NULL;
	char ins[6], del[6], unc[6], hash[6];


	if (pkt->p_upd == 0)
		return;
	putline(pkt, (char *) 0);
	rewind(pkt->p_xiop);
	if (stats) {
		if (stats->s_ins > MAX_LINES) {
			stats->s_ins = MAX_LINES;
		}
		if (stats->s_del > MAX_LINES) {
			stats->s_del = MAX_LINES;
		}
		if (stats->s_unc > MAX_LINES) {
			stats->s_unc = MAX_LINES;
		}
		sprintf(ins, "%.05d", stats->s_ins);
		sprintf(del, "%.05d", stats->s_del);
		sprintf(unc, "%.05d", stats->s_unc);
		for (p = (signed char *)ins, u_p = (unsigned char *)ins; *p; p++, u_p++) {
		    if (signed_chksum)
			pkt->p_nhash += (*p - '0');
		    else
			pkt->p_nhash += (*u_p - '0');
		}
		for (p = (signed char *)del, u_p = (unsigned char *)del; *p; p++, u_p++) {
		    if (signed_chksum)
			pkt->p_nhash += (*p - '0');
		    else
			pkt->p_nhash += (*u_p - '0');
		}
		for (p = (signed char *)unc, u_p = (unsigned char *)unc; *p; p++, u_p++) {
		    if (signed_chksum)
			pkt->p_nhash += (*p - '0');
		    else
			pkt->p_nhash += (*u_p - '0');
		}
	}

	sprintf(hash, "%5d", pkt->p_nhash&0xFFFF);
	for (p = (signed char *)hash; *p == ' '; p++)	/* replace initial blanks with '0's */
		*p = '0';
	putmagic(pkt, hash);
	if (stats)
		fprintf(pkt->p_xiop, "%c%c %s/%s/%s\n", CTLCHAR, STATS, ins, del, unc);
	if (fflush(pkt->p_xiop) == EOF)
		xmsg(xf, NOGETTEXT("flushline"));

	/*
	 * Lots of paranoia here, to try to catch
	 * delayed failure information from NFS.
	 */
#ifdef	HAVE_FSYNC
	if (fsync(fileno(pkt->p_xiop)) < 0)
		xmsg(xf, NOGETTEXT("flushline"));
#endif
	if (fclose(pkt->p_xiop) == EOF)
		xmsg(xf, NOGETTEXT("flushline"));
	pkt->p_xiop = NULL;
}

/*
 * Magic at the beginning of SCCS file.
 */
void
putmagic(pkt, hash)
register struct packet	*pkt;
		char	*hash;
{
	char	line[128];

	snprintf(line, sizeof (line), "%c%c%s%s\n", CTLCHAR, HEAD,
		(pkt->p_flags & PF_V6) ? "V6,sum=" : "",
		hash);

	if (pkt->p_xiop) {
		rewind(pkt->p_xiop);
		if (fputs(line, pkt->p_xiop) == EOF)
			FAILPUT;
		return;
	}
	putline(pkt, line);	/* First write does not change hash */
}

void
xrm(pkt)
	register struct packet	*pkt;
{
	if (pkt->p_xiop)
		(void) fclose(pkt->p_xiop);
	pkt->p_xiop = 0;
	pkt->p_xcreate = 0;
}
