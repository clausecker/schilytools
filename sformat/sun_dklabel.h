/*
 *	Sun conforming disk label definition
 *
 *	Copyright (c) 1997 J. Schilling
 */

#ifndef	_SUN_DKLABEL_H
#define	_SUN_DKLABEL_H

struct dk_map {
	daddr_t	dkl_cylno;		/* starting cylinder		*/
	daddr_t	dkl_nblk;		/* number of blocks		*/
};

#define	DKL_SIZE	512		/* the size of a sun disk label	*/
#define	DKL_ASCII	128		/* the length of the ascii name	*/
#define	DKL_PAD		DKL_SIZE - \
			(DKL_ASCII + \
			NDKMAP*sizeof(struct dk_map) + \
			14*sizeof(unsigned short))

#define	DKL_MAGIC	0xDABE		/* magic number			*/
#define	NDKMAP		8		/* # of logical partitions	*/

struct dk_label {
	char	dkl_asciilabel[128];	/* ascii label name		*/
	/*
	 * Here comes the new Solaris/SVr4 stuff.
	 */
#ifdef	__USED__
	struct dk_vtoc	dkl_vtoc;
	unsigned short	dkl_write_reinstruct; /* # sectors to skip, writes */
	unsigned short	dkl_read_reinstruct; /* # sectors to skip, reads */
#endif
	char	dkl_pad[DKL_PAD];
	unsigned short	dkl_rpm;	/* disk rotation rate		*/
	unsigned short	dkl_pcyl;	/* # physical cylinders		*/
	unsigned short	dkl_apc;	/* alternates per cylinder	*/
	unsigned short	dkl_gap1;	/* gap1				*/
	unsigned short	dkl_gap2;	/* gap2				*/
	unsigned short	dkl_intrlv;	/* interleaving factor		*/
	unsigned short	dkl_ncyl;	/* # of data cylinders		*/
	unsigned short	dkl_acyl;	/* # of alternate cylinders	*/
	unsigned short	dkl_nhead;	/* # of heads in this partition	*/
	unsigned short	dkl_nsect;	/* # of 512 byte sectors per track */
	unsigned short	dkl_bhead;	/* label head offset		*/
	unsigned short	dkl_ppart;	/* physical partition		*/
	struct dk_map	dkl_map[NDKMAP];/* logical partitions		*/
	unsigned short	dkl_magic;	/* magic number for this label	*/
	unsigned short	dkl_cksum;	/* xor checksum of label sector	*/
};

#endif	/* _SUN_DKLABEL_H */
