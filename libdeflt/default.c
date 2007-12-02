/* @(#)default.c	1.6 06/09/13 Copyright 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)default.c	1.6 06/09/13 Copyright 1997 J. Schilling";
#endif
/*
 *	Copyright (c) 1997 J. Schilling
 */
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

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <stdio.h>
#include <schily/string.h>
#include <schily/deflts.h>

#define	MAXLINE	512

static	FILE	*dfltfile	= (FILE *)NULL;

EXPORT	int	defltopen	__PR((const char *name));
EXPORT	int	defltclose	__PR((void));
EXPORT	void	defltfirst	__PR((void));
EXPORT	char	*defltread	__PR((const char *name));
EXPORT	char	*defltnext	__PR((const char *name));
EXPORT	int	defltcntl	__PR((int cmd, int flags));

EXPORT int
defltopen(name)
	const char	*name;
{
	if (dfltfile != (FILE *)NULL)
		fclose(dfltfile);

	if (name == (char *)NULL) {
		fclose(dfltfile);
		dfltfile = NULL;
		return (0);
	}

	if ((dfltfile = fopen(name, "r")) == (FILE *)NULL) {
		return (-1);
	}
	return (0);
}

EXPORT int
defltclose()
{
	int	ret;

	if (dfltfile != (FILE *)NULL) {
		ret = fclose(dfltfile);
		dfltfile = NULL;
		return (ret);
	}
	return (0);
}

EXPORT void
defltfirst()
{
	if (dfltfile == (FILE *)NULL) {
		return;
	}
	rewind(dfltfile);
}

EXPORT char *
defltread(name)
	const char	*name;
{
	if (dfltfile == (FILE *)NULL) {
		return ((char *)NULL);
	}
	rewind(dfltfile);
	return (defltnext(name));
}

EXPORT char *
defltnext(name)
	const char	*name;
{
	register int	len;
	register int	namelen;
	static	 char	buf[MAXLINE];

	if (dfltfile == (FILE *)NULL) {
		return ((char *)NULL);
	}
	namelen = strlen(name);

	while (fgets(buf, sizeof (buf), dfltfile)) {
		len = strlen(buf);
		if (buf[len-1] == '\n') {
			buf[len-1] = 0;
		} else {
			return ((char *)NULL);
		}
		if (strncmp(name, buf, namelen) == 0) {
			return (&buf[namelen]);
		}
	}
	return ((char *)NULL);
}

EXPORT int
defltcntl(cmd, flags)
	int	cmd;
	int	flags;
{
	int  oldflags = 0;

	return (oldflags);
}
