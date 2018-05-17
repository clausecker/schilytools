/* @(#)coloncmds.c	1.41 09/07/09 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)coloncmds.c	1.41 09/07/09 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	Commands that deal with ESC : commandline sequences
 *
 *	Copyright (c) 1986-2009 J. Schilling
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
#include <schily/varargs.h>
#include "terminal.h"

#define	iswhite(c)	((c) == ' ' || (c) == '\t')

/*#define	SYMSIZE	31*/
#define	SYMSIZE	127

LOCAL	Uchar	symbol[SYMSIZE+1];
LOCAL	Uchar	*cmdp;
LOCAL	int	cmdlen;

typedef	void (*function)	__PR((ewin_t *));

typedef struct {
	Uchar		*c_name;
	function	c_func;
	int		c_flag;
} _CMDTAB, *CMDTAB;

EXPORT	void	vcolon		__PR((ewin_t *wp));
LOCAL	CMDTAB	lookup		__PR((ewin_t *wp, Uchar* cmd, CMDTAB  cp));
LOCAL	BOOL	prefix		__PR((Uchar* pref, Uchar* s));
LOCAL	int	getsym		__PR((void));
LOCAL	BOOL	get_arg		__PR((ewin_t *wp));
LOCAL	BOOL	toint		__PR((ewin_t *wp, int *i));
LOCAL	BOOL	getint		__PR((ewin_t *wp, int *i, int low, int high));
LOCAL	void	bbind		__PR((ewin_t *wp));
LOCAL	void	bhelp		__PR((ewin_t *wp));
LOCAL	void	bmacro		__PR((ewin_t *wp));
LOCAL	void	bmap		__PR((ewin_t *wp));
LOCAL	void	bsmarkwrap	__PR((ewin_t *wp));
LOCAL	void	bnext_file	__PR((ewin_t *wp));
LOCAL	void	bprev_file	__PR((ewin_t *wp));
LOCAL	void	bsetcmd		__PR((ewin_t *wp));
LOCAL	void	bsautoindent	__PR((ewin_t *wp));
LOCAL	void	bsoptline	__PR((ewin_t *wp));
LOCAL	void	bspmargin	__PR((ewin_t *wp));
LOCAL	void	bsllen		__PR((ewin_t *wp));
LOCAL	void	bsmagic		__PR((ewin_t *wp));
LOCAL	void	bspsize		__PR((ewin_t *wp));
LOCAL	void	bstab		__PR((ewin_t *wp));
LOCAL	void	bstaglen	__PR((ewin_t *wp));
LOCAL	void	bstags		__PR((ewin_t *wp));
LOCAL	void	bswrapmargin	__PR((ewin_t *wp));
LOCAL	void	bsubst		__PR((ewin_t *wp));
LOCAL	void	btag		__PR((ewin_t *wp));
LOCAL	void	print_status	__PR((ewin_t *wp));
LOCAL	void	no_files	__PR((ewin_t *wp));
EXPORT	void	printscreen	__PR((ewin_t *wp, char *form, ...));
LOCAL	void	printbool	__PR((ewin_t *wp, BOOL  var, char *name));

#define	C_BOOL	1

LOCAL Uchar *no = UC"no";

LOCAL _CMDTAB cmdtab[] = {
			{ UC	"backup",	vbackup		},
			{ UC	"bind",		bbind		},
			{ UC	"help",		bhelp		},
			{ UC	"macro",	bmacro		},
			{ UC	"map",		bmap		},
			{ UC	"next",		bnext_file	},
			{ UC	"prev",		bprev_file	},
			{ UC	"quit",		vquit		},
			{ UC	"set",		bsetcmd		},
			{ UC	"substitute",	bsubst		},
			{ UC	"tag",		btag		},
			{ UC	NULL,		NULL		}};

LOCAL _CMDTAB settab[] = {
			{ UC	"autoindent",	bsautoindent,	C_BOOL	},
			{ UC	"linelen",	bsllen,		0	},
			{ UC	"magic",	bsmagic, 	C_BOOL	},
			{ UC	"markwrap",	bsmarkwrap,	C_BOOL	},
			{ UC	"optline",	bsoptline,	0	},
			{ UC	"pmargin",	bspmargin,	0	},
			{ UC	"psize",	bspsize,	0	},
			{ UC	"tabstop",	bstab,		0	},
			{ UC	"taglength",	bstaglen,	0	},
			{ UC	"tags",		bstags,		0	},
			{ UC	"wrapmargin",	bswrapmargin,	0	},
			{ UC	NULL,		NULL		}};

/*
 * Parse and execute a colon command
 */
EXPORT void
vcolon(wp)
	ewin_t	*wp;
{
	register	CMDTAB	cp;
			Uchar	cmdline[NAMESIZE];

	if ((cmdlen = getcmdline(wp, cmdline, sizeof (cmdline), ":")) == 0)
		return;
	cmdp = cmdline;
	if (!getsym()) {
		abortmsg(wp);
	} else if ((cp = lookup(wp, symbol, cmdtab)) != NULL) {
		(*cp->c_func)(wp);
	}
}

/*
 * Lookup command in command tab
 */
LOCAL CMDTAB
lookup(wp, cmd, cp)
		ewin_t	*wp;
		Uchar	*cmd;
	register CMDTAB	cp;
{
			CMDTAB	ocp = cp;
	register	CMDTAB	found = NULL;
			BOOL	first = TRUE;

again:
	for (; cp->c_name; cp++) {
		if (prefix(cmd, cp->c_name)) {
			if (!found) {
				found = cp;
			} else {
				writeerr(wp, "%s: AMBIGOUS", cmd);
				return (NULL);
			}
		}
	}
	if (!first && found && !(found->c_flag & C_BOOL))
		found = NULL;
	if (!found) {
		if (first && prefix(no, cmd)) {
			first = FALSE;
			cmd = &cmd[2];
			cp = ocp;
			goto again;
		}
		writeerr(wp, "%s: UNKNOWN", cmd);
	}
	return (found);
}

/*
 * Check if a string starts with a given prefix
 */
LOCAL BOOL
prefix(pref, s)
	Uchar	*pref;
	Uchar	*s;
{
	while (*pref) {
		if (*pref++ != *s++)
			return (FALSE);
	}
	return (TRUE);
}

/*
 * Get next symbol (word)
 */
LOCAL int
getsym()
{
	register Uchar	*r_cmd = cmdp;
	register Uchar	*r_sym = symbol;
	register int	i;

	while (*r_cmd && iswhite(*r_cmd))
		r_cmd++;
	for (i = 0; *r_cmd && !iswhite(*r_cmd) && i < SYMSIZE; i++)
		*r_sym++ = *r_cmd++;
	*r_sym = '\0';
	cmdlen -= r_cmd - cmdp;
	cmdp = r_cmd;
	return (i);
}

/*
 * Get next symbol (word) and check if one exists
 */
LOCAL BOOL
get_arg(wp)
	ewin_t	*wp;
{
	if (!getsym()) {
		writeerr(wp, "NO ARG");
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Get integer number, check if valid
 */
LOCAL BOOL
toint(wp, i)
	ewin_t	*wp;
	int	*i;
{
	if (*astoi(C symbol, i) != '\0') {
		writeerr(wp, "NOT A NUMBER: %s", symbol);
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Get integer number with bounds
 */
LOCAL BOOL
getint(wp, i, low, high)
	ewin_t	*wp;
	int	*i;
	int	low;
	int	high;
{
	if (!get_arg(wp))
		return (FALSE);

	if (toint(wp, i)) {
		if (*i < low || *i >= high) {
			writeerr(wp, "BAD ARG: %d", *i);
			return (FALSE);
		}
		return (TRUE);
	}
	return (FALSE);
}

/*
 * Run bind command
 */
LOCAL void
bbind(wp)
	ewin_t	*wp;
{
	bindcmd(wp, cmdp, cmdlen);
}

/*
 * Give online help for colon commands
 */
LOCAL void
bhelp(wp)
	ewin_t	*wp;
{
	register	CMDTAB	cp;

	MOVE_CURSOR_ABS(wp, 1, 0);
	printscreen(wp, "Available Commands are:\n");
	for (cp = cmdtab; cp->c_name; cp++)
		printscreen(wp, "%s\n", cp->c_name);
	wait_for_confirm(wp);
	vredisp(wp);
}

char mstr[128];

/*
 * Set temporary macro (call with ESC *)
 */
/* ARGSUSED */
LOCAL void
bmacro(wp)
	ewin_t	*wp;
{
	strncpy(mstr, C &cmdp[1], sizeof (mstr));
	mstr[sizeof (mstr)-1] = '\0';
}

/*
 * List current mappings
 */
LOCAL void
bmap(wp)
	ewin_t	*wp;
{
	MOVE_CURSOR_ABS(wp, 1, 0);
	list_map(wp);
	wait_for_confirm(wp);
	vredisp(wp);
}

/*
 * Set markwrap/no-markwrap
 */
LOCAL void
bsmarkwrap(wp)
	ewin_t	*wp;
{
	BOOL	save = wp->markwrap;

	wp->markwrap = !prefix(no, symbol);
	if (save != wp->markwrap) {
		wp->llen += wp->markwrap ? -1 : 1;
		vredisp(wp);
		setpos(wp);
	}
}

/*
 * Change to next file in argument list
 */
LOCAL void
bnext_file(wp)
	ewin_t	*wp;
{
	if (fileidx >= nfiles-1)
		no_files(wp);
	else if (!change_file(wp, files[++fileidx]))
		--fileidx;
	else
		newwindow(wp);
}

/*
 * Change to previous file in argument list
 */
LOCAL void
bprev_file(wp)
	ewin_t	*wp;
{
	if (fileidx <= 0)
		no_files(wp);
	else if (!change_file(wp, files[--fileidx]))
		++fileidx;
	else
		newwindow(wp);
}

/*
 * Set function that calls other sub-set function
 */
LOCAL void
bsetcmd(wp)
	ewin_t	*wp;
{
	register	CMDTAB	cp;

	if (!getsym())
		print_status(wp);
	else if ((cp = lookup(wp, symbol, settab)) != NULL)
		(*cp->c_func)(wp);
}

/*
 * Set auto-indent/no-auto-indent
 */
LOCAL void
bsautoindent(wp)
	ewin_t	*wp;
{
	wp->autoindent = !prefix(no, symbol);
}

/*
 * Set optimal line for screen adjust
 */
LOCAL void
bsoptline(wp)
	ewin_t	*wp;
{
	int	i;

	if (getint(wp, &i, 1, wp->psize - wp->pmargin))
		wp->optline = i;
}

/*
 * Set page-margin.
 * This sets the number of lines the curser must stay away from
 * the top or bottom of the screen.
 */
LOCAL void
bspmargin(wp)
	ewin_t	*wp;
{
	int	i;

	if (getint(wp, &i, 0, min(wp->psize/2, wp->optline)))
		wp->pmargin = i;
}

/*
 * Set line-len
 */
LOCAL void
bsllen(wp)
	ewin_t	*wp;
{
	int	save = wp->llen;

	if (getint(wp, &wp->llen, 1, 1000) && wp->llen != save) {
		vredisp(wp);
		setpos(wp);
	}
}

/*
 * Set magic/no-magic
 */
LOCAL void
bsmagic(wp)
	ewin_t	*wp;
{
	wp->magic = !prefix(no, symbol);
}

/*
 * Set page-size
 */
LOCAL void
bspsize(wp)
	ewin_t	*wp;
{
	int	save = wp->psize;

	if (getint(wp, &wp->psize, 1, 1000) && wp->psize != save) {
		if (wp->optline == save/2 || wp->optline > wp->psize)
			wp->optline = wp->psize/2;
		if (wp->pmargin > min(wp->psize/2, wp->optline))
			wp->pmargin = 0;
		vredisp(wp);
		setpos(wp);
	}
}

/*
 * Set width of a tab
 */
LOCAL void
bstab(wp)
	ewin_t	*wp;
{
	int	save = wp->tabstop;

	if (getint(wp, &wp->tabstop, 1, wp->llen) && wp->tabstop != save) {
		vredisp(wp);
		setpos(wp);
	}
}

/*
 * Set valig tagstring length
 */
LOCAL void
bstaglen(wp)
	ewin_t	*wp;
{
	int	i;

	if (getint(wp, &i, 0, 100) && i >= 0)
		taglength = i;
}

/*
 * Set tag database search path
 */
LOCAL void
bstags(wp)
	ewin_t	*wp;
{
	if (!get_arg(wp))
		return;

	strncpy(C tags, C symbol, NAMESIZE);
	tags[NAMESIZE-1] = '\0';
}

/*
 * Set auto-wrapmargin value
 */
LOCAL void
bswrapmargin(wp)
	ewin_t	*wp;
{
	getint(wp, &wp->wrapmargin, 0, wp->llen);
}

/*
 * Run substitute command
 */
LOCAL void
bsubst(wp)
	ewin_t	*wp;
{
	subst(wp, cmdp, cmdlen);
}

/*
 * Go to tag that was specified on command line
 */
LOCAL void
btag(wp)
	ewin_t	*wp;
{
	if (!get_arg(wp))
		return;

	gototag(wp, symbol);
}

/*
 * Print a summary of the values of all variables
 */
LOCAL void
print_status(wp)
	ewin_t	*wp;
{
	MOVE_CURSOR_ABS(wp, 1, 0);
	printscreen(wp, "psize: %-10d linelen: %-10d optline: %-10d\n",
						wp->psize, wp->llen, wp->optline);
	printscreen(wp, "pmargin: %d\n", wp->pmargin);
	printscreen(wp, "wrapmargin: %d\n", wp->wrapmargin);
	printscreen(wp, "maxlinelen: %d\n", wp->maxlinelen);
	printscreen(wp, "tabstop: %d\n", wp->tabstop);
	printbool(wp, wp->raw8, "raw8");
	printscreen(wp, "pid: %d\n", pid);
	printscreen(wp, "modflg: %ld\n", wp->modflg);
	printscreen(wp, "curnum: %lld\n", (Llong)wp->curnum);
/*	printscreen(wp, "mult: %d\n", mult);*/
	printscreen(wp, "cursor.hp: %d(%d) cursor.vp: %d(%d)\n",
		cursor.hp, realhp(wp, &cursor), cursor.vp, realvp(wp, &cursor));
	printscreen(wp, "window: %lld\n", (Llong)wp->window);
	printscreen(wp, "ewindow: %lld\n", (Llong)wp->ewindow);
	printscreen(wp, "dot: %lld\n", (Llong)wp->dot);
	printscreen(wp, "eof: %lld\n", (Llong)wp->eof);
	printscreen(wp, "mark: %lld\n", (Llong)wp->mark);
	printbool(wp, wp->markvalid, "markvalid");
	printbool(wp, wp->autoindent, "autoindent");
	if ((wp->eflags & FREADONLY) != 0)
		printscreen(wp, "readonly (locked by other program)\n");
	else
		printbool(wp, ReadOnly, "readonly");
	printbool(wp, !nobak, "bak");
	printbool(wp, !noedtmp, "edtmp");
	printbool(wp, recover, "recover");
	printbool(wp, wp->magic, "magic");
	printscreen(wp, wp->overstrikemode ? "overstrikemode\n" : "insertmode\n");
	printbool(wp, wp->visible, "visible");
	printbool(wp, wp->markwrap, "markwrap");
	printscreen(wp, "taglength: %d tags: '%s'\n", taglength, tags);
	wait_for_confirm(wp);
	vredisp(wp);
}

/*
 * Print warning at end of ESC : next/prev file list
 */
LOCAL void
no_files(wp)
	ewin_t	*wp;
{
	writeerr(wp, "NO MORE FILES");
}

/*
 * Print a line on screen, wait if the maximum
 * numbers of lines on screen is reached
 */
/* PRINTFLIKE2 */
#ifdef	PROTOTYPES
EXPORT void
printscreen(ewin_t *wp, char *form, ...)
#else
EXPORT void
printscreen(wp, form, va_alist)
	ewin_t	*wp;
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	Uchar	temp[NAMESIZE];
	Uchar	tform[NAMESIZE];
	int	slen;
	BOOL	nl	= FALSE;

	if (cpos.vp >= wp->psize) {
		wait_continue(wp);
		MOVE_CURSOR_ABS(wp, 1, 0);
	}
	strncpy(C tform, form, NAMESIZE);
	tform[NAMESIZE-1] = '\0';
	slen = strlen(C tform);
	if (tform[slen-1] == '\n') {
		nl = TRUE;
		tform[slen-1] = '\0';
	}
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	snprintf(C temp, sizeof (temp), "%r", tform, args);
	va_end(args);
	output(temp);
	CLEAR_TO_EOF_LINE(wp);
	if (nl) {
		addchar('\n');
	}
}

/*
 * Print a boolean value
 */
LOCAL void
printbool(wp, var, name)
	ewin_t	*wp;
	BOOL	var;
	char	*name;
{
	printscreen(wp, "%s%s\n", var ? "" : "no", name);
}
