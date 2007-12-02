/* @(#)sun_dkio.h	1.3 07/05/24 Copyright 1997-2007 J. Schilling */
/*
 *	Hack to compile sformat on Linux (will go away soon)
 */

#ifndef	_SUN_DKIO_H
#define	_SUN_DKIO_H

#include <schily/ioctl.h>
#include "sun_dklabel.h"

#define	DK_DEVLEN	16		/* name max length */

/*
 * This structure is wrong, it is only used to compile sformat on Linux.
 */
struct dk_conf {
	char	dkc_cname[DK_DEVLEN];	/* controller name		*/
	short	dkc_cnum;		/* controller number		*/
	int	dkc_addr;		/* controller address		*/
	short	dkc_slave;		/* slave number			*/
};

struct dk_allmap {
	struct dk_map	dka_map[NDKMAP];
};

struct dk_geom {
	unsigned short	dkg_ncyl;	/* # of data cylinders		*/
	unsigned short	dkg_acyl;	/* # of alternate cylinders	*/
	unsigned short	dkg_bcyl;	/* cyl offset (for fixed head area) */
	unsigned short	dkg_nhead;	/* # of heads			*/
	unsigned short	dkg_bhead;	/* label head offset		*/
	unsigned short	dkg_nsect;	/* # of data sectors per track	*/
	unsigned short	dkg_intrlv;	/* interleaving factor		*/
	unsigned short	dkg_gap1;	/* gap1				*/
	unsigned short	dkg_gap2;	/* gap2				*/
	unsigned short	dkg_apc;	/* alternates per cyl		*/
	unsigned short	dkg_rpm;	/* disk rotation rate		*/
	unsigned short	dkg_pcyl;	/* # physical cylinders		*/
};

#endif	/* _SUN_DKIO_H */
