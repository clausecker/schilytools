/* @(#)quitcmds.c	1.61 06/09/26 Copyright 1984-1989, 1994-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)quitcmds.c	1.61 06/09/26 Copyright 1984-1989, 1994-2004 J. Schilling";
#endif
/*
 *	Commands that deal with exiting the editor in a clean state
 *
 *	Copyright (c) 1984-1989, 1994-2004 J. Schilling
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

#include "ved.h"
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/dirent.h>
#include <signal.h>
#include <schily/errno.h>

EXPORT	BOOL	bakbuf		__PR((ewin_t *wp, BOOL force));
EXPORT	BOOL	writebuf	__PR((ewin_t *wp, BOOL force));
EXPORT	void	vbackup		__PR((ewin_t *wp));
EXPORT	void	vquit		__PR((ewin_t *wp));
EXPORT	void	eexit		__PR((ewin_t *wp));
EXPORT	void	vsusp		__PR((ewin_t *wp));
LOCAL	void	suspendme	__PR((ewin_t *wp));
#ifndef	JOS
LOCAL	int	suspend		__PR((int p));
#endif


/*
 * Write back the content of the current file buffer,
 * create a backup file if 'nobak' is not set.
 */
EXPORT BOOL
bakbuf(wp, force)
	ewin_t	*wp;
	BOOL	force;
{
		Uchar	backup[FNAMESIZE+4];	/* += strlen(".bak");	*/
	register Uchar	*t;
	register Uchar	*s;
	int	 oldmodes;
	BOOL	 backpresent = TRUE;

	if (wrtcheck(wp, TRUE) == FALSE) {
		if (!force)
			return (FALSE);
	}
	if (modcheck(wp) == FALSE) {
		if (!force)
			return (FALSE);
		sleep(1);
	}

	oldmodes = getfmodes(wp->curfile);
	strcpy(C backup, C wp->curfile);
	s = t = backup;
	while (*t) {			/* find rightmost '/'		    */
		if (*t++ == '/')
			s = t;
	}

#ifdef	FOUND_DIRSIZE
	if (t - s >= DIRSIZE)		/* If ".bak" would not fit, trucate */
		t = s + DIRSIZE - 4;
#endif

	strcpy(C t, ".bak");		/* add the the ".bak" extension	    */
	if (rename(C wp->curfile, C backup) < 0) { /* rename old curfile to backup */
		backpresent = FALSE;
		if (geterrno() != ENOENT)
			writemsg(wp, "Can't make *.bak file");
	}

	if (!writebuf(wp, force)) {	/* if writeback failed ... */
		if (backpresent)	/* rename backup to orig file name */
			rename(C backup, C wp->curfile);
		return (FALSE);
	}
	if (oldmodes >= 0)
		chmod(C wp->curfile, oldmodes);
	if (nobak)
		unlink(C backup);
	return (TRUE);
}

/*
 * Write back the content of the current file buffer,
 * do not reate a backup file, write into the currently edited file.
 */
EXPORT BOOL
writebuf(wp, force)
	ewin_t	*wp;
	BOOL	force;
{
	FILE	*outfile;
	BOOL	ret = TRUE;
#ifdef	HAVE_FSYNC
	int	err;
	int	cnt;
#endif

	if ((!force || ReadOnly > 1) && wrtcheck(wp, TRUE) == FALSE)
		return (FALSE);

	if (modcheck(wp) == FALSE) {
		if (!force)
			return (FALSE);
		sleep(1);
	}
	/*
	 * create outfile
	 */
	if ((outfile = opensyserr(wp, wp->curfile, "ctwub")) == NULL) {
		return (FALSE);
	}
	lockfd(fdown(outfile));

	/*
	 * If writeback failed for some reason, return FALSE.
	 * Be very serious that the file could actually written back correctly.
	 */
	if (wp->eof != 0 && !savefile(wp, (epos_t)0, wp->eof, outfile, "FILE"))
		ret = FALSE;
	if (fflush(outfile) != 0)
		ret = FALSE;
#ifdef	HAVE_FSYNC
	err = 0;
	cnt = 0;
	do {
		if (fsync(fdown(outfile)) != 0)
			err = geterrno();

		if (err == EINVAL)
			err = 0;
	} while (err == EINTR && ++cnt < 10);
	if (err != 0)
		ret = FALSE;
#endif
	if (fclose(outfile) != 0)
		ret = FALSE;

	if (ret == FALSE)
		write_errno(wp, "CAN'T PUT %s", wp->curfile);
	return (ret);
}

/*
 * Do a backup write of the current file,
 * create a backup file if required.
 */
EXPORT void
vbackup(wp)
	ewin_t	*wp;
{
	int	c;

	switch (c = getcmdchar(wp, NULL, "BACKUP?(Y/W/N/F/!) ")) {

	default:
		abortmsg(wp);
		/* FALLTHROUGH */
	case 0:			/* aborted */
	case 'n':
	case 'N':
		return;
	case 'w':
	case 'W':
	case '!':
		if (!writebuf(wp, c == '!'))
			return;
		goto go_on;
	case 'y':
	case 'Y':
	case 'f':
	case 'F':
		if (!bakbuf(wp, c == 'f' || c == 'F'))
			return;
	go_on:
		newprot(wp);
		wp->modflg = 0;
		wp->curftime = gftime(C wp->curfile);
		defaultinfo(wp, UC NULL);
	}
}

/*
 * Quit the editor, ask if the current buffer should be saved.
 */
EXPORT void
vquit(wp)
	ewin_t	*wp;
{
	int	c;

	vedstopstats();

	if (wp->modflg)
		writeerr(wp, "FILE MODIFIED!");

	switch (c = getcmdchar(wp, NULL, "QUITTING. PUT EDITS?(Y/W/N/F/!) ")) {

	default:
		abortmsg(wp);
		/* FALLTHROUGH */
	case 0:
		return;			/* aborted */
	case 'w':
	case 'W':
	case '!':
		if (!writebuf(wp, c == '!'))
			return;
		goto ex_it;
	case 'y':
	case 'Y':
	case 'f':
	case 'F':
		if (!bakbuf(wp, c == 'f' || c == 'F'))
			return;
		/* FALLTHROUGH */
	case 'n':
	case 'N':
	ex_it:
		put_vedtmp(wp, TRUE);
		eexit(wp);
		exit(0);
	}
}

/*
 * Exit the editor, reset the terminal, the tty driver and
 * delete temporary files.
 */
EXPORT void
eexit(wp)
	ewin_t	*wp;
{
	rsttmodes(wp);
	tmpcleanup(wp, TRUE);
	vedstatistics();
}

/*
 * Suspend the editor, ask if the current buffer should be saved.
 */
EXPORT void
vsuspend(wp)
	ewin_t	*wp;
{
#if	defined(SIGSTOP) || defined(JOS)
	int	c;

	if (wp->modflg)
		writeerr(wp, "FILE MODIFIED!");

	switch (c = getcmdchar(wp, NULL, "SUSPENDING. PUT EDITS?(Y/W/N/F/!) ")) {

	default:
		abortmsg(wp);
		/* FALLTHROUGH */
	case 0:
		return;			/* aborted */
	case 'w':
	case 'W':
	case '!':
		if (!writebuf(wp, c == '!'))
			return;
		goto go_on;
	case 'y':
	case 'Y':
	case 'f':
	case 'F':
		if (!bakbuf(wp, c == 'f' || c == 'F'))
			return;
	go_on:
		wp->modflg = 0;
		wp->curftime = gftime(C wp->curfile);
		defaultinfo(wp, UC NULL);
		newprot(wp);
		/* FALLTHROUGH */
	case 'n':
	case 'N':
		suspendme(wp);
	}
#else
	writeerr(wp, "NOT IMPLEMENTED!");
#endif
}

/*
 * Suspend the editor, reset the terminal, the tty driver,
 * do not delete temporary files.
 * Reset terminal and tty state on continue.
 */
#if	defined(SIGSTOP) || defined(JOS)
LOCAL void
suspendme(wp)
	ewin_t	*wp;
{
	rsttmodes(wp);
	suspend(0);
	settmodes(wp);
	vredisp(wp);
}
#endif

/*
 * Suspend a process
 */
#ifndef	JOS
LOCAL int
suspend(p)
	int	p;
{
#ifdef	SIGSTOP
	return (kill(p, SIGSTOP));
#else
	raisecond("suspend not implemented", 0L);
#endif
}
#endif
