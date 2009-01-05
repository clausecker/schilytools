/* @(#)modes.c	1.41 08/12/22 Copyright 1988-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)modes.c	1.41 08/12/22 Copyright 1988-2008 J. Schilling";
#endif
/*
 *	Handle SCSI mode pages
 *
 *	Copyright (c) 1988-2008 J. Schilling
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
#include <schily/utypes.h>
#include <schily/time.h>
#include <schily/ioctl.h>

#include <schily/standard.h>
#include <schily/schily.h>

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsidefs.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"
#include "fmt.h"

extern	int	debug;

extern	int	force;
extern	int	save_mp;
extern	int	defmodes;
extern	int	scsi_compliant;

LOCAL	BOOL	has_mode_page			__PR((SCSI *scgp, int, char *, int *));
EXPORT	BOOL	get_mode_params			__PR((SCSI *scgp, int page, char *pagename,
							u_char *modep, u_char *cmodep, u_char *dmodep, u_char *smodep,
							int *lenp));
EXPORT	BOOL	set_mode_params			__PR((SCSI *scgp, char *, u_char *, int, int,
							struct disk *));
EXPORT	BOOL	set_error_rec_params		__PR((SCSI *scgp, struct disk *));
LOCAL	void	get_secsize			__PR((struct disk *, u_char *));
LOCAL	void	get_capacity			__PR((struct disk *, u_char *));
LOCAL	BOOL	get_format_defaults		__PR((SCSI *scgp, struct disk *));
LOCAL	BOOL	get_geom_defaults		__PR((SCSI *scgp, struct disk *));
LOCAL	BOOL	get_common_control_defaults	__PR((SCSI *scgp, struct disk *));

LOCAL BOOL
has_mode_page(scgp, page, pagename, lenp)
	SCSI	*scgp;
	int	page;
	char	*pagename;
	int	*lenp;
{
	u_char	mode[0x100];
	int	len = 1;				/* Nach SCSI Norm */
	int	try = 0;
	struct	scsi_mode_page_header *mp;

again:
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	if (lenp)
		*lenp = 0;

	(void) test_unit_ready(scgp);
	scgp->silent++;
/* Maxoptix bringt Aborted cmd 0x0B mit code 0x4E (overlapping cmds)*/
	if (mode_sense(scgp, mode, len, page, 2) < 0) {	/* Page n default */
		scgp->silent--;
		return (FALSE);
	} else {
		len = ((struct scsi_mode_header *)mode)->sense_data_len + 1;
	}
	if (mode_sense(scgp, mode, len, page, 2) < 0) {	/* Page n default */
		scgp->silent--;
		return (FALSE);
	}
	scgp->silent--;

	if (scgp->verbose)
		prbytes("Mode Sense Data", mode, len - scg_getresid(scgp));
	mp = (struct scsi_mode_page_header *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
	if (scgp->verbose)
		prbytes("Mode Sense Data", (u_char *)mp, mp->p_len+2);

	if (mp->p_len == 0) {
		if (!scsi_compliant && try == 0) {
			len = sizeof (struct scsi_mode_header) +
			((struct scsi_mode_header *)mode)->blockdesc_len;
			/*
			 * add sizeof page header (page # + len byte)
			 * (should normaly result in len == 14)
			 * this allowes to work with:
			 * 	Quantum Q210S	(wants at least 13)
			 * 	MD2x		(wants at least 4)
			 */
			len += 2;
			try++;
			goto again;
		}
		errmsgno(EX_BAD,
			"Warning: controller returns zero sized %s page.\n",
								pagename);
	}

	if (lenp)
		*lenp = len;
	return (mp->p_len > 0);
}

EXPORT BOOL
get_mode_params(scgp, page, pagename, modep, cmodep, dmodep, smodep, lenp)
	SCSI	*scgp;
	int	page;
	char	*pagename;
	u_char	*modep;
	u_char	*cmodep;
	u_char	*dmodep;
	u_char	*smodep;
	int	*lenp;
{
	int	len;
	BOOL	ret = TRUE;

	if (lenp)
		*lenp = 0;
	if (!has_mode_page(scgp, page, pagename, &len)) {
		if (!scgp->silent) errmsgno(EX_BAD,
			"Warning: controller does not support %s page.\n",
								pagename);
		return (TRUE);	/* Hoffentlich klappt's trotzdem */
	}
	if (lenp)
		*lenp = len;

	if (modep) {
		fillbytes(modep, 0x100, '\0');
		if (mode_sense(scgp, modep, len, page, 0) < 0) { /* Page x current */
			errmsgno(EX_BAD, "Cannot get %s data.\n", pagename);
			ret = FALSE;
		} else if (scgp->verbose) {
			prbytes("Mode Sense Data", modep, len - scg_getresid(scgp));
		}
	}

	if (cmodep) {
		fillbytes(cmodep, 0x100, '\0');
		if (mode_sense(scgp, cmodep, len, page, 1) < 0) { /* Page x change */
			errmsgno(EX_BAD, "Cannot get %s mask.\n", pagename);
			ret = FALSE;
		} else if (scgp->verbose) {
			prbytes("Mode Sense Data", cmodep, len - scg_getresid(scgp));
		}
	}

	if (dmodep) {
		fillbytes(dmodep, 0x100, '\0');
		if (mode_sense(scgp, dmodep, len, page, 2) < 0) { /* Page x default */
			errmsgno(EX_BAD, "Cannot get default %s data.\n",
								pagename);
			ret = FALSE;
		} else if (scgp->verbose) {
			prbytes("Mode Sense Data", dmodep, len - scg_getresid(scgp));
		}
	}

	if (smodep) {
		fillbytes(smodep, 0x100, '\0');
		if (mode_sense(scgp, smodep, len, page, 3) < 0) { /* Page x saved */
			errmsgno(EX_BAD, "Cannot get saved %s data.\n", pagename);
			ret = FALSE;
		} else if (scgp->verbose) {
			prbytes("Mode Sense Data", smodep, len - scg_getresid(scgp));
		}
	}

	return (ret);
}

EXPORT BOOL
set_mode_params(scgp, pagename, modep, len, save, dp)
	SCSI	*scgp;
	char	*pagename;
	u_char	*modep;
	int	len;
	int	save;
	struct disk	*dp;
{
	int	i;

	((struct scsi_modesel_header *)modep)->sense_data_len	= 0;
	((struct scsi_modesel_header *)modep)->res2		= 0;

	i = ((struct scsi_mode_header *)modep)->blockdesc_len;
	if (i > 0) {
		/*
		 * Not only set the old "nblock" field to zero but also
		 * clear the "density" field that is used for the MSB
		 * of the disk size for "bigger disks".
		 */
		((struct scsi_mode_data *)modep)->blockdesc.density = 0;
		i_to_3_byte(
			((struct scsi_mode_data *)modep)->blockdesc.nlblock,
								0);
#ifdef	__DISKTEST__
		/*
		 * Test data to set a specific disk size.
		 */
		((struct scsi_mode_data *)modep)->blockdesc.density = 4;
		i_to_3_byte(
			((struct scsi_mode_data *)modep)->blockdesc.nlblock,
								0x3d6720);
#endif
		i_to_3_byte(((struct scsi_mode_data *)modep)->blockdesc.lblen,
							dp->secsize);
	}

	if (mode_select(scgp, modep, len, save, scgp->inq->ansi_version >= 2) < 0) {
		if (mode_select(scgp, modep, len, 0, scgp->inq->ansi_version >= 2) < 0) {
			errmsgno(EX_BAD,
			    "Warning: using default %s data.\n", pagename);
		}
	}
	return (TRUE);
}

EXPORT BOOL
set_error_rec_params(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	u_char	cmode[0x100];
	int	len;
	BOOL	mp_save;
	struct	scsi_mode_page_01 *mp;
	struct	scsi_mode_page_01 *cmp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	fillbytes((caddr_t)cmode, sizeof (cmode), '\0');

	if (!get_mode_params(scgp, 1, "error recovery parameter",
				(u_char *)0, cmode, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	cmp = (struct scsi_mode_page_01 *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);
	mp = (struct scsi_mode_page_01 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
	mp_save = save_mp && mp->parsave;
	mp->parsave = 0;

#ifdef	__nono__
	if (cmp->rd_retry_count != 0)	/*XXX*/
		mp->tranfer_block = 1;	/*XXX*/
#endif

	if (dp->rd_retry_count >= 0 && cmp->rd_retry_count != 0)
		mp->rd_retry_count = dp->rd_retry_count;

	if (dp->wr_retry_count >= 0 && cmp->wr_retry_count != 0)
		mp->wr_retry_count = dp->wr_retry_count;

	if (dp->recov_timelim >= 0 &&
	    a_to_u_2_byte(cmp->recov_timelim) != 0)
		i_to_2_byte(mp->recov_timelim, dp->recov_timelim);

	return (set_mode_params(scgp, "error recovery parameter", mode, len, mp_save, dp));
}

EXPORT BOOL
set_disre_params(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	u_char	cmode[0x100];
	int	len;
	BOOL	mp_save;
	struct	scsi_mode_page_02 *mp;
	struct	scsi_mode_page_02 *cmp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	fillbytes((caddr_t)cmode, sizeof (cmode), '\0');

	if (!get_mode_params(scgp, 2, "disre parameter",
				(u_char *)0, cmode, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	cmp = (struct scsi_mode_page_02 *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);
	mp = (struct scsi_mode_page_02 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
	mp_save = save_mp && mp->parsave;
	mp->parsave = 0;

	if (dp->buf_full_ratio >= 0 && cmp->buf_full_ratio != 0)
		mp->buf_full_ratio = dp->buf_full_ratio;

	if (dp->buf_empt_ratio >= 0 && cmp->buf_empt_ratio != 0)
		mp->buf_empt_ratio = dp->buf_empt_ratio;

	if (dp->bus_inact_limit >= 0 &&
	    a_to_u_2_byte(cmp->bus_inact_limit) != 0)
		i_to_2_byte(mp->bus_inact_limit, dp->bus_inact_limit);

	if (dp->disc_time_limit >= 0 &&
	    a_to_u_2_byte(cmp->disc_time_limit) != 0)
		i_to_2_byte(mp->disc_time_limit, dp->disc_time_limit);

	if (dp->conn_time_limit >= 0 &&
	    a_to_u_2_byte(cmp->conn_time_limit) != 0)
		i_to_2_byte(mp->conn_time_limit, dp->conn_time_limit);

	if (dp->max_burst_size >= 0 &&
	    a_to_u_2_byte(cmp->max_burst_size) != 0)
		i_to_2_byte(mp->max_burst_size, dp->max_burst_size);

	if (dp->data_tr_dis_ctl >= 0 && cmp->data_tr_dis_ctl != 0)
		mp->data_tr_dis_ctl = dp->data_tr_dis_ctl;

#ifdef	OLD
/*XXX*/						/* 4.0 ms max Connect time */
	if (a_to_u_2_byte(cmp->bus_inact_limit) != 0)
		i_to_2_byte(mp->bus_inact_limit, 40);
#endif

	return (set_mode_params(scgp, "disre parameter", mode, len, mp_save, dp));
}

EXPORT BOOL
set_format_params(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	u_char	cmode[0x100];
	int	len;
	int	i;
	BOOL	mp_save;
	struct	scsi_mode_page_03 *mp;
	struct	scsi_mode_page_03 *cmp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	fillbytes((caddr_t)cmode, sizeof (cmode), '\0');

	if (!get_mode_params(scgp, 3, "format parameter",
				(u_char *)0, cmode, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	i = ((struct scsi_mode_header *)mode)->blockdesc_len;
	if (i == 0)
		errmsgno(EX_BAD,
		"Warning: No Blockdesriptor, can't set logical Blocksize.\n");
	if (i > 8)
		errmsgno(EX_BAD, "More than one Blockdesriptor ???\n");

	cmp = (struct scsi_mode_page_03 *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);
	mp = (struct scsi_mode_page_03 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
	mp_save = save_mp && mp->parsave;
	mp->parsave = 0;

	if (a_to_u_2_byte(cmp->trk_per_zone) != 0)
		i_to_2_byte(mp->trk_per_zone, dp->tpz);

	if (a_to_u_2_byte(cmp->alt_sec_per_zone) != 0)
		i_to_2_byte(mp->alt_sec_per_zone, dp->aspz);

	if (a_to_u_2_byte(cmp->alt_trk_per_vol) != 0)
		i_to_2_byte(mp->alt_trk_per_vol, dp->atrk);

	if (a_to_u_2_byte(cmp->sect_per_trk) != 0)
		i_to_2_byte(mp->sect_per_trk, dp->spt);

	if (a_to_u_2_byte(cmp->bytes_per_phys_sect) != 0 &&
						dp->phys_secsize > 0)
		i_to_2_byte(mp->bytes_per_phys_sect, dp->phys_secsize);

	if (a_to_u_2_byte(cmp->trk_skew) != 0)
		i_to_2_byte(mp->trk_skew, dp->track_skew);

	if (a_to_u_2_byte(cmp->cyl_skew) != 0)
		i_to_2_byte(mp->cyl_skew, dp->cyl_skew);

#ifdef	OLD
	/*
	 * Der MD21 hat leider die Felde Sectors per Track und
	 * Physical Sectorsize auf Null gesetzt,
	 * wenn ein Hardsektoriertes Laufwerk angeschloszen ist.
	 * XXX Ist die Methode mode und cmode zu verunden ueberhaupt
	 * richtig ???
	 */
	mp = (struct scsi_mode_page_03 *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);
	if (a_to_u_2_byte(mp->sect_per_trk) == 0)
		i_to_2_byte(mp->sect_per_trk, dp->spt);
	if (a_to_u_2_byte(mp->bytes_per_phys_sect) == 0)
		i_to_2_byte(mp->bytes_per_phys_sect, dp->secsize);
#endif

	return (set_mode_params(scgp, "format parameter", mode, len, mp_save, dp));
}

EXPORT BOOL
set_geom(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	u_char	cmode[0x100];
	int	len;
	BOOL	mp_save;
	struct	scsi_mode_page_04 *mp;
	struct	scsi_mode_page_04 *cmp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	fillbytes((caddr_t)cmode, sizeof (cmode), '\0');

	if (!get_mode_params(scgp, 4, "geometry",
				(u_char *)0, cmode, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	cmp = (struct scsi_mode_page_04 *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);
	mp = (struct scsi_mode_page_04 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
	mp_save = save_mp && mp->parsave;
	mp->parsave = 0;

	if (a_to_u_3_byte(cmp->ncyl) != 0)
		i_to_3_byte(mp->ncyl, dp->pcyl);

	if (cmp->nhead != 0)
		mp->nhead = dp->nhead;

	if (cmp->rot_pos_locking != 0)
		mp->rot_pos_locking = dp->rot_pos_locking;

	if (cmp->rotational_off != 0)
		mp->rotational_off = dp->rotational_off;

	return (set_mode_params(scgp, "geometry", mode, len, mp_save, dp));
}

EXPORT BOOL
set_common_control(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	u_char	cmode[0x100];
	int	len;
	BOOL	mp_save;
	struct	scsi_mode_page_0A *mp;
	struct	scsi_mode_page_0A *cmp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	fillbytes((caddr_t)cmode, sizeof (cmode), '\0');

	if (!get_mode_params(scgp, 0xA, "common device control",
				(u_char *)0, cmode, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	cmp = (struct scsi_mode_page_0A *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);
	mp = (struct scsi_mode_page_0A *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
	mp_save = save_mp && mp->parsave;
	mp->parsave = 0;

	if (cmp->queue_alg_mod != 0)
		mp->queue_alg_mod = dp->queue_alg_mod;

	if (cmp->dis_queuing != 0)
		mp->dis_queuing = dp->dis_queuing;

	return (TRUE);
}


EXPORT void
get_mode_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	get_error_rec_defaults(scgp, dp);
	get_disre_defaults(scgp, dp);
	get_geom_defaults(scgp, dp); /* nhead wird in set_format_defaults gebraucht */
	get_format_defaults(scgp, dp);
	if (dp->secsize < 0) {
		if (read_capacity(scgp) < 0)
			return;
		if (scgp->cap->c_bsize > 0)
			dp->secsize = scgp->cap->c_bsize;
	}
	scgp->silent++;
	get_common_control_defaults(scgp, dp);
	scgp->silent--;
}

LOCAL void
get_secsize(dp, modep)
	struct disk	*dp;
	u_char	*modep;
{
	long	tmp;

	if (dp->secsize < 0 &&
			((struct scsi_mode_header *)modep)->blockdesc_len) {
		tmp = a_to_u_3_byte(((struct scsi_mode_data *)modep)->
							blockdesc.lblen);
		if (tmp != 0)
			dp->secsize = tmp;
	}
}

LOCAL void
get_capacity(dp, modep)
	struct disk	*dp;
	u_char	*modep;
{
	long	tmp;

	if (dp->capacity < 0 &&
			((struct scsi_mode_header *)modep)->blockdesc_len) {
		tmp = a_to_u_3_byte(((struct scsi_mode_data *)modep)->
							blockdesc.nlblock);
		if (tmp != 0)
			dp->capacity = tmp;
	}
}

EXPORT BOOL
get_error_rec_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	u_char	cmode[0x100];
	int	len;
	long	tmp;
	long	ctmp;
	struct	scsi_mode_page_01 *mp;
	struct	scsi_mode_page_01 *cmp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	fillbytes((caddr_t)cmode, sizeof (cmode), '\0');

	if (defmodes == 0 && dp->formatted > 0 &&
		get_mode_params(scgp, 1, "error recovery parameter",
				mode, cmode, (u_char *)0, (u_char *)0, &len))
		;
	else if (!get_mode_params(scgp, 1, "error recovery parameter",
				(u_char *)0, cmode, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	cmp = (struct scsi_mode_page_01 *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);
	mp = (struct scsi_mode_page_01 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	get_secsize(dp, mode);
	get_capacity(dp, mode);

	if (dp->rd_retry_count < 0 && cmp->rd_retry_count != 0)
		dp->rd_retry_count = mp->rd_retry_count;

	if (dp->wr_retry_count < 0 && cmp->wr_retry_count != 0)
		dp->wr_retry_count = mp->wr_retry_count;

	tmp  = a_to_u_2_byte(mp->recov_timelim);
	ctmp = a_to_u_2_byte(cmp->recov_timelim);
	if (dp->recov_timelim < 0 && ctmp != 0)
		dp->recov_timelim = tmp;

	return (TRUE);
}

EXPORT BOOL
get_disre_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	u_char	cmode[0x100];
	int	len;
	long	tmp;
	long	ctmp;
	struct	scsi_mode_page_02 *mp;
	struct	scsi_mode_page_02 *cmp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	fillbytes((caddr_t)cmode, sizeof (cmode), '\0');

	if (defmodes == 0 && dp->formatted > 0 &&
		get_mode_params(scgp, 2, "disre parameter",
				mode, cmode, (u_char *)0, (u_char *)0, &len))
		;
	else if (!get_mode_params(scgp, 2, "disre parameter",
				(u_char *)0, cmode, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	cmp = (struct scsi_mode_page_02 *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);
	mp = (struct scsi_mode_page_02 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	get_secsize(dp, mode);
	get_capacity(dp, mode);

	if (dp->buf_full_ratio < 0 && cmp->buf_full_ratio != 0)
		dp->buf_full_ratio = mp->buf_full_ratio;

	if (dp->buf_empt_ratio < 0 && cmp->buf_empt_ratio != 0)
		dp->buf_empt_ratio = mp->buf_empt_ratio;

	tmp  = a_to_u_2_byte(mp->bus_inact_limit);
	ctmp = a_to_u_2_byte(cmp->bus_inact_limit);
	if (dp->bus_inact_limit < 0 && ctmp != 0)
		dp->bus_inact_limit = tmp;

	tmp  = a_to_u_2_byte(mp->disc_time_limit);
	ctmp = a_to_u_2_byte(cmp->disc_time_limit);
	if (dp->disc_time_limit < 0 && ctmp != 0)
		dp->disc_time_limit = tmp;

	tmp  = a_to_u_2_byte(mp->conn_time_limit);
	ctmp = a_to_u_2_byte(cmp->conn_time_limit);
	if (dp->conn_time_limit < 0 && ctmp != 0)
		dp->conn_time_limit = tmp;

	tmp  = a_to_u_2_byte(mp->max_burst_size);
	ctmp = a_to_u_2_byte(cmp->max_burst_size);
	if (dp->max_burst_size < 0 && ctmp != 0)
		dp->max_burst_size = tmp;

	if (dp->data_tr_dis_ctl < 0 && cmp->data_tr_dis_ctl != 0)
		dp->data_tr_dis_ctl = mp->data_tr_dis_ctl;

	return (TRUE);
}

LOCAL BOOL
get_format_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	u_char	cmode[0x100];
	int	len;
	long	tmp;
	struct	scsi_mode_page_03 *mp;
	struct	scsi_mode_page_03 *cmp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	fillbytes((caddr_t)cmode, sizeof (cmode), '\0');

	if (defmodes == 0 && dp->formatted > 0 &&
		get_mode_params(scgp, 3, "format parameter",
				mode, cmode, (u_char *)0, (u_char *)0, &len))
		;
	else if (!get_mode_params(scgp, 3, "format parameter",
				(u_char *)0, cmode, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	cmp = (struct scsi_mode_page_03 *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);
	mp = (struct scsi_mode_page_03 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	get_secsize(dp, mode);
	get_capacity(dp, mode);

	tmp = a_to_u_2_byte(mp->trk_per_zone);
	if (dp->tpz < 0)
		dp->tpz = tmp;
	tmp = a_to_u_2_byte(cmp->trk_per_zone);
	if (dp->tpz < 0 || tmp != 0) {
		dp->mintpz = 0;
		dp->maxtpz = 0xFFFF;
	} else {
		dp->mintpz = dp->maxtpz = dp->tpz;
	}

	tmp = a_to_u_2_byte(mp->alt_sec_per_zone);
	if (dp->aspz < 0)
		dp->aspz = tmp;

	tmp = a_to_u_2_byte(mp->alt_trk_per_vol);
	if (dp->atrk < 0)
		dp->atrk = tmp;

	tmp = a_to_u_2_byte(mp->sect_per_trk);
	if (dp->spt < 0 && tmp != 0)
		dp->spt = tmp;

	tmp = a_to_u_2_byte(mp->bytes_per_phys_sect);
	if (dp->phys_secsize < 0 && tmp != 0) {
		long ctmp = a_to_u_2_byte(cmp->bytes_per_phys_sect);

		if (ctmp != 0)
			dp->phys_secsize = tmp;

		if (debug) printf("phys_secsize: %ld change: %ld (0x%lX)\n",
							tmp, ctmp, ctmp);
	}

	tmp = a_to_u_2_byte(mp->interleave);
	if (dp->interleave < 0)
		dp->interleave = tmp;

	tmp = a_to_u_2_byte(mp->trk_skew);
	if (dp->track_skew < 0)
		dp->track_skew = tmp;

	tmp = a_to_u_2_byte(mp->cyl_skew);
	if (dp->cyl_skew < 0)
		dp->cyl_skew = tmp;

	return (TRUE);
}

LOCAL BOOL
get_geom_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	int	len;
	long	tmp;
	struct	scsi_mode_page_04 *mp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (defmodes == 0 && dp->formatted > 0 &&
		get_mode_params(scgp, 4, "geometry",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		;
	else if (!get_mode_params(scgp, 4, "geometry",
			(u_char *)0, (u_char *)0, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	mp = (struct scsi_mode_page_04 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	get_secsize(dp, mode);
	get_capacity(dp, mode);

	tmp = a_to_u_3_byte(mp->ncyl);
	if (dp->pcyl < 0 && tmp != 0)
		dp->pcyl = tmp;

	if (dp->nhead < 0 && mp->nhead != 0)
		dp->nhead = mp->nhead;

	tmp = a_to_u_2_byte(mp->rotation_rate);
	if (dp->rpm < 0 && tmp != 0)
		dp->rpm = tmp;

	tmp = mp->rot_pos_locking;
	if (dp->rot_pos_locking < 0)
		dp->rot_pos_locking = tmp;

	tmp = mp->rotational_off;
	if (dp->rotational_off < 0)
		dp->rotational_off = tmp;

	return (TRUE);
}

LOCAL BOOL
get_common_control_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	int	len;
	struct	scsi_mode_page_0A *mp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (defmodes == 0 && dp->formatted > 0 &&
		get_mode_params(scgp, 0xA, "common device control",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		;
	else if (!get_mode_params(scgp, 0xA, "common device control",
			(u_char *)0, (u_char *)0, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	mp = (struct scsi_mode_page_0A *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	dp->queue_alg_mod = mp->queue_alg_mod;
	dp->dis_queuing = mp->dis_queuing;

	return (TRUE);
}


EXPORT BOOL
get_sony_format_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	int	len;
	long	tmp;
	struct	sony_mode_page_20 *mp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (defmodes == 0 && dp->formatted > 0 &&
		get_mode_params(scgp, 0x20, "sony format",
			mode, (u_char *)0, (u_char *)0, (u_char *)0, &len))
		;
	else if (!get_mode_params(scgp, 0x20, "sony format",
			(u_char *)0, (u_char *)0, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

	mp = (struct sony_mode_page_20 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	get_secsize(dp, mode);
	get_capacity(dp, mode);

	if (dp->fmt_mode < 0)
		dp->fmt_mode = mp->format_mode;

	tmp = a_to_u_2_byte(mp->spare_band_size);
	if (dp->fmt_mode > 1 && dp->spare_band_size < 0)
		dp->spare_band_size = tmp;

printf("mode: %d spare_band: %ld\n", mp->format_mode, tmp);

	return (TRUE);
}

EXPORT BOOL
set_sony_params(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	u_char	mode[0x100];
	u_char	cmode[0x100];
	int	len;
	BOOL	mp_save;
	struct	sony_mode_page_20 *mp;
#ifdef	notneeded
	struct	sony_mode_page_20 *cmp;
#endif

	if (force && yes("Abort? "))
		return (FALSE);
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	fillbytes((caddr_t)cmode, sizeof (cmode), '\0');

	if (!get_mode_params(scgp, 0x20, "sony format",
				(u_char *)0, cmode, mode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);

#ifdef	notneeded
	cmp = (struct sony_mode_page_20 *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);
#endif
	mp = (struct sony_mode_page_20 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
	mp_save = save_mp && mp->parsave;
	mp->parsave = 0;

	mp->format_mode = dp->fmt_mode;

	if (mp->format_mode <= 1) {
		fillbytes((char *)&mp->format_type,
			sizeof (*mp) - ((char *)&mp->format_type-(char *)mp),
									'\0');
	} else {
		mp->format_type = 1;
		i_to_4_byte(mp->num_bands, 1);
#ifdef	OLD
		{ int t;				/*XXX aspt ??? */
			t = dp->spt;
			if (dp->tpz != 0)
				t -= dp->aspz/dp->tpz;
			t *= dp->atrk;
			i_to_2_byte(mp->spare_band_size, t);
		}
#else
		i_to_2_byte(mp->spare_band_size, dp->spare_band_size);
#endif
	}

	return (set_mode_params(scgp, "sony format", mode, len, mp_save, dp));
}
