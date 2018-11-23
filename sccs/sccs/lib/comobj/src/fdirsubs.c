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
 * @(#)fdirsubs.c	1.3 18/11/07 Copyright 2018 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)fdirsubs.c 1.3 18/11/07 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)fdirsubs.c"
#pragma ident	"@(#)sccs:lib/comobj/fdirsubs.c"
#endif
#include	<defines.h>

EXPORT	int	opendirfd	__PR((const char *name));
EXPORT	int	closedirfd	__PR((int fd));

/*
 * Open a directory as reference for fchdir() and things like openat()
 * and call fdsetname() to allow the libschily emulation to work on older
 * platforms.
 */
EXPORT int
opendirfd(name)
	const char	*name;
{
#ifdef	SCHILY_BUILD		/* use libschily */
	return (diropen(name));
#else
	int	fd;

	if ((fd = open(name, O_SEARCH|O_DIRECTORY|O_NDELAY)) < 0)
		return (fd);
	if (fdsetname(fd, ".") < 0) {
		close(fd);
		fd = -1;
	}
	return (fd);
#endif
}

/*
 * Close directory in case it is open.
 */
EXPORT int
closedirfd(fd)
	int	fd;
{
	if (fd < 0)
		return (0);
#ifdef	SCHILY_BUILD		/* use libschily */
	return (dirclose(fd));
#else
	fdclosename(fd);
	return (close(fd));
#endif
}
