/* @(#)str.h	1.13 05/05/08 Copyright 1986-2005 J. Schilling */
/*
 *	Copyright (c) 1986-2005 J. Schilling
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

extern	char	relmsg[];

extern	char	bshopts[];

extern	char	sysinitname[];
extern	char	sysrinitname[];
extern	char	initname[];
extern	char	init2name[];
extern	char	finalname[];
extern	char	globalname[];
extern	char	localname[];
extern	char	aliasname[];
extern	char	laliasname[];
extern	char	historyname[];
extern	char	mapname[];
extern	char	defpath[];
extern	char	tmpname[];
extern	char	nulldev[];
extern	char	loginname[];

/*
 *	Conditions
 */
extern	char	sn_ctlc[];
extern	char	sn_no_mem[];
extern	char	sn_any_other[];
extern	char	sn_badtab[];
extern	char	sn_badfile[];

/*
 *	Open Modes
 */
extern	char	for_read[];
extern	char	for_ru[];
extern	char	for_rwct[];
extern	char	for_wct[];
extern	char	for_wca[];

/*
 *	Environment
 */
extern	char	homename[];
extern	char	pathname[];
extern	char	termname[];
extern	char	termcapname[];
extern	char	cwdname[];
extern	char	cdpathname[];
extern	char	username[];
extern	char	Elogname[];
extern	char	promptname[];
extern	char	prompt2name[];
extern	char	ignoreeofname[];
extern	char	histname[];
extern	char	savehistname[];
extern	char	slashname[];
extern	char	evlockname[];
extern	char	mailname[];
extern	char	mchkname[];
extern	char	mailpname[];

extern	char	nullstr[];
extern	char	nl[];
extern	char	slash[];
extern	char	eql[];
extern	char	lpar[];
extern	char	rpar[];
extern	char	on[];
extern	char	off[];
extern	char	commandname[];
extern	char	pipename[];

/*
 *	Parser
 */
extern	char	special[];
extern	char	ops[];
extern	char	spaces[];

/*
 *	Error Messages
 */
extern	char	ecantcreate[];
extern	char	ecantopen[];
extern	char	ecantread[];
extern	char	ecantexecute[];
extern	char	ecantfork[];
extern	char	ecantvfork[];
extern	char	enochildren[];
extern	char	eargtoolong[];
extern	char	erestricted[];
extern	char	ebadopt[];
extern	char	eioredef[];
extern	char	eiounimpl[];
extern	char	epiperedefio[];
extern	char	emissing[];
extern	char	emissiodelim[];
extern	char	emissnameinio[];
extern	char	emissterm[];
extern	char	emisscondcmd[];
extern	char	emisspipecmd[];
extern	char	emissabbrev[];
extern	char	enocmd[];
extern	char	enullcmd[];
extern	char	eambiguous[];
extern	char	ebadmodifier[];
extern	char	ebadpattern[];
extern	char	enotfound[];
extern	char	eshonly[];
extern	char	ecore[];
#ifdef	TEST			/* include test code */
extern	char	ebadop[];
extern	char	expected[];
extern	char	unexpected[];
extern	char	number[];
extern	char	argument[];
#endif



/*
 *	Usage Messages
 */
extern	char	helpname[];
extern	char	usage[];
extern	char	ubsh[];
extern	char	uexpr[];
extern	char	ubrack[];
extern	char	ucd[];
extern	char	uconcat[];
extern	char	udo[];
extern	char	uecho[];
extern	char	uenv[];
extern	char	uerrstr[];
extern	char	ueval[];
extern	char	uexec[];
extern	char	uexit[];
extern	char	ufunc[];
extern	char	uglob[];
extern	char	ukill[];
extern	char	uulimit[];
extern	char	ulogin[];
extern	char	umap[];
extern	char	upgrp[];
extern	char	upopd[];
extern	char	upushd[];
extern	char	uread[];
extern	char	urepeat[];
extern	char	uresume[];
extern	char	uset[];
extern	char	usetenv[];
extern	char	usetmask[];
extern	char	ushift[];
extern	char	usignal[];
extern	char	usource[];
extern	char	udot[];
extern	char	ustop[];
#ifdef	DO_SUID
extern	char	usuid[];
#endif
extern	char	ususpend[];
extern	char	utest[];
extern	char	utype[];
extern	char	uutime[];
extern	char	uumask[];
extern	char	uunmap[];
extern	char	uunset[];
extern	char	uunsetenv[];
extern	char	uwait[];
