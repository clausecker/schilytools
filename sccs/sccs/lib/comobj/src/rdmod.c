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
 * @(#)rdmod.c	1.7 11/08/07 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)rdmod.c 1.7 11/08/07 J. Schilling"
#endif
/*
 * @(#)rdmod.c 1.11 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)rdmod.c"
#pragma ident	"@(#)sccs:lib/comobj/rdmod.c"
#endif
# include	<defines.h>

static int	chkix	__PR((struct queue *new, struct queue *head));

int
readmod(pkt)
struct packet *pkt;
{
	char *p;
	int ser, iord, oldixmsg;
	struct apply *ap;

	pkt->p_flags &= ~PF_NONL;
	oldixmsg = pkt->p_ixmsg;
	while (getline(pkt) != NULL) {
	   p = pkt->p_lineptr = pkt->p_line;
	   if (*p++ != CTLCHAR) {
found_esc:
	      if (pkt->p_keep == YES) {
 		 pkt->p_glnno++;
		 if (pkt->p_verbose) {
		    if (pkt->p_ixmsg && oldixmsg == 0) {
		       fprintf(pkt->p_stdout,
			  gettext("include/exclude conflict begins at line %u (co25)\n"),
			  pkt->p_glnno);
		    } else {
		       if (pkt->p_ixmsg == 0 && oldixmsg) {
			  fprintf(pkt->p_stdout,
			     gettext("include/exclude conflict ends at line %u (co26)\n"),
			     pkt->p_glnno);
		       }
		    }
		 }   
		 return(1);
	      }
	   } else {
	      iord = *p++;
	      if (iord == CTLCHAR) {	/* "^A^Atext\n" -> "^Atext\n"	*/
		 pkt->p_lineptr++;
		 goto found_esc;
	      } else if (iord == NONL) { /* "^ANtext\n" -> "text"	*/
		 pkt->p_lineptr += 2;
		 /*
		  * We cannot kill the newline here because of getline()'s auto
		  * copy mode. Just flag the missing newline for our users.
		  */
		 if (pkt->p_keep == YES)
			 pkt->p_flags |= PF_NONL;
		 goto found_esc;
	      } else
	      if (iord != INS && iord != DEL && iord != END)
		 fmterr(pkt);
	      NONBLANK(p);
	      satoi(p, &ser);
	      if (!(ser > 0 && ser <= maxser(pkt)))
		 fmterr(pkt);
	      if (iord == END) {
		 remq(pkt,ser);
	      } else {
		 if ((ap = &pkt->p_apply[ser])->a_code == APPLY) {
		    addq(pkt, ser, iord == INS ? YES : NO, iord,
			ap->a_reason & USER);
		 } else {
		    addq(pkt, ser, iord == INS ? NO : 0, iord,
			ap->a_reason & USER);
		 }
	      }
	   }
	}
	if (pkt->p_q)
	   fatal(gettext("premature eof (co5)"));
	return(0);
}

void addq(pkt, ser, keep, iord, user)
struct packet *pkt;
int ser, keep, iord, user;
{
	struct queue *cur, *prev, *q;

	for (prev = cur = pkt->p_q; cur; cur = (prev = cur)->q_next) {
		if (cur->q_sernum <= ser)
			break;
	}
	if ((cur != NULL) && (cur->q_sernum == ser))
		fmterr(pkt);
	q = (struct queue *)fmalloc(sizeof(*q));
	if (pkt->p_q == cur) {
		pkt->p_q = q;
	} else {
		prev->q_next = q;
	}
	q->q_next = cur;
	q->q_sernum = ser;
	q->q_keep = keep;
	q->q_iord = iord;
	q->q_user = user;
	if (pkt->p_ixuser && (q->q_ixmsg = chkix(q, pkt->p_q))) {
		++(pkt->p_ixmsg);
	} else {
		q->q_ixmsg = 0;
	}
	setkeep(pkt);
}

void remq(pkt, ser)
struct packet *pkt;
int ser;
{
	struct queue *cur, *prev;

	for (prev = cur = pkt->p_q; cur; cur = (prev = cur)->q_next) {
	   if (cur->q_sernum == ser) {
	      if (cur->q_ixmsg)
	         --(pkt->p_ixmsg);
	      if (pkt->p_q == cur) {
	         pkt->p_q = cur->q_next;
	      } else {
	         prev->q_next = cur->q_next;
	      }
	      ffree((char *)cur);
	      setkeep(pkt);
	      return;
	   }
	}
	fmterr(pkt);
}

void setkeep(pkt)
struct packet *pkt;
{
	struct queue *q;
	struct sid *sp;

	for (q = pkt->p_q; q; q = q->q_next) {
		if (q->q_keep != '\0') {
			if ((pkt->p_keep = q->q_keep) == YES) {
				sp = &pkt->p_idel[q->q_sernum].i_sid;
				pkt->p_inssid.s_rel = sp->s_rel;
				pkt->p_inssid.s_lev = sp->s_lev;
				pkt->p_inssid.s_br = sp->s_br;
				pkt->p_inssid.s_seq = sp->s_seq;
			}
			return;
		}
	}
	pkt->p_keep = NO;
}

#define apply(qp)	((qp->q_iord == INS && qp->q_keep == YES) || \
			 (qp->q_iord == DEL && qp->q_keep == NO ))

static int
chkix(new, head)
struct queue *new, *head;
{
	struct queue *cur;
	int retval, firstins, lastdel;

	if (!apply(new))
		return(0);
	for (cur = head; cur; cur = cur->q_next) {
		if (cur->q_user)
			break;
	}
	if (!cur)
		return(0);
	retval = firstins = lastdel = 0;
	for (cur = head; cur; cur = cur->q_next) {
		if (apply(cur)) {
			if (cur->q_iord == DEL) {
				lastdel = cur->q_sernum;
			} else {
				if (firstins == 0)
					firstins = cur->q_sernum;
			}
		} 
		else {
			if (cur->q_iord == INS)
				retval++;
		}
	}
	if (retval == 0) {
		if (lastdel && (new->q_sernum > lastdel))
			retval++;
		if (firstins && (new->q_sernum < firstins))
			retval++;
	}
	return(retval);
}
