/* @(#)strar.c	1.3 17/02/15 Copyright 2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)strar.c	1.3 17/02/15 Copyright 2017 J. Schilling";
#endif
/*
 *	Manage a StreamArchive
 *
 *	Copyright (c) 2017 J. Schilling
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
#include <schily/types.h>
#include <schily/schily.h>
#include <schily/stdlib.h>
#include <schily/strar.h>

LOCAL	BOOL	debug;
	BOOL	cflag;
	BOOL	xflag;
LOCAL	BOOL	tflag;
EXPORT	int	verbose;

EXPORT	BOOL	Ctime;
EXPORT	time_t	now;
EXPORT	time_t	sixmonth;

LOCAL	char	*options = "help,version,debug,c,x,t,v+,f*,list*,nometa,basemeta";

LOCAL	void	usage	__PR((int exitcode));
LOCAL	void	pvers	__PR((void));
EXPORT	int	main	__PR((int ac, char *av[]));
LOCAL	int	create	__PR((strar *info, int ac, char * const av[], FILE *f));
LOCAL	int	extract	__PR((strar *info));
LOCAL	int	list	__PR((strar *info));
LOCAL	FILE	*openlist __PR((char *listfile));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	strar [options] [file1...filen]\n");
	error("Options:\n");
	error("\t-help\t\tprint this online help\n");
	error("\t-version\tprint version number\n");
	error("\t-debug\t\tprint additional debug output\n");
	error("\t-c\t\tcreate archive with named files\n");
	error("\t-x\t\textract archive\n");
	error("\t-t\t\tlist archive\n");
	error("\t-v\t\tincrement verbose level\n");
	error("\tlist=name\tread filenames from named file\n");
	error("\t-nometa\t\tdo not add meta data except path and size\n");
	error("\t-basemeta\tonly add basic meta data: path, mode, size, filetype\n");
	exit(exitcode);
}

LOCAL void
pvers()
{
	printf("strar %s (%s-%s-%s)\n\n", "1.3",
		HOST_CPU, HOST_VENDOR, HOST_OS);
	printf("Copyright (C) 2017 J�rg Schilling\n");
	printf("This is free software; see the source for copying conditions.  There is NO\n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	exit(0);
}

int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	i;
	int	cac;
	char	*const *cav;
	int	fac;
	char	*const *fav;
	BOOL	prvers = FALSE;
	BOOL	help = FALSE;
	BOOL	nometa = FALSE;
	BOOL	basemeta = FALSE;
	FINFO	finfo;
	char	*archive = NULL;
	char	*listfile = NULL;

	save_args(ac, av);
	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, options, &help, &prvers, &debug,
			&cflag, &xflag, &tflag, &verbose, &archive,
			&listfile,
			&nometa, &basemeta) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prvers)
		pvers();

	if ((cflag + xflag + tflag) > 1)
		comerrno(EX_BAD, "Only one of -c -x -l.\n");

	fac = ac;
	fav = av;
	for (i = 0; getfiles(&fac, &fav, options) > 0; i++, fac--, fav++);

	if (cflag && i == 0 && listfile == NULL) {
		comerrno(EX_BAD,
		"Too few arguments; will not create an empty archive..\n");
	}
	if (strar_open(&finfo, archive, 0, cflag ? OM_WRITE : OM_READ) < 0)
		comerr("Cannot open archive.\n");
	if (finfo.f_fp == stdout) {
		finfo.f_list = stderr;
		finfo.f_listname = "stderr";
	} else {
		finfo.f_list = stdout;
		finfo.f_listname = "stdout";
	}
	if (verbose > CMD_VERBOSE)
		verbose = CMD_VERBOSE;
	finfo.f_cmdflags |= verbose;

	if (cflag) {
		FILE	*f = NULL;

		if (listfile)
			f = openlist(listfile);
		finfo.f_cmdflags |= CMD_CREATE;
		if (nometa)
			finfo.f_xflags = 0;
		else if (basemeta)
			finfo.f_xflags = XF_BASE_FILEMETA;
		else
			finfo.f_xflags = XF_ALL_FILEMETA;
		return (create(&finfo, ac, av, f));
	} else if (xflag) {
		finfo.f_cmdflags |= CMD_XTRACT;
		strar_receive(&finfo, extract);
	} else if (tflag) {
		finfo.f_cmdflags |= CMD_LIST;
		strar_receive(&finfo, list);
	} else {
		errmsgno(EX_BAD, "No function specified.\n");
		usage(EX_BAD);
	}
	strar_close(&finfo);
	exit(0);
}

LOCAL int
create(info, ac, av, f)
	strar	*info;
	int	ac;
	char	* const av[];
	FILE	*f;
{

	strar_archtype(info);
	if (f) {
		char	*buf = NULL;
		size_t	len = 0;
		ssize_t	amt;

		while ((amt = getdelim(&buf, &len, '\n', f)) >= 0) {
			if (buf[amt-1] == '\n')
				buf[amt-1] = '\0';
			if (strar_send(info, buf) != 0)
				errmsg("Cannot archive '%s'.\n", buf);
		}
	} else {
		for (; getfiles(&ac, &av, options) > 0; ac--, av++) {
			if (strar_send(info, av[0]) != 0)
				errmsg("Cannot archive '%s'.\n", av[0]);
		}
	}
	strar_eof(info);
	return (0);
}


LOCAL int
extract(info)
	strar	*info;
{
	return (strar_get(info));
}

LOCAL int
list(info)
	strar	*info;
{
	strar_list_file(info);
	if (info->f_size == 0)
		return (0);
	strar_skip(info);
	return (0);
}

LOCAL FILE *
openlist(listfile)
	char	*listfile;
{
	FILE	*listf;

	if (streql(listfile, "-")) {
		listf = stdin;
	} else if ((listf = fileopen(listfile, "r")) == (FILE *)NULL)
		comerr("Cannot open '%s'.\n", listfile);

	return (listf);
}

