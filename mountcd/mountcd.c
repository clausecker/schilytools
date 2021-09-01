/* @(#)mountcd.c	1.14 21/08/20 Copyright 2005-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)mountcd.c	1.14 21/08/20 Copyright 2005-2021 J. Schilling";
#endif
/*
 *	Mount the "right" CD medium from a list of devices,
 *	use ISO-9660 volume ID to identify the correct device.
 *
 *	Copyright (c) 2005-2021 J. Schilling
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
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/fcntl.h>
#include <schily/dirent.h>
#include <schily/string.h>
#include <schily/stat.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/errno.h>
#include <schily/nlsdefs.h>

BOOL	debug = FALSE;

LOCAL	void	usage		__PR((int excode));
EXPORT	int	main		__PR((int ac, char *av[]));
LOCAL	int	trymount	__PR((char *name, char *volid));

LOCAL void
usage(excode)
	int	excode;
{
	error("Usage:	mountcd [options] [device path ...]\n");
	error("Options:\n");
	error("	-help		Print this help.\n");
	error("	-version	Print version info and exit.\n");
	error("	-debug		Print additional debug info.\n");
	error("	V=volid		Only mount a CD that matches the Volume ID 'volid'.\n");
	exit(excode);
}


EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac = ac;
	char	* const *cav = av;
	char	*options = "help,version,debug,V*";
	BOOL	help = FALSE;
	BOOL	pversion = FALSE;
	char	*volid = NULL;
	DIR	*dp;
	struct dirent *de;
	char	*p;
	int	i;

	save_args(ac, av);

	(void) setlocale(LC_ALL, "");

#ifdef  USE_NLS
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "mountcd"	/* Use this only if it weren't */
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

	cac--; cav++;
	if (getallargs(&cac, &cav, options, &help, &pversion, &debug, &volid) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (pversion) {
		printf("Mountcd release %s (%s-%s-%s) Copyright (C) 2005-2021 %s\n",
				"1.14", HOST_CPU, HOST_VENDOR, HOST_OS,
				_("Jörg Schilling"));
		exit(0);
	}
	if (debug) {
		error ("Looking for CD Volume ID '%s'.\n",
			volid ? volid:"<NO-Volid specified>");
	}
	cac = ac;
	cav = av;
	cac--; cav++;
	for (i = 0; getfiles(&cac, &cav, options); cac--, cav++, i++) {
		trymount(cav[0], volid);
	}

	if (i == 0) {
		dp = opendir("/dev/dsk");
		if (dp == NULL)
			exit(EX_BAD);
		while ((de = readdir(dp)) != NULL) {
#ifdef	__sparc
			if ((p = strstr(de->d_name, "s2")) != NULL && p[2] == '\0') {
#else
			if ((p = strstr(de->d_name, "p0")) != NULL && p[2] == '\0') {
#endif
				char	pname[225];

				snprintf(pname, sizeof (pname), "/dev/dsk/%s", de->d_name);

				if (debug)
					error("-->name %s\n", pname);
				trymount(pname, volid);
			}
		}
	}

	return (0);
}

#include <schily/types.h>
#ifdef	HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef	HAVE_SYS_MNTTAB_H
#include <sys/mnttab.h>
#endif
#ifdef	HAVE_SYS_MNTENT_H
#include <sys/mntent.h>
#endif

LOCAL int
trymount(name, volid)
	char	*name;
	char	*volid;
{
	char	buf[2048];
	int	len = 0;
	int	f;
	struct stat sb;

	f = open(name, O_RDONLY);
	if (f < 0) {
		if (debug)
			errmsg("Cannot open '%s'.\n", name);
		return (-1);
	}
	if (volid != NULL)
		len = strlen(volid);

	lseek(f, 16 * sizeof (buf), SEEK_SET);
	fillbytes(buf, sizeof (buf), '\0');
	if (read(f, buf, sizeof (buf)) < 0) {
		if (debug)
			errmsg("Cannot read '%s'.\n", name);
		close(f);
		return (-1);
	}
	if (strncmp(&buf[1], "CD001", 5) == 0) {
		if (debug) {
			error("At '%s':\n", name);
			error("found CD Volume ID: '%.32s'.\n", &buf[40]);
		}
		if (volid != NULL && strncmp(&buf[40], volid, len) != 0) {
			close(f);
			return (0);
		}
		if (debug)
			error("String begin match for CD Volume ID for '%s'.\n", name);
		if (buf[40+len] != ' ' && buf[40+len] != '\0') {
			close(f);
			return (0);
		}
		if (debug)
			error("Full match for CD Volume ID, trying to mount '%s'.\n", name);

		if (stat("/.cdrom", &sb) == -1 || !S_ISDIR(sb.st_mode)) {
			unlink("/.cdrom");
			if (mkdir("/.cdrom", 0755) < 0) {
				errmsg("Cannot make directory '%s'.\n", "/.cdrom");
				close(f);
				return (-1);
			}
		}
		strcpy(buf, "ro");
#ifdef	MS_OPTIONSTR
		if (mount(name, "/.cdrom", MS_RDONLY|MS_OPTIONSTR, "hsfs", NULL, 0, buf, 64) < 0) {
			errmsg("Cannot mount '%s' on '%s'.\n", name, "/.cdrom");
			exit(geterrno());
		}
#else
		comerrno(EX_BAD, "mountcd not implemented on this platform.\n");
#endif
		exit(0);
	}
	close(f);
	return (0);
}
