/* @(#)execcmds.c	1.41 06/09/13 Copyright 1984-1986, 1995-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)execcmds.c	1.41 06/09/13 Copyright 1984-1986, 1995-2004 J. Schilling";
#endif
/*
 *	Commands that deal with execution of shell commands from ved
 *
 *	Copyright (c) 1984-1986, 1995-2004 J. Schilling
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
#include <signal.h>
#include <schily/wait.h>

#define	XBUFSIZE	1024
LOCAL	Uchar	*exbase;	/* xbuffer base				*/
LOCAL	Uchar	*exbptr;	/* current read pointer in xbuffer	*/
LOCAL	epos_t	exbpos;		/* position foe next extract to xbuffer	*/
LOCAL	epos_t	exblen;		/* total amount of characters to extract*/

EXPORT	void	vexec		__PR((ewin_t *wp));
EXPORT	void	vtexec		__PR((ewin_t *wp));
EXPORT	void	vsexec		__PR((ewin_t *wp));
LOCAL	void	execcmd		__PR((ewin_t *wp, Uchar(*)(ewin_t *wp)));
LOCAL	void	doexec		__PR((ewin_t *wp, FILE *f));
LOCAL	void	execute		__PR((ewin_t *wp));
EXPORT	int	spawncmd	__PR((ewin_t *wp, char *name, char *arg));
EXPORT	BOOL	white		__PR((int ch));
LOCAL	Uchar	exbufgetc	__PR((ewin_t *wp));
LOCAL	Uchar	extakegetc	__PR((ewin_t *wp));

/*
 * Execute command from commandline.
 */
EXPORT void
vexec(wp)
	ewin_t	*wp;
{
	Uchar	cmdline[XBUFSIZE];

	if (! (exblen = getcmdline(wp, cmdline, sizeof (cmdline), "Execute: ")))
		return;
	exbptr = exbase = cmdline;
	execcmd(wp, exbufgetc);
	macro_reinit(wp);
}

/*
 * Execute content of current take buffer.
 */
EXPORT void
vtexec(wp)
	ewin_t	*wp;
{
	Uchar	buf[XBUFSIZE + 1];

	lseek(fdown(takefile), (off_t)0, SEEK_SET);
	exbase = buf;
	exbptr = &buf[XBUFSIZE];
	exblen = takesize;
	execcmd(wp, extakegetc);
}

/*
 * Execute content of selection (between cursor and mark).
 */
EXPORT void
vsexec(wp)
	ewin_t	*wp;
{
	Uchar	buf[XBUFSIZE + 1];
	epos_t	begin = min(wp->dot, wp->mark);
	epos_t	end   = max(wp->dot, wp->mark);

	if (! wp->markvalid) {
		nomarkmsg(wp);
	} else {
		exbase = buf;
		exbptr = &buf[XBUFSIZE];
		exbpos = begin;
		exblen = end - begin;
		execcmd(wp, exbufgetc);
	}
}

/*
 * Copy characters from comand buffer into execute-file.
 * Expand tape buffer names.
 * Execute content of execute-file.
 */
LOCAL void
execcmd(wp, nextc)
	ewin_t	*wp;
	Uchar (*nextc) __PR((ewin_t *wp));
{
	FILE	*f;
	Uchar	c;
	Uchar	*tfpath;
	Uchar	tbuf[NAMESIZE];	/* We don't know the max namelen of a takebuf*/
	int	i;

	if ((f = opensyserr(wp, execname, "ctwb")) == NULL)
		return;
	stmpfmodes(execname);

	for (;;) switch (c = (*nextc)(wp)) {

	case '\\':
		/*
		 * Check if this is a quoted take-buffer name.
		 */
		if ((c = (*nextc)(wp)) != '\\') {
			for (i = 0; !white(c) && i < (NAMESIZE-1); ) {
				tbuf[i++] = c;
				c = (*nextc)(wp);
			}
			tbuf[i] = '\0';
			if (!(tfpath = findtake(wp, tbuf)))
				return;

			/*
			 * Use real path name of take buffer
			 * instead of take-buffer name.
			 */
			while (*tfpath != '\0')
				putc(*tfpath++, f);
		}
		/* FALLTHROUGH */
	default:
		putc(c, f);
		continue;

	case '\0':
		doexec(wp, f);
		return;
	}
}

LOCAL void
doexec(wp, f)
	ewin_t	*wp;
	FILE	*f;
{
	int	c;

	putc('\n', f);		/* Add a newline csh wants it	*/
	fclose(f);
	if (!wp->modflg && !mflag) {
		/*
		 * XXX Falls das Kommando das File modifiziert,
		 * XXX wird es nicht wieder eingelesen!
		 */
		flush();
		execute(wp);
		vredisp(wp);
		return;
	}
	if (wp->modflg)
		writeerr(wp, "FILE MODIFIED!");

	switch (c = getcmdchar(wp, NULL, "EXECUTING. PUT EDITS?(Y/W/N/F/!) ")) {

	default:
		abortmsg(wp);
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
	ex_it:
		execute(wp);
		fchange(wp, wp->curfile);
		newwindow(wp);
		return;
	case 'n':
	case 'N':
		execute(wp);
		vredisp(wp);
		return;
	}
}

/*
 * Execute the content ot the execute file.
 * Write back the ontent of the current take buffer.
 */
LOCAL void
execute(wp)
	ewin_t	*wp;
{
	char	*sh;

	backuptake(wp);
	if ((sh = getenv("SHELL")) == NULL)
		spawncmd(wp, DEFSHELL, C execname);
	else
		spawncmd(wp, sh, C execname);
	loadtake(wp);
	wait_for_confirm(wp);
}

/*
 * Spawn new process and execute the content ot the execute file.
 * Do this with default signal handler and default tty modes.
 */
EXPORT int
spawncmd(wp, name, arg)
	ewin_t	*wp;
	char	*name;
	char	*arg;
{
#define	VOID_SIGS
#ifdef	VOID_SIGS
	void	(*old) __PR((int));
#else
	int	(*old) __PR((int));
#endif
	int stat;
	int newpid;
#ifdef	JOS
	int err;
#endif

#ifdef	HAVE_FORK
	old = signal(SIGINT, SIG_IGN);
	rsttmodes(wp);
	newpid = fork();
	if (newpid == 0) {
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		fexecl(name, stdin, stdout, stderr, name, arg, (char *)NULL);
		comerr("Can not execute %s.\n", name);
	}
#ifdef	JOS
	err = cwait(&newpid, &stat);
	if (err == 4)
		kill(newpid, SIGKILL);	/* UNOS "coredump" */
	if (err > 0 && stat == 0)
		stat = -1;
#else
	newpid = wait(&stat);
	if (stat & 0200)
		unlink("core");
#endif
	settmodes(wp);
	signal(SIGINT, old);
#else
	writeerr(wp, "Fork/exec not available");
	stat = -1;
#endif

	return (stat);
}

/*
 * Check for whitespace characters.
 */
EXPORT BOOL
white(ch)
	Uchar ch;
{
	if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\0')
		return (TRUE);
	return (FALSE);

}

/*
 * Extract one character from current selection.
 * Refill xbuffer if empty.
 */
LOCAL Uchar
exbufgetc(wp)
	ewin_t	*wp;
{
	int	amt;

	if (exbptr >= &exbase[XBUFSIZE]) {
		amt = extract(wp, exbpos, exbase, (int)min(exblen, XBUFSIZE));
		if (amt == 0)
			return ('\0');
		exbpos += amt;
		exbptr = exbase;
	}
	if (--exblen < 0)
		return ('\0');
	return (*exbptr++);
}

/*
 * Extract one character from current takefile.
 */
LOCAL Uchar
extakegetc(wp)
	ewin_t	*wp;
{
	int	amt;
extern	char	TAKEBUF[];

	if (exbptr >= &exbase[XBUFSIZE]) {
		amt = readsyserr(wp, takefile, exbase, (int)min(exblen, XBUFSIZE), UC TAKEBUF);
		if (amt == 0)
			return ('\0');
		exbptr = exbase;
	}
	if (--exblen < 0)
		return ('\0');
	return (*exbptr++);
}
