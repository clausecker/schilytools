/* @(#)map.h	1.5 00/07/23 Copyright 1995 J. Schilling */
/*
 *	Definitions for mapping logical disk names to scsibus/target/lun
 *
 *	Copyright (c) 1995 J. Schilling
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

typedef struct scg_driver {
	char	scg_cname[DK_DEVLEN];	/* Name des Hostadapters	*/
	int	scg_caddr;		/* Adresse des Hostadapters	*/
	short	scg_cunit;		/* Unit # des scg Treibers	*/
	short	scg_slave;		/* Initiator id (target*8) / -1	*/
} scgdrv;

extern	int	maptodisk	__PR((int, int, int));
extern	scgdrv	*scg_getdrv	__PR((int));
extern	char	*diskname	__PR((int));
extern	char	*diskdevname	__PR((int));
extern	int	print_disknames	__PR((int, int, int));

#ifdef	SVR4
/*
 * Erster Versuch der Portierung:
 * Umsetzen der Config Struktur.
 */
#undef	DKIOCGCONF			/* Only for dummy in dsklabel.h */
#define	DKIOCGCONF	DKIOCINFO
#define	dk_conf		dk_cinfo

#define	dkc_ctype	dki_ctype
#define	dkc_flags	dki_flags
#define	dkc_cname	dki_cname
#define	dkc_cnum	dki_cnum
#define	dkc_addr	dki_addr
#define	dkc_space	dki_space
#define	dkc_prio	dki_prio
#define	dkc_vec		dki_vec
#define	dkc_dname	dki_dname
#define	dkc_unit	dki_unit
#define	dkc_slave	dki_slave

#endif
