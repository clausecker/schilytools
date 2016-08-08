/* @(#)ved.c	1.80 16/08/05 Copyright 1984, 85, 86, 88, 89, 97, 2000-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ved.c	1.80 16/08/05 Copyright 1984, 85, 86, 88, 89, 97, 2000-2016 J. Schilling";
#endif
/*
 *	VED Visual EDitor
 *
 *	Copyright (c) 1984, 85, 86, 88, 89, 97, 2000-2016 J. Schilling
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

/*
 * This is the the command line parser and main loop ved.
 *
 * The following include files are available:
 *
 * buffer.h	- for users of the paged virtual memory package
 * func.h	- definitions for exported functions (included by ved.h)
 * map.h 	- for users ot the mapper package
 * movedot.h	- definitions for functions used in movedot.c
 * terminal.h	- external interface to the TERMCAP package
 * ttys.h	- internal interface to the TERMCAP package
 * ved.h	- main ved include file (includes func.h)
 */
#include "ved.h"
#include "buffer.h"
#include "movedot.h"
#include "terminal.h"
#include <schily/errno.h>
#include <schily/signal.h>
#include <schily/sigblk.h>

EXPORT	char	ved_version[] = "1.7";
EXPORT	int	mflag;		/* if > 0 : take characters from macro	    */
EXPORT	int	ReadOnly;	/* if > 0 : do not allow to write back mods */

EXPORT	int	nobak = 0;	/* Es wird kein ___.bak angelegt	    */
EXPORT	int	nolock = 0;	/* Es wird kein record locking versucht	    */
EXPORT	int	dotags = 0;	/* "Filename" ist ctag			    */
EXPORT	Llong	startline = -1;	/* Cursorposition Bei Start		    */
EXPORT	int	noedtmp = 0;	/* Kein .vedtmp erzeugen		    */
EXPORT	int	recover = 0;	/* altes File reparieren		    */
EXPORT	BOOL	autodos = TRUE;	/* wp->dosmode aus isdos() bestimmen	    */

EXPORT	int	nfiles;		/* Anzahl der zu editierenden Files	    */
EXPORT	int	fileidx = 0;	/* Index des editierten Files		    */
EXPORT	Uchar	**files;	/* Array der zu editierenden Files	    */
EXPORT	Uchar	curfname[FNAMESIZE]; /* global file name storage space	    */

EXPORT	int	pid;		/* process id used for unique tmp file names */

#ifndef	BFSIZE
LOCAL	Uchar bufferout[BUFSIZ]; /* To inhibit line buffering on stdout	    */
#endif

EXPORT	ewin_t	rootwin;

LOCAL	void	usage		__PR((int exitcode));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	int	handlesignal	__PR((void));
LOCAL	void	hupintr		__PR((int sig));
LOCAL	void	exintr		__PR((int sig));
EXPORT	void	settmodes	__PR((ewin_t *wp));
EXPORT	void	rsttmodes	__PR((ewin_t *wp));
LOCAL	Uchar	*gethelpfile	__PR((void));
LOCAL	Uchar	*_gethelpfile	__PR((char *name));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	ved [options]  [filename_1...filename_n]\n");
	error("Options:\n");
	error("	-version	Print version information and exit.\n");
	error("	-vhelp		Gives on-line Help.\n");
	error("	-readonly	Take readonly Mode.\n");
	error("	buffers=#	# of %d KB Incore Buffers (default is %d).\n",
						BUFFERSIZE/1024, MAXBUFFERS);
	error("	-nobak		Don't create filename.bak.\n");
	error("	-nolock		Don't lock the edited file for writing.\n");
	error("	-edtmp		Don't write to .vedtmp.\n");
	error("	-raw8		Don't expand 8bit chars.\n");
	error("	-dos		Map '\\n' to '\\r\\n', supress '\\r' in '\\r\\n'.\n");
	error("	-nodos		Do not map '\\n' to '\\r\\n'.\n");
	error("	-tag		filename_1 is a ctag.\n");
	error("	wrapmargin=#	set default wrapmargin to #.\n");
	error("	maxlinelen=#	set max line length for autowrap to #.\n");
	error("	+#		Start editing at line #.\n");
	error("	s=searchstr	Start editing with search to 'searchstr'.\n");
	error("	-Recover	Recover Session from filename.\n\n");
	error("	Options may be abbreviated by their first letter.\n");
	error("	If no file name is specified, the last edited file is used.\n");
	error("	Typing the command '^X ^H' gives on line help.\n");
	error("	Call 'ved -vhelp' to get the on-line help from command line.\n");
	exit(exitcode);

}

LOCAL	char	opts[] =
"help,version,readonly+,r+,vhelp,v,buffers#,b#,nobak,n,nolock,raw8,dos,d,nodos,tag,t,wrapmargin#,w#,maxlinelen#,maxll#,+#LL,s*,edtmp,e,Recover,R";

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	epos_t		pos = (epos_t)0;
	int		cac;
	char * const	*cav;
	BOOL		help = FALSE;
	BOOL		prvers = FALSE;
	BOOL		Vhelp = FALSE;
	int		buffers = -1;
	Uchar		*errstr;
	SIGBLK		sigblk;
	SIGBLK		sigfirst;
	int		err;
	char		*searchstr = NULL;
	ewin_t		*wp = &rootwin;
	BOOL		no_dos = FALSE;

	save_args(ac, av);

	cav = av, cac = ac;
	++cav, --cac;

	if (getallargs(&cac, &cav, opts,
			&help, &prvers,
			&ReadOnly, &ReadOnly,
			&Vhelp, &Vhelp,
			&buffers, &buffers,
			&nobak, &nobak,
			&nolock,
			&wp->raw8,
			&wp->dosmode, &wp->dosmode, &no_dos,
			&dotags, &dotags,
			&wp->wrapmargin, &wp->wrapmargin,
			&wp->maxlinelen, &wp->maxlinelen,
			&startline, &searchstr,
			&noedtmp, &noedtmp,
			&recover, &recover) < 0) {
		errmsgno(EX_BAD, "Bad flag '%s'.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prvers) {
		printf("ved %s (%s-%s-%s)\n\n", ved_version, HOST_CPU, HOST_VENDOR, HOST_OS);
		printf("Copyright (C) 1984, 85, 86, 88, 89, 97, 2000-2016 Jörg Schilling\n");
		printf("This is free software; see the source for copying conditions.  There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}
	if (Vhelp) {
		ReadOnly = 2;		/* Do not even allow QUIT !	*/
		noedtmp = 1;
	}
	if (searchstr) {
		extern Uchar sbuf[];
		extern int sflg;
		strcpy(C sbuf, searchstr);
		sflg = strlen(C sbuf);
	}
	if (no_dos)
		wp->dosmode = FALSE;
	if (no_dos || wp->dosmode)
		autodos = FALSE;
	/*
	 * Look for being called as ved-e or ved-w
	 */
	if ((errstr = (Uchar *)strrchr(av[0], '-')) != NULL) {
		errstr++;
		if (strchr((char *)errstr, 'e'))
			noedtmp = 1;
		if (strchr((char *)errstr, 'w') && wp->maxlinelen == 0)
			wp->maxlinelen = 78;	/* Try to follow RFC-2822 */
	}
	cav = av, cac = ac;
	++cav, --cac;

	file_raise((FILE *)0, FALSE);
	signal(SIGINT, exintr);
	get_modes(wp);			/* Get old ttymodes from tty driver  */
#ifdef	SIGHUP
	signal(SIGHUP, hupintr);
#endif
	signal(SIGTERM, hupintr);
	starthandlecond(&sigfirst);
	handlecond("any_other", &sigblk, (handlefunc_t)handlesignal, 0L);

	/*
	 * Initialize the TERMCAP package
	 */
	if ((errstr = t_start(wp)) != NULL)
		excomerrno(wp, EX_BAD, "%s\r\n", errstr);
	if (wp->llen <= 0 || wp->psize <= 0)	/* Paranoia: check screen size */
		excomerrno(wp, EX_BAD,
		"Bad line length or page size in terminal descriptor.\r\n");

	wp->markwrap = TRUE;
	wp->magic = TRUE;
	wp->eflags = COLUPDATE|SAVEDEL;
	wp->number = 1;			/* curnum is copied from this mult # master */
	wp->curnum = 1;			/* mult # fot next edit command	    */
	wp->tabstop = 8;		/* Breite eines 'tabs'		    */
	wp->optline = wp->psize/2;	/* set standard screen adjustment   */
	wp->curfd = -1;			/* Kein File writelock vorhanden    */
	set_modes();			/* set "ved" ttymodes		    */
	t_begin();			/* init terminal state for editing  */
	pid = getpid();			/* set pid for unique temp file names */
	initnum();			/* init number commands		    */

	if (Vhelp) {
		/*
		 * Edit help file.
		 */
		wp->curfile = gethelpfile();

	} else if (recover) {
		/*
		 * Run Recover session.
		 */
		if (getfiles(&cac, &cav, opts) > 0) {
			openrecoverfile(wp, cav[0]);
			wp->curfile = getrecoverfile(&pos, &wp->column);
			errmsgno(EX_BAD, "Recoverfile: %s.\r\n", wp->curfile);
		} else {
			excomerrno(wp, EX_BAD,
			"Must have name of File from which to recover.\n");
		}

	} else if (getfiles(&cac, &cav, opts) > 0) {
		/*
		 * We found filenames on the command line.
		 * Save them in the 'files' array.
		 * Save the current filename in .vedtmp.
		 */
		wp->curfile = UC cav[0];
		files = UCP cav;
		nfiles = cac;

		if (get_vedtmp(wp, &pos, &wp->column)) {
			if (startline >= 0 || searchstr) { /* XXX Pfusch */
				pos = (epos_t)0;
				wp->column = 0;
			}
		}
		/* XXX opensyserr()/openerrmsg() Probleme */
		if (!noedtmp && !dotags)
			put_vedtmp(wp, FALSE);

	} else {
		/*
		 * There were no filenames on the command line
		 * and -vhelp was not set.
		 * Try to get the filename from .vedtmp.
		 */
		if (get_vedtmp(wp, &pos, &wp->column)) {
			if (startline >= 0 || searchstr) { /* XXX Pfusch */
				pos = (epos_t)0;
				wp->column = 0;
			}
		}
		wp->curfile = curfname;
	}
	if (streql(getenv("SLASH"), "off") && strchr(C curfname, '/')) {
		if (wp->curfile != 0)
			*wp->curfile = '\0';
	}

	/*
	 * We had problems to get a proper filename setup.
	 * XXX There might be problems with macros if we introduce a "scratch"
	 * XXX file editing here. Check if this is possible.
	 */
	if (wp->curfile == 0 || *wp->curfile == '\0') {
		excomerrno(wp, EX_BAD,
			"No name in .vedtmp, need filename to edit.\n");
	}

	if (isatty(fdown(stdin)))	/* don't buffer stdin if it is a tty */
		setbuf(stdin, NULL);	/* so we get no pre read !!!	    */
#ifdef	BFSIZE
	setbuf(stdout, NULL);		/* Buffering on stdout is done in _bb */
#else
	setbuf(stdout, bufferout);	/* Inhibit line buffering on stdout */
#endif

	init_charset(wp);		/* Initialisierung der char Laengen */
					/* und Strings			    */

	init_binding();			/* Initialisierung der Bindung von  */
					/* Funktioinen an Zeichen	    */

	tmpsetup();			/* Init several tmp file names	    */

	/*
	 * Initialize the paged virtual memory and the status line.
	 */
	initbuffers(wp, buffers);
	bufdebug(wp);
	initmessage(wp);
	if (nfiles > 1)
		writemsg(wp, "%d files to edit.", nfiles);

	if (dotags) {
		switch (gettag(&wp->curfile)) {

		case -1:
			termbuffers(wp);
			excomerrno(wp, EX_BAD, "No such Tag.\n");
			/* NOTREACHED */

		case  0:
			err = geterrno();
			termbuffers(wp);
			excomerrno(wp, err, "Cannot open 'tags'.\n");
			/* NOTREACHED */

		case  1:
			strncpy(C curfname, C wp->curfile, sizeof (curfname));
			curfname[sizeof (curfname)-1] = '\0';
			wp->curfile = curfname;
		}
	}
	/*
	 * Load the file into the paged virtual memory.
	 */
	wp->curftime = gftime(C wp->curfile);
	if (!loadfile(wp, wp->curfile, TRUE)) {
		if (geterrno() != ENOENT || ReadOnly > 0) {
			err = geterrno();
			termbuffers(wp);
			excomerrno(wp, err, "Cannot open '%s'.\n", wp->curfile);
		}
	}
	if (pos > wp->eof) {
		if (recover) {
			termbuffers(wp);
			excomerrno(wp, EX_BAD,
				"Bad File to recover (file too small).\n");
		} else {
			pos = (epos_t)0;
			wp->column = 0;
			writeerr(wp, "BAD POS IN .vedtmp");
		}
	}
	if (autodos)
		wp->dosmode = isdos(wp);

	if (dotags && (pos = searchtag(wp, (epos_t)0)) > wp->eof)
		pos = (epos_t)0;

	namemsg(wp->curfile);
	CLEAR_SCREEN(wp);
	refreshmsg(wp);

	tmpopen(wp);			/* Open several temporary files	    */

	settakename(wp, UC "default");
	if (wp->dosmode)
		writemsg(wp, "DOS MODE");
	(void) wrtcheck(wp, FALSE);
	writelockmsg(wp);
	writenum(wp, wp->number);
	macro_init(wp);
	map_init();

	/*
	 * Go to the right place in the file and update the screen.
	 */
	wp->dot = pos;
	if (startline > 1 && wp->dot == 0)
		wp->dot = forwline(wp, wp->dot, (ecnt_t)startline - 1);
	if (searchstr)
		vagainsrch(wp);
	newwindow(wp);
	flush();
	wp->modflg = 0;
	newprot(wp);
	edit(wp);

	unhandlecond(&sigfirst);
	return (0);	/* Keep lint happy */
}

/*
 * Handle a software signal, do a clean exit.
 */
LOCAL int
handlesignal()
{
	rsttmodes(&rootwin);	/* XXX -> (ewin_t *)0 ??? */
	return (0);
}

/*
 * Signal handler for signals that kill us.
 * Used by SIGHUP & SIGTERM.
 */
/* ARGSUSED */
LOCAL void
hupintr(sig)
	int	sig;
{
	set_oldmodes();
	tmpcleanup(&rootwin, FALSE);	/* XXX -> (ewin_t *)0 ??? */

	exit(sig);
}

/*
 * Catch signals while ved is in startup.
 * Let us silently die when a signal is received
 * before the file is completely loaded.
 */
/* ARGSUSED */
LOCAL void
exintr(sig)
	int	sig;
{
	eexit(&rootwin);	/* XXX -> (ewin_t *)0 ??? */
	exit(EX_BAD);
}

/*
 * Get and save current tty modes, set editing tty modes
 * and set editing terminal state.
 */
EXPORT void
settmodes(wp)
	ewin_t	*wp;
{
	get_modes(wp);
	set_modes();
	t_begin();			/* init terminal state for editing   */
}

/*
 * Put cursor to the end of the screen, set back terminal state
 * and reset tty modes to saved values.
 */
EXPORT void
rsttmodes(wp)
	ewin_t	*wp;
{
	if (wp == NULL)
		wp = &rootwin;

	if (f_move_cursor) {
		MOVE_CURSOR(wp, wp->psize, 0);
		CLEAR_TO_EOF_LINE(wp);
	} else {
		output((Uchar *) "\n");
	}
	t_done();			/* put terminal to non-editing state */
	flush();
	set_oldmodes();
}

/*
 * Search for the ved.help file in the PATH of the user.
 * Assume that the file is ... bin/../man/help/ved.hlp
 */
LOCAL Uchar *
gethelpfile()
{
	Uchar	*hfile = HELPFILE;
	Uchar	*name;

	name = _gethelpfile("share/man/help/ved.help");
	if (name == NULL)
		name = _gethelpfile("man/help/ved.help");

	if (name == NULL)
		return (hfile);

	return (name);
}

LOCAL Uchar *
_gethelpfile(name)
	char	*name;
{
	char	*path = getenv("PATH");
	Uchar	*nbuf = curfname;	/* Construct path in curfname space */
	Uchar	*np;

	if (path == NULL)
		return (NULL);

	for (;;) {
		np = nbuf;
		while (*path != ':' && *path != '\0' &&
		    np < &nbuf[sizeof (curfname)-sizeof (name)-1])
				*np++ = *path++;
		*np = '\0';
		while (np > nbuf && np[-1] == '/')
			*--np = '\0';
		if (np >= &nbuf[4] && streql(C &np[-4], "/bin"))
			np = &np[-4];
		*np++ = '/';
		*np   = '\0';
		strcpy(C np, name);

		if (readable(nbuf))
			return (nbuf);

		if (*path == '\0')
			break;
		path++;
	}
	return (NULL);
}
