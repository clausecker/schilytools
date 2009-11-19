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
 * Copyright 1990 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2009 J. Schilling
 *
 * @(#)ccstypes.h	1.6 09/11/08 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)ccstypes.h 1.6 09/11/08 J. Schilling"
#endif
/*
 * @(#)ccstypes.h 1.2 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)ccstypes.h"
#pragma ident	"@(#)sgs-inc:common/ccstypes.h"
#endif

#include <schily/types.h>

#ifdef	never
#ifndef _SYS_TYPES_H
#if !(defined(sun) && defined(_TYPES_))
typedef	unsigned short uid_t;
typedef unsigned short gid_t;
#endif
typedef short pid_t;
typedef unsigned short mode_t;
typedef short nlink_t;
#endif
#ifdef uts
#	ifndef _SIZE_T
#		define _SIZE_T
#		ifndef size_t
#			define size_t	unsigned int
#		endif
#	endif
#endif

#endif /* never */
