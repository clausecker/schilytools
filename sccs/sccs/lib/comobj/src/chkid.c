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
 * @(#)chkid.c	1.8 18/04/29 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)chkid.c 1.8 18/04/29 J. Schilling"
#endif
/*
 * @(#)chkid.c 1.5 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)chkid.c"
#pragma ident	"@(#)sccs:lib/comobj/chkid.c"
#endif
#include	<defines.h>
#include	<ctype.h>


int
chkid(line, idstr, sflags)
	char	*line;
	char	*idstr;
	char	*sflags[];
{
	register char	*lp;
	register char	*p;
		int	expand_XIDs;
		char	*list_expand_IDs;
		int	Did_id = 0;

	if ((lp = strchr(line, '%')) == NULL)
		return (Did_id);

	if (idstr && idstr[0] != '\0' && (lp = strchr(idstr, '%')) == NULL)
		return (Did_id);

	list_expand_IDs = sflags[EXPANDFLAG - 'a'];
	expand_XIDs = sflags[EXTENSFLAG - 'a'] != NULL ||
			list_expand_IDs != NULL;

	for (; *lp != 0; lp++) {
		if (lp[0] == '%' && lp[1] != 0 && lp[2] == '%')
			if (list_expand_IDs != NULL) {
				if (!any(lp[1], list_expand_IDs)) {
					lp++;
					continue;
				}
			}
			switch (lp[1]) {
			case 'A': case 'B': case 'C': case 'D':
			case 'E': case 'F': case 'G': case 'H':
			case 'I': case 'L': case 'M': case 'P':
			case 'Q': case 'R': case 'S': case 'T':
			case 'U': case 'W': case 'Y': case 'Z':
				Did_id = 1;
				break;
			case 'd': case 'e': case 'g': case 'h':
				if (expand_XIDs)
					Did_id = 1;
				break;
			default:
				break;
			}
	}
	if (!Did_id)
		return (Did_id);	/* There's no keyword in idstr */
	if (!idstr || idstr[0] == '\0')
		return (Did_id);
	Did_id = 0;
	p = idstr;
	lp = line;
	while ((lp = strchr(lp, *p)) != NULL)
		if (!(strncmp(lp, p, strlen(p))))
			return (++Did_id);
		else
			++lp;
	return (Did_id);
}
