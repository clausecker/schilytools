/* @(#)scsicmds.h	1.4 01/06/21 Copyright 1996 J. Schilling */
/*
 *	Copyright (c) 1996 J. Schilling
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

/*
 * scsicmds.c:
 */
extern	BOOL	unit_ready		__PR((SCSI *scgp));
extern	BOOL	wait_unit_ready		__PR((SCSI *scgp, int secs));
extern	int	test_unit_ready		__PR((SCSI *scgp));
extern	int	rezero_unit		__PR((SCSI *scgp));
extern	int	mode_select		__PR((SCSI *scgp, u_char *dp, int cnt, int smp, int pf));
extern	int	mode_sense		__PR((SCSI *scgp, u_char *dp, int cnt, int page, int pcf));
extern	int	format_unit		__PR((SCSI *scgp, struct scsi_format_data *fmt, int ndefects, int list_format, int dmdl, int dgdl, int interlv, int pattern, int timeout));
extern	int	inquiry			__PR((SCSI *scgp, caddr_t bp, int cnt));
extern	int	read_capacity		__PR((SCSI *scgp));
extern	int	reassign_block		__PR((SCSI *scgp, struct scsi_def_list *bad, int nbad));
extern	int	read_scsi		__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
extern	int	read_g0			__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
extern	int	read_g1			__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
extern	int	write_scsi		__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
extern	int	write_g0		__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
extern	int	write_g1		__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
extern	int	seek_scsi		__PR((SCSI *scgp, long addr));
extern	int	seek_g0			__PR((SCSI *scgp, long addr));
extern	int	seek_g1			__PR((SCSI *scgp, long addr));
extern	int	translate		__PR((SCSI *scgp, struct scsi_def_bfi *dp, long lba));
extern	int	qic02			__PR((SCSI *scgp, int cmd));
extern	int	start_stop_unit		__PR((SCSI *scgp, int flg));
extern	int	receive_diagnostic	__PR((SCSI *scgp, caddr_t bp, int len));
extern	int	send_diagnostic		__PR((SCSI *scgp, caddr_t bp, int len));
extern	int	write_verify		__PR((SCSI *scgp, caddr_t bp, long start, int count, long *bad_block));
extern	int	verify			__PR((SCSI *scgp, long start, int count, long *bad_block));
extern	int	write_verify_split	__PR((SCSI *scgp, caddr_t bp, long start, int count, long *bad_block));
extern	int	read_defect_list	__PR((SCSI *scgp, int list_type, int list_format, BOOL print));
extern	void	xclear_phys_null	__PR((SCSI *scgp));
extern	void	clear_phys_null		__PR((SCSI *scgp));
extern	void	read_phys_null		__PR((SCSI *scgp, int ccs));
extern	void	md21_read_phys_null	__PR((SCSI *scgp));
extern	void	ccs_read_phys_null	__PR((SCSI *scgp));
extern	void	damage_phys_blk		__PR((SCSI *scgp));

