/* @(#)xpart.c	1.25 09/07/11 Copyright 1991-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)xpart.c	1.25 09/07/11 Copyright 1991-2009 J. Schilling";
#endif
/*
 *	Routines to handle external partition info (database)
 *
 *	Copyright (c) 1991-2009 J. Schilling
 *
 *	XXX #ifdef HAVE_DKIO ist vorerst nur ein Hack
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
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include "dsklabel.h"
#ifndef	HAVE_DKIO
#	undef	SVR4
#endif
#ifdef	SVR4
#include <sys/vtoc.h>
#endif
#include <schily/string.h>
#include "fmt.h"

LOCAL	struct dk_label * save_part __PR((struct dk_label *));
LOCAL	int	do_part		__PR((char *, char *, struct dk_label *));
LOCAL	void	set_labelitem	__PR((struct dk_label *, char *));
LOCAL	BOOL	set_onepart	__PR((char *, struct dk_label *));
LOCAL	BOOL	set_labelvar	__PR((char *, struct dk_label *));

#ifdef	FMT
extern	int	xdebug;
extern	int	save_mp;
#else
	int	xdebug = 1;
#endif

#ifndef	FMT
main(ac, av)
	int	ac;
	char	**av;
{
	char	*name;
	struct dk_label d_xlabel;

	if (ac > 1)
		name = av[1];
	else
		name = "Berthold 100MB (Quantum)";

	if (!opendatfile("sformat.dat"))
		return;

	ext_part(scgp, name, (char *)0, &d_xlabel, 0);

	closedatfile();
}
#endif

#define	MAXLABELS	20

LOCAL struct dk_label *
save_part(lp)
	struct dk_label *lp;
{
	struct dk_label *rp;

	rp = (struct dk_label *)malloc(sizeof (*lp));
	if (rp == 0) {
		errmsg("No space for label.\n");
		return ((struct dk_label *)0);
	}
	movebytes(lp, rp, sizeof (*lp));
	return (rp);
}

EXPORT BOOL
ext_part(scgp, dname, pname, defpname, lp, def_func, dp)
	SCSI	*scgp;
	char	*dname;
	char	*pname;
	char	*defpname;
	struct dk_label *lp;
	BOOL	(*def_func) __PR((SCSI *scgp, struct disk *, struct dk_label *));
	struct disk	*dp;
{
	struct dk_label tlabel;
	struct dk_label	*label[MAXLABELS];
	int	nlabel = 0;
	int	def;
	int	this;
	int	i;
	int	ret;

	if (xdebug)
		printf("ext_part(\"%s\", \"%s\", \"%s\")\n",
						dname, pname, defpname);

	for (i = 0; i < MAXLABELS; i++)
		label[i] = 0;

	if (rewinddatfile() < 0)
		goto defpart;

	while (scanfortable("partition", NULL)) {
		if (xdebug) printf("ext_part: curword: '%s'\n", curword());
		ret = do_part(dname, pname, &tlabel);
		if (xdebug) printf("ext_part: do_part() = %d\n", ret);
		if (pname && ret > 0) {
			movebytes(&tlabel, lp, sizeof (tlabel));
			return (TRUE);
		}
		if (ret > 0) {
			label[nlabel] = save_part(&tlabel);
			if (label[nlabel] == 0)
				break;
			nlabel++;
		}
	}
defpart:
	if (nlabel == 0 && def_func) {
		if (def_func && (*def_func)(scgp, dp, &tlabel)) {
			label[nlabel] = save_part(&tlabel);
			if (label[nlabel] == 0)
				return (FALSE);
			nlabel++;
		}
	}
	if (nlabel == 0) {
		error("No label found for this disk.\n");
		return (FALSE);
	}
	def = -1;
	if (defpname) for (i = 0; i < nlabel; i++) {
		if (streql(defpname, getasciilabel(label[i])))
			def = i;
	}
	if (getasciilabel(lp)) {
		for (i = 0; i < nlabel; i++) {
			if (label_cmp(label[i], lp) >= 1)
				break;
		}
		if (i < nlabel) {
			this = i;
		} else {
			label[nlabel] = lp;
			this = nlabel++;
		}
	} else {
		this = 0;
	}
	do {
		for (i = 0; i < nlabel; i++) {
			char	*name;

			name = getasciilabel(label[i]);
			printf("%s%2d)%s\t\"%s\"\n",
				i == this ? "*": " ", i,
				i == def ? "+" : " ",
				name);
		}
		if (nlabel == 1)
			this = 0;
		else
			getint("Select partition", &this, 0, nlabel-1);
		if (yes("Print partition table? ")) {
			printf("\n");
			prpartab(stdout, dname, label[this]);
			printf("\n");
			printparts(label[this]);
		}
	} while (nlabel != 1 && !yes("Use selection %d \"%s\"? ",
					this, getasciilabel(label[this])));
	movebytes(label[this], lp, sizeof (tlabel));

	for (i = 0; i < nlabel; i++) {
		if (label[i] != lp)
			free(label[i]);
	}
	return (TRUE);
}


/*---------------------------------------------------------------------------
|
|	Parst einen Partition Eintrag in der Steuerungsdatei
|
+---------------------------------------------------------------------------*/

LOCAL int
do_part(dname, pname, lp)
	char	*dname;
	char	*pname;
	struct dk_label *lp;
{
	char	*word;
	char	lname[sizeof (lp->dkl_asciilabel)];
	char	dnbuf[128];
	BOOL	disk_ok = FALSE;

	label_null(lp);
	if (xdebug) printf("do_part: line: %d curword: '%s'\n",
						getlineno(), curword());
	word = curword();
	if (pname && !streql(pname, word))
		return (FALSE);
	if (strlen(word) >= sizeof (lp->dkl_asciilabel))
		datfileerr("Label '%s' too long\n", word);
	strncpy(lname, word, sizeof (lp->dkl_asciilabel)-1);
	lname[sizeof (lp->dkl_asciilabel)-1] = '\0';

	(void) garbage(skipwhite(peekword()));
	if (!nextline())
		return (EOF);

	while ((word = scanforline(NULL, NULL)) != NULL) {
		if (!disk_ok)
			dnbuf[0] = '\0';
		set_labelitem(lp, dnbuf);
		if (!disk_ok && *dnbuf) {
			if (xdebug) {
				printf("name: %.28s\n", dname);
				printf("DATA: %.28s\n", dnbuf);
			}
			if (streql(dnbuf, dname))
				disk_ok = TRUE;
		}
	}
	if (xdebug) printf("disk_ok: %d\n", disk_ok);
	if (!labelgeom_ok(lp, 1))
		return (FALSE);
	if (disk_ok) {
		setasciilabel(lp, lname);
		if (xdebug) printf("label: <%s>\n", lp->dkl_asciilabel);
		lp->dkl_magic = DKL_MAGIC;
		lp->dkl_cksum = 0;
		lp->dkl_cksum = do_cksum(lp);
		if (xdebug) prpartab(stdout, dname, lp);
	}
	return (disk_ok);
}

LOCAL void
set_labelitem(lp, dname)
	struct dk_label	*lp;
	char		*dname;
{
	char	*word;

	for (word = curword(); *word; word = nextword()) {
		if (streql(word, ":"))
			continue;
		if (streql(word, "disk")) {
			if (!set_stringvar("Disk Name", dname, 79))
				break;
		} else if (word[1] == '\0' &&
				strchr("abcdefgh01234567", word[0])) {
			if (!set_onepart(word, lp))
				break;
		} else if (!set_labelvar(word, lp))
			break;
	}
	(void) nextword();
}

LOCAL BOOL
set_onepart(word, lp)
	char		*word;
	struct dk_label	*lp;
{
	int	n = *word - 'a';
	long	l;
	struct strval	*vtag;
	struct strval	*vflag;
	extern struct strval vtags[];
	extern struct strval vflags[];
#ifdef	SVR4
	extern struct dk_map2  default_vtmap[];
#endif

	if (n < 0 || n > 9)
		n = *word - '0';	/* allow Solaris 2.x partition names */

	if (xdebug) printf("part: '%s' :", word);

	if (!checkequal())
		return (FALSE);

	if (!isval(word = nextword()))
		return (FALSE);

	if ((vtag = namestrval(word, vtags)) != 0) {
		if (!checkcomma())
			return (FALSE);
		if (!isval(word = nextword()))
			return (FALSE);
	}
	if ((vflag = namestrval(word, vflags)) != 0) {
		if (!checkcomma())
			return (FALSE);
		if (!isval(word = nextword()))
			return (FALSE);
	}
#ifdef	SVR4
	lp->dkl_vtoc.v_version = V_VERSION;
	lp->dkl_vtoc.v_nparts = NDKMAP;
	lp->dkl_vtoc.v_sanity = VTOC_SANE;
	lp->dkl_vtoc.v_part[n].p_tag = default_vtmap[n].p_tag;
	lp->dkl_vtoc.v_part[n].p_flag = default_vtmap[n].p_flag;

	if (vtag || vflag) {
		if (vtag)
			lp->dkl_vtoc.v_part[n].p_tag = vtag->s_val;
		if (vflag)
			lp->dkl_vtoc.v_part[n].p_flag = vflag->s_val;
	}
#endif
	if (*astol(word, &l) != '\0') {
		datfileerr("not a number '%s'", word);
		return (FALSE);
	}
	lp->dkl_map[n].dkl_cylno = l;

	if (!checkcomma())
		return (FALSE);
	if (!isval(word = nextword()))
		return (FALSE);
	if (*astol(word, &l) != '\0') {
		datfileerr("not a number '%s'", word);
		return (FALSE);
	}
	lp->dkl_map[n].dkl_nblk = l;

	if (xdebug) printf("%ld, %ld\n",
			(long)lp->dkl_map[n].dkl_cylno,
			(long)lp->dkl_map[n].dkl_nblk);
	return (TRUE);
}

LOCAL BOOL
set_labelvar(word, lp)
	char		*word;
	struct dk_label	*lp;
{
	unsigned short	*valp;
		int	i;

	if (xdebug) printf("labelvar: '%s' :", word);

	if (streql(word, "lapc"))
		valp = &lp->dkl_apc;
	else if (streql(word, "ncyl"))
		valp = &lp->dkl_ncyl;
	else if (streql(word, "lncyl"))
		valp = &lp->dkl_ncyl;
	else if (streql(word, "lacyl"))
		valp = &lp->dkl_acyl;
	else if (streql(word, "lhead"))
		valp = &lp->dkl_nhead;
	else if (streql(word, "lspt"))
		valp = &lp->dkl_nsect;
	else {
		skip_illvar("label", word);
		return (FALSE);
	}

	if (!checkequal())
		return (FALSE);

	if (!isval(word = nextword()))
		return (FALSE);
	if (*astoi(word, &i) != '\0') {
		datfileerr("not a number '%s'", word);
		return (FALSE);
	}
	*valp = i;

	if (xdebug) printf("%d\n", *valp);
	return (TRUE);
}
