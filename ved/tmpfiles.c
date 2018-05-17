/* @(#)tmpfiles.c	1.14 09/07/09 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)tmpfiles.c	1.14 09/07/09 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	Filename manipulation for various tmp files of ved.
 *
 *	Copyright (c) 1984-2009 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include "ved.h"
#include "buffer.h"

EXPORT	Uchar	swapname[TMPNSIZE]; /* file name of current buffer backup file*/
EXPORT	Uchar	execname[TMPNSIZE]; /* file name of current execute buffer    */
EXPORT	Uchar	protname[TMPNSIZE]; /* file name of current recover protocol  */


EXPORT	FILE	*takefile;	/* FILE * of current take buffer	    */
EXPORT	epos_t	takesize;	/* file size of current take buffer	    */
EXPORT	Uchar	takename[TMPNSIZE]; /* file name of current take buffer	    */

EXPORT	FILE	*delfile;	/* FILE * of delete buffer		    */
EXPORT	epos_t	delsize;	/* file size of delete buffer		    */
EXPORT	Uchar	delname[TMPNSIZE]; /* file name of delete buffer	    */

EXPORT	FILE	*rubfile;	/* FILE * of rubout buffer		    */
EXPORT	epos_t	rubsize;	/* file size of rubout buffer		    */
EXPORT	Uchar	rubname[TMPNSIZE]; /* file name of rubout buffer	    */

char	*tmpdirs[] = {
	"/var/tmp",
	"/usr/tmp",
	"/tmp",
	NULL,
};

LOCAL	Uchar	*tmpdir;		/* Standard tmp file dir prefix	    */
LOCAL	Uchar	*ftmpdir = UC "/tmp";	/* Fast tmp directory if available  */
LOCAL	Uchar	*tmppre = UC "ved";	/* Filename prefix for all tmp files */

LOCAL	void	findtmpdir	__PR((void));
EXPORT	FILE	*tmpfopen	__PR((ewin_t *wp, Uchar *name, char *mode));
EXPORT	void	tmpsetup	__PR((void));
EXPORT	void	takepath	__PR((Uchar *pathname, int pathsize, Uchar *name));
EXPORT	void	tmpopen		__PR((ewin_t *wp));
LOCAL	void	tmpdelete	__PR((void));
EXPORT	void	tmpcleanup	__PR((ewin_t *wp, BOOL force));

/*
 * Find tmp directory prefix for this edit session.
 */
LOCAL void
findtmpdir()
{
	char	**tp;
	char	*ft;
	char	*td;

	ft = getenv("VED_FTMPDIR");
	td = getenv("VED_TMPDIR");
	if (ft == NULL)
		ft = td;

	if (ft != NULL && readable(UC ft) && writable(UC ft))
		ftmpdir = UC ft;
	if (td != NULL && readable(UC td) && writable(UC td))
		tmpdir = UC td;

	for (tp = &tmpdirs[0]; *tp != NULL; tp++) {
		if (readable(UC *tp) && writable(UC *tp))
			break;
	}
	if (tmpdir == NULL) {
		if (*tp != 0) {
			tmpdir = UC *tp;
		} else {
			tmpdir = UC ".";
		}
	}
/*	errmsgno(-1, "tmpdir: '%s'\n", tmpdir);*/
/*	sleep(1);*/
	if (!readable(ftmpdir) || !writable(ftmpdir))
		ftmpdir = UC ".";
}

/*
 * Open a tmp file and make set private access permissions.
 */
EXPORT FILE *
tmpfopen(wp, name, mode)
	ewin_t	*wp;
	Uchar	*name;
	char	*mode;
{
	FILE	*f;

	f = opencomerr(wp, name, mode);
	if (f != (FILE *)NULL)
		stmpfmodes(name);
	return (f);
}

/*
 * Set up the names of several files that are known
 * at start time of an editing session.
 */
EXPORT void
tmpsetup()
{
	if (tmpdir == NULL)
		findtmpdir();

	snprintf(C swapname, sizeof (swapname), "%s/%sF.%d", ftmpdir, tmppre, pid);
	snprintf(C protname, sizeof (protname), "%s/%sP.%d", tmpdir, tmppre, pid);

	snprintf(C execname, sizeof (execname), "%s/%sX.%d", tmpdir, tmppre, pid);

	snprintf(C delname, sizeof (delname), "%s/%sD.%d", tmpdir, tmppre, pid);
	snprintf(C rubname, sizeof (rubname), "%s/%sB.%d", tmpdir, tmppre, pid);
	snprintf(C takename, sizeof (takename), "%s/%sT.%d", tmpdir, tmppre, pid);
}

/*
 * Set up the pathname of a take buffer.
 */
EXPORT void
takepath(pathname, pathsize, name)
	Uchar	*pathname;
	int	pathsize;
	Uchar	*name;
{
	snprintf(C pathname, pathsize, "%s/%sT%s.%d", tmpdir, tmppre, name, pid);
}

/*
 * Open several tmp files at startup.
 */
EXPORT void
tmpopen(wp)
	ewin_t	*wp;
{
	delfile = tmpfopen(wp, delname, "cwrtb");
	rubfile = tmpfopen(wp, rubname, "cwrtb");
	takefile = tmpfopen(wp, takename, "cwrtb");
}

/*
 * Close and remove several tmp files.
 */
LOCAL void
tmpdelete()
{
	if (execname[0] != '\0')
		unlink(C execname);

	if (delfile != (FILE *)0)
		fclose(delfile);

	if (delname[0] != '\0')
		unlink(C delname);


	if (rubfile != (FILE *)0)
		fclose(rubfile);

	if (rubname[0] != '\0')
		unlink(C rubname);


	if (takefile != (FILE *)0)
		fclose(takefile);

	if (takename[0] != '\0')
		unlink(C takename);
}

/*
 * Close and remove all tmporary files of VED.
 */
EXPORT void
tmpcleanup(wp, force)
	ewin_t	*wp;
	BOOL	force;
{
	if (force || wp->modflg == 0) {
		termbuffers(wp);
		deleteprot();
		tmpdelete();
		deletetake();
	}
}
