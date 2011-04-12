/* @(#)hole.c	1.61 11/04/08 Copyright 1993-2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)hole.c	1.61 11/04/08 Copyright 1993-2011 J. Schilling";
#endif
/*
 *	Handle files with holes (sparse files)
 *
 *	Copyright (c) 1993-2011 J. Schilling
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
#include <schily/unistd.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include "star.h"
#include <schily/schily.h>
#include "props.h"
#include "table.h"
#include "starsubs.h"
#include "checkerr.h"
#ifdef	sun
#	include <sys/filio.h>
#	if	_FIOAI == _FIOOBSOLETE67
#	undef	_FIOAI
#	endif
#	ifdef	_FIOAI
#	include <sys/fs/ufs_filio.h>
#	endif
#endif	/* sun */

#ifdef	SEEK_DEBUG
#define	SEEK_DATA	3
#define	SEEK_HOLE	4
#endif

/*
 * Initial sparse (sp_t) array allocation.
 * We currently use SPARSE_IN_HDR + SPARSE_EXT_HDR,
 * if we implement a new POSIX.1-2001 based sparse file handling method, we
 * need to rething this ad it would make sense to start with 512 (8 KB).
 */
#define	NSPARSE_INIT	25

#define	DEBUG
#ifdef DEBUG
#define	EDEBUG(a)	if (debug) error a
#else
#define	EDEBUG(a)
#endif

extern	long	bigcnt;
extern	char	*bigptr;

extern	BOOL	debug;
extern	BOOL	force_hole;
extern	BOOL	nullout;

/*
 * XXX If we have really big files, there could in theory be more than
 * XXX 2 billions of hole/nohole pairs in a file.
 * XXX But if this happens, we would need > 32 GB of RAM for the hole list.
 */
typedef	struct {
	FILE	*fh_file;
	char	*fh_name;
	off_t	fh_size;
	off_t	fh_newpos;
	sp_t	*fh_sparse;
	int	fh_nsparse;
	int	fh_spindex;
	int	fh_diffs;
} fh_t;

LOCAL	int	force_hole_func	__PR((fh_t *fh, char *p, int amount));
EXPORT	int	get_forced_hole	__PR((FILE *f, FINFO *info));
LOCAL	void	bad_sparse_index __PR((fh_t *fh));
LOCAL	int	get_sparse_func	__PR((fh_t *fh, char *p, int amount));
LOCAL	int	get_ssparse_func __PR((fh_t *fh, char *p, int amount));
LOCAL	int	cmp_sparse_func	__PR((fh_t *fh, char *p, int amount));
LOCAL	int	put_sparse_func	__PR((fh_t *fh, char *p, int amount));
LOCAL	sp_t	*grow_sp_list	__PR((sp_t *sparse, int *nspp));
LOCAL	sp_t	*get_sp_list	__PR((FINFO *info, int *nsparsep));
LOCAL	int	mk_sp_list	__PR((int *fp, FINFO *info, sp_t **spp));
#if	defined(SEEK_HOLE) && defined(SEEK_DATA)
LOCAL	int	smk_sp_list	__PR((int f, FINFO *info, sp_t **spp, off_t hpos));
#endif
LOCAL	int	get_end_hole	__PR((fh_t *fh));
LOCAL	int	write_end_hole	__PR((fh_t *fh));
EXPORT	int	gnu_skip_extended __PR((FINFO *info));
EXPORT	int	get_sparse	__PR((FILE *f, FINFO *info));
EXPORT	int	get_as_hole	__PR((FILE *f, FINFO *info));
EXPORT	BOOL	cmp_sparse	__PR((FILE *f, FINFO *info));
EXPORT	void	put_sparse	__PR((int *fp, FINFO *info));
LOCAL	void	put_sp_list	__PR((FINFO *info, sp_t *sparse, int nsparse));
EXPORT	BOOL	sparse_file	__PR((int *fp, FINFO *info));

#define	vp_force_hole_func ((int(*)__PR((void *, char *, int)))force_hole_func)

LOCAL int
force_hole_func(fh, p, amount)
	register fh_t	*fh;
	register char	*p;
		int	amount;
{
	register int	cnt;

	fh->fh_newpos += amount;
	if (amount < fh->fh_size &&
				cmpnullbytes(bigptr, amount) >= amount) {
		if (lseek(fdown(fh->fh_file), fh->fh_newpos, SEEK_SET) == (off_t)-1) {
			if (!errhidden(E_WRITE, fh->fh_name)) {
				if (!errwarnonly(E_WRITE, fh->fh_name))
					xstats.s_rwerrs++;
				errmsg("Error seeking '%s'.\n", fh->fh_name);
				(void) errabort(E_WRITE, fh->fh_name, TRUE);
			}
		}

		fh->fh_size -= amount;
		return (amount);
	}
	cnt = ffilewrite(fh->fh_file, p, amount);
	fh->fh_size -= amount;
	return (cnt);
}

EXPORT BOOL
get_forced_hole(f, info)
	FILE	*f;
	FINFO	*info;
{
	fh_t	fh;

	fh.fh_file = f;
	fh.fh_name = info->f_name;
	fh.fh_size = info->f_rsize;
	fh.fh_newpos = (off_t)0;
	return (xt_file(info, vp_force_hole_func, &fh, TBLOCK, "writing"));
}

LOCAL void
bad_sparse_index(fh)
	register fh_t	*fh;
{
	errmsgno(EX_BAD,
		"Trying to access sparse aray beyond end (index %d).\n",
							fh->fh_spindex);
}

/*
 * Get "amount" bytes from the buffer and write it into the file.
 * If "amount" spans more then a single data chunk in the resulting file,
 * cut it into several chunks that probably start with a seek and get this
 * done by get_ssparse_func().
 */
#define	vp_get_sparse_func ((int(*)__PR((void *, char *, int)))get_sparse_func)
LOCAL int
get_sparse_func(fh, p, amount)
	register fh_t	*fh;
	register char	*p;
	register int	amount;
{
	register int	cnt = 0;
	register int	amt;
	off_t		next_hole;
	off_t		newpos = fh->fh_newpos;

	do {
		if (fh->fh_spindex >= fh->fh_nsparse) {
			seterrno(EX_BAD);
			bad_sparse_index(fh);
			return (-1);
		}
		if (fh->fh_sparse[fh->fh_spindex].sp_offset > newpos) {
			newpos = fh->fh_sparse[fh->fh_spindex].sp_offset;
		} else {
			newpos = fh->fh_newpos;
		}
		next_hole = fh->fh_sparse[fh->fh_spindex].sp_offset +
				fh->fh_sparse[fh->fh_spindex].sp_numbytes;
		amt = amount - cnt;
		if ((newpos + amt) > next_hole)
			amt = next_hole - newpos;
		amt = get_ssparse_func(fh, p, amt);
		if (amt < 0)
			return (-1);
		cnt += amt;
		p += amt;
	} while (cnt < amount);
	return (cnt);
}

/*
 * Get a single chunk of data from the buffer and write it into the file.
 */
LOCAL int
get_ssparse_func(fh, p, amount)
	register fh_t	*fh;
	register char	*p;
		int	amount;
{
	register int	cnt;

	EDEBUG(("amount: %d newpos: %lld index: %d\n",
					amount, (Llong)fh->fh_newpos, fh->fh_spindex));

	if (fh->fh_sparse[fh->fh_spindex].sp_offset > fh->fh_newpos) {

		EDEBUG(("seek to: %lld\n",
				(Llong)fh->fh_sparse[fh->fh_spindex].sp_offset));

		if (lseek(fdown(fh->fh_file),
				fh->fh_sparse[fh->fh_spindex].sp_offset, SEEK_SET) < 0) {
			if (!errhidden(E_WRITE, fh->fh_name)) {
				if (!errwarnonly(E_WRITE, fh->fh_name))
					xstats.s_rwerrs++;
				errmsg("Error seeking '%s'.\n", fh->fh_name);
				(void) errabort(E_WRITE, fh->fh_name, TRUE);
			}
		}

		fh->fh_newpos = fh->fh_sparse[fh->fh_spindex].sp_offset;
	}
	EDEBUG(("write %d at: %lld\n", amount, (Llong)fh->fh_newpos));

	cnt = ffilewrite(fh->fh_file, p, amount);
	fh->fh_size -= cnt;
	fh->fh_newpos += cnt;

	EDEBUG(("off: %lld numb: %lld cnt: %d off+numb: %lld newpos: %lld index: %d\n",
		(Llong)fh->fh_sparse[fh->fh_spindex].sp_offset,
		(Llong)fh->fh_sparse[fh->fh_spindex].sp_numbytes, cnt,
		(Llong)(fh->fh_sparse[fh->fh_spindex].sp_offset +
			fh->fh_sparse[fh->fh_spindex].sp_numbytes),
		(Llong)fh->fh_newpos,
		fh->fh_spindex));

	if ((fh->fh_sparse[fh->fh_spindex].sp_offset +
	    fh->fh_sparse[fh->fh_spindex].sp_numbytes)
	    <= fh->fh_newpos) {

		fh->fh_spindex++;

		EDEBUG(("new index: %d\n", fh->fh_spindex));
	}
	EDEBUG(("return (%d)\n", cnt));
	return (cnt);
}

#define	vp_cmp_sparse_func ((int(*)__PR((void *, char *, int)))cmp_sparse_func)

LOCAL int
cmp_sparse_func(fh, p, amount)
	register fh_t	*fh;
	register char	*p;
		int	amount;
{
	register int	cnt;
		char	*cmp_buf[TBLOCK];

	EDEBUG(("amount: %d newpos: %lld index: %d\n",
					amount,
					(Llong)fh->fh_newpos, fh->fh_spindex));

	if (fh->fh_spindex >= fh->fh_nsparse) {
		seterrno(EX_BAD);
		bad_sparse_index(fh);
		return (-1);
	}

	/*
	 * If we already found diffs we save time and only pass tape ...
	 */
	if (fh->fh_diffs)
		return (amount);

	if (fh->fh_sparse[fh->fh_spindex].sp_offset > fh->fh_newpos) {
		EDEBUG(("seek to: %lld\n",
				(Llong)fh->fh_sparse[fh->fh_spindex].sp_offset));

		while (fh->fh_newpos < fh->fh_sparse[fh->fh_spindex].sp_offset) {
			register int	amt;

			amt = min(TBLOCK,
				fh->fh_sparse[fh->fh_spindex].sp_offset -
				fh->fh_newpos);

			cnt = ffileread(fh->fh_file, cmp_buf, amt);
			if (cnt != amt)
				fh->fh_diffs++;

			if (cmpnullbytes(cmp_buf, amt) < cnt)
				fh->fh_diffs++;

			fh->fh_newpos += cnt;

			if (fh->fh_diffs)
				return (amount);
		}
	}
	EDEBUG(("read %d at: %lld\n", amount, (Llong)fh->fh_newpos));

	cnt = ffileread(fh->fh_file, cmp_buf, amount);
	if (cnt != amount)
		fh->fh_diffs++;

	if (cmpbytes(cmp_buf, p, cnt) < cnt)
		fh->fh_diffs++;

	fh->fh_size -= cnt;
	fh->fh_newpos += cnt;

	EDEBUG(("off: %lld numb: %lld cnt: %d off+numb: %lld newpos: %lld index: %d\n",
		(Llong)fh->fh_sparse[fh->fh_spindex].sp_offset,
		(Llong)fh->fh_sparse[fh->fh_spindex].sp_numbytes, cnt,
		(Llong)(fh->fh_sparse[fh->fh_spindex].sp_offset +
			fh->fh_sparse[fh->fh_spindex].sp_numbytes),
		(Llong)fh->fh_newpos,
		fh->fh_spindex));

	if ((fh->fh_sparse[fh->fh_spindex].sp_offset +
	    fh->fh_sparse[fh->fh_spindex].sp_numbytes)
	    <= fh->fh_newpos) {

		fh->fh_spindex++;

		EDEBUG(("new index: %d\n", fh->fh_spindex));
	}
	EDEBUG(("return (%d) diffs: %d\n", cnt, fh->fh_diffs));
	return (cnt);
}

#define	vp_put_sparse_func ((int(*)__PR((void *, char *, int)))put_sparse_func)

LOCAL int
put_sparse_func(fh, p, amount)
	register fh_t	*fh;
	register char	*p;
		int	amount;
{
	register int	cnt;

	EDEBUG(("amount: %d newpos: %lld index: %d\n",
					amount,
					(Llong)fh->fh_newpos, fh->fh_spindex));

	if (fh->fh_spindex < fh->fh_nsparse &&
		fh->fh_sparse[fh->fh_spindex].sp_offset > fh->fh_newpos) {

		EDEBUG(("seek to: %lld\n",
				(Llong)fh->fh_sparse[fh->fh_spindex].sp_offset));

		if (!nullout &&
		    lseek(*(int *)(fh->fh_file),
				fh->fh_sparse[fh->fh_spindex].sp_offset, SEEK_SET) < 0) {
			if (!errhidden(E_READ, fh->fh_name)) {
				if (!errwarnonly(E_READ, fh->fh_name))
					xstats.s_rwerrs++;
				errmsg("Error seeking '%s'.\n", fh->fh_name);
				(void) errabort(E_READ, fh->fh_name, TRUE);
			}
		}

		fh->fh_newpos = fh->fh_sparse[fh->fh_spindex].sp_offset;
	}
	/*
	 * I former times, cr_file has been called with an amt value of 512,
	 * we now try to optimize the I/O for the data chunk size.
	 * In case we are not yet at the expected end of the file, we limit the
	 * the amount to the end of the current data region, else (when cr_file
	 * tries to find whether the file did grow, we just pass the original.
	 */
	if (fh->fh_spindex < fh->fh_nsparse &&
	    (fh->fh_newpos + amount) >
	    (fh->fh_sparse[fh->fh_spindex].sp_offset +
	    fh->fh_sparse[fh->fh_spindex].sp_numbytes)) {
		amount =   (fh->fh_sparse[fh->fh_spindex].sp_offset +
			    fh->fh_sparse[fh->fh_spindex].sp_numbytes) -
			    fh->fh_newpos;
	}
	EDEBUG(("read %d at: %lld\n", amount, (Llong)fh->fh_newpos));

	if (nullout) {
		cnt = amount;
		if (cnt > fh->fh_size)
			cnt = fh->fh_size;
	} else {
		cnt = _fileread((int *)fh->fh_file, p, amount);
	}
	fh->fh_size -= cnt;
	fh->fh_newpos += cnt;

	EDEBUG(("off: %lld numb: %lld cnt: %d off+numb: %lld newpos: %lld index: %d\n",
		(Llong)fh->fh_sparse[fh->fh_spindex].sp_offset,
		(Llong)fh->fh_sparse[fh->fh_spindex].sp_numbytes, cnt,
		(Llong)(fh->fh_sparse[fh->fh_spindex].sp_offset +
			fh->fh_sparse[fh->fh_spindex].sp_numbytes),
		(Llong)fh->fh_newpos,
		fh->fh_spindex));

	if ((fh->fh_sparse[fh->fh_spindex].sp_offset +
	    fh->fh_sparse[fh->fh_spindex].sp_numbytes)
	    <= fh->fh_newpos) {

		fh->fh_spindex++;

		EDEBUG(("new index: %d\n", fh->fh_spindex));
	}
	EDEBUG(("return (%d)\n", cnt));
	return (cnt);
}

LOCAL sp_t *
grow_sp_list(sparse, nspp)
	sp_t	*sparse;
	int	*nspp;
{
	sp_t	*new;

	if (*nspp < 512)
		*nspp *= 2;
	else
		*nspp += 512;
	new = (sp_t *)realloc(sparse, *nspp*sizeof (sp_t));
	if (new == 0) {
		errmsg("Cannot grow sparse buf.\n");
		free(sparse);
	}
	return (new);
}

LOCAL sp_t *
get_sp_list(info, nsparsep)
	FINFO	*info;
	int	*nsparsep;
{
	TCB	tb;
	TCB	*ptb = info->f_tcb;
	sp_t	*sparse;
	int	nsparse = NSPARSE_INIT;
	int	extended;
	register int	i;
	register int	sparse_in_hdr = props.pr_sparse_in_hdr;
	register int	ind;
		char	*p;
	extern long hdrtype;	/* XXX */

	EDEBUG(("rsize: %lld\n", (Llong)info->f_rsize));

	sparse = (sp_t *)malloc(nsparse*sizeof (sp_t));
	if (sparse == 0) {
		errmsg("Cannot alloc sparse buf.\n");
		*nsparsep = 0;
		return (sparse);
	}

	if (H_TYPE(hdrtype) == H_GNUTAR) {
		p = (char *)ptb->gnu_in_dbuf.t_sp;
	} else {
		p = (char *)ptb->xstar_in_dbuf.t_sp;

		if (ptb->xstar_dbuf.t_prefix[0] == '\0' &&
		    ptb->xstar_in_dbuf.t_sp[0].t_offset[10] != '\0') {
			static	BOOL	warned;
			extern	BOOL	nowarn;

			if (!nowarn && !warned) {
				errmsgno(EX_BAD, "WARNING: Archive uses old sparse format. Please upgrade!\n");
				warned = TRUE;
			}
			sparse_in_hdr = SPARSE_IN_HDR;
		}
	}

	for (i = 0; i < sparse_in_hdr; i++) {
		Ullong	ull;

		stolli(p, &ull); p += 12;
		sparse[i].sp_offset = ull;
		stolli(p, &ull);   p += 12;
		sparse[i].sp_numbytes = ull;
		if (sparse[i].sp_offset == 0 &&
		    sparse[i].sp_numbytes == 0)
			break;
	}
#ifdef	DEBUG
	/* CSTYLED */
	if (debug) for (i = 0; i < sparse_in_hdr; i++) {
		error("i: %d offset: %lld numbytes: %lld\n", i,
				(Llong)sparse[i].sp_offset,
				(Llong)sparse[i].sp_numbytes);
		if (sparse[i].sp_offset == 0 &&
		    sparse[i].sp_numbytes == 0)
			break;
	}
#endif
	ind = sparse_in_hdr-SPARSE_EXT_HDR;

	if (H_TYPE(hdrtype) == H_GNUTAR)
		extended = ptb->gnu_in_dbuf.t_isextended;
	else
		extended = ptb->xstar_in_dbuf.t_isextended;

	extended |= (sparse_in_hdr == 0);
	/*
	 * Set "ind" to allow us to set *nsparsep past while loop.
	 */
	if (!extended) {
		info->f_flags |= F_SP_EXTENDED;
		ind = 0;
	}

	EDEBUG(("isextended: %d\n", extended));

	ptb = &tb;	/* don't destroy orig TCB */
	while (extended) {
		if (readblock((char *)ptb, TBLOCK) == EOF) {
			free(sparse);
			*nsparsep = 0;
			return (0);
		}
		if ((props.pr_flags & PR_GNU_SPARSE_BUG) == 0)
			info->f_rsize -= TBLOCK;

		EDEBUG(("rsize: %lld\n", (Llong)info->f_rsize));

		ind += SPARSE_EXT_HDR;

		EDEBUG(("ind: %d\n", ind));

		if ((ind+SPARSE_EXT_HDR) > nsparse) {
			if ((sparse = grow_sp_list(sparse, &nsparse)) == 0) {
				*nsparsep = 0;
				return ((sp_t *)0);
			}
		}
		p = (char *)ptb;
		for (i = 0; i < SPARSE_EXT_HDR; i++) {
			Ullong	ull;

			stolli(p, &ull);	p += 12;
			sparse[i+ind].sp_offset = ull;
			stolli(p, &ull);	p += 12;
			sparse[i+ind].sp_numbytes = ull;

			EDEBUG(("i: %d offset: %lld numbytes: %lld\n", i,
				(Llong)sparse[i+ind].sp_offset,
				(Llong)sparse[i+ind].sp_numbytes));

			if (sparse[i+ind].sp_offset == 0 &&
			    sparse[i+ind].sp_numbytes == 0)
				break;
		}
		extended = ptb->gnu_ext_dbuf.t_isextended;
	}
	info->f_flags |= F_SP_SKIPPED;
	ind += i;
	*nsparsep = ind;
	EDEBUG(("nsparse: %d\n", ind));
	if (ind < 1)
		return (sparse);
	EDEBUG(("i: %d offset: %lld numbytes: %lld\n", 0,
				(Llong)sparse[0].sp_offset,
				(Llong)sparse[0].sp_numbytes));
	for (i = 1; i < ind; i++) {
		if (((sparse[i-1].sp_offset + sparse[i-1].sp_numbytes) >
							sparse[i].sp_offset) &&
		    (sparse[i].sp_offset != 0 || sparse[i].sp_numbytes != 0)) {
			errmsgno(EX_BAD,
			"Bad sparse data:   offset %lld, numbytes %lld at idx %d.\n",
				(Llong)sparse[i].sp_offset,
				(Llong)sparse[i].sp_numbytes, i);
			errmsgno(EX_BAD, "Current write position is %lld.\n",
				(Llong)(sparse[i-1].sp_offset +
					sparse[i-1].sp_numbytes));
			free(sparse);
			*nsparsep = 0;
			return (0);
		}
		EDEBUG(("i: %d offset: %lld numbytes: %lld\n", i,
				(Llong)sparse[i].sp_offset,
				(Llong)sparse[i].sp_numbytes));
	}
	EDEBUG(("rsize: %lld\n", (Llong)info->f_rsize));

	return (sparse);
}

#if	defined(SEEK_HOLE) && defined(SEEK_DATA)
/*
 * The seek based sparse list parser for files.
 * The interface has been defined in 2004 for Solaris & star and will hopefully
 * be integrated into a future POSIX standard.
 */
LOCAL int
smk_sp_list(f, info, spp, hpos)
	int	f;
	FINFO	*info;
	sp_t	**spp;
	register off_t	hpos;
{
	register off_t	dpos;
	register sp_t	*sparse = *spp;
		int	nsparse = NSPARSE_INIT;
	register int	i = 0;

	*spp = (sp_t *)0;
	if (hpos != 0) {			/* Starts with data */
		if ((hpos % 512) != 0)		/* Unaligned hole ? */
			hpos = (hpos + 511) / 512 * 512;

		if (hpos >= info->f_size) {	/* File has no hole */
			lseek(f, (off_t)0, SEEK_SET);
			free(sparse);
			return (0);
		}
		sparse[i].sp_offset = 0;
		sparse[i].sp_numbytes = hpos;

		EDEBUG(("i: %d offset: %lld numbytes: %lld\n", i,
			(Llong)sparse[i].sp_offset,
			(Llong)sparse[i].sp_numbytes));

		info->f_rsize += sparse[i].sp_numbytes;
		i++;
	}
	while (hpos < info->f_size) {
		dpos = lseek(f, hpos, SEEK_DATA);
		if (dpos == (off_t)-1 && geterrno() == ENXIO) {
			/*
			 * Catch the case "No more data past 'hpos'".
			 */
			dpos = info->f_size;
		} else if (dpos == (off_t)-1) {
			if (!errhidden(E_SHRINK, info->f_name)) {
				if (!errwarnonly(E_SHRINK, info->f_name))
					xstats.s_sizeerrs++;
				errmsg(
				"'%s': SEEK_DATA file shrunk at offset %lld.\n",
				info->f_name, (Llong)hpos);
				(void) errabort(E_SHRINK, info->f_name, TRUE);
			}
			lseek(f, (off_t)0, SEEK_SET);
			free(sparse);
			return (0);
		}
		hpos = lseek(f, dpos, SEEK_HOLE);
		if (hpos == (off_t)-1 && geterrno() == ENXIO) {
			/*
			 * Catch the case "No more holes past 'dpos'".
			 */
			hpos = info->f_size;
		} else if (hpos == (off_t)-1) {
			if (!errhidden(E_SHRINK, info->f_name)) {
				if (!errwarnonly(E_SHRINK, info->f_name))
					xstats.s_sizeerrs++;
				errmsg(
				"'%s': SEEK_HOLE file shrunk at offset %lld.\n",
				info->f_name, (Llong)dpos);
				(void) errabort(E_SHRINK, info->f_name, TRUE);
			}
			lseek(f, (off_t)0, SEEK_SET);
			free(sparse);
			return (0);
		}
		if (dpos < info->f_size &&	/* Not the hole at EOF ? */
		    (dpos % 512) != 0) {	/* Unaligned data ? */
			dpos = dpos / 512 * 512;
			if (i > 0 &&		/* Combine with previous ? */
			    (sparse[i-1].sp_offset +
			    sparse[i-1].sp_numbytes) >= dpos) {
				i--;
				dpos = sparse[i].sp_offset;
			}
		}
		if (hpos < info->f_size &&	/* Not the hole at EOF ? */
		    (hpos % 512) != 0) {	/* Unaligned hole ? */
			hpos = (hpos + 511) / 512 * 512;
			if (hpos > info->f_size)
				hpos = info->f_size;
		}

		sparse[i].sp_offset = dpos;
		sparse[i].sp_numbytes = hpos - dpos;

		info->f_rsize += sparse[i].sp_numbytes;

		EDEBUG(("i: %d offset: %lld numbytes: %lld\n", i,
			(Llong)sparse[i].sp_offset,
			(Llong)sparse[i].sp_numbytes));

		i++;
		if (i >= nsparse) {
			if ((sparse = grow_sp_list(sparse,
					&nsparse)) == 0) {
				lseek(f, (off_t)0, SEEK_SET);
				return (0);
			}
		}
	}
	lseek(f, (off_t)0, SEEK_SET);
	*spp = sparse;
	return (i);
}
#endif

LOCAL int
mk_sp_list(fp, info, spp)
	int	*fp;
	FINFO	*info;
	sp_t	**spp;
{
	long	rbuf[32*1024/sizeof (long)];	/* Force it be long aligned */
	sp_t	*sparse;
	int	nsparse = NSPARSE_INIT;
	register int	amount = 0;
	register int	cnt = 0;
	register char	*rbp = (char *)rbuf;
	register off_t	pos = (off_t)0;
	register int	i = 0;
	register BOOL	data = FALSE;
	register BOOL	use_ai = FALSE;
	register BOOL	is_hole = FALSE;
#if	defined(SEEK_HOLE) && defined(SEEK_DATA)
#else
#ifdef	_FIOAI
		int	fai_idx;
	struct fioai	fai;
	struct fioai	*faip;
#	define	NFAI	1024
	daddr_t		fai_arr[NFAI];
#endif	/* _FIOAI */
#endif	/* SEEK_HOLE */

	*spp = (sp_t *)0;
	info->f_rsize = 0;
	sparse = (sp_t *)malloc(nsparse*sizeof (sp_t));
	if (sparse == 0) {
		errmsg("Cannot alloc sparse buf.\n");
		return (i);
	}

	/*
	 * Shortcut for files which consist of a single hole and no written
	 * data. In that case on some file systems a file may occupy 0 blocks
	 * on disk, and the F_ALL_HOLE flag could be set in info->f_flags. In
	 * such a case, we can avoid scanning the file on dumb operating
	 * systems that do not support SEEK_HOLE nor something equivalent.
	 */
	if (info->f_flags & F_ALL_HOLE) {
		pos = info->f_size;
		goto scan_done;
	}

#if	defined(SEEK_HOLE) && defined(SEEK_DATA)
	/*
	 * Error codes: EINVAL -> OS does not support SEEK_HOLE
	 * ENOTSUP -> Current filesystem does not support SEEK_HOLE.
	 */
	if (force_hole)
		pos = (off_t)-1;
	else
		pos = lseek(*fp, (off_t)0, SEEK_HOLE);
	if (pos != (off_t)-1) {			/* Use if operational */
		*spp = sparse;
		return (smk_sp_list(*fp, info, spp, pos));
	}
	pos = (off_t)0;
#else
#ifdef	_FIOAI
	fai.fai_off = 0;
	fai.fai_size = 512;
	fai.fai_num = 1;
	fai.fai_daddr = fai_arr;
	use_ai = ioctl(*fp, _FIOAI, &fai) >= 0;	/* Use if operational */
	fai_idx = 0;
	fai.fai_num = 0;			/* start at 0 again  */
#endif	/* _FIOAI */
#endif	/* SEEK_HOLE */

	for (;;) {
		if (use_ai) {
#if	defined(SEEK_HOLE) && defined(SEEK_DATA)
			/* EMPTY */
#else
#ifdef	_FIOAI
			if (fai_idx >= fai.fai_num) {
				fai.fai_off = pos;
				fai.fai_size = 512 * NFAI;
				fai.fai_num = NFAI;
			retry:
				ioctl(*fp, _FIOAI, &fai);
				if (fai.fai_num == 0) {
					/*
					 * Check whether we crossed the end of
					 * the file or off_t wrapps.
					 */
					if (pos < lseek(*fp, (off_t)0, SEEK_END)) {
						off_t	di;

						di = lseek(*fp, (off_t)0, SEEK_END);
						di -= pos;
						fai.fai_off = pos;
						fai.fai_size = (di/512) * 512;
						fai.fai_num = di/512;
						if (fai.fai_num != 0)
							goto retry;
						if (di > 0 && di < 512) {
							/*
							 * The last entry
							 * cannot be checked
							 * via the ooctl.
							 */
							fai.fai_size = 512;
							fai.fai_num = 1;
							fai.fai_daddr[0] = !_FIOAI_HOLE;
							goto fake;
						}
					}
					break;
				}
			fake:
				fai_idx = 0;
			}
			is_hole = fai.fai_daddr[fai_idx++] == _FIOAI_HOLE;

			amount = 512;
			if (pos + amount < 0)
				amount = info->f_size - pos;
			else if (pos + amount > info->f_size)
				amount = info->f_size - pos;
#else
			/* EMPTY */
#endif	/* _FIOAI */
#endif	/* SEEK_HOLE */
		} else {
			if (cnt <= 0) {
				if ((cnt = _fileread(fp, rbuf, sizeof (rbuf))) == 0)
					break;
				if (cnt < 0) {
					errmsg(
					"Error scanning for holes in '%s'.\n",
					info->f_name);

					errmsg("Current pos: %lld\n",
					(Llong)lseek(*fp, (off_t)0, SEEK_CUR));

					free(sparse);
					return (0);
				}
				rbp = (char *)rbuf;
			}
			if ((amount = cmpnullbytes(rbp, cnt)) >= cnt) {
				is_hole = TRUE;
				cnt = 0;
			} else {
				amount = (amount / TBLOCK) * TBLOCK;
				if ((is_hole = amount != 0) == FALSE) {
					amount += TBLOCK;
					if (amount > cnt)
						amount = cnt;
				}
				rbp += amount;
				cnt -= amount;
			}
		}

		if (is_hole) {
			if (data) {
				sparse[i].sp_numbytes =
						pos - sparse[i].sp_offset;
				info->f_rsize += sparse[i].sp_numbytes;

				EDEBUG(("i: %d offset: %lld numbytes: %lld\n", i,
					(Llong)sparse[i].sp_offset,
					(Llong)sparse[i].sp_numbytes));

				data = FALSE;
				i++;
				if (i >= nsparse) {
					if ((sparse = grow_sp_list(sparse,
							&nsparse)) == 0) {
						lseek(*fp, (off_t)0, SEEK_SET);
						return (0);
					}
				}
			}
		} else {
			if (!data) {
				sparse[i].sp_offset = pos;
				data = TRUE;
			}
		}
		pos += amount;
	}

scan_done:
	EDEBUG(("data: %d\n", data));

	if (data) {
		sparse[i].sp_numbytes = pos - sparse[i].sp_offset;
		info->f_rsize += sparse[i].sp_numbytes;

		EDEBUG(("i: %d offset: %lld numbytes: %lld\n", i,
				(Llong)sparse[i].sp_offset,
				(Llong)sparse[i].sp_numbytes));
	} else {
#ifdef	NO_HOLE_AT_END
		sparse[i].sp_offset = pos -1;
		sparse[i].sp_numbytes = 1;
		info->f_rsize += 1;
#else
		sparse[i].sp_offset = pos;
		sparse[i].sp_numbytes = 0;
#endif
		EDEBUG(("i: %d offset: %lld numbytes: %lld\n", i,
				(Llong)sparse[i].sp_offset,
				(Llong)sparse[i].sp_numbytes));
	}
	lseek(*fp, (off_t)0, SEEK_SET);
	*spp = sparse;
	return (++i);
}

LOCAL int
get_end_hole(fh)
	register fh_t	*fh;
{
#ifdef	HAVE_FTRUNCATE
	int	f;
	off_t	off;
	int	add = 0;
#endif

	/*
	 * At end of sparse array: return TRUE.
	 * There is no "end hole" in this case.
	 */
	if (fh->fh_spindex == fh->fh_nsparse)
		return (TRUE);
	/*
	 * Out of sparse array: return FALSE.
	 */
	if (fh->fh_spindex > fh->fh_nsparse)
		return (0);

	/*
	 * New offset is not > fh->fh_newpos: return.
	 */
	if (fh->fh_sparse[fh->fh_spindex].sp_offset <= fh->fh_newpos)
		return (0);

	/*
	 * No hole at end of file: return.
	 */
	if (fh->fh_sparse[fh->fh_spindex].sp_numbytes != 0)
		return (0);

#ifdef	HAVE_FTRUNCATE
	/*
	 * In order to prevent Solaris from allocating space at the end of the
	 * file we need to shrink the file. For this reason, we first create
	 * the file a bit too large.
	 */
	f = fdown(fh->fh_file);
	off = fh->fh_sparse[fh->fh_spindex].sp_offset;

#ifdef	_PC_MIN_HOLE_SIZE
	add = fpathconf(f, _PC_MIN_HOLE_SIZE);
#endif
	if (add <= 0)
		add = 8192;

	if ((OFF_T_MAX - off) > add)
		(void) ftruncate(f, off+add);

	if (ftruncate(f, off) < 0)
		return (write_end_hole(fh));
	return (TRUE);
#else
	return (write_end_hole(fh));
#endif
}

LOCAL int
write_end_hole(fh)
	register fh_t	*fh;
{
	int	f;
	off_t	off;

	f = fdown(fh->fh_file);
	off = fh->fh_sparse[fh->fh_spindex].sp_offset;

	seterrno(0);
	if (lseek(f, off-1, SEEK_SET) == (off_t)-1) {
		if (!errhidden(E_WRITE, fh->fh_name)) {
			if (!errwarnonly(E_WRITE, fh->fh_name))
				xstats.s_rwerrs++;
			errmsg("Error seeking '%s'.\n", fh->fh_name);
			(void) errabort(E_WRITE, fh->fh_name, TRUE);
			return (FALSE);
		}
	} else if (ffilewrite(fh->fh_file, "", 1) != 1) {
		if (!errhidden(E_WRITE, fh->fh_name)) {
			if (!errwarnonly(E_WRITE, fh->fh_name))
				xstats.s_rwerrs++;
			errmsg("Error writing '%s'.\n", fh->fh_name);
			(void) errabort(E_WRITE, fh->fh_name, TRUE);
			return (FALSE);
		}
	}
	return (TRUE);
}

EXPORT int
gnu_skip_extended(info)
	FINFO	*info;
{
	TCB	*ptb;

	if (info->f_flags & F_SP_SKIPPED)
		return (0);

	ptb = info->f_tcb;
	if (((info->f_flags & F_SP_EXTENDED) != 0) ||
	    ptb->gnu_in_dbuf.t_isextended) do {
		info->f_flags |= F_SP_EXTENDED;
		if (readblock((char *)ptb, TBLOCK) == EOF)
			return (EOF);
	} while (ptb->gnu_ext_dbuf.t_isextended);

	info->f_flags |= F_SP_SKIPPED;
	return (0);
}

EXPORT int
get_sparse(f, info)
	FILE	*f;
	FINFO	*info;
{
	fh_t	fh;
	sp_t	*sparse;
	int	nsparse;
	int	ret;

	sparse = get_sp_list(info, &nsparse);
	if (sparse == 0) {
		errmsgno(EX_BAD, "Skipping '%s' sorry ...\n", info->f_name);
		errmsgno(EX_BAD, "WARNING: '%s' is damaged\n", info->f_name);
		void_file(info);
		return (FALSE);
	}
	fh.fh_file = f;
	fh.fh_name = info->f_name;
	fh.fh_size = info->f_rsize;
	fh.fh_newpos = (off_t)0;
	fh.fh_sparse = sparse;
	fh.fh_nsparse = nsparse;
	fh.fh_spindex = 0;
	ret = xt_file(info, vp_get_sparse_func, &fh, 0, "writing");
	if (ret == TRUE)
		ret = get_end_hole(&fh);
	free(sparse);
	return (ret);
}

EXPORT int
get_as_hole(f, info)
	FILE	*f;
	FINFO	*info;
{
	fh_t	fh;
	sp_t	sparse;

	sparse.sp_offset = info->f_size;
	sparse.sp_numbytes = 0;

	fh.fh_file = f;
	fh.fh_name = info->f_name;
	fh.fh_size = info->f_rsize;
	fh.fh_newpos = (off_t)0;
	fh.fh_sparse = &sparse;
	fh.fh_nsparse = 1;
	fh.fh_spindex = 0;

	return (get_end_hole(&fh));
}

EXPORT BOOL
cmp_sparse(f, info)
	FILE	*f;
	FINFO	*info;
{
	fh_t	fh;
	sp_t	*sparse;
	int	nsparse;

	sparse = get_sp_list(info, &nsparse);
	if (sparse == 0) {
		errmsgno(EX_BAD, "Skipping '%s' sorry ...\n", info->f_name);
		void_file(info);
		fclose(f);
		return (FALSE);
	}
	fh.fh_file = f;
	fh.fh_name = info->f_name;
	fh.fh_size = info->f_rsize;
	fh.fh_newpos = (off_t)0;
	fh.fh_sparse = sparse;
	fh.fh_nsparse = nsparse;
	fh.fh_spindex = 0;
	fh.fh_diffs = 0;
	if (xt_file(info, vp_cmp_sparse_func, &fh, TBLOCK, "reading") < 0)
		die(EX_BAD);
	if (fclose(f) != 0) {
		if (!errhidden(E_READ, info->f_name)) {
			if (!errwarnonly(E_SHRINK, info->f_name))
				xstats.s_rwerrs++;
			(void) errabort(E_READ, info->f_name, TRUE);
		}
	}
	free(sparse);
	return (fh.fh_diffs == 0);
}

EXPORT void
put_sparse(fp, info)
	int	*fp;
	FINFO	*info;
{
	fh_t	fh;
	sp_t	*sparse;
	int	nsparse;
	off_t	rsize;

	nsparse = mk_sp_list(fp, info, &sparse);
	if (debug)
		error("numsparse: %d\n", nsparse);
	if (nsparse == 0) {
		info->f_rsize = info->f_size;
		put_tcb(info->f_tcb, info);
		vprint(info);
		errmsgno(EX_BAD, "Dumping SPARSE '%s' as file\n", info->f_name);
		put_file(fp, info);
		return;
	}
	rsize = info->f_rsize;

	EDEBUG(("rsize: %lld\n", (Llong)rsize));

	put_sp_list(info, sparse, nsparse);
	fh.fh_file = (FILE *)fp;
	fh.fh_name = info->f_name;
	fh.fh_size = info->f_rsize = rsize;
	fh.fh_newpos = (off_t)0;
	fh.fh_sparse = sparse;
	fh.fh_nsparse = nsparse;
	fh.fh_spindex = 0;
	cr_file(info, vp_put_sparse_func, &fh, 0, "reading");
	free(sparse);
}

LOCAL void
put_sp_list(info, sparse, nsparse)
	FINFO	*info;
	sp_t	*sparse;
	int	nsparse;
{
	register int	i;
	register int	sparse_in_hdr = props.pr_sparse_in_hdr;
	register char	*p;
		TCB	tb;
		TCB	*ptb = info->f_tcb;
	extern long hdrtype;	/* XXX */

	EDEBUG(("1nsparse: %d rsize: %lld\n", nsparse, (Llong)info->f_rsize));

	if (nsparse > sparse_in_hdr) {
		if ((props.pr_flags & PR_GNU_SPARSE_BUG) == 0)
			info->f_rsize +=
				(((nsparse-sparse_in_hdr)+SPARSE_EXT_HDR-1)/
				SPARSE_EXT_HDR)*TBLOCK;
	}
	EDEBUG(("2nsparse: %d rsize: %lld added: %d\n", nsparse, (Llong)info->f_rsize,
				(((nsparse-sparse_in_hdr)+SPARSE_EXT_HDR-1)/
				SPARSE_EXT_HDR)*TBLOCK));
	EDEBUG(("addr sp: %zd\n", (size_t)&((TCB *)0)->xstar_in_dbuf.t_sp));
	EDEBUG(("addr rs: %zd\n", (size_t)&((TCB *)0)->xstar_in_dbuf.t_realsize));
	EDEBUG(("flags: 0x%lX\n", info->f_flags));

	info->f_rxftype = info->f_xftype = XT_SPARSE;
	if (info->f_flags & F_SPLIT_NAME && props.pr_nflags & PR_PREFIX_REUSED)
		tcb_undo_split(ptb, info);

	info->f_xflags &= ~XF_SIZE;	/* Condensed file is smaller	*/
	info_to_tcb(info, ptb);		/* Re-Compute anything		*/

	if (H_TYPE(hdrtype) == H_GNUTAR)
		p = (char *)ptb->gnu_in_dbuf.t_sp;
	else
		p = (char *)ptb->xstar_in_dbuf.t_sp;
	for (i = 0; i < sparse_in_hdr && i < nsparse; i++) {
		llitos(p, (Ullong)sparse[i].sp_offset, 11);   p += 12;
		llitos(p, (Ullong)sparse[i].sp_numbytes, 11); p += 12;
	}
	if (sparse_in_hdr > 0 && nsparse > sparse_in_hdr) {
		if (H_TYPE(hdrtype) == H_GNUTAR)
			ptb->gnu_in_dbuf.t_isextended = 1;
		else
			ptb->xstar_in_dbuf.t_isextended = '1';
	}

	put_tcb(ptb, info);
	vprint(info);

	nsparse -= sparse_in_hdr;
	sparse += sparse_in_hdr;
	ptb = &tb;
	while (nsparse > 0) {
		filltcb(ptb);
		p = (char *)ptb;
		for (i = 0; i < SPARSE_EXT_HDR && i < nsparse; i++) {
			llitos(p, (Ullong)sparse[i].sp_offset, 11);   p += 12;
			llitos(p, (Ullong)sparse[i].sp_numbytes, 11); p += 12;
		}
		nsparse -= SPARSE_EXT_HDR;
		sparse += SPARSE_EXT_HDR;
		if (nsparse > 0)
			ptb->gnu_ext_dbuf.t_isextended = '1';
		writeblock((char *)ptb);
	}
}

EXPORT BOOL
sparse_file(fp, info)
	int	*fp;
	FINFO	*info;
{
#if	defined(SEEK_HOLE) && defined(SEEK_DATA)
	off_t	pos;
	int	f = *fp;

	/*
	 * If we have been compiled on an OS that supports SEEK_HOLE but run
	 * on an OS that does not support SEEK_HOLE, we get EINVAL.
	 * If the underlying filesystem does not support the SEEK_HOLE call,
	 * we get ENOTSUP. In all other cases, we will get either the position
	 * of the first real hole in the file or statb.st_size in case the file
	 * definitely has no holes.
	 */
	pos = lseek(f, (off_t)0, SEEK_HOLE);	/* Check for first hole	   */
	if (pos == (off_t)-1)			/* SEEK_HOLE not supported */
		return ((info->f_flags & F_SPARSE) != 0);

	if (pos != 0)				/* Not at pos 0: seek back */
		(void) lseek(f, 0, SEEK_SET);


	if (pos >= info->f_size) {		/* Definitely not sparse */
#ifdef	_PC_MIN_HOLE_SIZE
		/*
		 * If we are on an unfixed Solaris kernel and if the
		 * underlying filesystem does not support SEEK_HOLE,
		 * the file may be sparse, but SEEK_HOLE returns info->f_size.
		 * Fortunately, the fpathconf call only takes 1usec on a 550MHz
		 * Pentium III, so this call typically takes less than 1% of
		 * the total system time.
		 */
		if (fpathconf(f, _PC_MIN_HOLE_SIZE) < 0)
			return ((info->f_flags & F_SPARSE) != 0);
#endif
		return (FALSE);
	}
	return (TRUE);				/* Definitely sparse */
#else
	return ((info->f_flags & F_SPARSE) != 0);
#endif
}
