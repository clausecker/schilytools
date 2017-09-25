/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)io.c	1.21	08/01/29 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2017 J. Schilling
 *
 * @(#)io.c	1.36 17/09/25 2008-2017 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)io.c	1.36 17/09/25 2008-2017 J. Schilling";
#endif

/*
 * UNIX shell
 */
#ifdef	SCHILY_INCLUDES
#include	"dup.h"
#include	<schily/fcntl.h>
#include	<schily/types.h>
#include	<schily/stat.h>
#include	<schily/errno.h>
#undef	feof
#else
#include	"dup.h"
#include	<stdio.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>
#endif

short topfd;

/* ========	input output and file copying ======== */

	void	initf		__PR((int fd));
	int	estabf		__PR((unsigned char *s));
	void	push		__PR((struct fileblk *af));
	int	pop		__PR((void));
static	void	pushtemp	__PR((int fd, struct tempblk *tb));
	int	poptemp		__PR((void));
	void	chkpipe		__PR((int *pv));
	int	chkopen		__PR((unsigned char *idf, int mode));
	void	renamef		__PR((int f1, int f2));
	int	create		__PR((unsigned char *s, int iof));
	int	tmpfil		__PR((struct tempblk *tb));
	void	copy		__PR((struct ionod *ioparg));
	int	link_iodocs	__PR((struct ionod *i));
static	int	copy_file	__PR((char *fromname, char *toname));
	void	swap_iodoc_nm	__PR((struct ionod *i));
	int	savefd		__PR((int fd));
	void	restore		__PR((int last));

void
initf(fd)
	int	fd;
{
	struct fileblk *f = standin;

	f->fdes = fd;
	f->peekn = 0;
	f->fsiz = ((flags & oneflg) == 0 ? BUFFERSIZE : 1);
	f->fnxt = f->fend = f->fbuf;
	f->nxtoff = f->endoff = 0;
	f->feval = 0;
	f->flin = 1;
	f->feof = FALSE;
}

int
estabf(s)
	unsigned char	*s;
{
	struct fileblk *f;

	(f = standin)->fdes = -1;
	f->fend = length(s) + (f->fnxt = s);
	f->nxtoff = 0;
	f->endoff = length(s);
	f->flin = 1;
	return (f->feof = (s == 0));
}

void
push(af)
	struct fileblk	*af;
{
	struct fileblk *f;

	(f = af)->fstak = standin;
	f->peekn = 0;
	f->feof = 0;
	f->feval = 0;
	standin = f;
}

int
pop()
{
	struct fileblk *f;

	if ((f = standin)->fstak) {
		if (f->fdes >= 0)
			close(f->fdes);
		standin = f->fstak;
		return (TRUE);
	} else
		return (FALSE);
}

struct tempblk *tmpfptr;

static void
pushtemp(fd, tb)
	int		fd;
	struct tempblk	*tb;
{
	tb->fdes = fd;
	tb->fstak = tmpfptr;
	tmpfptr = tb;
}

int
poptemp()
{
	if (tmpfptr) {
		close(tmpfptr->fdes);
		tmpfptr = tmpfptr->fstak;
		return (TRUE);
	} else
		return (FALSE);
}

void
chkpipe(pv)
	int	*pv;
{
	if (pipe(pv) < 0 || pv[INPIPE] < 0 || pv[OTPIPE] < 0)
		error(piperr);
}

int
chkopen(idf, mode)
	unsigned char	*idf;
	int		mode;
{
	int	rc;

	if ((rc = open((char *)idf, mode, 0666)) < 0) {
		failed(idf, badopen);
		/*
		 * Returns only if flags & noexit is set.
		 */
	} else {
		struct stat sb;

		if (fstat(rc, &sb) < 0 || S_ISDIR(sb.st_mode)) {
			close(rc);
			failed(idf, eisdir);
			/*
			 * Returns only if flags & noexit is set.
			 */
			return (-1);
		}
		return (rc);
	}

	return (-1);
}

/*
 * Make f2 be a synonym (including the close-on-exec flag) for f1, which is
 * then closed.  If f2 is descriptor 0, modify the global ioset variable
 * accordingly.
 */
void
renamef(f1, f2)
	int	f1;
	int	f2;
{
#ifdef RES
	if (f1 != f2) {
		dup(f1 | DUPFLG, f2);
		close(f1);
		if (f2 == 0)
			ioset |= 1;
	}
#else
	int	fs;

	if (f1 != f2) {
		/*
		 * This is a hack to work around a bug triggered by
		 * DO_PIPE_PARENT that causes fd #10 to be renamed to fd #1
		 * twice with the result of loosing fd #1 completely.
		 * We currently only have a highly compley testcase that
		 * does not allow to trace for the reason. So it seems to
		 * be the best way to avoid the results of the bug until
		 * we can fix the real reason.
		 */
		if (f2 == 1 && f1 >= 10 && fcntl(f1, F_GETFD, 0) < 0)
			return;

		fs = fcntl(f2, F_GETFD, 0);
		close(f2);
		fcntl(f1, F_DUPFD, f2);
		close(f1);
		if (fs == 1)
			fcntl(f2, F_SETFD, FD_CLOEXEC);
		if (f2 == 0)
			ioset |= 1;
	}
#endif
}

int
create(s, iof)
	unsigned char	*s;
	int		iof;
{
	int	rc;
#ifdef	O_CREAT
	int	omode = O_WRONLY|O_CREAT|O_TRUNC;
#endif
#ifdef	DO_NOCLOBBER
	struct stat statb;

	if ((flags2 & noclobberflg) && (iof & IOCLOB) == 0) {
		if (stat((char *)s, &statb) >= 0) {
			if (S_ISREG(statb.st_mode)) {
				failed(s, eclobber);
				/*
				 * Returns only if flags & noexit is set.
				 */
				return (-1);
			}
		} else {
			statb.st_mode = 0;
		}
#ifdef	O_CREAT
		/*
		 * As we here assume that no plain file of that name exists,
		 * it would be wrong truncate the file.
		 */
		omode &= ~O_TRUNC;
		if (statb.st_mode == 0)
			omode |= O_EXCL;
#endif
	}
#endif
#ifdef	O_CREAT
#if defined(DO_O_APPEND) && defined(O_APPEND)
	if (iof & IOAPP)
		omode |= O_APPEND;
#endif
	if ((rc = open((char *)s, omode, 0666)) < 0) {
#else
	if ((rc = creat((char *)s, 0666)) < 0) {
#endif
		failed(s, badcreate);
		/*
		 * Returns only if flags & noexit is set.
		 */
	} else {
#ifdef	DO_NOCLOBBER
		if ((flags2 & noclobberflg) && (iof & IOCLOB) == 0 &&
		    statb.st_mode != 0) {
			/*
			 * Noclobber is set and a non regular file exists.
			 * Check for a race condition and verify that the
			 * opened file is not a regular file.
			 */
			fstat(rc, &statb);
			if (S_ISREG(statb.st_mode)) {
				close(rc);
				failed(s, eclobber);
				/*
				 * Returns only if flags & noexit is set.
				 */
				return (-1);
			}
		}
#endif
		return (rc);
	}
	return (-1);
}


int
tmpfil(tb)
	struct tempblk	*tb;
{
	int fd;
	int len;
	size_t size_left = TMPOUTSZ - tmpout_offset;

	/* make sure tmp file does not already exist. */
	do {
		len = snprintf((char *)&tmpout[tmpout_offset], size_left,
		    "%u", serial);
		fd = open((char *)tmpout, O_RDWR|O_CREAT|O_EXCL, 0600);
		serial++;
		if ((serial >= UINT_MAX) || (len >= size_left)) {
			/*
			 * We've already cycled through all the possible
			 * numbers or the tmp file name is being
			 * truncated anyway (although TMPOUTSZ should be
			 * big enough), so start over.
			 */
			serial = 0;
			break;
		}
	} while ((fd == -1) && (errno == EEXIST));
	if (fd != -1) {
		pushtemp(fd, tb);
		return (fd);
	} else {
		failed(tmpout, badcreate);
		/* NOTREACHED */
	}
	return (-1);		/* Not reached, but keeps GCC happy */
}

/*
 * set by trim
 */
extern BOOL		nosubst;
#define			CPYSIZ		512

void
copy(ioparg)
	struct ionod	*ioparg;
{
	unsigned char	*cline;		/* Start of current line */
	unsigned char	*clinep;	/* Current add pointer */
	struct ionod	*iop;
	unsigned int	c;
	unsigned char	*ends;
	unsigned char	*start;
	ptrdiff_t	poff;
	int		fd;
	int		i;
	int		stripflg;
	unsigned char	*pc;


	if ((iop = ioparg) != NULL) {
		struct tempblk tb;
		copy(iop->iolst);
		ends = mactrim((unsigned char *)iop->ioname);
		stripflg = iop->iofile & IOSTRIP;
		if (nosubst)
			iop->iofile &= ~IODOC_SUBST;
		fd = tmpfil(&tb);

		if (fndef)
			iop->ioname = (char *)make(tmpout);
		else
			iop->ioname = (char *)cpystak(tmpout);

		iop->iolst = iotemp;
		iotemp = iop;

		cline = clinep = start = locstak();
		poff = 0;
		if (stripflg) {
			iop->iofile &= ~IOSTRIP;
			while (*ends == '\t')
				ends++;
		}
		for (;;) {
			staktop = &clinep[1];
			chkpr();
			if (stakbot != start) {
				poff = stakbot - start;
				start += poff;
				cline += poff;
				clinep += poff;
				poff = 0;
			}
			staktop = start;
			if (nosubst) {
				c = readwc();
				if (stripflg)
					while (c == '\t')
						c = readwc();

				while (!eolchar(c)) {
					pc = readw(c);
					while (*pc) {
						GROWSTAK2(clinep, poff);
						*clinep++ = *pc++;
					}
					c = readwc();
				}
			} else {
				c = nextwc();
				if (stripflg)
					while (c == '\t')
						c = nextwc();

				while (!eolchar(c)) {
					pc = readw(c);
					while (*pc) {
						GROWSTAK2(clinep, poff);
						*clinep++ = *pc++;
					}
					if (c == '\\') {
						pc = readw(readwc());
						/* *pc might be NULL */
						/* BEGIN CSTYLED */
						if (*pc) {
							while (*pc) {
								GROWSTAK2(clinep, poff);
								*clinep++ = *pc++;
							}
						} else {
							GROWSTAK2(clinep, poff);
							*clinep++ = *pc;
						}
						/* END CSTYLED */
					}
					c = nextwc();
				}
			}

			GROWSTAK2(clinep, poff);
			*clinep = 0;
			if (poff) {
				cline += poff;
				start += poff;
				poff = 0;
			}
			if (eof || eq(cline, ends)) {
				if ((i = cline - start) > 0)
					write(fd, start, i);
				break;
			} else {
				GROWSTAK2(clinep, poff);
				*clinep++ = NL;
				if (poff) {
					cline += poff;
					start += poff;
					poff = 0;
				}
			}

			if ((i = clinep - start) < CPYSIZ) {
				cline = clinep;
			} else {
				write(fd, start, i);
				cline = clinep = start;
			}
		}

		/*
		 * Pushed in tmpfil -- bug fix for problem
		 * deleting in-line script.
		 */
		poptemp();
	}
}

int
link_iodocs(i)
	struct ionod	*i;
{
	int r;
	int len;
	int ret = 0;
	size_t size_left = TMPOUTSZ - tmpout_offset;

	while (i) {
		if (i->iofile & IOBARRIER)
			break;
		free(i->iolink);

		/* make sure tmp file does not already exist. */
		do {
			len = snprintf((char *)&tmpout[tmpout_offset],
			    size_left, "%u", serial);
			serial++;
			r = link(i->ioname, (char *)tmpout);
			/* Need to support Haiku */
			if (r == -1 && errno != EEXIST)
				r = copy_file(i->ioname, (char *)tmpout);

			if ((serial >= UINT_MAX) || (len >= size_left)) {
			/*
			 * We've already cycled through all the possible
			 * numbers or the tmp file name is being
			 * truncated anyway, so start over.
			 */
				serial = 0;
				break;
			}
		} while (r == -1 && errno == EEXIST);

		if (r != -1) {
			ret = 1;
			i->iolink = (char *)make(tmpout);
			i = i->iolst;
		} else {
			failed(tmpout, badcreate);
		}

	}
	return (ret);
}

/*
 * If link() fails because it is unimplemented (as on Haiku), we copy the file.
 */
static int
copy_file(fromname, toname)
	char	*fromname;
	char	*toname;
{
	int	fi = open(fromname, O_RDONLY);
	int	fo;
	int	amt;
	char	buf[1024];

	if (fi < 0)
		return (-1);

	fo = open(toname, O_WRONLY|O_CREAT|O_EXCL, 0600);
	if (fo < 0) {
		close(fi);
		return (-1);
	}
	while ((amt = read(fi, buf, sizeof (buf))) > 0) {
		if (write(fo, buf, amt) != amt) {
			amt = -1;
			break;
		}
	}
	close(fi);
	close(fo);
	return (amt);
}

void
swap_iodoc_nm(i)
	struct ionod	*i;
{
	while (i) {
		if (i->iofile & IOBARRIER)
			break;
		free(i->ioname);
		i->ioname = i->iolink;
		i->iolink = 0;

		i = i->iolst;
	}
}

int
savefd(fd)
	int	fd;
{
	int	f;

	f = fcntl(fd, F_DUPFD, 10);
	/* this saved fd should not be found in an exec'ed cmd */
	(void) fcntl(f, F_SETFD, FD_CLOEXEC);
	return (f);
}

void
restore(last)
	int	last;
{
	int	i;
	int	dupfd;

	for (i = topfd - 1; i >= last; i--) {
		if ((dupfd = fdmap[i].dup_fd) > 0)
			renamef(dupfd, fdmap[i].org_fd);
		else
			close(fdmap[i].org_fd);
	}
	topfd = last;
}
