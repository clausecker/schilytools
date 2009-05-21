/* @(#)str.c	1.21 09/05/17 Copyright 1986-2009n J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)str.c	1.21 09/05/17 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-2009 J. Schilling
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
#include <signal.h>
#include "bsh.h"

char	relmsg[]	= "\
\
bsh V%d.%02d (%s-%s-%s)\n\n\
	By J. Schilling\n\
	bsh -> best shell\n";

char	bshopts[]	= "v,V,i,c,e,h,2,g,l,n,s,t,f,F,o,q,help,version";

char	sysinitname[]	= "/etc/initbsh";
char	sysrinitname[]	= "/etc/initrbsh";
char	initname[]	= ".init";
char	init2name[] 	= ".init2";
char	finalname[]	= ".final";
#ifdef	NEW
char	globalname[]	= ".alias";
char	localname[]	= ".lalias";
#else
char	globalname[]	= ".globals";
char	localname[]	= ".locals";
#endif
char	aliasname[]	= ".alias";
char	laliasname[]	= ".lalias";
char	historyname[]	= ".history";
char	mapname[]	= ".bshmap";
char	defpath[]	= ":/usr/ucb:/bin:/usr/bin";
char	tmpname[]	= "/tmp/bsh";
char	nulldev[]	= "/dev/null";
char	loginname[]	= "login";

/*
 *	Conditions
 */
char	sn_ctlc[]	= "kill";
char	sn_no_mem[]	= "no_memory";
char	sn_any_other[]	= "any_other";
char	sn_badtab[]	= "bad_astab_number";
char	sn_badfile[]	= "bad_sym_file";

/*
 *	Open Modes
 */
char	for_read[]	= "r";
char	for_ru[]	= "ru";
char	for_rwct[]	= "rwct";
char	for_wct[]	= "wct";
char	for_wca[]	= "wca";

/*
 *	Environment
 */
char	homename[]	= "HOME";
char	pathname[]	= "PATH";
char	termname[]	= "TERM";
char	termcapname[]	= "TERMCAP";
char	cwdname[]	= "CWD";
char	cdpathname[]	= "CDPATH";
char	username[]	= "USER";
char	Elogname[]	= "LOGNAME";
char	promptname[]	= "PROMPT";
char	prompt2name[]	= "PROMPT2";
char	ignoreeofname[]	= "IGNOREEOF";
char	histname[]	= "HISTORY";
char	savehistname[]	= "SAVEHISTORY";
char	slashname[]	= "SLASH";
char	evlockname[]	= "EVLOCK";
char	mailname[]	= "MAIL";
char	mchkname[]	= "MAILCHECK";
char	mailpname[]	= "MAILPATH";

char	nullstr[]	= "";
char	nl[]		= "\n";
char	slash[]		= "/";
char	eql[]		= "=";
char	lpar[]		= "(";
char	rpar[]		= ")";
char	on[]		= "on";
char	off[]		= "off";
char	commandname[]	= "command";
char	pipename[]	= "pipe";

/*
 *	Parser
 */
char	special[]	= ";&|()<>% \t";
char	ops[]		= ";&|)";
char	spaces[]	= " \t\n";

/*
 *	Error Messages
 */
char	ecantcreate[]	= "Can't create '%s'. %s";
char	ecantopen[]	= "Can't open '%s'. %s";
char	ecantread[]	= "Can't read '%s'. %s";
char	ecantexecute[]	= "Can't execute '%s'. %s";
char	ecantfork[]	= "Can't fork %s";
char	ecantvfork[]	= "Can't vfork %s";
char	enochildren[]	= "No children.";
char	eargtoolong[]	= "Argument too long.";
char	erestricted[]	= "%s: restricted.";
char	ebadopt[]	= "%s: Bad Option: %s.";
char	eioredef[]	= "I/O redefined.";
char	eiounimpl[]	= "Unimplemented I/O.";
char	epiperedefio[]	= "Pipe redefines I/O.";
char	emissing[]	= "Missing";
char	emissiodelim[]	= "Missing I/O delimiter.";
char	emissnameinio[]	= "Missing I/O name.";
char	emissterm[]	= "Missing I/O terminator.";
char	emisscondcmd[]	= "Missing command in conditional.";
char	emisspipecmd[]	= "Missing command in pipe.";
char	emissabbrev[]	= "Missing abbrev name.";
char	enocmd[]	= "Unknown command.";
char	enullcmd[]	= "Null command name.";
char	eambiguous[]	= "Ambiguous.";
char	ebadmodifier[]	= "Bad modifier.";
char	ebadpattern[]	= "Bad pattern.";
char	enotfound[]	= "Not found.";
char	eshonly[]	= "#! only works in commandfiles.";
char	ecore[]		= " - core dumped";
#ifdef	TEST			/* include test code */
char	ebadop[]	= "bad operator: %s";
char	expected[]	= "%s expected";
char	unexpected[]	= "%s unexpected";
char	number[]	= "number";
char	argument[]	= "argument";
#endif


/*
 *	Usage Messages
 */
char	helpname[]	= "-help";
char	usage[]		= "Usage: ";
char	ubsh[]		= "\
[options] [arg1 ... argn]\n\
	Read commands from file arg1. If no args,\n\
	Read commands from stdin.\n\
Options:\n\
	-i	Force interactive prompting.\n\
	-v	Set verbose mode after processing dotfiles.\n\
	-V	Start with verbose mode on.\n\
	-c	Execute arg1 as a command with arg2-n as args.\n\
	-e	Exit immediately if a noninteractive command fails.\n\
	-n	Read commands but do not execute them.\n\
	-s	Read commands from stdin - even if args.\n\
	-t	Exit after reading and executing one command.\n\
	-2	Don't read ~/.init2 file.\n\
		also true if -c set and name is 'command'.\n\
	-h	Don't use ~/.history file.\n\
	-g	Don't use ~/.globals file.\n\
	-l	Don't use .locals file.\n\
	-f	(fast) same as -2h.\n\
	-F	(extra fast) same as -2hgl.\n\
	-o	Don't close nostd files on exec.";

char	ualias[]	= "[name[=value]...]\n\
Options:\n\
	-l	Use local aliases.\n\
	-reload	Reload aliases from file.";
char	uunalias[]	= "[name...]\n\
Options:\n\
	-l	Use local aliases.";
char	uexpr[]		= "name = expr";
char	ufg[]		= "[job ...]";
char	ubrack[]	= "expr ]";
char	ucd[]		= "[directory]";
char	uconcat[]	= "name val1 ... valn";
char	udo[]		= "command";
char	uecho[]		= "[-n|-nnl] args";
char	uenv[]		= "[-i] [name=value] [command [args]]";
char	uerrstr[]	= "errno";
char	ueval[]		= "[args]";
char	uexec[]		= "[av0=name] [args]";
char	uexit[]		= "[exitcode]";
char	ufunc[]		= "[funcname] ['cmdlist']";
char	uglob[]		= "args";
char	ukill[]		= "[-l][-sig] pid1 ... pidn";
char	uulimit[]	= "[[resource] [[curlimit] [maxlimit]]]";
char	ulogin[]	= "[username]";
char	umap[]		= "[fromstr tostr [comment]]";
char	upgrp[]		= "[pid]";
char	upopd[]		= "[offsdet]";
char	upushd[]	= "[directory|offset]";
char	uread[]		= "varname";
char	urepeat[]	= "\
[delay=seconds] [count=times] command\n\
	All flags must be before the command to be repeated.";
char	uresume[]	= "pid";
char	uset[]		= "[name=value]";
char	usetenv[]	= "[name value]";
char	usetmask[]	= "\
[+-]{rwx}  [+-]{rwx}  [+-]{rwx}\n\
	\t   Owner      Group      World\n\n\
	Accepted mode characters are also :\n\
		=   same mode as before\n\
		.   delete all permissions";
char	ushift[]	= "[count]";
char	usignal[]	= "['cmdlist'] [sig#1 .. sig#n]";
char	usource[]	= "[-h] bshfile";
char	udot[]		= "shellfile";
char	ureturn[]	= "[retval]";
char	ustop[]		= "pid1 ... pidn";
#ifdef	DO_SUID
char	usuid[]		= "\
[name]\n\
	The user will be prompted for password";
#endif
char	ususpend[]	= "[pid1 ... pidn]";
char	utest[]		= "expr";
char	utype[]		= "name ...";
char	uutime[]	= "noch nicht fertig";
char	uumask[]	= "[-S] [mask]";
char	uunmap[]	= "fromstr";
char	uunset[]	= "name";
char	uunsetenv[]	= "name";
char	uwait[]		= "pid1 ... pidn";
