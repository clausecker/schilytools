/* @(#)label.c	1.35 21/08/20 Copyright 1988-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)label.c	1.35 21/08/20 Copyright 1988-2021 J. Schilling";
#endif
/*
 *	Copyright (c) 1988-2021 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/param.h>	/* Include various defs needed with some OS */
#include <schily/ioctl.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/nlsdefs.h>
#include "dsklabel.h"
#include "fmt.h"

struct disk	cur_disk;
struct	dk_label x_label;
struct  dk_label *d_label = &x_label;
	char	*disknm;

char	opt[] = "help,version,print,p,Prpart,P,set,s,if*,lproto*,of*,lname*,f*";

extern char	*Lname;
extern char	*Lproto;

LOCAL	void	usage		__PR((int ret));
EXPORT	int	main		__PR((int, char **));
EXPORT	void	disk_null	__PR((struct disk *dp, int init));

void
get_lgeom_defaults(scgp, dp)
	SCSI	*scgp;
	struct disk	*dp;
{
}

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage: label [options]\n");
	error("Options:\n");
	error("	-help		Print this help.\n");
	error("	-version	Print version number.\n");
	error("\t-print,-p\tonly print label\n");
	error("\t-set,-s\t\tset driver tables from disk label\n");
	error("\tof=name\t\tname of output Label (default: '%s')\n", Lname);
	error("\tlname=name\tname of output Label (default: '%s')\n", Lname);
	error("\tif=name\t\tname of input  Label (default: '%s')\n", Lproto);
	error("\tlproto=name\tname of input  Label (default: '%s')\n", Lproto);
	error("\tf=name\tname of input & output  Label\n");
	exit(ret);
}

int
main(ac, av)
	int	ac;
	char	*av[];
{
	BOOL	help	= FALSE;
	BOOL	prversion = FALSE;
	int	print	= 0;
	int	Prpart	= 0;
	int	set	= 0;
	int	cac	= ac;
	char	*const *cav= av;
	SCSI	scg;

	save_args(ac, av);

	(void) setlocale(LC_ALL, "");

#ifdef  USE_NLS
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "p"		/* Use this only if it weren't */
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

	disk_null(&cur_disk, TRUE);

	cac--;
	cav++;	
	if (getallargs(&cac, &cav, opt, &help, &prversion,
						&print, &print,
						&Prpart, &Prpart,
						&set, &set,
						&Lproto, &Lproto,
						&Lname, &Lname,
						&disknm) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prversion) {
		gtprintf("Label release %s (%s-%s-%s) Copyright (C) 1988-2021 %s\n",
				"1.35",
				HOST_CPU, HOST_VENDOR, HOST_OS,
				_("Jörg Schilling"));
		exit(0);
	}

	if (disknm)
		Lname = Lproto = disknm;

	if (print || Prpart || set) {
		if (readlabel(Lproto, d_label)) {
			cur_disk.labelread = 0;

			if (Prpart) {
				prpartab(stdout, "UNKNOWN", d_label);
			} else {
				printlabel(d_label);
				checklabel(&cur_disk, d_label, 0);
			}
			if (!setval_from_label(&cur_disk, d_label))
				comerrno(EX_BAD, "Bad Label '%s'\n", Lproto);
			if (set)
				exit(setlabel(Lname, d_label));
			exit(0);
		}
		exit(-1);
	}

	if (readlabel(Lproto, d_label)) {
		cur_disk.labelread = 0;
		if (setval_from_label(&cur_disk, d_label))
			cur_disk.labelread = 1;
	}
	do {
		getlong("Enter number of physical cylinders",
						&cur_disk.pcyl, 1L, 0xFFFE);
						cur_disk.lpcyl = cur_disk.pcyl;
		getlong("Enter number of data cylinders",
						&cur_disk.lncyl, 1L, 0xFFFE);
		getlong("Enter number of alternate cylinders (Label)",
						&cur_disk.lacyl,0L, 10000);
		getlong("Enter number of data      heads     (Label)",
						&cur_disk.lhead, 1L, 100);
		getlong("Enter number of data sectors/track  (Label)",
						&cur_disk.lspt, 1L, 0xFFFE);
		getlong("Enter rpm of drive",
						&cur_disk.rpm, 0L, 10000L);

		makelabel(&scg, &cur_disk, d_label);
		printlabel(d_label);
		checklabel(&cur_disk, d_label, 1);
	} while(!yes("Use this label? "));
	/*
	 * fill in default vtmap if not done yet
	 */
	check_vtmap(d_label, 1);
	writelabel(Lname, d_label);
	return (0);
}

EXPORT void
disk_null(dp, init)
	struct disk	*dp;
	int		init;
{
	register int i = sizeof(struct disk);
	register char *p = (char *)dp;

	while (--i >= 0) {
		*p++ = -1;
	}
}
