/* @(#)volhdr.c	1.48 19/12/03 Copyright 1994, 2003-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)volhdr.c	1.48 19/12/03 Copyright 1994, 2003-2019 J. Schilling";
#endif
/*
 *	Volume header related routines.
 *
 *	Copyright (c) 1994, 2003-2019 J. Schilling
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
#include <schily/unistd.h>
#include <schily/hostname.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include <schily/standard.h>
#include <schily/string.h>
#define	__XDEV__	/* Needed to activate _dev_major()/_dev_minor() */
#include <schily/device.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/libport.h>
#include "starsubs.h"
#include "dumpdate.h"
#include "xtab.h"
#include "fifo.h"

extern	FILE	*vpr;
extern	BOOL	multivol;
extern	BOOL	binflag;
extern	long	chdrtype;
extern	char	*vers;
extern	int	verbose;
extern	Ullong	tsize;
extern	BOOL	ghdr;

extern struct timespec	ddate;			/* The current dump date	*/

extern	m_stats	*stats;

extern	GINFO	*gip;				/* Global information pointer	*/
extern	GINFO	*grip;				/* Global read info pointer	*/

EXPORT	void	ginit		__PR((void));
EXPORT	void	grinit		__PR((void));
LOCAL	int	xstrcpy		__PR((char **newp, char *old, char *p, int len));
EXPORT	void	gipsetup	__PR((GINFO *gp));
EXPORT	void	griprint	__PR((GINFO *gp));
EXPORT	BOOL	verifyvol	__PR((char *buf, int amt, int volno, int *skipp));
LOCAL	BOOL	vrfy_gvolhdr	__PR((char *buf, int amt, int volno, int *skipp));
EXPORT	char	*dt_name	__PR((int type));
EXPORT	int	dt_type		__PR((char *name));
EXPORT	void	put_release	__PR((void));
EXPORT	void	put_archtype	__PR((void));
EXPORT	void	put_gvolhdr	__PR((char *name));
EXPORT	void	put_volhdr	__PR((char *name, BOOL putv));
EXPORT	void	put_svolhdr	__PR((char *name));
EXPORT	void	put_multhdr	__PR((off_t size, off_t off));
EXPORT	BOOL	get_volhdr	__PR((FINFO *info, char *vhname));
LOCAL	void	get_label	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_hostname	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_filesys	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_cwd		__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_device	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_dumptype	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_dumplevel	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_reflevel	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_dumpdate	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_refdate	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_volno	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_blockoff	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_blocksize	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));
LOCAL	void	get_tapesize	__PR((FINFO *info, char *keyword, int klen, char *arg, size_t len));

/*
 * Important for the correctness of gen_number(): As long as we stay <= 55
 * chars for the keyword, a 128 bit number entry will fit into 100 chars.
 */
EXPORT xtab_t volhtab[] = {
			{ "SCHILY.volhdr.label",	19, get_label,	   0	},
			{ "SCHILY.volhdr.hostname",	22, get_hostname,  0	},
			{ "SCHILY.volhdr.filesys",	21, get_filesys,   0	},
			{ "SCHILY.volhdr.cwd",		17, get_cwd,	   0	},
			{ "SCHILY.volhdr.device",	20, get_device,	   0	},
			{ "SCHILY.volhdr.dumptype",	22, get_dumptype,  0	},
			{ "SCHILY.volhdr.dumplevel",	23, get_dumplevel, 0	},
			{ "SCHILY.volhdr.reflevel",	22, get_reflevel,  0	},
			{ "SCHILY.volhdr.dumpdate",	22, get_dumpdate,  0	},
			{ "SCHILY.volhdr.refdate",	21, get_refdate,   0	},
			{ "SCHILY.volhdr.volno",	19, get_volno,	   0	},
			{ "SCHILY.volhdr.blockoff",	22, get_blockoff,  0	},
			{ "SCHILY.volhdr.blocksize",	23, get_blocksize, 0	},
			{ "SCHILY.volhdr.tapesize",	22, get_tapesize,  0	},

			{ NULL,				0, NULL,	   0	}};

EXPORT void
ginit()
{
extern	int	dumplevel;
extern	int	nblocks;

	gip->label	= NULL;
	gip->hostname	= NULL;
	gip->filesys	= NULL;
	gip->cwd	= NULL;
	gip->device	= NULL;
	gip->release	= vers;
	gip->archtype	= chdrtype;
	gip->dumptype	= 0;
	gip->dumplevel	= dumplevel;
	gip->reflevel	= -1;
	gip->dumpdate	= ddate;
	gip->refdate.tv_sec  = 0;
	gip->refdate.tv_nsec = 0;
	gip->volno	= 1;
	gip->tapesize	= tsize;
	gip->blockoff	= 0;
	gip->blocksize	= nblocks;
	gip->gflags	=   GF_RELEASE|GF_DUMPLEVEL|GF_REFLEVEL|GF_DUMPDATE|
			    GF_VOLNO|GF_TAPESIZE|GF_BLOCKOFF|GF_BLOCKSIZE;
}

EXPORT void
grinit()
{
	if (grip->label) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->label);
		grip->label	= NULL;
	}
	if (grip->hostname) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->hostname);
		grip->hostname	= NULL;
	}
	if (grip->filesys) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->filesys);
		grip->filesys	= NULL;
	}
	if (grip->cwd) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->cwd);
		grip->cwd	= NULL;
	}
	if (grip->device) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->device);
		grip->device	= NULL;
	}
	if (grip->release) {
		if ((grip->gflags & GF_NOALLOC) == 0)
			free(grip->release);
		grip->release	= NULL;
	}
	grip->archtype	= H_UNDEF;
	grip->dumptype	= 0;
	grip->dumplevel	= 0;
	grip->reflevel	= 0;
	grip->dumpdate.tv_sec  = 0;
	grip->dumpdate.tv_nsec = 0;
	grip->refdate.tv_sec   = 0;
	grip->refdate.tv_nsec  = 0;
	grip->volno	= 0;
	grip->tapesize	= 0;
	grip->blockoff	= 0;
	grip->blocksize	= 0;
	grip->gflags	= 0;
}

/*
 * A special string copy that is used copy strings into the limited space
 * in the shared memory.
 */
LOCAL int
xstrcpy(newp, old, p, len)
	char	**newp;
	char	*old;
	char	*p;
	int	len;
{
	int	slen;

	if (old == NULL)
		return (0);

	slen = strlen(old) + 1;
	if (slen > len)
		return (0);
	*newp = p;
	strncpy(p, old, len);
	p[len-1] = '\0';

	return (slen);
}

/*
 * Set up the global GINFO *gip structure from a structure that just
 * has been read from the information on the current medium.
 * This structure is inside the shared memory if we are using the fifo.
 */
EXPORT void
gipsetup(gp)
	GINFO	*gp;
{
#ifdef	FIFO
extern	m_head	*mp;
extern	BOOL	use_fifo;
#endif
	if (gip->gflags & GF_MINIT) {
		return;
	}
	*gip = *gp;
	gip->label	= NULL;
	gip->hostname	= NULL;
	gip->filesys	= NULL;
	gip->cwd	= NULL;
	gip->device	= NULL;
	gip->release	= NULL;

#ifdef	FIFO
	if (use_fifo) {
		char	*p = (char *)&gip[1];
		int	len = mp->rsize;
		int	slen;

		slen = xstrcpy(&gip->label, gp->label, p, len);
		p += slen;
		len -= slen;
		slen = xstrcpy(&gip->filesys, gp->filesys, p, len);
		p += slen;
		len -= slen;
		slen = xstrcpy(&gip->cwd, gp->cwd, p, len);
		p += slen;
		len -= slen;
		slen = xstrcpy(&gip->hostname, gp->hostname, p, len);
		p += slen;
		len -= slen;
		slen = xstrcpy(&gip->release, gp->release, p, len);
		p += slen;
		len -= slen;
		slen = xstrcpy(&gip->device, gp->device, p, len);
		p += slen;
		len -= slen;
		gip->gflags |= GF_NOALLOC;
	} else
#endif
	{
		if (gp->label) {
			if (gip->label)
				free(gip->label);
			gip->label = ___savestr(gp->label);
		}
		if (gp->filesys) {
			if (gip->filesys)
				free(gip->filesys);
			gip->filesys = ___savestr(gp->filesys);
		}
		if (gp->cwd) {
			if (gip->cwd)
				free(gip->cwd);
			gip->cwd = ___savestr(gp->cwd);
		}
		if (gp->hostname) {
			if (gip->hostname)
				free(gip->hostname);
			gip->hostname = ___savestr(gp->hostname);
		}
		if (gp->release) {
			if (gip->release)
				free(gip->release);
			gip->release = ___savestr(gp->release);
		}
		if (gp->device) {
			if (gip->device)
				free(gip->device);
			gip->device = ___savestr(gp->device);
		}
	}
	if (gp->volno > 1)		/* Allow to start with vol # != 1 */
		stats->volno = gp->volno;
	gip->gflags |= GF_MINIT;
}

EXPORT void
griprint(gp)
	GINFO	*gp;
{
	register FILE	*f = vpr;

	if (verbose <= 0)
		return;

	if (gp->label)
		fgtprintf(f, "Label       %s\n", gp->label);

	if (gp->hostname)
		fgtprintf(f, "Host name   %s\n", gp->hostname);

	if (gp->filesys)
		fgtprintf(f, "File system %s\n", gp->filesys);

	if (gp->cwd)
		fgtprintf(f, "Working dir %s\n", gp->cwd);

	if (gp->device)
		fgtprintf(f, "Device      %s\n", gp->device);

	if (gp->release)
		fgtprintf(f, "Release     %s\n", gp->release);

	if (gp->archtype != H_UNDEF)
		fgtprintf(f, "Archtype    %s\n", hdr_name(gp->archtype));

	if (gp->gflags & GF_DUMPTYPE)
		fgtprintf(f, "Dumptype    %s\n", dt_name(gp->dumptype));

	if (gp->gflags & GF_DUMPLEVEL)
		fgtprintf(f, "Dumplevel   %d\n", gp->dumplevel);

	if (gp->gflags & GF_REFLEVEL)
		fgtprintf(f, "Reflevel    %d\n", gp->reflevel);

	if (gp->gflags & GF_DUMPDATE) {
		fgtprintf(f, "Dumpdate    %lld.%9.9lld (%s)\n",
			(Llong)gp->dumpdate.tv_sec,
			(Llong)gp->dumpdate.tv_nsec,
			dumpdate(&gp->dumpdate));
	}
	if (gp->gflags & GF_REFDATE) {
		fgtprintf(f, "Refdate     %lld.%9.9lld (%s)\n",
			(Llong)gp->refdate.tv_sec,
			(Llong)gp->refdate.tv_nsec,
			dumpdate(&gp->refdate));
	}
	if (gp->gflags & GF_VOLNO)
		fgtprintf(f, "Volno       %d\n", gp->volno);
	if (gp->gflags & GF_BLOCKOFF)
		fgtprintf(f, "Blockoff    %llu records\n", gp->blockoff);
	if (gp->gflags & GF_BLOCKSIZE)
		fprintf(f, "Blocksize   %d records\n", gp->blocksize);
	if (gp->gflags & GF_TAPESIZE)
		fgtprintf(f, "Tapesize    %llu records\n", gp->tapesize);
}

EXPORT BOOL
verifyvol(buf, amt, volno, skipp)
	char	*buf;
	int	amt;
	int	volno;
	int	*skipp;
{
	TCB	*ptb = (TCB *)buf;

	*skipp = 0;

	/*
	 * Minimale Blockgroesse ist 2,5k
	 * 'g' Header, 512 Byte Content and a 'V' header
	 */
	if (ptb->ustar_dbuf.t_typeflag == 'g') {
		if (pr_validtype(ptb->ustar_dbuf.t_typeflag) &&
		    tarsum_ok(ptb))
			return (vrfy_gvolhdr(buf, amt, volno, skipp));
	}
	if (ptb->ustar_dbuf.t_typeflag == 'x') {
		if (pr_validtype(ptb->ustar_dbuf.t_typeflag) &&
		    tarsum_ok(ptb)) {
			Ullong	ull;
			int	xlen;

			stolli(ptb->dbuf.t_size, &ull);
			xlen = ull;
			xlen = 1 + tarblocks(xlen);
			*skipp += xlen;
			ptb = (TCB *)&((char *)ptb)[*skipp * TBLOCK];
		}
	}

	if (ptb->ustar_dbuf.t_typeflag == 'V' ||
	    ptb->ustar_dbuf.t_typeflag == 'M') {
		if (pr_validtype(ptb->ustar_dbuf.t_typeflag) &&
		    tarsum_ok(ptb)) {
			*skipp += 1;
			ptb = (TCB *)&buf[*skipp * TBLOCK];
		}
	}
	return (TRUE);
}

LOCAL BOOL
vrfy_gvolhdr(buf, amt, volno, skipp)
	char	*buf;
	int	amt;
	int	volno;
	int	*skipp;
{
	TCB	*ptb = (TCB *)buf;
	FINFO	finfo;
	Ullong	ull;
	int	xlen = amt - TBLOCK - 1;
	char	*p = &buf[TBLOCK];
	char	*ep;
	char	ec;
	Llong	bytes;
	Llong	blockoff;
	BOOL	ret = FALSE;

	fillbytes((char *)&finfo, sizeof (finfo), '\0');
	finfo.f_tcb = ptb;

	if (init_pspace(PS_STDERR, &finfo.f_pname) < 0)
		return (FALSE);
	if (init_pspace(PS_STDERR, &finfo.f_plname) < 0)
		return (FALSE);

	finfo.f_name = finfo.f_pname.ps_path;
	finfo.f_lname = finfo.f_plname.ps_path;

	/*
	 * File size is strlen of extended header
	 */
	stolli(ptb->dbuf.t_size, &ull);
	if (xlen > ull)
		xlen = ull;

	grinit();	/* Clear/initialize current GINFO read struct */
	ep = p+xlen;
	ec = *ep;
	*ep = '\0';
	xhparse(&finfo, p, p+xlen);
	*ep = ec;
	griprint(grip);

	/*
	 * Return TRUE (no skip) if this was not a volume continuation header.
	 */
	if ((grip->gflags & GF_VOLNO) == 0) {
		ret = TRUE;
		goto out;
	}

	if ((gip->dumpdate.tv_sec != grip->dumpdate.tv_sec) ||
	    (gip->dumpdate.tv_nsec != grip->dumpdate.tv_nsec)) {
		errmsgno(EX_BAD,
			"Dump date %s does not match expected",
					dumpdate(&grip->dumpdate));
		error(" %s\n", dumpdate(&gip->dumpdate));
		ret = FALSE;
		goto out;
	}
	if (volno != grip->volno) {
		errmsgno(EX_BAD,
			"Volume number %d does not match expected %d\n",
					grip->volno, volno);
		ret = FALSE;
		goto out;
	}
	bytes = stats->Tblocks * (Llong)stats->blocksize + stats->Tparts;
	blockoff = bytes / TBLOCK;
	/*
	 * In case we did start past Volume #1, we need to add
	 * the offset of the first volume we did see.
	 */
	blockoff += gip->blockoff;

	if (grip->blockoff != 0 &&
	    blockoff != grip->blockoff) {
		comerrno(EX_BAD,
			"Volume offset %lld does not match expected %lld\n",
				grip->blockoff, blockoff);
			/* NOTREACHED */
	}

	*skipp += 1 + tarblocks(xlen);

	/*
	 * Hier muß noch ein 'V' Header hin, sonst ist das Archiv nicht
	 * konform zum POSIX Standard.
	 * Alternativ kann aber auch ein 'M'ultivol continuation Header stehen.
	 */
	ptb = (TCB *)&buf[(1 + tarblocks(xlen)) * TBLOCK];

	if (ptb->ustar_dbuf.t_typeflag == 'x') {
		if (pr_validtype(ptb->ustar_dbuf.t_typeflag) &&
		    tarsum_ok(ptb)) {
			stolli(ptb->dbuf.t_size, &ull);
			xlen = ull;
			xlen = 1 + tarblocks(xlen);
			*skipp += xlen;
			ptb = (TCB *)&((char *)ptb)[xlen * TBLOCK];
		}
	}

	if (ptb->ustar_dbuf.t_typeflag == 'V' ||
	    ptb->ustar_dbuf.t_typeflag == 'M') {
		if (pr_validtype(ptb->ustar_dbuf.t_typeflag) &&
		    tarsum_ok(ptb)) {
			*skipp += 1;
		}
	}
	ret = TRUE;

out:
	free_pspace(&finfo.f_pname);
	free_pspace(&finfo.f_plname);
	return (ret);
}

EXPORT char *
dt_name(type)
	int	type;
{
	switch (type) {

	case DT_NONE:		return ("none");
	case DT_FULL:		return ("full");
	case DT_PARTIAL:	return ("partial");
	default:		return ("unknown");
	}
}

EXPORT int
dt_type(name)
	char	*name;
{
	if (streql(name, "none")) {
		return (DT_NONE);
	} else if (streql(name, "full")) {
		return (DT_FULL);
	} else if (streql(name, "partial")) {
		return (DT_PARTIAL);
	} else {
		return (DT_UNKN);
	}
}


EXPORT void
put_release()
{
	if ((props.pr_flags & PR_VU_XHDR) == 0 || props.pr_xc != 'x')
		return;

	/*
	 * We may change this in future when more tar implementations
	 * implement POSIX.1-2001
	 */
	if (H_TYPE(chdrtype) == H_XUSTAR)
		return;

	gen_text("SCHILY.release", vers, (size_t)-1, 0);

	ghdr = TRUE;
}

EXPORT void
put_archtype()
{
	if ((props.pr_flags & PR_VU_XHDR) == 0 || props.pr_xc != 'x')
		return;

	/*
	 * We may change this in future when more tar implementations
	 * implement POSIX.1-2001
	 */
	if (H_TYPE(chdrtype) == H_XUSTAR)
		return;

	gen_text("SCHILY.archtype", hdr_name(chdrtype), (size_t)-1, 0);

	ghdr = TRUE;
}

EXPORT void
put_gvolhdr(name)
	char	*name;
{
	char	nbuf[1024];
extern	BOOL dodump;

	if ((props.pr_flags & PR_VU_XHDR) == 0 || props.pr_xc != 'x')
		return;

	/*
	 * We may change this in future when more tar implementations
	 * implement POSIX.1-2001
	 */
	if (H_TYPE(chdrtype) == H_XUSTAR)
		return;

#ifndef	DEV_MINOR_NONCONTIG
	gen_unumber("SCHILY.devminorbits", minorbits);
#endif

	gip->label = name;
	if (gip->dumplevel >= 0 || dodump > 1) {
		nbuf[0] = '\0';
		gethostname(nbuf, sizeof (nbuf));
		gip->hostname = ___savestr(nbuf);
	}

	if (gip->label)
		gen_text("SCHILY.volhdr.label", gip->label, (size_t)-1, 0);
	if (gip->hostname)
		gen_text("SCHILY.volhdr.hostname", gip->hostname,
							(size_t)-1, 0);
	if (gip->filesys)
		gen_text("SCHILY.volhdr.filesys", gip->filesys, (size_t)-1, 0);
	if (gip->cwd)
		gen_text("SCHILY.volhdr.cwd", gip->cwd, (size_t)-1, 0);
	if (gip->device)
		gen_text("SCHILY.volhdr.device", gip->device, (size_t)-1, 0);

	if (gip->dumptype > 0)
		gen_text("SCHILY.volhdr.dumptype", dt_name(gip->dumptype),
							(size_t)-1, 0);
	if (gip->dumplevel >= 0)
		gen_number("SCHILY.volhdr.dumplevel", gip->dumplevel);
	if (gip->reflevel >= 0)
		gen_number("SCHILY.volhdr.reflevel", gip->reflevel);

	gen_xtime("SCHILY.volhdr.dumpdate", gip->dumpdate.tv_sec, gip->dumpdate.tv_nsec);
	if (gip->refdate.tv_sec)
		gen_xtime("SCHILY.volhdr.refdate", gip->refdate.tv_sec, gip->refdate.tv_nsec);

	if (gip->volno > 0)
		gen_number("SCHILY.volhdr.volno", gip->volno);
	if (gip->blockoff > 0)
		gen_number("SCHILY.volhdr.blockoff", gip->blockoff);
	if (gip->blocksize > 0)
		gen_number("SCHILY.volhdr.blocksize", gip->blocksize);
	if (gip->tapesize > 0)
		gen_number("SCHILY.volhdr.tapesize", gip->tapesize);

	if (binflag)
		gen_text("hdrcharset", "BINARY", (size_t)-1, 0);

	if ((xhsize() + 2 * TBLOCK) > (gip->blocksize * TBLOCK)) {
		errmsgno(EX_BAD, "Panic: Tape record size too small.\n");
		comerrno(EX_BAD, "Panic: Shorten label or increase tape block size.\n");
	}
	ghdr = TRUE;
}

EXPORT void
put_volhdr(name, putv)
	char	*name;
	BOOL	putv;
{
extern	Ullong	tsize;

	if ((multivol || tsize > 0) && name == 0)
		name = "<none>";

	put_gvolhdr(name);

	if (name == 0)
		return;

	if (!putv)
		return;	/* Only a 'g' header is needed */

	put_svolhdr(name);
}

EXPORT void
put_svolhdr(name)
	char	*name;
{
	FINFO	finfo;
	TCB	tb;

	if ((props.pr_flags & PR_VOLHDR) == 0)
		return;

	if (name == 0 || *name == '\0')
		name = "<none>";

	fillbytes((char *)&finfo, sizeof (FINFO), '\0');
	filltcb(&tb);
	finfo.f_name = name;
	finfo.f_namelen = strlen(name);
	finfo.f_xftype = XT_VOLHDR;
	finfo.f_rxftype = XT_VOLHDR;
	finfo.f_mtime = ddate.tv_sec;
	finfo.f_mnsec = ddate.tv_nsec;
	finfo.f_atime = gip->volno;
	finfo.f_ansec = 0;
	finfo.f_tcb = &tb;
	finfo.f_xflags = XF_NOTIME;

	if (!name_to_tcb(&finfo, &tb))	/* Name too long */
		return;

	info_to_tcb(&finfo, &tb);
	put_tcb(&tb, &finfo);
	vprint(&finfo);
}

EXPORT void
put_multhdr(size, off)
	off_t	size;
	off_t	off;
{
extern	BOOL dodump;
	FINFO	finfo;
	TCB	tb;
	TCB	*mptb;
	BOOL	ododump = dodump;

	fillbytes((char *)&finfo, sizeof (finfo), '\0');

	if ((mptb = (TCB *)get_block(TBLOCK)) == NULL)
		mptb = &tb;
	else
		finfo.f_flags |= F_TCB_BUF;
	filltcb(mptb);
	strcpy(mptb->dbuf.t_name, "././@MultHeader");
	finfo.f_mode = TUREAD|TUWRITE;
	finfo.f_size = size;
	finfo.f_rsize = size - off;
	finfo.f_contoffset = off;
	finfo.f_xftype = XT_MULTIVOL;
	finfo.f_rxftype = XT_MULTIVOL;
	if (props.pr_flags & PR_XHDR)
		finfo.f_xflags |= XF_NOTIME|XF_REALSIZE|XF_OFFSET;

	info_to_tcb(&finfo, mptb);

	dodump = FALSE;
	put_tcb(mptb, &finfo);
	dodump = ododump;
}

EXPORT BOOL
get_volhdr(info, vhname)
	FINFO	*info;
	char	*vhname;
{
	error("Volhdr: %s\n", info->f_name);

	if (vhname) {
		if (!streql(info->f_name, vhname))
			return (FALSE);
	}

	return (TRUE);
}

/* ARGSUSED */
LOCAL void
get_label(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	grip->gflags |= GF_LABEL;
	grip->label = ___savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_hostname(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	grip->gflags |= GF_HOSTNAME;
	grip->hostname = ___savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_filesys(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	grip->gflags |= GF_FILESYS;
	grip->filesys = ___savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_cwd(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	grip->gflags |= GF_CWD;
	grip->cwd    = ___savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_device(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	grip->gflags |= GF_DEVICE;
	grip->device = ___savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_dumptype(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	if (len == 0) {
		grip->gflags &= ~GF_DUMPTYPE;
		grip->dumptype = 0;
		return;
	}
	grip->dumptype = dt_type(arg);
	if (grip->dumptype == DT_UNKN)
		errmsgno(EX_BAD, "Unknown dump type '%s'\n", arg);
	else
		grip->gflags |= GF_DUMPTYPE;
}

/* ARGSUSED */
LOCAL void
get_dumplevel(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_DUMPLEVEL;
		grip->dumplevel = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, 1000)) {
		grip->gflags |= GF_DUMPLEVEL;
		grip->dumplevel = ull;
		if (grip->dumplevel != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->dumplevel = 0;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_reflevel(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_REFLEVEL;
		grip->reflevel = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, 1000)) {
		grip->gflags |= GF_REFLEVEL;
		grip->reflevel = ull;
		if (grip->reflevel != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->reflevel = 0;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_dumpdate(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	long	nsec;
	time_t	t;	/* FreeBSD/MacOS X have broken tv_sec/time_t */

	if (len == 0) {
		grip->gflags &= ~GF_DUMPDATE;
		grip->dumpdate.tv_sec  = 0;
		grip->dumpdate.tv_nsec = 0;
		return;
	}
	if (get_xtime(keyword, arg, len, &t, &nsec)) {
		grip->gflags |= GF_DUMPDATE;
		grip->dumpdate.tv_sec  = t;
		grip->dumpdate.tv_nsec = nsec;
	}
}

/* ARGSUSED */
LOCAL void
get_refdate(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	long	nsec;
	time_t	t;	/* FreeBSD/MacOS X have broken tv_sec/time_t */

	if (len == 0) {
		grip->gflags &= ~GF_REFDATE;
		grip->refdate.tv_sec  = 0;
		grip->refdate.tv_nsec = 0;
		return;
	}
	if (get_xtime(keyword, arg, len, &t, &nsec)) {
		grip->gflags |= GF_REFDATE;
		grip->refdate.tv_sec  = t;
		grip->refdate.tv_nsec = nsec;
	}
}

/* ARGSUSED */
LOCAL void
get_volno(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_VOLNO;
		grip->volno = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, INT_MAX)) {
		grip->gflags |= GF_VOLNO;
		grip->volno = ull;
		if (grip->volno != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->volno = 0;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_blockoff(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_BLOCKOFF;
		grip->blockoff = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, ULLONG_MAX)) {
		grip->gflags |= GF_BLOCKOFF;
		grip->blockoff = ull;
		if (grip->blockoff != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->blockoff = 0;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_blocksize(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_BLOCKSIZE;
		grip->blocksize = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, INT_MAX)) {
		grip->gflags |= GF_BLOCKSIZE;
		grip->blocksize = ull;
		if (grip->blocksize != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->blocksize = 0;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_tapesize(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	size_t	len;
{
	Ullong	ull;

	if (len == 0) {
		grip->gflags &= ~GF_TAPESIZE;
		grip->tapesize = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, ULLONG_MAX)) {
		grip->gflags |= GF_TAPESIZE;
		grip->tapesize = ull;
		if (grip->tapesize != ull) {
			xh_rangeerr(keyword, arg, len);
			grip->tapesize = 0;
		}
	}
}
