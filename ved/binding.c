/* @(#)binding.c	1.13 09/07/09 Copyright 1984,1997,2000-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)binding.c	1.13 09/07/09 Copyright 1984,1997,2000-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 1984,1997,2000-2009 J. Schilling
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

#define	CTRL(c)	((c) & 037)

struct funcs {
	void	(*func)	__PR((ewin_t *));	/* The function pointer	*/
	char	*name;				/* The function name	*/
	Uchar	deftab;				/* The default table	*/
	Uchar	defmap;				/* The default mapping	*/
};

EXPORT	void	(*chartab[NCTAB][0200])	__PR((ewin_t *));
EXPORT	int	ctabidx;	/* The table idx where we take commands from */

LOCAL	void	setesctab	__PR((ewin_t *wp));
LOCAL	void	setalttab	__PR((ewin_t *wp));
LOCAL	void	saltesctab	__PR((ewin_t *wp));
LOCAL	void	setx		__PR((ewin_t *wp));
LOCAL	void	init_table	__PR((void (*tab[0200])(ewin_t *), void (*func)(ewin_t *)));
LOCAL	void	init_chars	__PR((void (*tab[0200])(ewin_t *), void (*func)(ewin_t *)));
EXPORT	void	init_binding	__PR((void));
EXPORT	void	bindcmd		__PR((ewin_t *wp, Uchar* cmd, int cmdlen));

struct funcs	funcs[] = {

/*
 * Functions that have many bindings and need to be initialized separately.
 */
{ vnorm,	"insert-this-char",	TABNONE,	0		},
{ vsnorm,	"mark-insert-this-char", TABNONE,	0		},
{ vnum,		"set-mult",		TABNONE,	0		},
{ vxmac,	"execute-macro",	TABNONE,	0		},
{ verror,	"error-function",	TABNONE,	0		},

/*
 * Functions that have one binding and are initialized from this table.
 */
{ vwhere,	"where-from-top",	CTAB,		CTRL('@')	},
{ vewhere,	"where-from-bot",	ESCTAB,		CTRL('@')	},
{ vswhere,	"where-mark-from-top",	ALTTAB,		CTRL('@')	},
{ vsewhere,	"where-mark-from-bot",	AESCTAB,	CTRL('@')	},

{ vbegin,	"begin-line",		CTAB,		CTRL('a')	},
{ vpbegin,	"begin-para",		ESCTAB,		CTRL('a')	},
{ vsbegin,	"mark-begin-line",	ALTTAB,		CTRL('a')	},
{ vspbegin,	"mark-begin-para",	AESCTAB,	CTRL('a')	},

{ vtop,		"top",			CTAB,		CTRL('b')	},
{ vbottom,	"bottom",		ESCTAB,		CTRL('b')	},
{ vstop,	"mark-top",		ALTTAB,		CTRL('b')	},
{ vsbottom,	"mark-bottom",		AESCTAB,	CTRL('b')	},

{ vquit,	"quit",			CTAB,		CTRL('c')	},
{ vbackup,	"backup",		ESCTAB,		CTRL('c')	},
/* ALT-^C	is unused */
/* ALT-ESC-^C	is unused */

{ vdel,		"delete-char",		CTAB,		CTRL('d')	},
{ vwdel,	"delete-word",		ESCTAB,		CTRL('d')	},

{ vend,		"end-line",		CTAB,		CTRL('e')	},
{ vpend,	"end-para",		ESCTAB,		CTRL('e')	},
{ vsend,	"mark-end-line",	ALTTAB,		CTRL('e')	},
{ vspend,	"mark-end-para",	AESCTAB,	CTRL('e')	},

{ vforw,	"forward-char",		CTAB,		CTRL('f')	},
{ vwforw,	"forward-word",		ESCTAB,		CTRL('f')	},
{ vsforw,	"mark-forward-char",	ALTTAB,		CTRL('f')	},
{ vswforw,	"mark-forward-word",	AESCTAB,	CTRL('f')	},

{ vread,	"read-file",		CTAB,		CTRL('g')	},
{ vchange,	"change-file",		ESCTAB,		CTRL('g')	},
{ vemac,	"edit-macro-file",	ALTTAB,		CTRL('g')	},
/* ALT-ESC-^G	is unused */

{ vrev,		"backward-char",	CTAB,		CTRL('h')	},
{ vwrev,	"backward-word",	ESCTAB,		CTRL('h')	},
{ vsrev,	"mark-backward-char",	ALTTAB,		CTRL('h')	},
{ vswrev,	"mark-backward-word",	AESCTAB,	CTRL('h')	},

{ vnl,		"new-line",		CTAB,		CTRL('j')	},
/* ESC-^J	is unused */
{ vjumpmark,	"jump-to-mark",		ALTTAB,		CTRL('j')	},
{ vexchmarkdot,	"switch-mark-dot",	AESCTAB,	CTRL('j')	},

{ vkill,	"delete-line",		CTAB,		CTRL('k')	},
{ vpkill,	"delete-para",		ESCTAB,		CTRL('k')	},
{ vskill,	"delete-selection",	ALTTAB,		CTRL('k')	},
/* ALT-ESC-^K	is unused */

{ vredisp,	"redisplay",		CTAB,		CTRL('l')	},
{ vadjwin,	"adjust-window",	ESCTAB,		CTRL('l')	},
{ vltopwin,	"adjust-top-line",	ALTTAB,		CTRL('l')	},
/* ALT-ESC-^L	is unused */

/*{ vnl,		"new-line",		CTAB,		CTRL('m')	},*/
{ vmode,	"set-mode",		ESCTAB,		CTRL('m')	},
{ vsetmark,	"set-mark",		ALTTAB,		CTRL('m')	},
/* ALT-ESC-^M	is unused */

{ vdown,	"down-line",		CTAB,		CTRL('n')	},
{ vpdown,	"down-para",		ESCTAB,		CTRL('n')	},
{ vsdown,	"mark-down-line",	ALTTAB,		CTRL('n')	},
{ vspdwn,	"mark-down-para",	AESCTAB,	CTRL('n')	},

{ vopen,	"open-line",		CTAB,		CTRL('o')	},
/* ESC-^O	is unused */
{ vsopen,	"mark-open-line",	ALTTAB,		CTRL('o')	},
/* ALT-ESC-^O	is unused */

{ vup,		"up-line",		CTAB,		CTRL('p')	},
{ vpup,		"up-para",		ESCTAB,		CTRL('p')	},
{ vsup,		"mark-up-line",		ALTTAB,		CTRL('p')	},
{ vspup,	"mark-up-para",		AESCTAB,	CTRL('p')	},

/* ^Q		is unused - never use it or VED will fail with a VT100 */
/* ESC-^Q	is unused - never use it or VED will fail with a VT100 */
/* ALT-^Q	is unused - never use it or VED will fail with a VT100 */
/* ALT-ESC-^Q	is unused - never use it or VED will fail with a VT100 */

{ vsearch,	"search",		CTAB,		CTRL('r')	},
{ vrsearch,	"reverse-search",	ESCTAB,		CTRL('r')	},
{ vssearch,	"mark-search",		ALTTAB,		CTRL('r')	},
{ vsrsearch,	"mark-reverse-search", 	AESCTAB,	CTRL('r')	},

/* ^S		is unused - never use it or VED will fail with a VT100 */
/* ESC-^S	is unused - never use it or VED will fail with a VT100 */
/* ALT-^S	is unused - never use it or VED will fail with a VT100 */
/* ALT-ESC-^S	is unused - never use it or VED will fail with a VT100 */

{ vlsave,	"save-line",		CTAB,		CTRL('t')	},
{ vpsave,	"save-para",		ESCTAB,		CTRL('t')	},
{ vssave,	"save-selection",	ALTTAB,		CTRL('t')	},
/* ALT-ESC-^T	is unused */

{ vmult,	"multiply",		CTAB,		CTRL('u')	},
{ vsmult,	"set-multiplyer",	ESCTAB,		CTRL('u')	},
/* ALT-^U	is unused */
/* ALT-ESC-^U	is unused */

{ vget,		"get-take",		CTAB,		CTRL('v')	},
{ vgetclr,	"get-take-clear",	ESCTAB,		CTRL('v')	},
{ vsget,	"delete-sel-get-take",	ALTTAB,		CTRL('v')	},
{ vsgetclr,	"delete-sel-get-take-clear", AESCTAB,	CTRL('v')	},

{ vwrite,	"write-file",		CTAB,		CTRL('w')	},
{ vwrtake,	"write-take",		ESCTAB,		CTRL('w')	},
{ vswrite,	"write-sel",		ALTTAB,		CTRL('w')	},
/* ALT-ESC-^W	is unused */

{ setx,		"set-x-cmd",		CTAB,		CTRL('x')	},
{ vtexec,	"execute-take",		ESCTAB,		CTRL('x')	},
{ vsexec,	"execute-selection",	ALTTAB,		CTRL('x')	},
/* ALT-ESC-^X	is unused */

{ vcsave,	"save-char",		CTAB,		CTRL('y')	},
{ vwsave,	"save-word",		ESCTAB,		CTRL('y')	},
/* ALT-^Y	is unused */
/* ALT-ESC-^Y	is unused */

{ vagainsrch,	"re-search",		CTAB,		CTRL('z')	},
{ vrevsrch,	"re-reverse-search",	ESCTAB,		CTRL('z')	},
{ vsagainsrch,	"mark-re-search",	ALTTAB,		CTRL('z')	},
{ vsrevsrch,	"mark-re-reverse-search", AESCTAB,	CTRL('z')	},

{ setesctab,	"set-escape-cmd",	CTAB,		CTRL('[')	},
/* ESC-^[	is unused */
{ saltesctab,	"set-altesc-cmd",	ALTTAB,		CTRL('[')	},
/* ALT-ESC-^[	is unused */

{ vtname,	"set-take-name",	CTAB,		CTRL('\\')	},
{ vcleartake,	"clear-take",		ESCTAB,		CTRL('\\')	},
/* ALT-^\	is unused */
/* ALT-ESC-^\	is unused */

{ setalttab,	"set-alt-cmd",		CTAB,		CTRL(']')	},
{ saltesctab,	"set-altesc-cmd",	ESCTAB,		CTRL(']')	},
{ vtag,		"goto-tag",		ALTTAB,		CTRL(']')	},
{ vrtag,	"pop-tag-stack",	AESCTAB,	CTRL(']')	},

{ vquote,	"quote-char",		CTAB,		CTRL('^')	},
{ v8quote,	"quote8-char",		ESCTAB,		CTRL('^')	},
{ v8cntlq,	"quote8ctl-char",	ALTTAB,		CTRL('^')	},
{ vhex,		"quotehex-char",	AESCTAB,	CTRL('^')	},

{ vundel,	"undo-del",		CTAB,		CTRL('_')	},
/* ESC-^_	is unused */
{ vclearmark,	"clear-mark",		ALTTAB,		CTRL('_')	},
/* ALT-ESC-^_	is unused */

{ vrub,		"rubout-char",		CTAB,		DEL		},
{ vwrub,	"rubout-word",		ESCTAB,		DEL		},
/* ALT-DEL	is unused */
/* ALT-ESC-DEL	is unused */

{ vbrack,	"match-bracket",	ESCTAB,		'%'		},
{ vmac,		"execute-temp-macro",	ESCTAB,		'*'		},
{ vcolon,	"colon-cmd",		ESCTAB,		':'		},
{ vpagedwn,	"page-down",		ESCTAB,		'n'		},
{ vspagedwn,	"mark-page-down",	AESCTAB,	'n'		},
{ vpageup,	"page-up",		ESCTAB,		'p'		},
{ vspageup,	"mark-page-up",		AESCTAB,	'p'		},

{ vhelp,	"help",			XTAB,		CTRL('h')	},
{ vexec,	"execute",		XTAB,		CTRL('x')	},
{ vsuspend,	"suspend",		XTAB,		CTRL('z')	},

/*
 * End marker
 */
{ NULL,		NULL,			TABNONE,	0		},
};

/*
 * Next command is an escape command
 */
LOCAL void
setesctab(wp)
	ewin_t	*wp;
{
	ctabidx = ESCTAB;
	wp->eflags &= ~COLUPDATE;
}

/*
 * Next command is an alternate command
 */
LOCAL void
setalttab(wp)
	ewin_t	*wp;
{
	ctabidx = ALTTAB;
	wp->eflags &= ~COLUPDATE;
}

/*
 * Next command is an alternate escape command
 */
LOCAL void
saltesctab(wp)
	ewin_t	*wp;
{
	ctabidx = AESCTAB;
	wp->eflags &= ~COLUPDATE;
}

LOCAL void
setx(wp)
	ewin_t	*wp;
{
	ctabidx = XTAB;
	wp->eflags &= ~COLUPDATE;
}

/*#ifdef	BIND_VERIFY*/
#ifdef	BIND_VERIFY

#include "btab.c"

LOCAL	void	cmptab		__PR((char *name,
					void (*tab[0200])(ewin_t *),
					void (*tab2[0200])(ewin_t *)));

LOCAL void
cmptab(name, tab, tab2)
	char	*name;
	void	(*tab[0200])	__PR((ewin_t *));
	void	(*tab2[0200])	__PR((ewin_t *));
{
	register int	i;

	for (i = 0; i < 128; i++) {
		if (tab[i] != tab2[i]) {
			printf("%s[%d]: %p != %p\r\n",
				name, i,
				(void *)tab[i],
				(void *)tab2[i]);
		}
	}
}
#endif

LOCAL void
init_table(tab, func)
	void	(*tab[0200])	__PR((ewin_t *));
	void	(*func)		__PR((ewin_t *));
{
	register int	i;

	for (i = 0; i < 128; i++) {
		tab[i] = func;
	}
}

LOCAL void
init_chars(tab, func)
	void	(*tab[0200])	__PR((ewin_t *));
	void	(*func)		__PR((ewin_t *));
{
	register int	i;

	for (i = 32; i < 127; i++) {
		tab[i] = func;
	}
}

EXPORT void
init_binding()
{
	int		i;
	struct	funcs	*fp;

	/*
	 * First initialize all tables with the error function.
	 */
	for (i = TABFIRST; i <= TABLAST; i++)
		init_table(chartab[i], verror);

	/*
	 * Initialize non-control characters with insert function
	 * resp. macro execution.
	 */
	init_chars(chartab[CTAB], vnorm);
	init_chars(chartab[ALTTAB], vsnorm);
	init_chars(chartab[ESCTAB], vxmac);

	/*
	 * Now install all default bindings from the function table.
	 */
	for (fp = funcs; fp->func; fp++) {
		if (fp->deftab <= TABLAST)
			chartab[fp->deftab][fp->defmap] = fp->func;
	}

	/*
	 * TAB is a control char but we also like to insert a TAB a normal char
	 */
	chartab[CTAB][CTRL('i')] = vnorm;
	chartab[ALTTAB][CTRL('i')] = vsnorm;

	/*
	 * <CR> is an alias for <NEW LINE>
	 */
	chartab[CTAB][CTRL('m')] = vnl;

	/*
	 * If ESC is followed by a number, we would like to set the multiplyer.
	 */
	for (i = '0'; i <= '9'; i++)
		chartab[ESCTAB][i] = vnum;

	/*
	 * Allow macros to start with ESC.
	 * This makes it possible to bind a macro to a function key.
	 * XXX Not really needed anymore as we support mappings too.
	 */
	chartab[ESCTAB][CTRL('[')] = vxmac;

#ifdef	BIND_VERIFY
	cmptab("chartab", Ochartab[CTAB], chartab[CTAB]);
	cmptab("esctab", Ochartab[ESCTAB], chartab[ESCTAB]);
	cmptab("alttab", Ochartab[ALTTAB], chartab[ALTTAB]);
	cmptab("altesctab", Ochartab[AESCTAB], chartab[AESCTAB]);
	cmptab("xtab", Ochartab[XTAB], chartab[XTAB]);
	fflush(stdout);
	sleep(2);
#endif
}

LOCAL char *tabnames[] = {
	"char",
	"ESC",
	"ALT",
	"ALTESC",
	"X",
};

#include "terminal.h"

/* ARGSUSED */
EXPORT void
bindcmd(wp, cmd, cmdlen)
	ewin_t	*wp;
	Uchar	*cmd;
	int	cmdlen;
{
	int		i;
	struct	funcs	*fp;
	char	*tabname;
	char	*charname;
extern	Uchar   *ctab[256];

	MOVE_CURSOR_ABS(wp, 1, 0);
	for (fp = funcs; fp->func; fp++) {
		if (fp->deftab <= TABLAST) {
			tabname = tabnames[fp->deftab];
			charname = (char *)ctab[fp->defmap];
		} else {
			charname = tabname = "NONE";
		}
		printscreen(wp, "%s-%s%n",
			tabname, charname, &i);
		printscreen(wp, "%*s%s\n",
			12-i, "", fp->name);
	}
	wait_for_confirm(wp);
	vredisp(wp);
}
