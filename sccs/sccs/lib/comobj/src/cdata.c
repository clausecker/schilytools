/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */
/*
 * @(#)cdata.c	1.4 14/08/09 Copyright 2011-2014 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)cdata.c 1.4 14/08/09 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)cdata.c"
#pragma ident	"@(#)sccs:lib/comobj/cdata.c"
#endif
#include	<defines.h>

char	*sccs_insbase;	/* The install base if set by main program */
char	*Comments;	/* Comments from command line arg */
char	*Mrs;		/* MR numbers from command line arg */

int	Domrs;		/* The 'v' flag was found */

#if	defined(IS_MACOS_X)
/*
 * The Mac OS X static linker is too silly to link in .o files from static libs
 * if only a variable is referenced. The elegant workaround for this bug (using
 * common variables) triggers a different bug in the dynamic linker from Mac OS
 * that is unable to link common variables. This forces us to introduce funcs
 * that need to be called from central places to enforce to link in the vars.
 */
EXPORT	void __comobj __PR((void));

EXPORT void
__comobj()
{
}
#endif

EXPORT void
sccs_setinsbase(base)
	char	*base;
{
	sccs_insbase = base;
}
