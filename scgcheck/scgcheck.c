/* @(#)scgcheck.c	1.11 07/09/30 Copyright 1998-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)scgcheck.c	1.11 07/09/30 Copyright 1998-2007 J. Schilling";
#endif
/*
 *	Copyright (c) 1998-2007 J. Schilling
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

#include <schily/mconfig.h>
#include <stdio.h>
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
EXPORT	void	flushit		__PR((void));
EXPORT	int	countopen	__PR((void));


	char	*dev;
int		debug;		/* print debug messages */
int		kdebug;		/* kernel debug messages */
int		scsi_verbose;	/* SCSI verbose flag */
int		lverbose;	/* local verbose flag */
int		silent;		/* SCSI silent flag */
int		deftimeout = 40; /* default SCSI timeout */
int		xdebug;		/* extended debug flag */

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
	error("\n");
	exit(ret);
}	

char	opts[]   = "debug#,d+,kdebug#,kd#,timeout#,verbose+,v+,Verbose+,V+,silent,s,x+,xd#,help,h,version,dev*,f*";

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
			&filename) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (pversion) {
		printf("scgcheck %s (%s-%s-%s) Copyright (C) 1998-2007 Jörg Schilling\n",
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

	printf("Scgcheck %s (%s-%s-%s) SCSI user level transport library ABI checker.\n\
Copyright (C) 1998,2001 Jörg Schilling\n",
						scgc_version,
						HOST_CPU, HOST_VENDOR, HOST_OS);
	fprintf(logfile, "Scgcheck %s (%s-%s-%s) SCSI user level transport library ABI checker.\n\
Copyright (C) 1998,2001 Jörg Schilling\n",
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

	printf("Checking if your implementation supports to scan the SCSI bus.\n");
	fprintf(logfile, "Checking if your implementation supports to scan the SCSI bus.\n");
	printf("Trying to open device: '%s'.\n", dev);
	fprintf(logfile, "Trying to open device: '%s'.\n", dev);
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

		printf("Trying to open device: '%s'.\n", device);
		fprintf(logfile, "Trying to open device: '%s'.\n", device);
		scgp = doopen(device);
	} while (scgp == NULL);
	if (scgp) {
		checkversion(scgp);
		getbuf(scgp);

		ret = select_target(scgp, stdout);
		select_target(scgp, logfile);
		scg_close(scgp);
		scgp = NULL;
		if (ret < 1) {
			printf("----------> SCSI scan bus test: found NO TARGETS\n");
			fprintf(logfile, "----------> SCSI scan bus test: found NO TARGETS\n");
		} else {
			printf("----------> SCSI scan bus test PASSED\n");
			fprintf(logfile, "----------> SCSI scan bus test PASSED\n");
		}
	} else {
		printf("----------> SCSI scan bus test FAILED\n");
		fprintf(logfile, "----------> SCSI scan bus test FAILED\n");
	}

	if (xdebug)
		error("nopen: %d\n", countopen());
	printf("For the next test we need to open a single SCSI device.\n");
	fprintf(logfile, "For the next test we need to open a single SCSI device.\n");
	printf("Best results will be obtained if you specify a modern CD-ROM drive.\n");
	fprintf(logfile, "Best results will be obtained if you specify a modern CD-ROM drive.\n");
	strcpy(device, "0,6,0");
	do {
		error("Enter SCSI device name [%s]: ", device);
		flushit();
		(void) getline(device, sizeof (device));
		if (device[0] == '\0')
			strcpy(device, "0,6,0");

		printf("Trying to open device: '%s'.\n", device);
		fprintf(logfile, "Trying to open device: '%s'.\n", device);
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
	flushit();
	(void) getline(abuf, sizeof (abuf));
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
			printf("First SCSI open OK - device usable\n");
			printf("Checking for second SCSI open.\n");
			fprintf(logfile, "First SCSI open OK - device usable\n");
			fprintf(logfile, "Checking for second SCSI open.\n");
			if ((scgp2 = doopen(device)) != NULL) {
				printf("Second SCSI open for same device succeeded, %d file descriptor(s) used.\n",
					countopen() - oldopen);
				fprintf(logfile,
					"Second SCSI open for same device succeeded, %d file descriptor(s) used.\n",
					countopen() - oldopen);
				scgp->silent++;
				ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
				scgp->silent--;
				if (ret >= 0 || scgp->scmd->error == SCG_RETRYABLE) {
					printf("Second SCSI open is usable\n");
					fprintf(logfile, "Second SCSI open is usable\n");
				}
				printf("Closing second SCSI.\n");
				fprintf(logfile, "Closing second SCSI.\n");
				scg_close(scgp2);
				scgp2 = NULL;
				printf("Checking first SCSI.\n");
				fprintf(logfile, "Checking first SCSI.\n");
				scgp->silent++;
				ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
				scgp->silent--;
				if (ret >= 0 || scgp->scmd->error == SCG_RETRYABLE) {
					printf("First SCSI open is still usable\n");
					printf("Second SCSI open test passed.\n");
					fprintf(logfile, "First SCSI open is still usable\n");
					fprintf(logfile, "Second SCSI open test passed.\n");
				} else if (ret < 0 && scgp->scmd->error == SCG_FATAL) {
					second_ok = FALSE;
					printf("First SCSI open does not work anymore.\n");
					printf("Second SCSI open test FAILED.\n");
					fprintf(logfile, "First SCSI open does not work anymore.\n");
					fprintf(logfile, "Second SCSI open test FAILED.\n");
				} else {
					second_ok = FALSE;
					printf("First SCSI open has strange problems.\n");
					printf("Second SCSI open test FAILED.\n");
					fprintf(logfile, "First SCSI open has strange problems.\n");
					fprintf(logfile, "Second SCSI open test FAILED.\n");
				}
			} else {
				second_ok = FALSE;
				printf("Cannot open same SCSI device a second time.\n");
				printf("Second SCSI open test FAILED.\n");
				fprintf(logfile, "Cannot open same SCSI device a second time.\n");
				fprintf(logfile, "Second SCSI open test FAILED.\n");
			}
		} else {
			second_ok = FALSE;
			printf("First SCSI open is not usable\n");
			printf("Second SCSI open test FAILED.\n");
			fprintf(logfile, "First SCSI open is not usable\n");
			fprintf(logfile, "Second SCSI open test FAILED.\n");
		}
		if (!second_ok && scgp2) {
			if (xdebug > 1)
				error("scgp %p scgp2 %p\n", scgp, scgp2);
			if (scgp)
				scg_close(scgp);
			if (scgp2)
				scg_close(scgp2);
			scgp = doopen(device);
			if (xdebug > 1)
				error("scgp %p\n", scgp);
		}
	}
#endif	/* CHECK_SECOND_OPEN */

	printf("Ready to start test for succeeded command? Enter <CR> to continue: ");
	flushit();
	(void) getline(abuf, sizeof (abuf));
	scgp->verbose++;
	ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
	scg_vsetup(scgp);
	scg_errfflush(scgp, logfile);
	scgp->verbose--;
	if (ret >= 0 && !scg_cmd_err(scgp)) {
		printf("----------> SCSI succeeded command test PASSED\n");
		fprintf(logfile, "----------> SCSI succeeded command test PASSED\n");
	} else {
		printf("----------> SCSI succeeded command test FAILED\n");
		fprintf(logfile, "----------> SCSI succeeded command test FAILED\n");
	}

	sensetest(scgp);
	printf("----------> SCSI status byte test NOT YET READY\n");
	fprintf(logfile, "----------> SCSI status byte test NOT YET READY\n");
/*
scan OK
work OK
fail OK
sense data/count OK
SCSI status
dma resid
->error GOOD/FAIL/timeout/noselect
	*    ??

reset
*/

	dmaresid(scgp);
	printf("----------> SCSI transport code test NOT YET READY\n");
	fprintf(logfile, "----------> SCSI transport code test NOT YET READY\n");
	return (0);
}

LOCAL SCSI *
doopen(devname)
	char	*devname;
{
	SCSI	*scgp;
	char	errstr[128];

	if ((scgp = scg_open(devname, errstr, sizeof (errstr), debug, lverbose)) == (SCSI *)0) {
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
	printf("Using libscg version '%s-%s'\n", auth, vers);
	fprintf(logfile, "Using libscg version '%s-%s'\n", auth, vers);
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
		printf("Max DMA buffer size: %ld\n", bufsize);
		fprintf(logfile, "Max DMA buffer size: %ld\n", bufsize);
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
