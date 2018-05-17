/* @(#)mt.c	1.28 11/08/14 Copyright 2000-2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)mt.c	1.28 11/08/14 Copyright 2000-2011 J. Schilling";
#endif
/*
 *	Magnetic tape manipulation program
 *
 *	Copyright (c) 2000-2011 J. Schilling
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

#include <schily/mconfig.h>

/*
 * XXX Until we find a better way, the next definitions must be in sync
 * XXX with the definitions in librmt/remote.c
 */
#if !defined(HAVE_FORK) || !defined(HAVE_SOCKETPAIR) || !defined(HAVE_DUP2)
#undef	USE_RCMD_RSH
#endif
#if !defined(HAVE_GETSERVBYNAME)
#undef	USE_REMOTE				/* Cannot get rcmd() port # */
#endif
#if (!defined(HAVE_NETDB_H) || !defined(HAVE_RCMD)) && !defined(USE_RCMD_RSH)
#undef	USE_REMOTE				/* There is no rcmd() */
#endif

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/fcntl.h>
#include <schily/ioctl.h>
#include <schily/errno.h>

#include <schily/schily.h>
#include <schily/standard.h>
/*#undef	HAVE_SYS_MTIO_H*/
#include <schily/mtio.h>
#include <schily/librmt.h>

LOCAL BOOL	help;
LOCAL BOOL	prvers;
LOCAL BOOL	wready;
LOCAL int	debug;

LOCAL struct mtop	mt_op;
LOCAL struct rmtget	mt_status;

#ifndef	HAVE_MTGET_TYPE
#ifdef	HAVE_MTGET_MODEL
#define	HAVE_MTGET_TYPE
#define	mt_type	mt_model
#endif
#endif

#define	NO_ASF		1000
#define	NO_NBSF		1001
#ifndef	MTASF
#	define	MTASF	NO_ASF
#endif
#ifndef	MTNBSF
#	define	MTNBSF	NO_NBSF
#endif

#define	MTC_NONE	0	/* No flags defined			*/
#define	MTC_RW		0	/* This command writes to the tape	*/
#define	MTC_RDO		1	/* This command does not write		*/
#define	MTC_CNT		2	/* This command uses the count arg	*/
#define	MTC_NDEL	4	/* Open the tape drive with O_NDELAY	*/


LOCAL struct mt_cmds {
	char *mtc_name;		/* The name of the command		*/
	char *mtc_text;		/* Description of the command		*/
	int mtc_opcode;		/* The opcode for mtio			*/
	int mtc_flags;		/* Flags for this command		*/
} cmds[] = {
#ifdef	MTWEOF
	{ "weof",	"write EOF mark",		MTWEOF,		MTC_RW|MTC_CNT },
	{ "eof",	"write EOF mark",		MTWEOF,		MTC_RW|MTC_CNT },
#endif
#ifdef	MTFSF
	{ "fsf",	"forward skip FILE mark",	MTFSF,		MTC_RDO|MTC_CNT },
#endif
#ifdef	MTBSF
	{ "bsf",	"backward skip FILE mark",	MTBSF,		MTC_RDO|MTC_CNT },
#endif
	{ "asf",	"absolute FILE mark pos",	MTASF,		MTC_RDO|MTC_CNT },
#ifdef	MTFSR
	{ "fsr",	"forward skip record",		MTFSR,		MTC_RDO|MTC_CNT },
#endif
#ifdef	MTBSR
	{ "bsr",	"backward skip record",		MTBSR,		MTC_RDO|MTC_CNT },
#endif
#ifdef	MTREW
	{ "rewind",	"rewind tape",			MTREW,		MTC_RDO },
#endif
#ifdef	MTOFFL
	{ "offline",	"rewind and unload",		MTOFFL,		MTC_RDO },
	{ "rewoffl",	"rewind and unload",		MTOFFL,		MTC_RDO },
#endif
#ifdef	MTNOP
	{ "status",	"get tape status",		MTNOP,		MTC_RDO },
#endif
	{ "nop",	"no operation",			MTNOP,		MTC_RDO },
#ifdef	MTRETEN
	{ "retension",	"retension tape cartridge",	MTRETEN,	MTC_RDO },
#endif
#ifdef	MTERASE
	{ "erase",	"erase tape",			MTERASE,	MTC_RW },
#endif
#ifdef	MTEOM
	{ "eom",	"position to EOM",		MTEOM,		MTC_RDO },
#endif

#if	MTNBSF != NO_NBSF
	{ "nbsf",	"backward skip FILE mark",	MTNBSF,		MTC_RDO|MTC_CNT },
#endif

#ifdef	MTLOAD
	{ "load",	"load tape",			MTLOAD,		MTC_RDO|MTC_NDEL },
#endif

	{ NULL, 	NULL,				0,		MTC_NONE }
};

LOCAL	void	usage		__PR((int ex));
EXPORT	int	main		__PR((int ac, char *av[]));
LOCAL	void	mtstatus	__PR((struct rmtget *sp));
LOCAL	char 	*print_key	__PR((Llong key));
LOCAL	int	openremote	__PR((char *tape));
LOCAL	int	opentape	__PR((char *tape, struct mt_cmds *cp));
LOCAL	int	mtioctl		__PR((int cmd, caddr_t arg));

LOCAL void
usage(ex)
	int	ex;
{
	struct mt_cmds	*cp;
	int		i;

	error("Usage: mt [ -f device ] [options] command [ count ]\n");
	error("Options:\n");
	error("\t-help\t\tprint this online help\n");
	error("\t-version\tprint version number\n");
	error("\t-wready\t\twait for the tape to become ready before doing command\n");
	error("\n");
	error("Commands are:\n");
	for (cp = cmds; cp->mtc_name != NULL; cp++) {
		error("%s%n", cp->mtc_name, &i);
		error("%*s%s\n", 14-i, "", cp->mtc_text);
	}
	exit(ex);
}

LOCAL char	opts[] = "f*,t*,version,help,h,debug,wready";

int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	* const *cav;
	char	*tape = NULL;
	char	*cmd = "BADCMD";
	int	count = 1;
	struct mt_cmds	*cp;

	save_args(ac, av);
	cac = --ac;
	cav = ++av;
	
	if (getallargs(&cac, &cav, opts,
			&tape, &tape,
			&prvers,
			&help, &help,
			&debug,
			&wready) < 0) {
		errmsgno(EX_BAD, "Bad Option: '%s'.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help) usage(0);
	if (prvers) {
		printf("mt %s (%s-%s-%s)\n\n", "1.28", HOST_CPU, HOST_VENDOR, HOST_OS);
		printf("Copyright (C) 2000-2009 Jörg Schilling\n");
		printf("This is free software; see the source for copying conditions.  There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}

	if (tape == NULL && (tape = getenv("TAPE")) == NULL) {
#ifdef	DEFTAPE
		tape = DEFTAPE;
#else
		errmsgno(EX_BAD, "No default tape defined.\n");
		usage(EX_BAD);
		/* NOTREACHED */
#endif
	}

	cac = ac;
	cav = av;
	if (getfiles(&cac, &cav, opts) == 0) {
		errmsgno(EX_BAD, "Missing args.\n");
		usage(EX_BAD);
	} else {
		cmd = cav[0];
		cav++;
		cac--;	
	}
	if (getfiles(&cac, &cav, opts) > 0) {
		if (*astoi(cav[0], &count) != '\0') {
			errmsgno(EX_BAD, "Not a number: '%s'.\n", cav[0]);
			usage(EX_BAD);
		}
		if (count < 0) {
			comerrno(EX_BAD, "negative file number or repeat count\n");
			/* NOTREACHED */
		}
		cav++;
		cac--;	
	}
	if (getfiles(&cac, &cav, opts) > 0) {
		errmsgno(EX_BAD, "Too many args.\n");
		usage(EX_BAD);
	}

	for (cp = cmds; cp->mtc_name != NULL; cp++) {
		if (strncmp(cmd, cp->mtc_name, strlen(cmd)) == 0)
			break;
	}
	if (cp->mtc_name == NULL) {
		comerrno(EX_BAD, "Unknown command: %s\n", cmd);
		/* NOTREACHED */
	}
#ifdef	DEBUG
	error("cmd: %s opcode: %d %s %s\n",
		cp->mtc_name, cp->mtc_opcode,
		(cp->mtc_flags & MTC_RDO) != 0 ? "RO":"RW",
		(cp->mtc_flags & MTC_CNT) != 0 ? "usecount":"");
#endif

	if ((cp->mtc_flags & MTC_CNT) == 0)
		count = 1;

#ifdef	USE_REMOTE
	rmtdebug(debug);
	(void)openremote(tape);		/* This needs super user privilleges */
#endif
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

	if (opentape(tape, cp) < 0) {
		if (geterrno() == EIO) {
			comerrno(EX_BAD, "'%s': no tape loaded or drive offline.\n",
				tape);
		} else if ((cp->mtc_flags & MTC_RDO) == 0 &&
			    geterrno() == EACCES) {
			comerr("Cannot open '%s': tape may be write protected.\n", tape);
		} else {
			comerr("Cannot open '%s'.\n", tape); 
		}
		/* NOTREACHED */
	}

#ifdef	DEBUG
	error("Tape: %s cmd : %s (%s) count: %d\n", tape, cmd, cp->mtc_name, count);
#endif

	if (cp->mtc_opcode == MTNOP) {
		if (strcmp(cp->mtc_name, "nop")) {
			/*
			 * Status ioctl
			 */
			if (mtioctl(MTIOCGET, (caddr_t)&mt_status) < 0) {
				comerr("Cannot get mt status from '%s'.\n", tape);
				/* NOTREACHED */
			}
			mtstatus(&mt_status);
		}
#if	MTASF == NO_ASF
	} else if (cp->mtc_opcode == MTASF) {
		(void)mtioctl(MTIOCGET, (caddr_t)&mt_status);
		if (mtioctl(MTIOCGET, (caddr_t)&mt_status) < 0) {
			comerr("Cannot get mt status from '%s'.\n", tape); 
			/* NOTREACHED */
		}
		/*
		 * If the device does not support to report the current file
		 * tape file position - rewind the tape, and space forward.
		 */
#ifndef	MTF_ASF
		if (1) {
#else
		if (!(mt_status.mt_flags & MTF_ASF) || MTNBSF == NO_NBSF) {
#endif
			mt_status.mt_fileno = 0;
			mt_op.mt_count = 1;
			mt_op.mt_op = MTREW;
			if (mtioctl(MTIOCTOP, (caddr_t)&mt_op) < 0) {
				comerr("%s %s %d failed\n", tape, cp->mtc_name,
							count);
				/* NOTREACHED */
			}
		}
		if (count < mt_status.mt_fileno) {
			mt_op.mt_op = MTNBSF;
			mt_op.mt_count =  mt_status.mt_fileno - count;
			/*printf("mt: bsf= %lld\n", (Llong)mt_op.mt_count);*/
		} else {
			mt_op.mt_op = MTFSF;
			mt_op.mt_count =  count - mt_status.mt_fileno;
			/*printf("mt: fsf= %lld\n", (Llong)mt_op.mt_count);*/
		}
		if (mtioctl(MTIOCTOP, (caddr_t)&mt_op) < 0) {
			if (mtioctl(MTIOCTOP, (caddr_t)&mt_op) < 0) {
				comerr("%s %s %d failed\n", tape, cp->mtc_name,
							count);
				/* NOTREACHED */
			}
		}
#endif
	} else {
		/*
		 * Regular magnetic tape ioctl
		 */
		mt_op.mt_op = cp->mtc_opcode;
		mt_op.mt_count = count;
		if (mtioctl(MTIOCTOP, (caddr_t)&mt_op) < 0) {
			comerr("%s %s %lld failed\n", tape, cp->mtc_name,
						(Llong)mt_op.mt_count);
			/* NOTREACHED */
		}
	}
	return (0);
}

/*
 * If we try to make this portable, we need a way to initialize it
 * in an OS independant way.
 * Don't use it for now.
 */
/*LOCAL */
struct tape_info {
	short	t_type;		/* type of magnetic tape device	*/
	char	*t_name;	/* name for prining		*/
	char	*t_dsbits;	/* "drive status" register	*/
	char	*t_erbits;	/* "error" register		*/
};
#ifdef	XXX
 tapes[] = {
	{ MT_ISTS,	"ts11",		0,		TSXS0_BITS },
	{ 0 }
};
#endif

/*
 * Interpret the status buffer returned
 */
LOCAL void
mtstatus(sp)
	register struct rmtget *sp;
{
	register struct tape_info *mt = NULL;

#ifdef	XXX
#ifdef	HAVE_MTGET_TYPE
	for (mt = tapes; mt->t_type; mt++)
		if (mt->t_type == sp->mt_type)
			break;
#endif
#endif

#if	defined(MTF_SCSI)

	if ((sp->mt_xflags & RMT_FLAGS) && (sp->mt_flags & MTF_SCSI)) {
		/*
		 * Handle SCSI tape drives specially.
		 */
		if (sp->mt_xflags & RMT_TYPE) {
			if (mt != NULL && mt->t_type == sp->mt_type)
				printf("%s tape drive:\n", mt->t_name);
			else
				printf("%s tape drive:\n", "SCSI");
		} else {
			printf("Unknown SCSI tape drive:\n");
		}
		printf("   sense key(0x%llx)= %s   residual= %lld   ",
			sp->mt_erreg, print_key(sp->mt_erreg), sp->mt_resid);
		printf("retries= %lld\n", sp->mt_dsreg);
	} else
#endif	/* MTF_SCSI */
		{
		/*
		 * Handle other drives below.
		 */
		if (sp->mt_xflags & RMT_TYPE) {
			if (mt == NULL || mt->t_type == 0) {
				printf("Unknown tape drive type (0x%llX):\n", (Ullong)sp->mt_type);
			} else {
				printf("%s tape drive:\n", mt->t_name);
			}
		} else {
			printf("Unknown tape drive:\n");
		}
		if (sp->mt_xflags & RMT_RESID)
			printf("   residual= %lld", sp->mt_resid);
		/*
		 * If we implement better support for specific OS,
		 * then we may want to implement something like the
		 * *BSD kernel %b printf format (e.g. printreg).
		 */
		if (sp->mt_xflags & RMT_DSREG)
			printf  ("   ds = %llX", (Ullong)sp->mt_dsreg);

		if (sp->mt_xflags & RMT_ERREG)
			printf  ("   er = %llX", sp->mt_erreg);
		putchar('\n');
	}
	printf("   file no= %lld   block no= %lld\n",
			(sp->mt_xflags & RMT_FILENO)?
				sp->mt_fileno:
				(Llong)-1,
			(sp->mt_xflags & RMT_BLKNO)?
				sp->mt_blkno:
				(Llong)-1);

	if (sp->mt_xflags & RMT_BF)
		printf("   optimum blocking factor= %ld\n", sp->mt_bf);

	if (sp->mt_xflags & RMT_FLAGS)
		printf("   flags= 0x%llX\n", sp->mt_flags);
}

static char *sense_keys[] = {
	"No Additional Sense",		/* 0x00 */
	"Recovered Error",		/* 0x01 */
	"Not Ready",			/* 0x02 */
	"Medium Error",			/* 0x03 */
	"Hardware Error",		/* 0x04 */
	"Illegal Request",		/* 0x05 */
	"Unit Attention",		/* 0x06 */
	"Data Protect",			/* 0x07 */
	"Blank Check",			/* 0x08 */
	"Vendor Unique",		/* 0x09 */
	"Copy Aborted",			/* 0x0a */
	"Aborted Command",		/* 0x0b */
	"Equal",			/* 0x0c */
	"Volume Overflow",		/* 0x0d */
	"Miscompare",			/* 0x0e */
	"Reserved"			/* 0x0f */
};

LOCAL char *
print_key(key)
	Llong	key;
{
	static	char keys[32];

	if (key >= 0 && key < (sizeof(sense_keys)/sizeof(sense_keys[0])))
		return (sense_keys[key]);
	js_snprintf(keys, sizeof(keys), "Unknown Key: %lld", key);
	return (keys);
}

/*--------------------------------------------------------------------------*/
LOCAL int	isremote;
LOCAL int	remfd	= -1;
LOCAL int	mtfd;
LOCAL char	*remfn;

#ifdef	USE_REMOTE
LOCAL int
openremote(tape)
	char	*tape;
{
	char	host[128];

	if ((remfn = rmtfilename(tape)) != NULL) {
		rmthostname(host, sizeof(host), tape);
		isremote++;

		if (debug)
			errmsgno(EX_BAD, "Remote: %s Host: %s file: %s\n",
							tape, host, remfn);

		if ((remfd = rmtgetconn(host, 4096, 0)) < 0)
			comerrno(EX_BAD, "Cannot get connection to '%s'.\n",
				/* errno not valid !! */		host);
	}
	return (isremote);
}
#endif

LOCAL int
opentape(tape, cp)
		char		*tape;
	register struct mt_cmds *cp;
{
	int	ret;
	int	n = 0;
	int	oflag;

	oflag = (cp->mtc_flags & MTC_RDO) ? O_RDONLY : O_RDWR;
#ifdef	O_NDELAY
	if (cp->mtc_flags & MTC_NDEL)
		oflag |= O_NDELAY;
#endif

retry:
	ret = 0;
	if (isremote) {
#ifdef	USE_REMOTE
		if (rmtopen(remfd, remfn, oflag) < 0)
			ret = -1;
#else
		comerrno(EX_BAD, "Remote tape support not present.\n");
#endif
	} else if ((mtfd = open(tape, oflag)) < 0) {
			ret = -1;
	}
	if (wready && n++ < 120 && (geterrno() == EIO || geterrno() == EBUSY)) {
		sleep(1);
		goto retry;
	}
	return (ret);
}

LOCAL int
mtioctl(cmd, arg)
	int	cmd;
	caddr_t	arg;
{
	int	ret = -1;
	struct rmtget *mtp;
	struct mtop *mop;

	if (isremote) {
#ifdef	USE_REMOTE
		switch (cmd) {

		case MTIOCGET:
			ret = rmtxstatus(remfd, (struct rmtget *)arg);
			if (ret < 0)
				return (ret);

			mtp = (struct rmtget *)arg;
/*#define	DEBUG*/
#ifdef	DEBUG
error("type: %llX ds: %llX er: %llX resid: %lld fileno: %lld blkno: %lld flags: %llX bf: %ld\n",
mtp->mt_type, mtp->mt_dsreg, mtp->mt_erreg, mtp->mt_resid, mtp->mt_fileno,
mtp->mt_blkno, mtp->mt_flags, mtp->mt_bf);
#endif
			break;
		case MTIOCTOP:
			mop = (struct mtop *)arg;
			ret = rmtioctl(remfd, mop->mt_op, mop->mt_count);
			break;
		default:
			comerrno(ENOTTY, "Invalid mtioctl.\n");
			/* NOTREACHED */
		}
#else
		comerrno(EX_BAD, "Remote tape support not present.\n");
#endif
	} else {
#ifdef	HAVE_IOCTL
		if (cmd == MTIOCGET) {
			struct mtget mtget;

			ret = ioctl(mtfd, cmd, &mtget);
			if (ret >= 0) {
				if (_mtg2rmtg((struct rmtget *)arg, &mtget) < 0)
					ret = -1;
			}
		} else
		ret = ioctl(mtfd, cmd, arg);
#else
		ret = -1;
#ifdef	ENOSYS
		seterrno(ENOSYS);
#else
		seterrno(EINVAL);
#endif
#endif
	}
	return (ret);
}
