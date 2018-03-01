/* @(#)fmt.h	1.34 18/02/19 Copyright 1991-2018 J. Schilling */
/*
 *	Definitions for the format utility
 *
 *	Copyright (c) 1991-2018 J. Schilling
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

#include <schily/ccomdefs.h>

#include <scg/scsitransp.h>		/* XXX fuer SCSI *	*/

struct disk {
	/*
	 * Diese Werte werden aus dem Label uebernommen
	 */
	long	lhead;		/* # der Köpfe im Label */
	long	lacyl;		/* # der alternativen Zylinder im Label */
	long	lpcyl;		/* # der Zylinder im Label (total) */
	long	lncyl;		/* # der Daten Zylinder im Label */
	long	lspt;		/* # der Sektoren/Spur im Label */
	long	lapc;		/* # der alternativen Sektoren/Zyl im Label */

	/*
	 * Werte, die mit Hilfe von Parametern/Dateioptionen
	 * geaendert werden koennen.
	 */
	long	nhead;		/* # der Köpfe */
	long	pcyl;		/* # der Zylinder (total) */
	long	atrk;		/* # der alternativen Spuren/Volume */
	long	tpz;		/* # der Spuren/Zone */
	long	spt;		/* # der Sektoren/Spur (total) */
	long	mintpz;		/* Minimal zulässige # der Spuren/Zone */
	long	maxtpz;		/* Maximal zulässige # der Spuren/Zone */
	long	aspz;		/* # der Alternativen Sektoren/Zone */
	long	secsize;	/* Größe eines Sektors in Bytes */
	long	phys_secsize;	/* Größe eines phys. Sektors in Bytes */
	long	capacity;	/* Kapazität aus sformat.dat */
	long	min_capacity;	/* Minimale Kapazität aus sformat.dat */
	long	rpm;		/* Umdrehungen pro Minute */
	long	track_skew;	/* Spiraloffset bei Kopfumschaltung */
	long	cyl_skew;	/* Spiraloffset bei Zylinderumschaltung */
	long	reduced_curr;	/* Zylinder# f. Red. Schreibstrom (acb-4000)*/
	long	write_precomp;	/* Zylinder# f. Schreibkompens.	  (acb-4000)*/
	long	step_rate;	/* Step Rate f. Buffered Seek	  (acb-4000)*/
	long	rot_pos_locking; /* Typ f. Spindelsync */
	long	rotational_off;	/* Spindelsync phase (x/256) */
	long	interleave;	/* Interleaving Faktor */
	long	fmt_pattern;	/* Format Pattern */
	long	fmt_mode;	/* Format mode (Sony SMO-C501) */
	long	spare_band_size; /* Spare Band Size (Sony SMO-C501) */

	long	def_lst_format;	/* Defect List Format */
	long	split_wv_cmd;	/* split write-verify cmd = TRUE else FALSE */

	/* Hier bedeutet -1 == nicht supported */
	long	rd_retry_count;	/* Read retry count */
	long	wr_retry_count;	/* Write retry count */
	long	recov_timelim;	/* Recovery time limit in msec Einheiten */

	/* Hier bedeutet -1 == nicht supported */
	long	buf_full_ratio;	/* Buffer full ratio */
	long	buf_empt_ratio;	/* Buffer empty ratio */
	long	bus_inact_limit; /* Bus inactivity limit 100 usec Einheiten */
	long	disc_time_limit; /* Disconnect time limit 100 usec Einheiten */
	long	conn_time_limit; /* Connect time limit 100 usec Einheiten */
	long	max_burst_size;	/* Maximale Burst Größe 512 Bytes Einheiten */
	long	data_tr_dis_ctl; /* Data transfer disconnect control */

	/* Hier bedeutet -1 == nicht supported */
	long	queue_alg_mod;	/* Queuing alg modifier */
	long	dis_queuing;	/* Queuing enable */

	/*
	 * Modifizierbare Variablen mit interner Bedeutung
	 */
	long	gap1;		/* Wird nicht benötigt */
	long	gap2;		/* Wird nicht benötigt */
	long	int_cyl;	/* Intern vom Kontroller verwaltete Zylinder*/
	long	fmt_time;	/* Foratierzeit in Sekunden (1 Durchlauf) */
	long	fmt_timeout;	/* Foratier timeout in Sekunden für Treiber */
	long	veri_time;	/* Verifizierzeit in Sekunden (1 Durchlauf) */
	long	veri_count;	/* Sectorcount pro VERIFY Kommando */
	long	wr_veri_count;	/* Sectorcount pro WRITE_VERIFY Kommando */
	long	veri_loops;	/* Anzahl der Verify Durchgänge */

	long	dummy;		/* Für ignorierte Variablen in sformat.dat */

	/*
	 * Hier folgen interne Statusvariablen
	 */
	long	flags;		/* Diverse Statusflags */
	long	formatted;	/* Platte ist formatiert und hat Label */
				/* < 0 unformatiert */
				/* = 0 formatiert */
				/* > 0 formatiert  mit Label */
	long	labelread;	/* Label wurde gelesen und Daten übernommen */
				/* < 0 kein Label */
				/* = 0 Label aus Datenbank */
				/* > 0 Label von Platte gelesen */
	long	cur_capacity;	/* Aktuelle Kapazitaet fuer Test vor/nach fmt*/
	long	default_data;	/* Dieser Datensatz ist Default */
	long	bridge_controller; /* Dieser Datensatz ist fuer einen */
				/* Bridge Kontroller z.B. SCSI/EDSI */
	char	*disk_type;	/* Name der Platte in der Parameter Datei */
	char	*alt_disk_type;	/* Alternativer Name in der Parameter Datei */
	char	*default_part;	/* Name der default Patition für die Platte */
	char	*mode_pages;	/* Name des Modepage Eintrages in der Datei */
	struct node *parts;	/* Liste der möglichen Label für die Platte */
};

struct node {
	struct node	*n_next;
	char		*n_data;
};

#define	D_INQ_FOUND	0x0001	/* SCSI Inquiry (Vendor & Product) found */
#define	D_FIRMW_FOUND	0x0002	/* SCSI Inquiry (Firmware) found */
#define	D_FTIME_FOUND	0x0004	/* Disk format time found */
#define	D_VTIME_FOUND	0x0008	/* Disk verify time found */
#define	D_DISK_FOUND	0x0010	/* Inquiry & Physical layout matches */
#define	D_DISK_CURRENT	0x0100	/* This is a "CURRENT" disk */
#define	D_DISK_LPCYL	0x0200	/* A cheated version of lpcyl has been set up */
#define	D_DB_BAD	0x1000	/* Data base entry found past signature */

#define	D_OK_MASK	(D_INQ_FOUND|D_FIRMW_FOUND|D_DISK_FOUND|D_DISK_CURRENT)
#define	D_FOUND_MASK	(D_DISK_FOUND|D_DISK_CURRENT)

#define	MIN_SECSIZE	256
#define	MAX_SECSIZE	8192

#define	NVERI	5
extern	int	n_test_patterns;
#define	NWVERI	n_test_patterns
#define	CVERI		1000
#define	CWVERI		1000

#ifdef	SVR4
#define	PARTOFF		'0'
#else
#define	PARTOFF		'a'
#endif
#define	PART(dev)	(minor(dev) & 0x07)

struct strval {
	int	s_val;
	char	*s_name;
	char	*s_text;
};

/*--------------------------------------------------------------------------*/
#ifndef	NO_PROT
/*
 * Prototypes:
 */

/*
 * acb4000.c:
 */
extern	int	acbdev __PR((SCSI *scgp));
extern	BOOL	get_acb4000defaults __PR((SCSI *scgp, struct disk *));
extern	BOOL	set_acb4000params __PR((SCSI *scgp, struct disk *));

/*
 * autopart.c:
 */
#ifdef	NDKMAP
extern	BOOL	get_part_defaults __PR((SCSI *scgp, struct disk *, struct dk_label *));
#endif

/*
 * badblock.c:
 */
extern	void	clear_bad		__PR((void));
extern	int	get_nbad		__PR((void));
extern	void	insert_bad		__PR((long baddr));
extern	int	print_bad		__PR((void));
extern	int	bad_to_def		__PR((SCSI *scgp));
extern	void	reassign_bad		__PR((SCSI *scgp));
extern	void	repair_found_blocks	__PR((SCSI *scgp, int nbad));
extern	void	reassign_one		__PR((SCSI *scgp));
extern	int	reassign_one_block	__PR((SCSI *scgp, long n));
#ifdef	_SCG_SCSIREG_H
extern	void	print_defect_list	__PR((struct scsi_def_list *l));
extern	void	print_def_bfi		__PR((struct scsi_def_list *l));
#endif

/*
 * bcrypt.c:
 */
extern	char		*getnenv	__PR((const char *, int));
extern	unsigned long	my_gethostid	__PR((void));
extern	BOOL		bsecurity	__PR((int));
extern	unsigned long	bcrypt		__PR((unsigned long));
extern	char		*bmap		__PR((unsigned long));
extern	unsigned long	bunmap		__PR((const char *));

/*
 * check_part.c:
 */
#ifdef	NDKMAP
extern	void	checklabel		__PR((struct disk *, struct dk_label *, int));
#endif

/*
 * checkmount.c:
 */
extern	BOOL	checkmount		__PR((int, int, int, long, long));

/*
 * datio.c:
 */
extern	BOOL	opendatfile	__PR((char *));
extern	char	*datfilename	__PR((void));
extern	BOOL	past_df_sig	__PR((void));
extern	BOOL	closedatfile	__PR((void));
extern	int	rewinddatfile	__PR((void));
extern	BOOL	datfile_chksum	__PR((void));
extern	char	*nextline	__PR((void));
extern	BOOL	firstitem	__PR((void));
extern	int	getlineno	__PR((void));
extern	char	*curword	__PR((void));
extern	char	*peekword	__PR((void));
extern	char	*nextword	__PR((void));
extern	char	*nextitem	__PR((void));
extern	char	*scanforline	__PR((char *, char *));
extern	BOOL	scanfortable	__PR((char *, char *));
extern	BOOL	checkequal	__PR((void));
extern	BOOL	checkcomma	__PR((void));
extern	BOOL	garbage		__PR((char *));
extern	BOOL	isval		__PR((char *));
extern	void	skip_illvar	__PR((char *, char *));
extern	BOOL	set_stringvar	__PR((char *, char *, int));
extern	int	datfileerr	__PR((char *, ...)) __printflike__(1, 2);

/*
 * defect.c:
 */
extern	void	convert_def_blk	__PR((SCSI *scgp));
extern	void	write_def_blk	__PR((SCSI *scgp, BOOL));
extern	void	read_def_blk	__PR((SCSI *scgp));
extern	BOOL	edit_def_blk	__PR((void));
#ifdef	DMAGIC
extern	void	add_def_bfi	__PR((struct scsi_def_bfi *));
#endif

/*
 * diskfmt.c:
 */
extern	int	printgeom		__PR((SCSI *scgp, int current));
extern	void	testformat		__PR((SCSI *scgp, struct disk *dp));
extern	int	Adaptec4000		__PR((SCSI *scgp));
extern	int	Emulex_MD21		__PR((SCSI *scgp));
extern	void	get_defaults		__PR((SCSI *scgp, struct disk *dp));
extern	void	get_lgeom_defaults	__PR((SCSI *scgp, struct disk *dp));
extern	int	reformat_disk		__PR((SCSI *scgp, struct disk *dp));
extern	int	acb_format_disk		__PR((SCSI *scgp, struct disk *dp, int clear_gdl));

/*
 * diskparam.c:
 */
extern	void	select_parameters	__PR((SCSI *scgp, struct disk *dp));

/*
 * fmt.c
 */
extern	int	main			__PR((int ac, char **av));
extern	void	getdev			__PR((SCSI *scgp, BOOL print));
extern	void	estimate_times		__PR((struct disk *dp));
extern	void	print_fmt_time		__PR((struct disk *dp));
extern	void	print_fmt_timeout	__PR((struct disk *dp));
extern	char	*datestr		__PR((void));
extern	void	prdate			__PR((void));
extern	void	getstarttime		__PR((void));
extern	void	getstoptime		__PR((void));
#ifdef timerclear
extern	long	gettimediff		__PR((struct timeval *tp));
#endif
extern	long	prstats			__PR((void));
extern	void	helpexit		__PR((void));
extern	void	disk_null		__PR((struct disk *dp, int init));

/*
 * io.c:
 */
extern	char	*skipwhite	__PR((const char *));
extern	BOOL	cvt_std		__PR((char *, long *, long, long,
							struct disk *));
extern	BOOL	getvalue	__PR((char *, long *, long, long,
				void (*)(char *, long, long, long, struct disk *),
				BOOL (*)(char *, long *, long, long, struct disk *),
				struct disk *));
extern	BOOL	getlong		__PR((char *, long *, long, long));
extern	BOOL	getint		__PR((char *, int *, int, int));
extern	BOOL	getdiskcyls	__PR((char *, long *, long, long));
extern	BOOL	getdiskblocks	__PR((char *, long *, long, long,
						struct disk *));
extern	BOOL	yes		__PR((char *, ...)) __printflike__(1, 2);
extern	void	prbytes		__PR((char *, unsigned char *, int));
extern	char	*permstring	__PR((const char *));
extern	struct strval *strval	__PR((int, struct strval *));
extern	struct strval *namestrval __PR((const char *, struct strval *));
extern	BOOL	getstrval	__PR((const char *, long *,
						struct strval *, long));
/*
 * labelsubs.h
 */
extern	void	read_primary_label	__PR((SCSI *scgp, struct disk *dp));
extern	void	create_label		__PR((SCSI *scgp, struct disk *dp));
extern	void	label_disk		__PR((SCSI *scgp, struct disk *dp));
#ifdef	NDKMAP
extern	int	read_disk_label		__PR((SCSI *scgp, struct dk_label *lp, long secno));
#endif
extern	long	get_default_lncyl	__PR((SCSI *scgp, struct disk *dp));
extern	void	select_label_geom	__PR((SCSI *scgp, struct disk *dp));
extern	BOOL	select_backup_label	__PR((SCSI *scgp, struct disk *dp, BOOL lgeom_ok));
extern	void	select_partition	__PR((SCSI *scgp, struct disk *dp));
extern	void	get_default_partition	__PR((SCSI *scgp, struct disk *dp));

/*
 * makelabel.c:
 */
#ifdef	NDKMAP
extern	void		set_default_vtmap __PR((struct dk_label *));
extern	unsigned short	do_cksum	__PR((struct dk_label *));
extern	BOOL		check_vtmap	__PR((struct dk_label *, BOOL));
extern	void		setlabel_from_val __PR((SCSI *scgp, struct disk *,
						struct dk_label *));
extern	void		makelabel	__PR((SCSI *scgp, struct disk *,
						struct dk_label *));
#ifdef	NDKMAP
#ifdef	EOF
extern	void		prpartab	__PR((FILE *f, char *disk_type, struct dk_label *lp));
#endif
#endif
extern	void		printlabel	__PR((struct dk_label *));
extern	void		printpart	__PR((struct dk_label *, int));
extern	void		printparts	__PR((struct dk_label *));
extern	int		readlabel	__PR((char *, struct dk_label *));
extern	void		writelabel	__PR((char *, struct dk_label *));
extern	int		setlabel	__PR((char *, struct dk_label *));
extern	char		*getasciilabel	__PR((struct dk_label *));
extern	void		setasciilabel	__PR((struct dk_label *, char *));
extern	BOOL		setval_from_label __PR((struct disk *,
						struct dk_label *));
extern	void		label_null	__PR((struct dk_label *));
extern	int		label_cmp	__PR((struct dk_label *,
						struct dk_label *));
extern	BOOL		labelgeom_ok	__PR((struct dk_label *, BOOL));
#endif
extern	BOOL		cvt_cyls	__PR((char *, long *, long, long,
						struct disk *));
extern	BOOL		cvt_bcyls	__PR((char *, long *, long, long,
						struct disk *));

/*
 * maptodisk.c:
 */
extern	int	maptodisk		__PR((int, int, int));
/*extern	scgdrv	*scg_getdrv	__PR((int));*/
extern	char	*diskname		__PR((int));
extern	char	*diskdevname		__PR((int));
extern	int	print_disknames		__PR((int, int, int));

/*
 * modes.c:
 */
extern	BOOL	get_mode_params		__PR((SCSI *scgp, int, char *,
						unsigned char *, unsigned char *,
						unsigned char *, unsigned char *,
						int *));
extern	BOOL	set_mode_params		__PR((SCSI *scgp, char *, unsigned char *, int, int,
						struct disk *));
extern	BOOL	set_error_rec_params	__PR((SCSI *scgp, struct disk *));
extern	BOOL	set_disre_params	__PR((SCSI *scgp, struct disk *));
extern	BOOL	set_format_params	__PR((SCSI *scgp, struct disk *));
extern	BOOL	set_geom		__PR((SCSI *scgp, struct disk *));
extern	BOOL	set_common_control	__PR((SCSI *scgp, struct disk *));
extern	void	get_mode_defaults	__PR((SCSI *scgp, struct disk *));
extern	BOOL	get_error_rec_defaults	__PR((SCSI *scgp, struct disk *));
extern	BOOL	get_disre_defaults	__PR((SCSI *scgp, struct disk *));
extern	BOOL	get_sony_format_defaults __PR((SCSI *scgp, struct disk *));
extern	BOOL	set_sony_params		__PR((SCSI *scgp, struct disk *));

/*
 * modeselect.c:
 */
extern	void	do_modes	__PR((SCSI *scgp));

/*
 * rand_rw.c:
 */
extern	int	random_rw_test	__PR((SCSI *scgp, long first, long last));
extern	int	random_v_test	__PR((SCSI *scgp, long first, long last));

/*
 * repair.c:
 */
extern	int	verify_and_repair_disk	__PR((SCSI *scgp, struct disk *dp));
extern	void	verify_disk		__PR((SCSI *scgp, struct disk *dp, int pass, long first, long last, long maxbad));
extern	void	ext_reassign_block	__PR((SCSI *scgp, long n));

/*
 * sinfo.c:
 */
#ifdef	EOF
extern	void	print_sinfo	__PR((FILE *, SCSI *scgp));
#endif
extern	BOOL	read_sinfo	__PR((SCSI *scgp, struct disk *, BOOL));
extern	BOOL	write_sinfo	__PR((SCSI *scgp, struct disk *));

/*
 * xdelay.c:
 */
extern	void	xdelay __PR((void));

/*
 * xdisk.c:
 */
extern	BOOL	pdisk_eql		__PR((struct disk *, struct disk *));
extern	BOOL	get_ext_diskdata	__PR((SCSI *scgp, char *, struct disk *));
extern	int	cmp_disk		__PR((struct disk *, struct disk *));
extern	BOOL	has_error_rec_params	__PR((struct disk *));
extern	BOOL	has_disre_params	__PR((struct disk *));
#ifdef	INQ_DEV_PRESENT
extern	void	prdisk			__PR((FILE *, struct disk *, struct scsi_inquiry *));
#endif
extern	int	need_params		__PR((struct disk *dp, int type));
extern	int	need_label_params	__PR((struct disk *dp));
extern	int	need_geom_params	__PR((struct disk *dp));
extern	int	need_scsi_params	__PR((struct disk *dp));
extern	int	need_error_rec_params	__PR((struct disk *dp));
extern	int	need_disre_params	__PR((struct disk *dp));

/*
 * xmodes.c:
 */
extern	void	ext_modeselect		__PR((SCSI *scgp, struct disk *));

/*
 * xpart.c:
 */
#ifdef	NDKMAP
extern	BOOL	ext_part __PR((SCSI *scgp, char *, char *, char *, struct dk_label *,
				BOOL (*)(SCSI *, struct disk *, struct dk_label *),
				struct disk *));
#endif

#endif	/* NO_PROT */
