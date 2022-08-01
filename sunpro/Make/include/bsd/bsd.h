/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
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
/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)bsd.h 1.6 06/12/12
 */

#pragma	ident	"@(#)bsd.h	1.6	06/12/12"

/*
 * Copyright 2017-2021 J. Schilling
 * Copyright 2022 the schilytools team
 *
 * @(#)bsd.h	1.8 21/08/15 2017-2021 J. Schilling
 */

/*
 * bsd/bsd.h: Interface definitions to BSD compatibility functions for SVR4.
 */

#ifndef _BSD_BSD_H
#define _BSD_BSD_H

/*
 * Some Linux versions come with an incompatible prototype for bsd_signal()
 */
#define	bsd_signal	no_bsd_signal
#if defined(SCHILY_BUILD) || defined(SCHILY_INCLUDES)
#include <schily/signal.h>
#else
#include <signal.h>
#endif
#undef	bsd_signal

#if !defined(SIG_PF)

#ifdef __cplusplus
extern "C" {
	typedef void SIG_FUNC_TYP(int);
}
#else
typedef void SIG_FUNC_TYP(int);
#endif

typedef SIG_FUNC_TYP *SIG_TYP;
#define SIG_PF SIG_TYP
#endif

#ifdef __cplusplus
extern "C" {
	SIG_PF bsd_signal(int a, SIG_PF b);
}
#else
extern void (*bsd_signal(int, void (*) (int))) (int);
#endif
extern void bsd_signals(void);

#endif

