/* @(#)skel.c	1.14 08/01/16 Copyright 1987, 1995-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)skel.c	1.14 08/01/16 Copyright 1987, 1995-2007 J. Schilling";
#endif
/*
 *	Skeleton for the use of the scg genearal SCSI - driver
 *
 *	Copyright (c) 1987, 1995-2007 J. Schilling
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
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/fcntl.h>
#include <schily/time.h>
#include <schily/errno.h>
#include <signal.h>
#include <schily/schily.h>
#ifdef	HAVE_PRIV_H
#include <priv.h>
#endif

#if	defined(__CYGWIN32__) || defined(__EMX__) || defined(__DJGPP__)
#include <io.h>					/* for setmode() prototype */
#endif

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsi_scan.h"
#include "cdrecord.h"
#include "defaults.h"

char	skel_version[] = "1.1";

extern	BOOL	getlong		__PR((char *, long *, long, long));
extern	BOOL	getint		__PR((char *, int *, int, int));

struct exargs {
	SCSI	*scgp;
	int	old_secsize;
	int	flags;
	int	exflags;
	char	oerr[3];
} exargs;

LOCAL	void	usage		__PR((int ret));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	void	intr		__PR((int sig));
LOCAL	void	exscsi		__PR((int excode, void *arg));
LOCAL	void	excdr		__PR((int excode, void *arg));
LOCAL	int	prstats		__PR((void));
LOCAL	int	prstats_silent	__PR((void));
LOCAL	void	doit		__PR((SCSI *scgp));
LOCAL	void	dofile		__PR((SCSI *scgp, char *filename));

struct timeval	starttime;
struct timeval	stoptime;
int	didintr;
int	exsig;

char	*Sbuf;
long	Sbufsize = -1L;

int	help;
int	xdebug;
int	lverbose;
int	quiet;
BOOL	is_suid;

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\tbtcflash [options] f=firmwarefile\n");
	error("options:\n");
	error("\t-version	print version information and exit\n");
	error("\tdev=target	SCSI target to use\n");
	error("\tf=filename	Name of firmware file to read from\n");
	error("\tts=#		set maximum transfer size for a single SCSI command\n");
	error("\ttimeout=#	set the default SCSI command timeout to #.\n");
	error("\tdebug=#,-d	Set to # or increment misc debug level\n");
	error("\tkdebug=#,kd=#	do Kernel debugging\n");
	error("\t-quiet,-q	be more quiet in error retry mode\n");
	error("\t-verbose,-v	increment general verbose level by one\n");
	error("\t-Verbose,-V	increment SCSI command transport verbose level by one\n");
	error("\t-silent,-s	do not print status of failed SCSI commands\n");
	error("\t-scanbus	scan the SCSI bus and exit\n");
	exit(ret);
}

/* CSTYLED */
char	opts[]   = "debug#,d+,kdebug#,kd#,timeout#,quiet,q,verbose+,v+,Verbose+,V+,x+,xd#,silent,s,help,h,version,scanbus,dev*,ts&,f*";

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	char	*dev = NULL;
	int	fcount;
	int	cac;
	char	* const *cav;
	int	scsibus	= -1;
	int	target	= -1;
	int	lun	= -1;
	int	silent	= 0;
	int	verbose	= 0;
	int	kdebug	= 0;
	int	debug	= 0;
	int	deftimeout = 40;
	int	pversion = 0;
	int	scanbus = 0;
	SCSI	*scgp;
	char	*filename = NULL;
	int	err;

	save_args(ac, av);

	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, opts,
			&debug, &debug,
			&kdebug, &kdebug,
			&deftimeout,
			&quiet, &quiet,
			&lverbose, &lverbose,
			&verbose, &verbose,
			&xdebug, &xdebug,
			&silent, &silent,
			&help, &help, &pversion,
			&scanbus,
			&dev,
			getnum, &Sbufsize,
			&filename) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (pversion) {
		printf("btcflash %s (%s-%s-%s) Copyright (C) 1987, 1995-2007 Jörg Schilling (C) 2004 David Huang\n",
								skel_version,
								HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	fcount = 0;
	cac = ac;
	cav = av;

	while (getfiles(&cac, &cav, opts) > 0) {
		fcount++;
		if (fcount == 1) {
			if (*astoi(cav[0], &target) != '\0') {
				errmsgno(EX_BAD,
					"Target '%s' is not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
		}
		if (fcount == 2) {
			if (*astoi(cav[0], &lun) != '\0') {
				errmsgno(EX_BAD,
					"Lun is '%s' not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
		}
		if (fcount == 3) {
			if (*astoi(cav[0], &scsibus) != '\0') {
				errmsgno(EX_BAD,
					"Scsibus is '%s' not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
		} else {
			scsibus = 0;
		}
		cac--;
		cav++;
	}
/*error("dev: '%s'\n", dev);*/

	cdr_defaults(&dev, NULL, NULL, &Sbufsize, NULL);
	if (debug) {
		printf("dev: '%s'\n", dev);
	}
	if (!scanbus && dev == NULL &&
	    scsibus == -1 && (target == -1 || lun == -1)) {
		errmsgno(EX_BAD, "No SCSI device specified.\n");
		usage(EX_BAD);
	}
	if (dev || scanbus) {
		char	errstr[80];

		/*
		 * Call scg_remote() to force loading the remote SCSI transport
		 * library code that is located in librscg instead of the dummy
		 * remote routines that are located inside libscg.
		 */
		scg_remote();
		if (dev != NULL &&
		    ((strncmp(dev, "HELP", 4) == 0) ||
		    (strncmp(dev, "help", 4) == 0))) {
			scg_help(stderr);
			exit(0);
		}
		if ((scgp = scg_open(dev, errstr, sizeof (errstr), debug, lverbose)) == (SCSI *)0) {
			int	err = geterrno();

			errmsgno(err, "%s%sCannot open SCSI driver.\n", errstr, errstr[0]?". ":"");
			errmsgno(EX_BAD, "For possible targets try 'scgskeleton -scanbus'. Make sure you are root.\n");
			errmsgno(EX_BAD, "For possible transport specifiers try 'scgskeleton dev=help'.\n");
			exit(err);
		}
	} else {
		if (scsibus == -1 && target >= 0 && lun >= 0)
			scsibus = 0;

		scgp = scg_smalloc();
		scgp->debug = debug;
		scgp->kdebug = kdebug;

		scg_settarget(scgp, scsibus, target, lun);
		if (scg__open(scgp, NULL) <= 0)
			comerr("Cannot open SCSI driver.\n");
	}
	scgp->silent = silent;
	scgp->verbose = verbose;
	scgp->debug = debug;
	scgp->kdebug = kdebug;
	scg_settimeout(scgp, deftimeout);

	if (Sbufsize < 0)
		Sbufsize = 256*1024L;
	Sbufsize = scg_bufsize(scgp, Sbufsize);
	if ((Sbuf = scg_getbuf(scgp, Sbufsize)) == NULL)
		comerr("Cannot get SCSI I/O buffer.\n");

#ifdef	HAVE_PRIV_SET
	/*
	 * Give up privs we do not need anymore.
	 * We no longer need:
	 *	file_dac_read,net_privaddr
	 * We still need:
	 *	sys_devices
	 */
	priv_set(PRIV_OFF, PRIV_EFFECTIVE,
		PRIV_FILE_DAC_READ, PRIV_NET_PRIVADDR, NULL);
	priv_set(PRIV_OFF, PRIV_PERMITTED,
		PRIV_FILE_DAC_READ, PRIV_NET_PRIVADDR, NULL);
	priv_set(PRIV_OFF, PRIV_INHERITABLE,
		PRIV_FILE_DAC_READ, PRIV_NET_PRIVADDR, PRIV_SYS_DEVICES, NULL);
#endif
	/*
	 * This is only for OS that do not support fine grained privs.
	 */
	is_suid = geteuid() != getuid();
	/*
	 * We don't need root privilleges anymore.
	 */
#ifdef	HAVE_SETREUID
	if (setreuid(-1, getuid()) < 0)
#else
#ifdef	HAVE_SETEUID
	if (seteuid(getuid()) < 0)
#else
	if (setuid(getuid()) < 0)
#endif
#endif
		comerr("Panic cannot set back effective uid.\n");

	/* code to use SCG */

	if (scanbus) {
		if (select_target(scgp, stdout) < 0)
			exit(EX_BAD);
		exit(0);
	}
	seterrno(0);
	do_inquiry(scgp, FALSE);
	err = geterrno();
	if (err == EPERM || err == EACCES)
		exit(EX_BAD);
	allow_atapi(scgp, TRUE);    /* Try to switch to 10 byte mode cmds */

	exargs.scgp	   = scgp;
	exargs.old_secsize = -1;
/*	exargs.flags	   = flags;*/
	exargs.oerr[2]	   = 0;

	/*
	 * Install exit handler before we change the drive status.
	 */
	on_comerr(exscsi, &exargs);
	signal(SIGINT, intr);
	signal(SIGTERM, intr);

	if (filename) {
		dofile(scgp, filename);
	} else {
		errmsgno(EX_BAD, "Firmware file missing.\n");
		doit(scgp);
	}

	comexit(0);
	return (0);
}

/*
 * XXX Leider kann man vim Signalhandler keine SCSI Kommandos verschicken
 * XXX da meistens das letzte SCSI Kommando noch laeuft.
 * XXX Eine Loesung waere ein Abort Callback in SCSI *.
 */
LOCAL void
intr(sig)
	int	sig;
{
	didintr++;
	exsig = sig;
/*	comexit(sig);*/
}

/* ARGSUSED */
LOCAL void
exscsi(excode, arg)
	int	excode;
	void	*arg;
{
	struct exargs	*exp = (struct exargs *)arg;
		int	i;

	/*
	 * Try to restore the old sector size.
	 */
	if (exp != NULL && exp->exflags == 0) {
		for (i = 0; i < 10*100; i++) {
			if (!exp->scgp->running)
				break;
			if (i == 10) {
				errmsgno(EX_BAD,
					"Waiting for current SCSI command to finish.\n");
			}
			usleep(100000);
		}

#ifdef	___NEEDED___
		/*
		 * Try to set drive back to original state.
		 */
		if (!exp->scgp->running) {
			if (exp->oerr[2] != 0) {
				domode(exp->scgp, exp->oerr[0], exp->oerr[1]);
			}
			if (exp->old_secsize > 0 && exp->old_secsize != 2048)
				select_secsize(exp->scgp, exp->old_secsize);
		}
#endif
		exp->exflags++;	/* Make sure that it only get called once */
	}
}

LOCAL void
excdr(excode, arg)
	int	excode;
	void	*arg;
{
	exscsi(excode, arg);

#ifdef	needed
	/* Do several other restores/statistics here (see cdrecord.c) */
#endif
}

/*
 * Return milliseconds since start time.
 */
LOCAL int
prstats()
{
	int	sec;
	int	usec;
	int	tmsec;

	if (gettimeofday(&stoptime, (struct timezone *)0) < 0)
		comerr("Cannot get time\n");

	sec = stoptime.tv_sec - starttime.tv_sec;
	usec = stoptime.tv_usec - starttime.tv_usec;
	tmsec = sec*1000 + usec/1000;
#ifdef	lint
	tmsec = tmsec;	/* Bisz spaeter */
#endif
	if (usec < 0) {
		sec--;
		usec += 1000000;
	}

	error("Time total: %d.%03dsec\n", sec, usec/1000);
	return (1000*sec + (usec / 1000));
}

/*
 * Return milliseconds since start time, but be silent this time.
 */
LOCAL int
prstats_silent()
{
	int	sec;
	int	usec;
	int	tmsec;

	if (gettimeofday(&stoptime, (struct timezone *)0) < 0)
		comerr("Cannot get time\n");

	sec = stoptime.tv_sec - starttime.tv_sec;
	usec = stoptime.tv_usec - starttime.tv_usec;
	tmsec = sec*1000 + usec/1000;
#ifdef	lint
	tmsec = tmsec;	/* Bisz spaeter */
#endif
	if (usec < 0) {
		sec--;
		usec += 1000000;
	}

	return (1000*sec + (usec / 1000));
}

LOCAL void
doit(scgp)
	SCSI	*scgp;
{
	int	i = 0;

	for (;;) {
		if (!wait_unit_ready(scgp, 60))
			comerrno(EX_BAD, "Device not ready.\n");

		printf("0:read\n");
/*		printf("7:wne  8:floppy 9:verify 10:checkcmds  11:read disk 12:write disk\n");*/

		getint("Enter selection:", &i, 0, 20);
		if (didintr)
			return;

		switch (i) {

/*		case 1:		read_disk(scgp, 0);	break;*/

		default:	error("Unimplemented selection %d\n", i);
		}
	}
}

LOCAL int btcmain		__PR((SCSI *scgp, const char *fwfile));

LOCAL void
dofile(scgp, filename)
	SCSI	*scgp;
	char	*filename;
{
	if (btcmain(scgp, filename) != 0)
		exit(EX_BAD);
}

/*
 * Add your own code below....
 */

#include "btcflash.c"
