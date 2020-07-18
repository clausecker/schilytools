/* @(#)cpiohdr.c	1.35 20/07/08 Copyright 1994-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cpiohdr.c	1.35 20/07/08 Copyright 1994-2020 J. Schilling";
#endif
/*
 *	Handling routines to read/write, parse/create
 *	cpio archive headers
 *
 *	Copyright (c) 1994-2020 J. Schilling
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
#include "star.h"
#include "props.h"
#include "table.h"
#include <schily/dirent.h>
#include <schily/standard.h>
#include <schily/string.h>
#define	__XDEV__	/* Needed to activate _dev_major()/_dev_minor() */
#include <schily/device.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/intcvt.h>
#include "starsubs.h"
#include "movearch.h"
#include "fifo.h"

extern	long	hdrtype;
extern	BOOL	ignoreerr;
extern	BOOL	link_dirs;

EXPORT	void	put_cpioh	__PR((TCB *ptb, FINFO *info));
EXPORT	void	cpio_weof	__PR((void));
EXPORT	void	cpioinfo_to_tcb	__PR((FINFO *info, TCB *ptb));
EXPORT	int	cpiotcb_to_info	__PR((TCB *ptb, FINFO *info));
EXPORT	void	cpio_resync	__PR((void));
EXPORT	int	cpio_checkswab	__PR((TCB *ptb));
LOCAL	Int32_t	cpio_cksum	__PR((char *name));
LOCAL	void	astoo_cpio	__PR((char *s, Ulong *l, int cnt));
LOCAL	void	astoh_cpio	__PR((char *s, Ulong *l, int cnt));
LOCAL	void	litoh_cpio	__PR((char *s, Ulong l, int fieldw));

EXPORT void
put_cpioh(ptb, info)
	TCB	*ptb;
	register FINFO	*info;
{
	move_t	move;
	FINFO	ninfo;

	if (is_file(info) && H_TYPE(hdrtype) == H_CPIO_ACRC) {
		info->f_flags |= F_CRC;
		cpioinfo_to_tcb(info, ptb);
	}
	if ((info->f_flags & F_TCB_BUF) != 0) {	/* TCB is on buffer */
		put_block(props.pr_hdrsize);
	} else {
		size_t	i;
	extern char	*bigptr;

		/*
		 * This is a writeblock() implementation that
		 * deals with buffer wrap around problems.
		 */
		i = buf_wait(props.pr_hdrsize);
		if (i < props.pr_hdrsize) {
			int	rest;

			movebytes(ptb, bigptr, i);
			buf_wake(i);
			rest = props.pr_hdrsize - i;
			buf_wait(rest);
			movebytes(((char *)ptb)+i, bigptr, rest);
			buf_wake(rest);
		} else {
			movebytes(ptb, bigptr, props.pr_hdrsize);
			buf_wake(props.pr_hdrsize);
		}
	}

	/*
	 * If we decide to be able to auto-add a slash at the end of a
	 * directory we need to change name_to_tcb() in longname.c
	 */
	fillbytes(&ninfo, sizeof (ninfo), '\0');
	move.m_data = info->f_name;
	move.m_flags = 0;			/* No MF_ADDSLASH */
	move.m_size = info->f_namelen + 1;
	ninfo.f_rsize = info->f_namelen + 1;
	ninfo.f_flags |= F_LONGNAME;
	ninfo.f_name = "file name";
	cr_file(&ninfo, vp_move_to_arch, &move, 0, "moving file name");

	if (is_symlink(info)) {
		move.m_data = info->f_lname;
		move.m_size = info->f_lnamelen;
		ninfo.f_rsize = info->f_lnamelen;
		ninfo.f_flags = 0;
		cr_file(&ninfo, vp_move_to_arch, &move, 0, "moving link name");
	}
}

EXPORT void
cpio_weof()
{
		TCB	tb;
	register TCB	*ptb;
		FINFO	ninfo;

	fillbytes(&ninfo, sizeof (ninfo), '\0');

	if ((ptb = (TCB *)get_block(props.pr_hdrsize)) == NULL)
		ptb = &tb;
	else
		ninfo.f_flags |= F_TCB_BUF;

	ninfo.f_name = "TRAILER!!!";
	ninfo.f_namelen = 10;
	ninfo.f_nlink = 1;
	cpioinfo_to_tcb(&ninfo, ptb);
	put_cpioh(ptb, &ninfo);
}

EXPORT void
cpioinfo_to_tcb(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
	register char	*p = (char *)ptb;
		nlink_t	nlink = info->f_nlink;
		Ulong	crc = 0L;
		Int16_t	s;
		UInt16_t us;
		Int32_t	l;


	/*
	 * The header is rewritten later in read_link()
	 * with a new unique st_dev/st_ino.
	 */
	if (!link_dirs && is_dir(info))
		nlink = 1;

	switch (H_TYPE(hdrtype)) {

	case H_CPIO_BIN:
		/*
		 * We always create binary archives in network byte order.
		 */
		*p++ = '\161'; *p++ = '\307';
		s = info->f_dev;
		i_to_2_byte(p, s); p += 2;
		us = info->f_ino;
		i_to_2_byte(p, us); p += 2;
		us = info->f_type|info->f_mode;
		i_to_2_byte(p, us); p += 2;
		us = info->f_uid;
		i_to_2_byte(p, us); p += 2;
		us = info->f_gid;
		i_to_2_byte(p, us); p += 2;
		s = info->f_nlink;
		i_to_2_byte(p, s); p += 2;
		s = info->f_rdev;
		i_to_2_byte(p, s); p += 2;
		l = info->f_mtime;
		i_to_4_byte(p, l); p += 4;
		s = info->f_namelen+1;
		i_to_2_byte(p, s); p += 2;
		if (is_symlink(info))
			l = info->f_lnamelen;
		else
			l = info->f_rsize;
		i_to_4_byte(p, l); p += 4;
		break;

	case H_CPIO_CHR:
	case H_CPIO_ODC:
		*p++ = '0'; *p++ = '7';
		*p++ = '0'; *p++ = '7';
		*p++ = '0'; *p++ = '7';
		llitos(&((char *)ptb)[6], (Ullong)info->f_dev, 6);
		llitos(&((char *)ptb)[12], (Ullong)info->f_ino, 6);
		llitos(&((char *)ptb)[18], (Ullong)(info->f_type|info->f_mode), 6);
		llitos(&((char *)ptb)[24], (Ullong)info->f_uid, 6);
		llitos(&((char *)ptb)[30], (Ullong)info->f_gid, 6);
		llitos(&((char *)ptb)[36], (Ullong)nlink, 6);
		llitos(&((char *)ptb)[42], (Ullong)info->f_rdev, 6);
		llitos(&((char *)ptb)[48], (Ullong)info->f_mtime, 11);
		llitos(&((char *)ptb)[59], (Ullong)info->f_namelen+1, 6);
		if (is_symlink(info))
			llitos(&((char *)ptb)[65], (Ullong)info->f_lnamelen, 11);
		else
			llitos(&((char *)ptb)[65], (Ullong)info->f_rsize, 11);
		break;

	case H_CPIO_ASC:
	case H_CPIO_ACRC:
		*p++ = '0'; *p++ = '7';
		*p++ = '0'; *p++ = '7';
		if (H_TYPE(hdrtype) == H_CPIO_ASC) {
			*p++ = '0'; *p++ = '1';
		} else {
			*p++ = '0'; *p++ = '2';
		}
		litoh_cpio(&((char *)ptb)[6], (Ulong)info->f_ino, 8);
		litoh_cpio(&((char *)ptb)[14], (Ulong)(info->f_type|info->f_mode), 8);
		litoh_cpio(&((char *)ptb)[22], (Ulong)info->f_uid, 8);
		litoh_cpio(&((char *)ptb)[30], (Ulong)info->f_gid, 8);
		litoh_cpio(&((char *)ptb)[38], (Ulong)nlink, 8);
		litoh_cpio(&((char *)ptb)[46], (Ulong)info->f_mtime, 8);
		if (is_symlink(info))
			litoh_cpio(&((char *)ptb)[54], (Ulong)info->f_lnamelen, 8);
		else
			litoh_cpio(&((char *)ptb)[54], (Ulong)info->f_rsize, 8);
		litoh_cpio(&((char *)ptb)[62], (Ulong)major(info->f_dev), 8);
		litoh_cpio(&((char *)ptb)[70], (Ulong)minor(info->f_dev), 8);
		litoh_cpio(&((char *)ptb)[78], (Ulong)info->f_rdevmaj, 8);
		litoh_cpio(&((char *)ptb)[86], (Ulong)info->f_rdevmin, 8);
		litoh_cpio(&((char *)ptb)[94], (Ulong)info->f_namelen+1, 8);
		if (is_file(info) && info->f_flags & F_CRC)
			crc = cpio_cksum(info->f_name);
		litoh_cpio(&((char *)ptb)[102], crc, 8);
		break;

	default:
		errmsgno(EX_BAD, "Found CPIO type %ld/%d ", hdrtype, H_TYPE(hdrtype));
		print_hdrtype(stderr, hdrtype);
		comerrno(EX_BAD, "Can't handle this cpio archive type (yet).\n");
	}
}

/*
 * CPIO header offsets:
 *
 *	-Hodc (POSIX)
 *	c_magic[6]		0
 *	c_dev[6]		6
 *	c_ino[6]		12
 *	c_mode[6]		18
 *	c_uid[6]		24
 *	c_gid[6]		30
 *	c_nlink[6]		36
 *	c_rdev[6]		42
 *	c_mtime[11]		48
 *	c_namesize[6]		59
 *	c_filesize[11]		65
 *	c_name[]		76
 *
 *	-Hcrc
 *	E_magic[6]		0
 *	E_ino[8]		6
 *	E_mode[8]		14
 *	E_uid[8]		22
 *	E_gid[8]		30
 *	E_nlink[8]		38
 *	E_mtime[8]		46
 *	E_filesize[8]		54
 *	E_maj[8]		62
 *	E_min[8]		70
 *	E_rmaj[8]		78
 *	E_rmin[8]		86
 *	E_namesize[8]		94
 *	E_chksum[8]		102
 *	E_name[]		110
 *
 *	Binary V7 default
 *	h_magic[2]		0
 *	h_dev[2]		2
 *	h_ino[2]		4
 *	h_mode[2]		6
 *	h_uid[2]		8
 *	h_gid[2]		10
 *	h_nlink[2]		12
 *	h_rdev[2]		14
 *	h_mtime[4]		16
 *	h_namesize[2]		20
 *	h_filesize[4]		22
 *
 */

EXPORT int
cpiotcb_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	Ulong	ul;
	move_t	move;
	long	l1;
	long	l2;
	BOOL	swapped = FALSE;
	char	binhdr[CPIOBIN_HDRSZ];


	switch (H_TYPE(hdrtype)) {

	case H_CPIO_BIN:
		l1 = (((char *)ptb)[20] & 0xFF) * 256 + (((char *)ptb)[21] & 0xFF);
		l2 = (((char *)ptb)[21] & 0xFF) * 256 + (((char *)ptb)[20] & 0xFF);
		if (l1 <= 257 || l2 <= 257) {
			if (l2 <= 257)
				swapped = TRUE;
		}
		if (swapped) {
			movebytes(ptb, binhdr, CPIOBIN_HDRSZ);
			ptb = (TCB *)binhdr;
			swabbytes((char *)ptb, props.pr_hdrsize);
		}

		l1 = ((char *)ptb)[2] * 256 + (((char *)ptb)[3] & 0xFF);
		info->f_dev = l1;
		ul = (((char *)ptb)[4] & 0xFF) * 256 + (((char *)ptb)[5] & 0xFF);
		info->f_ino = ul;
		ul = (((char *)ptb)[6] & 0xFF) * 256 + (((char *)ptb)[7] & 0xFF);
		info->f_mode = ul;
		info->f_type = info->f_mode & S_IFMT;
		info->f_mode = info->f_mode & 07777;
		info->f_rxftype = info->f_xftype = IFTOXT(info->f_type);
		info->f_filetype = XTTOST(info->f_xftype);
		info->f_typeflag = XTTOUS(info->f_xftype);
		ul = (((char *)ptb)[8] & 0xFF) * 256 + (((char *)ptb)[9] & 0xFF);
		info->f_uid = ul;
		ul = (((char *)ptb)[10] & 0xFF) * 256 + (((char *)ptb)[11] & 0xFF);
		info->f_gid = ul & 0xFFFF;
		l1 = ((char *)ptb)[12] * 256 + (((char *)ptb)[13] & 0xFF);
		if (is_dir(info) && !link_dirs)
			l1 = 1;
		info->f_nlink = l1;
		l1 = ((char *)ptb)[14] * 256 + (((char *)ptb)[15] & 0xFF);
		info->f_rdev = l1;
		info->f_rdevmaj	= _dev_major(8, info->f_rdev);
		info->f_rdevmin	= _dev_minor(8, info->f_rdev);
		info->f_rdev = makedev(info->f_rdevmaj, info->f_rdevmin);

		l1 = ((char *)ptb)[16] * 256 + (((char *)ptb)[17] & 0xFF);
		l2 = (((char *)ptb)[18] & 0xFF) * 256 + (((char *)ptb)[19] & 0xFF);
		l1 *= 0x10000;
		l1 += l2;
		info->f_atime = info->f_mtime = info->f_ctime = (time_t)l1;

		l1 = ((char *)ptb)[20] * 256 + (((char *)ptb)[21] & 0xFF);
		info->f_namelen = l1;

		l1 = ((char *)ptb)[22] * 256 + (((char *)ptb)[23] & 0xFF);
		l2 = (((char *)ptb)[24] & 0xFF) * 256 + (((char *)ptb)[25] & 0xFF);
		l1 *= 0x10000;
		l1 += l2;
		info->f_size = l1;
		break;

	case H_CPIO_CHR:
	case H_CPIO_ODC:

		astoo_cpio(&((char *)ptb)[6], &ul, 6);
		info->f_dev = ul;
		astoo_cpio(&((char *)ptb)[12], &ul, 6);
		info->f_ino = ul;
		astoo_cpio(&((char *)ptb)[18], &ul, 6);
		info->f_mode = ul;
		info->f_type = info->f_mode & S_IFMT;
		info->f_mode = info->f_mode & 07777;
		info->f_rxftype = info->f_xftype = IFTOXT(info->f_type);
		info->f_filetype = XTTOST(info->f_xftype);
		info->f_typeflag = XTTOUS(info->f_xftype);
		astoo_cpio(&((char *)ptb)[24], &ul, 6);
		info->f_uid = ul;
		astoo_cpio(&((char *)ptb)[30], &ul, 6);
		info->f_gid = ul;
		astoo_cpio(&((char *)ptb)[36], &ul, 6);
		if (is_dir(info) && !link_dirs)
			ul = 1;
		info->f_nlink = ul;
		astoo_cpio(&((char *)ptb)[42], &ul, 6);
		info->f_rdev = ul;
		/*
		 * This is undefined by POSIX.
		 * Let us hope that all implementations will assume 8 bits
		 * in the minor number.
		 */
		info->f_rdevmaj	= _dev_major(8, info->f_rdev);
		info->f_rdevmin	= _dev_minor(8, info->f_rdev);
		info->f_rdev = makedev(info->f_rdevmaj, info->f_rdevmin);

		astoo_cpio(&((char *)ptb)[48], &ul, 11);
		info->f_atime = info->f_mtime = info->f_ctime = (time_t)ul;

		astoo_cpio(&((char *)ptb)[59], &ul, 6);
		info->f_namelen = ul;

		astoo_cpio(&((char *)ptb)[65], &ul, 11);
		info->f_size = ul;
		break;

	case H_CPIO_ASC:
	case H_CPIO_ACRC:

		astoh_cpio(&((char *)ptb)[6], &ul, 8);
		info->f_ino = ul;
		astoh_cpio(&((char *)ptb)[14], &ul, 8);
		info->f_mode = ul;
		info->f_type = info->f_mode & S_IFMT;
		info->f_mode = info->f_mode & 07777;
		info->f_rxftype = info->f_xftype = IFTOXT(info->f_type);
		info->f_filetype = XTTOST(info->f_xftype);
		info->f_typeflag = XTTOUS(info->f_xftype);
		astoh_cpio(&((char *)ptb)[22], &ul, 8);
		info->f_uid = ul;
		astoh_cpio(&((char *)ptb)[30], &ul, 8);
		info->f_gid = ul;
		astoh_cpio(&((char *)ptb)[38], &ul, 8);
		if (is_dir(info) && !link_dirs)
			ul = 1;
		info->f_nlink = ul;

		astoh_cpio(&((char *)ptb)[46], &ul, 8);
		info->f_atime = info->f_mtime = info->f_ctime = (time_t)ul;

		astoh_cpio(&((char *)ptb)[54], &ul, 8);
		info->f_size = ul;

		astoh_cpio(&((char *)ptb)[62], &ul, 8);
		info->f_rdevmaj = ul;
		astoh_cpio(&((char *)ptb)[70], &ul, 8);
		info->f_rdevmin = ul;
		info->f_dev = makedev(info->f_rdevmaj, info->f_rdevmin);

		astoh_cpio(&((char *)ptb)[78], &ul, 8);
		info->f_rdevmaj = ul;
		astoh_cpio(&((char *)ptb)[86], &ul, 8);
		info->f_rdevmin = ul;
		info->f_rdev = makedev(info->f_rdevmaj, info->f_rdevmin);

		astoh_cpio(&((char *)ptb)[94], &ul, 8);
		info->f_namelen = ul;
		break;

	default:
		errmsgno(EX_BAD, "Found CPIO type %ld/%d ", hdrtype, H_TYPE(hdrtype));
		print_hdrtype(stderr, hdrtype);
		comerrno(EX_BAD, "Can't handle this cpio archive type (yet).\n");
	}

	move.m_data = info->f_name;
	move.m_flags = 0;
	if (info->f_namelen <= 1) {
		errmsgno(EX_BAD, "Name size <= 0.\n");
		die(EX_BAD);		/* Need to change to permit -i resync */
	}
#ifdef	ENFORCE_CPIO_NAMELEN
	if (info->f_namelen > props.pr_maxnamelen) {
		errmsgno(EX_BAD, "Name length (%lu) larger than maximum (%d).\n",
			info->f_namelen, props.pr_maxnamelen);
		die(EX_BAD);		/* Need to change to permit -i resync */
	}
#endif
	if (info->f_namelen > PATH_MAX) {
		errmsgno(EX_BAD, "Name length (%lu) larger than PATH_MAX.\n",
			info->f_namelen);
		die(EX_BAD);		/* Need to change to permit -i resync */
	}
	info->f_rsize = info->f_llsize = info->f_namelen;
	info->f_flags |= F_LONGNAME;
	if (xt_file(info, vp_move_from_arch, &move, 0, "moving file name") < 0)
		die(EX_BAD);
	info->f_flags &= ~F_LONGNAME;
	info->f_flags &= ~F_DATA_SKIPPED;
	info->f_rsize = info->f_llsize = info->f_size;
	info->f_namelen -= 1;		/* Null byte */

	info->f_lname[0] = '\0';
	if (is_symlink(info)) {
		move.m_data = info->f_lname;
		move.m_flags = 0;
		if (info->f_size <= 0) {
			errmsgno(EX_BAD, "Symlink size <= 0.\n");
			die(EX_BAD);	/* Need to change to permit -i resync */
		}
		if (info->f_size > PATH_MAX) {
			errmsgno(EX_BAD, "Linkname length (%lld) larger than PATH_MAX.\n",
				(Llong)info->f_size);
			die(EX_BAD);	/* Need to change to permit -i resync */
		}
		if (xt_file(info, vp_move_from_arch, &move, 0, "moving link name") < 0)
			die(EX_BAD);
		info->f_flags &= ~F_DATA_SKIPPED;
		info->f_lname[info->f_size] = '\0';
		info->f_rsize = info->f_llsize = 0;
	}

	if (!is_file(info))
		info->f_rsize = info->f_llsize = 0;

	if (info->f_namelen == 10 &&
	    info->f_name[0] == 'T' &&
	    strncmp(info->f_name, "TRAILER!!!", 10) == 0) {
		extern	m_stats	*stats;

		stats->eofblock = tblocks();

		if (ignoreerr)
			errmsgno(EX_BAD, "EOF Block at: %lld ignored.\n",
							tblocks());
		return (EOF);
	}
	if (info->f_nlink > 1) {
		TCB	tb;
		ul = info->f_rsize;
		info->f_flags |= F_EXTRACT;
		if (read_link(info->f_name, info->f_namelen, info, &tb)) {
			info->f_typeflag = LNKTYPE;
			info->f_rsize = ul;
			return (0);
		}
		info->f_flags &= ~F_EXTRACT;
		info->f_rsize = ul;
	}
	return (0);
}

EXPORT void
cpio_resync()
{
	comerrno(EX_BAD, "Cpio resync not yet implemented.\n");
}

/*
 * Check whether the archive is completely byte swapped.
 * Unfortunately, this only works if the strlen(f->f_name) is odd.
 */
EXPORT int
cpio_checkswab(ptb)
	register TCB	*ptb;
{
	long	l1;
	long	l2;


	l1 = (((char *)ptb)[20] & 0xFF) * 256 + (((char *)ptb)[21] & 0xFF);
	l2 = (((char *)ptb)[21] & 0xFF) * 256 + (((char *)ptb)[20] & 0xFF);
	if (l1 <= 257 || l2 <= 257) {
		if (l2 <= 257)
			l1 = l2;
	}
	/*
	 * The maximum filename length in binary cpio is 256 bytes.
	 * In theory, more would be possible, but a long filename length
	 * may be caused by a rotten archive. As we may safely assume that
	 * the whole size of a PTB (TBLOCK) is accessible, we run the check
	 * for the null byte at the end of the filename as long as the
	 * filename length is less or equal to TBLOCK - CPIOBIN_HDRSZ.
	 */
	if ((l1 + CPIOBIN_HDRSZ) > TBLOCK)
		return (H_CPIO_BIN);

	if (((char *)ptb)[CPIOBIN_HDRSZ+l1-2] == '\0' &&
	    ((char *)ptb)[CPIOBIN_HDRSZ+l1-1] != '\0')
		return (H_SWAPPED(H_CPIO_BIN));
	return (H_CPIO_BIN);
}

/*
 * This simple sum is used for the SYSvr4 file content checksum.
 * It is a simple 32 bit sum even though the related CPIO format is called CRC.
 * Use Int32_t to implement the same behavior as the AT&T cpio command.
 */
LOCAL Int32_t
cpio_cksum(name)
	char	*name;
{
		char		buf[8192];
		int		f = _lfileopen(name, "rb");
	register int		amt;
	register char		*p;
	register char		*ep;
	register Int32_t	sum = 0;

	if (f >= 0) {
		while ((amt = _fileread(&f, buf, sizeof (buf))) > 0) {
			p = buf;
			ep = &p[amt];
			while (p < ep) {
				sum += (long)*p++;
			}
		}
		if (amt < 0)
			sum = -1;
		close(f);
	} else {
		sum = -1;
	}
	return (sum);
}

/*
 * Convert octal string -> long int
 */
LOCAL void
astoo_cpio(s, l, cnt)
	register char	*s;
		Ulong	*l;
	register int	cnt;
{
	register Ulong	ret = 0L;
	register char	c;
	register int	t;

	for (; cnt > 0; cnt--) {
		c = *s++;
		if (isoctal(c))
			t = c - '0';
		else
			break;
		ret *= 8;
		ret += t;
	}
	*l = ret;
}

/*
 * Convert hex string -> long int
 */
LOCAL void
astoh_cpio(s, l, cnt)
	register char	*s;
		Ulong	*l;
	register int	cnt;
{
	register Ulong	ret = 0L;
	register char	c;
	register int	t;

	for (; cnt > 0; cnt--) {
		c = *s++;
		if (isdigit(c))
			t = c - '0';
		else if (c >= 'a' && c <= 'f')
			t = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			t = c - 'A' + 10;
		else
			break;
		ret *= 16;
		ret += t;
	}
	*l = ret;
}

/*
 * Convert long int -> hex string.
 */
LOCAL void
litoh_cpio(s, l, fieldw)
		char	*s;
	register Ulong	l;
	register int	fieldw;
{
	register char	*p	= &s[fieldw];
	register char	fill	= props.pr_fillc;

	do {
		*--p = "0123456789abcdef"[l%16];	/* Compiler optimiert */

	} while (--fieldw > 0 && (l /= 16) > 0);

	switch (fieldw) {

	default:
		break;
	case 11:	*--p = fill;	/* FALLTHROUGH */
	case 10:	*--p = fill;	/* FALLTHROUGH */
	case 9:		*--p = fill;	/* FALLTHROUGH */
	case 8:		*--p = fill;	/* FALLTHROUGH */
	case 7:		*--p = fill;	/* FALLTHROUGH */
	case 6:		*--p = fill;	/* FALLTHROUGH */
	case 5:		*--p = fill;	/* FALLTHROUGH */
	case 4:		*--p = fill;	/* FALLTHROUGH */
	case 3:		*--p = fill;	/* FALLTHROUGH */
	case 2:		*--p = fill;	/* FALLTHROUGH */
	case 1:		*--p = fill;	/* FALLTHROUGH */
	case 0:		break;
	}
}
