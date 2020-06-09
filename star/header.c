/* @(#)header.c	1.205 20/06/05 Copyright 1985, 1994-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)header.c	1.205 20/06/05 Copyright 1985, 1994-2020 J. Schilling";
#endif
/*
 *	Handling routines to read/write, parse/create
 *	archive headers
 *
 *	Copyright (c) 1985, 1994-2020 J. Schilling
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
#include <schily/idcache.h>
#include "starsubs.h"
#include "checkerr.h"
#include "fifo.h"

	/* ustar */
LOCAL	char	magic[TMAGLEN] = TMAGIC;
	/* star */
LOCAL	char	stmagic[STMAGLEN] = STMAGIC;
	/* gnu tar */
LOCAL	char	gmagic[GMAGLEN] = GMAGIC;

typedef struct {
	char	*h_name;
	char	*h_text;
	Int8_t	h_type;
	UInt8_t	h_flags;
} htab_t;

#define	HF_RO	0x01	/* Read only entry - may not be set via cmd line */

LOCAL	htab_t	htab[] = {
/* BEGIN CSTYLED */
/* 0 */	{ "UNDEFINED",	"Undefined archive",			H_UNDEF, HF_RO	},
/* 1 */	{ "unknown tar", "Unknown tar format", 			H_TAR,   HF_RO	},
/* 2 */	{ "v7tar",	"Old UNIX V7 tar format",		H_V7TAR,  0	},
/* 3 */	{ "tar",	"Old BSD tar format",			H_OTAR,  0	},
/* 4 */	{ "star",	"Old star format from 1985",		H_STAR,  0	},
/* 5 */	{ "gnutar",	"GNU tar format 1989 (violates POSIX)",	H_GNUTAR, 0	},
/* 6 */	{ "ustar",	"Standard POSIX.1-1988 tar format",	H_USTAR, 0	},
/* 7 */	{ "xstar",	"Extended standard tar (star 1994)",	H_XSTAR, 0	},
/* 8 */	{ "xustar",	"'xstar' format without tar signature",	H_XUSTAR, 0	},
/* 9 */	{ "exustar",	"'xustar' format - always x-header",	H_EXUSTAR, 0	},
/*10 */	{ "pax",	"Extended POSIX.1-2001 standard tar",	H_PAX,    0	},
/*11 */	{ "epax",	"Extended POSIX.1-2001 standard tar + x-header",	H_EPAX,    0	},
/*12 */	{ "suntar",	"Sun's extended pre-POSIX.1-2001",	H_SUNTAR, 0	},

/*15 */	{ "bar",	"SunOS 4.x bar format",			H_BAR,    HF_RO	},

/*16 */	{ "bin",	"cpio UNIX V7 binary format",		H_CPIO_BIN, 0	},
/*17 */	{ "cpio",	"cpio POSIX.1-1988 format",		H_CPIO_CHR, 0	},
/*18 */	{ "odc",	"cpio POSIX.1-1988 with SYSv compat",	H_CPIO_ODC, 0	},
/*19 */	{ "nbin",	"CPIO NBIN",				H_CPIO_NBIN, HF_RO	},
/*20 */	{ "crcbin",	"CPIO CRCBIN",				H_CPIO_CRC, HF_RO	},
/*21 */	{ "asc",	"SYSvr4 cpio ascii expanded device #",	H_CPIO_ASC, 0	},
/*22 */	{ "crc",	"'asc' format + CRC",			H_CPIO_ACRC, 0	},

	{ NULL,		NULL, 					0, 0 },
};
/* END CSTYLED */

/*
 * Compression names
 */
LOCAL	char	*cnames[] = {
	"unknown",		/*  0 C_NONE	*/
	"pack",			/*  1 C_PACK	*/
	"gzip",			/*  2 C_GZIP	*/
	"lzw",			/*  3 C_LZW	*/
	"freeze",		/*  4 C_FREEZE	*/
	"lzh",			/*  5 C_LZH	*/
	"pkzip",		/*  6 C_PKZIP	*/
	"bzip2",		/*  7 C_BZIP2	*/
	"lzo",			/*  8 C_LZO	*/
	"7z",			/*  9 C_7Z	*/
	"xz",			/* 10 C_XZ	*/
	"lzip",			/* 11 C_LZIP	*/
	"zstd",			/* 12 C_ZSTD	*/
	"lzma",			/* 13 C_LZMA	*/
	"freeze2",		/* 14 C_FREEZE2	*/
};

extern	FILE	*tty;
extern	FILE	*vpr;
extern	const	char	*tarfiles[];	/* Cycle list of all archives	*/
extern	int	tarfindex;		/* Current index in list	*/
extern	BOOL	multivol;
extern	long	hdrtype;
extern	long	chdrtype;
extern	int	cmptype;
extern	int	version;
extern	int	swapflg;
extern	BOOL	print_artype;
extern	BOOL	debug;
extern	BOOL	numeric;
extern	int	verbose;
extern	BOOL	rflag;
extern	BOOL	uflag;
extern	BOOL	xflag;
extern	BOOL	nflag;
extern	BOOL	ignoreerr;
extern	BOOL	signedcksum;
extern	BOOL	nowarn;
extern	BOOL	nullout;
extern	BOOL	modebits;		/* -modebits more than 12 bits  */
extern	BOOL	linkdata;

extern	Ullong	tsize;

extern	char	*bigbuf;
extern	int	bigsize;

LOCAL	Ulong	checksum	__PR((TCB *ptb));
LOCAL	Ulong	bar_checksum	__PR((TCB *ptb));
LOCAL	BOOL	signedtarsum	__PR((TCB *ptb, Ulong ocheck));
EXPORT	BOOL	tarsum_ok	__PR((TCB *ptb));
LOCAL	BOOL	isstmagic	__PR((char *s));
LOCAL	BOOL	isxmagic	__PR((TCB *ptb));
LOCAL	BOOL	ismagic		__PR((char *s));
LOCAL	BOOL	isgnumagic	__PR((char *s));
LOCAL	BOOL	strxneql	__PR((char *s1, char *s2, int l));
LOCAL	BOOL	istarnumber	__PR((char *s, int fieldw));
LOCAL	BOOL	ustmagcheck	__PR((TCB *ptb));
EXPORT	void	print_hdrtype	__PR((FILE *f, int type));
EXPORT	char	*hdr_name	__PR((int type));
EXPORT	char	*hdr_text	__PR((int type));
EXPORT	int	hdr_type	__PR((char *name));
EXPORT	void	hdr_usage	__PR((void));
EXPORT	int	get_hdrtype	__PR((TCB *ptb, BOOL isrecurse));
LOCAL	int	get_xhtype	__PR((TCB *ptb, int htype));
EXPORT	int	get_compression	__PR((TCB *ptb));
EXPORT	char	*get_cmpname	__PR((int type));
EXPORT	int	get_tcb		__PR((TCB *ptb));
EXPORT	void	put_tcb		__PR((TCB *ptb, FINFO *info));
EXPORT	void	write_tcb	__PR((TCB *ptb, FINFO *info));
EXPORT	void	info_to_tcb	__PR((FINFO *info, TCB *ptb));
LOCAL	void	info_to_star	__PR((FINFO *info, TCB *ptb));
LOCAL	void	info_to_ustar	__PR((FINFO *info, TCB *ptb));
LOCAL	void	info_to_xstar	__PR((FINFO *info, TCB *ptb));
LOCAL	void	info_to_gnutar	__PR((FINFO *info, TCB *ptb));
EXPORT	int	tcb_to_info	__PR((TCB *ptb, FINFO *info));
LOCAL	void	tar_to_info	__PR((TCB *ptb, FINFO *info));
LOCAL	void	star_to_info	__PR((TCB *ptb, FINFO *info));
LOCAL	void	ustar_to_info	__PR((TCB *ptb, FINFO *info));
LOCAL	void	xstar_to_info	__PR((TCB *ptb, FINFO *info));
LOCAL	void	gnutar_to_info	__PR((TCB *ptb, FINFO *info));
LOCAL	int	ustoxt		__PR((int ustype));
LOCAL	BOOL	checkeof	__PR((TCB *ptb));
LOCAL	BOOL	eofblock	__PR((TCB *ptb));
LOCAL	void	stoli		__PR((char *s, Ulong * l, int fieldw));
EXPORT	void	stolli		__PR((char *s, Ullong *ull));
LOCAL	void	litos		__PR((char *s, Ulong l, int fieldw));
EXPORT	void	llitos		__PR((char *s, Ullong ull, int fieldw));
LOCAL	void	stob		__PR((char *s, Ulong *l, int fieldw));
LOCAL	void	stollb		__PR((char *s, Ullong *ull, int fieldw));
LOCAL	void	btos		__PR((char *s, Ulong l, int fieldw));
LOCAL	void	llbtos		__PR((char *s, Ullong ull, int fieldw));
LOCAL	BOOL	nameascii	__PR((char *name));
LOCAL	void	print_hrange	__PR((char *type, Ullong ull));
EXPORT	void	dump_info	__PR((FINFO *info));

/*
 * XXX Hier sollte eine tar/bar universelle Checksummenfunktion sein!
 */
#define	CHECKS	sizeof (ptb->ustar_dbuf.t_chksum)
/*
 * We know, that sizeof (TCP) is 512 and therefore has no
 * reminder when dividing by 8
 *
 * CHECKS is known to be 8 too, use loop unrolling.
 */
#define	DO8(a)	a; a; a; a; a; a; a; a;

LOCAL Ulong
checksum(ptb)
	register	TCB	*ptb;
{
	register	int	i;
	register	Ulong	sum = 0;
	register	Uchar	*us;

	if (signedcksum) {
		register	char	*ss;
		register	long	ssum = 0;

		ss = (char *)ptb;
		for (i = sizeof (*ptb)/8; --i >= 0; ) {
			DO8(ssum += *ss++);
		}
		if (ssum == 0L) {	/* Block containing 512 nul's? */
			signedcksum = FALSE;
			sum = checksum(ptb);
			signedcksum = TRUE;
			return (sum);
		}

		ss = (char *)ptb->ustar_dbuf.t_chksum;
		DO8(ssum -= *ss++);
		ssum += CHECKS*' ';
		sum = ssum;
	} else {
		us = (Uchar *)ptb;
		for (i = sizeof (*ptb)/8; --i >= 0; ) {
			DO8(sum += *us++);
		}
		if (sum == 0L)		/* Block containing 512 nul's */
			return (sum);

		us = (Uchar *)ptb->ustar_dbuf.t_chksum;
		DO8(sum -= *us++);
		sum += CHECKS*' ';
	}
	return (sum);
}
#undef	CHECKS

#define	CHECKS	sizeof (ptb->bar_dbuf.t_chksum)

LOCAL Ulong
bar_checksum(ptb)
	register	TCB	*ptb;
{
	register	int	i;
	register	Ulong	sum = 0;
	register	Uchar	*us;

	if (signedcksum) {
		register	char	*ss;
		register	long	ssum = 0;

		ss = (char *)ptb;
		for (i = sizeof (*ptb); --i >= 0; )
			ssum += *ss++;
		if (ssum == 0L) {	/* Block containing 512 nul's? */
			signedcksum = FALSE;
			sum = bar_checksum(ptb);
			signedcksum = TRUE;
			return (sum);
		}

		for (i = CHECKS, ss = (char *)ptb->bar_dbuf.t_chksum; --i >= 0; )
			ssum -= *ss++;
		ssum += CHECKS*' ';
		sum = ssum;
	} else {
		us = (Uchar *)ptb;
		for (i = sizeof (*ptb); --i >= 0; )
			sum += *us++;
		if (sum == 0L)		/* Block containing 512 nul's */
			return (sum);

		for (i = CHECKS, us = (Uchar *)ptb->bar_dbuf.t_chksum; --i >= 0; )
			sum -= *us++;
		sum += CHECKS*' ';
	}
	return (sum);
}
#undef	CHECKS

LOCAL BOOL
signedtarsum(ptb, ocheck)
	TCB	*ptb;
	Ulong	ocheck;
{
	BOOL	osigned = signedcksum;
	Ulong	check;

	if (ocheck == 0)
		return (FALSE);

	signedcksum = !signedcksum;
	check = checksum(ptb);
	if (ocheck == check) {
		errmsgno(EX_BAD, "WARNING: archive uses %s checksums.\n",
				signedcksum?"signed":"unsigned");
		return (TRUE);
	}
	signedcksum = osigned;
	return (FALSE);
}

/*
 * XXX Shouldn't we use this or something similar from get_tcb() too?
 */
EXPORT BOOL
tarsum_ok(ptb)
	TCB	*ptb;
{
	Ulong	ocheck;
	Ulong	check;

	/*
	 * We are currently only called with TAR type archives, so using 7 for
	 * fieldwidth is OK.
	 */
	stoli(ptb->dbuf.t_chksum, &ocheck, 7);
	if (ocheck == 0)
		return (FALSE);
	check = checksum(ptb);
	if (ocheck != check && !signedtarsum(ptb, ocheck))
		return (FALSE);
	return (TRUE);
}

LOCAL BOOL
isstmagic(s)
	char	*s;
{
	return (strxneql(s, stmagic, STMAGLEN));
}

/*
 * Check for XSTAR / XUSTAR format.
 *
 * Since we use this function after we checked for the "tar" signature, it
 * is only used as a XUSTAR check.
 *
 * This is star's upcoming new standard format. This format understands star's
 * old extended POSIX format and in future will write POSIX.1-2001 extensions
 * using 'x' headers.
 * Star also detects the archive format from the value of
 * the "SCHILY.archtype=" POSIX.1-2001 header tag.
 */
LOCAL BOOL
isxmagic(ptb)
	TCB	*ptb;
{
	register int	i;

	/*
	 * prefix[130] is is granted to be '\0' with 'xstar'.
	 */
	if (ptb->xstar_dbuf.t_prefix[130] == '\0') {
		/*
		 * True for all 'standard' TCBs except with typeflag 'M'
		 */
		/* EMPTY */
		;
	} else if (ptb->ustar_dbuf.t_typeflag == 'M') {
		/*
		 * We come only here if we try to read in a multivol archive
		 * starting past volume #0. If we start with volume #0, all
		 * multi volume headers are skiped by the low level multi
		 * volume handling code.
		 */
		if ((ptb->xstar_in_dbuf.t_offset[11] != ' ') &&
		    ((ptb->xstar_in_dbuf.t_offset[0] & 0x80) == 0))
			return (FALSE);
	} else {
		return (FALSE);
	}

	/*
	 * If atime[0]...atime[10] or ctime[0]...ctime[10]
	 * is not a POSIX octal number it cannot be 'xstar'.
	 * With the octal representation we may store any date
	 * for 1970 +- 136 years (1834 ... 2106).
	 *
	 * After 2106 we will most likely always use POSIX.1-2001 'x'
	 * headers but still use base 256 numbers in the old tar header.
	 * We thus still need to check for base 256 numbers even though
	 * this is very unlikely since we create 'x' headers if nanoseconds
	 * are != 0 and then use a 0 time stamp for atime/ctime in the
	 * 'x' header.
	 */
	for (i = 0; i < 11; i++) {
		if ((ptb->xstar_dbuf.t_atime[0] & 0x80) == 0 &&
		    (ptb->xstar_dbuf.t_atime[i] < '0' ||
		    ptb->xstar_dbuf.t_atime[i] > '7'))
			return (FALSE);
		if (((ptb->xstar_dbuf.t_ctime[0] & 0x80) == 0) &&
		    (ptb->xstar_dbuf.t_ctime[i] < '0' ||
		    ptb->xstar_dbuf.t_ctime[i] > '7'))
			return (FALSE);
	}

	/*
	 * Check for both POSIX compliant end of number characters
	 * if not using base 256.
	 */
	if ((ptb->xstar_dbuf.t_atime[0] & 0x80) == 0 &&
	    ptb->xstar_dbuf.t_atime[11] != ' ' &&
	    ptb->xstar_dbuf.t_atime[11]  != '\0')
		return (FALSE);

	if ((ptb->xstar_dbuf.t_ctime[0] & 0x80) == 0 &&
	    ptb->xstar_dbuf.t_ctime[11] != ' ' &&
	    ptb->xstar_dbuf.t_ctime[11]  != '\0')
		return (FALSE);

	return (TRUE);
}

LOCAL BOOL
ismagic(s)
	char	*s;
{
	return (strxneql(s, magic, TMAGLEN));
}

LOCAL BOOL
isgnumagic(s)
	char	*s;
{
	return (strxneql(s, gmagic, GMAGLEN));
}

LOCAL BOOL
strxneql(s1, s2, l)
	register char	*s1;
	register char	*s2;
	register int	l;
{
	while (--l >= 0)
		if (*s1++ != *s2++)
			return (FALSE);
	return (TRUE);
}

/*
 * Check whether a field looks like a TAR numeric field.
 * This intentionally does not include a check for base-256 numbers.
 */
LOCAL BOOL
istarnumber(s, fieldw)
	char	*s;
	int	fieldw;
{
	register int	c;

	while (*s == ' ' && --fieldw >= 0)
		s++;

	/*
	 * We need at least one octal number, skip other octals numbers.
	 */
	c = *s;
	if (fieldw < 0 || !isoctal(c))
		return (FALSE);
	while ((c = *s) && --fieldw >= 0) {
		if (!isoctal(c))
			break;
		s++;
	}

	/*
	 * All digits used within fieldw. This is the non-compliant case
	 * where the number is followed by the next one (starting with space
	 * or an octal digit. We are gracious and permit this case.
	 */
	c = *s;
	if ((c == ' ' || isoctal(c)) && fieldw == -1)
		return (TRUE);

	/*
	 * Typical standard compliant end.
	 */
	if ((c == '\0' || c == ' ') && (fieldw <= 2 && fieldw >= 0))
		return (TRUE);

	/*
	 * We either got a field separator too early or after
	 * fieldw was exhausted. This still may be tolerable.
	 */
	if (fieldw < 0 || !isoctal(c))
		return (FALSE);

	return (TRUE);
}

LOCAL BOOL
ustmagcheck(ptb)
	TCB	*ptb;
{
	if (ismagic(ptb->xstar_dbuf.t_magic) &&
				strxneql(ptb->xstar_dbuf.t_version, "00", 2))
		return (TRUE);
	return (FALSE);
}

EXPORT void
print_hdrtype(f, type)
	FILE	*f;
	int	type;
{
		BOOL	isswapped = H_ISSWAPPED(type);

	if (H_TYPE(type) > H_MAX_ARCH)
		type = H_UNDEF;
	type = H_TYPE(type);

	fgtprintf(f, "%s%s archive.\n", isswapped?"swapped ":"", hdr_name(type));
}

EXPORT char *
hdr_name(type)
	int	type;
{
	register htab_t	*htp = htab;

	for (; htp->h_name; htp++) {
		if (htp->h_type == type)
			return (htp->h_name);
	}
	return (htab->h_name);
}

EXPORT char *
hdr_text(type)
	int	type;
{
	register htab_t	*htp = htab;

	for (; htp->h_name; htp++) {
		if (htp->h_type == type)
			return (htp->h_text);
	}
	return (htab->h_text);
}

EXPORT int
hdr_type(name)
	char	*name;
{
	register htab_t	*htp = htab;

	for (; htp->h_name; htp++) {
		if (htp->h_flags & HF_RO)
			continue;
		if (streql(name, htp->h_name))
			return (htp->h_type);
	}
	return (-1);
}

EXPORT void
hdr_usage()
{
	register htab_t	*htp = htab;

	for (; htp->h_name; htp++) {
		if (htp->h_flags & HF_RO)
			continue;
		error("%s\t%s\t%s\n",
			hdrtype == htp->h_type ? "*":"",
			htp->h_name, htp->h_text);
	}
}

EXPORT int
get_hdrtype(ptb, isrecurse)
	TCB	*ptb;
	BOOL	isrecurse;
{
	Ulong	check;
	Ulong	ocheck;
	int	ret = H_UNDEF;

	/*
	 * We don't like to get "WARNING: Unterminated octal number at..."
	 * when we may have e.g. a CPIO archive to check.
	 * So enforce to null-terminate the TAR checksum field and use a
	 * fieldwidth of 8 as CPIO archives may have all characters from
	 * ptb->dbuf.t_chksum[] != 0.
	 */
	check = ptb->dbuf.t_linkflag;
	ptb->dbuf.t_linkflag = 0;
	stoli(ptb->dbuf.t_chksum, &ocheck, 8);
	ptb->dbuf.t_linkflag = check;		/* Restore old value */
	if (ocheck == 0)
		goto nottar;
	check = checksum(ptb);
	if (ocheck != check && !signedtarsum(ptb, ocheck)) {
		if (debug && !isrecurse) {
			errmsgno(EX_BAD,
				"Bad tar checksum at: %lld: 0%lo should be 0%lo.\n",
							tblocks(),
							ocheck, check);
		}
		goto nottar;
	}

	/*
	 * t_mode never needs base-256, so this is a good place to check.
	 *
	 * Unfortunately, GNU tar does not fill in t_mode for 'V'holhdr,
	 * but only t_mtime.
	 * Unfortunately, GNU tar does not fill in t_mode for 'M'ultivol,
	 * but only t_size.
	 */
	if (!istarnumber(ptb->dbuf.t_mode, 8)) {
		/*
		 * A non standard compliant header was seen. Try to work around
		 * the deviations from GNU tar and permit GNU tar headers to be
		 * seen as OK.
		 */
		switch (ptb->ustar_dbuf.t_typeflag) {

		case 'V':
			if (!(ptb->dbuf.t_mtime[0] & 0x80) &&
			    !istarnumber(ptb->dbuf.t_mtime, 12))
				goto nottar;
			break;
		case 'M':
			if (!(ptb->dbuf.t_size[0] & 0x80) &&
			    !istarnumber(ptb->dbuf.t_size, 12))
				goto nottar;
			break;

		default:		/* Not a problematic GNU tar header */
			goto nottar;
		}
	}

	if (isstmagic(ptb->dbuf.t_magic)) {	/* Check for 'tar\0' at end */
		if (ustmagcheck(ptb))
			ret = H_XSTAR;
		else
			ret = H_STAR;
		if (debug) print_hdrtype(stderr, ret);
		return (ret);
	}
	if (ustmagcheck(ptb)) {			/* 'ustar\000' POSIX magic */
		if (isxmagic(ptb)) {		/* Check for xustar	   */
#ifdef	__historic__
			/*
			 * H_EXUSTAR was introduced in August 2001 but since
			 * October 2003 we have SCHILY.archtype that is always
			 * used together with H_EXUSTAR. Determine the achive
			 * type from SCHILY.archtype in the 'g' header.
			 */
			if (ptb->ustar_dbuf.t_typeflag == 'g' ||
			    ptb->ustar_dbuf.t_typeflag == 'x')
				ret = H_EXUSTAR;
			else
#endif
				ret = H_XUSTAR;
		} else {
			if (ptb->ustar_dbuf.t_typeflag == 'g' ||
			    ptb->ustar_dbuf.t_typeflag == 'x')
				ret = H_PAX;
			else if (ptb->ustar_dbuf.t_typeflag == 'X')
				ret = H_SUNTAR;
			else
				ret = H_USTAR;
		}
		if (debug) print_hdrtype(stderr, ret);
		return (ret);
	}
	if (isgnumagic(ptb->ustar_dbuf.t_magic)) { /* 'ustar  ' GNU magic */
		ret = H_GNUTAR;
		if (debug) print_hdrtype(stderr, ret);
		return (ret);
	}
	if ((ptb->dbuf.t_mode[6] == ' ' && ptb->dbuf.t_mode[7] == '\0')) {
		ret = H_OTAR;
		if (debug) print_hdrtype(stderr, ret);
		return (ret);
	}
	if (ptb->ustar_dbuf.t_typeflag == LF_VOLHDR ||
			    ptb->ustar_dbuf.t_typeflag == LF_MULTIVOL) {
		/*
		 * Gnu volume headers & multi volume headers
		 * are no real tar headers.
		 */
		if (debug) error("gnutar buggy archive.\n");
		ret = H_GNUTAR;
		if (debug) print_hdrtype(stderr, ret);
		return (ret);
	}
	/*
	 * The only thing we know here is:
	 * we found a header with a correct tar checksum.
	 */
	ret = H_TAR;
	if (debug) print_hdrtype(stderr, ret);
	return (ret);

nottar:
	if (ptb->bar_dbuf.bar_magic[0] == 'V') {
		/*
		 * We don't like "WARNING: Unterminated octal number at..."
		 * when we may have e.g. a CPIO archive to check.
		 * So enforce to null-terminate the BAR checksum field and use a
		 * fieldwidth of 8 as junk may have all characters from
		 * ptb->bar_dbuf.t_chksum[] != 0.
		 */
		check = ptb->bar_dbuf.rdev[0];
		stoli(ptb->bar_dbuf.t_chksum, &ocheck, 8);
		ptb->bar_dbuf.rdev[0] = check;
		check = bar_checksum(ptb);

		if (ocheck == 0) {
			/* EMPTY */
			;
		} else if (ocheck == check) {
			ret = H_BAR;
			if (debug) print_hdrtype(stderr, ret);
			return (ret);
		} else if (debug && !isrecurse) {
			errmsgno(EX_BAD,
				"Bad bar checksum at: %lld: 0%lo should be 0%lo.\n",
							tblocks(),
							ocheck, check);
		}

	}
	if (strxneql((char *)ptb, "070701", 6)) {
		/*
		 * CPIO ASCII hex header with expanded device numbers
		 */
		ret = H_CPIO_ASC;		/* cpio -c */
		if (debug) print_hdrtype(stderr, ret);
		return (ret);
	}
	if (strxneql((char *)ptb, "070702", 6)) {
		/*
		 * CPIO ASCII hex header with expanded device numbers and CRC
		 */
		ret = H_CPIO_ACRC;		/* cpio -Hcrc */
		if (debug) print_hdrtype(stderr, ret);
		return (ret);
	}
	if (strxneql((char *)ptb, "070707", 6)) {
		/*
		 * POSIX small (6 octal digit device numbers)
		 */
		ret = H_CPIO_CHR;		/* cpio -Hodc */
		if (debug) print_hdrtype(stderr, ret);
		return (ret);

	}
	if (strxneql((char *)ptb, "\161\301", 2)) {
		/*
		 * 0161 0301 -> 0x71 0xC1 -> 070701
		 */
		ret = H_CPIO_NBIN;
		if (debug) print_hdrtype(stderr, ret);
		return (ret);
	}
	if (strxneql((char *)ptb, "\161\302", 2)) {
		/*
		 * 0161 0302 -> 0x71 0xC2 -> 070702
		 */
		ret = H_CPIO_CRC;
		if (debug) print_hdrtype(stderr, ret);
		return (ret);
	}
	if (strxneql((char *)ptb, "\161\307", 2) ||
	    strxneql((char *)ptb, "\307\161", 2)) {
		/*
		 * cpio default
		 * 0161 0307 -> 0x71 0xC7 -> 070707
		 * 0307 0161 -> 0xC7 0x71 -> 0143561
		 *
		 * Binary cpio archives may use any byte order for the numbers
		 * in the header so we cannot use the byte order in the header
		 * to detect swapped archives.
		 * Filenames with odd length result in a null byte inside the
		 * filename and this allows us to auto-detect byte swapped
		 * archives.
		 *
		 * If strlen(info->f_name) is odd, we may autodetect
		 * whether this archive has been swapped as whole.
		 * cpio_checkswab() returns either H_CPIO_BIN or
		 * H_SWAPPED(H_CPIO_BIN).
		 */
		ret = cpio_checkswab(ptb);
		if (debug) print_hdrtype(stderr, ret);
		return (ret);
	}
	if (debug) error("no tar archive??\n");

	if (!isrecurse) {
		int	rret;
		swabbytes((char *)ptb, TBLOCK);
		rret = get_hdrtype(ptb, TRUE);
		swabbytes((char *)ptb, TBLOCK);
		rret = H_SWAPPED(rret);
		if (debug) print_hdrtype(stderr, rret);
		return (rret);
	}

	if (debug) print_hdrtype(stderr, ret);
	return (ret);
}

LOCAL int
get_xhtype(ptb, htype)
	TCB	*ptb;
	int	htype;
{
	FINFO	finfo;
	Ullong	ull;
	int	xhsiz = bigsize-TBLOCK;
	char	*xhp = &bigbuf[TBLOCK];
	BOOL	swapped;
	int	t;
	GINFO	gsav;
extern	GINFO	*grip;				/* Global read info pointer */

	gsav = *grip;				/* Save old content */

	t = H_TYPE(htype);
	if (t < H_TARMIN || t > H_TARMAX)
		return (htype);

	swapped = H_ISSWAPPED(htype);
	/*
	 * Swap TCB & io buffer.
	 */
	if (swapped) {
		swabbytes(ptb, TBLOCK);
		swabbytes(bigbuf, bigsize);
	}

	if (ptb->ustar_dbuf.t_typeflag != 'g' &&
	    ptb->ustar_dbuf.t_typeflag != 'x')
		goto out;

	/*
	 * File size is strlen of extended header
	 */
	stolli(ptb->dbuf.t_size, &ull);
	finfo.f_size = ull;
	finfo.f_rsize = (off_t)finfo.f_size;

	if (xhsiz > ull)
		xhsiz = ull;

	/*
	 * Mark the path name and link name pointer uninitalized to avoid that
	 * xhparse() will try to copy a possible path= or lpath= entry in the
	 * first extended header to finfo->f_name & finfo->f_lname.
	 */
	finfo.f_name = NULL;
	finfo.f_lname = NULL;
	finfo.f_devminorbits = 0;
	finfo.f_xflags = 0;

	grip->archtype = H_UNDEF;
	xhparse(&finfo, xhp, xhp+xhsiz);
	if (grip->archtype != H_UNDEF) {
		htype = grip->archtype;
		if (swapped)
			htype = H_SWAPPED(htype);
	}
	*grip = gsav;				/* Restore old content */

out:
	/*
	 * Swap back TCB & io buffer.
	 */
	if (swapped) {
		swabbytes(ptb, TBLOCK);
		swabbytes(bigbuf, bigsize);
	}
	return (htype);
}

EXPORT int
get_compression(ptb)
	TCB	*ptb;
{
	char	*p = (char *)ptb;

	if (p[0] == '\037') {
		if (p[1] == '\036')	/* Packed		*/
			return (C_PACK);
		if (p[1] == '\213')	/* Gzip compressed	*/
			return (C_GZIP);
		if (p[1] == '\235')	/* LZW compressed	*/
			return (C_LZW);
		if (p[1] == '\236')	/* Freezed		*/
			return (C_FREEZE);
		if (p[1] == '\237')	/* Freeze-2		*/
			return (C_FREEZE2);
		if (p[1] == '\240')	/* SCO LZH compressed	*/
			return (C_LZH);
	}
	if (p[0] == 'P' && p[1] == 'K' && p[2] == '\003' && p[3] == '\004')
		return (C_PKZIP);
	if (p[0] == 'B' && p[1] == 'Z' && p[2] == 'h')
		return (C_BZIP2);
	if (p[0] == '\211' && p[1] == 'L' && p[2] == 'Z' && p[3] == 'O')
		return (C_LZO);

	/*
	 * p[6] && p[7] contain the version number
	 */
	if (p[0] == '7' && p[1] == 'z' && p[2] == '\274' && p[3] == '\257' &&
	    p[4] == '\047' && p[5] == '\034')
		return (C_7Z);

	/*
	 * p[6] && p[7] contain the stream flags
	 */
	if (p[0] == '\375' &&
	    p[1] == '7' && p[2] == 'z' && p[3] == 'X' && p[4] == 'Z' &&
	    p[5] == '\0')
		return (C_XZ);

	/*
	 * The lzip file format has four "magic bytes", followed by a version
	 * byte (0 or 1 currently), then the coded dictionary size. To reduce
	 * the number of false-positive detections, require the version byte
	 * be 0 or 1, and validate the dictionary size.
	 */
	if (p[0] == 'L' && p[1] == 'Z' && p[2] == 'I' && p[3] == 'P' &&
	    (p[4] == '\0' || p[4] == '\001') &&
	    ((p[5] & 0x1f) > 12 || (p[5] & 0x1f) == 0 || p[5] == 12))
		return (C_LZIP);

	if (p[0] == (char) 0x28 && p[1] == (char) 0xB5 &&
	    p[2] == (char) 0x2F && p[3] == (char) 0xFD)
		return (C_ZSTD);

	/*
	 * There is no grant that this is true, but it seems to be OK from the
	 * magic used by file(1) from Christos Zoulas <christos@zoulas.com>
	 */
	if (p[0] == ']' && p[1] == 0 && p[2] == 0)
		return (C_LZMA);

	return (C_NONE);
}

EXPORT char *
get_cmpname(type)
	int	type;
{
	if (type < 0 || type > C_MAX)
		type = C_NONE;

	/*
	 * Paranoia for incomplete cnames[] entries.
	 */
	if (type >= (sizeof (cnames) / sizeof (cnames[0])))
		type = C_NONE;

	return (cnames[type]);
}

EXPORT int
get_tcb(ptb)
	TCB	*ptb;
{
	Ulong	check;
	Ulong	ocheck;
	BOOL	eof = FALSE;
extern	long	iskip;
extern	Llong	mtskip;
extern	char	*bigptr;
extern	m_stats	*stats;

	/*
	 * bei der Option -i wird ein genulltes File
	 * fehlerhaft als EOF Block erkannt !
	 * wenn nicht t_magic gesetzt ist.
	 */
	do {
		/*
		 * First tar control block
		 */
		if (swapflg < 0) {
			BOOL	swapped;

			if (peekblock((char *)ptb, TBLOCK) == EOF) {
				/*
				 * The minimal size of a senseful TAR archive is
				 * 3 blocks (1536).
				 * The minimal size of a senseful CPIO archive is
				 * 26+2 + 26+12 = 66 bytes for a BIN archive, but
				 * a CPIO archive in whole needs to a multiple of
				 * 512 bytes.
				 */
				errmsgno(EX_BAD,
				"Hard EOF on input, first EOF block is missing at %lld.\n",
				tblocks());
#ifdef	FIFO_EOF_DEBUG
				if (use_fifo)	/* Debug a rare EOF problem */
					fifo_prmp(1);
#endif
				xstats.s_hardeof++;
				return (EOF);
			}
			if (mtskip) {
				int	nbl = stats->blocksize / TBLOCK;

				iskip = mtskip % nbl;
				iskip *= TBLOCK;
			}
			if (iskip) {
				if (stats->blocksize >= (iskip+TBLOCK)) {
					movetcb((TCB *)(bigptr+iskip), (TCB *)ptb);
				}
			}
			hdrtype = get_hdrtype(ptb, FALSE);
			hdrtype = get_xhtype(ptb, hdrtype);
			if (print_artype) {
				printf("%s: ", tarfiles[tarfindex]);
				if (cmptype != C_NONE) {
					gtprintf("%s compressed ",
						get_cmpname(cmptype));
				}
				print_hdrtype(stdout, hdrtype);
				exit(0);
			}
			swapped = H_ISSWAPPED(hdrtype);
			if (chdrtype != H_UNDEF &&
					swapped != H_ISSWAPPED(chdrtype)) {

				swapped = H_ISSWAPPED(chdrtype);
			}
			if (swapped) {
				swapflg = 1;
				/*
				 * We swap everything already read here.
				 * We tell the input routines later (inside the
				 * buf_resume() call) that further swapping is
				 * needed.
				 */
				swabbytes((char *)ptb, TBLOCK);	/* copy of TCB */
				swabbytes(bigbuf, bigsize);	/* io buffer  */
			} else {
				swapflg = 0;
			}
			if (H_TYPE(hdrtype) == H_BAR) {
				comerrno(EX_BAD, "Can't handle bar archives (yet).\n");
			}
			if (H_TYPE(hdrtype) == H_UNDEF) {
				int	t;

				switch (t = get_compression(ptb)) {

				case C_NONE:
					break;
				case C_PACK:
				case C_GZIP:
				case C_LZW:
				case C_FREEZE:
				case C_LZH:
				case C_PKZIP:
					comerrno(EX_BAD, "Archive is '%s' compressed, try to use the -z option.\n",
							get_cmpname(t));
					break;
				case C_BZIP2:
					comerrno(EX_BAD, "Archive is 'bzip2' compressed, try to use the -bz option.\n");
					break;
				case C_LZO:
					comerrno(EX_BAD, "Archive is 'lzop' compressed, try to use the -bz option.\n");
					break;
				case C_7Z:
					comerrno(EX_BAD, "Archive is '7z' compressed, try to use the -7z option.\n");
					break;
				case C_XZ:
					comerrno(EX_BAD, "Archive is 'xz' compressed, try to use the -xz option.\n");
					break;
				case C_LZIP:
					comerrno(EX_BAD, "Archive is 'lzip' compressed, try to use the -lzip option.\n");
					break;
				case C_ZSTD:
					comerrno(EX_BAD, "Archive is 'zstd' compressed, try to use the -zstd option.\n");
					break;
				case C_LZMA:
					comerrno(EX_BAD, "Archive is 'lzma' compressed, try to use the -lzma option.\n");
					break;
				case C_FREEZE2:
					comerrno(EX_BAD, "Archive is 'freeze2' compressed, try to use the -freeze option.\n");
					break;
				default:
					errmsgno(EX_BAD, "WARNING: Unknown compression type %d.\n", t);
					break;
				}
				if (!ignoreerr) {
					comerrno(EX_BAD,
					"Unknown archive type (neither tar, nor bar/cpio).\n");
				}
			}
			if ((chdrtype != H_UNDEF || (rflag || uflag)) &&
			    chdrtype != hdrtype) {
				errmsgno(EX_BAD, "Found: ");
				print_hdrtype(stderr, hdrtype);
				if (chdrtype == H_UNDEF) {
					chdrtype = hdrtype;
					if (rflag || uflag) {
						setprops(hdrtype);
						star_verifyopts();
					}
				} else {
					errmsgno(EX_BAD,
						"WARNING: extracting as ");
					print_hdrtype(stderr, chdrtype);
				}
				hdrtype = chdrtype;
			}
			setprops(hdrtype);
			/*
			 * If the archive format contains extended headers, we
			 * need to set up iconv().
			 */
			if (props.pr_flags & PR_XHDR) {
				int	t = S_EXTRACT;

				if (rflag || uflag)
					t |= S_CREATE;
				utf8_init(t);	/* iconv() setup for xhd */
			}
			/*
			 * Wake up fifo (first block has been swapped above)
			 * buf_resume() will trigger a shadow call to
			 * setprops() in the fifo process to make sure that
			 * the props structure is correct even in the second
			 * process.
			 */
			buf_resume();
			buf_rwake(props.pr_hdrsize); /* eat up archive header */
			if (iskip)
				buf_rwake(iskip);
		} else {
			if (readblock((char *)ptb, props.pr_hdrsize) == EOF) {
				errmsgno(EX_BAD,
				"Hard EOF on input, first EOF block is missing at %lld.\n",
				tblocks());
#ifdef	FIFO_EOF_DEBUG
				if (use_fifo)	/* Debug a rare EOF problem */
					fifo_prmp(1);
#endif
				xstats.s_hardeof++;
				return (EOF);
			}
		}

		if (H_TYPE(hdrtype) >= H_CPIO_BASE) {
			/*
			 * CPIO EOF check is currently in cpiotcb_to_info()
			 * There is no checksum for the CPIO header.
			 */
			check = ocheck = 1;
		} else {
			/*
			 * We have a TAR type archive.
			 */
			eof = (ptb->dbuf.t_name[0] == '\0') && checkeof(ptb);
			if (eof && !ignoreerr) {
				return (EOF);
			}
			/*
			 * XXX Hier muß eine Universalchecksummenüberprüfung hin
			 * XXX Shouldn't we use tarsum_ok() from here?
			 */
			stoli(ptb->dbuf.t_chksum, &ocheck, 7);
			check = checksum(ptb);
		}
		/*
		 * check == 0 : genullter Block.
		 */
		if (check != 0 && ocheck == check) {
			char	*tmagic = ptb->ustar_dbuf.t_magic;

			switch (H_TYPE(hdrtype)) {

			case H_XUSTAR:
			case H_EXUSTAR:
				if (ismagic(tmagic) && isxmagic(ptb))
					return (0);
				/*
				 * Both formats are equivalent.
				 * Acept XSTAR too.
				 */
				/* FALLTHROUGH */
			case H_XSTAR:
				if (ismagic(tmagic) &&
				    isstmagic(ptb->xstar_dbuf.t_xmagic))
					return (0);
				break;
			case H_PAX:
			case H_EPAX:
			case H_USTAR:
			case H_SUNTAR:
				if (ismagic(tmagic))
					return (0);
				break;
			case H_GNUTAR:
				if (isgnumagic(tmagic))
					return (0);
				break;
			case H_STAR:
				tmagic = ptb->star_dbuf.t_magic;
				if (ptb->dbuf.t_vers < STVERSION ||
				    isstmagic(tmagic))
				return (0);
				break;
			default:
			case H_TAR:
			case H_OTAR:
				return (0);

			case H_CPIO_CHR:		/* cpio -Hodc */
				if (strxneql((char *)ptb, "070707", 6))
					return (0);
				break;
			case H_CPIO_ASC:		/* cpio -c */
				if (strxneql((char *)ptb, "070701", 6))
					return (0);
				break;
			case H_CPIO_ACRC:		/* cpio -Hcrc */
				if (strxneql((char *)ptb, "070702", 6))
					return (0);
				break;
			case H_CPIO_NBIN:
			case H_CPIO_CRC:
				errmsgno(EX_BAD, "Unimplemented %ld cpio format.\n",
					hdrtype);
				break;
			case H_CPIO_BIN:		/* cpio default */
				if (strxneql((char *)ptb, "\161\307", 2))
					return (0);
				if (strxneql((char *)ptb, "\307\161", 2))
					return (0);
				break;
			}
			switch (H_TYPE(hdrtype)) {

			case H_CPIO_CHR:		/* cpio -Hodc */
			case H_CPIO_ASC:		/* cpio -c */
			case H_CPIO_ACRC:		/* cpio -Hcrc */
							/* First Block# is 0 */
				errmsgno(EX_BAD, "Wrong magic at: %lld: '%.6s'.\n",
						tblocks(), (char *)ptb);
				break;
			case H_CPIO_NBIN:
			case H_CPIO_CRC:
			case H_CPIO_BIN:		/* cpio default */
							/* First Block# is 0 */
				errmsgno(EX_BAD, "Wrong magic at: %lld: '0%6.6o'.\n",
						tblocks(),
						(((char *)ptb)[0] & 0xFF) * 256 +
						(((char *)ptb)[1] & 0xFF));
				break;
			default:
							/* First Block# is 0 */
				errmsgno(EX_BAD, "Wrong magic at: %lld: '%.8s'.\n",
						tblocks(), tmagic);
			}
			/*
			 * Allow buggy gnu Volheaders & Multivolheaders to work
			 */
			if (H_TYPE(hdrtype) == H_GNUTAR)
				return (0);

		} else if (eof) {
			errmsgno(EX_BAD, "EOF Block at: %lld ignored.\n",
							tblocks());
		} else {
			if (signedtarsum(ptb, ocheck))
				return (0);
			errmsgno(EX_BAD, "Checksum error at: %lld: 0%lo should be 0%lo.\n",
							tblocks(),
							ocheck, check);
		}
		if (ignoreerr) {
			if (H_TYPE(hdrtype) >= H_CPIO_BASE)
				cpio_resync();
		}
	} while (ignoreerr);
	exprstats(EX_BAD);
	/* NOTREACHED */
	return (EOF);		/* Keep lint happy */
}

EXPORT void
put_tcb(ptb, info)
	TCB	*ptb;
	register FINFO	*info;
{
	TCB	tb;
	int	x1 = 0;
	int	x2 = 0;
	BOOL	xhdr = FALSE;
extern	BOOL	ghdr;

	if ((props.pr_flags & PR_CPIO) != 0) {
		put_cpioh(ptb, info);
		return;
	}

	if (info->f_flags & (F_LONGNAME|F_LONGLINK))
		x1++;
	if (info->f_xflags & (XF_PATH|XF_LINKPATH))
		x1++;

/* XXX start alter code und Test */
	if (((info->f_flags & F_ADDSLASH) ? 1:0 +
	    info->f_namelen > props.pr_maxsname &&
	    (ptb->dbuf.t_prefix[0] == '\0' || H_TYPE(hdrtype) == H_GNUTAR)) ||
		    info->f_lnamelen > props.pr_maxslname)
		x2++;

	if ((x1 != x2) && info->f_xftype != XT_META) {
error("type: %ld name: '%s' x1 %d x2 %d namelen: %ld prefix: '%s' lnamelen: %ld\n",
info->f_filetype, info->f_name, x1, x2,
info->f_namelen, ptb->dbuf.t_prefix, info->f_lnamelen);
	}
/* XXX ende alter code und Test */

	if (props.pr_flags & PR_XHDR) {
		if (!(info->f_xflags & XF_PATH)) {
			if (info->f_name && !nameascii(info->f_name))
				info->f_xflags |= XF_PATH;
		}
		if (!(info->f_xflags & XF_LINKPATH) && info->f_lnamelen) {
			if (info->f_lname && !nameascii(info->f_lname))
				info->f_xflags |= XF_LINKPATH;
		}
	}

	if (x1 || x2 || (info->f_xflags != 0) || ghdr)
		xhdr = TRUE;

	if (!multivol && tsize > 0) {
		Llong	left;
		off_t	size = info->f_rsize;

		left = tsize - tblocks();
		if (xhdr)
			left -= 6;	/* Extimated for Longname/Longlink */

		if (is_link(info))
			size = 0L;
						/* file + tcb + EOF */
		if (left < (tarblocks(size)+1+2)) {
			if ((info->f_flags & F_TCB_BUF) != 0) {
				movetcb(ptb, &tb);
				ptb = &tb;
				info->f_flags &= ~F_TCB_BUF;
			}
			nextotape();
			newvolhdr((char *)NULL, 0, use_fifo);
		}
	}

	if (xhdr) {
		if ((info->f_flags & F_TCB_BUF) != 0) {	/* TCB is on buffer */
			movetcb(ptb, &tb);
			ptb = &tb;
			info->f_flags &= ~F_TCB_BUF;
		}
		if ((info->f_xflags != 0) || ghdr) {
			info_to_xhdr(info, ptb);
		} else {
			write_longnames(info);
		}
	}
	write_tcb(ptb, info);
}

EXPORT void
write_tcb(ptb, info)
	TCB	*ptb;
	register FINFO	*info;
{
	char	*addr;

	if (!nullout) {				/* 17 (> 16) Bit !!! */
		if (props.pr_fillc == '0')
			litos(ptb->dbuf.t_chksum, checksum(ptb) & 0x1FFFF, 7);
		else
			litos(ptb->dbuf.t_chksum, checksum(ptb) & 0x1FFFF, 6);
	}
	if ((info->f_flags & F_TCB_BUF) != 0) {	/* TCB is on buffer */
		addr = (char *)ptb;
		put_block(TBLOCK);
	} else {
		addr = writeblock((char *)ptb);
	}
	marktcb(addr);
}

EXPORT void
info_to_tcb(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
	if (nullout)
		return;

	if (H_TYPE(hdrtype) >= H_CPIO_BASE) {
		cpioinfo_to_tcb(info, ptb);
		return;
	}

	if (props.pr_fillc == '0') {
		/*
		 * This is a POSIX compliant header, it is allowed to use
		 * 7 bytes from 8 byte headers as POSIX only requires a ' ' or
		 * '\0' as last char.
		 */
		if (modebits)
			litos(ptb->dbuf.t_mode, (Ulong)(info->f_mode|info->f_type) & 0xFFFF, 7);
		else
			litos(ptb->dbuf.t_mode, (Ulong)info->f_mode & 0xFFFF, 7);

		if (info->f_uid > MAXOCTAL7) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_UID;

			if (props.pr_flags & PR_BASE256) {
#if	SIZEOF_UID_T > SIZEOF_UNSIGNED_LONG_INT
				if (!(info->f_uid <= ULONG_MAX))
					llbtos(ptb->dbuf.t_uid, (Ullong)info->f_uid, 7);
				else
#endif
					btos(ptb->dbuf.t_uid, info->f_uid, 7);
			} else {
				/*
				 * Use uid_nobody?
				 */
				litos(ptb->dbuf.t_uid, (Ulong)0, 7);
				if ((info->f_xflags & XF_UID) == 0 &&
				    !errhidden(E_ID, info->f_name)) {
					if (!errwarnonly(E_ID, info->f_name))
						xstats.s_id++;
					errmsgno(EX_BAD,
						"Uid %lld for '%s' out of range.\n",
						(Ullong)info->f_uid, info->f_name);
					(void) errabort(E_ID, info->f_name, TRUE);
				}
			}
		} else {
			litos(ptb->dbuf.t_uid, info->f_uid, 7);
		}

		if (info->f_gid > MAXOCTAL7) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_GID;

			if (props.pr_flags & PR_BASE256) {
#if	SIZEOF_GID_T > SIZEOF_UNSIGNED_LONG_INT
				if (!(info->f_gid <= ULONG_MAX))
					llbtos(ptb->dbuf.t_gid, (Ullong)info->f_gid, 7);
				else
#endif
					btos(ptb->dbuf.t_gid, info->f_gid, 7);
			} else {
				/*
				 * Use gid_nobody?
				 */
				litos(ptb->dbuf.t_gid, (Ulong)0, 7);
				if ((info->f_xflags & XF_GID) == 0 &&
				    !errhidden(E_ID, info->f_name)) {
					if (!errwarnonly(E_ID, info->f_name))
						xstats.s_id++;
					errmsgno(EX_BAD,
						"Gid %lld for '%s' out of range.\n",
						(Ullong)info->f_gid, info->f_name);
					(void) errabort(E_ID, info->f_name, TRUE);
				}
			}
		} else {
			litos(ptb->dbuf.t_gid, info->f_gid, 7);
		}
	} else {
		/*
		 * This is a pre POSIX header, it is only allowed to use
		 * 6 bytes from 8 byte headers as historic TAR requires a ' '
		 * and a '\0' as last char.
		 */
		if (modebits)
			litos(ptb->dbuf.t_mode, (Ulong)(info->f_mode|info->f_type) & 0xFFFF, 6);
		else
			litos(ptb->dbuf.t_mode, (Ulong)info->f_mode & 0xFFFF, 6);

		if (info->f_uid > MAXOCTAL6) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_UID;

			if (props.pr_flags & PR_BASE256) {
#if	SIZEOF_UID_T > SIZEOF_UNSIGNED_LONG_INT
				if (!(info->f_uid <= ULONG_MAX))
					llbtos(ptb->dbuf.t_uid, (Ullong)info->f_uid, 7);
				else
#endif
					btos(ptb->dbuf.t_uid, info->f_uid, 7);
			} else {
				/*
				 * Use uid_nobody?
				 */
				litos(ptb->dbuf.t_uid, (Ulong)0, 6);
				if ((info->f_xflags & XF_UID) == 0 &&
				    !errhidden(E_ID, info->f_name)) {
					if (!errwarnonly(E_ID, info->f_name))
						xstats.s_id++;
					errmsgno(EX_BAD,
						"Uid %lld for '%s' out of range.\n",
						(Ullong)info->f_uid, info->f_name);
					(void) errabort(E_ID, info->f_name, TRUE);
				}
			}
		} else {
			litos(ptb->dbuf.t_uid, info->f_uid, 6);
		}

		if (info->f_gid > MAXOCTAL6) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_GID;

			if (props.pr_flags & PR_BASE256) {
#if	SIZEOF_GID_T > SIZEOF_UNSIGNED_LONG_INT
				if (!(info->f_gid <= ULONG_MAX))
					llbtos(ptb->dbuf.t_gid, (Ullong)info->f_gid, 7);
				else
#endif
					btos(ptb->dbuf.t_gid, info->f_gid, 7);
			} else {
				/*
				 * Use gid_nobody?
				 */
				litos(ptb->dbuf.t_gid, (Ulong)0, 6);
				if ((info->f_xflags & XF_GID) == 0 &&
				    !errhidden(E_ID, info->f_name)) {
					if (!errwarnonly(E_ID, info->f_name))
						xstats.s_id++;
					errmsgno(EX_BAD,
						"Gid %lld for '%s' out of range.\n",
						(Ullong)info->f_gid, info->f_name);
					(void) errabort(E_ID, info->f_name, TRUE);
				}
			}
		} else {
			litos(ptb->dbuf.t_gid, info->f_gid, 6);
		}
	}

	if (info->f_rsize > MAXOCTAL11 && (props.pr_flags & PR_XHDR)) {
		info->f_xflags |= XF_SIZE;
	}
/* XXX */
	if (info->f_rsize <= MAXINT32) {
		litos(ptb->dbuf.t_size, (Ulong)info->f_rsize, 11);
	} else {
		if (info->f_rsize > MAXOCTAL11 &&
		    (props.pr_flags & PR_BASE256) == 0) {
			litos(ptb->dbuf.t_size, (Ulong)0, 11);
		} else {
			llitos(ptb->dbuf.t_size, (Ullong)info->f_rsize, 11);
		}
	}

	if (info->f_mtime < 0 || info->f_mtime > MAXOCTAL11) {
		if (props.pr_flags & PR_XHDR)
			info->f_xflags |= XF_ATIME|XF_MTIME|XF_CTIME;

		if (props.pr_flags & PR_BASE256) {
			if (info->f_mtime <= ULONG_MAX)
				btos(ptb->dbuf.t_mtime, info->f_mtime, 11);
			else
				llbtos(ptb->dbuf.t_mtime, (Ullong)info->f_mtime, 11);
		} else {
			litos(ptb->dbuf.t_mtime, (Ulong)info->f_mtime, 11);
			if ((info->f_xflags & XF_MTIME) == 0 &&
			    !errhidden(E_TIME, info->f_name)) {
				if (!errwarnonly(E_TIME, info->f_name))
					xstats.s_time++;
				errmsgno(EX_BAD,
					"Time %lld for '%s' out of range.\n",
					(Ullong)info->f_mtime, info->f_name);
				(void) errabort(E_TIME, info->f_name, TRUE);
			}
		}
	} else {
		if (props.pr_flags & PR_XHDR) {
			if (info->f_mnsec != 0)
				info->f_xflags |= XF_ATIME|XF_MTIME|XF_CTIME;
		}
		litos(ptb->dbuf.t_mtime, (Ulong)info->f_mtime, 11);
	}

	ptb->dbuf.t_linkflag = XTTOUS(info->f_xftype);

	if (H_TYPE(hdrtype) == H_USTAR) {
		info_to_ustar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_PAX) {
		info_to_ustar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_EPAX) {
		info_to_ustar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_SUNTAR) {
		info_to_ustar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_XSTAR) {
		info_to_xstar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_XUSTAR) {
		info_to_xstar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_EXUSTAR) {
		info_to_xstar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_GNUTAR) {
		info_to_gnutar(info, ptb);
	} else if (H_TYPE(hdrtype) == H_STAR) {
		info_to_star(info, ptb);
	}
}

/*
 * Used to create old star format header.
 */
LOCAL void
info_to_star(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
	ptb->dbuf.t_vers = STVERSION;
	litos(ptb->dbuf.t_filetype, info->f_filetype & 0xFFFF, 6);	/* XXX -> 7 ??? */
	litos(ptb->dbuf.t_type, (Ulong)info->f_type & 0xFFFF, 11);
#ifdef	needed
	/* XXX we need to do something if st_rdev is > 32 bits */
	if ((info->f_rdevmaj > MAXOCTAL7 || info->f_rdevmin > MAXOCTAL7) &&
	    (props.pr_flags & PR_XHDR)) {
		info->f_xflags |= XF_DEVMAJOR|XF_DEVMINOR;
	}
#endif

#if	(SIZEOF_DEV_T > SIZEOF_UNSIGNED_LONG_INT) || (SIZEOF_DEV_T > 4)
	if (info->f_rdev > MAXOCTAL11) {
		if (info->f_rdev <= ULONG_MAX)
			btos(ptb->dbuf.t_rdev, info->f_rdev, 10);
		else
			llbtos(ptb->dbuf.t_rdev, (Ullong)info->f_rdev, 10);
	} else
#endif
		litos(ptb->dbuf.t_rdev, info->f_rdev, 11);

#ifdef	DEV_MINOR_NONCONTIG
	ptb->dbuf.t_devminorbits = '@';
	if (props.pr_flags & PR_XHDR) {
		info->f_xflags |= XF_DEVMAJOR|XF_DEVMINOR;
	}
#else
	ptb->dbuf.t_devminorbits = '@' + minorbits;
#endif

	if (info->f_atime < 0 || info->f_atime > MAXOCTAL11) {
		if (info->f_atime <= ULONG_MAX)
			btos(ptb->dbuf.t_atime, info->f_atime, 11);
		else
			llbtos(ptb->dbuf.t_atime, (Ullong)info->f_atime, 11);
	} else {
		litos(ptb->dbuf.t_atime, (Ulong)info->f_atime, 11);
	}
	if (info->f_ctime < 0 || info->f_ctime > MAXOCTAL11) {
		if (info->f_ctime <= ULONG_MAX)
			btos(ptb->dbuf.t_ctime, info->f_ctime, 11);
		else
			llbtos(ptb->dbuf.t_ctime, (Ullong)info->f_ctime, 11);
	} else {
		litos(ptb->dbuf.t_ctime, (Ulong)info->f_ctime, 11);
	}
	ptb->dbuf.t_magic[0] = 't';
	ptb->dbuf.t_magic[1] = 'a';
	ptb->dbuf.t_magic[2] = 'r';
	if (!numeric) {
		char	opfx0 = ptb->dbuf.t_prefix[0];

		if (ic_nameuid(ptb->dbuf.t_uname, STUNMLEN+1, info->f_uid) >
									TRUE) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_UNAME;
		}
		/* XXX Korrektes overflowchecking */
		if (ptb->dbuf.t_uname[STUNMLEN-1] != '\0' &&
		    props.pr_flags & PR_XHDR) {
			info->f_xflags |= XF_UNAME;
		}
		if (ic_namegid(ptb->dbuf.t_gname, STGNMLEN+1, info->f_gid) >
									TRUE) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_GNAME;
		}
		/* XXX Korrektes overflowchecking */
		if (ptb->dbuf.t_gname[STGNMLEN-1] != '\0' &&
		    props.pr_flags & PR_XHDR) {
			info->f_xflags |= XF_GNAME;
		}
		if (*ptb->dbuf.t_uname) {
			info->f_uname = ptb->dbuf.t_uname;
			info->f_umaxlen = STUNMLEN;
		}
		if (*ptb->dbuf.t_gname) {
			info->f_gname = ptb->dbuf.t_gname;
			info->f_gmaxlen = STGNMLEN;
		}
		ptb->dbuf.t_prefix[0] = opfx0;	/* Overwritten by strlcpy() */
	}

	if (is_sparse(info) || is_multivol(info)) {
		if (info->f_size > MAXOCTAL11 && (props.pr_flags & PR_XHDR)) {
			info->f_xflags |= XF_REALSIZE;
		}
		/* XXX Korrektes overflowchecking fuer xhdr */
		if (info->f_size <= MAXINT32) {
			litos(ptb->xstar_in_dbuf.t_realsize, (Ulong)info->f_size, 11);
		} else {
			llitos(ptb->xstar_in_dbuf.t_realsize, (Ullong)info->f_size, 11);
		}
	}
	if (is_multivol(info)) {
		if (info->f_contoffset > MAXOCTAL11 && (props.pr_flags & PR_XHDR)) {
			info->f_xflags |= XF_OFFSET;
		}
		if ((info->f_xflags & XF_OFFSET) == 0) {
			/*
			 * Don't fill out contoffset if we have a xheader.
			 */
			if (info->f_contoffset <= MAXINT32) {
				litos(ptb->xstar_in_dbuf.t_offset,
					(Ulong)info->f_contoffset, 11);
			} else {
				llitos(ptb->xstar_in_dbuf.t_offset,
					(Ullong)info->f_contoffset, 11);
			}
		}
	}
}

/*
 * Used to create USTAR, PAX, SunTAR format header.
 */
LOCAL void
info_to_ustar(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
	ptb->ustar_dbuf.t_magic[0] = 'u';
	ptb->ustar_dbuf.t_magic[1] = 's';
	ptb->ustar_dbuf.t_magic[2] = 't';
	ptb->ustar_dbuf.t_magic[3] = 'a';
	ptb->ustar_dbuf.t_magic[4] = 'r';

	/*
	 * strncpy is slow: use handcrafted replacement.
	 */
	ptb->ustar_dbuf.t_version[0] = '0';
	ptb->ustar_dbuf.t_version[1] = '0';

	if (!numeric) {
		/* XXX Korrektes overflowchecking fuer xhdr */
		if (ic_nameuid(ptb->ustar_dbuf.t_uname, TUNMLEN, info->f_uid) >
									TRUE) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_UNAME;
		}
		/* XXX Korrektes overflowchecking fuer xhdr */
		if (ic_namegid(ptb->ustar_dbuf.t_gname, TGNMLEN, info->f_gid) >
									TRUE) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_GNAME;
		}
		if (*ptb->ustar_dbuf.t_uname) {
			info->f_uname = ptb->ustar_dbuf.t_uname;
			info->f_umaxlen = TUNMLEN;
		}
		if (*ptb->ustar_dbuf.t_gname) {
			info->f_gname = ptb->ustar_dbuf.t_gname;
			info->f_gmaxlen = TGNMLEN;
		}
	}
	if (info->f_rdevmaj > MAXOCTAL7) {
		if (props.pr_flags & PR_XHDR)
			info->f_xflags |= XF_DEVMAJOR;
		/*
		 * XXX If we ever need to write more than a long into
		 * XXX devmajor, we need to change llitos() to check
		 * XXX for 7 char limits too.
		 */
/* XXX */
		btos(ptb->ustar_dbuf.t_devmajor, info->f_rdevmaj, 7);
	} else {
		litos(ptb->ustar_dbuf.t_devmajor, info->f_rdevmaj, 7);
	}
#if	DEV_MINOR_BITS > 21		/* XXX */
	/*
	 * XXX The DEV_MINOR_BITS autoconf macro is only tested with 32 bit
	 * XXX ints but this does not matter as it is sufficient to know that
	 * XXX it will not fit into a 7 digit octal number.
	 */
	if (info->f_rdevmin > MAXOCTAL7) {
		extern	BOOL	hpdev;

		if (props.pr_flags & PR_XHDR) {
			info->f_xflags |= XF_DEVMINOR;
		}
		if ((info->f_rdevmin <= MAXOCTAL8) && hpdev) {
			char	c;

			/*
			 * Implement the method from HP-UX that allows 24 bit
			 * for the device minor number. Note that this method
			 * violates the POSIX specs.
			 */
			c = ptb->ustar_dbuf.t_prefix[0];
			litos(ptb->ustar_dbuf.t_devminor, info->f_rdevmin, 8);
			ptb->ustar_dbuf.t_prefix[0] = c;
		} else {
			/*
			 * XXX If we ever need to write more than a long into
			 * XXX devmainor, we need to change llitos() to check
			 * XXX for 7 char limits too.
			 */
/* XXX */
			btos(ptb->ustar_dbuf.t_devminor, info->f_rdevmin, 7);
		}
	} else
#endif
		{
		litos(ptb->ustar_dbuf.t_devminor, info->f_rdevmin, 7);
	}
}

/*
 * Used to create XSTAR, XUSTAR, EXUSTAR format header.
 */
LOCAL void
info_to_xstar(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
	info_to_ustar(info, ptb);

	if (info->f_atime < 0 || info->f_atime > MAXOCTAL11) {
		if (info->f_atime <= ULONG_MAX)
			btos(ptb->xstar_dbuf.t_atime, (Ulong)info->f_atime, 11);
		else
			llbtos(ptb->xstar_dbuf.t_atime, (Ullong)info->f_atime, 11);
	} else {
		litos(ptb->xstar_dbuf.t_atime, (Ulong)info->f_atime, 11);
	}
	if (info->f_ctime < 0 || info->f_ctime > MAXOCTAL11) {
		if (info->f_ctime <= ULONG_MAX)
			btos(ptb->xstar_dbuf.t_ctime, (Ulong)info->f_ctime, 11);
		else
			llbtos(ptb->xstar_dbuf.t_ctime, (Ullong)info->f_ctime, 11);
	} else {
		litos(ptb->xstar_dbuf.t_ctime, (Ulong)info->f_ctime, 11);
	}

	/*
	 * Help recognition in isxmagic(), make sure that prefix[130] is null.
	 */
	ptb->xstar_dbuf.t_prefix[130] = '\0';

	if (H_TYPE(hdrtype) == H_XSTAR) {
		ptb->xstar_dbuf.t_xmagic[0] = 't';
		ptb->xstar_dbuf.t_xmagic[1] = 'a';
		ptb->xstar_dbuf.t_xmagic[2] = 'r';
	}
	if (is_sparse(info) || is_multivol(info)) {
		if (info->f_size > MAXOCTAL11 && (props.pr_flags & PR_XHDR)) {
			info->f_xflags |= XF_REALSIZE;
		}
		/* XXX Korrektes overflowchecking fuer xhdr */
		if (info->f_size <= MAXINT32) {
			litos(ptb->xstar_in_dbuf.t_realsize, (Ulong)info->f_size, 11);
		} else {
			llitos(ptb->xstar_in_dbuf.t_realsize, (Ullong)info->f_size, 11);
		}
	}
	if (is_multivol(info)) {
		if (info->f_contoffset > MAXOCTAL11 && (props.pr_flags & PR_XHDR)) {
			info->f_xflags |= XF_OFFSET;
		}
		if ((info->f_xflags & XF_OFFSET) == 0) {
			/*
			 * Don't fill out contoffset if we have a xheader.
			 */
			if (info->f_contoffset <= MAXINT32) {
				litos(ptb->xstar_in_dbuf.t_offset,
					(Ulong)info->f_contoffset, 11);
			} else {
				llitos(ptb->xstar_in_dbuf.t_offset,
					(Ullong)info->f_contoffset, 11);
			}
		}
	}
}

/*
 * Used to create GNU tar format header.
 */
LOCAL void
info_to_gnutar(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
	strcpy(ptb->gnu_dbuf.t_magic, gmagic);

	if (!numeric) {
		if (ic_nameuid(ptb->ustar_dbuf.t_uname, TUNMLEN, info->f_uid) >
									TRUE) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_UNAME;
		}
		if (ic_namegid(ptb->ustar_dbuf.t_gname, TGNMLEN, info->f_gid) >
									TRUE) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_GNAME;
		}
		if (*ptb->ustar_dbuf.t_uname) {
			info->f_uname = ptb->ustar_dbuf.t_uname;
			info->f_umaxlen = TUNMLEN;
		}
		if (*ptb->ustar_dbuf.t_gname) {
			info->f_gname = ptb->ustar_dbuf.t_gname;
			info->f_gmaxlen = TGNMLEN;
		}
	}
	if (info->f_xftype == XT_CHR || info->f_xftype == XT_BLK) {
		if (info->f_rdevmaj > MAXOCTAL7) {
			btos(ptb->ustar_dbuf.t_devmajor, info->f_rdevmaj, 7);
		} else {
			litos(ptb->ustar_dbuf.t_devmajor, info->f_rdevmaj, 7);
		}
		if (info->f_rdevmin > MAXOCTAL7) {
			btos(ptb->ustar_dbuf.t_devminor, info->f_rdevmin, 7);
		} else {
			litos(ptb->ustar_dbuf.t_devminor, info->f_rdevmin, 7);
		}
	}

	/*
	 * XXX GNU tar only fill this if doing a gnudump.
	 */
	if (info->f_atime < 0 || info->f_atime > MAXOCTAL11) {
		if (info->f_atime <= ULONG_MAX)
			btos(ptb->gnu_dbuf.t_atime, (Ulong)info->f_atime, 11);
		else
			llbtos(ptb->gnu_dbuf.t_atime, (Ullong)info->f_atime, 11);
	} else {
		litos(ptb->gnu_dbuf.t_atime, (Ulong)info->f_atime, 11);
	}
	if (info->f_ctime < 0 || info->f_ctime > MAXOCTAL11) {
		if (info->f_ctime <= ULONG_MAX)
			btos(ptb->gnu_dbuf.t_ctime, (Ulong)info->f_ctime, 11);
		else
			llbtos(ptb->gnu_dbuf.t_ctime, (Ullong)info->f_ctime, 11);
	} else {
		litos(ptb->gnu_dbuf.t_ctime, (Ulong)info->f_ctime, 11);
	}

	if (is_sparse(info)) {
		if (info->f_size <= MAXINT32) {
			litos(ptb->gnu_in_dbuf.t_realsize, (Ulong)info->f_size, 11);
		} else {
			llitos(ptb->gnu_in_dbuf.t_realsize, (Ullong)info->f_size, 11);
		}
	}
}

EXPORT int
tcb_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	int	ret = 0;
	char	xname;
	char	xlink;
	char	xpfx;
	Ulong	ul;
	Ullong	ull;
	int	xt = XT_BAD;
	int	rxt = XT_BAD;
static	BOOL	posixwarn = FALSE;
static	BOOL	namewarn = FALSE;
static	BOOL	modewarn = FALSE;

	/*
	 * F_HAS_NAME is only used from list.c when the -listnew option is
	 * present. Keep f_lname and f_name, don't read LF_LONGLINK/LF_LONGNAME
	 * in this case.
	 */
	info->f_namelen = info->f_lnamelen = 0;
	info->f_uname = info->f_gname = NULL;
	info->f_umaxlen = info->f_gmaxlen = 0L;
	info->f_xftype = XT_BAD;
	info->f_rxftype = XT_BAD;
	info->f_xflags = 0;
	info->f_contoffset = (off_t)0;
	info->f_flags &= F_HAS_NAME;
	info->f_timeres = 1L;
	info->f_fflags = 0L;
	info->f_nlink = 0;
	info->f_dir = NULL;
	info->f_dirinos = NULL;
	info->f_dirents = 0;
	info->f_llsize = 0;
	info->f_devminorbits = 0;

	tcb_to_xhdr_reset();	/* Falsch wenn wir mehr als @ in list wollen */

	if (H_TYPE(hdrtype) >= H_CPIO_BASE)
		return (cpiotcb_to_info(ptb, info));

	while (pr_isxheader(ptb->dbuf.t_linkflag)) {
		/*
		 * Handle POSIX.1-2001 extensions.
		 */
		if ((ptb->dbuf.t_linkflag == LF_XHDR ||
				    ptb->dbuf.t_linkflag == LF_GHDR ||
				    ptb->dbuf.t_linkflag == LF_VU_XHDR)) {
			ret = tcb_to_xhdr(ptb, info);
			info->f_flags &= ~F_DATA_SKIPPED;
			if (ret != 0)
				return (ret);

			xt  = info->f_xftype;
			rxt = info->f_rxftype;
		}
		/*
		 * Handle very long names the old (star & gnutar) way.
		 */
		if ((info->f_flags & F_HAS_NAME) == 0 &&
					props.pr_nflags & PR_LONG_NAMES) {
			while (ret == 0 &&
				    (ptb->dbuf.t_linkflag == LF_LONGLINK ||
				    ptb->dbuf.t_linkflag == LF_LONGNAME)) {
				ret = tcb_to_longname(ptb, info);
				info->f_flags &= ~F_DATA_SKIPPED;
			}
			if (ret)
				return (ret);
		}
	}
	if (!pr_validtype(ptb->dbuf.t_linkflag)) {
		errmsgno(EX_BAD,
		"WARNING: Archive contains unknown typeflag '%c' (0x%02X) at %lld.\n",
			ptb->dbuf.t_linkflag, ptb->dbuf.t_linkflag, tblocks());
	}

	if (ptb->ndbuf.t_name[NAMSIZ] == '\0') { /* "ndbuf" is NAMSIZE+1 */
		if (ptb->dbuf.t_name[NAMSIZ-1] == '\0') {
			if (!nowarn && !modewarn) {
				errmsgno(EX_BAD,
				"WARNING: Archive violates POSIX 1003.1 (mode field starts with null byte).\n");
				modewarn = TRUE;
			}
		} else if (!nowarn && !namewarn) {
			errmsgno(EX_BAD,
			"WARNING: Archive violates POSIX 1003.1 (100 char filename is null terminated).\n");
			namewarn = TRUE;
		}
		ptb->ndbuf.t_name[NAMSIZ] = ' ';
	}
	stoli(ptb->dbuf.t_mode, &ul, 7);
	info->f_mode = ul;
	if (info->f_mode & ~07777) {
		if (!nowarn && !modebits && H_TYPE(hdrtype) == H_USTAR && !posixwarn) {
			errmsgno(EX_BAD,
			"WARNING: Archive violates POSIX 1003.1 (too many bits in mode field).\n");
			posixwarn = TRUE;
		}
		info->f_mode &= 07777;
	}
	if ((info->f_xflags & XF_UID) == 0) {
		if (ptb->dbuf.t_uid[0] & 0x80)
			stob(ptb->dbuf.t_uid, &ul, 7);
		else
			stoli(ptb->dbuf.t_uid, &ul, 7);
		info->f_uid = ul;
		if (info->f_uid != ul) {
			print_hrange("uid", (Ullong)ul);
			info->f_flags |= F_BAD_UID;
			info->f_uid = ic_uid_nobody();
		}
	}
	if ((info->f_xflags & XF_UID) == 0) {
		if (ptb->dbuf.t_gid[0] & 0x80)
			stob(ptb->dbuf.t_gid, &ul, 7);
		else
			stoli(ptb->dbuf.t_gid, &ul, 7);
		info->f_gid = ul;
		if (info->f_gid != ul) {
			print_hrange("gid", (Ullong)ul);
			info->f_flags |= F_BAD_GID;
			info->f_gid = ic_gid_nobody();
		}
	}

	/*
	 * Possible flags from extended header:
	 * XF_SIZE|XF_REALSIZE	Found either	"size" or
	 *					"SCHILY.realsize" and "size"
	 * XF_REALSIZE		Found only	"SCHILY.realsize"
	 * 0			Found neither	"SCHILY.realsize" nor "size"
	 */
	if ((info->f_xflags & XF_SIZE) == 0) {
		stolli(ptb->dbuf.t_size, &ull);
		if ((info->f_xflags & XF_SIZE) == 0) {
			info->f_rsize = info->f_llsize = ull;
			if (info->f_rsize != ull) {
				print_hrange("size", ull);
				info->f_rsize = 0,
				info->f_flags |= (F_BAD_META | F_BAD_SIZE);
			}
		}
		if ((info->f_xflags & XF_REALSIZE) == 0)
			info->f_size = ull;
	}

	switch (ptb->dbuf.t_linkflag) {

	case LNKTYPE:
		if (linkdata)
			break;
		if (props.pr_flags & PR_LINK_DATA)
			break;
		/* FALLTHROUGH */
	case DIRTYPE:
	case CHRTYPE:
	case BLKTYPE:
	case FIFOTYPE:
	case LF_META:
		info->f_rsize = 0L;
		info->f_llsize = 0L;
		break;

	default:
		/*
		 * Nothing to do here, we did handle this already above the
		 * switch statement.
		 */
		break;
	}

	if ((info->f_xflags & XF_MTIME) == 0) {
		if (ptb->dbuf.t_mtime[0] & 0x80)
			stob(ptb->dbuf.t_mtime, &ul, 11);
		else
			stoli(ptb->dbuf.t_mtime, &ul, 11);
		info->f_mtime = (time_t)ul;
		info->f_mnsec = 0L;
		if (info->f_mtime != ul) {
			print_hrange("time", (Ullong)ul);
			info->f_mtime = 0;
		}
	}

	info->f_typeflag = ptb->ustar_dbuf.t_typeflag;

	switch (H_TYPE(hdrtype)) {

	default:
	case H_TAR:
	case H_OTAR:
		tar_to_info(ptb, info);
		break;
	case H_PAX:
	case H_EPAX:
	case H_USTAR:
	case H_SUNTAR:
		ustar_to_info(ptb, info);
		break;
	case H_XSTAR:
	case H_XUSTAR:
	case H_EXUSTAR:
		xstar_to_info(ptb, info);
		break;
	case H_GNUTAR:
		gnutar_to_info(ptb, info);
		break;
	case H_STAR:
		star_to_info(ptb, info);
		break;
	}
	info->f_rxftype = info->f_xftype;
	if (rxt != XT_BAD) {
		info->f_rxftype = rxt;
		info->f_filetype = XTTOST(info->f_rxftype);
		info->f_type = XTTOIF(info->f_rxftype);
		/*
		 * XT_LINK may be any 'real' file type,
		 * XT_META may be either a regular file or a contigouos file.
		 */
		if (info->f_xftype != XT_LINK && info->f_xftype != XT_META)
			info->f_xftype = info->f_rxftype;
	}
	if (xt != XT_BAD) {
		info->f_xftype = xt;
	}

	/*
	 * Hack for list module (option -newest) ...
	 * Save and restore t_name[NAMSIZ] & t_linkname[NAMSIZ]
	 * Use "ndbuf" to permit NAMESIZE as index.
	 */
	xname = ptb->ndbuf.t_name[NAMSIZ];
	ptb->ndbuf.t_name[NAMSIZ] = '\0';	/* allow 100 chars in name */
	xlink = ptb->ndbuf.t_linkname[NAMSIZ];
	ptb->ndbuf.t_linkname[NAMSIZ] = '\0'; /* allow 100 chars in linkname */
	xpfx = ptb->ndbuf.t_prefix[PFXSIZ];
	ptb->ndbuf.t_prefix[PFXSIZ] = '\0';	/* allow 155 chars in prefix */

	/*
	 * Handle long name in posix split form now.
	 * Also copy ptb->dbuf.t_linkname[] if namelen is == 100.
	 */
	tcb_to_name(ptb, info);

	ptb->ndbuf.t_name[NAMSIZ] = xname;	/* restore remembered value */
	ptb->ndbuf.t_linkname[NAMSIZ] = xlink;	/* restore remembered value */
	ptb->ndbuf.t_prefix[PFXSIZ] = xpfx;	/* restore remembered value */

	if (info->f_flags & F_BAD_UID)
		info->f_mode &= ~TSUID;
	if (info->f_flags & F_BAD_GID)
		info->f_mode &= ~TSGID;

	return (ret);
}

/*
 * Used to convert from old tar format header.
 */
LOCAL void
tar_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	register int	typeflag = ptb->ustar_dbuf.t_typeflag;
		char	xname;

	xname = ptb->ndbuf.t_name[NAMSIZ];
	ptb->ndbuf.t_name[NAMSIZ] = '\0';	/* allow 100 chars in name */

	if (ptb->ndbuf.t_name[0] != '\0' &&
	    ptb->ndbuf.t_name[strlen(ptb->ndbuf.t_name) - 1] == '/') {
		typeflag = DIRTYPE;
		info->f_filetype = F_DIR;
		info->f_rsize = (off_t)0;	/* XXX hier?? siehe oben */
	} else if (typeflag == SYMTYPE) {
		info->f_filetype = F_SLINK;
	} else if (typeflag != DIRTYPE) {
		info->f_filetype = F_FILE;
	}
	ptb->ndbuf.t_name[NAMSIZ] = xname;	/* restore remembered value */

	info->f_xftype = USTOXT(typeflag);
	info->f_type = XTTOIF(info->f_xftype);
	info->f_rdevmaj = info->f_rdevmin = info->f_rdev = 0;
	info->f_ctime = info->f_atime = info->f_mtime;
	info->f_cnsec = info->f_ansec = 0L;
}

/*
 * Used to convert from old star format header.
 */
LOCAL void
star_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	Ulong	id;
	uid_t	uid;
	gid_t	gid;
	Ullong	ull;
	int	mbits;

	version = ptb->dbuf.t_vers;
	if (ptb->dbuf.t_vers < STVERSION) {
		tar_to_info(ptb, info);
		return;
	}
	stoli(ptb->dbuf.t_filetype, &info->f_filetype, 7);
	stoli(ptb->dbuf.t_type, &id, 11);
	info->f_type = id;
	/*
	 * star Erweiterungen sind wieder ANSI kompatibel, d.h. linkflag
	 * hält den echten Dateityp (LONKLINK, LONGNAME, SPARSE ...)
	 */
	if (ptb->dbuf.t_linkflag < '1')
		info->f_xftype = IFTOXT(info->f_type);
	else
		info->f_xftype = USTOXT(ptb->ustar_dbuf.t_typeflag);

	if (ptb->dbuf.t_rdev[0] & 0x80)
		stob(ptb->dbuf.t_rdev, &id, 10); /* Last c is t_devminorbits */
	else
		stoli(ptb->dbuf.t_rdev, &id, 11);
	info->f_rdev = id;
	if ((info->f_rdev != id) && is_dev(info)) {
		print_hrange("rdev", (Ullong)id);
		info->f_flags |= F_BAD_META;
	}
	if ((info->f_xflags & (XF_DEVMAJOR|XF_DEVMINOR)) !=
						(XF_DEVMAJOR|XF_DEVMINOR)) {
		mbits = ptb->dbuf.t_devminorbits - '@';
		if (((mbits + CHAR_BIT-1) / CHAR_BIT) > sizeof (info->f_rdev)) {
			errmsgno(EX_BAD,
			"WARNING: Too many (%d) minor bits in header at %lld.\n",
				mbits,
				tblocks());
			mbits = -1;
		}
		if (mbits == 0) {
			static	BOOL	dwarned = FALSE;
			if (!dwarned && is_dev(info)) {	/* Only warn for devices */
#ifdef	DEV_MINOR_NONCONTIG
				errmsgno(EX_BAD,
				"WARNING: Minor device numbers are non contiguous.\n");
				errmsgno(EX_BAD,
				"WARNING: Devices may not be extracted correctly.\n");
#else
				errmsgno(EX_BAD,
				"WARNING: The archiving system used non contiguous minor numbers.\n");
				errmsgno(EX_BAD,
				"WARNING: Cannot extract devices correctly.\n");
#endif
				dwarned = TRUE;
			}
			/*
			 * Let us hope that both, the archiving and the
			 * extracting system use the same major()/minor()
			 * mapping.
			 */
			info->f_rdevmaj	= major(info->f_rdev);
			info->f_rdevmin	= minor(info->f_rdev);
		} else {
			/*
			 * Convert from remote major()/minor() mapping to
			 * local major()/minor() mapping.
			 */
			if (mbits < 0)		/* Old star format */
				mbits = 8;
			info->f_rdevmaj	= _dev_major(mbits, info->f_rdev);
			info->f_rdevmin	= _dev_minor(mbits, info->f_rdev);
			info->f_rdev = makedev(info->f_rdevmaj, info->f_rdevmin);
		}
	}

	if ((info->f_xflags & XF_ATIME) == 0) {
		if (ptb->dbuf.t_atime[0] & 0x80)
			stob(ptb->dbuf.t_atime, &id, 11);
		else
			stoli(ptb->dbuf.t_atime, &id, 11);
		info->f_atime = (time_t)id;
		info->f_ansec = 0L;
	}
	if ((info->f_xflags & XF_CTIME) == 0) {
		if (ptb->dbuf.t_ctime[0] & 0x80)
			stob(ptb->dbuf.t_ctime, &id, 11);
		else
			stoli(ptb->dbuf.t_ctime, &id, 11);
		info->f_ctime = (time_t)id;
		info->f_cnsec = 0L;
	}

	if ((info->f_xflags & XF_UNAME) == 0) {
		if (*ptb->dbuf.t_uname) {
			info->f_uname = ptb->dbuf.t_uname;
			info->f_umaxlen = STUNMLEN;
		}
	}
	if (info->f_uname) {
		char	xname;

		xname = ptb->dbuf.t_gname[0];
		ptb->dbuf.t_gname[0] = '\0';
		if (!numeric && ic_uidname(info->f_uname, info->f_umaxlen, &uid)) {
			info->f_flags &= ~F_BAD_UID;
			info->f_uid = uid;
		}
		ptb->dbuf.t_gname[0] = xname;
	}
	if ((info->f_xflags & XF_GNAME) == 0) {
		if (*ptb->dbuf.t_gname) {
			info->f_gname = ptb->dbuf.t_gname;
			info->f_gmaxlen = STGNMLEN;
		}
	}
	if (info->f_gname) {
		char	xname;

		xname = ptb->dbuf.t_prefix[0];
		ptb->dbuf.t_prefix[0] = '\0';
		if (!numeric && ic_gidname(info->f_gname, info->f_gmaxlen, &gid)) {
			info->f_flags &= ~F_BAD_GID;
			info->f_gid = gid;
		}
		ptb->dbuf.t_prefix[0] = xname;
	}

	if (is_sparse(info) || is_multivol(info)) {
		if ((info->f_xflags & XF_REALSIZE) == 0) {
			stolli(ptb->xstar_in_dbuf.t_realsize, &ull);
			info->f_size = ull;
		}
	}
	if (is_multivol(info)) {
		if ((info->f_xflags & XF_OFFSET) == 0) {
			stolli(ptb->xstar_in_dbuf.t_offset, &ull);
			info->f_contoffset = ull;
		}
	}
}

/*
 * Used to convert from USTAR, PAX, SunTAR format header.
 */
LOCAL void
ustar_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	uid_t	uid;
	gid_t	gid;
	dev_t	d;
	Ulong	ul;
	char	c;

	info->f_xftype = USTOXT(ptb->ustar_dbuf.t_typeflag);
	info->f_filetype = XTTOST(info->f_xftype);
	info->f_type = XTTOIF(info->f_xftype);

	if ((info->f_xflags & XF_UNAME) == 0) {
		if (*ptb->ustar_dbuf.t_uname) {
			info->f_uname = ptb->ustar_dbuf.t_uname;
			info->f_umaxlen = TUNMLEN;
		}
	}
	if (info->f_uname) {
		char	xname;

		xname = ptb->ustar_dbuf.t_gname[0];
		ptb->ustar_dbuf.t_gname[0] = '\0';
		if (!numeric && ic_uidname(info->f_uname, info->f_umaxlen, &uid)) {
			info->f_flags &= ~F_BAD_UID;
			info->f_uid = uid;
		}
		ptb->ustar_dbuf.t_gname[0] = xname;
	}
	if ((info->f_xflags & XF_GNAME) == 0) {
		if (*ptb->ustar_dbuf.t_gname) {
			info->f_gname = ptb->ustar_dbuf.t_gname;
			info->f_gmaxlen = TGNMLEN;
		}
	}
	if (info->f_gname) {
		char	xname;

		xname = ptb->ustar_dbuf.t_devmajor[0];
		ptb->ustar_dbuf.t_devmajor[0] = '\0';
		if (!numeric && ic_gidname(info->f_gname, info->f_gmaxlen, &gid)) {
			info->f_flags &= ~F_BAD_GID;
			info->f_gid = gid;
		}
		ptb->ustar_dbuf.t_devmajor[0] = xname;
	}

	if ((info->f_xflags & XF_DEVMAJOR) == 0) {
		if (ptb->ustar_dbuf.t_devmajor[0] & 0x80)
			stob(ptb->ustar_dbuf.t_devmajor, &ul, 7);
		else
			stoli(ptb->ustar_dbuf.t_devmajor, &ul, 7);
		info->f_rdevmaj = ul;
		d = makedev(info->f_rdevmaj, 0);
		d = major(d);
		if ((d != ul) && is_dev(info)) {
			print_hrange("rdevmajor", (Ullong)ul);
			info->f_flags |= F_BAD_META;
		}
	}

	if ((info->f_xflags & XF_DEVMINOR) == 0) {
		if (ptb->ustar_dbuf.t_devminor[0] & 0x80) {
			stob(ptb->ustar_dbuf.t_devminor, &ul, 7);
		} else {
			/*
			 * The 'tar' that comes with HP-UX writes illegal tar
			 * archives. It includes 8 characters in the minor
			 * field and allows to archive 24 bits for the minor
			 * device which are used by HP-UX. As we like to be
			 * able to read these archives, we need to convert
			 * the number carefully by temporarily writing a NULL
			 * to the next character and restoring the right
			 * content afterwards.
			 */
			c = ptb->ustar_dbuf.t_prefix[0];
			ptb->ustar_dbuf.t_prefix[0] = '\0';
			stoli(ptb->ustar_dbuf.t_devminor, &ul, 8);
			ptb->ustar_dbuf.t_prefix[0] = c;
		}
		info->f_rdevmin = ul;
		d = makedev(0, info->f_rdevmin);
		d = minor(d);
		if ((d != ul) && is_dev(info)) {
			print_hrange("rdevminor", (Ullong)ul);
			info->f_flags |= F_BAD_META;
		}
	}

	info->f_rdev = makedev(info->f_rdevmaj, info->f_rdevmin);

	/*
	 * ANSI Tar hat keine atime & ctime im Header!
	 */
	if ((info->f_xflags & XF_ATIME) == 0) {
		info->f_atime = info->f_mtime;
		info->f_ansec = 0L;
	}
	if ((info->f_xflags & XF_CTIME) == 0) {
		info->f_ctime = info->f_mtime;
		info->f_cnsec = 0L;
	}
}

/*
 * Used to convert from XSTAR, XUSTAR, EXUSTAR format header.
 */
LOCAL void
xstar_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	Ulong	ul;
	Ullong	ull;

	ustar_to_info(ptb, info);

	if ((info->f_xflags & XF_ATIME) == 0) {
		if (ptb->xstar_dbuf.t_atime[0] & 0x80)
			stob(ptb->xstar_dbuf.t_atime, &ul, 11);
		else
			stoli(ptb->xstar_dbuf.t_atime, &ul, 11);
		info->f_atime = (time_t)ul;
		info->f_ansec = 0L;
	}
	if ((info->f_xflags & XF_CTIME) == 0) {
		if (ptb->xstar_dbuf.t_ctime[0] & 0x80)
			stob(ptb->xstar_dbuf.t_ctime, &ul, 11);
		else
			stoli(ptb->xstar_dbuf.t_ctime, &ul, 11);
		info->f_ctime = (time_t)ul;
		info->f_cnsec = 0L;
	}

	if (is_sparse(info) || is_multivol(info)) {
		if ((info->f_xflags & XF_REALSIZE) == 0) {
			stolli(ptb->xstar_in_dbuf.t_realsize, &ull);
			info->f_size = ull;
		}
	}
	if (is_multivol(info)) {
		if ((info->f_xflags & XF_OFFSET) == 0) {
			stolli(ptb->xstar_in_dbuf.t_offset, &ull);
			info->f_contoffset = ull;
		}
	}
}

/*
 * Used to convert from GNU tar format header.
 */
LOCAL void
gnutar_to_info(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	Ulong	ul;
	Ullong	ull;

	ustar_to_info(ptb, info);

	if ((info->f_xflags & XF_ATIME) == 0) {
		if (ptb->gnu_dbuf.t_atime[0] & 0x80)
			stob(ptb->gnu_dbuf.t_atime, &ul, 11);
		else
			stoli(ptb->gnu_dbuf.t_atime, &ul, 11);
		info->f_atime = (time_t)ul;
		info->f_ansec = 0L;
		if (info->f_atime == 0 && ptb->gnu_dbuf.t_atime[0] == '\0')
			info->f_atime = info->f_mtime;
		else
			info->f_xflags |= XF_ATIME;
	}

	if ((info->f_xflags & XF_CTIME) == 0) {
		if (ptb->gnu_dbuf.t_ctime[0] & 0x80)
			stob(ptb->gnu_dbuf.t_ctime, &ul, 11);
		else
			stoli(ptb->gnu_dbuf.t_ctime, &ul, 11);
		info->f_ctime = (time_t)ul;
		info->f_cnsec = 0L;
		if (info->f_ctime == 0 && ptb->gnu_dbuf.t_ctime[0] == '\0')
			info->f_ctime = info->f_mtime;
		else
			info->f_xflags |= XF_CTIME;
	}

	if (is_sparse(info)) {
		stolli(ptb->gnu_in_dbuf.t_realsize, &ull);
		info->f_size = ull;
	}
	if (is_multivol(info)) {
		stolli(ptb->gnu_dbuf.t_offset, &ull);
		info->f_contoffset = ull;
	}
}

LOCAL int
ustoxt(ustype)
	char	ustype;
{
	/*
	 * Map ANSI types
	 */
	if (ustype >= REGTYPE && ustype <= CONTTYPE)
		return (_USTOXT(ustype));

	/*
	 * Map Vendor Unique (Gnu tar & Star) types ANSI: "local enhancements"
	 */
	if ((props.pr_flags & (PR_LOCAL_STAR|PR_LOCAL_GNU)) &&
					ustype >= 'A' && ustype <= 'Z')
		return (_VTTOXT(ustype));

	/*
	 * treat unknown types as regular files conforming to standard
	 */
	return (XT_FILE);
}

LOCAL BOOL
checkeof(ptb)
	TCB	*ptb;
{
extern	m_stats	*stats;
extern	long	bigcnt;
extern	char	*bigbase;
	long	lastsize = 0;

	if (!eofblock(ptb))
		return (FALSE);
	if (debug)
		errmsgno(EX_BAD, "First  EOF Block at %lld OK\n", tblocks());

	stats->eofblock = tblocks();
	markeof();
	if (bigcnt == 0 && (rflag || uflag)) {
		/*
		 * If bigcnt == 0, the next readblock() call reads a new
		 * tape record and overwrites the current bigbuf data.
		 * Save the current read data in the buffer save area.
		 */
		movebytes(bigbuf, bigbase, bigsize);
		lastsize = stats->lastsize;
	}

	if (readblock((char *)ptb, TBLOCK) == EOF) {
		errmsgno(EX_BAD,
		"Incorrect EOF, second EOF block is missing at %lld.\n",
		tblocks());
		goto goteof;
	}
	if (!eofblock(ptb)) {
		if (!nowarn)
			errmsgno(EX_BAD,
			"WARNING: Partial (single block) EOF detected at %lld.\n",
			tblocks());
		return (FALSE);
	}
	if (debug)
		errmsgno(EX_BAD, "Second EOF Block OK at %lld\n", tblocks());

	stats->eofblock = tblocks();
goteof:
	if (lastsize) {
		/*
		 * Restore the old buffer content and back position the archive
		 * by one more tape block.
		 */
		movebytes(bigbase, bigbuf, bigsize);
		stats->lastsize = lastsize;
		backtape();
	}
	return (TRUE);
}

LOCAL BOOL
eofblock(ptb)
	TCB	*ptb;
{
	register short	i;
	register char	*s = (char *)ptb;

	if (props.pr_nflags & PR_DUMB_EOF)
		return (ptb->dbuf.t_name[0] == '\0');

	for (i = 0; i < TBLOCK; i++)
		if (*s++ != '\0')
			return (FALSE);
	return (TRUE);
}

/*
 * Convert string -> long int
 */
LOCAL void
stoli(s, l, fieldw)
	register char	*s;
		Ulong	*l;
		int	fieldw;
{
	register Ulong	ret = 0L;
	register char	c;
	register int	t;
	register char	*ep = s + fieldw;

#ifdef	__never__
	/*
	 * We do not like this to be used for the t_chksum field.
	 */
	if (*((Uchar*)s) & 0x80) {
		stob(s, l, 7);
		return;
	}
#endif

	while (*s == ' ') {
		if (s++ >= ep)
			break;
	}

	for (; s <= ep; ) {
		c = *s++;
		if (isoctal(c)) {
			t = c - '0';
		} else {
			--s;
			break;
		}
		ret *= 8;
		ret += t;
	}
	*l = ret;
	if (s > ep) {
		errmsgno(EX_BAD,
			"WARNING: Unterminated octal number at %lld.\n",
			tblocks());
	}
}

/*
 * Convert string -> long long int
 */
EXPORT void
stolli(s, ull)
	register char	*s;
		Ullong	*ull;
{
	register Ullong	ret = (Ullong)0;
	register char	c;
	register int	t;
	register char	*ep = s + 11;

	if (*((Uchar*)s) & 0x80) {
		stollb(s, ull, 11);
#ifdef	B256_DEBUG
		fprintf(stderr, "VALL %lld\n", *ull);
#endif
		return;
	}

	while (*s == ' ') {
		if (s++ >= ep)
			break;
	}

	for (; s <= ep; ) {
		c = *s++;
		if (isoctal(c)) {
			t = c - '0';
		} else {
			--s;
			break;
		}
		ret *= 8;
		ret += t;
	}
	*ull = ret;
	if (s > ep) {
		errmsgno(EX_BAD,
			"WARNING: Unterminated octal number at %lld.\n",
			tblocks());
	}
}

/*
 * Convert long int -> string.
 */
LOCAL void
litos(s, l, fieldw)
		char	*s;
	register Ulong	l;
	register int	fieldw;
{
	register char	*p	= &s[fieldw+1];
	register char	fill	= props.pr_fillc;

	/*
	 * Bei 12 Byte Feldern würde hier das Nächste Feld überschrieben, wenn
	 * entgegen der normalen Reihenfolge geschrieben wird!
	 * Da der TCB sowieso vorher genullt wird ist es aber kein Problem
	 * das bei 8 Bytes Feldern notwendige Nullbyte wegzulassen.
	 */
#ifdef	__needed__
	*p = '\0';
#endif
	/*
	 * Das Zeichen nach einer Zahl.
	 * XXX Soll hier besser ein NULL Byte bei POSIX Tar hin?
	 * XXX Wuerde das Probleme mit einer automatischen Erkennung geben?
	 */
	*--p = ' ';

	do {
		*--p = (l%8) + '0';	/* Compiler optimiert */

	} while (--fieldw > 0 && (l /= 8) > 0);

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
	case 0:;
	}
}

/*
 * Convert long long int -> string.
 */
EXPORT void
llitos(s, ull, fieldw)
		char	*s;
	register Ullong	ull;
	register int	fieldw;
{
	register char	*p	= &s[fieldw+1];
	register char	fill	= props.pr_fillc;

	/*
	 * Currently only used with fieldwidth == 11.
	 * XXX Large 8 byte fields are handled separately.
	 */
	if (/* fieldw == 11 && */ ull > MAXOCTAL11) {
		llbtos(s, ull, fieldw);
		return;
	}

	/*
	 * Bei 12 Byte Feldern würde hier das Nächste Feld überschrieben, wenn
	 * entgegen der normalen Reihenfolge geschrieben wird!
	 * Da der TCB sowieso vorher genullt wird ist es aber kein Problem
	 * das bei 8 Bytes Feldern notwendige Nullbyte wegzulassen.
	 */
#ifdef	__needed__
	*p = '\0';
#endif
	/*
	 * Das Zeichen nach einer Zahl.
	 * XXX Soll hier besser ein NULL Byte bei POSIX Tar hin?
	 * XXX Wuerde das Probleme mit einer automatischen Erkennung geben?
	 */
	*--p = ' ';

	do {
		*--p = (ull%8) + '0';	/* Compiler optimiert */

	} while (--fieldw > 0 && (ull /= 8) > 0);

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
	case 0:;
	}
}

/*
 * Convert binary (base 256) string -> long int.
 */
LOCAL void
stob(s, l, fieldw)
	register char	*s;
		Ulong	*l;
	register int	fieldw;
{
	register Ulong	ret = 0L;
	register Uchar	c;

	c = *s++ & 0x7F;
	ret = c * 256;

	while (--fieldw >= 0) {
		c = *s++;
		ret *= 256;
		ret += c;
	}
	*l = ret;
}

/*
 * Convert binary (base 256) string -> long long int.
 */
LOCAL void
stollb(s, ull, fieldw)
	register char	*s;
		Ullong	*ull;
	register int	fieldw;
{
	register Ullong	ret = 0L;
	register Uchar	c;

	c = *s++ & 0x7F;
	ret = c * 256;

	while (--fieldw >= 0) {
		c = *s++;
		ret *= 256;
		ret += c;
	}
	*ull = ret;
}

/*
 * Convert long int -> binary (base 256) string.
 */
LOCAL void
btos(s, l, fieldw)
		char	*s;
	register Ulong	l;
	register int	fieldw;
{
	register char	*p	= &s[fieldw+1];

	do {
		*--p = l%256;	/* Compiler optimiert */

	} while (--fieldw > 0 && (l /= 256) > 0);

	s[0] |= 0x80;
}

/*
 * Convert long long int -> binary (base 256) string.
 */
LOCAL void
llbtos(s, ull, fieldw)
		char	*s;
	register Ullong	ull;
	register int	fieldw;
{
	register char	*p	= &s[fieldw+1];

	do {
		*--p = ull%256;	/* Compiler optimiert */

	} while (--fieldw > 0 && (ull /= 256) > 0);

	s[0] |= 0x80;
}

LOCAL BOOL
nameascii(name)
	register char	*name;
{
	register unsigned char	c;
	while ((c = (unsigned char)*name++) != '\0') {
		if (c > 127)
			return (FALSE);
	}
	return (TRUE);
}

LOCAL void
print_hrange(type, ull)
	char	*type;
	Ullong	ull;
{
	if (nowarn)
		return;
	errmsgno(EX_BAD,
		"WARNING: %s '%llu' in header exceeds local range at %lld.\n",
		type, ull, tblocks());
}

/* ------------------------------------------------------------------------- */
EXPORT void
dump_info(info)
	FINFO	*info;
{
	error("f_name:      '%s'\n", info->f_name);
	error("f_namelen:   %lu\n", info->f_namelen);
	error("f_lname:     '%s'\n", info->f_lname);
	error("f_lnamelen:  %lu\n", info->f_lnamelen);
	error("f_uname:     '%s'\n", info->f_uname);
	error("f_umaxlen:   %lu\n", info->f_umaxlen);
	error("f_gname:     '%s'\n", info->f_gname);
	error("f_gmaxlen:   %lu\n", info->f_gmaxlen);

	error("f_dir:       %p\n", info->f_dir);
	error("f_dirinos:   %p\n", info->f_dirinos);
	error("f_dirlen:    %lld\n", (Llong)info->f_dirlen);
	error("f_dirents:   %lld\n", (Llong)info->f_dirents);
	error("f_dev:       0x%llX\n", (Ullong)info->f_dev);
	error("f_ino:       %llu\n", (Ullong)info->f_ino);
	error("f_nlink:     %llu\n", (Ullong)info->f_nlink);
	error("f_mode:      0%llo\n", (Ullong)info->f_mode);
	error("f_uid:       %lld\n", (Llong)info->f_uid);
	error("f_gid:       %lld\n", (Llong)info->f_gid);

	error("f_size:      %lld\n", (Llong)info->f_size);
	error("f_rsize:     %lld\n", (Llong)info->f_rsize);
	error("f_contoffset:%lld\n", (Llong)info->f_contoffset);
	error("f_flags:     0x%lX\n", info->f_flags);
	error("f_xflags:    0x%lX\n", info->f_xflags);
	error("f_xftype:    %lu (%s)\n", info->f_xftype, XTTONAME(info->f_xftype));
	error("f_rxftype:   %lu (%s)\n", info->f_rxftype, XTTONAME(info->f_rxftype));
	error("f_filetype:  %lu\n", info->f_filetype);
	error("f_typeflag:  '%c'\n", info->f_typeflag);
	error("f_type:      0%llo\n", (Ullong)info->f_type);
	error("f_rdev:      %lu\n", info->f_rdev);
	error("f_rdevmaj:   %lu\n", info->f_rdevmaj);
	error("f_rdevmin:   %lu\n", info->f_rdevmin);
	error("f_atime:     %.24s +0.%9.9ld s\n", ctime(&info->f_atime), info->f_ansec);
	error("f_mtime:     %.24s +0.%9.9ld s\n", ctime(&info->f_mtime), info->f_mnsec);
	error("f_ctime:     %.24s +0.%9.9ld s\n", ctime(&info->f_ctime), info->f_cnsec);
	error("f_fflags:    0x%lX\n", info->f_fflags);
}
