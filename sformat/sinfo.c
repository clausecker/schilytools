/* @(#)sinfo.c	1.35 06/09/13 Copyright 1988-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)sinfo.c	1.35 06/09/13 Copyright 1988-2004 J. Schilling";
#endif
/*
 *	Copyright (c) 1988-2004 J. Schilling
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
#ifdef	HAVE_SYS_PARAM_H
#include <sys/param.h>	/* Include various defs needed with some OS */
#endif
#include <stdio.h>
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/time.h>
#include <schily/schily.h>
#include <schily/libport.h>

/*#include <scg/scgcmd.h>*/

#include "fmt.h"

#include <scg/scsireg.h>
#include <scg/scsidefs.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"

LOCAL long	sibuf[8*1024/sizeof (long)];

extern	char	*Sbuf;
extern	long	Sbufsize;

/*
 * The next two structures always contain
 * Mototola / network byte order.
 */
struct si_stamp {
	u_long	si_xtime;	/* Reserve for year > 2038 */
	u_long	si_time;	/* 'sformat' format time */
	char	si_serial[16];	/* Hardware serial */
	char	si_uname[16];	/* user name */
	char	si_hname[64];	/* host name */
	char	si_dname[64];	/* domain name */
};

struct sinfo {
	u_long	si_id;		/* some id */
	u_long	si_magic;	/* The magic to see if release >= 3.0 */
	u_long	si_release;	/* The release (for later enhancements) */
	u_long	si_reformats;	/* # of times the disk has been reformatted */
	u_short	si_ncyl;	/* # of cyls (may be we need this for dos) */
	u_short	si_spt;		/* # of sectors/track (see above) */
	u_char	si_nhead;	/* # of heads (may be we need this for dos) */
	u_char	si_cres[3];	/* Fill up to long boundary */
	u_long	si_res[2];	/* Reserved for later enhancements */
	struct si_stamp si_last; /* The data for the last format on this disk */
	struct si_stamp si_first; /* Data for the first sformat on this disk */
};

#define	SI_MAGIC	0x4A534348
#define	SI_RELEASE	3

extern	int	autoformat;

LOCAL	u_long	sinfo_chksum	__PR((SCSI *scgp, u_long *));
LOCAL	void	fill_blk	__PR((SCSI *scgp, u_long *, u_long));
LOCAL	void	fill_sinfo	__PR((SCSI *scgp, struct sinfo *sinfo));
LOCAL	void	fill_stamp	__PR((struct si_stamp *stamp));
LOCAL	void	print_stamp	__PR((FILE *f, const char *fmt, struct si_stamp *stamp));
LOCAL	void	sinfo_geom	__PR((SCSI *scgp, struct disk *, long *, long *, long *));
EXPORT	void	print_sinfo	__PR((FILE *, SCSI *scgp));
EXPORT	BOOL	read_sinfo	__PR((SCSI *scgp, struct disk *, BOOL));
EXPORT	BOOL	write_sinfo	__PR((SCSI *scgp, struct disk *));

/*
void
print_bk(dp, n)
	struct disk	*dp;
	long	n;
{
	long	cy;
	long	hd;
	long	se;
	long	sx;
	long	xx;

	sx = dp->spt;
	if (dp->tpz != 0)
		sx -= dp->aspz/dp->tpz;
	xx = sx * dp->nhead;
	cy = 1 + n / xx;
	hd = n % xx;
	se = hd % sx;
	hd /= sx;

	printf("Cyl: %ld Head: %ld Sec: %ld\n", cy, hd, se);
}
*/

LOCAL u_long
sinfo_chksum(scgp, lp)
	SCSI		*scgp;
	register u_long	*lp;
{
	register u_long	chksum;
	register int	i;

	chksum = 0L;
	for (i = scgp->cap->c_bsize/sizeof (u_long) - 1; --i >= 0; ) {
		chksum ^= *lp++;
	}
	return (chksum);
}

LOCAL void
fill_blk(scgp, blk, v)
	SCSI		*scgp;
		u_long	*blk;
	register u_long	v;
{
	register u_long	*lp;
	register int	i;

	lp = blk;
	srand((int)v);
	for (i = scgp->cap->c_bsize/sizeof (u_long) - 1; --i >= 0; ) {
		*lp++ = v;
		v = rand();
	}
}

LOCAL void
fill_sinfo(scgp, sinfo)
	SCSI		*scgp;
	struct sinfo	*sinfo;
{
/*	print_sinfo(stdout, scgp);*/
	fill_blk(scgp, (u_long *)sinfo, 0);
	sinfo->si_magic = SI_MAGIC;
	sinfo->si_release = SI_RELEASE;
	sinfo->si_reformats = 0;

	sinfo->si_ncyl  = 0;
	sinfo->si_spt   = 0;
	sinfo->si_nhead = 0;

	fill_stamp(&sinfo->si_first);
	fill_stamp(&sinfo->si_last);

	((u_long *)sibuf)[scgp->cap->c_bsize/sizeof (u_long) - 1] =
					sinfo_chksum(scgp, (u_long *)sinfo);
}

LOCAL void
fill_stamp(stamp)
	struct si_stamp	*stamp;
{
	struct	timeval	tv;
	char	*uname;

	gettimeofday(&tv, (struct timezone *)0);
	stamp->si_time = tv.tv_sec;
	stamp->si_xtime = 0;

	sprintf(stamp->si_serial, "%lX", (unsigned long)gethostid());

	uname = getlogin();
	if (uname == NULL)
		uname = getenv("USER");
	stamp->si_uname[0] = '\0';
	if (uname != NULL)
		strncpy(stamp->si_uname, uname, sizeof (stamp->si_uname));
	stamp->si_uname[sizeof (stamp->si_uname)-1] = '\0';

	gethostname(stamp->si_hname, sizeof (stamp->si_hname)-1);
	stamp->si_hname[sizeof (stamp->si_hname)-1] = '\0';

	getdomainname(stamp->si_dname, sizeof (stamp->si_dname)-1);
	stamp->si_dname[sizeof (stamp->si_dname)-1] = '\0';
}

LOCAL void
print_stamp(f, fmt, stamp)
	FILE		*f;
	const	char	*fmt;
	struct si_stamp	*stamp;
{
	fprintf(f, "%s formatted with sformat id %s by:\n\t%s@%s%s%s on %s",
			fmt,
			stamp->si_serial,
			stamp->si_uname, stamp->si_hname,
			stamp->si_dname[0] == '.' ? "":".", stamp->si_dname,
			asctime(localtime((time_t *)&stamp->si_time)));
}

LOCAL void
sinfo_geom(scgp, dp, sptp, aspzp, tpzp)
	SCSI		*scgp;
	struct 	disk	*dp;
	long	*sptp;	/* Sectors/Track */
	long	*aspzp;	/* Alternate Sectors/Zone */
	long	*tpzp;	/* Tracks/Zone */
{
	if (dp->spt < 0 || dp->aspz < 0 || dp->tpz < 0) {
		scgp->silent++;
		dp->formatted++;
		get_defaults(scgp, dp);
		dp->formatted--;
		scgp->silent--;
	}
	*sptp  = dp->spt;
	*aspzp = dp->aspz;
	*tpzp  = dp->tpz;
	/*
	 * Wenn tpz == 0 (die ganze Platte ist eine Zone), dann wird
	 * aspz == 0 und tpz == 1, damit wird (spt - aspz/tpz) == spt,
	 * was der Realitaet entspricht.
	 */
	if (dp->tpz == 0) {
		*tpzp = 1L;
		*aspzp = 0L;
	}
	/*
	 * Bei unbekannter Geometrie kommen der primaere und der sekundaere
	 * Sformat info Block direkt hintereinander: (spt - aspz/tpz) == 1.
	 */
	if (dp->spt < 0 || dp->aspz < 0 || dp->tpz < 0) {
		*sptp = *tpzp = 1L;
		*aspzp = 0L;
	}
}

EXPORT void
print_sinfo(f, scgp)
	FILE	*f;
	SCSI	*scgp;
{
	struct sinfo	*sinfo = (struct sinfo *)sibuf;

	fprintf(f, "Disk info:\n");
	if (sinfo->si_id != 0)
		fprintf(f, "\tSinfo id: %ld\n", sinfo->si_id);
	if (sinfo->si_magic != SI_MAGIC)
		fprintf(f, "\tSinfo magic: 0x%lX\n", sinfo->si_magic);

	if (sinfo->si_magic != SI_MAGIC ||
			(sinfo_chksum(scgp, (u_long *)sibuf) !=
			((u_long *)sibuf)[scgp->cap->c_bsize/sizeof (u_long) - 1])) {
		fprintf(f, "\tDisk seems not to be formatted with sformat (release >= 3.0) before.\n");
		return;
	}
	fprintf(f, "\tNumber of reformats: %ld\n", sinfo->si_reformats);

	print_stamp(f, "\tFirst", &sinfo->si_first);
	if (sinfo->si_reformats > 0)
		print_stamp(f, "\tLast ", &sinfo->si_last);
	fprintf(f, "\n");
}

EXPORT BOOL
read_sinfo(scgp, dp, isformat)
	SCSI		*scgp;
	struct disk	*dp;
	BOOL		isformat;
{
	int	overbose;
	long	spt;
	long	aspz;
	long	tpz;
	BOOL	ret = TRUE;

	overbose = scgp->verbose;
	scgp->silent++;
	if (read_capacity(scgp) < 0) {
		scgp->silent--;
		scgp->verbose = overbose;
		return (FALSE);
	}
	if (scgp->cap->c_bsize > sizeof (sibuf))
		comerrno(EX_BAD, "PANIC Sectorsize.\n");

	sinfo_geom(scgp, dp, &spt, &aspz, &tpz);

	fillbytes((caddr_t)sibuf, sizeof (sibuf), '\0');
	if (read_scsi(scgp, (caddr_t)sibuf, scgp->cap->c_baddr, 1) < 0 &&
	    read_scsi(scgp, (caddr_t)sibuf, scgp->cap->c_baddr - (spt - aspz/tpz), 1) < 0) {
		errmsgno(EX_BAD, "Cannot read sformat info.\n");
		scgp->silent--;
		scgp->verbose = overbose;
		return (FALSE);
	}
	scgp->silent--;
	scgp->verbose = overbose;

	if (sinfo_chksum(scgp, (u_long *)sibuf) !=
			((u_long *)sibuf)[scgp->cap->c_bsize/sizeof (u_long) - 1]) {
		errmsgno(EX_BAD, "Sformat info not initialized or damaged.\n");
		ret = FALSE;
	}
	if (isformat) {
		if (((struct sinfo *)sibuf)->si_magic != SI_MAGIC ||
				((struct sinfo *)sibuf)->si_release < SI_RELEASE) {
			/*
			 * Need to upgrade sinfo
			 */
			fill_sinfo(scgp, (struct sinfo *)sibuf);
		} else {
			/*
			 * Need to mark this format action
			 */
			((struct sinfo *)sibuf)->si_reformats++;
			fill_stamp(&((struct sinfo *)sibuf)->si_last);
			((u_long *)sibuf)[scgp->cap->c_bsize/sizeof (u_long) - 1] =
						sinfo_chksum(scgp, (u_long *)sibuf);
		}
	}
	return (ret);
}

EXPORT BOOL
write_sinfo(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	int	overbose;
	long	spt;
	long	aspz;
	long	tpz;

	overbose = scgp->verbose;
	scgp->silent++;
	if (read_capacity(scgp) < 0) {
		scgp->silent--;
		scgp->verbose = overbose;
		return (FALSE);
	}
	if (scgp->cap->c_bsize > sizeof (sibuf))
		comerrno(EX_BAD, "PANIC Sectorsize.\n");

	sinfo_geom(scgp, dp, &spt, &aspz, &tpz);

	if (write_scsi(scgp, (caddr_t)sibuf, scgp->cap->c_baddr, 1) < 0) {
		errmsgno(EX_BAD, "Cannot write sformat info.\n");
		scgp->silent--;
		scgp->verbose = overbose;
		return (FALSE);
	}
	if (write_scsi(scgp, (caddr_t)sibuf, scgp->cap->c_baddr - (spt - aspz/tpz), 1) < 0) {
		errmsgno(EX_BAD, "Cannot write backup sformat info.\n");
		scgp->silent--;
		scgp->verbose = overbose;
		return (FALSE);
	}
	scgp->silent--;
	scgp->verbose = overbose;
	return (TRUE);
}

