/* @(#)maptodisk.c	1.28 08/12/22 Copyright 1991-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)maptodisk.c	1.28 08/12/22 Copyright 1991-2008 J. Schilling";
#endif
/*
 *	Routines to map SCSI targets to logical disk names
 *
 *	Copyright (c) 1991-2008 J. Schilling
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

/*
 *	Wenn unterschiedliche SCSI Kontroller zugelassen werden sollen,
 *	dann muﬂ im scg Treiber der DKIOGCONF ioctl eingebaut sein.
 *
 *	XXX #ifdef HAVE_DKIO ist vorerst nur ein Hack
 *
 */
#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/fcntl.h>
#include <schily/errno.h>
#include <schily/stat.h>
#include <schily/ioctl.h>
#include "dsklabel.h"
#include <dirent.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/libport.h>
#include <ctype.h>
#ifndef	SVR4
#ifdef	sun
#include <sun3/cpu.h>	/* Tempor‰r f¸r alte Maschinen */
#endif
#endif
#include "fmt.h"
#include "map.h"

#define	TARGET(slave)	((slave >> 3) & 07)
#define	LUN(slave)	(slave & 07)

#define	MAX_SCG	8

typedef struct scsi_devices {
	char	lun[8];
}scdev;

#define	NDISKNAMES	64

LOCAL	scdev	diskmap[8][8];
LOCAL	char	*disknames[NDISKNAMES];
LOCAL	char	*diskdevnames[NDISKNAMES];
LOCAL	scgdrv	scgmap[MAX_SCG];
LOCAL	int	scgmap_known = FALSE;
LOCAL	BOOL	diskmap_init = FALSE;
LOCAL	BOOL	scgmap_init = FALSE;

LOCAL	void	init_diskmap	__PR((void));
LOCAL	void	diskmap_null	__PR((void));
LOCAL	void	scgmap_null	__PR((void));
LOCAL	BOOL	have_diskname	__PR((char *));
#ifdef	SVR4
LOCAL	BOOL	is_svr4_disk	__PR((char *));
#else
LOCAL	BOOL	is_4x_disk	__PR((char *));
#endif
#ifdef	SVR4
LOCAL	void	make_svr4_dname	__PR((char *, char *));
#else
LOCAL	void	make_dname	__PR((char *, char *));
#endif
LOCAL	BOOL	make_disknames	__PR((char *, char *, char *));
LOCAL	int	opendisk	__PR((char *));
LOCAL	void	map_alldisks	__PR((void));
#ifdef	HAVE_DKIO
LOCAL	BOOL	map_disk	__PR((int, int, char *, char *));
LOCAL	int	scg_mapbus	__PR((struct dk_conf *));
#endif	/* HAVE_DKIO */
LOCAL	void	map_scg		__PR((void));

extern	int	nomap;
extern	int	debug;

EXPORT int
maptodisk(scsibus, target, lun)
	int	scsibus;
	int	target;
	int	lun;
{
	if (nomap)
		return (-1);

	if (!diskmap_init)
		init_diskmap();

	if (scsibus < 0 || target < 0 || lun < 0 ||
				scsibus >= MAX_SCG || target >= 8 || lun >= 8)
		return (-1);

	if (debug)
		printf("maptodisk(%d, %d, %d) = %d\n",
					scsibus, target, lun,
					diskmap[scsibus][target].lun[lun]);
	return (diskmap[scsibus][target].lun[lun]);
}

EXPORT scgdrv	*
scg_getdrv(scsibus)
	int	scsibus;
{
	return (&scgmap[scsibus]);
}

EXPORT char *
diskname(diskno)
	int	diskno;
{
	return (disknames[diskno]);
}

EXPORT char *
diskdevname(diskno)
	int	diskno;
{
	return (diskdevnames[diskno]);
}

EXPORT int
print_disknames(scsibus, target, lun)
	int	scsibus;
	int	target;
	int	lun;
{
	int	printed = 0;
	int	dnum;
	int	i;
	int	ccnt	= 0;

	if (nomap)
		return (0);

	if (lun >= 0) {
		dnum = maptodisk(scsibus, target, lun);
		if (dnum >= 0)
			ccnt += printf("%s", disknames[dnum]);
		return (ccnt);
	}
	for (i = 0; i < 8; i++) {
		dnum = maptodisk(scsibus, target, i);
		if (dnum >= 0) {
			ccnt += printf("%s%s", printed?",":"", disknames[dnum]);
			printed++;
		}
	}
	return (ccnt);
}

#ifdef	needed
/*
 * No more needed. Now part of libscg.
 */
EXPORT int
scg_initiator_id(scsibus)
	int	scsibus;
{
	short	id;

	if (!scgmap_init)
		map_scg();
	if ((id = scgmap[scsibus].scg_slave) != -1)
		id = TARGET(id);
	return (id);
}
#endif

LOCAL void
init_diskmap()
{
	if (!scgmap_init)
		map_scg();
	map_alldisks();
}

LOCAL void
diskmap_null()
{
	register int	i;
	register int	j;
	register int	k;
	register scdev	*sp;

	for (i = 8; --i >= 0; ) {
		for (j = 8; --j >= 0; ) {
			sp = &diskmap[i][j];
			for (k = 8; --k >= 0; ) {
				sp->lun[k] = -1;
			}
		}
	}
}

LOCAL void
scgmap_null()
{
	register int	i;

	for (i = MAX_SCG; --i >= 0; ) {
		scgmap[i].scg_cname[0] = 0;
		scgmap[i].scg_caddr = 0;
		scgmap[i].scg_cunit = -1;
		scgmap[i].scg_slave = -1;
	}
}

LOCAL BOOL
have_diskname(name)
	char	*name;
{
	int	i;

	for (i = 0; i < NDISKNAMES; i++) {
		if (disknames[i] && streql(disknames[i], name))
			return (TRUE);
	}
	return (FALSE);
}

#define	check_if(s, c)	if (*s++ != c) return (FALSE);
#define	skip_digits(s)	while (isdigit(*s)) s++;

#ifdef	SVR4

LOCAL BOOL
is_svr4_disk(s)
	char	*s;
{
	Uchar	*s1 = (Uchar *)s;
	char	*s2;

	if ((s2 = strrchr((char *)s1, '/')) != NULL)
		s1 = (Uchar *)++s2;
	check_if(s1, 'c');
	skip_digits(s1);
	if (*s1 == 't') {
		s1++;
		skip_digits(s1);
	}
	check_if(s1, 'd');
	skip_digits(s1);
	check_if(s1, 's');
	skip_digits(s1);
	return (*s1 == '\0');
}

LOCAL void
make_svr4_dname(to, from)
	char	*to;
	char	*from;
{
	char	*p;

	if ((p = strchr(from, 'c')) != NULL) {
		strcpy(to, p);
		if ((p = strrchr(to, 's')) != NULL)
			*p = '\0';
	} else {
		*to = '\0';
	}
}

LOCAL BOOL
make_disknames(name, dname, cdname)
	char	*name;
	char	*dname;
	char	*cdname;
{
	if (is_svr4_disk(name)) {
		make_svr4_dname(cdname, name);
		if (have_diskname(cdname)) {
			return (FALSE);
		}
		sprintf(dname, "/dev/rdsk/%s", name);
	} else {
		sprintf(dname, "/dev/rdsk/%s", name);
		strcpy(cdname, name);
	}
	return (TRUE);
}

#else	/* !SVR4 */

LOCAL BOOL
is_4x_disk(s)
	char	*s;
{
	Uchar	*s1 = (Uchar *)s;
	char	*s2;

	if ((s2 = strrchr((char *)s1, '/')) != NULL)
		s1 = (Uchar *)++s2;
	if (!strstr((char *)s1, "rsd"))
		return (FALSE);
	s1 += 3;
	skip_digits(s1);
	if (!strchr("abcdefgh", *s1))
		return (FALSE);
	return (*++s1 == '\0');
}

LOCAL void
make_dname(to, from)
	char	*to;
	char	*from;
{
	char	*p;

	if ((p = strchr(from, 'r')) != NULL) {
		p++;
		strcpy(to, p);
		to[strlen(to)-1] = '\0';
	} else {
		*to = '\0';
	}
}

LOCAL BOOL
make_disknames(name, dname, cdname)
	char	*name;
	char	*dname;
	char	*cdname;
{
	if (!is_4x_disk(name))
		return (FALSE);

	make_dname(cdname, name);
	if (have_diskname(cdname)) {
		return (FALSE);
	}
	sprintf(dname, "/dev/%s", name);
	return (TRUE);
}
#endif

#ifdef	SVR4
#define	is_disk(n)	is_svr4_disk(n)
#else
#define	is_disk(n)	is_4x_disk(n)
#endif

LOCAL int
opendisk(dname)
	char	*dname;
{
	struct stat	sb;
	register int	c = -1;
	register int	nlen = 0;
	int		f;

	f = -1;
	/*
	 * we always try to open 'c' partition (slice 2) first
	 * XXX This only works on sun's with NDKMAP <= 10 and
	 * XXX slice 2 beeing the whole disk!
	 */
	if (is_disk(dname)) {
		nlen = strlen(dname);
		c = dname[nlen-1];
		dname[nlen-1] = PARTOFF+2;
	}
	if (stat(dname, &sb) < 0 ||
				((f = open(dname, O_RDONLY|O_NDELAY)) < 0 &&
							geterrno() == ENOENT)) {
		/*
		 * It did'nt work try to open the original dname
		 */
		if (c >= 0) {
			dname[nlen-1] = c;
			if (stat(dname, &sb) < 0)
				return (f);
			f = open(dname, O_RDONLY|O_NDELAY);
		}
	}
	return (f);
}

LOCAL void
map_alldisks()
{
#ifdef	HAVE_DKIO
	char		dname[64];
	char		cdname[64];
	register int	i;
	int		f;
	DIR		*d;
	struct dirent	*dp;
#endif	/* HAVE_DKIO */

	diskmap_null();
	diskmap_init = TRUE;

#ifdef	HAVE_DKIO
#ifdef	SVR4
	d = opendir("/dev/rdsk/");
	if (d == 0) {
		errmsg("Cannot open '/dev/rdsk'\n");
		return;
	}
#else
	d = opendir("/dev/");
	if (d == 0) {
		errmsg("Cannot open '/dev'\n");
		return;
	}
#endif

	i = -1;
	while ((dp = readdir(d)) != 0) {
		if (dp->d_name[0] == '.' && (dp->d_name[1] == '\0' ||
			(dp->d_name[1] == '.' && dp->d_name[2] == '\0')))
			continue;

		if (!make_disknames(dp->d_name, dname, cdname))
			continue;

		if (++i >= NDISKNAMES) {
			errmsgno(EX_BAD,
				"WARNING: Too many disks. Cannot map %s\n",
				cdname);
			break;
		}
		f = opendisk(dname);
		if (f < 0) {
			if (geterrno() == ENXIO) {
				/*
				 * The device node exists!
				 * Store the name to avoid looking up all other
				 * partitions.
				 */
				disknames[i] = permstring(cdname);
				diskdevnames[i] = permstring(dname);
			} else {
				--i;
			}
			continue;
		}
		if (!map_disk(f, i, dname, cdname))
			--i;
		close(f);
	}
	closedir(d);
#endif	/* HAVE_DKIO */
}

#ifdef	HAVE_DKIO
LOCAL BOOL
map_disk(f, dnum, dname, cdname)
	int	f;
	int	dnum;
	char	*dname;
	char	*cdname;
{
	struct dk_conf  conf;
	int		scg_bus;

	if (ioctl(f, DKIOCGCONF, &conf) < 0) {
		errmsg("DKIOGCONF on disk '%s'\n", dname);
		return (FALSE);
	}
	scg_bus = scg_mapbus(&conf);
	if (scg_bus == -1) {
		errmsgno(-1, "No scg driver for disk '%s' on %s%d\n",
				dname, conf.dkc_cname, conf.dkc_cnum);
		return (FALSE);
	}
	if (diskmap[scg_bus][TARGET(conf.dkc_slave)].
					lun[LUN(conf.dkc_slave)] != -1)
		errmsgno(EX_BAD, "Double diskmap entry: '%s'\n", dname);

	diskmap[scg_bus][TARGET(conf.dkc_slave)].
					lun[LUN(conf.dkc_slave)] = dnum;
	if (disknames[dnum]) {
		errmsgno(EX_BAD, "Double diskname entry: '%s'\n", cdname);
	} else {
		disknames[dnum] = permstring(cdname);
		diskdevnames[dnum] = permstring(dname);
	}
	return (TRUE);
}

LOCAL int
scg_mapbus(conf)
	struct dk_conf  *conf;
{
	register int	i;

	/*
	 * Stimmt dann, wenn nur ein Typ SCSI Kontroller vorhanden ist.
	 */
	if (!scgmap_known)
		return (conf->dkc_cnum);

	for (i = 0; i < MAX_SCG; i++) {
		if (conf->dkc_cnum == scgmap[i].scg_cunit &&
			conf->dkc_addr == scgmap[i].scg_caddr &&
				streql(conf->dkc_cname, scgmap[i].scg_cname))
			return (i);
	}
	return (-1);
}
#endif	/* HAVE_DKIO */

LOCAL void
map_scg()
{
#ifdef	HAVE_DKIO
	struct dk_conf  conf;
	char		dname[64];
	register int	i;
	int		f;
#ifndef	SVR4
	int		cpu_type = gethostid() >> 24;
#endif
#endif	/* HAVE_DKIO */


	scgmap_null();
	scgmap_init = TRUE;
#ifdef	HAVE_DKIO
	for (i = 0; i < MAX_SCG; i++) {
		sprintf(dname, "/dev/scg%d", i);
		if ((f = open(dname, 0)) < 0)
			continue;

		if (ioctl(f, DKIOCGCONF, &conf) < 0) {
#ifndef	SVR4
#ifdef	sun
			if (cpu_type != CPU_SUN3_50 &&
				cpu_type != CPU_SUN3_60)
#endif
#endif
				errmsg("DKIOGCONF not supported by '%s'.\n\
\nUpgrade Kernel for proper logical disk mapping.\n\n", dname);
			return;
		}
		scgmap_known = TRUE;
		strcpy(scgmap[i].scg_cname, conf.dkc_cname);
		scgmap[i].scg_caddr =  conf.dkc_addr;
		scgmap[i].scg_cunit =  conf.dkc_cnum;
		scgmap[i].scg_slave =  conf.dkc_slave;
		close(f);

		printf("scg%d at %s%d initiator id %d\n",
			i,
			conf.dkc_cname, conf.dkc_cnum,
			conf.dkc_slave == -1 ? -1 : conf.dkc_slave/8);
	}
#endif	/* HAVE_DKIO */
}
