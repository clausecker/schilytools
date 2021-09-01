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
 * Copyright 2001 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)intl.h 1.19 06/12/12
 */

#pragma	ident	"@(#)intl.h	1.19	06/12/12"

/*
 * Copyright 2017 J. Schilling
 *
 * @(#)intl.h	1.3 21/08/16 2017 J. Schilling
 */

#ifndef _AVO_INTL_H
#define _AVO_INTL_H

#if defined(SUN4_x) || defined(HP_UX)
#include <avo/widefake.h>
#endif

#ifdef HP_UX
#ifdef __cplusplus
#ifndef _STDLIB_INCLUDED
#include <stdlib.h>             /* for wchar_t definition and HP-UX - */
#endif                          /* wide character function prototypes. */
extern "C" {
char *gettext(char *msg);
char *dgettext(const char *, const char *);
char *bindtextdomain(const char *, const char *);
char *textdomain(char *);
}
#endif /* __cplusplus */
#endif

/*
 * NOCATGETS is a dummy macro that returns it argument.
 * It is used to identify strings that we consciously do not
 * want to apply catgets() to.  We have tools that check the
 * sources for strings that are not catgets'd and the tools
 * ignore strings that are NOCATGETS'd.
 */
#define	NOCATGETS(str)	(str)

/*
 * Define the various text domains
 */
#define	AVO_DOMAIN_CODEMGR	"codemgr"
#define AVO_DOMAIN_VERTOOL	"vertool"
#define AVO_DOMAIN_FILEMERGE	"filemerge"
#define AVO_DOMAIN_DMAKE	"dmake"
#define AVO_DOMAIN_PMAKE	"pmake"
#define AVO_DOMAIN_FREEZEPOINT	"freezept"
#define AVO_DOMAIN_MAKETOOL	"maketool"

#endif
