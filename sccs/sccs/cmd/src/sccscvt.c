/* @(#)sccscvt.c	1.12 11/10/13 Copyright 2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)sccscvt.c	1.12 11/10/13 Copyright 2011 J. Schilling";
#endif
/*
 *	Convert a SCCS v4 history file to a SCCS v6 file and vice versa.
 *	When converting from v6 to v4, the extra meta data from the delta table
 *	is kept in special degenerated comment at the beginning of the comment
 *	block.
 *
 *	Copyright (c) 2011 J. Schilling
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

#include <defines.h>
#include <version.h>
#include <had.h>
#include <i18n.h>
#include <schily/utsname.h>

#define	COPY	0
#define	NOCOPY	1

LOCAL	void	usage		__PR((int exitcode));
EXPORT	int	main		__PR((int ac, char *av[]));
LOCAL	void	dodir		__PR((char *name));
LOCAL	void	convert		__PR((char *file));
LOCAL	void	cvtdelt2v4	__PR((struct packet *pkt));
LOCAL	void	cvtdelt2v6	__PR((struct packet *pkt));
LOCAL	void	get_setup	__PR((char *file));
LOCAL	int	get_hash	__PR((int ser));
LOCAL	void	clean_up	__PR((void));

LOCAL	struct utsname	un;
LOCAL	char		*uuname;
LOCAL	struct packet	gpkt;

LOCAL	BOOL	dov6	= -1;
LOCAL	BOOL	keepold;
LOCAL	BOOL	discardv6;

LOCAL void
usage(exitcode)
	int	exitcode;
{
	fprintf(stderr, _("Usage: sccscvt [options] file1..filen\n"));
	fprintf(stderr, _("	-help	Print this help.\n"));
	fprintf(stderr, _("	-version Print version number.\n"));
	fprintf(stderr, _("	-V4	Convert history files to SCCS v4.\n"));
	fprintf(stderr, _("	-V6	Convert history files to SCCS v6.\n"));
	fprintf(stderr, _("	-d	Discard SCCS v6 meta data.\n"));
	fprintf(stderr, _("	-keep,-k Keep original history file as o.file.\n"));
	exit(exitcode);
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	* const *cav;
	char	*opts = "help,V,version,V4%0,V6,d,k,keep";
	BOOL	help = FALSE;
	BOOL	pversion = FALSE;
	int	nargs = 0;

	save_args(ac, av);

	/*
	 * Set locale for all categories.
	 */
	setlocale(LC_ALL, "");

	/*
	 * Set directory to search for general l10n SCCS messages.
	 */
#ifdef	PROTOTYPES
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	    NOGETTEXT(INS_BASE "/ccs/lib/locale/"));
#else
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	    NOGETTEXT("/usr/ccs/lib/locale/"));
#endif

	(void) textdomain(NOGETTEXT("SUNW_SPRO_SCCS"));

	tzset();	/* Set up timezome related vars */

	set_clean_up(clean_up);
	Fflags = FTLEXIT | FTLMSG | FTLCLN;

	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, opts,
			&help, &pversion, &pversion,
			&dov6, &dov6,
			&discardv6,
			&keepold, &keepold) < 0) {
		errmsgno(EX_BAD, _("Bad flag: %s.\n"), cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (pversion) {
		printf(
	_("sccscvt %s-SCCS version %s %s (%s-%s-%s) Copyright (C) 2011 %s\n"),
			PROVIDER,
			VERSION,
			VDATE,
			HOST_CPU, HOST_VENDOR, HOST_OS,
			_("Jörg Schilling"));
		exit(0);
	}
	if (dov6 < 0) {
		errmsgno(EX_BAD, _("Need to specify -V4 or -V6.\n"));
		usage(EX_BAD);
	}

	cac = ac;
	cav = av;

	while (getfiles(&cac, &cav, opts) > 0) {
		struct stat	sb;

		if (stat(cav[0], &sb) >= 0 && S_ISDIR(sb.st_mode))
			dodir(cav[0]);
		else
			convert(cav[0]);
		cac--;
		cav++;
		nargs++;
	}
	if (nargs == 0) {
		errmsgno(EX_BAD, _("Missing arg.\n"));
		usage(EX_BAD);
	}
	return (0);
}

LOCAL void
dodir(name)
	char	*name;
{
	DIR		*dp = opendir(name);
	struct dirent	*d;
	char		*np;
	char		fname[MAXPATHNAME+1];
	char		*base;
	int		len;

	if (dp == NULL) {
		errmsg(_("Cannot open directory '%s'\n"), name);
	}
	strlcpy(fname, name, sizeof (fname));
	base = &fname[strlen(fname)-1];
	if (*base != '/')
		*++base = '/';
	base++;
	len = sizeof (fname) - strlen(fname);
	while ((d = readdir(dp)) != NULL) {
		np = d->d_name;

		if (np[0] != 's' || np[1] != '.' || np[2] == '\0')
			continue;

		strlcpy(base, np, len);
		convert(fname);
	}
	closedir(dp);
}

LOCAL void
convert(file)
	char	*file;
{
	struct stat sbuf;
	char	hash[32];
	char	*xf;

	/*
	 * Set up exception handling for fatal().
	 */
	if (setjmp(Fjmp))
		return;

	if (!sccsfile(file)) {
		errmsgno(EX_BAD, _("%s: not an SCCS file (co1).\n"), file);
		return;
	}

	/*
	 * Init and check for validity of file name but do not open the file.
	 * This prevents us from potentially damaging files with lockit().
	 */
	sinit(&gpkt, file, 0);

	/*
	 * Obtain a lock on the SCCS history file.
	 */
	uname(&un);
	uuname = un.nodename;
	if (lockit(auxf(gpkt.p_file, 'z'),
	    SCCS_LOCK_ATTEMPTS, getpid(), uuname))
		fatal(_("cannot create lock file (cm4)"));

	/*
	 * Open s. file.
	 */
	sinit(&gpkt, file, 1);
	if (dov6) {
		if (gpkt.p_flags & PF_V6) {
			errmsgno(EX_BAD, _("%s: already in SCCS v6 format.\n"),
				file);
			if (gpkt.p_iop) {
				fclose(gpkt.p_iop);
				gpkt.p_iop = NULL;
			}
			return;
		}
		gpkt.p_flags |= PF_V6;
	} else {
		if ((gpkt.p_flags & PF_V6) == 0) {
			errmsgno(EX_BAD, _("%s: already in SCCS v4 format.\n"),
				file);
			if (gpkt.p_iop) {
				fclose(gpkt.p_iop);
				gpkt.p_iop = NULL;
			}
			return;
		}
		gpkt.p_flags &= ~PF_V6;	/* Make sure initial chksum is V4 */
	}

	/*
	 * Write a magic in the expected new format.
	 */
	gpkt.p_upd = 1;
	putmagic(&gpkt, "00000");
	gpkt.p_wrttn = 1;

	get_setup(file);		/* Read delta table for get_hash() */

	/*
	 * The main conversion work happens here.
	 * Convert the delta table.
	 */
	if (dov6)
		cvtdelt2v6(&gpkt);
	else
		cvtdelt2v4(&gpkt);

	/*
	 * Copy user permissions, flags,
	 * v6 flags, v6 extensions and
	 * descruptive user text.
	 */
	flushto(&gpkt, EUSERTXT, COPY);

	/*
	 * Copy interleaved delta block.
	 */
	gpkt.p_chkeof = 1;		/* set EOF is ok */
	while (getline(&gpkt))
		;
	putline(&gpkt, (char *)0);

	/*
	 * Write checksum.
	 */
	if (!dov6)
		gpkt.p_flags &= ~PF_V6;
	sprintf(hash, "%5.5d", gpkt.p_nhash&0xFFFF);
	putmagic(&gpkt, hash);

	/*
	 * Make sure the data is stable in the file on disk.
	 */
	xf = auxf(gpkt.p_file, 'x');
	if (fflush(gpkt.p_xiop) == EOF)
		xmsg(xf, NOGETTEXT("convert"));

	/*
	 * Lots of paranoia here, to try to catch
	 * delayed failure information from NFS.
	 */
#ifdef	HAVE_FSYNC
	if (fsync(fileno(gpkt.p_xiop)) < 0)
		xmsg(xf, NOGETTEXT("convert"));
#endif
	if (fclose(gpkt.p_xiop) == EOF)
		xmsg(xf, NOGETTEXT("flushline"));
	gpkt.p_xiop = NULL;

	stat(gpkt.p_file, &sbuf);
	if (keepold)
		rename(gpkt.p_file, auxf(gpkt.p_file, 'o'));	

	rename(auxf(gpkt.p_file, 'x'), gpkt.p_file);
	chmod(gpkt.p_file, (unsigned int)sbuf.st_mode);

	chown(gpkt.p_file, (unsigned int)sbuf.st_uid,
			(unsigned int)sbuf.st_gid);
	clean_up();
}

LOCAL char	xcomment[] = { CTLCHAR, COMMENTS, '_', '\0' };	/* "^Ac_" */

/*
 * Convert the delta table from a SCCS v6 history file to SCCS v4
 */
LOCAL void
cvtdelt2v4(pkt)
	register struct packet *pkt;
{
	char		line[BUFSIZ];
	struct deltab	dt;

	/*
	 * We need to permit to read and evaluate SCCS v6 time stamps.
	 */
	pkt->p_flags |= PF_V6;

	line[0] = '\0';
	while (getline(pkt) != NULL) {
		if (pkt->p_line[0] != CTLCHAR)
			fmterr(pkt);
		switch (pkt->p_line[1]) {

		case BDELTAB:
			/*
			 * Convert SCCS v6 to SCCS v4 delta line and save the
			 * old line as wrapped degenerated comment.
			 */
			del_ab(pkt->p_line, &dt, pkt);
			del_ba(&dt, line, pkt->p_flags & ~PF_V6);
			putline(pkt, line);
			line[0] = '\0';
			pkt->p_wrttn = 1;
			if (discardv6)
				continue;
			if (dt.d_dtime.dt_zone != DT_NO_ZONE) {
				strcpy(line, xcomment);
				strlcpy(&line[3], &pkt->p_line[1],
							sizeof (line) - 3);
			}
			continue;

		case STATS:
		case INCLUDE:
		case EXCLUDE:
		case IGNORE:
		case MRNUM:
			continue;

		case SIDEXTENS:
			if (discardv6) {
				pkt->p_wrttn = 1;
				continue;
			}
			/*
			 * Keep SCCS v6 extensions as degenerated comment.
			 *
			 * If not yet flushed, flush delta line.
			 */
			if (line[0] != '\0')
				putline(pkt, line);
			line[0] = '\0';
			putline(pkt, xcomment);
			putline(pkt, &pkt->p_line[1]);
			pkt->p_wrttn = 1;
			continue;

		case COMMENTS:
			/*
			 * If not yet flushed, flush delta line.
			 */
			if (line[0] != '\0')
				putline(pkt, line);
			line[0] = '\0';
			continue;

		case EDELTAB:
			continue;

		case BUSERNAM:
			return;

		default:
			fmterr(pkt);
		}
	}
}

/*
 * Convert the delta table from a SCCS v4 history file to SCCS v6
 */
LOCAL void
cvtdelt2v6(pkt)
	register struct packet *pkt;
{
	char		line[BUFSIZ];
#define	MAX_DELT_LINES	1024
	char		*lines[MAX_DELT_LINES];
	int		nlines = 0;
	int		i;
	int		commentstate = 0;
	BOOL		incomment = FALSE;
	BOOL		needthis = FALSE;
	struct deltab	dt;
	struct deltab	dt2;

	while (getline(pkt) != NULL) {
		if (pkt->p_line[0] != CTLCHAR)
			fmterr(pkt);
		switch (pkt->p_line[1]) {

		case BDELTAB:
			/*
			 * Converting a delta line from SCCS v4 to SCCS v6
			 * is harder as we need to save all following lines and
			 * look for old saved SCCS v6 content in degenerated
			 * comment first.
			 */
			commentstate = 0;
			incomment = FALSE;
			del_ab(pkt->p_line, &dt, pkt);
			if (dt.d_dtime.dt_zone != DT_NO_ZONE)
				fmterr(pkt);

			pkt->p_wrttn = 1;
			while (getline(pkt) != NULL) {
				if (pkt->p_line[0] != CTLCHAR)
					fmterr(pkt);
				/*
				 * Stop on first comment line or at the end
				 * of this delta table block.
				 */
				if (pkt->p_line[1] == COMMENTS)
					break;
				if (pkt->p_line[1] == EDELTAB)
					break;
				if (nlines >= MAX_DELT_LINES)
					fatal(_("OUT OF SPACE (ut9)"));
				lines[nlines] = strdup(pkt->p_line);
				if (lines[nlines++] == NULL)
					fatal(_("OUT OF SPACE (ut9)"));
				pkt->p_wrttn = 1;
			}
			/*
			 * If _we_ previously created a degenerated comment,
			 * then the first line is a saved v6 delta line.
			 * The first degenerated comment must be of type 'd'
			 * and the second degenerated comment must be of
			 * type 'S s'.
			 */
			if ((pkt->p_line[1] == COMMENTS) &&
			    (pkt->p_line[2] == '_') &&
			    (pkt->p_line[3] == BDELTAB) &&
			    (pkt->p_line[4] == ' ')) {
				pkt->p_line[2] = CTLCHAR;
				del_ab(&pkt->p_line[2], &dt2, pkt);
				if (dt2.d_dtime.dt_zone != DT_NO_ZONE) {
					dt.d_dtime.dt_zone =
						dt2.d_dtime.dt_zone;
					dt.d_dtime.dt_nsec =
						dt2.d_dtime.dt_nsec;
				} else {
					dt.d_dtime.dt_zone =
						gmtoff(dt.d_dtime.dt_sec);
				}
				commentstate = 1;
				needthis = FALSE;
			} else {
				dt.d_dtime.dt_zone =
					gmtoff(dt.d_dtime.dt_sec);
				needthis = TRUE;
			}
			/*
			 * Regenerate or generate a v6 delta line and write
			 * the remembered other lines.
			 */
			del_ba(&dt, line, pkt->p_flags);
			putline(pkt, line);
			for (i = 0; i < nlines; i++) {
				putline(pkt, lines[i]);
				free(lines[i]);
			}
			nlines = 0;

			if (commentstate == 1) {
				pkt->p_wrttn = 1;
				getline(pkt);
				if ((pkt->p_line[1] == COMMENTS) &&
				    (pkt->p_line[2] == '_') &&
				    (pkt->p_line[3] == SIDEXTENS) &&
				    (pkt->p_line[4] == ' ') &&
				    (pkt->p_line[5] == 's')) {
					commentstate = 2;
				} else if ((pkt->p_line[1] == COMMENTS) &&
				    (pkt->p_line[2] == '_')) {
					commentstate = 3;
				} else {
					needthis = TRUE;
				}
			}
			if (commentstate != 2 && dt.d_type == 'D') {
				pkt->p_ghash = get_hash(dt.d_serial);
				sidext_ba(pkt, &dt);
			}
			if (commentstate >= 2)
				goto dcomment;

			/*
			 * If the first delta comment was not a degenerated
			 * comment from us, write it back unmodified.
			 */
			if (needthis)
				putline(pkt, (char *)0);
			pkt->p_wrttn = 1;
			continue;

		case COMMENTS:
			if (incomment)
				continue;
			/*
			 * Convert v6 extensions saved as degenerated comment
			 * back to v6 extensions.
			 */
		dcomment:
			if ((pkt->p_line[1] == COMMENTS) &&
			    (pkt->p_line[2] == '_') &&
			    (pkt->p_line[3] == SIDEXTENS)) {
				putctl(pkt);
				putline(pkt, &pkt->p_line[3]);
				pkt->p_wrttn = 1;
				continue;
			}
			incomment = TRUE;
			continue;

		case STATS:
		case INCLUDE:
		case EXCLUDE:
		case IGNORE:
		case MRNUM:
			continue;

		case SIDEXTENS:
			fmterr(pkt);
			continue;

		case EDELTAB:
			incomment = FALSE;
			continue;

		case BUSERNAM:
			return;

		default:
			fmterr(pkt);
		}
	}
}

LOCAL struct packet pk2;
LOCAL off_t	get_off;
LOCAL int	slnno;

LOCAL void
get_setup(file)
	char	*file;
{
	struct stats stats;

	sinit(&pk2, file, 1);

	pk2.p_stdout = stderr;
	pk2.p_reopen = 1;
	pk2.p_cutoff = MAX_TIME;

	if ((pk2.p_flags & PF_V6) == 0) 
		pk2.p_flags |= PF_GMT; 

	if (dodelt(&pk2, &stats, (struct sid *) 0, 0) == 0)
		fmterr(&pk2);
	flushto(&pk2, EUSERTXT, NOCOPY);
	get_off = ftell(pk2.p_iop);
	slnno = pk2.p_slnno;
}

LOCAL int
get_hash(ser)
	int	ser;
{
	int	max_ser = maxser(&pk2);

	if (ser > max_ser)
		return (-1);

	fseek(pk2.p_iop, get_off, SEEK_SET);
	pk2.p_slnno = slnno;

	pk2.p_reopen = 1;
	pk2.p_chkeof = 1;
	pk2.p_gotsid = pk2.p_idel[ser].i_sid;
	pk2.p_reqsid = pk2.p_gotsid;

	zero((char *) pk2.p_apply, (max_ser+1)*sizeof(*pk2.p_apply));
	setup(&pk2, ser);

	pk2.p_ghash = 0;
	while (readmod(&pk2))
		;
	return (pk2.p_ghash & 0xFFFF);
}



LOCAL void
clean_up()
{
	uname(&un);
	uuname = un.nodename;
	if (mylock(auxf(gpkt.p_file, 'z'), getpid(), uuname)) {
		if (gpkt.p_iop) {
			fclose(gpkt.p_iop);
			gpkt.p_iop = NULL;
		}
		if (gpkt.p_xiop) {
			fclose(gpkt.p_xiop);
			gpkt.p_xiop = NULL;
			unlink(auxf(gpkt.p_file, 'x'));
		}
		if (pk2.p_iop) {
			fclose(pk2.p_iop);
			pk2.p_iop = NULL;
		}
		xrm(&gpkt);
		ffreeall();
		unlockit(auxf(gpkt.p_file, 'z'), getpid(), uuname);
	}
}
