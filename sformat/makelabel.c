/* @(#)makelabel.c	1.52 07/05/24 Copyright 1988-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)makelabel.c	1.52 07/05/24 Copyright 1988-2004 J. Schilling";
#endif
/*
 *	Routines to create / modify a label
 *
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
#include <stdio.h>
#include <schily/standard.h>
#include <schily/ioctl.h>
#ifdef	SVR4
#include <termios.h>
#endif
#include <schily/fcntl.h>
#include <sys/file.h>
#include "dsklabel.h"
#ifndef	HAVE_DKIO
#	undef	SVR4
#endif
#ifdef	SVR4
#include <sys/vtoc.h>
#endif
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/schily.h>

#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "fmt.h"

LOCAL	char	labelbuf[80];
extern	struct	dk_label *d_label;

char	labelname[] = "Label";
char	labelproto[] = "Label.prototype";
char	*Lname = labelname;
char	*Lproto = labelproto;

EXPORT	void	set_default_vtmap	__PR((struct dk_label *lp));
EXPORT	unsigned short do_cksum		__PR((struct dk_label *l));
EXPORT	BOOL	check_vtmap		__PR((struct dk_label *lp, BOOL set));
EXPORT	void	setlabel_from_val	__PR((SCSI *scgp, struct disk *dp, struct dk_label *lp));
EXPORT	void	makelabel		__PR((SCSI *scgp, struct disk *dp, struct dk_label *lp));
LOCAL	void	chpart			__PR((struct disk *dp, struct dk_label *lp, int idx));
#ifdef	SVR4
LOCAL	int	prvttab			__PR((FILE *f, struct dk_label *lp, int i, int n));
#endif
EXPORT	void	prpartab		__PR((FILE *f, char *disk_type, struct dk_label *lp));
EXPORT	void	printlabel		__PR((struct dk_label *lp));
LOCAL	void	printparthead		__PR((void));
EXPORT	void	printpart		__PR((struct dk_label *lp, int i));
LOCAL	void	_printpart		__PR((struct dk_label *lp, int i));
EXPORT	void	printparts		__PR((struct dk_label *lp));
EXPORT	int	readlabel		__PR((char *name, struct dk_label *lp));
EXPORT	void	writelabel		__PR((char *name, struct dk_label *lp));
LOCAL	void	writebackuplabel	__PR((int f, struct dk_label *lp));
EXPORT	int	setlabel		__PR((char *name, struct dk_label *lp));
LOCAL	int	setgeom			__PR((int f, struct dk_label *lp));
EXPORT	char	*getasciilabel		__PR((struct dk_label *lp));
EXPORT	void	setasciilabel		__PR((struct dk_label *lp, char *lname));
EXPORT	BOOL	setval_from_label	__PR((struct disk *dp, struct dk_label *lp));
EXPORT	void	label_null		__PR((struct dk_label *lp));
EXPORT	int	label_cmp		__PR((struct dk_label *lp1, struct dk_label *lp2));
EXPORT	BOOL	labelgeom_ok		__PR((struct dk_label *lp, BOOL print));
LOCAL	void	lerror			__PR((struct dk_label *lp, char *name));
LOCAL	void	tty_insert		__PR((char *s));
LOCAL	long	getprevpart		__PR((void));
LOCAL	long	getnextpart		__PR((void));
EXPORT	BOOL	cvt_cyls		__PR((char *linep, long *lp, long mini, long maxi, struct disk *dp));
EXPORT	BOOL	cvt_bcyls		__PR((char *linep, long *lp, long mini, long maxi, struct disk *dp));

#ifdef	SVR4
struct strval	vtags[] = {
	{	V_UNASSIGNED,	"unassigned",	"" },
	{	V_BOOT,		"boot",		"" },
	{	V_ROOT,		"root",		"" },
	{	V_SWAP,		"swap",		"" },
	{	V_USR,		"usr",		"" },
	{	V_BACKUP,	"backup",	"" },
	{	V_STAND,	"stand",	"(used for opt)" },
	{	V_VAR,		"var",		"" },
	{	V_HOME,		"home",		"" },
	{ 0 }
};

struct strval	vflags[] = {
	{ 0,			"wm",	"read-write, mountable" },
	{ V_UNMNT,		"wu",	"read-write, unmountable" },
	{ V_RONLY,		"rm",	"read-only, mountable" },
	{ V_RONLY|V_UNMNT,	"ru",	"read-only, unmountable" },
	{ 0 }
};

struct dk_map2	default_vtmap[NDKMAP] = {
	{	V_ROOT,		0	},	/* a == 0 */
	{	V_SWAP,		V_UNMNT	},	/* b == 1 */
	{	V_BACKUP,	V_UNMNT	},	/* c == 2 */
	{	V_UNASSIGNED,	0	},	/* d == 3 */
	{	V_UNASSIGNED,	0	},	/* e == 4 */
	{	V_UNASSIGNED,	0	},	/* f == 5 */
	{	V_USR,		0	},	/* g == 6 */
	{	V_UNASSIGNED,	0	},	/* h == 7 */
};

EXPORT void
set_default_vtmap(lp)
	register struct dk_label *lp;
{
	register int i;

	lp->dkl_vtoc.v_version = V_VERSION;
	lp->dkl_vtoc.v_nparts = NDKMAP;
	lp->dkl_vtoc.v_sanity = VTOC_SANE;

	for (i = 0; i < NDKMAP; i++) {
		lp->dkl_vtoc.v_part[i].p_tag = default_vtmap[i].p_tag;
		lp->dkl_vtoc.v_part[i].p_flag = default_vtmap[i].p_flag;

	}
}
#else
/*
 * dummies for SunOS 4.x
 * to allow the interpratation of a unique database for
 * SunOS 4.x and Solaris 2.x
 */
struct strval	vtags[] = {
	{	0,	"unassigned",	"" },
	{	0,	"boot",		"" },
	{	0,	"root",		"" },
	{	0,	"swap",		"" },
	{	0,	"usr",		"" },
	{	0,	"backup",	"" },
	{	0,	"stand",	"" },
	{	0,	"var",		"" },
	{	0,	"home",		"" },
	{ 0 }
};

struct strval	vflags[] = {
	{ 0,	"wm",	"" },
	{ 0,	"wu",	"" },
	{ 0,	"rm",	"" },
	{ 0,	"ru",	"" },
	{ 0 }
};

/* ARGSUSED */
EXPORT void
set_default_vtmap(lp)
	register struct dk_label *lp;
{
}
#endif

EXPORT unsigned short
do_cksum(l)
	register struct dk_label *l;
{
	register short	*sp;
	register short	sum = 0;
	register short	count = (sizeof (struct dk_label)/sizeof (short)) - 1;

	sp = (short *)l;
	while (count--)  {
		sum ^= *sp++;
	}
	return (sum);
}

EXPORT BOOL
check_vtmap(lp, set)
	register struct dk_label *lp;
		BOOL		set;
{
	int	ret = TRUE;

#ifdef	SVR4
	if (lp->dkl_vtoc.v_version != V_VERSION) {
		unsigned short sum = do_cksum(lp);

		ret = FALSE;
		if (set) {
			printf("Assigning default vtoc map to label.\n");
			set_default_vtmap(lp);
			if (sum == lp->dkl_cksum)
				lp->dkl_cksum =	do_cksum(lp);
		} else {
			printf("WARNING: no vtoc map in label.\n");
		}
	}
#endif
	return (ret);
}

EXPORT void
setlabel_from_val(scgp, dp, lp)
	SCSI			*scgp;
	struct disk		*dp;
	register struct dk_label *lp;
{
	if (lp->dkl_magic != DKL_MAGIC)
		fillbytes((caddr_t)lp, sizeof (*lp), '\0');

	getasciilabel(lp);

	lp->dkl_rpm = dp->rpm;

	/*
	 * XXX
	 * Alternates pro Zylinder sind von SUN noch nicht vollständig
	 * implementiert. Es ist noch nicht klar, wie
	 */
/*	lp->dkl_apc = dp->nhead * dp->aspz / dp->tpz;*/
	lp->dkl_apc = 0;
	if (dp->gap1 < 0)		/* Hack fuer label.c */
		dp->gap1 = 0;
	if (dp->gap2 < 0)		/* Hack fuer label.c */
		dp->gap2 = 0;
	lp->dkl_gap1 = dp->gap1;
	lp->dkl_gap2 = dp->gap2;
	if (dp->interleave < 0)	/* Hack fuer label.c */
		dp->interleave = 1;
	lp->dkl_intrlv = dp->interleave;
	if (lp->dkl_intrlv == 0)	/* Hack fuer Sony */
		lp->dkl_intrlv = 1;
	if (dp->lncyl <= 0)	/* XXX kann spaeter weg */
		raisecond("setlabel_from_val: lncyl", 0L);
	if (dp->lpcyl <= 0 || dp->lpcyl > 0xFFFE)
		get_lgeom_defaults(scgp, dp);
	lp->dkl_ncyl = dp->lncyl;
	lp->dkl_acyl = dp->lacyl;
	lp->dkl_pcyl = dp->lpcyl;
	lp->dkl_nhead = dp->lhead;
	lp->dkl_nsect = dp->lspt;
	lp->dkl_bhead = 0;	/* Kopf auf dem das Label steht */
	lp->dkl_ppart = 0;	/* Nummer der physikalischen Partition */

	setasciilabel(lp, labelbuf);

	if (dp->labelread < 0) {
		printf("Setting part '%c' to calculated values.\n", PARTOFF+2);
		lp->dkl_map[2].dkl_nblk =
			lp->dkl_ncyl * lp->dkl_nhead * lp->dkl_nsect;
		lp->dkl_map[2].dkl_cylno = 0;
	}

	lp->dkl_magic = DKL_MAGIC;
	lp->dkl_cksum =	do_cksum(lp);
}

EXPORT void
makelabel(scgp, dp, lp)
	SCSI			*scgp;
	struct disk		*dp;
	register struct dk_label *lp;
{
	char	lbuf[80];

	setlabel_from_val(scgp, dp, lp);

	if (yes("Label: <%s> change ? ", labelbuf)) {
		printf("Enter disk label: "); flush();
		tty_insert(labelbuf);
		(void) getline(lbuf, sizeof (lbuf));
		strcpy(labelbuf, lbuf);
	}
	setasciilabel(lp, labelbuf);

#ifdef	SVR4
	strncpy(lbuf, lp->dkl_vtoc.v_volume, LEN_DKL_VVOL);
	lbuf[LEN_DKL_VVOL] = '\0';
	if (yes("Volume Name: <%s> change ? ", lbuf)) {
		printf("Enter volume name: "); flush();
		tty_insert(lbuf);
		(void) getline(lbuf, LEN_DKL_VVOL+1);
		strncpy(lp->dkl_vtoc.v_volume, lbuf, LEN_DKL_VVOL);
	}
#endif

	if (yes("Change partition table? ")) {
		register int	i;

		for (i = 0; i < 8; i++) {
			printpart(lp, i);
			if (yes("Change ? "))
				chpart(dp, lp, i);
		}
	}

	lp->dkl_magic = DKL_MAGIC;
	lp->dkl_cksum =	do_cksum(lp);
}

LOCAL struct dk_label	*cur_lp;	/* for cvt_cyls() */
LOCAL long		cur_part;	/* for cvt_cyls() */

LOCAL void
chpart(dp, lp, idx)
	struct disk		*dp;
	register struct dk_label *lp;
	register int		idx;
{
	long	l;
	long	maxsect = lp->dkl_ncyl * lp->dkl_nhead * lp->dkl_nsect;

	do {
top:
		printf("Partition %c\n", idx+PARTOFF);

#ifdef	SVR4
		lp->dkl_vtoc.v_version = V_VERSION;
		lp->dkl_vtoc.v_nparts = NDKMAP;
		lp->dkl_vtoc.v_sanity = VTOC_SANE;

		l = lp->dkl_vtoc.v_part[idx].p_tag;
		getstrval("Enter partition id Tag", &l, vtags, 0);
		lp->dkl_vtoc.v_part[idx].p_tag = l;
		l = lp->dkl_vtoc.v_part[idx].p_flag;
		getstrval("Enter partition permission Flags", &l, vflags, 0);
		lp->dkl_vtoc.v_part[idx].p_flag = l;
#endif

		cur_lp = lp;
		cur_part = idx;
		l = lp->dkl_map[idx].dkl_cylno;
		getdiskcyls("Enter starting cylinder",
				&l, 0L, (long)lp->dkl_ncyl);
		lp->dkl_map[idx].dkl_cylno = l;

		l = lp->dkl_map[idx].dkl_nblk;
		getdiskblocks("Enter number of blocks",
				&l, 0L,
		    (long)(maxsect -
		    (lp->dkl_map[idx].dkl_cylno*lp->dkl_nhead*lp->dkl_nsect)),
				dp);
		lp->dkl_map[idx].dkl_nblk = l;


		if (lp->dkl_map[idx].dkl_nblk %
					(long)(lp->dkl_nhead*lp->dkl_nsect))
			printf("Not on a cylidner boundary!!!\n");

		if (lp->dkl_map[idx].dkl_cylno*
				(long)(lp->dkl_nhead*lp->dkl_nsect) +
						lp->dkl_map[idx].dkl_nblk >
						maxsect) {
			printf("Partition exceeds end of disk.\n");
			goto top;
		}
		printpart(lp, idx);
	} while (!yes("Ok ? "));
}

#ifdef	SVR4
LOCAL int
prvttab(f, lp, i, n)
	register FILE	*f;
	register struct dk_label *lp;
	int	i;
	int	n;
{
	struct strval	*sp;

	if (lp->dkl_vtoc.v_part[i].p_tag != default_vtmap[i].p_tag) {
		if ((sp = strval(lp->dkl_vtoc.v_part[i].p_tag, vtags)) != 0) {
			fprintf(f, "%s, ", sp->s_name);
			if (n % 4)
				n++;
		}
	}
	if (lp->dkl_vtoc.v_part[i].p_flag != default_vtmap[i].p_flag) {
		if ((sp = strval(lp->dkl_vtoc.v_part[i].p_flag, vflags)) != 0) {
			fprintf(f, "%s, ", sp-> s_name);
			if (n % 4)
				n++;
		}
	}
	return (n);
}
#endif

EXPORT void
prpartab(f, disk_type, lp)
	register FILE	*f;
	char		*disk_type;
	register struct dk_label *lp;
{
	register int	i;
	register int	n = 0;

	getasciilabel(lp);
	fprintf(f, "partition = \"%s\"\n", labelbuf);
	fprintf(f, "\tdisk = \"%s\"\n\t", disk_type);
#ifdef	nono
/*	fprintf(f, "lrpm = %d : ", lp->dkl_rpm);*/
	fprintf(f, "pcyl:       %d\n", lp->dkl_pcyl);
	fprintf(f, "apc:        %d\n", lp->dkl_apc);
	fprintf(f, "gap1:       %d\n", lp->dkl_gap1);
	fprintf(f, "gap2:       %d\n", lp->dkl_gap2);
	fprintf(f, "interleave: %d\n", lp->dkl_intrlv);
#endif
	if (f == stdout) {
		/*
		 * We don't want this to appear in the database.
		 */
		fprintf(f, "lpcyl = %d : ", lp->dkl_pcyl);
	}

	fprintf(f, "lncyl = %d : ", lp->dkl_ncyl);
	fprintf(f, "lacyl = %d : ", lp->dkl_acyl);
	fprintf(f, "lhead = %d : ", lp->dkl_nhead);
	fprintf(f, "lspt = %d : ", lp->dkl_nsect);
#ifdef	nono
	fprintf(f, "bhead:      %d\n", lp->dkl_bhead);
	fprintf(f, "ppart:      %d\n", lp->dkl_ppart);
#endif

	for (i = 0; i < NDKMAP; i++) {
		if (lp->dkl_map[i].dkl_nblk) {

			if (n++ % 4 == 0)
				fprintf(f, "\n\t");
			else
				fprintf(f, " : ");
			fprintf(f, "%c = ", i + PARTOFF);
#ifdef	SVR4
			n = prvttab(f, lp, i, n);
#endif
			fprintf(f, "%ld, %ld",
					(long)lp->dkl_map[i].dkl_cylno,
					(long)lp->dkl_map[i].dkl_nblk);
		}
	}
	fprintf(f, "\n");
}

EXPORT void
printlabel(lp)
	register struct dk_label *lp;
{
	printf("label:      <%s>\n", lp->dkl_asciilabel);
#ifdef	SVR4
	printf("volname:    '%.8s'\n", lp->dkl_vtoc.v_volume);
#endif
	printf("rpm:        %d\n", lp->dkl_rpm);
	printf("pcyl:       %d\n", lp->dkl_pcyl);
	printf("apc:        %d\n", lp->dkl_apc);
	if ((short)lp->dkl_gap1 > 0 || (short)lp->dkl_gap2 > 0) {
		printf("gap1:       %d\n", lp->dkl_gap1);
		printf("gap2:       %d\n", lp->dkl_gap2);
	}
	printf("interleave: %d\n", lp->dkl_intrlv);
	printf("ncyl:       %d\n", lp->dkl_ncyl);
	printf("acyl:       %d\n", lp->dkl_acyl);
	printf("nhead:      %d\n", lp->dkl_nhead);
	printf("nsect:      %d\n", lp->dkl_nsect);
	if (lp->dkl_bhead || lp->dkl_ppart) {
		printf("bhead:      %d\n", lp->dkl_bhead);
		printf("ppart:      %d\n", lp->dkl_ppart);
	}

	if (lp->dkl_nhead == 0 || lp->dkl_nsect == 0)
		printf("ILLEGAL GEOMETRY\n");
	else
		printparts(lp);

	if (lp->dkl_magic != DKL_MAGIC)
		printf("WRONG MAGIC: 0x%X\n", lp->dkl_magic);

	printf("Checksum: 0x%X", lp->dkl_cksum);
	if (lp->dkl_cksum != do_cksum(lp))
		printf(" (should be 0x%X)", do_cksum(lp));
	printf("\n");
}

LOCAL void
printparthead()
{
/*
 *              0       root 00     0 -   34        0    49595     24.22MB (35)
 */
#ifdef	SVR4
	printf("Part     Tag Flag  Cylinders Startsec # of Sec      Size    Blocks\n");
#else
	printf("Part Cylinders Startsec # of Sec      Size    Blocks\n");
#endif
}

EXPORT void
printpart(lp, i)
	struct dk_label *lp;
	int	i;
{
	printparthead();
	_printpart(lp, i);
}

LOCAL void
_printpart(lp, i)
	register struct dk_label *lp;
	register int	i;
{
	register long	a;
#ifdef	SVR4
	struct strval	*sp;
	char		*tag = "ILLEGAL";
	char		*flag = "XX";
#endif

	a = (lp->dkl_map[i].dkl_nblk-1)/(long)(lp->dkl_nhead*lp->dkl_nsect);
	a += lp->dkl_map[i].dkl_cylno;
	if (lp->dkl_map[i].dkl_nblk == 0)
		a = 0;

#ifdef	SVR4
	sp = strval(lp->dkl_vtoc.v_part[i].p_tag, vtags);
	if (sp)
		tag = sp->s_name;
	sp = strval(lp->dkl_vtoc.v_part[i].p_flag, vflags);
	if (sp)
		flag = sp->s_name;

	/*
	 * Ugly, but Linux incude files violate POSIX and #define printf
	 * so we cannot include the #ifdef into the printf() arg list.
	 */
#	define	__PARTARGS1	(i + PARTOFF), tag, flag
#	define	__PARTFMT	"%c %10s %2s %5ld -%5ld %8ld %8ld %6ld.%02ldMB ("
#else
#	define	__PARTARGS1	(i + PARTOFF)
#	define	__PARTFMT	"%c %5ld -%5ld %8ld %8ld %6ld.%02ldMB ("
#endif
	printf(__PARTFMT,
			__PARTARGS1,
			(long)lp->dkl_map[i].dkl_cylno, a,
			(long)lp->dkl_map[i].dkl_cylno*lp->dkl_nsect*lp->dkl_nhead,
			(long)lp->dkl_map[i].dkl_nblk,
			(long)lp->dkl_map[i].dkl_nblk/(2*1024),
			(long)(lp->dkl_map[i].dkl_nblk%(2*1024))*100/(2*1024));
	a = lp->dkl_map[i].dkl_nblk/(long)(lp->dkl_nhead*lp->dkl_nsect);
	printf("%ld", a);
	a = lp->dkl_map[i].dkl_nblk%(long)(lp->dkl_nhead*lp->dkl_nsect);
	printf("/%ld/%ld",
			a / (long)lp->dkl_nsect,
			a % (long)lp->dkl_nsect);
	printf(")\n");
}

EXPORT void
printparts(lp)
	register struct dk_label *lp;
{
	register int	i;

	printparthead();
	for (i = 0; i < NDKMAP; i++) {
		if (lp->dkl_map[i].dkl_nblk)
			_printpart(lp, i);
	}
}

EXPORT int
readlabel(name, lp)
	char	*name;
	register struct dk_label *lp;
{
	int	f;

	if ((f = open(name, O_RDONLY|O_NDELAY)) < 0) {
		errmsg("Cannot open prototype '%s'\n", name);
		return (0);
	}
	if (read(f, (char *)lp, sizeof (*lp)) != sizeof (*lp)) {
		errmsg("Cannot read prototype '%s'\n", name);
		close(f);
		return (0);
	}
	close(f);
	return (1);
}

/*
 * This function is only used by the stand-alone 'label' program.
 */
EXPORT void
writelabel(name, lp)
	char	*name;
	register struct dk_label *lp;
{
	register int	f;
	struct dk_allmap	map;

	if ((f = open(name, O_CREAT|O_WRONLY|O_NDELAY, (mode_t)0666)) < 0)
		comerr("Cannot create '%s'\n", name);
	if (write(f, (char *)lp, sizeof (*lp)) < 0)
		errmsg("Cannot write '%s'\n", name);

	if (ioctl(f, DKIOCGAPART, &map) >= 0) {
		/*
		 * It seemes to be a disk!
		 */
		writebackuplabel(f, lp);
		if (ioctl(f, DKIOCSAPART,
				(struct dk_allmap *)&lp->dkl_map[0]) < 0)
			comerr("Cannot set partitions.\n");
		(void) setgeom(f, lp);
	}
	close(f);
	exit(0);
}

LOCAL void
writebackuplabel(f, lp)
	int	f;
	register struct dk_label *lp;
{
	struct dk_allmap	map;
		long	backup_label_blk;
		int	i;

	backup_label_blk = lp->dkl_ncyl + lp->dkl_acyl - 1;
	backup_label_blk *= lp->dkl_nhead * lp->dkl_nsect;
	backup_label_blk += (lp->dkl_nhead-1) * lp->dkl_nsect;

	ioctl(f, DKIOCGAPART, &map);
	/*
	 * If he's not using partition '2' (the whole disk)
	 * he will fail later on.
	 */
	map.dka_map[2].dkl_cylno = 0;
	map.dka_map[2].dkl_nblk = backup_label_blk + 10;

	if (ioctl(f, DKIOCSAPART, &map) >= 0) {
		for (i = 1; i < 10; i += 2) {
			if (lseek(f, (off_t)(512*(backup_label_blk + i)),
								SEEK_SET) < 0) {
				errmsg("Seek error for backup label\n");
				continue;
			}
			if (write(f, (char *)lp, sizeof (*lp)) < 0)
				errmsg("Cannot write backup label\n");
		}
	}
}

EXPORT int
setlabel(name, lp)
	char	*name;
	register struct dk_label *lp;
{
	register int	f;
	int		err;
	struct dk_allmap	map;

	if ((f = open(name, O_RDONLY|O_NDELAY)) < 0) {
		err = geterrno();
		errmsgno(err, "Cannot open '%s'\n", name);
		return (err);
	}

	if (ioctl(f, DKIOCGAPART, &map) < 0) {
		err = geterrno();
		errmsgno(err, "Cannot get partitions from '%s'.\n", name);
		close(f);
		return (err);
	}

	if (ioctl(f, DKIOCSAPART, (struct dk_allmap *)&lp->dkl_map[0]) < 0) {
		err = geterrno();
		errmsgno(err, "Cannot set partitions on '%s'.\n", name);
		close(f);
		return (err);
	}
	err = setgeom(f, lp);
	close(f);
	return (err);
}

LOCAL int
setgeom(f, lp)
	int	f;
	register struct dk_label *lp;
{
	struct dk_geom geom;
	register struct dk_geom *gp;

	gp = &geom;

	gp->dkg_ncyl = lp->dkl_ncyl;
	gp->dkg_acyl = lp->dkl_acyl;
/*	gp->dkg_bcyl = lp->dkl_bcyl;*/
	gp->dkg_bcyl = 0;
	gp->dkg_nhead = lp->dkl_nhead;
	gp->dkg_bhead = lp->dkl_bhead;
	gp->dkg_nsect = lp->dkl_nsect;
	gp->dkg_intrlv = lp->dkl_intrlv;
	gp->dkg_gap1 = lp->dkl_gap1;
	gp->dkg_gap2 = lp->dkl_gap2;
	gp->dkg_apc = lp->dkl_apc;
	gp->dkg_rpm = lp->dkl_rpm;
	gp->dkg_pcyl = lp->dkl_pcyl;

	if (ioctl(f, DKIOCSGEOM, gp) < 0) {
		int	err = geterrno();

		errmsgno(err, "Cannot set geometry\n");
		return (err);
	}
	return (0);
}

EXPORT char *
getasciilabel(lp)
	register struct dk_label *lp;
{
	register char	*idx;

	labelbuf[0] = '\0';
	if (lp->dkl_magic != DKL_MAGIC || lp->dkl_cksum != do_cksum(lp))
		return ((char *)0);
	if ((idx = strstr(lp->dkl_asciilabel, " cyl")) != NULL) {
		strncpy(labelbuf, lp->dkl_asciilabel, idx-lp->dkl_asciilabel);
		labelbuf[idx - lp->dkl_asciilabel] = '\0';
		return (labelbuf);
	}
	strcpy(labelbuf, lp->dkl_asciilabel);
	return (labelbuf);
}

EXPORT void
setasciilabel(lp, lname)
	register struct dk_label *lp;
	char	*lname;
{
	char	lbuf[150];

	sprintf(lbuf, "%.80s cyl %d alt %d hd %d sec %d",
			lname,
			lp->dkl_ncyl, lp->dkl_acyl,
			lp->dkl_nhead, lp->dkl_nsect);
	sprintf(lp->dkl_asciilabel, "%.128s", lbuf);
}

EXPORT BOOL
setval_from_label(dp, lp)
	struct disk		*dp;
	register struct dk_label *lp;
{
	if (lp->dkl_magic != DKL_MAGIC || lp->dkl_cksum != do_cksum(lp))
		return (FALSE);

	getasciilabel(lp);

	/*
	 * Die nicht direkt der evt. gefälschten Labelgeometrie zugeordneten
	 * Werte dürfen nicht die schon vorhandenen echten Werte überschreiben.
	 */
	if (dp->rpm <= 0 && lp->dkl_rpm != ((unsigned short)-1) &&
							lp->dkl_rpm > 0)
		dp->rpm = lp->dkl_rpm;

	if (dp->pcyl <= 0 && lp->dkl_pcyl != ((unsigned short)-1)) {
		if (lp->dkl_pcyl > 0) {
			dp->pcyl = lp->dkl_pcyl;
		} else {
			dp->pcyl = lp->dkl_ncyl + lp->dkl_acyl;
			if (dp->atrk >= 0 && dp->nhead > 0)
				dp->pcyl += dp->atrk/dp->nhead;
			if (dp->int_cyl >= 0)
				dp->pcyl += dp->int_cyl;
		}
		if (dp->lpcyl < 0)
			dp->lpcyl = dp->pcyl;
	}

	if (dp->gap1 < 0 && lp->dkl_gap1 != ((unsigned short)-1))
		dp->gap1 = lp->dkl_gap1;
	if (dp->gap2 < 0 && lp->dkl_gap1 != ((unsigned short)-1))
		dp->gap2 = lp->dkl_gap2;

	if (dp->interleave < 0 && lp->dkl_intrlv != ((unsigned short)-1))
		dp->interleave = lp->dkl_intrlv;

	/*
	 * Hier folgen die evt. gefälschten Werte für die Labegeometrie.
	 */
	dp->lncyl = lp->dkl_ncyl;
	dp->lacyl = lp->dkl_acyl;
	dp->lhead = lp->dkl_nhead;
	dp->lspt = lp->dkl_nsect;
	/*
	 *XXX
	 * Alternates pro Zylinder sind von SUN noch nicht vollständig
	 * implementiert. Es ist noch nicht klar, wie
	 */
/*	dp->aspz = lp->dkl_apc * dp->tpz / lp->dkl_nhead;*/
	if (dp->spt < 0)
		dp->spt = dp->lspt /*+ lp->dkl_apc / lp->dkl_nhead*/;

	if (dp->int_cyl < 0 && dp->nhead > 0) {
		dp->int_cyl = dp->pcyl - dp->lncyl
					- dp->lacyl
					- dp->atrk/dp->nhead;
	}
	return (TRUE);
}

EXPORT void
label_null(lp)
	register struct dk_label *lp;
{
	fillbytes((caddr_t)lp, sizeof (*lp), '\0');

	lp->dkl_rpm	= -1;	/* rotations per minute */
	lp->dkl_pcyl	= -1;	/* # physical cylinders */
	lp->dkl_apc	= -1;	/* alternates per cylinder */
	lp->dkl_gap1	= -1;	/* used to be gap1 */
	lp->dkl_gap2	= -1;	/* used to be gap2 */
	lp->dkl_intrlv	= -1;	/* interleave factor */
	lp->dkl_ncyl	= -1;	/* # of data cylinders */
	lp->dkl_acyl	= -1;	/* # of alternate cylinders */
	lp->dkl_nhead	= -1;	/* # of heads in this partition */
	lp->dkl_nsect	= -1;	/* # of 512 byte sectors per track */
	lp->dkl_bhead	= -1;	/* used to be label head offset */
	lp->dkl_ppart	= -1;	/* used to by physical partition */
}

#define	loff(a)		((int)&((struct dk_label *)0)->a)

EXPORT int
label_cmp(lp1, lp2)
	register struct dk_label *lp1;
	register struct dk_label *lp2;
{
	int	ret = 0;

	/*
	 * If Partition differs return (0)
	 */
	if (cmpbytes(lp1->dkl_map, lp2->dkl_map, sizeof (lp1->dkl_map)) <
						sizeof (lp1->dkl_map))
		return (0);

#ifdef	SVR4
	if (cmpbytes(&lp1->dkl_vtoc, &lp2->dkl_vtoc, sizeof (lp1->dkl_vtoc)) >=
						sizeof (lp1->dkl_vtoc))
		ret++;
#endif

	if (cmpbytes(lp1->dkl_pad, lp2->dkl_pad,
				loff(dkl_map[0]) - loff(dkl_pad[0])) >=
				loff(dkl_map[0]) - loff(dkl_pad[0]))
		ret++;

	if (streql(lp1->dkl_asciilabel, lp2->dkl_asciilabel))
		ret++;

	return (ret);
}

EXPORT BOOL
labelgeom_ok(lp, print)
	register struct dk_label *lp;
	BOOL	print;
{
	if (lp->dkl_ncyl == (unsigned short) -1) {
		if (print) lerror(lp, "lncyl");
		return (FALSE);
	}
	if (lp->dkl_acyl == (unsigned short) -1) {
		if (print) lerror(lp, "lacyl");
		return (FALSE);
	}
	if (lp->dkl_nhead == (unsigned short) -1) {
		if (print) lerror(lp, "lnhead");
		return (FALSE);
	}
	if (lp->dkl_nsect == (unsigned short) -1) {
		if (print) lerror(lp, "lspt");
		return (FALSE);
	}
	return (TRUE);
}

LOCAL void
lerror(lp, name)
	register struct dk_label *lp;
	char	*name;
{
	error("Missing '%s' in label \"%s\".\n", name, getasciilabel(lp));
}

LOCAL void
tty_insert(s)
	char	*s;
{
#ifdef	TIOCSTI
	while (*s)
		ioctl(fdown(stdin), TIOCSTI, s++);
#endif
}


LOCAL long
getprevpart()
{
	long part = cur_part - 1;

	/*
	 * Look for previous partition that is in use
	 * and ends before end of the disk.
	 */
	while (part > 0 &&
			(cur_lp->dkl_map[part].dkl_nblk == 0 ||
			(cur_lp->dkl_map[part].dkl_cylno +
			(cur_lp->dkl_map[part].dkl_nblk-1) /
			(long)(cur_lp->dkl_nhead * cur_lp->dkl_nsect))
					>= (long)(cur_lp->dkl_ncyl - 1))) {
		part--;
	}
	return (part);
}

LOCAL long
getnextpart()
{
	long part = cur_part + 1;

	/*
	 * Look for next partition that is in use
	 * and starts after the beginning of the current partition.
	 */
	while (part < NDKMAP &&
			(cur_lp->dkl_map[part].dkl_nblk == 0 ||
			cur_lp->dkl_map[part].dkl_cylno <=
			cur_lp->dkl_map[cur_part].dkl_cylno)) {
		part++;
	}
	return (part);
}

/*---------------------------------------------------------------------------
|
| Hier wird der Anfang einer Partition festgelegt
|
+---------------------------------------------------------------------------*/

EXPORT BOOL
cvt_cyls(linep, lp, mini, maxi, dp)
	char		*linep;
	long		*lp;
	long		mini;
	long		maxi;
	struct disk	*dp;
{
	long	l;
	long	part	= 0L;
	BOOL	doshift	= FALSE;

	if (linep[0] == '?') {
		printf("To enter cylinders as simple number:\n");
		(void) cvt_std(linep, lp, mini, maxi, dp);

		printf("To start this partition past the end of a partition:\n");
		printf("\tEnter '>' to start past the end of the previous valid partition (%ld).\n",
					getprevpart());
		printf("\tEnter '>g' to start past the end of partition 'g'.\n");
		printf("\tEnter '>5' to start past the end of partition '5'.\n");

		printf("To shift this partition to end at the beginning of a partition:\n");
		printf("\tEnter '>>' to shift to the beginning of the next valid partition (%ld).\n",
					getnextpart());
		printf("\tEnter '>>g' to shift to the beginning of partition 'g'.\n");
		printf("\tEnter '>>5' to shift to the beginning of partition '5'.\n");
		printf("\tEnter '>>%d' to shift to the the end of the disk.\n", NDKMAP);
		return (FALSE);
	}
	/*
	 * If line does not begin with '>'
	 * convert as int.
	 */
	if (*linep != '>')
		return (cvt_std(linep, lp, mini, maxi, (void *)0));

	if (linep[1] == '>') {
		doshift = TRUE;
		linep++;
	}

	if (*++linep) {

		/*
		 * It's a > part 'x' end notation.
		 */
		if (strchr("abcdefgh", linep[0]) && linep[1] == '\0')
			part = linep[0] - 'a';
		else
			if (!cvt_std(linep, &part, 0L, NDKMAP, (void *)0))
				return (FALSE);
	} else {
		if (doshift) {
			part = getnextpart();
		} else {
			part = getprevpart();
			if (part < 0) {
				*lp = mini;
				return (TRUE);
			}
		}
	}
	if ((doshift && (part <= 0 || part > NDKMAP)) ||
	    (!doshift && (part < 0 || part >= NDKMAP))) {
		printf("partition '%ld' is out of range.\n", part);
		return (FALSE);
	}
	if (doshift)
		printf("shifting to partition %ld\n", part);
	else
		printf("appending to partition %ld\n", part);

	if (doshift) {
		if (part == NDKMAP) {
			l =  cur_lp->dkl_ncyl;
		} else {
			l =  cur_lp->dkl_map[part].dkl_cylno;
		}
		/*
		 * Round up to start at prev cylinder
		 */
		l -= (cur_lp->dkl_map[cur_part].dkl_nblk-1) /
			(long)(cur_lp->dkl_nhead * cur_lp->dkl_nsect);
		l -= 1;
	} else {
		/*
		 * Round up to start at next cylinder
		 */
		l =  cur_lp->dkl_map[part].dkl_cylno;
		l += (cur_lp->dkl_map[part].dkl_nblk-1) /
			(long)(cur_lp->dkl_nhead * cur_lp->dkl_nsect);
		l += 1;
	}

	if (l < mini || l > maxi) {
		printf("'%ld' is out of range.\n", l);
		return (FALSE);
	}
	*lp = l;
	return (TRUE);
}

/*---------------------------------------------------------------------------
|
| Hier wird das Ende einer Partition festgelegt
|
+---------------------------------------------------------------------------*/

EXPORT BOOL
cvt_bcyls(linep, lp, mini, maxi, dp)
	char		*linep;
	long		*lp;
	long		mini;
	long		maxi;
	struct disk	*dp;
{
	long	l;
	long	part	= 0L;

	if (linep[0] == '?') {
		printf("To fill up this partition to the beginning of a partition:\n");
		printf("\tEnter '>' to fill up to the beginning of the next valid partition (%ld).\n",
					getnextpart());
		printf("\tEnter '>g' to fill up to the beginning of partition 'g'.\n");
		printf("\tEnter '>5' to fill up to the beginning of partition '5'.\n");
		return (FALSE);
	}
	/*
	 * If line does not begin with '>'
	 * convert as int.
	 */
	if (*linep != '>')
		return (cvt_std(linep, lp, mini, maxi, (void *)0));

	if (*++linep) {

		/*
		 * It's a > part 'x' end notation.
		 */
		if (strchr("abcdefgh", linep[0]) && linep[1] == '\0')
			part = linep[0] - 'a';
		else
			if (!cvt_std(linep, &part, 0L, NDKMAP, (void *)0))
				return (FALSE);
	} else {
		part = getnextpart();
		if (part >= NDKMAP) {
			*lp = maxi;
			return (TRUE);
		}
	}
	if (part < 0 || part >= NDKMAP) {
		printf("partition '%ld' is out of range.\n", part);
		return (FALSE);
	}
	printf("filling up to partition %ld\n", part);

	l =  cur_lp->dkl_map[part].dkl_cylno -
			cur_lp->dkl_map[cur_part].dkl_cylno;

	l *= (long)(cur_lp->dkl_nhead * cur_lp->dkl_nsect);

	if (l < mini || l > maxi) {
		printf("'%ld' is out of range.\n", l);
		return (FALSE);
	}
	*lp = l;
	return (TRUE);
}
