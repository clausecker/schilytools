/* @(#)modeselect.c	1.16 08/12/22 Copyright 1995-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)modeselect.c	1.16 08/12/22 Copyright 1995-2008 J. Schilling";
#endif
/*
 *	Interactive interface for SCSI mode pages
 *
 *	Copyright (c) 1995-2008 J. Schilling
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

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/schily.h>

#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "fmt.h"
#include <schily/intcvt.h>

LOCAL	void	printmodeheader		__PR((void *));
LOCAL	void	printblockdesc		__PR((void *));
LOCAL	void	printmpheader		__PR((struct scsi_mode_page_header *));
LOCAL	void	print_modepage_01	__PR((void *, int));
LOCAL	void	print_modepage_02	__PR((void *, int));
LOCAL	void	print_modepage_03	__PR((void *, int));
LOCAL	void	print_modepage_04	__PR((void *, int));
LOCAL	void	print_modepage_05	__PR((void *, int));
LOCAL	void	print_modepage_08	__PR((void *, int));
LOCAL	void	print_modepage_0A	__PR((void *, int));
LOCAL	void	print_modepage_38	__PR((void *, int));
LOCAL	void	printpage		__PR((int, u_char *, int));


EXPORT void
do_modes(scgp)
	SCSI	*scgp;
{
	u_char	mode[0x100];
	u_char	dmode[0x100];
	u_char	smode[0x100];
	u_char	cmode[0x100];
	int	page = 0x3F;
	int	len;
	int	headlen;
	char	pagename[64];
	do {
		getint("Enter Page #", &page, 0x0, 0x3F);
		sprintf(pagename, "Mode Page '0x%X'", page);
		if (!get_mode_params(scgp, page, pagename,
					mode, cmode, dmode, smode, &len)) {
		;
	}
		if (len > 0) {
			headlen = sizeof (struct scsi_mode_header) +
			((struct scsi_mode_header *)mode)->blockdesc_len;

			printf("Page: 0x%X\n", page);
/*			prbytes("Header ", mode, headlen);*/
			prbytes("Header ", mode, sizeof (struct scsi_mode_header));
			if (((struct scsi_mode_header *)mode)->blockdesc_len)
			prbytes("Blkdesc", mode+sizeof (struct scsi_mode_header), ((struct scsi_mode_header *)mode)->blockdesc_len);
			prbytes("Current", mode+headlen, len-headlen);

			if (cmpbytes(mode, smode, headlen) < headlen)
				printf("Diff smode head\n");
			prbytes("Saved  ", smode+headlen, len-headlen);

			if (cmpbytes(mode, dmode, headlen) < headlen)
				printf("Diff dmode head\n");
			prbytes("Default", dmode+headlen, len-headlen);

			if (cmpbytes(mode, cmode, headlen) < headlen)
				printf("Diff cmode head\n");
			prbytes("Mask   ", cmode+headlen, len-headlen);

			printmodeheader(mode /*, len*/);
			printblockdesc(mode /*, len*/);
			printpage(page, mode, len);
		}
	} while (!yes("done ? "));
	exit(0);
}

LOCAL void
printmodeheader(modep)
	void	*modep;
{
	struct scsi_mode_header *mh = modep;

	printf("Sense Data len: %d\n", mh->sense_data_len);
	printf("Medium Type: 0x%02X Writeprot: %d Res: %d Cache: %d Res2: %d\n",
					mh->medium_type, mh->write_prot,
					mh->res, mh->cache, mh->res2);
	printf("Blockdesc len: %d\n", mh->blockdesc_len);
}

LOCAL void
printblockdesc(modep)
	void	*modep;
{
	unsigned long	nlblk;
	unsigned long	lblen;
	struct scsi_mode_header *mh = modep;
	struct scsi_mode_blockdesc *bd;

	if (mh->blockdesc_len) {
		bd = (struct scsi_mode_blockdesc *)
			(((char *)modep) + sizeof (struct scsi_mode_header));
		nlblk = a_to_u_3_byte(bd->nlblock);
		lblen = a_to_u_3_byte(bd->lblen);
		printf("Density:   0x%02X Nblocks: %ld Blocklen: %ld\n",
			bd->density, nlblk, lblen);
	}
}

LOCAL void
printmpheader(ph)
	struct scsi_mode_page_header *ph;
{
	printf("Mode Page: 0x%02X Parsave: %d Res: %d len: %d\n",
		ph->p_code, ph->parsave, ph->res, ph->p_len);
}

LOCAL void
print_modepage_01(modep, len)
	void	*modep;
	int	len;
{
	struct scsi_mode_page_01 *mp = modep;

	len -= 2;	/* page header */
	if (len > 0) {
		printf("auto_allw: %d auto_allr: %d tb: %d rc: %d ec: %d rep: %d te: %d dc: %d\n",
		mp->en_auto_reall_w, mp->en_auto_reall_r,
		mp->tranfer_block, mp->read_continuous,
		mp->en_early_corr, mp->report_rec_err,
		mp->term_on_rec_err, mp->disa_correction);
		len -= 1;
	}
	if (len > 0) {
		printf("read retry cnt:  %d\n", mp->rd_retry_count);
		len -= 1;
	}
	if (len > 0) {
		printf("correction span: %d\n", mp->correction_span);
		len -= 1;
	}
	if (len > 0) {
		printf("head offset cnt: %d\n", mp->head_offset_count);
		len -= 1;
	}
	if (len > 0) {
		printf("data strobe off: %d\n", mp->data_strobe_offset);
		len -= 1;
	}
	len -= 1;	/* res */
	if (len > 0) {
		printf("write retry cnt: %d\n", mp->wr_retry_count);
		len -= 1;
	}
	len -= 2;	/* res tape */
	if (len > 0) {
		printf("recov time lim:  %d\n", a_to_u_2_byte(mp->recov_timelim));
		len -= 2;
	}
}

LOCAL void
print_modepage_02(modep, len)
	void	*modep;
	int	len;
{
	struct scsi_mode_page_02 *mp = modep;

	len -= 2;	/* page header */
	if (len > 0) {
		printf("buf full ratio: %d\n", mp->buf_full_ratio);
		len -= 1;
	}
	if (len > 0) {
		printf("buf empt ratio: %d\n", mp->buf_empt_ratio);
		len -= 1;
	}
	if (len > 0) {
		printf("bus incat lim:  %d\n", a_to_u_2_byte(mp->bus_inact_limit));
		len -= 2;
	}
	if (len > 0) {
		printf("disc time lim:  %d\n", a_to_u_2_byte(mp->disc_time_limit));
		len -= 2;
	}
	if (len > 0) {
		printf("conn time lim:  %d\n", a_to_u_2_byte(mp->conn_time_limit));
		len -= 2;
	}
	if (len > 0) {
		printf("max burst size: %d\n", a_to_u_2_byte(mp->max_burst_size));
		len -= 2;
	}
	if (len > 0) {
		printf("data tr dis ctl:%d\n", mp->data_tr_dis_ctl);
		len -= 1;
	}
}

LOCAL void
print_modepage_03(modep, len)
	void	*modep;
	int	len;
{
	struct scsi_mode_page_03 *mp = modep;

	len -= 2;	/* page header */
	if (len > 0) {
		printf("trk per zone:     %d\n", a_to_u_2_byte(mp->trk_per_zone));
		len -= 2;
	}
	if (len > 0) {
		printf("alt sec per zone: %d\n", a_to_u_2_byte(mp->alt_sec_per_zone));
		len -= 2;
	}
	if (len > 0) {
		printf("alt trk per zone: %d\n", a_to_u_2_byte(mp->alt_trk_per_zone));
		len -= 2;
	}
	if (len > 0) {
		printf("alt trk per vol:  %d\n", a_to_u_2_byte(mp->alt_trk_per_vol));
		len -= 2;
	}
	if (len > 0) {
		printf("sect per trk:     %d\n", a_to_u_2_byte(mp->sect_per_trk));
		len -= 2;
	}
	if (len > 0) {
		printf("bytes per psect:  %d\n", a_to_u_2_byte(mp->bytes_per_phys_sect));
		len -= 2;
	}
	if (len > 0) {
		printf("interleave:       %d\n", a_to_u_2_byte(mp->interleave));
		len -= 2;
	}
	if (len > 0) {
		printf("trk skew:         %d\n", a_to_u_2_byte(mp->trk_skew));
		len -= 2;
	}
	if (len > 0) {
		printf("cyl skew:         %d\n", a_to_u_2_byte(mp->cyl_skew));
		len -= 2;
	}
	if (len > 0) {
		printf("softsec: %d hardsec: %d removable: %d fmt by surf: %d inhibit save: %d\n",
			mp->soft_sec, mp->hard_sec,
			mp->removable, mp->fmt_by_surface,
			mp->inhibit_save);
		len -= 1;
	}
}

LOCAL void
print_modepage_04(modep, len)
	void	*modep;
	int	len;
{
	struct scsi_mode_page_04 *mp = modep;

	len -= 2;	/* page header */
	if (len > 0) {
		printf("ncyl:            %ld\n", a_to_u_3_byte(mp->ncyl));
		len -= 3;
	}
	if (len > 0) {
		printf("nnead:           %d\n", mp->nhead);
		len -= 1;
	}
	if (len > 0) {
		printf("start precomp:   %ld\n", a_to_u_3_byte(mp->start_precomp));
		len -= 3;
	}
	if (len > 0) {
		printf("start red wrcur: %ld\n", a_to_u_3_byte(mp->start_red_wcurrent));
		len -= 3;
	}
	if (len > 0) {
		printf("step rate:       %d\n", a_to_u_2_byte(mp->step_rate));
		len -= 2;
	}
	if (len > 0) {
		printf("landing zone:    %ld\n", a_to_u_3_byte(mp->landing_zone));
		len -= 3;
	}
	if (len > 0) {
		printf("rot pos locking: %d\n", mp->rot_pos_locking);
		len -= 1;
	}
	if (len > 0) {
		printf("rotational off:  %d\n", mp->rotational_off);
		len -= 1;
	}
	len -= 1;	/* res1 */
	if (len > 0) {
		printf("rotation rate:   %d\n", a_to_u_2_byte(mp->rotation_rate));
		len -= 2;
	}
}

LOCAL void
print_modepage_05(modep, len)
	void	*modep;
	int	len;
{
	struct scsi_mode_page_05 *mp = modep;

	len -= 2;	/* page header */
	if (len > 0) {
		printf("transfer rate:     %X\n", a_to_u_2_byte(mp->transfer_rate));
		len -= 2;
	}
	if (len > 0) {
		printf("nnead:             %d\n", mp->nhead);
		len -= 1;
	}
	if (len > 0) {
		printf("sect per track:    %d\n", mp->sect_per_trk);
		len -= 1;
	}
	if (len > 0) {
		printf("bytes per psect:   %d\n", a_to_u_2_byte(mp->bytes_per_phys_sect));
		len -= 2;
	}
	if (len > 0) {
		printf("ncyl:              %d\n", a_to_u_2_byte(mp->ncyl));
		len -= 2;
	}
	if (len > 0) {
		printf("start precomp:     %d\n", a_to_u_2_byte(mp->start_precomp));
		len -= 2;
	}
	if (len > 0) {
		printf("start red wrcur:   %d\n", a_to_u_2_byte(mp->start_red_wcurrent));
		len -= 2;
	}
	if (len > 0) {
		printf("step rate:         %d\n", a_to_u_2_byte(mp->step_rate));
		len -= 2;
	}
	if (len > 0) {
		printf("step pule width:   %d\n", mp->step_pulse_width);
		len -= 1;
	}
	if (len > 0) {
		printf("head settle delay: %d\n", a_to_u_2_byte(mp->head_settle_delay));
		len -= 2;
	}
	if (len > 0) {
		printf("motor on delay:    %d\n", mp->motor_on_delay);
		len -= 1;
	}
	if (len > 0) {
		printf("motor off delay:   %d\n", mp->motor_off_delay);
		len -= 1;
	}
	if (len > 0) {
		printf("XXX:               %X\n", *(char *)&(&mp->motor_off_delay)[1]);
		len -= 1;
	}
	if (len > 0) {
		printf("XXX:               %X\n", *(char *)&(&mp->motor_off_delay)[2]);
		len -= 1;
	}
	if (len > 0) {
		printf("write compensation:%d\n", mp->write_compensation);
		len -= 1;
	}
	if (len > 0) {
		printf("head load delay:   %d\n", mp->head_load_delay);
		len -= 1;
	}
	if (len > 0) {
		printf("head unload delay: %d\n", mp->head_unload_delay);
		len -= 1;
	}
	if (len > 0) {
		printf("pin 2 use   :      %d\n", mp->pin_2_use);
		printf("pin 34 use  :      %d\n", mp->pin_34_use);
		len -= 1;
	}
	if (len > 0) {
		printf("pin 1 use   :      %d\n", mp->pin_1_use);
		printf("pin 4 use   :      %d\n", mp->pin_4_use);
		len -= 1;
	}
	if (len > 0) {
		printf("rotation rate:     %d\n", a_to_u_2_byte(mp->rotation_rate));
		len -= 2;
	}
	if (len > 0) {
		printf("res:               %02X,%02X\n", mp->res[0], mp->res[1]);
		len -= 2;
	}
}

LOCAL void
print_modepage_08(modep, len)
	void	*modep;
	int	len;
{
	struct scsi_mode_page_08 *mp = modep;

	len -= 2;	/* page header */
	if (len > 0) {
		printf("en wt cache: %d multiple fac: %d disa rd cache: %d\n",
			mp->en_wt_cache, mp->muliple_fact, mp->disa_rd_cache);
		len -= 1;
	}
	if (len > 0) {
		printf("demand rd ret pri: %d wt ret pri: %d\n",
			mp->demand_rd_ret_pri, mp->wt_ret_pri);
		len -= 1;
	}
	if (len > 0) {
		printf("disa pref tr len: %d\n", a_to_u_2_byte(mp->disa_pref_tr_len));
		len -= 2;
	}
	if (len > 0) {
		printf("min pref:  %d\n", a_to_u_2_byte(mp->min_pref));
		len -= 2;
	}
	if (len > 0) {
		printf("max pref:  %d\n", a_to_u_2_byte(mp->max_pref));
		len -= 2;
	}
	if (len > 0) {
		printf("max pref ceiling:  %d\n", a_to_u_2_byte(mp->max_pref_ceiling));
		len -= 2;
	}
}

LOCAL void
print_modepage_0A(modep, len)
	void	*modep;
	int	len;
{
	struct scsi_mode_page_0A *mp = modep;

	len -= 2;	/* page header */
	if (len > 0) {
		printf("rep log exeption: %d\n",
			mp->rep_log_exeption);
		len -= 1;
	}
	if (len > 0) {
		printf("queue alg mod: %d queuing err man: %d dis queuing: %d\n",
			mp->queue_alg_mod, mp->queuing_err_man, mp->dis_queuing);
		len -= 1;
	}
	if (len > 0) {
		printf("en ext cont all: %d RAENP: %d UAENP: %d EAENP: %d\n",
			mp->en_ext_cont_all, mp->RAENP, mp->UAENP, mp->EAENP);
		len -= 1;
	}
	len -= 1;	/* res1 */
	if (len > 0) {
		printf("ready_aen_hold_per:  %d\n",
			a_to_u_2_byte(mp->ready_aen_hold_per));
		len -= 2;
	}
}

LOCAL void
print_modepage_38(modep, len)
	void	*modep;
	int	len;
{
	struct ccs_mode_page_38 *mp = modep;

	len -= 2;	/* page header */
	if (len > 0) {
		printf("wr index en: %d cache en: %d: cache table size: %d\n",
		mp->wr_index_en, mp->cache_en, mp->cache_table_size);
		len -= 1;
	}
	if (len > 0) {
		printf("threshold: %d\n", mp->threshold);
		len -= 1;
	}
	if (len > 0) {
		printf("max prefetch: %d\n", mp->max_prefetch);
		len -= 1;
	}
	if (len > 0) {
		printf("max multiplier: %d\n", mp->max_multiplier);
		len -= 1;
	}
	if (len > 0) {
		printf("min prefetch: %d\n", mp->min_prefetch);
		len -= 1;
	}
	if (len > 0) {
		printf("min multiplier: %d\n", mp->min_multiplier);
		len -= 1;
	}
}

LOCAL void
printpage(page, modep, len)
	int	page;
	u_char	*modep;
	int	len;
{
	struct scsi_mode_page_header *mh;

	mh = (struct scsi_mode_page_header *)
			(modep + sizeof (struct scsi_mode_header) +
			((struct scsi_mode_header *)modep)->blockdesc_len);

	printmpheader(mh);
	len = modep[0];
	len += 1;
	len -= (u_char *)mh - modep;
	printf("len: %d\n", len);

	switch (page) {
	case 0x01:	print_modepage_01((u_char *)mh, len); break;
	case 0x02:	print_modepage_02((u_char *)mh, len); break;
	case 0x03:	print_modepage_03((u_char *)mh, len); break;
	case 0x04:	print_modepage_04((u_char *)mh, len); break;
	case 0x05:	print_modepage_05((u_char *)mh, len); break;
	case 0x08:	print_modepage_08((u_char *)mh, len); break;
	case 0x0A:	print_modepage_0A((u_char *)mh, len); break;
	case 0x38:	print_modepage_38((u_char *)mh, len); break;
	default: prbytes("Unknown Page", (u_char *)mh, len);
	};
}
