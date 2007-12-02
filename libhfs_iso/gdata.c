/* @(#)gdata.c	1.3 07/02/08 Copyright 2001 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)gdata.c	1.3 07/02/08 Copyright 2001 J. Schilling";
#endif

#include <schily/mconfig.h>
#include "internal.h"

char *hfs_error = "no error";	/* static error string */

#if	defined(IS_MACOS_X)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * Make __roothandle a "data" variable.
 */
hfsvol *hfs_mounts = 0;		/* linked list of mounted volumes */
hfsvol *hfs_curvol = 0;		/* current volume */
#else
hfsvol *hfs_mounts;		/* linked list of mounted volumes */
hfsvol *hfs_curvol;		/* current volume */
#endif
