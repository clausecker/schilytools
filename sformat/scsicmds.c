/* @(#)scsicmds.c	1.59 08/12/22 Copyright 1988-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)scsicmds.c	1.59 08/12/22 Copyright 1988-2008 J. Schilling";
#endif
/*
 *	SCSI commands for sformat program
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
#ifdef	HAVE_SYS_PARAM_H
#include <sys/param.h>	/* Include various defs needed with some OS */
#endif
#include <stdio.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <signal.h>
#include <schily/errno.h>
#include <schily/stdlib.h>
#include <schily/intcvt.h>
#include <schily/time.h>
#include <schily/ioctl.h>
#include <schily/schily.h>
#include <schily/libport.h>

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"

#ifdef	FMT
#include "fmt.h"
#else
extern	void	print_defect_list	__PR((struct scsi_def_list *l));

extern	BOOL	getlong			__PR((char *, long *, long, long));
extern	BOOL	getint			__PR((char *, int *, int, int));
extern	BOOL	yes			__PR((char *, ...)) __printflike__(1, 2);
#endif

extern	char	*Sbuf;
extern	int	Sbufsize;

#define	G0_MAXADDR	0x1FFFFFL

EXPORT	BOOL	unit_ready		__PR((SCSI *scgp));
EXPORT	BOOL	wait_unit_ready		__PR((SCSI *scgp, int secs));
EXPORT	int	test_unit_ready		__PR((SCSI *scgp));
EXPORT	int	rezero_unit		__PR((SCSI *scgp));
EXPORT	int	mode_select		__PR((SCSI *scgp, u_char *dp, int cnt, int smp, int pf));
EXPORT	int	mode_sense		__PR((SCSI *scgp, u_char *dp, int cnt, int page, int pcf));
EXPORT	int	format_unit		__PR((SCSI *scgp, struct scsi_format_data *fmt,
						int ndefects, int list_format,
						int dmdl, int dgdl, int interlv,
						int pattern, int timeout));
EXPORT	int	inquiry			__PR((SCSI *scgp, caddr_t bp, int cnt));
EXPORT	int	read_capacity		__PR((SCSI *scgp));
EXPORT	int	reassign_block		__PR((SCSI *scgp, struct scsi_def_list *bad, int nbad));
EXPORT	int	read_scsi		__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
EXPORT	int	read_g0			__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
EXPORT	int	read_g1			__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
EXPORT	int	write_scsi		__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
EXPORT	int	write_g0		__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
EXPORT	int	write_g1		__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
EXPORT	int	seek_scsi		__PR((SCSI *scgp, long addr));
EXPORT	int	seek_g0			__PR((SCSI *scgp, long addr));
EXPORT	int	seek_g1			__PR((SCSI *scgp, long addr));
EXPORT	int	translate		__PR((SCSI *scgp, struct scsi_def_bfi *dp, long lba));
EXPORT	int	qic02			__PR((SCSI *scgp, int cmd));
EXPORT	int	start_stop_unit		__PR((SCSI *scgp, int flg));
EXPORT	int	receive_diagnostic	__PR((SCSI *scgp, caddr_t bp, int len));
EXPORT	int	send_diagnostic		__PR((SCSI *scgp, caddr_t bp, int len));
EXPORT	int	write_verify		__PR((SCSI *scgp, caddr_t bp, long start, int count, long *bad_block));
EXPORT	int	verify			__PR((SCSI *scgp, long start, int count, long *bad_block));
EXPORT	int	write_verify_split	__PR((SCSI *scgp, caddr_t bp, long start, int count, long *bad_block));
EXPORT	int	read_defect_list	__PR((SCSI *scgp, int list_type, int list_format, BOOL print));
EXPORT	void	xclear_phys_null	__PR((SCSI *scgp));
EXPORT	void	clear_phys_null		__PR((SCSI *scgp));
EXPORT	void	read_phys_null		__PR((SCSI *scgp, BOOL ccs));
EXPORT	void	md21_read_phys_null	__PR((SCSI *scgp));
EXPORT	void	ccs_read_phys_null	__PR((SCSI *scgp));
EXPORT	void	damage_phys_blk		__PR((SCSI *scgp));

EXPORT BOOL
unit_ready(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (test_unit_ready(scgp) >= 0)		/* alles OK */
		return (TRUE);
	else if (scmd->error >= SCG_FATAL)	/* nicht selektierbar */
		return (FALSE);

	if (scg_sense_key(scgp) == SC_UNIT_ATTENTION) {
		if (test_unit_ready(scgp) >= 0)	/* alles OK */
			return (TRUE);
	}
	if ((scg_cmd_status(scgp) & ST_BUSY) != 0) {
		/*
		 * Busy/reservation_conflict
		 */
		usleep(500000);
		if (test_unit_ready(scgp) >= 0)	/* alles OK */
			return (TRUE);
	}
	if (scg_sense_key(scgp) == -1) {	/* non extended Sense */
		if (scg_sense_code(scgp) == 4)	/* NOT_READY */
			return (FALSE);
		return (TRUE);
	}
						/* FALSE wenn NOT_READY */
	return (scg_sense_key(scgp) != SC_NOT_READY);
}

EXPORT BOOL
wait_unit_ready(scgp, secs)
	SCSI	*scgp;
	int	secs;
{
	int	i;
	int	c;
	int	k;
	int	ret;

	scgp->silent++;
	ret = test_unit_ready(scgp);		/* eat up unit attention */
	if (ret < 0)
		ret = test_unit_ready(scgp);	/* got power on condition? */
	scgp->silent--;

	if (ret >= 0)				/* success that's enough */
		return (TRUE);

	scgp->silent++;
	for (i = 0; i < secs && (ret = test_unit_ready(scgp)) < 0; i++) {
		if (scgp->scmd->scb.busy != 0) {
			sleep(1);
			continue;
		}
		c = scg_sense_code(scgp);
		k = scg_sense_key(scgp);
		if (k == SC_NOT_READY && (c == 0x3A || c == 0x30)) {
			if (scgp->silent <= 1)
				scg_printerr(scgp);
			scgp->silent--;
			return (FALSE);
		}
		sleep(1);
	}
	scgp->silent--;
	if (ret < 0)
		return (FALSE);
	return (TRUE);
}

EXPORT int
test_unit_ready(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA | (scgp->silent ? SCG_SILENT:0);
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_TEST_UNIT_READY;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);

	scgp->cmdname = "test unit ready";

	return (scg_cmd(scgp));
}

EXPORT int
rezero_unit(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_REZERO_UNIT;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);

	scgp->cmdname = "rezero unit";

	return (scg_cmd(scgp));
}

EXPORT int
mode_select(scgp, dp, cnt, smp, pf)
	SCSI	*scgp;
	u_char	*dp;
	int	cnt;
	int	smp;
	int	pf;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)dp;
	scmd->size = cnt;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_MODE_SELECT;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.high_addr = smp ? 1 : 0 | pf ? 0x10 : 0;
	scmd->cdb.g0_cdb.count = cnt;

	fprintf(stderr, "%s ", smp?"Save":"Set ");
	scg_prbytes("Mode Parameters", dp, cnt);

	scgp->cmdname = "mode select";

	return (scg_cmd(scgp));
}

EXPORT int
mode_sense(scgp, dp, cnt, page, pcf)
	SCSI	*scgp;
	u_char	*dp;
	int	cnt;
	int	page;
	int	pcf;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)dp;
	scmd->size = 0xFF;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_MODE_SENSE;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
#ifdef	nonono
	scmd->cdb.g0_cdb.high_addr = 1<<4;	/* XXX PCF ???*/
#endif
	scmd->cdb.g0_cdb.mid_addr = (page&0x3F) | ((pcf<<6)&0xC0);
	scmd->cdb.g0_cdb.count = page ? 0xFF : 24;
	scmd->cdb.g0_cdb.count = cnt;

	scgp->cmdname = "mode sense";

	if (scg_cmd(scgp) < 0)
		return (-1);
	if (scgp->debug) scg_prbytes("Mode Sense Data", dp, cnt - scg_getresid(scgp));
	return (0);
}

#define	FMTDAT	0x10
#define	CMPLST	0x08

EXPORT int
format_unit(scgp, fmt, ndefects, list_format, dmdl, dgdl, interlv, pattern, timeout)
	SCSI	*scgp;
	struct	scsi_format_data *fmt;
	int	ndefects;
	int	list_format;
	int	dmdl;		/* disable manufacturers defect list	*/
	int	dgdl;		/* disable grown defect list		*/
	int	interlv;
	int	pattern;
	int	timeout;
{
	int	length = 0;
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (fmt)
		fillbytes((caddr_t)fmt, sizeof (fmt->hd), '\0');
	else if (dmdl || ndefects || list_format || !dgdl)
		raisecond("fmt == 0", 0L);
	if (dmdl) {
		fmt->hd.enable = 1;
		fmt->hd.dmdl = 1;
		fmt->hd.dcert = 1;
	}
	if (fmt) {
		switch (list_format) {

		case SC_DEF_BLOCK:
			length = ndefects * sizeof (fmt->def_block[0]);
			break;
		case SC_DEF_BFI:
			length = ndefects * sizeof (fmt->def_bfi[0]);
			break;
		case SC_DEF_PHYS:
			length = ndefects * sizeof (fmt->def_phys[0]);
			break;
		default:
			raisecond("list_format", 0L);
		}
	}
	i_to_2_byte(fmt->hd.length, length);
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)fmt;
	scmd->size = fmt ? sizeof (fmt->hd)+length /*+1*/ : 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	if (timeout < 0)
		timeout = 24*3600;	/* Kein Timeout XXX kann haengen */
	scmd->timeout = timeout;
	scmd->cdb.g0_cdb.cmd = 0x04;	/* Format Unit */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.high_addr = (fmt?FMTDAT:0)|(dgdl?CMPLST:0)|list_format;
	scmd->cdb.g0_cdb.mid_addr = pattern;
	scmd->cdb.g0_cdb.count = interlv;

	if (scgp->verbose && fmt)
		scg_prbytes("Defect list Header: ", (u_char *)fmt, scmd->size);

	scgp->cmdname = "format unit";

	return (scg_cmd(scgp));
}

#ifdef	OLD
EXPORT int
old_format_unit(scgp, fmt, ndefects)
	SCSI	*scgp;
	struct	scsi_format_data *fmt;
	int	ndefects;
{
	int	length;
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	fillbytes((caddr_t)fmt, sizeof (*fmt), '\0'); /* XXX */
	if (disable_mdl) {
		fmt->hd.enable = 1;
		fmt->hd.dmdl = 1;
		fmt->hd.dcert = 1;
	}
	length = ndefects * sizeof (long);
	i_to_2_byte(fmt->hd.length, length);
	scmd->addr = (caddr_t)fmt;
	scmd->size = sizeof (fmt->hd)+length /*+1*/;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x04;	/* Format Unit */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
#ifdef	nonono
	scmd->cdb.g0_cdb.high_addr = 0x00; /* Archive */
#endif
	scmd->cdb.g0_cdb.high_addr = 0x18;
	scmd->cdb.g0_cdb.count = interleave;

	scg_prbytes("Defect list Header: ", (u_char *)fmt, scmd->size);

	scgp->cmdname = "format unit";

	return (scg_cmd(scgp));
}

EXPORT int
format_unit_ncl(scgp, fmt, ndefects)
	SCSI	*scgp;
	struct	scsi_format_data *fmt;
	int	ndefects;
{
	int	length;
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	fillbytes((caddr_t)fmt, sizeof (*fmt), '\0'); /* XXX */
	if (disable_mdl) {
		fmt->hd.enable = 1;
		fmt->hd.dmdl = 1;
		fmt->hd.dcert = 1;
	}
	length = ndefects * sizeof (long);
	i_to_2_byte(fmt->hd.length, length);
	scmd->addr = (caddr_t)fmt;
	scmd->size = sizeof (fmt->hd)+length /*+1*/;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x04;	/* Format Unit */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.high_addr = 0x10;
	scmd->cdb.g0_cdb.count = interleave;

	scg_prbytes("Defect list Header: ", (u_char *)fmt, scmd->size);

	scgp->cmdname = "format unit";

	return (scg_cmd(scgp));
}
#endif	/* OLD */

EXPORT int
inquiry(scgp, bp, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes(bp, cnt, '\0');
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_INQUIRY;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.count = cnt;

	scgp->cmdname = "inquiry";

	if (scg_cmd(scgp) < 0)
		return (-1);
	if (scgp->verbose) {
		scg_prbytes("Inquiry Data: ", (u_char *)bp, cnt - scg_getresid(scgp));
		printf("\n");
	}
	return (0);
}

EXPORT int
read_capacity(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	register struct	scsi_capacity	*cap = scgp->cap;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)cap;
	scmd->size = sizeof (*cap);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x25;	/* Read Capacity */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdblen(&scmd->cdb.g1_cdb, 0); /* Full Media */

	scgp->cmdname = "read capacity";

	if (scg_cmd(scgp) < 0) {
		return (-1);
	} else {
		long	kb;
		long	mb;
		long	prmb;
		double	dkb;
		long	cbsize;
		long	cbaddr;

		/*
		 * c_bsize & c_baddr are signed Int32_t
		 * so we use signed int conversion here.
		 */
		cbsize = a_to_4_byte(&cap->c_bsize);
		cbaddr = a_to_4_byte(&cap->c_baddr);
		cap->c_bsize = cbsize;
		cap->c_baddr = cbaddr;

		if (scgp->silent)
			return (0);

		dkb =  (cap->c_baddr+1.0) * (cap->c_bsize/1024.0);
		kb = dkb;
		mb = dkb / 1024.0;
		prmb = dkb / 1000.0 * 1.024;
		printf("Capacity: %ld Blocks = %ld kBytes = %ld MBytes = %ld prMB\n",
			(long)cap->c_baddr+1, kb, mb, prmb);
		printf("Sectorsize: %ld Bytes\n", (long)cap->c_bsize);
	}
	return (0);
}

EXPORT int
reassign_block(scgp, bad, nbad)
	SCSI	*scgp;
	struct scsi_def_list *bad;
	int	nbad;
{
	int	length;
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	fillbytes((caddr_t)&bad->hd, sizeof (struct scsi_def_header), '\0');
	length = sizeof (long) * nbad;
	i_to_2_byte(bad->hd.length, length);
	scmd->addr = (caddr_t)bad;
	scmd->size = length + sizeof (struct scsi_def_header) /*+1*/;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
/*	scmd->timeout = nbad*20;*/
	scmd->timeout = nbad*200;
	scmd->cdb.g0_cdb.cmd = 0x07;	/* Reassign Block */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);

	scgp->cmdname = "reassign block";

	return (scg_cmd(scgp));
}

EXPORT int
read_scsi(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	if (addr <= G0_MAXADDR && cnt < 256) {
		return (read_g0(scgp, bp, addr, cnt));
	} else {
		return (read_g1(scgp, bp, addr, cnt));
	}
}

EXPORT int
read_g0(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_READ;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	g0_cdbaddr(&scmd->cdb.g0_cdb, addr);
	scmd->cdb.g0_cdb.count = cnt;

	scgp->cmdname = "read_g0";

	return (scg_cmd(scgp));
}

EXPORT int
read_g1(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = SC_EREAD;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read_g1";

	return (scg_cmd(scgp));
}

EXPORT int
write_scsi(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	if (addr <= G0_MAXADDR)
		return (write_g0(scgp, bp, addr, cnt));
	else
		return (write_g1(scgp, bp, addr, cnt));
}

EXPORT int
write_g0(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_WRITE;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	g0_cdbaddr(&scmd->cdb.g0_cdb, addr);
	scmd->cdb.g0_cdb.count = cnt;

	scgp->cmdname = "write_g0";

	return (scg_cmd(scgp));
}

EXPORT int
write_g1(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = SC_EWRITE;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "write_g1";

	return (scg_cmd(scgp));
}

EXPORT int
seek_scsi(scgp, addr)
	SCSI	*scgp;
	long	addr;
{
	if (addr <= G0_MAXADDR)
		return (seek_g0(scgp, addr));
	else
		return (seek_g1(scgp, addr));
}

EXPORT int
seek_g0(scgp, addr)
	SCSI	*scgp;
	long	addr;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x0B;	/* Seek */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	g0_cdbaddr(&scmd->cdb.g0_cdb, addr);

	scgp->cmdname = "seek_g0";

	return (scg_cmd(scgp));
}

EXPORT int
seek_g1(scgp, addr)
	SCSI	*scgp;
	long	addr;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x2B;	/* Seek G1 */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);

	scgp->cmdname = "seek_g1";

	return (scg_cmd(scgp));
}

EXPORT int
translate(scgp, dp, lba)
	SCSI	*scgp;
	struct scsi_def_bfi *dp;
	long	lba;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)dp;
	scmd->size = sizeof (struct scsi_def_bfi);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x0F;		/* Translate (Adaptec) */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	g0_cdbaddr(&scmd->cdb.g0_cdb, lba);

	scgp->cmdname = "translate";

	return (scg_cmd(scgp));
}

EXPORT int
qic02(scgp, cmd)
	SCSI	*scgp;
	int	cmd;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = DEF_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x0D;	/* qic02 Sysgen SC4000 */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.mid_addr = cmd;

	scgp->cmdname = "qic 02";

	return (scg_cmd(scgp));
}

EXPORT int
start_stop_unit(scgp, flg)
	SCSI	*scgp;
	int	flg;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x1B;	/* Start Stop Unit */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.count = flg & 0x1;

	scgp->cmdname = "start/stop unit";

	return (scg_cmd(scgp));
}

EXPORT int
receive_diagnostic(scgp, bp, len)
	SCSI	*scgp;
	caddr_t	bp;
	int	len;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = len;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x1C;	/* Receive Diagostic Results */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.low_addr = (len >> 8) & 0xFF;
	scmd->cdb.g0_cdb.count = len & 0xFF;

	scgp->cmdname = "receive diagnostic";

	return (scg_cmd(scgp));
}

EXPORT int
send_diagnostic(scgp, bp, len)
	SCSI	*scgp;
	caddr_t	bp;
	int	len;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = len;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x1D;	/* Send Diagostic */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.low_addr = (len >> 8) & 0xFF;
	scmd->cdb.g0_cdb.count = len & 0xFF;

	scgp->cmdname = "send diagnostic";
	return (scg_cmd(scgp));
}

EXPORT int
write_verify(scgp, bp, start, count, bad_block)
	SCSI	*scgp;
	caddr_t	bp;
	long	start;
	int	count;
	long	*bad_block;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = count*scgp->cap->c_bsize;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x2E;	/* Write Verify */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, start);
	g1_cdblen(&scmd->cdb.g1_cdb, count);

	scgp->cmdname = "write verify";

	if (scg_cmd(scgp) < 0) {
		if (scmd->sense.code >= 0x70) {	/* extended Sense */
			*bad_block =
				a_to_4_byte(&((struct scsi_ext_sense *)
							&scmd->sense)->info_1);
		} else {
			*bad_block = a_to_u_3_byte(&scmd->sense.high_addr);
		}
		return (-1);
	}
	return (0);
}

EXPORT int
verify(scgp, start, count, bad_block)
	SCSI	*scgp;
	long	start;
	int	count;
	long	*bad_block;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x2F;	/* Verify */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, start);
	g1_cdblen(&scmd->cdb.g1_cdb, count);

	scgp->cmdname = "verify";

	if (scg_cmd(scgp) < 0) {
		if (scmd->sense.code >= 0x70) {	/* extended Sense */
			*bad_block =
				a_to_4_byte(&((struct scsi_ext_sense *)
							&scmd->sense)->info_1);
		} else {
			*bad_block = a_to_u_3_byte(&scmd->sense.high_addr);
		}
		return (-1);
	}
	return (0);
}

/*	beim verify kein bitcompare !	*/
EXPORT int
write_verify_split(scgp, bp, start, count, bad_block)
	SCSI	*scgp;
	caddr_t	bp;
	long	start;
	int	count;
	long	*bad_block;
{
	if (write_scsi(scgp, bp, start, count) < 0) {
		if (scgp->scmd->sense.code >= 0x70) {	/* exetend Sense */
			*bad_block =
				a_to_4_byte(&((struct scsi_ext_sense *)
							&scgp->scmd->sense)->info_1);
		} else {
			*bad_block = a_to_u_3_byte(&scgp->scmd->sense.high_addr);
		}
		return (-1);
	}
	return (verify(scgp, start, count, bad_block));
}

EXPORT int
read_defect_list(scgp, list_type, list_format, print)
	SCSI	*scgp;
	int	list_type;
	int	list_format;
	BOOL	print;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)Sbuf;
	scmd->size = Sbufsize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x37;	/* Read Defect List */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = ((list_type&3) << 3) | (list_format & 7);
	g1_cdblen(&scmd->cdb.g1_cdb, Sbufsize);

	scgp->cmdname = "read defect list";

	if (scg_cmd(scgp) < 0)
		return (-1);
	if (print)
		print_defect_list((struct scsi_def_list *)Sbuf);
	return (0);
}

EXPORT void
xclear_phys_null(scgp /*, bp, addr, cnt*/)
	SCSI	*scgp;
/*
	caddr_t	bp;
	int	addr;
	int	cnt;
*/
{
	caddr_t	bp;
	int	cnt;
	long	baddr = 0L;
	register struct	scg_cmd	*scmd = scgp->scmd;

	bp = Sbuf;
/*	addr = 0;*/
#ifdef	nonono
	addr |= 0x80000000;	/* Physical Block address */
#endif
	cnt = 1;
loop:
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	baddr &= ~((unsigned long)1 << 31);
	getlong("Block Address", &baddr, 0L, 1000000L);
	baddr |= (yes("Physical Address ? ")<<31);
	scmd->addr = bp;
#ifdef	nonono
	scmd->size = cnt*512+6;	/*XXX 6 Bytes ECC */
#endif
	scmd->size = cnt*512+512; /*XXX 512 Bytes ECC ??*/
	fillbytes(bp, scmd->size, '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xEA;	/* Write long */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, baddr);
/*	scmd->cdb.g1_cdb.count = cnt;*/

	scgp->cmdname = "write long";

	/*return*/(void)(scg_cmd(scgp));
	goto loop;
/*	exit(0);*/
}

EXPORT void
clear_phys_null(scgp /*, bp, addr, cnt*/)
	SCSI	*scgp;
/*
	caddr_t	bp;
	int	addr;
	int	cnt;
*/
{
	caddr_t	bp;
	int	cnt;
	long	baddr = 0L;
	long	i;
	long	n;
	register struct	scg_cmd	*scmd = scgp->scmd;

	bp = Sbuf;
/*	addr = 0;*/
#ifdef	nonono
	addr |= 0x80000000;	/* Physical Block address */
#endif
	cnt = 1;

	n = 0;
	getlong("Count", &n, 0L, 1000000L);

	for (i = 0; n == 0 || i < n; i++) {

	if (i == 0 || n == 0) {
		baddr &= ~((unsigned long)1 << 31);
		getlong("Block Address", &baddr, 0L, 1000000L);
		baddr |= (yes("Physical Address ? ")<<31);
	} else {
		baddr += 1;
	}
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
#ifdef	nonono
	scmd->size = cnt*512+6;	/*XXX 6 Bytes ECC */
#endif
	scmd->size = cnt*512+512; /*XXX 512 Bytes ECC ??*/
	fillbytes(bp, scmd->size, '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xEA;	/* Write long */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, baddr);
/*	scmd->cdb.g1_cdb.count = cnt;*/

	scgp->cmdname = "write long";

	/*return*/(void)(scg_cmd(scgp));
	}
	exit(0);
}

EXPORT void
read_phys_null(scgp, ccs)
	SCSI	*scgp;
	BOOL	ccs;
{
	if (ccs)
		ccs_read_phys_null(scgp);
	else
		md21_read_phys_null(scgp);
}

EXPORT void
md21_read_phys_null(scgp /*, bp, addr, cnt*/)
	SCSI	*scgp;
/*
	caddr_t	bp;
	int	addr;
	int	cnt;
*/
{
	caddr_t	bp;
	long	baddr;
	int	cnt;
	register struct	scg_cmd	*scmd = scgp->scmd;

	bp = Sbuf;
	baddr = 0L;
	baddr |= 0x80000000;	/* Physical Block address */
loop:
	cnt = 1;
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	baddr &= ~((unsigned long)1 << 31);
	getlong("Block Address", &baddr, 0L, 1000000L);
	baddr |= (yes("Physical Address ? ")<<31);
	scmd->addr = bp;
	scmd->size = cnt*512+6;	/*XXX 6 Bytes ECC */
	fillbytes(bp, scmd->size, '\0');
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xE8;	/* Read long */
	/* Read long */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, baddr);
/*	scmd->cdb.g1_cdb.count = cnt;*/

	scgp->cmdname = "read long";

	/*return*/ (void)(scg_cmd(scgp));

	printf("Data: %d Bytes\n", scmd->size - scg_getresid(scgp));
	filewrite(stdout, bp, scmd->size - scg_getresid(scgp));
	printf("END\n"); flush();
goto loop;
/*	exit(0);*/
}

EXPORT void
ccs_read_phys_null(scgp /*, bp, addr, cnt*/)
	SCSI	*scgp;
/*
	caddr_t	bp;
	int	addr;
	int	cnt;
*/
{
	caddr_t	bp;
	long	baddr;
	int	cnt = 512+6;
	register struct	scg_cmd	*scmd = scgp->scmd;

	bp = Sbuf;
	baddr = 0L;
	getint("Byte count", &cnt, 0L, 10000L);
loop:
	cnt = 1;
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	baddr &= ~((unsigned long)1 << 31);
	getlong("Block Address", &baddr, 0L, 1000000L);
	scmd->addr = bp;
	scmd->size = cnt*512+512; /*XXX 512 Bytes ECC ??*/
	fillbytes(bp, scmd->size, '\0');
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x3E;	/* CCS Read long */
	/* Read long */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, baddr);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read long";

	/*return*/ (void)(scg_cmd(scgp));

	printf("Data: %d Bytes\n", scmd->size - scg_getresid(scgp));
	filewrite(stdout, bp, scmd->size - scg_getresid(scgp));
	printf("END\n"); flush();
goto loop;
/*	exit(0);*/
}

EXPORT void
damage_phys_blk(scgp /*, bp, addr, cnt*/)
	SCSI	*scgp;
/*
	caddr_t	bp;
	int	addr;
	int	cnt;
*/
{
	caddr_t	bp;
	long	baddr;
	int	cnt;
	register struct	scg_cmd	*scmd = scgp->scmd;

	bp = Sbuf;
	baddr = 0L;
	baddr |= 0x80000000;	/* Physical Block address */
/*loop:*/
	cnt = 1;
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	baddr &= ~((unsigned long)1 << 31);
	getlong("Block Address", &baddr, 0L, 1000000L);
	baddr |= (yes("Physical Address ? ")<<31);
	scmd->addr = bp;
#ifdef	nonono
	scmd->size = cnt*512+6;	/*XXX 6 Bytes ECC */
#endif
	scmd->size = cnt*512+512; /*XXX 512 Bytes ECC ??*/
	fillbytes(bp, scmd->size, '\0');
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xE8;	/* Read long */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, baddr);
/*	scmd->cdb.g1_cdb.count = cnt;*/

	scgp->cmdname = "read long";

	if (scg_cmd(scgp) < 0)
		exit(-1);

	Sbuf[0] &= 0x7F;	/* damage */

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*512+6;	/*XXX*/
	fillbytes(bp, scmd->size, '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xEA;	/* Write long */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, baddr);

	scgp->cmdname = "write long";

	/*return*/(void)(scg_cmd(scgp));
/*goto loop;*/
	exit(0);
}
