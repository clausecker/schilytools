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
 * Copyright 1998 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)avo_alloca.h 1.4 06/12/12
 */

#pragma ident   "@(#)avo_alloca.h 1.4     06/12/12"

/*
 * Copyright 2017 J. Schilling
 *
 * @(#)avo_alloca.h	1.3 21/08/16 2017 J. Schilling
 */

#ifndef _AVO_ALLOCA_H
#define _AVO_ALLOCA_H

#if defined(SCHILY_BUILD) || defined(SCHILY_INCLUDES)
#include <schily/alloca.h>
#else
#include <alloca.h>

#ifdef __SunOS_5_4
// The following prototype declaration is necessary when compiling on Solaris
// 2.4 using 5.0 compilers. On Solaris 2.4 the necessary prototypes are not
// included in alloca.h. The 4.x compilers provide a workaround by declaring the
// prototype as a pre-defined type. The 5.0 compilers do not implement this workaround.
// This can be removed when support for 2.4 is dropped

#include <stdlib.h> 	// for size_t
extern "C" void *__builtin_alloca(size_t);

#endif  // ifdef __SunOS_5_4
#endif  // defined(SCHILY_BUILD) || defined(SCHILY_INCLUDES)

#endif  // ifdef _AVO_ALLOCA_H

