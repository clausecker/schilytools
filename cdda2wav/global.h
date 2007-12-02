/* @(#)global.h	1.17 07/09/10 Copyright 1998-2004 Heiko Eissfeldt, Copyright 2004-2006 J. Schilling */
/*
 * Global Variables
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

#ifdef  MD5_SIGNATURES
#include "md5.h"
#endif
#ifdef	USE_PARANOIA
#include "cdda_paranoia.h"
#endif

#define	outfp	global.out_fp

typedef struct index_list {
	struct index_list	*next;
	int			frameoffset;
} index_list;

typedef struct global {

	char			*dev_name;		/* device name */
	char			*aux_name;		/* device name */
	char			fname_base[200];

	int			have_forked;
	int			parent_died;
	int			audio;
	struct soundfile	*audio_out;
	int			cooked_fd;
	int			no_file;
	int			no_infofile;
	int			no_cddbfile;
	int			quiet;
	int			verbose;
	int			scsi_silent;
	int			scsi_verbose;
	int			scanbus;
	int			multiname;
	int			sh_bits;
	int			Remainder;
	int			SkippedSamples;
	int			OutSampleSize;
	int			need_big_endian;
	int			need_hostorder;
	int			channels;
	unsigned long		iloop;
	unsigned long		nSamplesDoneInTrack;
	unsigned		overlap;
	int			useroverlap;
	FILE			*out_fp;
	long			bufsize; /* The size of the SCSI buffer */
	unsigned		nsectors;
	unsigned		buffers;
	unsigned		shmsize;
	long			pagesize;
	int			in_lendian;
	int			outputendianess;
	int			findminmax;
	int			maxamp[2];
	int			minamp[2];
	unsigned		speed;
	int			userspeed;
	int			ismono;
	int			findmono;
	int			swapchannels;
	int			deemphasize;
	int			gui;
	long			playback_rate;
	int			target; /* SCSI Id to be used */
	int			lun;    /* SCSI Lun to be used */
	UINT4			cddb_id;
	int			cddbp;
	char *			cddbp_server;
	char *			cddbp_port;
	unsigned		cddb_revision;
	int			cddb_year;
	char			cddb_genre[60];
	int			illleadout_cd;
	int			reads_illleadout;
	unsigned char		*cdindex_id;
	unsigned char		*creator;
	unsigned char		*copyright_message;
	unsigned char		*disctitle;
	unsigned char		*tracktitle[100];
	unsigned char		*trackcreator[100];
	index_list		*trackindexlist[100];

	int			paranoia_selected;
#ifdef	USE_PARANOIA
	cdrom_paranoia  	*cdp;

	struct paranoia_parms_t {
		Ucbit	disable_paranoia:1;
		Ucbit	disable_extra_paranoia:1;
		Ucbit	disable_scratch_detect:1;
		Ucbit	disable_scratch_repair:1;
		int	retries;
		int	overlap;
		int	mindynoverlap;
		int	maxdynoverlap;
	} paranoia_parms;
#endif

	unsigned		md5blocksize;
#ifdef	MD5_SIGNATURES
	int			md5count;
	MD5_CTX			context;
	unsigned char		MD5_result[16];
#endif

#ifdef	ECHO_TO_SOUNDCARD
	int			soundcard_fd;
#endif
	int			echo;

	int			just_the_toc;
} global_t;

extern global_t global;
