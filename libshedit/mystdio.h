/* @(#)mystdio.h	1.15 16/03/30 Copyright 2006-2016 J. Schilling */
/*
 *	Defines to make FILE * -> int *, used to allow
 *	the Bourne shell to use functions that expect stdio.
 *
 *	Copyright (c) 2006-2016 J. Schilling
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

#ifndef	NO_SCHILY_STDIO_H
#ifndef	_MYSTDIO_H
#define	_MYSTDIO_H

/*
 * First include the official stdio.h to make sure that no other system include
 * file will pull it in, given that the platform is recent enough to protect
 * stdio.h from multiple #includes.
 */
#ifndef _INCL_STDIO_H
#include <stdio.h>
#define _INCL_STDIO_H
#endif

/*
 * Now make the definitions from the platforms system stdio.h vanish and
 * replace them by what we need in order to replace "FILE *" by "int *".
 */

#define	_FILE_DEFINED	/* Prevent MSC from redefining FILE in wchar.h */

#undef	FILE
#define	FILE	int
#ifdef	linux
#undef	__FILE
#define	__FILE	int	/* Needed for Linux */
#endif
#define	EOF	(-1)

#undef	fflush
#define	fflush(a)	(0)
#undef	clearerr
#define	clearerr(a)
#undef	ferror
#define	ferror(a)	(0)

/*
 * shell_* names are internal
 */
#define	printf		shell_printf
#define	fprintf		shell_fprintf
#define	sprintf		shell_sprintf
#define	snprintf	shell_snprintf
#define	error		shell_error

#define	js_printf	shell_printf
#define	js_fprintf	shell_fprintf
#define	js_sprintf	shell_sprintf
#define	js_snprintf	shell_snprintf

extern	int	__in__;
extern	int	__out__;
extern	int	__err__;

#undef	stdin
#define	stdin		(&__in__)
#undef	stdout
#define	stdout		(&__out__)
#undef	stderr
#define	stderr		(&__err__)

#if	defined(HAVE_LARGEFILES)
#define	fileopen64	shell_fileopen64
#else
#define	fileopen	shell_fileopen
#endif
#define	fclose		shell_fclose

#undef	getc
#define	getc		shell_getc
#define	fgetc		shell_fgetc

#undef	putc
#define	putc		shell_putc

#define	fileread	shell_fileread
#define	filewrite	shell_filewrite

#define	fdown(f)	(*(f))


extern	int	fclose	__PR((FILE *f));
extern	int	getc	__PR((FILE *f));
extern	int	fgetc	__PR((FILE *f));
extern	int	putc	__PR((int c, FILE *f));

/*
 * shedit_* names are external symbols.
 * These are now only functions (no variables) to allow to use lazy
 * linking and to stay within the features of the limited Mac OS X linker.
 */
#define	chghistory	shedit_chghistory
#define	remap		shedit_remap
#define	list_map	shedit_list_map
#define	add_map		shedit_add_map
#define	del_map		shedit_del_map
#define	append_line	shedit_append_line

/*
 * dat.c
 * These variables previously have been exported but are no longer exported
 * in order to support lazy linking and the limited Mac OS X linker.
 * These internal valaues are now set up from the main program via
 * parameters to shedit_setprompts().
 */
#define	prompts		shell_prompts	/* char *prompts[2] for the editor */
#define	prompt		shell_prompt	/* int prompt index for prompts	   */
#define	delim		shell_delim	/* int delim			   */

#define	ctlc		shell_ctlc
#define	evarray		shell_evarray
#define	ex_status	shell_ex_status
#define	gstd		shell_gstd
#define	inithome	shell_inithome	/* ??? immer mit getenv("HOME") ??? */
#define	prflg		shell_prflg
#define	ttyflg		shell_ttyflg

/*
 * comerr.c
 */
#define	_comerr		shell__comerr
#define	comerr		shell_comerr
#define	comerrno	shell_comerrno
#define	comexit		shell_comexit
#define	errmsg		shell_errmsg
#define	errmsgno	shell_errmsgno
#define	errmsgstr	shell_errmsgstr
#define	on_comerr	shell_on_comerr
#define	xcomerr		shell_xcomerr
#define	xcomerrno	shell_xcomerrno

/*
 * edit.c
 */
#define	berror		shell_berror
#define	errstr		shell_errstr
#define	ev_eql		shell_ev_eql
#define	exitbsh		shell_exitbsh	/* Need a call into the Bourne Shell */
#define	is_dir		shell_is_dir
#define	myhome		shell_myhome	/* XXX See inithome */
#define	pushline	shell_pushline
#define	setinput	shell_setinput
#define	toint		shell_toint
/*
 * The local getenv() putenv() routines that call functions from the
 * main program via function pointers set up via
 * shedit_getenv()/shedit_putenv() from the main program.
 */
#define	ev_insert	shell_putenv
#define	getcurenv	shell_getenv

/*
 * error.c
 */
#define	js_error	shell_error

/*
 * expand.c
 */
#define	any_match	shell_any_match
#define	expand		shell_expand

/*
 * fgetline.c
 */
#define	js_fgetline	shell_fgetline
#define	js_getline	shell_getline

/*
 * inputc.c
 */
#define	_nextwc		shell__nextwc
#define	append_wline	shell_append_wline
#define	get_histlen	shell_get_histlen
#define	get_line	shell_get_line
#define	getinfile	shell_getinfile
#define	getnextc	shell_getnextc
#define	init_input	shell_init_input
#define	make_line	shell_make_line
#define	makewstr	shell_makewstr
#define	match_hist	shell_match_hist
#define	nextc		shell_nextc
#define	put_history	shell_put_history
#define	read_init_history	shell_read_init_history
#define	readhistory	shell_readhistory
#define	save_history	shell_save_history
#define	space		shell_space

/*
 * map.c
 */
#define	gmap		shell_gmap
#define	map_init	shell_map_init
#define	mapflag		shell_mapflag
#define	mapgetc		shell_mapgetc
#define	maptab		shell_maptab
#define	mp_init		shell_mp_init
#define	rxmap		shell_rxmap

/*
 * node.c
 */
#define	_printio	shell__printio
#define	allocnode	shell_allocnode
#define	allocvec	shell_allocvec
#define	freevec		shell_freevec
#define	listlen		shell_listlen
#define	printio		shell_printio
#define	printstring	shell_printstring
#define	printtree	shell_printtree
#ifndef	freetree
#define	freetree	shell_freetree
#endif

/*
 * str.c
 */
#define	nullstr		shell_nullstr
#define	ebadpattern	shell_ebadpattern
#define	ecantopen	shell_ecantopen
#define	enotfound	shell_enotfound
#define	eql		shell_eql
#define	for_read	shell_for_read
#define	for_wct		shell_for_wct
#define	histname	shell_histname
#define	historyname	shell_historyname
#define	ignoreeofname	shell_ignoreeofname
#define	mapname		shell_mapname
#define	on		shell_on
#define	slash		shell_slash
#define	sn_no_mem	shell_sn_no_mem
#define	special		shell_special
#define	termcapname	shell_termcapname
#define	termname	shell_termname

/*
 * strsubs.c
 */
#define	concat		shell_concat
#define	concatv		shell_concatv
#define	fbasename	shell_fbasename
#define	makestr		shell_makestr
#define	pretty_string	shell_pretty_string
#define	quote_string	shell_quote_string
#define	strbeg		shell_strbeg
#define	streql		shell_streql
#define	streqln		shell_streqln
#define	strindex	shell_strindex
#define	wordeql		shell_wordeql

/*
 * ttymodes.c
 */
#define	get_tty_modes	shell_get_tty_modes
#define	i_should_echo	shell_i_should_echo
#define	ins_mode	shell_ins_mode
#define	reset_line_disc	shell_reset_line_disc
#define	reset_tty_modes	shell_reset_tty_modes
#define	reset_tty_pgrp	shell_reset_tty_pgrp
#define	set_append_modes	shell_set_append_modes
#define	set_insert_modes	shell_set_insert_modes
#define	tty_getpgrp	shell_tty_getpgrp
#define	tty_setpgrp	shell_tty_setpgrp

#endif /* _MYSTDIO_H */
#endif /* NO_SCHILY_STDIO_H */
