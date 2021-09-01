/* @(#)fmt.c	1.100 21/08/20 Copyright 1986-1991, 93-97, 2000-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fmt.c	1.100 21/08/20 Copyright 1986-1991, 93-97, 2000-2021 J. Schilling";
#endif
/*
 *	Format & check/repair SCSI disks
 *
 *	Copyright (c) 1986-1991, 93-97, 2000-2021 J. Schilling
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

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/signal.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/time.h>
#include <schily/errno.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/libport.h>
#include <schily/nlsdefs.h>

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsidefs.h>
#include <scg/scsitransp.h>

#include "defect.h"
#include "scsicmds.h"
#include "fmt.h"

#define	strindex(s1, s2)	strstr((s2), (s1))

char	fmt_version[] = "3.7";

long		thistime = 0;
struct timeval	starttime;
struct timeval	stoptime;

char	*Sbuf;
long	Sbufsize;
int	xdebug;
int	debug;
int	nomap;
int	nowait;
int	force;
int	ask;
int	noformat;
int	save_mp;
int	defmodes;
int	no_heuristic_defaults;
int	scsi_compliant;
int	ign_not_found;

int	Proto;
int	Prpart;
int	label;
int	translate_blk;
int	autoformat;
int	reformat_only;
int	disable_mdl;
int	reassign;
int	greassign;
int	veri;
int	repair;
int	refresh_only;
int	wrveri;
int	setmodes;
int	modesel;
LOCAL	int	deftimeout;
/*#define	NVERI	5*/
/*extern	int	n_test_patterns;*/
/*#define	NWVERI	n_test_patterns*/
/*#define	CVERI	1000*/
/*#define	CWVERI	1000*/
int	Nveri	= -1;
int	Cveri	= -1;
int	CWveri	= -1;
long	Vstart	= 0;
long	Vend	= -1;
long	MAXbad	= -1;
int	S;
int	ESDI	= 0;
int	do_inq;
int	prgeom;
int	prcurgeom;
int	clearnull;
int	readnull;
int	damage;
int	prdefect;
int	seek;
int	Sstart;
int	Sstop;
int	help;
int	xhelp;
int	format_confirmed;
int	format_done;
int	randv;
int	randrw;
long	RW = -1L;
char	*datafile;
BOOL	datfile_present;
BOOL	datfile_chk = TRUE;	/* No bad data, if no data base is present */

struct	disk	cur_disk;
struct	disk	alt_disk;

extern	char	*Lname;
extern	char	*Lproto;

LOCAL	void	usage			__PR((int ret));
LOCAL	void	xusage			__PR((int ret));
EXPORT	int	main			__PR((int ac, char **av));
LOCAL	void	print_product		__PR((struct scsi_inquiry *ip));
LOCAL	void	select_target		__PR((SCSI *scgp));
LOCAL	void	select_unit		__PR((SCSI *scgp));
LOCAL	int	format_one		__PR((SCSI *scgp));
EXPORT	void	getdev			__PR((SCSI *scgp, BOOL print));
LOCAL	void	printdev		__PR((SCSI *scgp));
LOCAL	void	translate_lba		__PR((SCSI *scgp));
LOCAL	void	read_sector_header	__PR((SCSI *scgp));
LOCAL	void	esdi_command		__PR((SCSI *scgp));
LOCAL	long	estimate_format_time	__PR((struct disk *dp, int mult));
EXPORT	void	estimate_times		__PR((struct disk *dp));
EXPORT	void	print_fmt_time		__PR((struct disk *dp));
EXPORT	char	*datestr		__PR((void));
EXPORT	void	prdate			__PR((void));
EXPORT	void	getstarttime		__PR((void));
EXPORT	void	getstoptime		__PR((void));
EXPORT	long	gettimediff		__PR((struct timeval *tp));
EXPORT	long	prstats			__PR((void));
EXPORT	void	helpexit		__PR((void));
EXPORT	void	disk_null		__PR((struct disk *dp, int init));

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\tformat [options] [dev=scsidev]|[target lun [scsibus]]\n");
	error("options:\n");
	error("\tdev=target	SCSI target to use\n");
	error("\tscgopts=spec	SCSI options for libscg\n");
	error("\t-help,-h\tprint this help\n");
	error("\t-xhelp\t\tprint extended help\n");
	error("\t-version\tPrint version number.\n");
	error("\t-nomap\t\tDo not map SCSI address to logical disk name\n");
	error("\t-nowait\t\tDo not wait after formatting disk\n");
	error("\t-force\t\tForce to continue at certain errors\n");
	error("\t-ask\t\tAsk again at certain critical sections\n");
	error("\t-noformat\tForce not to format disk\n");
	error("\t-smp\t\tDo not try to save mode parameters\n");
	error("\tdata=name\tName of diskdata file (default: 'sformat.dat')\n");
#ifdef	OLD
	error("\t-label,-l\tproduce disk label in file 'Label'\n");
	error("\tlname=name\tName of output Label (default: '%s')\n", Lname);
	error("\tlproto=name\tName of input  Label (default: '%s')\n", Lproto);
#endif
	error("\t-tr,-t\t\ttranslate blocknumbers\n");
	error("\t-auto,-a\tautoformat mode (for production systems)\n");
	error("\t-dmdl\t\tdo not use manufacturer defect list\n");
	error("\t-r\t\treformat only mode (gives less messages on label menu)\n");
	error("\t-reassign\treassign block (fancy mode: for prefered use)\n");
	error("\t-greassign\treassign block (guru mode: don't use this)\n");
	error("\t-verify\t\tverify disk\n");
	error("\t-repair\t\tverify and repair disk\n");
	error("\t-refresh_only\trefresh only with -repair (do not reassign)\n");
	error("\t-modes\t\tintercative modesense/modeselect\n");
	error("\t-setmodes\tdo only a modeselect instead of formatting the disk\n");
	error("\t-wrveri\t\tdo write verify instead of verify\n");
	error("\t-randv\t\trandom verify-Test\n");
	error("\t-randrw\t\trandom R/W-Test\n");
	error("\t-seek\t\tdo seek tests\n");
	error("\t-inq\t\tget and print Inquiry Data\n");
	error("\t-prgeom\t\tget and print geometry Data\n");
	error("\t-prdefect\tget and print defect Data\n");
	error("\t-Proto\t\tgenerate data base entry for disk\n");

	error("\n");
	error("\tAll answers to non numeric questions\n");
	error("\texcept [y]es or [Y]ES are interpreted as if <no> has been entered\n");
	error("\tActions that may damage data must be confirmed with <yes>\n");
	exit(ret);
}	

LOCAL void
xusage(ret)
	int	ret;
{
	error("Usage:\tformat [options] target lun [scsibus]\n");
	error("Extended options:\n");
	error("\tdebug=#,-d\tSet to # or increment misc debug level\n");
	error("\tkdebug=#,kd=#\tKernel debug value for scg driver\n");
	error("\txdebug=#,xd=#\tDebug level for eXternal disk database\n");
	error("\t-verbose,-v\tincrement SCSI verbose level by one (outdated)\n");
	error("\t-Verbose,-V\tincrement SCSI command transport verbose level by one\n");
	error("\t-silent,-s\tdo not print status of failed SCSI commands\n");
	error("\t-defmodes\tGet default mode parameters from disk\n");
	error("\t-no_defaults\tDo not set heuristic defaults\n");
	error("\t-scsi_compliant\tBe as SCSI-compliant as possible\n");
	error("\t-ign_not_found\tIgnore record not found errors on verify\n");
	error("\ttimeout=#\tTimeout for SCSI commands (default %d)\n", deftimeout);
	error("\tVL=#\t\tNumber of verify loops (default %d)\n", NVERI);
	error("\tRW=#\t\tNumber of random R/W loops\n");
	error("\t\t\t(default Number of physical Sectors / 100)\n");
	error("\tCveri=#\t\tNumber of blocks/verify\n");
	error("\tCWveri=#\tNumber of blocks/write-verify\n");
	error("\tVstart=#\tNumber of first block to verify\n");
	error("\tVend=#\tNumber of last block to verify\n");
	error("\tmaxbad=#\tNumber of max. bad blocks/verify loop before reformat \n");
#ifdef	OLD
	error("\tnhead=#\t\tNumber of Heads\n");
	error("\tlhead=#\t\tNumber of logical Heads (Label)\n");
	error("\tpcyl=#\t\tNumber of physical Cylinders\n");
	error("\tatrk=#\t\tNumber of alternate Tracks/Volume\n");
	error("\tlacyl=#\t\tNumber of Label alternate Cylinders\n");
	error("\tlncyl=#\t\tNumber of data Cylinders (Label)\n");
	error("\ttpz=#\t\tNumber of Tracks/Zone\n");
	error("\tspt=#\t\tNumber of physical Sectors/Track\n");
	error("\tlspt=#\t\tNumber of logical Sectors/Track (Label)\n");
	error("\taspz=#\t\tNumber of alternate Sectors/Zone\n");
	error("\tsecsize=#\tNumber of Bytes/Sector\n");
	error("\trpm=#\t\tNumber of revolutions/minute\n");
	error("\ttrskew=#\tTrack skew\n");
	error("\tcylskew=#\tCylinder skew\n");
	error("\tinterlv=#\tInterleave\n");
	error("\tfmtpat=#\tFormat Pattern\n");
#endif

	exit(ret);
}	

char opts[] = "version,dev*,scgopts*,kdebug#,kd#,xdebug#,xd#,debug#,d+,silent,s,verbose+,v+,Verbose+,V+,nomap,nowait,force,ask,noformat,smp,defmodes,no_defaults,scsi_compliant,ign_not_found,data*,Proto,Prpart,P,label,l,lname*,lproto*,tr,t,auto,a,dmdl,r,reassign,greassign,verify,repair,refresh_only,modes,setmodes,timeout#,noparity,wrveri,VL#,Cveri#,C#,Vstart#L,Vend#L,CWveri#,CW#,maxbad#L,randv,randrw,RW#L,S,ESDI#,inq,prgeom,prcurgeom,clearnull,readnull,damage,prdefect,seek,start,stop,nhead#l,lhead#l,pcyl#l,atrk#l,lacyl#l,lncyl#l,tpz#l,spt#l,lspt#l,aspz#l,secsize#l,rpm#l,trskew#l,cylskew#l,interlv#l,fmtpat#l,help,h,xhelp";

#define	MAXVERI	100

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	ret	= 0;
	int	fcount;
	int	cac;
	char	* const *cav;
	int	scsibus	= -1;
	int	target	= -1;
	int	lun	= -1;
	BOOL	prvers	= FALSE;
	int	silent	= 0;
	int	verbose	= 0;
	int	kdebug	= 0;
	int	noparity = 0;
	SCSI	*scgp;
	char	*dev = NULL;
	char	*scgopts = NULL;

	save_args(ac, av);

	(void) setlocale(LC_ALL, "");

#ifdef  USE_NLS
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "sformat"	/* Use this only if it weren't */
#endif
	{ char	*dir;
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#if defined(PROTOTYPES) && defined(INS_BASE)
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
	}
#endif 	/* USE_NLS */
	disk_null(&cur_disk, 1);
	disk_null(&alt_disk, 1);

	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, opts,
#ifndef	lint		/* lint kann leider nur 52 args !!! */
			&prvers, &dev, &scgopts,
			&kdebug, &kdebug, &xdebug, &xdebug, &debug, &debug,
			&silent, &silent,
			&verbose, &verbose,
			&verbose, &verbose,
			&nomap,
			&nowait,
			&force, &ask,
			&noformat,
			&save_mp,
			&defmodes,
			&no_heuristic_defaults,
			&scsi_compliant,
			&ign_not_found,
			&datafile,
			&Proto, &Prpart, &Prpart,
			&label, &label,
			&Lname, &Lproto,
			&translate_blk, &translate_blk,
			&autoformat, &autoformat,
			&disable_mdl,
			&reformat_only,
			&reassign, &greassign,
			&veri, &repair, &refresh_only,
			&modesel, &setmodes,
			&deftimeout,
			&noparity,
			&wrveri, &Nveri, &Cveri, &Cveri,
			&Vstart, &Vend,
			&CWveri, &CWveri,
			&MAXbad,
			&randv,
			&randrw, &RW,
			&S,
			&ESDI,
			&do_inq,
			&prgeom, &prcurgeom,
			&clearnull,
			&readnull,
			&damage,
			&prdefect,
			&seek,
			&Sstart, &Sstop,
			&cur_disk.nhead, &cur_disk.lhead,
			&cur_disk.pcyl, &cur_disk.atrk,
			&cur_disk.lacyl, &cur_disk.lncyl,
			&cur_disk.tpz, &cur_disk.spt,
			&cur_disk.lspt, &cur_disk.aspz,
			&cur_disk.secsize,
			&cur_disk.rpm,
			&cur_disk.track_skew, &cur_disk.cyl_skew,
			&cur_disk.interleave, &cur_disk.fmt_pattern,
			&help, &help, &xhelp,

#endif
					0) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	else if (xhelp)
		xusage(0);
	if (prvers) {
		gtprintf("sformat %s (%s-%s-%s)\n\n", fmt_version, HOST_CPU, HOST_VENDOR, HOST_OS);
		gtprintf("Copyright (C) 1986-1991, 93-97, 2000-2021 %s\n", _("Jörg Schilling"));
		gtprintf("This is free software; see the source for copying conditions.  There is NO\n");
		gtprintf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}

	if (Sstart && Sstop)
		comerrno(EX_BAD, "Only one of start/stop\n");

	if (getenv("FMT_SILENT"))
		silent = TRUE;
	if (getenv("FMT_AUTO"))
		autoformat = TRUE;

	if (autoformat)
		defmodes = TRUE;

	if (setmodes)
		noformat = TRUE;

	if (autoformat && (force || noformat))
		comerrno(EX_BAD, "Cannot specify force/noformat in auto mode\n");

	if (kdebug)
		silent = TRUE;

	if (Proto) {
		Prpart = TRUE;
		defmodes = TRUE;
		no_heuristic_defaults = TRUE;
		scsi_compliant = TRUE;
	}

	if (Nveri != -1 && (Nveri < 0 || Nveri > 1000))
		comerrno(EX_BAD, "Bad number of Verify loops\n");
	if (Cveri != -1 && (Cveri < 1 || Cveri > 65000))
		comerrno(EX_BAD, "Bad number of Sectors/Verify\n");
	if (CWveri != -1 && CWveri < 1)
		comerrno(EX_BAD, "Bad number of Sectors/Write-Verify\n");
	if (MAXbad != -1 && (MAXbad <= 0L || MAXbad > 127L))
		comerrno(EX_BAD, "Bad number of max. bad Blocks / verify loop\n");
	if (Vstart < 0L || (Vend > 0L && Vstart > Vend))
		comerrno(EX_BAD, "Bad verify start %ld.\n", Vstart);

	if (cur_disk.nhead == 0 || cur_disk.nhead > 32) /* XXX */
		comerrno(EX_BAD, "Bad number of heads\n");
	if (cur_disk.lhead == 0 || cur_disk.lhead > 32) /* XXX */
		comerrno(EX_BAD, "Bad number of logical heads\n");

	if (cur_disk.fmt_pattern < -1 || cur_disk.fmt_pattern > 0xFF)
		comerrno(EX_BAD, "Bad format pattern\n");

	save_mp ^= 1;

	fcount = 0;
	cac = ac;
	cav = av;

	while (getfiles(&cac, &cav, opts) > 0) {
		fcount++;
		switch (fcount) {

		case 1:
			if (*astoi(cav[0], &target) != '\0') {
				errmsgno(EX_BAD,
					"Target '%s' is not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
			break;
		case 2:
			if (*astoi(cav[0], &lun) != '\0') {
				errmsgno(EX_BAD,
					"Lun '%s' is not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
			break;
		case 3:
			if (*astoi(cav[0], &scsibus) != '\0') {
				errmsgno(EX_BAD,
					"Scsibus '%s' is not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
			break;
		default:
			errmsgno(EX_BAD, "Unknown arg '%s'.\n", cav[0]);
			usage(EX_BAD);
			/* NOTREACHED */
		}
		cac--;
		cav++;
	}
	getstarttime();

	scg_remote();
	if (dev) {
		char	errstr[80];

/*		if ((scgp = scg_open(dev, errstr, sizeof (errstr), debug, lverbose)) == (SCSI *)0)*/
		if ((scgp = scg_open(dev, errstr, sizeof (errstr), debug, 0)) == (SCSI *)0)
			comerr("%s%sCannot open SCSI driver.\n", errstr, errstr[0]?". ":"");
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
	if (scgopts) {
		int	i = scg_opts(scgp, scgopts);
		if (i <= 0)
			exit(i < 0 ? EX_BAD : 0);
	}
	scgp->silent = silent;
	scgp->verbose = verbose;
	scgp->debug = debug;
	scgp->kdebug = kdebug;
	if (deftimeout > 0)
		scgp->deftimeout = deftimeout;
	scgp->noparity = noparity;

	Sbufsize = scg_bufsize(scgp, 256*1024L);
	if ((Sbuf = scg_getbuf(scgp, Sbufsize)) == NULL)
		comerr("Cannot get SCSI I/O buffer.\n");
	if (debug)
		printf("bufsize: %ld bufaddr: %p\n", Sbufsize, Sbuf);

	if (CWveri > (Sbufsize/MIN_SECSIZE))
		comerrno(EX_BAD, "Too many Sectors/Write-Verify\n");

	if ((datfile_present = opendatfile(datafile)) == TRUE)
		datfile_chk = datfile_chksum();

	if (!autoformat) {
/*		signal(SIGINT, sighandler);*/
	}

	gtprintf("sformat SCSI format/analysis/repair utilities\n");
	gtprintf("Release %s, Copyright J. Schilling\n\n", fmt_version);

	if (!datfile_chk) {
		if (datfile_present)
			error(
			"Disk database '%s' contains uncertified data.\n",
							datfilename());
		if (autoformat) {
			error("Datenbasis ist zerstoert.\n");
			exit(EX_BAD);
		}
	}

/* XXX schon vor scg__open()! */
/*	if (scgp->scsibus < 0)*/
/*		scgp->scsibus = 0;*/
	if (scg_target(scgp) < 0 || scg_target(scgp) > 7 || scg_lun(scgp) < 0 || scg_lun(scgp) > 7) {
		if (autoformat || scg_target(scgp) != -1 || scg_lun(scgp) != -1)
			errmsgno(EX_BAD, "Target or lun missing or bad\n");
		if (autoformat)
			exit(EX_BAD);
		select_target(scgp);
		exit(0);
	}
	if (prgeom || prcurgeom)
		ret = printgeom(scgp, prcurgeom);
	else
		ret = format_one(scgp);
	exit(ret);
	/* NOTREACHED */
	return (ret);		/* Keep lint happy */
}	/* end of main() */

LOCAL void
print_product(ip)
	struct	scsi_inquiry *ip;
{
	printf("'%.8s' ", ip->inq_vendor_info);
	printf("'%.16s' ", ip->inq_prod_ident);
	printf("'%.4s' ", ip->inq_prod_revision);
	if (ip->add_len < 31) {
		printf("NON CCS ");
	}
	scg_printdev(ip);
}

LOCAL void
select_target(scgp)
	SCSI	*scgp;
{
	int	initiator;
#ifdef	FMT
	int	cscsibus = scg_scsibus(scgp);
	int	ctarget  = scg_target(scgp);
	int	clun	 = scg_lun(scgp);
#endif
	int	n;
	int	low	= -1;
	int	high	= -1;
	int	bus;
	int	tgt;
	int	lun = 0;
	int	err;
	BOOL	have_tgt;

	scgp->silent++;

	for (bus = 0; bus < 16; bus++) {
		scg_settarget(scgp, bus, 0, 0);

		if (!scg_havebus(scgp, bus))
			continue;

		initiator = scg_initiator_id(scgp);
		printf("scsibus%d:\n", bus);

		for (tgt = 0; tgt < 16; tgt++) {
			n = bus*100 + tgt;

			scg_settarget(scgp, bus, tgt, lun);
			seterrno(0);
			have_tgt = unit_ready(scgp) || scgp->scmd->error != SCG_FATAL;
			err = geterrno();
			if (err == EPERM || err == EACCES)
				exit(EX_BAD);

			if (!have_tgt && tgt > 7) {
				if (scgp->scmd->ux_errno == EINVAL)
					break;
				continue;
			}

#ifdef	FMT
			if (print_disknames(bus, tgt, -1) < 8)
				printf("\t");
			else
				printf(" ");
#else
			printf("\t");
#endif
			if (printf("%d,%d,%d", bus, tgt, lun) < 8)
				printf("\t");
			else
				printf(" ");
			printf("%3d) ", n);
			if (tgt == initiator) {
				printf("HOST ADAPTOR\n");
				continue;
			}
			if (!have_tgt) {
				printf("*\n");
				continue;
			}
			if (low < 0)
				low = n;
			high = n;

			getdev(scgp, FALSE);
			print_product(scgp->inq);
		}
	}
	scgp->silent--;

	if (low < 0)
		comerrno(EX_BAD, "No target found.\n");
	n = -1;
#ifdef	FMT
	getint("Select target", &n, low, high);
	bus = n/100;
	tgt = n%100;
	scg_settarget(scgp, bus, tgt, lun);
	select_unit(scgp);

	scg_settarget(scgp, cscsibus, ctarget, clun);
#else
	exit(0);
#endif
}

LOCAL void
select_unit(scgp)
	SCSI	*scgp;
{
	int	initiator;
	int	clun	= scg_lun(scgp);
	int	low	= -1;
	int	high	= -1;
	int	lun;

	scgp->silent++;

	printf("scsibus%d target %d:\n", scg_scsibus(scgp), scg_target(scgp));

	initiator = scg_initiator_id(scgp);
	for (lun = 0; lun < 8; lun++) {

#ifdef	FMT
		if (print_disknames(scg_scsibus(scgp), scg_target(scgp), lun) < 8)
			printf("\t");
		else
			printf(" ");
#else
		printf("\t");
#endif
		if (printf("%d,%d,%d", scg_scsibus(scgp), scg_target(scgp), lun) < 8)
			printf("\t");
		else
			printf(" ");
		printf("%3d) ", lun);
		if (scg_target(scgp) == initiator) {
			printf("HOST ADAPTOR\n");
			continue;
		}
		scg_settarget(scgp, scg_scsibus(scgp), scg_target(scgp), lun);
		if (!unit_ready(scgp) && scgp->scmd->error == SCG_FATAL) {
			printf("*\n");
			continue;
		}
		if (unit_ready(scgp)) {
			/* non extended sense illegal lun */
			if (scgp->scmd->sense.code == 0x25) {
				printf("BAD UNIT\n");
				continue;
			}
		}
		if (low < 0)
			low = lun;
		high = lun;

		getdev(scgp, FALSE);
		print_product(scgp->inq);
	}
	scgp->silent--;

	if (low < 0)
		comerrno(EX_BAD, "No lun found.\n");
	lun = -1;
#ifdef	FMT
	getint("Select lun", &lun, low, high);
	scg_settarget(scgp, scg_scsibus(scgp), scg_target(scgp), lun);
	format_one(scgp);
#else
	exit(0);
#endif

	scg_settarget(scgp, scg_scsibus(scgp), scg_target(scgp), clun);
}

LOCAL int
format_one(scgp)
	SCSI	*scgp;
{
	struct disk	*dp = &cur_disk;
	int	ret	= 0;
	int	i;

	printf("scsibus%d target %d lun %d\n", scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
	if (checkmount(scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp), -1L, 0L)) {
		gtprintf("WARNING: Disk has mounted partitions. ");
		if (!yes("Continue? "))
			exit(EX_BAD);
	}
	if (do_inq)
		scgp->silent++;
	getdev(scgp, TRUE);
	if (scgp->scmd->error == SCG_FATAL)
		comerrno(EX_BAD, "Cannot select Drive.\n");
	if (!do_inq)
		printdev(scgp);
	else
		scgp->silent--;
	if (do_inq) {
		if (scgp->verbose && (scgp->inq->add_len+5) > sizeof (struct scsi_inquiry)) {
			inquiry(scgp, Sbuf, (int)(scgp->inq->add_len+5));

			for (i = 0; i < (int)scgp->inq->add_len+5; i++) {
				if ((Sbuf[i]&0xFF) >= ' ' &&
							(Sbuf[i]&0xFF) <= '~')
					printf("%c", Sbuf[i]);
				else
					printf("\\%03o", Sbuf[i]&0xFF);
			}
			printf("\n");
		}
		return (0);
	}
	if (Sstart) {
		start_stop_unit(scgp, Sstart);
		return (0);
	}
	scgp->silent++;
	for (i = 0; ; i++) {
		if (unit_ready(scgp))
			break;
		if (i > 30 || scgp->scmd->error >= SCG_FATAL) {
			scg_printerr(scgp);
			comerrno(EX_BAD, "Drive not ready.\n");
		}
		if (scgp->scmd->scb.busy ||
			(scgp->scmd->scb.chk &&
			(scgp->scmd->sense_count >= 3 && scgp->scmd->sense.code >= 0x70 &&
				((struct scsi_ext_sense *)&scgp->scmd->sense)->key
				== SC_NOT_READY)) ||
				scgp->scmd->sense.code == 0x04 ||
				scgp->scmd->u_sense.cmd_sense[0] == 0x84)
			sleep(1);
	}
	/*
	 * Da ACB-5500 und die ACB-4000 Serie nicht zu unterscheiden sind,
	 * wenn das Laufwerk nicht bereit ist, wird hier noch einmal
	 * versucht, den Controllertyp zu bestimmen.
	 */
	if (scgp->dev == DEV_ACB40X0)
		getdev(scgp, FALSE);
	scgp->silent--;


	scgp->silent++;
	if (read_capacity(scgp) >= 0)		/* Adaptec 40x0 reagiert uebel, wenn*/
		(void) seek_scsi(scgp, 0L);	/* die Platte nicht formatiert ist */
	for (i = 0; i < 100; i++) {
		usleep(200000);
		if (unit_ready(scgp))
			break;
	}
	scgp->silent--;
	(void) rezero_unit(scgp);

	/*
	 * Exit here on wrong device type?
	 */
	if (scgp->inq->type == INQ_DASD || scgp->inq->type == INQ_ROMD ||
						scgp->inq->type == INQ_OMEM) {
		read_sinfo(scgp, dp, FALSE);
		print_sinfo(stdout, scgp);
	}

	if (reassign || greassign) {
		reassign_one(scgp);
		/* NOTREACHED */
	} else if (translate_blk) {
		if (strindex("EMULEX", scgp->inq->inq_vendor_info))
			read_sector_header(scgp);
		else
			translate_lba(scgp);
		return (0);
	} else if (veri || repair) {
		int	nbad = 0;

		verify_disk(scgp, dp, 0, Vstart, Vend, MAXbad);
		nbad = print_bad();
		if (!is_ccs(scgp->dev)) {		/* XXX Adaptec ?? */
			/* Only print them */
			(void) bad_to_def(scgp);
		} else if (repair && nbad > 0) {
			gtprintf("WARNING: Repair may change data on disk.\n");
			if (yes("Do you want to continue? "))
				repair_found_blocks(scgp, nbad);
		}
		return (0);
	} else if (modesel) {
		do_modes(scgp);
		/* NOTREACHED */
	} else if (randv) {
			return (random_v_test(scgp, Vstart, Vend));
	} else if (randrw) {
		gtprintf("WARNING: Random read/write-test may destroy data.\n");
		if (yes("Do you want to continue? "))
			return (random_rw_test(scgp, Vstart, Vend));
		return (0);
	}
	else if (ESDI)
		esdi_command(scgp);
	else if (clearnull)
		clear_phys_null(scgp);
	else if (readnull)
		read_phys_null(scgp, scgp->dev != DEV_MD21 && scgp->dev != DEV_MD23);
	else if (damage)
		damage_phys_blk(scgp);
	else if (prdefect) {
		(void) read_defect_list(scgp, 1, SC_DEF_BLOCK, TRUE); flush();
		(void) read_defect_list(scgp, 1, SC_DEF_BFI, TRUE); flush();
		(void) read_defect_list(scgp, 1, SC_DEF_PHYS, TRUE); flush();
		(void) read_defect_list(scgp, 2, SC_DEF_BLOCK, TRUE); flush();
		(void) read_defect_list(scgp, 2, SC_DEF_BFI, TRUE); flush();
		(void) read_defect_list(scgp, 2, SC_DEF_PHYS, TRUE); flush();
		return (0);
	} else if (Sstart || Sstop) {
		start_stop_unit(scgp, Sstart);
		return (0);
	} else if (seek) {
		int	err = 0;
		long	start = Vstart;
		long	end;
		long	amount;

		end = scgp->cap->c_baddr;
		if (start < 0 || start > end)
			start = 0L;
		if (Vend > 0 && Vend < end)
			end = Vend;
		amount = end - start + 1;
		printf("start: %ld end: %ld amount: %ld last baddr: %ld\n",
					start, end, amount, (long)scgp->cap->c_baddr);

		gtprintf("Select full stroke or random seeks:\n");
		if (yes("Full stroke seek? ")) for (;;) {
			if (i == 0)
				getstarttime();
			i++;
			if (read_scsi(scgp, Sbuf, start, 1) < 0)
				err++;
			if (read_scsi(scgp, Sbuf, end, 1) < 0)
				err++;

			if (i% 1000 == 0) {
				getstoptime();
				gtprintf("Total: %d errs: %d %ld.%03ldms/seek\n",
					i, err,
					gettimediff(0)/(i/1000),
					gettimediff(0)%1000);
			}
		} else for (;;) {
			if (i == 0)
				getstarttime();
			i++;
#ifdef	HAVE_DRAND48
			if (read_scsi(scgp, Sbuf,
					start + drand48() * amount, 1) < 0) {
#else
			if (read_scsi(scgp, Sbuf,
					start + rand() % amount, 1) < 0) {
#endif
				err++;
				gtprintf("Gesamt: %d errs: %d\n", i, err);
			}
			if (i% 1000 == 0) {
				getstoptime();
				gtprintf("Total: %d errs: %d %ld.%03ldms/seek\n",
					i, err,
					gettimediff(0)/(i/1000),
					gettimediff(0)%1000);
			}
		}
	}

	scgp->silent++;
	if (read_capacity(scgp) >= 0)		/* Adaptec 40x0 reagiert uebel, wenn*/
		(void) seek_scsi(scgp, 0L);	/* die Platte nicht formatiert ist */
	for (i = 0; i < 100; i++) {
		usleep(100000);
		if (unit_ready(scgp))
			break;
	}
	scgp->silent--;
	(void) rezero_unit(scgp);
	if (scgp->inq->type != INQ_DASD && scgp->inq->type != INQ_ROMD &&
	    scgp->inq->type != INQ_OMEM) {
		comerrno(EX_BAD, "Bad Device type: 0x%x\n", (int)scgp->inq->type);
	} else switch (scgp->dev) {

	case DEV_ACB40X0:
	case DEV_ACB4000:
	case DEV_ACB4010:
	case DEV_ACB4070:
	case DEV_ACB5500:	i = Adaptec4000(scgp);	break;
	case DEV_ACB4520A:
	case DEV_ACB4525:
	case DEV_MD21:
	case DEV_MD23:
	case DEV_NON_CCS_DSK:
	case DEV_CCS_GENDISK:
				i = Emulex_MD21(scgp);	break;
	case DEV_SONY_SMO:
				i = Emulex_MD21(scgp);	break; /* Temporaer */
	default:
	case DEV_UNKNOWN:	errmsgno(EX_BAD, "Unknown Device\n");
				i = Emulex_MD21(scgp);	break;
	}

	prstats();

	if (i == FALSE) {
		if (autoformat) {
			error("A C H T U N G\n");
			error("Diese Platte laeszt sich nicht formatieren\n");
			error("Bitte Entwicklung benachrichtigen\n");
			exit(EX_BAD);
		} else {
			comerrno(EX_BAD, "Cannot format disk.\n");
		}
	}

	create_label(scgp, dp);

	ret = verify_and_repair_disk(scgp, dp);

	label_disk(scgp, dp);
	convert_def_blk(scgp);
	write_def_blk(scgp, TRUE);
	return (ret);
}

EXPORT void
getdev(scgp, print)
	SCSI	*scgp;
	BOOL	print;
{
	BOOL	got_inquiry = TRUE;
	register struct scsi_inquiry *inq = scgp->inq;

	fillbytes((caddr_t)inq, sizeof (*inq), '\0');
	scgp->dev = DEV_UNKNOWN;
	scgp->silent++;
	(void) unit_ready(scgp);
	if (scgp->scmd->error >= SCG_FATAL &&
				!(scgp->scmd->scb.chk && scgp->scmd->sense_count > 0)) {
		scgp->silent--;
		return;
	}


/*	if (scgp->scmd->error < SCG_FATAL || scgp->scmd->scb.chk && scgp->scmd->sense_count > 0){*/

	if (inquiry(scgp, (caddr_t)inq, sizeof (*inq)) < 0) {
		got_inquiry = FALSE;
		if (scgp->verbose) {
			printf(
		"error: %d scb.chk: %d sense_count: %d sense.code: 0x%x\n",
				scgp->scmd->error, (int)scgp->scmd->scb.chk,
				scgp->scmd->sense_count, (int)scgp->scmd->sense.code);
		}
			/*
			 * Folgende Kontroller kennen das Kommando
			 * INQUIRY nicht:
			 *
			 * ADAPTEC	ACB-4000, ACB-4010, ACB 4070
			 * SYSGEN	SC4000
			 *
			 * Leider reagieren ACB40X0 und ACB5500 identisch
			 * wenn drive not ready (code == not ready),
			 * sie sind dann nicht zu unterscheiden.
			 */

		if (scgp->scmd->scb.chk && scgp->scmd->sense_count == 4) {
			/* Test auf SYSGEN			 */
			(void) qic02(scgp, 0x12);	/* soft lock on  */
			if (qic02(scgp, 1) < 0) {	/* soft lock off */
				scgp->dev = DEV_ACB40X0;
				scgp->dev = acbdev(scgp);
			} else {
				scgp->dev = DEV_SC4000;
				inq->type = INQ_SEQD;
				inq->removable = 1;
			}
		}
	}
	switch (inq->type) {

	case INQ_DASD:
		if (inq->add_len == 0) {
			if (scgp->dev == DEV_UNKNOWN && got_inquiry) {
				scgp->dev = DEV_ACB5500;
				strncpy(inq->inq_info_space,
					"ADAPTEC ACB-5500        FAKE",
					sizeof (inq->inq_info_space));

			} else switch (scgp->dev) {

			case DEV_ACB40X0:
				strncpy(inq->inq_info_space,
					"ADAPTEC ACB-40X0        FAKE",
					sizeof (inq->inq_info_space));
				break;
			case DEV_ACB4000:
				strncpy(inq->inq_info_space,
					"ADAPTEC ACB-4000        FAKE",
					sizeof (inq->inq_info_space));
				break;
			case DEV_ACB4010:
				strncpy(inq->inq_info_space,
					"ADAPTEC ACB-4010        FAKE",
					sizeof (inq->inq_info_space));
				break;
			case DEV_ACB4070:
				strncpy(inq->inq_info_space,
					"ADAPTEC ACB-4070        FAKE",
					sizeof (inq->inq_info_space));
				break;
			}
		} else if (inq->add_len < 31) {
			scgp->dev = DEV_NON_CCS_DSK;

		} else if (strindex("EMULEX", inq->inq_vendor_info)) {
			if (strindex("MD21", inq->inq_prod_ident))
				scgp->dev = DEV_MD21;
			if (strindex("MD23", inq->inq_prod_ident))
				scgp->dev = DEV_MD23;
			else
				scgp->dev = DEV_CCS_GENDISK;
		} else if (strindex("ADAPTEC", inq->inq_vendor_info)) {
			if (strindex("ACB-4520", inq->inq_prod_ident))
				scgp->dev = DEV_ACB4520A;
			if (strindex("ACB-4525", inq->inq_prod_ident))
				scgp->dev = DEV_ACB4525;
			else
				scgp->dev = DEV_CCS_GENDISK;
		} else if (strindex("SONY", inq->inq_vendor_info) &&
					strindex("SMO-C501", inq->inq_prod_ident)) {
			scgp->dev = DEV_SONY_SMO;
		} else {
			scgp->dev = DEV_CCS_GENDISK;
		}
		break;

	case INQ_SEQD:
		if (scgp->dev == DEV_SC4000) {
			strncpy(inq->inq_info_space,
				"SYSGEN  SC4000          FAKE",
					sizeof (inq->inq_info_space));
		} else if (inq->add_len == 0 &&
					inq->removable &&
						inq->ansi_version == 1) {
			scgp->dev = DEV_MT02;
			strncpy(inq->inq_info_space,
				"EMULEX  MT02            FAKE",
					sizeof (inq->inq_info_space));
		}
		break;

	case INQ_OPTD:
		if (strindex("RXT-800S", inq->inq_prod_ident))
			scgp->dev = DEV_RXT800S;
		break;

	case INQ_PROCD:
		if (strindex("BERTHOLD", inq->inq_vendor_info)) {
			if (strindex("", inq->inq_prod_ident))
				scgp->dev = DEV_HRSCAN;
		}
		break;

	case INQ_SCAN:
		scgp->dev = DEV_MS300A;
		break;
	}
	scgp->silent--;
	if (!print)
		return;

	if (scgp->dev == DEV_UNKNOWN && !got_inquiry)
		return;

	printf("Device type    : ");
	scg_printdev(inq);
	printf("Version        : %d\n", (int)inq->ansi_version);
	printf("Response Format: %d\n", (int)inq->data_format);
	if (inq->data_format >= 2) {
		printf("Capabilities   : ");
		if (inq->aenc)		printf("AENC ");
		if (inq->termiop)	printf("TERMIOP ");
		if (inq->reladr)	printf("RELADR ");
		if (inq->wbus32)	printf("WBUS32 ");
		if (inq->wbus16)	printf("WBUS16 ");
		if (inq->sync)		printf("SYNC ");
		if (inq->linked)	printf("LINKED ");
		if (inq->cmdque)	printf("CMDQUE ");
		if (inq->softreset)	printf("SOFTRESET ");
		printf("\n");
	}
	if (inq->add_len >= 31 ||
			inq->inq_vendor_info[0] ||
			inq->inq_prod_ident[0] ||
			inq->inq_prod_revision[0]) {
		printf("Vendor_info    : '%.8s'\n", inq->inq_vendor_info);
		printf("Identifikation : '%.16s'\n", inq->inq_prod_ident);
		printf("Revision       : '%.4s'\n", inq->inq_prod_revision);
	}
}

LOCAL void
printdev(scgp)
	SCSI	*scgp;
{
	gtprintf("Device seems to be: ");

	switch (scgp->dev) {

	case DEV_UNKNOWN:	printf("unknown");		break;
	case DEV_ACB40X0:	printf("Adaptec 4000/4010/4070"); break;
	case DEV_ACB4000:	printf("Adaptec 4000");		break;
	case DEV_ACB4010:	printf("Adaptec 4010");		break;
	case DEV_ACB4070:	printf("Adaptec 4070");		break;
	case DEV_ACB5500:	printf("Adaptec 5500");		break;
	case DEV_ACB4520A:	printf("Adaptec 4520A");	break;
	case DEV_ACB4525:	printf("Adaptec 4525");		break;
	case DEV_MD21:		printf("Emulex MD21");		break;
	case DEV_MD23:		printf("Emulex MD23");		break;
	case DEV_NON_CCS_DSK:	printf("Generic NON CCS Disk");	break;
	case DEV_CCS_GENDISK:	printf("Generic CCS Disk");	break;
	case DEV_SONY_SMO:	printf("Sony SMO-C501");	break;
	case DEV_MT02:		printf("Emulex MT02");		break;
	case DEV_SC4000:	printf("Sysgen SC4000");	break;
	case DEV_RXT800S:	printf("Maxtor RXT800S");	break;
	case DEV_HRSCAN:	printf("Berthold HR-Scanner");	break;
	case DEV_MS300A:	printf("Microtek MS300A");	break;

	}
	printf(".\n");

}

LOCAL void
translate_lba(scgp)
	SCSI	*scgp;
{
	struct scsi_def_list def;
	long	baddr = 0L;
	int	i;

loop:
	getlong("Block Address", &baddr, 0L, 1000000L);

	fillbytes((caddr_t)&def, sizeof (def), '\0');
	i = sizeof (struct scsi_def_bfi);	/* Ein Defekt */
	i_to_2_byte(def.hd.length, i);

	if (translate(scgp, &def.def_bfi[0], baddr) >= 0) {
		print_def_bfi(&def);
	}
	goto loop;
}

LOCAL void
read_sector_header(scgp)
	SCSI	*scgp;
{
	struct	scsi_send_diag_cmd	cmd;
	register struct	scsi_sector_header	*header;
	char	bh[512];
	long	baddr = 0L;
	register int i;
	register int maxi;

loop:
	fillbytes((caddr_t)&cmd, sizeof (cmd), '\0');
	fillbytes((caddr_t)bh, sizeof (bh), '\0');
	cmd.cmd = 7;	/* Read Header */
	getlong("Block Address", &baddr, 0L, 1000000L);
	i_to_4_byte(cmd.addr, baddr);
	cmd.addr[0] |= (yes("Physical Address? ")<<7);
/*	print_bk(baddr);*/

	if (send_diagnostic(scgp, (caddr_t)&cmd, sizeof (cmd)) >= 0)
		if (receive_diagnostic(scgp, (caddr_t)bh, sizeof (bh)) >= 0) {
			maxi = (sizeof (bh) - scg_getresid(scgp))/5;
			for (i = 0; i < maxi; i++) {
			header = (struct scsi_sector_header *)&bh[i*5];
			printf("Cyl %4d  Head %2d  Sec %2d (%2d)%s %s %s\n",
				a_to_u_2_byte(header->cyl),
				header->head,
				header->sec, i,
				header->dt ? "Defective Track" : "",
				header->sp ? "Spare Sector" : "",
				header->rp ? "Has been replaced" : "");
			}
		}
	goto loop;
}

LOCAL void
esdi_command(scgp)
	SCSI	*scgp;
{
	struct	scsi_send_diag_cmd	cmd;
	char	bh[512];
	register int i;
	register int maxi;

	fillbytes((caddr_t)&cmd, sizeof (cmd), '\0');
	fillbytes((caddr_t)bh, sizeof (bh), '\0');
	cmd.cmd = 4;	/* Pass Drive Command */
	cmd.addr[0] = 0xFF;
	i_to_2_byte(&cmd.addr[1], ESDI);

	if (send_diagnostic(scgp, (caddr_t)&cmd, sizeof (cmd)) >= 0)
		if (receive_diagnostic(scgp, (caddr_t)bh, sizeof (bh)) >= 0) {
			maxi = (sizeof (bh) - scg_getresid(scgp));
			i = 0;
			if (maxi)
				printf("ESDI Status: 0x%02X\n", bh[i++]&0xFF);
			for (; i < maxi; i++) {
				printf("0x%02X %08Z\n",
					(int)bh[i]&0xFF, (int)bh[i]&0xFF);
			}
			printf("\n");
			exit(0);
		}
	exit(127);
}

/*---------------------------------------------------------------------------
|
|	Nhead + 1 damit die Spurwechselzeit beruecksichtigt wird
|	Addition von rpm/2 bzw. 30 bewirkt Rundung
|
+---------------------------------------------------------------------------*/

LOCAL long
estimate_format_time(dp, mult)
	struct disk	*dp;
	int	mult;
{
	long etime;

	if (dp->rpm < 0 || dp->pcyl < 0 || dp->nhead < 0)
		etime = -1L;
	else if (mult == 0 || dp->rpm == 0)	/* Division duch 0 */
		etime = 0L;
	else
		etime = (mult * dp->pcyl * (dp->nhead+1)
			+ dp->rpm/2) / dp->rpm * 60L;
	return (etime);
}

EXPORT void
estimate_times(dp)
	struct disk	*dp;
{
	if (dp->fmt_time <= 0) {
		dp->fmt_time = estimate_format_time(dp, 2);
	} else if (dp->fmt_timeout < 0 || dp->fmt_timeout < 6 * dp->fmt_time) {
		if (dp->flags & D_FTIME_FOUND) {
			dp->fmt_timeout = 3 * dp->fmt_time;
		} else {
			dp->fmt_timeout = 6 * dp->fmt_time;
		}
	}
	if (dp->veri_time <= 0)
		dp->veri_time = estimate_format_time(dp,
					(int)(dp->interleave < 1 ? 1 :
					dp->interleave));
}

EXPORT void
print_fmt_time(dp)
	struct disk	*dp;
{
	if (dp->fmt_time > 0) {
		gtprintf("Estimated time: %ld minutes%s\n",
			(dp->fmt_time+30)/60,
			dp->flags & D_FTIME_FOUND?" (known)":"");
	}
}

EXPORT void
print_fmt_timeout(dp)
	struct disk	*dp;
{
	if (dp->fmt_timeout > 0) {
		gtprintf("Format timeout: %ld minutes\n",
			(dp->fmt_timeout+30)/60);
	}
}

EXPORT char *
datestr()
{
	time_t	clck;

	clck = time((time_t *)0);
	return (asctime(localtime(&clck)));
}

EXPORT void
prdate()
{
	printf("%s", datestr());
}

EXPORT void
getstarttime()
{
	if (gettimeofday(&starttime, (struct timezone *)0) < 0)
		comerr("Cannot get start time\n");
}

EXPORT void
getstoptime()
{
	if (gettimeofday(&stoptime, (struct timezone *)0) < 0)
		comerr("Cannot get stop time\n");
}

EXPORT long
gettimediff(tp)
	struct timeval *tp;
{
	long	sec;
	long	usec;
	long	tmsec;

	sec = stoptime.tv_sec - starttime.tv_sec;
	usec = stoptime.tv_usec - starttime.tv_usec;
	tmsec = sec*1000 + usec/1000;
#ifdef	lint
	tmsec = tmsec;	/* Bisz spaeter */
#endif
	while (usec < 0) {
		sec--;
		usec += 1000000;
	}
	if (tp != (struct timeval *)0) {
		tp->tv_sec = sec;
		tp->tv_usec = usec;
	}
	return (sec + (usec / 500000));
}

EXPORT long
prstats()
{
	long		sec;
	struct timeval	tv;

	getstoptime();
	sec = gettimediff(&tv);

	gtprintf("Time total: %ld.%03ldsec\n", (long)tv.tv_sec, (long)tv.tv_usec/1000);
	return (sec);
}

EXPORT void
helpexit()
{
	comerrno(EX_BAD, "Help.............\n");
	/* NOTREACHED */
}

#ifdef	used
initbuf()
{
	register char	*cp;
	register int	i;
	register int	len;
	static	 char	str[128];
		long	t;
	extern	char	*strcat();

	cp = Sbuf;
	t = time((time_t *)0);
	sprintf(str, "J. Schilling  -Schreibtest- %s", ctime(&t));
	len = strlen(str);
	str[--len] = '\0';
	len--;

	for (i = 0; i < Sbufsize; i++) {
		if (i % 2048 == 0) {
			(void) strcat(&cp[i], str);
			i += len;
		} else
			cp[i] = i & 0xFF;
	}
}
#endif

EXPORT void
disk_null(dp, init)
	struct disk	*dp;
	int		init;
{
	register int i = sizeof (struct disk);
	register char *p = (char *)dp;

	if (!init) {
		if (dp->disk_type)
			free(dp->disk_type);
		if (dp->alt_disk_type)
			free(dp->alt_disk_type);
		if (dp->default_part)
			free(dp->default_part);
		if (dp->mode_pages)
			free(dp->mode_pages);
		if (dp->parts) {
			struct node *pp = dp->parts;
			struct node *ppp;

			while ((ppp = pp) != 0) {
				pp = ppp->n_next;
				free(ppp->n_data);
				free((char *)ppp);
			}
		}
	}

	while (--i >= 0) {
		*p++ = -1;
	}
	dp->flags		= 0;
	dp->disk_type		= 0;
	dp->alt_disk_type	= 0;
	dp->default_part	= 0;
	dp->mode_pages		= 0;
	dp->parts		= 0;
}
