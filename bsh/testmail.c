/* @(#)testmail.c	1.14 07/03/11 Copyright 1986-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)testmail.c	1.14 07/03/11 Copyright 1986-2007 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-2007 J. Schilling
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
#include <stdio.h>
#include <schily/standard.h>
#include <schily/stat.h>
#include "bsh.h"
#include "str.h"
#include "strsubs.h"

EXPORT	void	testmail	__PR((void));

#ifdef	TESTMAIL
LOCAL	time_t	testamail	__PR((char *mname, time_t mtime));

EXPORT void
testmail()
{
		char	*mp;
		char	*mname = NULL;
		char	*p;
	static	time_t	lastcheck = (time_t)0;
	static	time_t	mtime = (time_t)0;
		time_t	tnew;
		time_t	t;
		BOOL	dopath = TRUE;

	if (mailcheck > 0) {
		time_t	ltime = time(0);

		if ((lastcheck + mailcheck) > ltime)
			return;
		lastcheck = ltime;
	}
	/*
	 * str.c:char	mailname[]	= "MAIL";
	 * str.c:char	mchkname[]	= "MAILCHECK";
	 * str.c:char	mailpname[]	= "MAILPATH";
	 */
	mp = getcurenv(mailpname);
	if (mp == NULL) {
		mp = getcurenv(mailname);
		dopath = FALSE;
	}
	if (mp == NULL)
		return;

	mname = mp;
	tnew = mtime;
	p = NULL;
	while (mname && *mname != '\0') {
		if (dopath && (p = strchr(mname, ':')) != NULL) {
			*p = '\0';
		}
		t = testamail(mname, mtime);
		if (t > tnew)
			tnew = t;
		if (p == NULL)
			break;
		*p++ = ':';
		mname = p;
	}
	if (tnew > mtime)
		mtime = tnew;
}

LOCAL time_t
testamail(mname, mtime)
	char	*mname;
	time_t	mtime;
{
	struct	stat	stbuf;

	if (stat(mname, &stbuf) < 0)
		return ((time_t)0);

	if (stbuf.st_size == 0)
		return ((time_t)0);

	if (stbuf.st_atime < stbuf.st_mtime && stbuf.st_mtime > mtime) {
		printf("You have new mail in %s.\n", mname);
		return (stbuf.st_mtime);
	}
	if (stbuf.st_mtime > mtime) {
		printf("You have mail in %s.\n", mname);
		return (stbuf.st_mtime);
	}
	return ((time_t)0);
}
#else

EXPORT void
testmail() {}

#endif
