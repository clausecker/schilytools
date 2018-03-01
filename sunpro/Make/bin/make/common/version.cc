/* @(#)version.cc	1.12 18/03/01 Copyright 2017-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)version.cc	1.12 18/03/01 Copyright 2017-2018 J. Schilling";
#endif

/*
 * Copyright (c) 2018 J. Schilling
 *
 * @(#)version.cc	1.12 18/03/01 2017-2018 J. Schilling
 */

#define	VERSION_DATE	"2018/03/01"
#define	VERSION_STR	"1.1"
#ifdef	SCHILY_BUILD
#define	VERSION_NAME	"Schily-Tools"
#else
#define	VERSION_NAME	"SchilliX-ON"
#endif
#ifdef	PMAKE
#define	MAKE_TYPE	" Parallel "
#else
#define	MAKE_TYPE	" Serial "
#endif

char	verstring[] = VERSION_NAME MAKE_TYPE "Make " VERSION_STR " " \
			HOST_OS "-" HOST_CPU " " \
			VERSION_DATE \
			"\n\tderived from SunPro Make sources";
