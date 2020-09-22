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
 * @(#)doget.c	1.4 20/09/07 Copyright 2015-2018 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)doget.c	1.4 20/09/07 Copyright 2015-2020 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)doget.c"
#pragma ident	"@(#)sccs:lib/comobj/doget.c"
#endif
#include	<defines.h>
#include	<had.h>
#include	<i18n.h>

/*
 * Library version of get(1)
 */
void
doget(afile, gname, ser)
	char		*afile;
	char		*gname;
	int		ser;
{
	struct packet	pk2;
	struct stats	stats;
	char		ohade;

	sinit(&pk2, afile, SI_OPEN);

	pk2.p_stdout = stderr;
	pk2.p_cutoff = MAX_TIME;

	if (dodelt(&pk2, &stats, (struct sid *) 0, 0) == 0)
		fmterr(&pk2);
	finduser(&pk2);
	doflags(&pk2);
	donamedflags(&pk2);
	dometa(&pk2);
	flushto(&pk2, EUSERTXT, FLUSH_NOCOPY);

	pk2.p_chkeof = 1;
	pk2.p_gotsid = pk2.p_idel[ser].i_sid;
	pk2.p_reqsid = pk2.p_gotsid;

	setup(&pk2, ser);
	ohade = HADE;
	HADE = 0;
	idsetup(&pk2, NULL);
	HADE = ohade;

	if (exists(afile) && (S_IEXEC & Statbuf.st_mode)) {
		pk2.p_gout = xfcreat(gname, HADK ?
			((mode_t)0755) : ((mode_t)0555));
	} else {
		pk2.p_gout = xfcreat(gname, HADK ?
			((mode_t)0644) : ((mode_t)0444));
	}

#ifdef	USE_SETVBUF
	setvbuf(pk2.p_gout, NULL, _IOFBF, VBUF_SIZE);
#endif

	pk2.p_ghash = 0;
	if (pk2.p_encoding & EF_UUENCODE) {
		while (readmod(&pk2)) {
			decode(pk2.p_line, pk2.p_gout);
		}
	} else {
		while (readmod(&pk2)) {
			char	*p;

			if (pk2.p_flags & PF_NONL) {
				pk2.p_line[--(pk2.p_line_length)] = '\0';
				pk2.p_line_length -= 2;	/* ^AN */
			}
			p = idsubst(&pk2, pk2.p_lineptr);
			if (p != pk2.p_lineptr) {
				if (fputs(p, pk2.p_gout) == EOF)
					xmsg(gname, NOGETTEXT("get"));
			} else {
				if (fwrite(p, 1, pk2.p_line_length,
				    pk2.p_gout) != pk2.p_line_length)
					xmsg(gname, NOGETTEXT("get"));
			}
			if (pk2.p_flags & PF_NONL) {
				pk2.p_line_length += 2;	/* ^AN */
				pk2.p_line[(pk2.p_line_length)++] = '\n';
			}
		}
	}
	if (fflush(pk2.p_gout) == EOF)
		xmsg(gname, NOGETTEXT("get"));
	/*
	 * Force g-file to disk and verify
	 * that it actually got there.
	 */
#ifdef	HAVE_FSYNC
	if (fsync(fileno(pk2.p_gout)) < 0)
		xmsg(gname, NOGETTEXT("get"));
#endif
	if (fclose(pk2.p_gout) == EOF)
		xmsg(gname, NOGETTEXT("get"));

	sclose(&pk2);
	sfree(&pk2);
}
