/* @(#)idops.c	1.38 09/07/14 Copyright 1985-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)idops.c	1.38 09/07/14 Copyright 1985-2009 J. Schilling";
#endif
/*
 *	uid und gid Routinen
 *
 *	Copyright (c) 1985-2009 J. Schilling
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
#ifndef	__CYGWIN32__
#include <schily/stdio.h>
#include <schily/signal.h>
#include <schily/pwd.h>
#include <schily/shadow.h>
#	define	SUNAME	"root"
#include "bsh.h"
#include "str.h"
#include "abbrev.h"
#include "strsubs.h"
#include <schily/string.h>
#ifdef	HAVE_SYSLOG_H
#include <syslog.h>
#endif
#include <schily/unistd.h>
#include <schily/stdlib.h>

#define	UGROUP		0
#define	UPASSWD		1
#define	UGID		2
#define	UISIZE		4
#define	UNAME		0

extern BOOL 	firstsh;
extern BOOL	vflg;

extern	char	*crypt		__PR((const char *key, const char *salt));

#ifdef	DO_SUID
EXPORT	void	bsuid		__PR((Argvec * vp, FILE ** std, int flag));
#endif
LOCAL	void	dosyslog	__PR((char *type));
LOCAL	int	readpw		__PR((FILE ** std, char *passwd, int size));


#ifdef	DO_SUID

/* ARGSUSED */
EXPORT void
bsuid(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	char		passwd[100];
	struct passwd	*pw;
#if	defined(HAVE_GETSPNAM) || defined(HAVE_GETSPNAM)
#ifdef	HAVE_GETSPNAM
	struct spwd	*sp;
#else
	struct s_passwd	*sp;
#define	sp_pwdp		pw_passwd
#endif
#endif
	pid_t		child;
	char		*av0;
	char		*name;
	uid_t		new_uid;
	sigtype		o_sig2;
	sigtype		o_sig3;
	char		*init2;
	extern pid_t mypid;
	extern pid_t mypgrp;
	extern pid_t opgrp;

	if (ev_eql("SUID", off)) {
		fprintf(std[2], "suid: restricted.\n");
		ex_status = 1;
		return;
	}
	if (vp->av_ac > 2) {
		wrong_args(vp, std);
		return;
	}
	name = (vp->av_ac == 2) ? vp->av_av[1] : SUNAME;
#if	defined(HAVE_GETSPNAM) || defined(HAVE_GETSPNAM)
	seteuid(0);
	if ((pw = getpwnam(name)) == (struct passwd *)0 ||
#ifdef	HAVE_GETSPNAM
			(sp = getspnam(name)) == (struct spwd *)0) {
#else
			/*
			 * Note: getspwnam() (HP-UX) is in libsec.a
			 */
			(sp = getspwnam(name)) == (struct s_passwd *)0) {
#endif
		seteuid(getuid());
		fprintf(std[2], "Unknown login: %s\n", name);
		ex_status = 1;
		return;
	}
	seteuid(getuid());
#else
	if ((pw = getpwnam(name)) == (struct passwd *)0) {
		fprintf(std[2], "Unknown login: %s\n", name);
		ex_status = 1;
		return;
	}
#endif
#ifdef DEBUG
	printf("geteuid: %d, pw->pw_uid: %d.\n", geteuid(), pw->pw_uid);
#endif
	if (geteuid() == pw->pw_uid) {
		fprintf(std[2], "Already %s.\n", pw->pw_name);
		ex_status = 1;
		return;
	}
	new_uid = pw->pw_uid;
#if	defined(HAVE_GETSPNAM) || defined(HAVE_GETSPNAM)
	if (geteuid() && (strlen(sp->sp_pwdp) > (unsigned)0)) {
#else
	if (geteuid() && (strlen(pw->pw_passwd) > (unsigned)0)) {
#endif
/*		char buf[20];*/
		char *bp;

		if (readpw(std, passwd, sizeof (passwd)) < 0) {
			ex_status = 1;
			return;
		}
		fputc('\n', std[1]);
/*		bp = encrypt_password(passwd, buf);*/

#if	defined(HAVE_GETSPNAM) || defined(HAVE_GETSPNAM)
		bp = crypt(passwd, sp->sp_pwdp);
		if (!streql(bp, sp->sp_pwdp)) {
			seteuid(0);
#ifdef	HAVE_GETSPNAM
			if (!(sp = getspnam(SUNAME))) {
#else
			/*
			 * Note: getspwnam() (HP-UX) is in libsec.a
			 */
			if (!(sp = getspwnam(SUNAME))) {
#endif
				seteuid(getuid());
				ex_status = 1;
				return;
			}
			seteuid(getuid());
			bp = crypt(passwd, sp->sp_pwdp);
			if (*(sp->sp_pwdp) && !streql(bp, sp->sp_pwdp)) {
#else
		bp = crypt(passwd, pw->pw_passwd);
		if (!streql(bp, pw->pw_passwd)) {
			if (!(pw = getpwnam(SUNAME))) {
				ex_status = 1;
				return;
			}
			bp = crypt(passwd, pw->pw_passwd);
			if (*(pw->pw_passwd) && !streql(bp, pw->pw_passwd)) {
#endif /* HAVE_SHADOW_H */
				fprintf(std[2], "Sorry\n");
				dosyslog("BAD");
				ex_status = 1;
				return;
			}
		}
	}
	dosyslog("");
	o_sig2 = signal(SIGINT, (sigtype) SIG_IGN);
	o_sig3 = signal(SIGQUIT, (sigtype) SIG_IGN);
	signal(SIGINT, o_sig2);
	signal(SIGQUIT, o_sig3);
	if ((child = shfork(flag|NOSIG)) != 0) {
		if (child < 0) {
			ex_status = child;
			return;
		}
		ewait(child, WALL);
		return;
	}
	/*
	 * This is the child...
	 */
	if (!(flag & NOSIG)) {
		osig2 = o_sig2;
		osig3 = o_sig3;
		if (o_sig2 != (sigtype) SIG_IGN) {
			signal(SIGINT, o_sig2);
			osig2 = osig3 = (sigtype) SIG_DFL;
		}
	}
	signal(SIGTERM, (sigtype) SIG_IGN);
#ifdef	SIGTSTP
	signal(SIGTSTP, (sigtype) SIG_IGN);
#endif
#ifdef	SIGTTIN
	signal(SIGTTIN, (sigtype) SIG_IGN);
#endif
#ifdef	SIGTTOU
	signal(SIGTTOU, (sigtype) SIG_IGN);
#endif
	mypid = getpid();
	mypgrp = opgrp = getpgid(0);
	av0 = saved_av()[0];
	*av0 = '+';
	vflg = FALSE;
	ev_insert(concat(ignoreeofname, eql, off, (char *)NULL));
	init2 = concat(pw->pw_dir, slash, init2name, (char *)NULL);
	dofile(init2, GLOBAL_AB, NOTMS, std, FALSE);
	free(init2);
	ev_insert(concat(vp->av_ac == 2?"PROMPT=++":"PROMPT=",
						name, "> ", (char *)NULL));
#if	defined(HAVE_SETEUID)
	seteuid(0);
#else
#ifdef	HAVE_SETRESUID
	setresuid(-1, 0, -1);	/* HP-UX setresuid(ruid, euid, suid)*/
#else
#ifdef	HAVE_SETREUID		/* BSD & POSIX */
	setreuid(-1, 0);
#else
	/*
	 * Hier sollte nur dann eine Warnung/Abbruch kommen, wenn
	 * der bsh tatsächlich suid root installiert ist.
	 */
#if !defined(__EMX__) && !defined(__DJGPP__) && !defined(__BEOS__)
error  No function to set uid available
#endif

#endif
#endif
#endif
	if (vp->av_ac == 2)	/* mit su admin wird man real admin */
		setuid(new_uid);
}

#ifdef	SYSLOG
LOCAL void
dosyslog(type)
	char	*type;
{
	char	*uname;

	uname = getuname(geteuid());
	openlog("bsh", LOG_NULL);
	syslog(LOG_CRIT, "%sSU: %s %s", type, uname, ttyname(fdown(gstd[0])));
	closelog();
	free(uname);
}
#else
LOCAL void
dosyslog(type)
	char	*type;
{
#ifdef	DOSYSLOG
	extern	char	**evarray;
	extern	unsigned evasize;
	int	oeve = evaent;
	char	*s;
	char	*uname = getuname(geteuid());
	char	**oeva = evarray;
	unsigned oevs = evasize;

	evarray = (char **)NULL;
	evasize = 0;
	evaent = 0;
	ev_inc();
	s = concat("syslog -i bsh -CRIT ",
		type, "SU: ", uname, " ", ttyname(fdown(gstd[0])), (char *)NULL);
	pushline(s);
	freetree(cmdline(0, gstd, FALSE));
	free(s);
	free(evarray);
	evarray	 = oeva;
	evasize = oevs;
	evaent = oeve;
#else
	return;
#endif
}
#endif

LOCAL int
readpw(std, passwd, size)
	FILE	*std[];
	char	*passwd;
	int	size;
{
	int	code;

#ifdef	INTERACTIVE
	set_append_modes(std[0]);
	set_insert_modes(std[0]);
	fprintf(std[1], "Password:");
	code = fgetline(std[0], passwd, size);
/*XXX ???	reset_tty_modes(std[0]);*/
	reset_tty_modes();
#else
	char	*p;

	p = getpass("Passdord:");
	if (p == NULL)
		return (-1);
	strcpy(passwd, p);
	return (1);
#endif
#ifdef DEBUGX
	printf("Passwd: %s, retcode: %d, file: %d\n", passwd, code, f);
#endif
	return (code);
}

#endif

#endif	/* __CYGWIN32__ */
