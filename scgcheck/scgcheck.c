/* @(#)scgcheck.c	1.19 10/05/24 Copyright 1998-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scgcheck.c	1.19 10/05/24 Copyright 1998-2010 J. Schilling";
#endif
/*
 *	Copyright (c) 1998-2010 J. Schilling
 *
 * Warning: This program has been written to verify the correctness
 * of the upper layer interface from the library "libscg". If you
 * modify code from the program "scgcheck", you must change the
 * name of the program.
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

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/standard.h>

#include <schily/utypes.h>
#include <schily/btorder.h>
#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>
#include "scsi_scan.h"

#include "cdrecord.h"
#include "scgcheck.h"
#include "version.h"

LOCAL	void	usage		__PR((int ret));
EXPORT	int	main		__PR((int ac, char *av[]));
LOCAL	SCSI	*doopen		__PR((char *dev));
LOCAL	void	checkversion	__PR((SCSI *scgp));
LOCAL	void	getbuf		__PR((SCSI *scgp));
LOCAL	void	scg_openerr	__PR((char *errstr));
LOCAL	int	find_drive	__PR((SCSI *scgp, char *sdevname, int flags));
EXPORT	void	flushit		__PR((void));
EXPORT	int	countopen	__PR((void));
EXPORT	int	chkprint	__PR((const char *, ...)) __printflike__(1, 2);
EXPORT	int	chkgetline	__PR((char *, int));


	char	*dev;
int		debug;		/* print debug messages */
int		kdebug;		/* kernel debug messages */
int		scsi_verbose;	/* SCSI verbose flag */
int		lverbose;	/* local verbose flag */
int		silent;		/* SCSI silent flag */
int		deftimeout = 40; /* default SCSI timeout */
int		xdebug;		/* extended debug flag */
BOOL		autotest;

char	*buf;			/* The transfer buffer */
long	bufsize;		/* The size of the transfer buffer */

FILE	*logfile;
char	unavail[] = "<data unavaiable>";
char	scgc_version[] = VERSION;
int	basefds;

#define	BUF_SIZE	(126*1024)
#define	MAX_BUF_SIZE	(16*1024*1024)

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\tscgcheck [options]\n");
	error("Options:\n");
	error("\t-version	print version information and exit\n");
	error("\tdev=target	SCSI target to use\n");
	error("\ttimeout=#	set the default SCSI command timeout to #.\n");
	error("\tdebug=#,-d	Set to # or increment misc debug level\n");
	error("\tkdebug=#,kd=#	do Kernel debugging\n");
	error("\t-verbose,-v	increment general verbose level by one\n");
	error("\t-Verbose,-V	increment SCSI command transport verbose level by one\n");
	error("\t-silent,-s	do not print status of failed SCSI commands\n");
	error("\tf=filename	Name of file to write log data to.\n");
	error("\t-auto		try to do a fully automated test\n");
	error("\n");
	exit(ret);
}

char	opts[]   = "debug#,d+,kdebug#,kd#,timeout#,verbose+,v+,Verbose+,V+,silent,s,x+,xd#,help,h,version,dev*,f*,auto";

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	* const *cav;
	SCSI	*scgp = NULL;
	char	device[128];
	char	abuf[2];
	int	ret;
	int	fcount;
	BOOL	help = FALSE;
	BOOL	pversion = FALSE;
	char	*filename = "check.log";

	save_args(ac, av);

	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, opts,
			&debug, &debug,
			&kdebug, &kdebug,
			&deftimeout,
			&lverbose, &lverbose,
			&scsi_verbose, &scsi_verbose,
			&silent, &silent,
			&xdebug, &xdebug,
			&help, &help, &pversion,
			&dev,
			&filename, &autotest) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (pversion) {
		printf("scgcheck %s (%s-%s-%s) Copyright (C) 1998-2010 Jörg Schilling\n",
								scgc_version,
								HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	fcount = 0;
	cac = ac;
	cav = av;

	while (getfiles(&cac, &cav, opts) > 0) {
		fcount++;
		cac--;
		cav++;
	}
	if (fcount > 0)
		comerrno(EX_BAD, "Bad argument(s).\n");
/*error("dev: '%s'\n", dev);*/

	logfile = fileopen(filename, "wct");
	if (logfile == NULL)
		comerr("Cannot open logfile.\n");

	chkprint("Scgcheck %s (%s-%s-%s) SCSI user level transport library ABI checker.\n\
Copyright (C) 1998-2008 Jörg Schilling\n",
						scgc_version,
						HOST_CPU, HOST_VENDOR, HOST_OS);
	/*
	 * Call scg_remote() to force loading the remote SCSI transport library
	 * code that is located in librscg instead of the dummy remote routines
	 * that are located inside libscg.
	 */
	scg_remote();

	basefds = countopen();
	if (xdebug)
		error("nopen: %d\n", basefds);

	chkprint("**********> Checking whether your implementation supports to scan the SCSI bus.\n");
	chkprint("Trying to open device: '%s'.\n", dev);
	scgp = doopen(dev);

	if (xdebug) {
		error("nopen: %d\n", countopen());
		error("Scanopen opened %d new files.\n", countopen() - basefds);
	}

	device[0] = '\0';
	if (scgp == NULL) do {
		error("SCSI open failed...\n");
		if (!scg_yes("Retry with different device name? "))
			break;
		error("Enter SCSI device name for bus scanning [%s]: ", device);
		flushit();
		(void) getline(device, sizeof (device));
		if (device[0] == '\0')
			strcpy(device, "0,6,0");

		chkprint("Trying to open device: '%s'.\n", device);
		scgp = doopen(device);
	} while (scgp == NULL);
	if (scgp) {
		checkversion(scgp);
		getbuf(scgp);

		ret = select_target(scgp, stdout);
		select_target(scgp, logfile);
		if (ret < 1) {
			chkprint("----------> SCSI scan bus test: found NO TARGETS\n");
		} else {
			chkprint("----------> SCSI scan bus test PASSED\n");
		}
	} else {
		chkprint("----------> SCSI scan bus test FAILED\n");
	}

	if (xdebug)
		error("nopen: %d\n", countopen());
	chkprint("For the next test we need to open a single SCSI device.\n");
	chkprint("Best results will be obtained if you specify a modern CD-ROM drive.\n");

	if (scgp) {		/* Scanbus works / may work */
		int	i;

		i = find_drive(scgp, dev, 0);
		if (i < 0) {
			scg_openerr("");
			/* NOTREACHED */
		}
		snprintf(device, sizeof (device),
			"%s%s%d,%d,%d",
			dev?dev:"", dev?(dev[strlen(dev)-1] == ':'?"":":"):"",
			scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
		scg_close(scgp);
		scgp = NULL;
	}

	if (device[0] == '\0')
		strcpy(device, "0,6,0");
	do {
		char	Device[128];

		error("Enter SCSI device name [%s]: ", device);
		(void) chkgetline(Device, sizeof (Device));
		if (Device[0] != '\0')
			strcpy(device, Device);

		chkprint("Trying to open device: '%s'.\n", device);
		scgp = doopen(device);
		if (scgp) {
			checkversion(scgp);
			getbuf(scgp);
		}
		/*
		 * XXX hier muß getestet werden ob das Gerät brauchbar für die folgenden Tests ist.
		 */
	} while (scgp == NULL);
	if (xdebug)
		error("nopen: %d\n", countopen());
	/*
	 * First try to check which type of SCSI device we
	 * have.
	 */
	scgp->silent++;
	(void) unit_ready(scgp);	/* eat up unit attention */
	scgp->silent--;
	getdev(scgp, TRUE);
	printinq(scgp, logfile);

	printf("Ready to start test for second SCSI open? Enter <CR> to continue: ");
	(void) chkgetline(abuf, sizeof (abuf));
#define	CHECK_SECOND_OPEN
#ifdef	CHECK_SECOND_OPEN
	if (!streql(abuf, "n")) {
		SCSI	*scgp2 = NULL;
		int	oldopen = countopen();
		BOOL	second_ok = TRUE;

		scgp->silent++;
		ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
		scgp->silent--;
		if (xdebug)
			error("ret: %d key: %d\n", ret, scg_sense_key(scgp));
		if (ret >= 0 || scgp->scmd->error == SCG_RETRYABLE) {
			chkprint("First SCSI open OK - device usable\n");
			chkprint("**********> Checking for second SCSI open.\n");
			if ((scgp2 = doopen(device)) != NULL) {
				/*
				 * XXX Separates getbuf() fuer scgp2?
				 */
				chkprint("Second SCSI open for same device succeeded, %d additional file descriptor(s) used.\n",
					countopen() - oldopen);
				scgp->silent++;
				ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
				scgp->silent--;
				if (ret >= 0 || scgp->scmd->error == SCG_RETRYABLE) {
					chkprint("Second SCSI open is usable\n");
				}
				chkprint("Closing second SCSI.\n");
				scg_close(scgp2);
				scgp2 = NULL;
				chkprint("Checking first SCSI.\n");
				scgp->silent++;
				ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
				scgp->silent--;
				if (ret >= 0 || scgp->scmd->error == SCG_RETRYABLE) {
					second_ok = TRUE;
					chkprint("First SCSI open is still usable\n");
					chkprint("----------> Second SCSI open test PASSED.\n");
				} else if (ret < 0 && scgp->scmd->error == SCG_FATAL) {
					second_ok = FALSE;
					chkprint("First SCSI open does not work anymore.\n");
					chkprint("----------> Second SCSI open test FAILED.\n");
				} else {
					second_ok = FALSE;
					chkprint("First SCSI open has strange problems.\n");
					chkprint("----------> Second SCSI open test FAILED.\n");
				}
			} else {
				second_ok = FALSE;
				chkprint("Cannot open same SCSI device a second time.\n");
				chkprint("----------> Second SCSI open test FAILED.\n");
			}
		} else {
			second_ok = FALSE;
			chkprint("First SCSI open is not usable\n");
			chkprint("----------> Second SCSI open test FAILED.\n");
		}
		if (xdebug > 1)
			error("scgp %p scgp2 %p\n", scgp, scgp2);
		if (scgp2)
			scg_close(scgp2);
		if (scgp) {
			scgp->silent++;
			ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
			scgp->silent--;
			if (ret >= 0 || scgp->scmd->error == SCG_RETRYABLE) {
				chkprint("First SCSI open is still usable\n");
			} else {
				scg_freebuf(scgp);
				scg_close(scgp);
				scgp = doopen(device);
				getbuf(scgp);
				if (xdebug > 1)
					error("scgp %p\n", scgp);
			}
		}
	}
#endif	/* CHECK_SECOND_OPEN */

	printf("Ready to start test for succeeded command? Enter <CR> to continue: ");
	(void) chkgetline(abuf, sizeof (abuf));
	chkprint("**********> Checking for succeeded SCSI command.\n");
	scgp->verbose++;
	ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
	scg_vsetup(scgp);
	scg_errfflush(scgp, logfile);
	scgp->verbose--;
	scg_fprbytes(logfile, "Inquiry Data   :", (Uchar *)buf,
			sizeof (struct scsi_inquiry) - scg_getresid(scgp));

	if (ret >= 0 && !scg_cmd_err(scgp)) {
		chkprint("----------> SCSI succeeded command test PASSED\n");
	} else {
		chkprint("----------> SCSI succeeded command test FAILED\n");
	}

	sensetest(scgp);
	if (!autotest)
		chkprint("----------> SCSI status byte test NOT YET READY\n");
/*
 * scan OK
 * work OK
 * fail OK
 * sense data/count OK
 * SCSI status
 * dma resid
 * ->error GOOD/FAIL/timeout/noselect
 *    ??
 *
 * reset
 */

	dmaresid(scgp);
	chkprint("----------> SCSI transport code test NOT YET READY\n");
	return (0);
}

LOCAL SCSI *
doopen(sdevname)
	char	*sdevname;
{
	SCSI	*scgp;
	char	errstr[128];

	if ((scgp = scg_open(sdevname, errstr, sizeof (errstr), debug, lverbose)) == (SCSI *)0) {
		errmsg("%s%sCannot open SCSI driver.\n", errstr, errstr[0]?". ":"");
		fprintf(logfile, "%s. %s%sCannot open SCSI driver.\n",
			errmsgstr(geterrno()), errstr, errstr[0]?". ":"");
		errmsgno(EX_BAD, "For possible targets try 'cdrecord -scanbus'. Make sure you are root.\n");
		return (scgp);
	}
	scg_settimeout(scgp, deftimeout);
	scgp->verbose = scsi_verbose;
	scgp->silent = silent;
	scgp->debug = debug;
	scgp->kdebug = kdebug;
	scgp->cap->c_bsize = 2048;

	return (scgp);
}

LOCAL void
checkversion(scgp)
	SCSI	*scgp;
{
	char	*vers;
	char	*auth;

	/*
	 * Warning: If you modify this section of code, you must
	 * change the name of the program.
	 */
	vers = scg_version(0, SCG_VERSION);
	auth = scg_version(0, SCG_AUTHOR);
	chkprint("Using libscg version '%s-%s'\n", auth, vers);
	if (auth == 0 || strcmp("schily", auth) != 0) {
		errmsgno(EX_BAD,
		"Warning: using inofficial version of libscg (%s-%s '%s').\n",
			auth, vers, scg_version(0, SCG_SCCS_ID));
	}

	vers = scg_version(scgp, SCG_VERSION);
	auth = scg_version(scgp, SCG_AUTHOR);
	if (lverbose > 1)
		error("Using libscg transport code version '%s-%s'\n", auth, vers);
	fprintf(logfile, "Using libscg transport code version '%s-%s'\n", auth, vers);
	if (auth == 0 || strcmp("schily", auth) != 0) {
		errmsgno(EX_BAD,
		"Warning: using inofficial libscg transport code version (%s-%s '%s').\n",
			auth, vers, scg_version(scgp, SCG_SCCS_ID));
	}
	vers = scg_version(scgp, SCG_KVERSION);
	if (vers == NULL)
		vers = unavail;
	fprintf(logfile, "Using kernel transport code version '%s'\n", vers);

	vers = scg_version(scgp, SCG_RVERSION);
	auth = scg_version(scgp, SCG_RAUTHOR);
	if (lverbose > 1 && vers && auth)
		error("Using remote transport code version '%s-%s'\n", auth, vers);

	if (auth != 0 && strcmp("schily", auth) != 0) {
		errmsgno(EX_BAD,
		"Warning: using inofficial remote transport code version (%s-%s '%s').\n",
			auth, vers, scg_version(scgp, SCG_RSCCS_ID));
	}
	if (auth == NULL)
		auth = unavail;
	if (vers == NULL)
		vers = unavail;
	fprintf(logfile, "Using remote transport code version '%s-%s'\n", auth, vers);
}

LOCAL void
getbuf(scgp)
	SCSI	*scgp;
{
	bufsize = scg_bufsize(scgp, MAX_BUF_SIZE);
	chkprint("Max DMA buffer size: %ld\n", bufsize);
	seterrno(0);
	if ((buf = scg_getbuf(scgp, bufsize)) == NULL) {
		errmsg("Cannot get SCSI buffer (%ld bytes).\n", bufsize);
		fprintf(logfile, "%s. Cannot get SCSI buffer (%ld bytes).\n",
			errmsgstr(geterrno()), bufsize);
	} else {
		scg_freebuf(scgp);
	}

	bufsize = scg_bufsize(scgp, BUF_SIZE);
	if (debug)
		error("SCSI buffer size: %ld\n", bufsize);
	if ((buf = scg_getbuf(scgp, bufsize)) == NULL)
		comerr("Cannot get SCSI I/O buffer.\n");
}

LOCAL void
scg_openerr(errstr)
	char	*errstr;
{
	errmsg("%s%sCannot open or use SCSI driver.\n", errstr, errstr[0]?". ":"");
	errmsgno(EX_BAD, "For possible targets try 'cdrecord -scanbus'.%s\n",
				geteuid() ? " Make sure you are root.":"");
	errmsgno(EX_BAD, "For possible transport specifiers try 'cdrecord dev=help'.\n");
	exit(EX_BAD);
}

LOCAL int
find_drive(scgp, sdevname, flags)
	SCSI	*scgp;
	char	*sdevname;
	int	flags;
{
	int	ntarget;
	int	type = INQ_ROMD;

	if ((flags & F_MSINFO) == 0)
		error("No target specified, trying to find one...\n");
	ntarget = find_target(scgp, type, -1);
	if (ntarget < 0)
		return (ntarget);
	if (ntarget == 1) {
		/*
		 * Simple case, exactly one CD-ROM found.
		 */
		find_target(scgp, type, 1);
	} else if (ntarget <= 0 && (ntarget = find_target(scgp, type = INQ_WORM, -1)) == 1) {
		/*
		 * Exactly one CD-ROM acting as WORM found.
		 */
		find_target(scgp, type, 1);
	} else if (ntarget <= 0) {
		/*
		 * No single CD-ROM or WORM found.
		 */
		errmsgno(EX_BAD, "No CD/DVD/BD-Recorder target found.\n");
		errmsgno(EX_BAD, "Your platform may not allow to scan for SCSI devices.\n");
		comerrno(EX_BAD, "Call 'cdrecord dev=help' or ask your sysadmin for possible targets.\n");
	} else {
		errmsgno(EX_BAD, "Too many CD/DVD/BD-Recorder targets found.\n");
#ifdef	nonono
		select_target(scgp, stdout);
		comerrno(EX_BAD, "Select a target from the list above and use 'cdrecord dev=%s%sb,t,l'.\n",
			sdevname?sdevname:"",
			sdevname?(sdevname[strlen(sdevname)-1] == ':'?"":":"):"");
#endif	/* nonono */
		find_target(scgp, type, 1);
	}
	if ((flags & F_MSINFO) == 0)
		error("Using dev=%s%s%d,%d,%d.\n",
			sdevname?sdevname:"",
			sdevname?(sdevname[strlen(sdevname)-1] == ':'?"":":"):"",
			scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));

	return (ntarget);
}


EXPORT void
flushit()
{
	flush();
	fflush(logfile);
}

/*--------------------------------------------------------------------------*/
#include <schily/fcntl.h>

int
countopen()
{
	int	nopen = 0;
	int	i;

	for (i = 0; i < 1000; i++) {
		if (fcntl(i, F_GETFD, 0) >= 0)
			nopen++;
	}
	return (nopen);
}
/*--------------------------------------------------------------------------*/
#include <schily/varargs.h>

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
chkprint(const char *fmt, ...)
#else
EXPORT int
chkprint(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	ret = js_fprintf(stdout, "%r", fmt, args);
	va_end(args);
	if (ret < 0)
		return (ret);
#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	ret = js_fprintf(logfile, "%r", fmt, args);
	va_end(args);
	return (ret);
}

EXPORT int
chkgetline(lbuf, len)
	char	*lbuf;
	int	len;
{
	flushit();
	if (autotest) {
		printf("\n");
		flush();
		if (len > 0)
			lbuf[0] = '\0';
		return (0);
	}
	return (getline(lbuf, len));
}
