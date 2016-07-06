/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)msg.c	1.15	06/06/20 SMI"
#endif

#include "defs.h"
#include <schily/errno.h>

/*
 * Copyright 2008-2016 J. Schilling
 *
 * @(#)msg.c	1.67 16/07/06 2008-2016 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)msg.c	1.67 16/07/06 2008-2016 J. Schilling";
#endif

/*
 *	UNIX shell
 */

#include	"sym.h"

/*
 * error messages
 */
#ifndef __STDC__
#define	const
#endif

const char	badopt[]	= "bad option(s)";
const char	mailmsg[]	= "you have mail\n";
const char	nospace[]	= "no space";
const char	nostack[]	= "no stack space";
const char	synmsg[]	= "syntax error";

const char	badnum[]	= "bad number";
const char	badsig[]	= "bad signal";
const char	badid[]		= "invalid id";
const char	badparam[]	= "parameter null or not set";
const char	unset[]		= "parameter not set";
const char	badsub[]	= "bad substitution";
const char	badcreate[]	= "cannot create";
#ifdef	DO_NOCLOBBER
const char	eclobber[]	= "file already exists";
#endif
const char	nofork[]	= "fork failed - too many processes";
const char	noswap[]	= "cannot fork: no swap space";
const char	restricted[]	= "restricted";
const char	piperr[]	= "cannot make pipe";
const char	badopen[]	= "cannot open";
const char	coredump[]	= " - core dumped";
const char	arglist[]	= "arg list too long";
const char	txtbsy[]	= "text busy";
const char	toobig[]	= "too big";
const char	badexec[]	= "cannot execute";
const char	notfound[]	= "not found";
const char	badfile[]	= "bad file number";
const char	badshift[]	= "cannot shift";
const char	baddir[]	= "bad directory";
const char	badoff[]	= "bad offset";
const char	emptystack[]	= "stack empty";
const char	badtrap[]	= "bad trap";
const char	wtfailed[]	= "is read only";
const char	notid[]		= "is not an identifier";
const char	badulimit[]	= "exceeds allowable limit";
const char	badreturn[]	= "cannot return when not in function";
#ifndef	DO_POSIX_UNSET
const char	badexport[]	= "cannot export functions";
#endif
#ifndef	DO_POSIX_UNSET
const char	badunset[]	= "cannot unset";
#endif
const char	nohome[]	= "no home directory";
const char	badperm[]	= "execute permission denied";
const char	longpwd[]	= "sh error: pwd too long";
const char	mssgargn[]	= "missing arguments";
const char	toomanyargs[]	= "too many arguments";
#ifdef	ELIBACC
const char	libacc[]	= "can't access a needed shared library";
#endif
#ifdef	ELIBBAD
const char	libbad[]	= "accessing a corrupted shared library";
#endif
#ifdef	ELIBSCN
const char	libscn[]	= ".lib section in a.out corrupted";
#endif
#ifdef	ELIBMAX
const char	libmax[]	= "attempting to link in too many libs";
#endif
#ifdef	EMULTIHOP
const char	emultihop[]	= "Multihop attempted";
#endif
const char	nulldir[]	= "null directory";
const char	enotdir[]	= "not a directory";
const char	eisdir[]	= "is a directory";
const char	enoent[]	= "does not exist";
const char	eacces[]	= "permission denied";
#ifdef	ENOLINK
const char	enolink[]	= "remote link inactive";
#endif
const char	exited[]	= "Done";
const char	running[]	= "Running";
const char	ambiguous[]	= "ambiguous";
const char	usage[]		= "usage";
const char	nojc[]		= "no job control";
#ifdef	DO_SYSALIAS
const char	aliasuse[]	=
		"alias [-a] [-e] [-g] [-l] [-p] [-r] [--raw] [name[=value]...]";
const char	unaliasuse[]	= "unalias [-a] [-g] [-l] [-p] [name...]";
#endif
#ifdef	DO_SYSREPEAT
const char	repuse[]	= "repeat [-c count] [-d delay] cmd [args]";
#endif
#ifdef	DO_SYSBUILTIN
const char	builtinuse[]	=
		"builtin [-d] [-f lib] [-i] [-s] [pathname...]";
#endif
const char	stopuse[]	= "stop id ...";
#ifdef	DO_POSIX_TRAP
const char	trapuse[]	= "trap [action condition ...]";
#endif
const char	ulimuse[]	=
		"ulimit [ -HSacdefilmnqrstuvLMPRS ] [ limit ]";
const char	killuse[]	=
		"kill [ [ -sig | -s sig ] id ... | -l [ signo ... ] ]";
const char	jobsuse[]	= "jobs [ [ -l | -p ] [ id ... ] | -x cmd ]";
const char	nosuchjob[]	= "no such job";
const char	nosuchpid[]	= "no such process";
const char	nosuchpgid[]	= "no such process group";
const char	nocurjob[]	= "no current job";
const char	jobsstopped[]	= "there are stopped jobs";
const char	jobsrunning[]	= "there are running jobs";
const char	loginsh[]	= "cannot stop login shell";
const char	nlorsemi[]	= "newline or ;";
const char	signalnum[]	= "Signal ";
const char	badpwd[]	= "cannot determine current directory";
const char	badlocale[]	= "couldn't set locale correctly\n";
const char	nobracket[]	= "] missing";
const char	noparen[]	= ") expected";
const char	noarg[]		= "argument expected";
const char	unimplemented[]	= "unimplemented";
#if	defined(DO_DOL_PAREN) || defined(DO_SYSATEXPR)
const char	divzero[]	= "division by zero";
#endif

/*
 * messages for 'builtin' functions
 */
const char	btest[]		= "test";
const char	badop[]		= "unknown operator ";
const char	badumask[]	= "bad umask";

/*
 * built in names
 */
const char	pathname[]	= "PATH";
const char	ppidname[]	= "PPID";
const char	cdpname[]	= "CDPATH";
const char	envname[]	= "ENV";
const char	homename[]	= "HOME";
const char	mailname[]	= "MAIL";
const char	ifsname[]	= "IFS";
const char	ps1name[]	= "PS1";
const char	ps2name[]	= "PS2";
const char	ps3name[]	= "PS3";
const char	ps4name[]	= "PS4";
const char	linenoname[]	= "LINENO";
const char	mchkname[]	= "MAILCHECK";
const char	opwdname[]	= "OLDPWD";
const char	pwdname[]	= "PWD";
const char	repname[]	= "REPLY";
const char	acctname[]	= "SHACCT";
const char	mailpname[]	= "MAILPATH";
const char	timefmtname[]	= "TIMEFORMAT";

/*
 * string constants
 */
const char	nullstr[]	= "";
const char	sptbnl[]	= " \t\n";
const char	defpath[]	= "/usr/bin:";
#ifdef	DO_SYSCOMMAND
/*
 * The correct POSIX default PATH for Solaris + "/bin" after "/usr/bin" should
 * be sufficient for all platforms.
 */
const char	defppath[]	=
"/usr/xpg6/bin:/usr/xpg4/bin:/usr/ccs/bin:/usr/bin:/bin:/opt/SUNWspro/bin:";
#endif
const char	colon[]		= ": ";
const char	minus[]		= "-";
const char	endoffile[]	= "end of file";
const char	unexpected[]	= " unexpected";
const char	atline[]	= " at line ";
const char	devnull[]	= "/dev/null";
const char	execpmsg[]	= "+ ";
const char	selectmsg[]	= "#? ";
const char	readmsg[]	= "> ";
const char	stdprompt[]	= "$ ";
const char	supprompt[]	= "# ";
const char	profile[]	= ".profile";
const char	sysprofile[]	= "/etc/profile";
#ifdef	DO_SHRCFILES
const char	rcfile[]	= "$HOME/.shrc";
const char	sysrcfile[]	= "/etc/sh.shrc";
#endif
const char	globalname[]	= ".globals";
const char	localname[]	= ".locals";
const unsigned char shname[]	= "sh (Schily Bourne Shell)";

/*
 * locale testing
 */
const char	localedir[]	= "/usr/lib/locale";
int		localedir_exists;

/*
 * tables
 */

const struct sysnod reserved[] =
{
#ifdef	DO_NOTSYM
	{ "!",		NOTSYM	},
#endif
	{ "case",	CASYM	},
	{ "do",		DOSYM	},
	{ "done",	ODSYM	},
	{ "elif",	EFSYM	},
	{ "else",	ELSYM	},
	{ "esac",	ESSYM	},
	{ "fi",		FISYM	},
	{ "for",	FORSYM	},
	{ "if",		IFSYM	},
	{ "in",		INSYM	},
#ifdef	DO_SELECT
	{ "select",	SELSYM	},
#endif
	{ "then",	THSYM	},
#ifdef	DO_TIME
	{ "time",	TIMSYM	},
#endif
	{ "until",	UNSYM	},
	{ "while",	WHSYM	},
	{ "{",		BRSYM	},
	{ "}",		KTSYM	}
};

const int no_reserved = sizeof (reserved)/sizeof (struct sysnod);

const char	export[] = "export";
const char	readonly[] = "readonly";

/*
 * In the POSIX teleconferences from November and December 2014, we
 * tried to create a new notation for shell builtins and the rules
 * they need to follow.
 *
 * Special built-in utilities and intrinsic utilities are called before any
 * attempt for a PATH search is done.
 * Special built-in utilities in addition keep variable assignement in effect
 * after the builtin completes.
 * See http://austingroupbugs.net/view.php?id=854#c2495 for the new POSIX
 * classification of shell builtins.
 *
 * Built-ins marked with "S" are POSIX special built-in utilities.
 * Built-ins marked with "I" are POSIX intrinsic utilities.
 * Built-ins marked with "i" may not be permitted intrincics in the future.
 * Built-ins marked with "-" do not follow utility syntax guidelines.
 * Built-ins marked with "U" follow utility syntax guidelines and support --.
 *
 * The POSIX standard in addition defines the regular intrinsic utility
 * "fc" that is not part of the Bourne Shell as the Bourne Shell uses a command
 * line history editor taken from "bsh" that was fully integrated in 1984
 * already, when ksh still called an external editor command.
 */
const struct sysnod commands[] =
{
	{ ".",		SYSDOT,		BLT_SPC	},	/* S - */
	{ ":",		SYSNULL,	BLT_SPC	},	/* S - */
#ifdef DO_SYSATEXPR
	{ "@",		SYSEXPR,	0	},
#endif
#ifndef RES
	{ "[",		SYSTST,		0 },		/*  i  */
#endif
#ifdef	DO_SYSALIAS
	{ "alias",	SYSALIAS,	BLT_INT },	/*  I  */
#endif
#ifdef	DO_SYSALLOC
	{ "alloc",	SYSALLOC,	0 },
#endif
	{ "bg",		SYSFGBG,	BLT_INT },	/*  I  */
	{ "break",	SYSBREAK,	BLT_SPC },	/* S - */
#ifdef	DO_SYSBUILTIN
	{ "builtin",	SYSBUILTIN },
#endif
	{ "cd",		SYSCD,		BLT_INT },	/*  I  */
	{ "chdir",	SYSCD,		0 },
#ifdef	DO_SYSCOMMAND
	{ "command",	SYSCOMMAND,	BLT_INT },	/*  I  */
#endif
	{ "continue",	SYSCONT,	BLT_SPC	},	/* S - */
#ifdef	DO_SYSPUSHD
	{ "dirs",	SYSDIRS,	0 },
#endif
#ifdef	DO_SYSDOSH
	{ "dosh",	SYSDOSH,	0 },
#endif
	{ "echo",	SYSECHO,	0 },		/*  i  */
#ifdef	DO_SYSERRSTR
	{ "errstr",	SYSERRSTR,	0 },
#endif
	{ "eval",	SYSEVAL,	BLT_SPC	},	/* S - */
	{ "exec",	SYSEXEC,	BLT_SPC	},	/* S - */
	{ "exit",	SYSEXIT,	BLT_SPC	},	/* S - */
	{ "export",	SYSXPORT,	BLT_SPC },	/* S U */
#ifdef	DO_SYSTRUE
	{ "false",	SYSFALSE,	0 },		/*  i  */
#endif
	{ "fg",		SYSFGBG,	BLT_INT },	/*  I  */
#ifdef	DO_SYSFIND
	{ "find",	SYSFIND },
#endif
	{ "getopts",	SYSGETOPT,	BLT_INT },	/*  I  */
	{ "hash",	SYSHASH,	BLT_INT },	/*  I  */
#ifdef	INTERACTIVE
	{ "history",	SYSHISTORY,	0 },
#endif
	{ "jobs",	SYSJOBS,	BLT_INT },	/*  I  */
	{ "kill",	SYSKILL,	BLT_INT },	/*  I  */
#ifdef	DO_SYSKILLPG
	{ "killpg",	SYSKILL,	0 },
#endif
#ifdef	DO_SYSLOCAL
	{ "local",	SYSLOCAL,	0 },
#endif
#ifdef RES
	{ "login",	SYSLOGIN,	0 },
#endif
#ifdef	INTERACTIVE
	{ "map",	SYSMAP,		0 },
#endif
#ifdef RES
	{ "newgrp",	SYSLOGIN,	0 },
#else
	{ "newgrp",	SYSNEWGRP,	0 },		/*  i  */
#endif

#ifdef	DO_SYSPGRP
	{ "pgrp",	SYSPGRP,	0 },
#endif
#ifdef	DO_SYSPUSHD
	{ "popd",	SYSPOPD,	0 },
#endif
#ifdef	DO_SYSPRINTF
	{ "printf",	SYSPRINTF,	0 },		/*  i  */
#endif
#ifdef	DO_SYSPUSHD
	{ "pushd",	SYSPUSHD,	0 },
#endif
	{ "pwd",	SYSPWD,		0 },		/*  i  */
	{ "read",	SYSREAD,	BLT_INT },	/*  I  */
	{ "readonly",	SYSRDONLY,	BLT_SPC },	/* S U */
#ifdef	DO_SYSREPEAT
	{ "repeat",	SYSREPEAT,	0 },
#endif
	{ "return",	SYSRETURN,	BLT_SPC },	/* S - */
#ifdef	INTERACTIVE
	{ "savehistory", SYSSAVEHIST,	0 },
#endif
	{ "set",	SYSSET,		BLT_SPC	},	/* S U */
	{ "shift",	SYSSHFT,	BLT_SPC	},	/* S - */
	{ "stop",	SYSSTOP,	0 },
	{ "suspend",	SYSSUSP,	0 },
#ifdef	DO_SYSSYNC
	{ "sync",	SYSSYNC,	0 },
#endif
	{ "test",	SYSTST,		0 },		/*  i  */
	{ "times",	SYSTIMES,	BLT_SPC },	/* S - */
	{ "trap",	SYSTRAP,	BLT_SPC	},	/* S U */
#ifdef	DO_SYSTRUE
	{ "true",	SYSTRUE,	0 },		/*  i  */
#endif
	{ "type",	SYSTYPE,	BLT_INT },	/*  I  */


#ifndef RES
	{ "ulimit",	SYSULIMIT,	BLT_INT },	/*  I  */
	{ "umask",	SYSUMASK,	BLT_INT },	/*  I  */
#endif
#ifdef	DO_SYSALIAS
	{ "unalias",	SYSUNALIAS,	BLT_INT },	/*  I  */
#endif

	{ "unset",	SYSUNS,		BLT_SPC },	/* S U */
	{ "wait",	SYSWAIT,	BLT_INT }	/*  I  */
};

const int no_commands = sizeof (commands)/sizeof (struct sysnod);
