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
 * This file contains modifications Copyright 2006-2018 J. Schilling
 *
 * @(#)setup.c	1.7 18/03/15 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)setup.c 1.7 18/03/15 J. Schilling"
#endif
/*
 * @(#)setup.c 1.5 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)setup.c"
#pragma ident	"@(#)sccs:lib/comobj/setup.c"
#endif
# include	<defines.h>

static void	ixgsetup	__PR((struct apply *ap, struct ixg *ixgp));

void
setup(pkt,serial)
register struct packet *pkt;
int serial;
{
	register int n;
	register struct apply *rap;
	int	first_app   =   1;

	pkt->p_apply[serial].a_inline = 1;
	for (n = maxser(pkt); n; n--) {
		rap = &pkt->p_apply[n];
		if (rap->a_inline) {
			if (n != 1 && pkt->p_idel[n].i_pred == 0)
				fmterr(pkt);
			pkt->p_apply[pkt->p_idel[n].i_pred].a_inline = 1;
			if (pkt->p_idel[n].i_datetime.tv_sec > pkt->p_cutoff)
				condset(rap,NOAPPLY,CUTOFF);
			else {
				if (first_app)
					pkt->p_gotsid = pkt->p_idel[n].i_sid;
				first_app = 0;
				condset(rap,APPLY,SX_EMPTY);
			}
		}
		else
			condset(rap,NOAPPLY,SX_EMPTY);
		if (rap->a_code == APPLY) {
			ixgsetup(pkt->p_apply,pkt->p_idel[n].i_ixg);
		}
	}
}


static void
ixgsetup(ap,ixgp)
struct apply *ap;
struct ixg *ixgp;
{
	int n;
	int code = 0, reason = 0;
	register int *ip;
	register struct ixg *cur;

	for (cur = ixgp; cur != NULL; cur = cur->i_next) {
		switch (cur->i_type) {

		case INCLUDE:
			code = APPLY;
			reason = INCL;
			break;
		case EXCLUDE:
			code = NOAPPLY;
			reason = EXCL;
			break;
		case IGNORE:
			/*
			code = SX_EMPTY;
			reason = IGNR;
			*/
			code = NOAPPLY;
			reason = EXCL;
			break;
		}
		ip = cur->i_ser;
		for (n = cur->i_cnt; n; n--)
			condset(&ap[*ip++],code,reason);
	}
}


void
condset(ap,code,reason)
register struct apply *ap;
int code, reason;
{
	if (code == SX_EMPTY)
		ap->a_reason |= reason;
	else if (ap->a_code == SX_EMPTY) {
		ap->a_code = code;
		ap->a_reason |= reason;
	}
}
