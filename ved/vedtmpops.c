/* @(#)vedtmpops.c	1.36 18/08/26 Copyright 1988, 1993-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)vedtmpops.c	1.36 18/08/26 Copyright 1988, 1993-2018 J. Schilling";
#endif
/*
 *	Routines that deal with reading/writing of .vedtmp files
 *
 *	Copyright (c) 1988, 1993-2018 J. Schilling
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

/*
 * Layout of the current .vedtmp file (several lines):
 *
 * |.123456789.123456789.123456789.123456789.123456789.123456789.123456789...
 * |            1
 * |edtmpops.c^@                               393^@            4^@    897773870
 * |filename                           file-offset         column      date
 *
 * Layout of the old .vedtmp file (one line only):
 *
 * |filename^@393^@4^@
 *
 * The new .vedtmp file layout:
 *
 * Line one (new format):
 *
 * -	index		the line # of the of the entry for the current file
 *
 * All subsequent lines (new format):
 *
 * -    filename        the null terminated filename
 *                      (filled with blanks to a minimum of 32 bytes)
 * -    file-offset     decimal file offset 13 bytes field width null terninated
 * -    column          decimal column 13 bytes field width null terminated
 * -    date            decimal UNIX date of last visit 13 bytes field width
 *			this works for more than year 9999
 *
 * The first line contains the offset line # of the current file. The first
 * line with a filename is number one.
 */

#include <schily/time.h>
#include "ved.h"

/*extern	time_t	gftime		__PR((char *file));*/
/*
 * XXX Bei True64 ist time_t int! Unsere Definition ist long.
 * XXX Pruefen ob es keine Probleme gibt.
 */
extern	long	gftime		__PR((char *file));

/*
 * Should go away in the near future.
 */
#define	CHECK_EDTMP

#ifdef	CHECK_EDTMP
LOCAL	Uchar	EDTMP[] = ".EDTMP";
#endif
LOCAL	Uchar	vedtmp[] = ".vedtmp";
LOCAL	Uchar	uvedtmp[20];
LOCAL	long	uid	 = -1;

LOCAL	void	vedtmp_name	__PR((void));
LOCAL	void	write_vedtmp	__PR((ewin_t *wp, FILE *f,
				int (*writefunc)(ewin_t *wp, FILE *  f,
				void *buf, int len, Uchar* name),
				BOOL inedit));
EXPORT	void	put_vedtmp	__PR((ewin_t *wp, BOOL inedit));
LOCAL	int	read_vedtmp	__PR((ewin_t *wp, char *buf, int len,
				int *idxp, time_t *ctimep, BOOL inedit));
EXPORT	BOOL	get_vedtmp	__PR((ewin_t *wp, epos_t *posp, int *colp));
LOCAL	void	cvt_vedtmp	__PR((Uchar *buf, int amt, epos_t *posp,
				int *colp, long *datp));

/*
 * Create a per user unique name for the .vedtmp file
 */
LOCAL void
vedtmp_name()
{
	uid = getuid();
	snprintf(C uvedtmp, sizeof (uvedtmp), "%s.%ld", vedtmp, (long)uid);
}

/*
 * Write the .vedtmp file. This is called when the editor is running.
 */
LOCAL void
write_vedtmp(wp, f, writefunc, inedit)
	ewin_t	*wp;
	FILE	*f;
	int	(*writefunc) __PR((ewin_t *wp, FILE *  f,
				void *buf, int len, Uchar* name));
	BOOL	inedit;
{
	Uchar	str[64];
	int	slen;
	int	idx;
	time_t	checktime;
	int	i;
	BOOL	newent = FALSE;
	char	lbuf[1024];

	read_vedtmp(wp, lbuf, sizeof (lbuf), &idx, &checktime, inedit);

	if (idx == 0) {
		newent = TRUE;
		/*
		 * Current filename is not yet in the .vedtmp file.
		 * Find offset for next entry.
		 */
		for (i = 0; ; i++) {
			if (fgetline(f, lbuf, sizeof (lbuf)) < 0)
				break;
		}
		if (i == 0)	/* .vedtmp is empty */
			i++;
		idx = i;
		fflush(f); fileseek(f, (off_t)0);
		snprintf(C str, sizeof (str), "%13d\n", idx);
		writefunc(wp, f, str, strlen(C str), uvedtmp);
		fflush(f); fileseek(f, filesize(f));
	} else {
		/*
		 * Filename is already in the .vedtmp file.
		 * Skip over the entries before.
		 */
		for (i = 0; i < idx; i++) {
			fgetline(f, lbuf, sizeof (lbuf));
		}
		if (idx < 0) {
			newent = TRUE;
			/*
			 * Convert old type entry to a new one.
			 */
			idx = 1;
			snprintf(C str, sizeof (str), "%13d\n", idx);
			writefunc(wp, f, str, strlen(C str), uvedtmp);
		}
		fflush(f); fileseek(f, filepos(f));
	}
	slen = strlen(C wp->curfile)+1;
	writefunc(wp, f, wp->curfile, slen, uvedtmp);
	slen = 32 - slen;
	if (slen < 0)
		slen = 0;
	if (wp->dot != (epos_t)-1 || newent) {
		struct timeval	tv;
		Llong		tdot = (Llong)wp->dot;
		int		tcol = wp->column;

		if (newent && wp->dot == (epos_t)-1)
			wp->dot = (epos_t)0;
		/*
		 * If wp->dot >= ~ 2**44, we do not like to clobber the
		 * vedtmp file but rather remember the beginning of the file.
		 * So do not write numbers that take more than 13 columns.
		 */
#if	defined(HAVE_LARGEFILES) || defined(HAVE_LONGLONG)
		if (tdot > (Llong)9999999999999LL) {
			tdot = (Llong)0;
			tcol = 0;
		}
#endif
		snprintf(C str, sizeof (str), "%*s%13lld", slen, "", tdot);
		writefunc(wp, f, str, strlen(C str)+1, uvedtmp);
		snprintf(C str, sizeof (str), "%13d", tcol);
		writefunc(wp, f, str, strlen(C str)+1, uvedtmp);
		gettimeofday(&tv, 0);
		snprintf(C str, sizeof (str), "%13ld\n", (long)tv.tv_sec);
		writefunc(wp, f, str, strlen(C str), uvedtmp);
	}
	fflush(f); fileseek(f, (off_t)0);
	snprintf(C str, sizeof (str), "%13d\n", idx);
	writefunc(wp, f, str, strlen(C str), uvedtmp);
	fflush(f);
}

/*
 * Write the .vedtmp file.
 * "inedit" indicates if the editor is in command line mode.
 */
EXPORT void
put_vedtmp(wp, inedit)
	ewin_t	*wp;
	BOOL	inedit;
{
	FILE	*f;
	epos_t	sdot = wp->dot;
	int	scolumn = wp->column;

	if (uid < 0)
		vedtmp_name();
	if (noedtmp)
		return;
	if (!readable(wp->curfile))	/* Don't add filename if it doesn't exist */
		return;

	if (inedit) {
		if ((f = opensyserr(wp, uvedtmp, "crwb")) != NULL) {
			write_vedtmp(wp, f, writebsyserr, inedit);
			fclose(f);
		}
	} else {
		wp->dot = (epos_t)-1;
		wp->column = 0;
		if ((f = openerrmsg(uvedtmp, "crwb")) != NULL) {
			write_vedtmp(wp, f, writeerrmsg, inedit);
			fclose(f);
		}
		wp->dot = sdot;
		wp->column = scolumn;
	}
}

/*
 * Open and read the .vedtmp file
 */
LOCAL int
read_vedtmp(wp, buf, len, idxp, ctimep, inedit)
	ewin_t	*wp;
	char	*buf;
	int	len;
	int	*idxp;
	time_t	*ctimep;
	BOOL	inedit;
{
	FILE	*f;
	Uchar	*tmpname;
	int	cnt = -1;
	off_t	fsize;
	long	idx = 0;
	int	nlen = 0;
	time_t	checktime = (time_t)0;
	long	dat;
	struct timeval tv;
	long	i;

	if (uid < 0)
		vedtmp_name();

	if (idxp != NULL)
		*idxp = 0;
	if (ctimep != NULL)
		*ctimep = checktime;

	if (readable(uvedtmp))
		tmpname = uvedtmp;
#ifdef	CHECK_EDTMP
	else if (readable(EDTMP))
		tmpname = EDTMP;
#endif
	else
		return (0);

	if (inedit)
		f = openerrmsg(tmpname, "rb");
	else
		f = opencomerr(wp, tmpname, "rb");
	if (f == (FILE *)NULL)
		return (-1);

	fsize = filesize(f);
	if (!inedit && fsize > 16384) {
		errmsgno(EX_BAD, "WARNING: %s file size is %lld bytes.\n",
						tmpname, (Llong)fsize);
#ifdef	WARN_SLEEP
		sleep(2);
#endif
	}
	cnt = fgetline(f, buf, len);
	if (cnt < 0) {
		if (ferror(f) && !inedit)
			exitcomerr(wp, "Error reading '%s'\n", tmpname);
		return (-1);
	}
	if (cnt == fsize) {
		/*
		 * Old type data is not terminated with a new line.
		 * Ad fgetline strips off new line chars, cnt must
		 * less than fsize if we are reading a new type file.
		 *
		 * Mark old type entry with idx == -1.
		 */
		if (idxp != NULL)
			*idxp = -1;
		fclose(f);
		if (wp->curfile != NULL && !streql(C wp->curfile, buf))
			return (0);
		return (cnt);
	}
	/*
	 * If no current filename was specified, look for index.
	 */
	if (wp->curfile == NULL && *astol(buf, &idx) != '\0')
		idx = 0L;
	if (wp->curfile != NULL)
		nlen = strlen(C wp->curfile);
	gettimeofday(&tv, 0);
	/*
	 * Search as long as we did not yet scan the whole file.
	 * Remember a possible slot in "idx" even if we did not
	 * yet find the filename we are looking for. If we find
	 * the right filename in the vedtmp file, use it, otherwise
	 * use the first remembered slot that may be overwritten.
	 */
	for (i = 1; ; i++) {
		cnt = fgetline(f, buf, len);
		if (cnt < 0) {
			if (ferror(f)) {
				if (!inedit)
					exitcomerr(wp, "Error reading '%s'\n",
								tmpname);
			}
			break;	/* EOF */
		}
		/*
		 * Current filename was specified, look for match.
		 */
		if (wp->curfile != NULL && streql(C wp->curfile, buf)) {
			idx = i;
			break;
		}
		/*
		 * Filename was not specified,
		 * look for index of last edited file.
		 */
		if (wp->curfile == NULL && idx == i)
			break;

		/*
		 * We need to read all entries to make sure our name
		 * is not in.
		 */
		if (idxp != NULL && idx == 0 && nlen > 0 && nlen < 32) {
/*			time_t	ft;*/
/* XXX gftime() ist z.Zt. long	   */
			long	ft;

			/*
			 * Try to re-use standard 32 byte slots if the files
			 * are not present anymore.
			 * First check time of last visit...
			 */
			cvt_vedtmp(UC buf, cnt, 0, 0, &dat);
#ifdef	DEBUG
			if (0 && !inedit) {
				time_t	t = dat;
				error("Name: '%s' len: %d %.24s %s\r\n",
					buf, (int)strlen(buf),
					ctime(&t),
					((long)tv.tv_sec > (dat+3600*24*30))?"OLD":"NEW");
			}
#endif
			/*
			 * If the file no longer exists, we may use this slot.
			 */
			ft = gftime(buf);
			if (ft == 0)
				goto found;
			/*
			 * If the file exists and this entry is not more than
			 * a month old, keep on searching.
			 */
			if ((long)tv.tv_sec < (dat+3600*24*30))
				continue;

		found:
			/*
			 * If the file name is longer than
			 * 32 chars leave it untouched.
			 */
			if (strlen(buf) >= 32)
				continue;
#ifdef	DEBUG
			if (!inedit) {
				time_t	t = dat;
				error("Name: '%s' len: %d %.24s %s\r\n",
					buf, (int)strlen(buf),
					ctime(&t),
					((long)tv.tv_sec > (dat+3600*24*30))?"OLD":"NEW");
			}
#endif
			/*
			 * We found a free slot that fits.
			 * Remember it and use it in case we have no
			 * filename match.
			 */
			idx = i;
		}
	}
	if (idx > i) {
		/*
		 * File was smaller than expected, nofify caller.
		 */
		cnt = 0;
		*buf = '\0';
	} else if (idxp != NULL) {
		/*
		 * Return line number index for current file name.
		 */
		*idxp = idx;
	}
	fclose(f);
	return (cnt);
}

/*
 * Read the filename, the cursor position and the cursor column
 * from the .vedtmp file
 */
EXPORT BOOL
get_vedtmp(wp, posp, colp)
	ewin_t	*wp;
	epos_t	*posp;
	int	*colp;
{
	int	amt;

	if ((amt = read_vedtmp(wp, C curfname, sizeof (curfname),
						(int *)0, (time_t *)0,
								FALSE)) > 0) {
		cvt_vedtmp(curfname, amt, posp, colp, 0);
		return (TRUE);
	}
	return (FALSE);
}

/*
 * Convert one line from the .vedtmp filem decoding position (offset),
 * column and date of last visit.
 */
LOCAL void
cvt_vedtmp(buf, amt, posp, colp, datp)
	Uchar	*buf;
	int	amt;
	epos_t	*posp;
	int	*colp;
	long	*datp;
{
	Llong	pos;
	int	col;
	long	dat;
	register Uchar	*p;

	p = buf;
	while (*p++)			/* Skip over file name */
		;
	if (p < (buf+amt)) {
		/*
		 * Position (offet) info
		 */
		p = UC astoll(C p, &pos);
		if (*p++ != '\0')
			pos = (epos_t)0;
		if (posp)
			*posp = (epos_t)pos;
	}
	if (p < (buf+amt)) {
		/*
		 * Current cursor column
		 */
		p = UC astoi(C p, &col);
		if (*p++ != '\0')
			col = 0;
		if (colp)
			*colp = col;
	}
	if (p < (buf+amt)) {
		/*
		 * Date of last visit
		 */
		p = UC astol(C p, &dat);
		if (*p++ != '\0')
			dat = 0L;
		if (datp)
			*datp = dat;
	}
}
