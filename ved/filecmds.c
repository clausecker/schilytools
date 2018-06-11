/* @(#)filecmds.c	1.54 09/07/09 Copyright 1984-1986, 1989-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)filecmds.c	1.54 09/07/09 Copyright 1984-1986, 1989-2009 J. Schilling";
#endif
/*
 *	Commands that deal with filenames and read/write of files
 *
 *	Copyright (c) 1984-1986, 1989-2009 J. Schilling
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
#include "terminal.h"
#include <schily/wait.h>

EXPORT	void	vread		__PR((ewin_t *wp));
EXPORT	void	vwrite		__PR((ewin_t *wp));
LOCAL	BOOL	shellspecial	__PR((Uchar *pattern, int length));
LOCAL	int	glob		__PR((ewin_t *wp, Uchar* buf));
EXPORT	void	vchange		__PR((ewin_t *wp));
EXPORT	BOOL	change_file	__PR((ewin_t *wp, Uchar* file));
EXPORT	void	fchange		__PR((ewin_t *wp, Uchar* file));
EXPORT	void	vswrite		__PR((ewin_t *wp));

/*
 * Read a file into the current buffer
 */
EXPORT void
vread(wp)
	ewin_t	*wp;
{
	register epos_t	savedot = wp->dot;
	int	len;
	Uchar	name[FNAMESIZE];

	if (! (len = getcmdline(wp, name, sizeof (name), "Get from: ")))
		return;
	if (!issimple(name, len) || shellspecial(name, len))
		if (!glob(wp, name))
			return;
	loadfile(wp, name, FALSE);
	if (wp->dot == savedot)		/*  Did not load anything.  */
		return;
	dispup(wp, wp->dot, savedot);
	wp->dot = savedot;
	modified(wp);
}

/*
 * Write current buffer to a file.
 */
EXPORT void
vwrite(wp)
	ewin_t	*wp;
{
	FILE	*f;
	int	len;
	Uchar	name[FNAMESIZE];

	if (! (len = getcmdline(wp, name, sizeof (name), "Write to: ")))
		return;
	if (!issimple(name, len) || shellspecial(name, len))
		if (!glob(wp, name))
			return;
	if ((f = opensyserr(wp, name, "ctwub")) == (FILE *) NULL)
		return;
	lockfd(fdown(f));
	if (savefile(wp, (epos_t)0, wp->eof, f, "FILE")) {
		wp->modflg = 0;
		defaultinfo(wp, UC NULL);
	}
	fclose(f);
}

/*
 * Check if the pattern  contains shell special characters
 * that (in addition to patthern matching characters) will
 * expand filenames.
 */
LOCAL BOOL
shellspecial(pattern, length)
	register Uchar *pattern;
	register int length;
{
	while (length-- > 0) {
		switch (*pattern++) {

		case '~':
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
 * Let shell do filename globbing - pipe buffer through shell.
 */
LOCAL int
glob(wp, buf)
	ewin_t	*wp;
	Uchar	*buf;
{
#ifdef	HAVE_FORK
	FILE	*pp[2];
	int	mypid;
	char	*sh;
	char	cmd[FNAMESIZE+5];	/* + strlen ("echo ") */
	char	tmp[FNAMESIZE];

	if ((sh = getenv("SHELL")) == NULL)
		sh = "/bin/sh";

	strcatl(cmd, "echo ", buf, (char *)NULL);
/*	cdbg("cmd: %s\n", cmd);*/
	if (fpipe(pp) == 0) {
		write_errno(wp, "Glob pipe failed");
		return (0);
	}
	mypid = fork();
	if (mypid < 0) {
		write_errno(wp, "Glob fork failed");
		return (0);
	}
	if (mypid == 0) {
		FILE	*null;

		fclose(pp[0]);

		/* We don't want to see errors */
		null = fileopen("/dev/null", "rwb");

		fexecl(sh, null, pp[1], null, "sh", "-c", cmd, (char *)NULL);
		write_errno(wp, "Glob exec failed");
		_exit(-1);
	}
	fclose(pp[1]);
	tmp[0] = '\0';
	fgetline(pp[0], tmp, sizeof (tmp));
	fclose(pp[0]);
	wait(0);
/*	cdbg("line: '%s'\n", tmp);*/
	if (strchr(tmp, ' ')) {
		writeerr(wp, "Ambiguous");
		return (0);
	}
	strcpy(C buf, tmp);
	return (1);
#else
	writeerr(wp, "Glob not available");
	return (0);
#endif
}

/*
 * Replace the current file in current buffer
 */
EXPORT void
vchange(wp)
	ewin_t	*wp;
{
	int	len;
	Uchar	name[FNAMESIZE];

	/*
	 * Get the new filename, abort on empty input.
	 */
	if (! (len = getcmdline(wp, name, sizeof (name), "Change to: ")))
		return;
	if (!issimple(name, len) || shellspecial(name, len))
		if (!glob(wp, name))
			return;

	if (!change_file(wp, name))
		return;

	newwindow(wp);
}

/*
 * Ask the user if and how the old file should be written back.
 */
EXPORT BOOL
change_file(wp, file)
	ewin_t	*wp;
	Uchar	*file;
{
	int	c;

	if (wp->modflg)
		writeerr(wp, "FILE MODIFIED!");
	if (wp->modflg || mflag) {

		switch (c = getcmdchar(wp, NULL,
			"CHANGING TO: %s. PUT EDITS?(Y/W/N/F/!) ", file)) {

		default:
			abortmsg(wp);
			/* FALLTHROUGH */
		case 0:
			return (FALSE);
		case 'n':
		case 'N':
			break;
		case 'w':
		case 'W':
		case '!':
			if (!writebuf(wp, c == '!'))
				return (FALSE);
			break;
		case 'y':
		case 'Y':
		case 'f':
		case 'F':
			if (!bakbuf(wp, c == 'f' || c == 'F'))
				return (FALSE);
		}
	}
	flush();

	fchange(wp, file);
	return (TRUE);
}

/*
 * Replace the content of the buffer with new file and put new file on screen.
 */
EXPORT void
fchange(wp, file)
	ewin_t	*wp;
	Uchar *file;
{
	/*
	 * Delete the old file.
	 */
	wp->dot = 0;
	wp->markvalid = 0;
	delete(wp, wp->eof);
	bufdebug(wp);
	/*
	 * Load the new file.
	 */
	defaulterr(wp, UC NULL);
	wp->curftime = gftime(C file);
	loadfile(wp, file, TRUE);
	wp->dot = 0;

	if (autodos)
		wp->dosmode = isdos(wp);

	namemsg(file);
	strncpy(C curfname, C file, sizeof (curfname));
	curfname[sizeof (curfname)-1] = '\0';
	wp->curfile = curfname;

	put_vedtmp(wp, TRUE);	/* save new filename */
	newprot(wp);		/* change Prot file */

	wp->modflg = 0;

	/*
	 * Put the new file on screen.
	 */
	CLEAR_SCREEN(wp);
	defaultinfo(wp, UC NULL);
	refreshmsg(wp);
	if (wp->dosmode)
		writemsg(wp, "DOS MODE");
	(void) wrtcheck(wp, FALSE);
	writelockmsg(wp);
}

/*
 * Save the content of the current selection (between cursor and mark) to file.
 */
EXPORT void
vswrite(wp)
	ewin_t	*wp;
{
	FILE	*f;
	int	len;
	Uchar	name[FNAMESIZE];
	epos_t	begin = min(wp->dot, wp->mark);
	epos_t	end   = max(wp->dot, wp->mark);

	/*
	 * Ask the user for the filename, abort on empty input.
	 */
	if (! (len = getcmdline(wp, name, sizeof (name), "Sel to: ")))
		return;
	if (!issimple(name, len) || shellspecial(name, len))
		if (!glob(wp, name))
			return;
	if ((f = opensyserr(wp, name, "ctwub")) == (FILE *) NULL)
		return;
	lockfd(fdown(f));
	savefile(wp, begin, end, f, "FILE");
	fclose(f);
}
