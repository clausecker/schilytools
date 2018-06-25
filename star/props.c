/* @(#)props.c	1.61 18/06/16 Copyright 1994-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)props.c	1.61 18/06/16 Copyright 1994-2018 J. Schilling";
#endif
/*
 *	Set up properties for different archive types
 *
 *	NOTE that part of the information in struct propertiesis available more
 *	than once. This is needed as different parts of the source need the
 *	information in different ways. Partly for performance reasons, partly
 *	because one method of storing the information is inappropriate for all
 *	places in the source.
 *
 *	If you add new features or information related to the fields
 *	pr_flags/pr_nflags or the fields pr_xftypetab[]/pr_typeflagtab[]
 *	take care of possible problems due to this fact.
 *
 *	Copyright (c) 1994-2018 J. Schilling
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
#include "star.h"
#include "props.h"
#include "table.h"
#include "diff.h"
#include <schily/standard.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include "starsubs.h"

extern	BOOL	debug;
extern	BOOL	dodump;

struct properties props;

EXPORT	void	setprops	__PR((long htype));
EXPORT	void	printprops	__PR((void));
LOCAL	void	prsettypeflags	__PR((char *types, int typeflag));

EXPORT void
setprops(htype)
	long	htype;
{
	fillbytes(props.pr_typeflagtab, sizeof (props.pr_typeflagtab), '\0');

	switch (H_TYPE(htype)) {

	case H_STAR:				/* Old star format (1985)    */
		props.pr_maxsize = 0;
		props.pr_hdrsize = TAR_HDRSZ;
		props.pr_flags = PR_LOCAL_STAR|PR_SPARSE|PR_VOLHDR|PR_BASE256;
		props.pr_xhdflags = 0;
		props.pr_xhmask = 0;
		props.pr_fillc = ' ';		/* Use old tar octal format  */
		props.pr_xc    = 'x';		/* Use POSIX.1-2001 x-hdr    */
		props.pr_pad   = 0;
		props.pr_diffmask = 0L;
		props.pr_nflags =
			PR_POSIX_SPLIT|PR_PREFIX_REUSED|PR_LONG_NAMES;
		props.pr_maxnamelen =  PATH_MAX;
		props.pr_maxlnamelen = PATH_MAX;
		props.pr_maxsname =    NAMSIZ;
		props.pr_maxslname =   NAMSIZ;
		props.pr_maxprefix =   PFXSIZ;
		props.pr_sparse_in_hdr = 0;
		/*
		 * The STAR format extensions from 1986 archive nearly any UNIX file type
		 */
		movebytes(xtstar_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));

		/*
		 * The STAR format is pre-POSIX but supports many POSIX/GNU
		 * extensions.
		 */
		prsettypeflags("01234567gxIKLMSV", TF_VALIDTYPE);
		prsettypeflags("gxKL",		   TF_XHEADERS);
		break;

	case H_XSTAR:				/* ext P-1988 format (1994)  */
	case H_XUSTAR:				/* ext ^ no "tar" sig (1998) */
	case H_EXUSTAR:				/* ext P-2001 format (2001)  */
		props.pr_maxsize = 0;
		props.pr_hdrsize = TAR_HDRSZ;
		props.pr_flags =
			PR_POSIX_OCTAL|PR_LOCAL_STAR|PR_SPARSE|
			PR_MULTIVOL|PR_VOLHDR|PR_BASE256;
		props.pr_xhdflags = 0;
		props.pr_xhmask = 0;
		if (H_TYPE(htype) == H_XUSTAR || H_TYPE(htype) == H_EXUSTAR) {
			props.pr_flags |= (PR_XHDR|PR_VU_XHDR);
			props.pr_xhmask = ~0;	/* xustar/exustar supp. all */
		}
		if (H_TYPE(htype) == H_EXUSTAR) {
			props.pr_flags |= PR_LINK_DATA;
			props.pr_xhdflags |= (XF_ATIME|XF_CTIME|XF_MTIME);
		}
		props.pr_fillc = '0';		/* Use ustar octal format    */
		props.pr_xc    = 'x';		/* Use POSIX.1-2001 x-hdr    */
		props.pr_pad   = 0;
		props.pr_diffmask = 0L;
		props.pr_nflags =
			PR_POSIX_SPLIT|PR_PREFIX_REUSED|PR_LONG_NAMES;
		props.pr_maxnamelen =  PATH_MAX;
		props.pr_maxlnamelen = PATH_MAX;
		props.pr_maxsname =    NAMSIZ;
		props.pr_maxslname =   NAMSIZ;
		props.pr_maxprefix =   130;
		props.pr_sparse_in_hdr = 0;
		/*
		 * As long as we don't support the POSIX.1-2001 speudo 'x' file
		 * We can only archive file types from the USTAR list and
		 * sparse files as well as
		 * meta files (regular files without archived content).
		 */
		movebytes(xtustar_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));
		props.pr_xftypetab[XT_SPARSE] = 1;
		props.pr_xftypetab[XT_META] = 1;

		/*
		 * Extended PAX format may allow more filetypes in a vendor
		 * unique extension of the POSIX.1-2001 extended headers.
		 * XXX As we currently only add the SCHILY.filetype headers
		 * XXX if -dump has been specified, we can only allow more
		 * XXX filetypes in this case.
		 */
		if (dodump && H_TYPE(htype) == H_EXUSTAR)
			movebytes(xtexustar_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));

		/*
		 * The X-STAR format family is POSIX with extensions.
		 */
		prsettypeflags("01234567gxIKLMSV", TF_VALIDTYPE);
		prsettypeflags("gxKL",		   TF_XHEADERS);
		break;

	case H_PAX:				/* ieee 1003.1-2001 ext ustar */
	case H_EPAX:				/* ieee 1003.1-2001 ext ustar + xheader */
	case H_USTAR:				/* ieee 1003.1-1988 ustar    */
	case H_SUNTAR:				/* Sun's tar from Solaris 7-9 */
		props.pr_maxsize = MAXOCTAL11;
		props.pr_hdrsize = TAR_HDRSZ;
		props.pr_flags = PR_POSIX_OCTAL;
		if (H_TYPE(htype) == H_PAX ||
		    H_TYPE(htype) == H_EPAX ||
		    H_TYPE(htype) == H_SUNTAR) {
			props.pr_maxsize = 0;
			props.pr_flags |= PR_XHDR;
		}
		props.pr_xhdflags = 0;
		props.pr_xhmask = 0;
		if (H_TYPE(htype) == H_PAX ||
		    H_TYPE(htype) == H_EPAX)
			props.pr_xhmask = XF_POSIX;
		if (H_TYPE(htype) == H_EPAX) {
			props.pr_xhdflags |= (XF_ATIME|XF_CTIME|XF_MTIME);
		}
		props.pr_fillc = '0';		/* Use ustar octal format    */
		props.pr_xc    = 'x';		/* Use POSIX.1-2001 x-hdr    */
		props.pr_pad   = 0;
		if (H_TYPE(htype) == H_SUNTAR) {
			props.pr_flags |= PR_VU_XHDR;
			props.pr_xhdflags = XF_MTIME;
			props.pr_xhmask =   XF_MTIME|XF_UID|XF_UNAME|
					    XF_GID|XF_GNAME|
					    XF_PATH|XF_LINKPATH|
					    XF_SIZE|XF_DEVMAJOR|XF_DEVMINOR;
			props.pr_xc    = 'X';	/* Use Sun Solaris  X-hdr    */
		}
		props.pr_diffmask = (D_SPARS|D_ATIME|D_CTIME);
		if (H_TYPE(htype) == H_PAX ||
		    H_TYPE(htype) == H_EPAX) {
			props.pr_diffmask = 0L;
		}
		props.pr_nflags = PR_POSIX_SPLIT;
		props.pr_maxnamelen =  NAMSIZ + 1 + PFXSIZ;	/* 256 */
		props.pr_maxlnamelen = NAMSIZ;
		if (H_TYPE(htype) == H_PAX ||
		    H_TYPE(htype) == H_EPAX ||
		    H_TYPE(htype) == H_SUNTAR) {
			props.pr_nflags |= PR_LONG_NAMES;
			props.pr_maxnamelen =  PATH_MAX;
			props.pr_maxlnamelen = PATH_MAX;
		}
		props.pr_maxsname =    NAMSIZ;
		props.pr_maxslname =   NAMSIZ;
		props.pr_maxprefix =   PFXSIZ;
		props.pr_sparse_in_hdr = 0;
		/*
		 * USTAR is limited to 7 basic file types
		 */
		movebytes(xtustar_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));

		/*
		 * Even the USTAR format should support POSIX extension headers.
		 */
		prsettypeflags("01234567gx",	TF_VALIDTYPE);
		prsettypeflags("gx",		TF_XHEADERS);
		if (H_TYPE(htype) == H_SUNTAR) {
			prsettypeflags("01234567gxX",	TF_VALIDTYPE);
			prsettypeflags("gxX",		TF_XHEADERS);
		}
		break;

	case H_GNUTAR:				/* gnu tar format (1989)    */
		props.pr_maxsize = 0;
		props.pr_hdrsize = TAR_HDRSZ;
		props.pr_flags =
			PR_LOCAL_GNU|PR_SPARSE|PR_GNU_SPARSE_BUG|PR_VOLHDR|PR_BASE256;
		props.pr_xhdflags = 0;
		props.pr_xhmask = 0;
		props.pr_fillc = ' ';		/* Use old tar octal format  */
		props.pr_xc    = 'x';		/* Really ??? */
		props.pr_pad   = 0;
		props.pr_diffmask = 0L;
		props.pr_nflags = PR_LONG_NAMES;
		props.pr_maxnamelen =  PATH_MAX;
		props.pr_maxlnamelen = PATH_MAX;
		props.pr_maxsname =    NAMSIZ-1;
		props.pr_maxslname =   NAMSIZ-1;
		props.pr_maxprefix =   0;
		props.pr_sparse_in_hdr = SPARSE_IN_HDR;
		/*
		 * GNU tar is limited to the USTAR file types + sparse files
		 */
		movebytes(xtustar_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));
		props.pr_xftypetab[XT_SPARSE] = 1;

		prsettypeflags("01234567DKLMNSV", TF_VALIDTYPE);
		prsettypeflags("KL",		  TF_XHEADERS);
		break;

	default:
		errmsgno(EX_BAD, "setprops: unexpected header type %ld '%s' (%s).\n",
						htype,
						hdr_name(htype),
						hdr_text(htype));
		errmsgno(EX_BAD, "setprops: defaulting to '%s' (%s).\n",
						hdr_name(H_OTAR),
						hdr_text(H_OTAR));

	case H_UNDEF:				/* from star main read mode  */
	case H_TAR:				/* tar with unknown format   */
	case H_V7TAR:				/* tar old UNIX V7 format    */
	case H_OTAR:				/* tar old BSD format (1978) */
		/*
		 * Since the large file summit was in 1995, we may assume
		 * that any large file aware TAR implementation must be POSIX
		 * compliant too. The only known exception is GNU tar which
		 * is recognised separately.
		 * Limit max file size to the max that is supported
		 * by non large file aware systems.
		 */
		props.pr_maxsize = MAXNONLARGEFILE;
		props.pr_hdrsize = TAR_HDRSZ;
		props.pr_flags = 0;
		props.pr_xhdflags = 0;
		props.pr_xhmask = 0;
		props.pr_fillc = ' ';		/* Use old tar octal format  */
		props.pr_xc    = 'x';		/* Really ??? */
		props.pr_pad   = 0;
		props.pr_diffmask = (D_SPARS|D_ATIME|D_CTIME);
		props.pr_nflags = PR_DUMB_EOF;
		props.pr_maxnamelen =  NAMSIZ-1;
		props.pr_maxlnamelen = NAMSIZ-1;
		props.pr_maxsname =    NAMSIZ-1;
		props.pr_maxslname =   NAMSIZ-1;
		props.pr_maxprefix =   0;
		props.pr_sparse_in_hdr = 0;
		/*
		 * Only a very limited set of file types (file,symlink,dir)
		 */
		if (H_TYPE(htype) == H_V7TAR)
			movebytes(xtv7tar_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));
		else
			movebytes(xttar_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));

		/*
		 * Old BSD tar does no know about file types beyond "012" but
		 * the USTAR basic format has been designed to be compatible
		 * with the old tar format.
		 * OLD UNIX V7 tar does not know anything but plain files and
		 * hard links. Note that we never see H_TYPE(htype) == H_V7TAR
		 * in extract mode.
		 */
		if (H_TYPE(htype) == H_V7TAR)
			prsettypeflags("01", TF_VALIDTYPE);
		else
			prsettypeflags("01234567", TF_VALIDTYPE);
		prsettypeflags("",	   TF_XHEADERS);
		break;

	case H_BAR:				/* Sun bar format */
		props.pr_maxsize = MAXNONLARGEFILE; /* bar is pre-largefile */
		props.pr_hdrsize = BAR_HDRSZ;
		props.pr_flags = 0;
		props.pr_xhdflags = 0;
		props.pr_xhmask = 0;
		props.pr_fillc = ' ';		/* Use old tar octal format  */
		props.pr_xc    = 'x';		/* Really ??? */
		props.pr_pad   = 0;
		props.pr_diffmask = (D_SPARS|D_ATIME|D_CTIME);
		props.pr_nflags = PR_DUMB_EOF;
		props.pr_maxnamelen =  PATH_MAX;
		props.pr_maxlnamelen = PATH_MAX;
		props.pr_maxsname =    NAMSIZ-1; /* XXX Really ??? */
		props.pr_maxslname =   NAMSIZ-1; /* XXX Really ??? */
		props.pr_maxprefix =   0;
		props.pr_sparse_in_hdr = 0;
		/*
		 * BAR is limited to 7 basic file types
		 */
		movebytes(xtcpio_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));
		props.pr_xftypetab[XT_SOCK] = 0;
		break;

	case H_CPIO_BIN:
		props.pr_maxsize = MAXNONLARGEFILE;
		props.pr_hdrsize = CPIOBIN_HDRSZ;
		props.pr_flags = PR_CPIO;
		props.pr_xhdflags = 0;
		props.pr_xhmask = 0;
		props.pr_fillc = '0';
		props.pr_xc    = 'x';		/* Not needed */
		props.pr_pad   = 1;
		props.pr_diffmask = (D_SPARS|D_ATIME|D_CTIME);
		props.pr_nflags = PR_LONG_NAMES;
		props.pr_maxnamelen =  256;
		props.pr_maxlnamelen = PATH_MAX;
		props.pr_maxsname =    NAMSIZ-1; /* XXX Really ??? */
		props.pr_maxslname =   NAMSIZ-1; /* XXX Really ??? */
		props.pr_maxprefix =   0;
		props.pr_sparse_in_hdr = 0;
		/*
		 * CPIO is limited to 7 basic file types
		 */
		movebytes(xtcpio_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));
		props.pr_xftypetab[XT_SOCK] = 0;
		break;

	case H_CPIO_CHR:
	case H_CPIO_ODC:
		props.pr_maxsize = MAXOCTAL11;
		props.pr_hdrsize = CPIOODC_HDRSZ;
		props.pr_flags = PR_CPIO;
		props.pr_xhdflags = 0;
		props.pr_xhmask = 0;
		props.pr_fillc = '0';
		props.pr_xc    = 'x';		/* Not needed */
		props.pr_pad   = 0;
		props.pr_diffmask = (D_SPARS|D_ATIME|D_CTIME);
		props.pr_nflags = PR_LONG_NAMES;
		props.pr_maxnamelen =  PATH_MAX;
		props.pr_maxlnamelen = PATH_MAX;
		props.pr_maxsname =    NAMSIZ-1; /* XXX Really ??? */
		props.pr_maxslname =   NAMSIZ-1; /* XXX Really ??? */
		props.pr_maxprefix =   0;
		props.pr_sparse_in_hdr = 0;
		/*
		 * CPIO is limited to 7 basic file types + sockets
		 */
		movebytes(xtcpio_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));

		if (H_TYPE(htype) == H_CPIO_ODC) {
			/*
			 * Be compatible to the useless SYSv cpio -Hodc
			 * limitations.
			 */
			props.pr_maxnamelen =  255;
			props.pr_xftypetab[XT_SOCK] = 0;
		}
		break;

	case H_CPIO_ASC:
	case H_CPIO_ACRC:
		props.pr_maxsize = (unsigned long)0xFFFFFFFF;
		props.pr_hdrsize = CPIOCRC_HDRSZ;
		props.pr_flags = PR_CPIO | PR_SV_CPIO_LINKS;
		props.pr_xhdflags = 0;
		props.pr_xhmask = 0;
		props.pr_fillc = '0';
		props.pr_xc    = 'x';		/* Not needed */
		props.pr_pad   = 3;
		props.pr_diffmask = (D_SPARS|D_ATIME|D_CTIME);
		props.pr_nflags = PR_LONG_NAMES;
		props.pr_maxnamelen =  PATH_MAX;
		props.pr_maxlnamelen = PATH_MAX;
		props.pr_maxsname =    NAMSIZ-1; /* XXX Really ??? */
		props.pr_maxslname =   NAMSIZ-1; /* XXX Really ??? */
		props.pr_maxprefix =   0;
		props.pr_sparse_in_hdr = 0;
		/*
		 * CPIO is limited to 7 basic file types
		 */
		movebytes(xtcpio_tab, props.pr_xftypetab, sizeof (props.pr_xftypetab));
		props.pr_xftypetab[XT_SOCK] = 0;
		break;
	}
	if (debug) printprops();
}

EXPORT void
printprops()
{
extern long hdrtype;
	error("hdrtype:          %s (%ld)\n", hdr_name(hdrtype), hdrtype);
	if (props.pr_maxsize)
		error("pr_maxsize:       %llu\n", (Ullong)props.pr_maxsize);
	else
		error("pr_maxsize:       unlimited\n");
	error("pr_flags:         0x%X\n", props.pr_flags);
	error("pr_xhdflags:      0x%X\n", props.pr_xhdflags);
	error("pr_xhmask:        0x%X\n", props.pr_xhmask);
	error("pr_fillc:         '%c'\n", props.pr_fillc);
	prdiffopts(stderr, "pr_diffmask:      ", props.pr_diffmask);
	error("pr_nflags:        0x%X\n", props.pr_nflags);
	error("pr_maxnamelen:    %d\n", props.pr_maxnamelen);
	error("pr_maxlnamelen:   %d\n", props.pr_maxlnamelen);
	error("pr_maxsname:      %d\n", props.pr_maxsname);
	error("pr_maxslname:     %d\n", props.pr_maxslname);
	error("pr_maxprefix:     %d\n", props.pr_maxprefix);
	error("pr_sparse_in_hdr: %d\n", props.pr_sparse_in_hdr);
}

LOCAL void
prsettypeflags(types, typeflag)
	char	*types;
	int	typeflag;
{
	register Uchar	*ucp = (Uchar *)types;

	/*
	 * 'Otar' flag for plain file is always a valid typeflag.
	 */
	if (typeflag == TF_VALIDTYPE)
		props.pr_typeflagtab[0] = typeflag;

	while (*ucp != '\0')
		props.pr_typeflagtab[*ucp++] |= typeflag;
}
