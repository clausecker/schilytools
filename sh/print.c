/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)print.c	1.18	06/06/16 SMI"
#endif

/*
 * This file contains modifications Copyright 2008-2009 J. Schilling
 *
 * @(#)print.c	1.13 09/11/01 2008-2009 J. Schilling
 */
#ifdef	SCHILY_BUILD
#include <schily/mconfig.h>
#endif
#ifndef lint
static	UConst char sccsid[] =
	"@(#)print.c	1.13 09/11/01 2008-2009 J. Schilling";
#endif

/*
 * UNIX shell
 *
 */

#ifdef	SCHILY_BUILD
#include	<schily/mconfig.h>
#include	<stdio.h>
#undef	feof
#include	"defs.h"
#include	<schily/param.h>
#include	<schily/wchar.h>
#include	<schily/wctype.h>
#else
#include	"defs.h"
#include	<sys/param.h>
#include	<locale.h>
#include	<wctype.h>	/* iswprint() */
#endif

#define		BUFLEN		256

unsigned char numbuf[21];

static unsigned char buffer[BUFLEN];
static unsigned char *bufp = buffer;
static int bindex = 0;
static int buffd = 1;

	void	prp	__PR((void));
	void	prs	__PR((unsigned char *as));
	void	prc	__PR((unsigned char c));
	void	prwc	__PR((wchar_t c));
	void	prt	__PR((long t));
	void	prn	__PR((int n));
	void	itos	__PR((int n));
	int	stoi	__PR((unsigned char *icp));
	int	ltos	__PR((long n));

static int ulltos	__PR((UIntmax_t n));
	void flushb	__PR((void));
	void prc_buff	__PR((unsigned char c));
	void prs_buff	__PR((unsigned char *s));
static unsigned char *octal __PR((unsigned char c, unsigned char *ptr));
	void prs_cntl	__PR((unsigned char *s));
	void prull_buff	__PR((UIntmax_t lc));
	void prn_buff	__PR((int n));
	int setb	__PR((int fd));


void	prc_buff	__PR((unsigned char c));
void	prs_buff	__PR((unsigned char *s));
void	prn_buff	__PR((int n));
void	prs_cntl	__PR((unsigned char *s));

/*
 * printing and io conversion
 */
void
prp()
{
	if ((flags & prompt) == 0 && cmdadr) {
		prs_cntl(cmdadr);
		prs((unsigned char *)colon);
	}
}

void
prs(as)
	unsigned char	*as;
{
	if (as) {
		write(output, as, length(as) - 1);
	}
}

#ifdef	PROTOTYPES
void
prc(unsigned char c)
#else
void
prc(c)
	unsigned char	c;
#endif
{
	if (c) {
		write(output, &c, 1);
	}
}

#ifdef	PROTOTYPES
void
prwc(wchar_t c)
#else
void
prwc(c)
	wchar_t	c;
#endif
{
	char	mb[MB_LEN_MAX + 1];
	int	len;

	if (c == 0) {
		return;
	}
	if ((len = wctomb(mb, c)) < 0) {
		mb[0] = (unsigned char)c;
		len = 1;
	}
	write(output, mb, len);
}

#ifndef	HZ
#define	HZ	sysconf(_SC_CLK_TCK)
#endif

void
prt(t)
	long	t;
{
	int hr, min, sec;
	int _hz = HZ;

	t += _hz / 2;
	t /= _hz;
	sec = t % 60;
	t /= 60;
	min = t % 60;

	if ((hr = t / 60) != 0) {
		prn_buff(hr);
		prc_buff('h');
	}

	prn_buff(min);
	prc_buff('m');
	prn_buff(sec);
	prc_buff('s');
}

void
prn(n)
	int	n;
{
	itos(n);

	prs(numbuf);
}

void
itos(n)
	int	n;
{
	unsigned char buf[21];
	unsigned char *abuf = &buf[20];
	int d;

	*--abuf = (unsigned char)'\0';

	do {
		 *--abuf = (unsigned char)('0' + n - 10 * (d = n / 10));
	} while ((n = d) != 0);

	strncpy((char *)numbuf, (char *)abuf, sizeof (numbuf));
}

int
stoi(icp)
	unsigned char	*icp;
{
	unsigned char	*cp = icp;
	int	r = 0;
	unsigned char	c;

	while ((c = *cp, digit(c)) && c && r >= 0) {
		r = r * 10 + c - '0';
		cp++;
	}
	if (r < 0 || cp == icp) {
		failed(icp, badnum);
		/* NOTREACHED */
	} else {
		return (r);
	}

	return (-1);		/* Not reached, but keeps GCC happy */
}

int
ltos(n)
	long	n;
{
	int i;

	numbuf[20] = '\0';
	for (i = 19; i >= 0; i--) {
		numbuf[i] = n % 10 + '0';
		if ((n /= 10) == 0) {
			break;
		}
	}
	return (i);
}

static int
ulltos(n)
	UIntmax_t	n;
{
	int i;

	/* The max unsigned long long is 20 characters (+1 for '\0') */
	numbuf[20] = '\0';
	for (i = 19; i >= 0; i--) {
		numbuf[i] = n % 10 + '0';
		if ((n /= 10) == 0) {
			break;
		}
	}
	return (i);
}

void
flushb()
{
	if (bindex) {
		bufp[bindex] = '\0';
		write(buffd, bufp, length(bufp) - 1);
		bindex = 0;
	}
}

#ifdef	PROTOTYPES
void
prc_buff(unsigned char c)
#else
void
prc_buff(c)
	unsigned char	c;
#endif
{
	if (c) {
		if (buffd != -1 && bindex + 1 >= BUFLEN) {
			flushb();
		}

		bufp[bindex++] = c;
	} else {
		flushb();
		write(buffd, &c, 1);
	}
}

void
prs_buff(s)
	unsigned char	*s;
{
	int len = length(s) - 1;

	if (buffd != -1 && bindex + len >= BUFLEN) {
		flushb();
	}

	if (buffd != -1 && len >= BUFLEN) {
		write(buffd, s, len);
	} else {
		movstr(s, &bufp[bindex]);
		bindex += len;
	}
}

#ifdef	PROTOTYPES
static unsigned char *
octal(unsigned char c, unsigned char *ptr)
#else
static unsigned char *
octal(c, ptr)
	unsigned char	c;
	unsigned char	*ptr;
#endif
{
	*ptr++ = '\\';
	*ptr++ = ((unsigned int)c >> 6) + '0';
	*ptr++ = (((unsigned int)c >> 3) & 07) + '0';
	*ptr++ = (c & 07) + '0';
	return (ptr);
}

void
prs_cntl(s)
	unsigned char	*s;
{
	int n;
	wchar_t wc;
	unsigned char *olds = s;
	unsigned char *ptr = bufp;
	wchar_t c;

	(void) mbtowc(NULL, NULL, 0);
	if ((n = mbtowc(&wc, (const char *)s, MB_LEN_MAX)) <= 0) {
		(void) mbtowc(NULL, NULL, 0);
		n = 0;
	}
	if (wc == 0)
		n = 0;
	while (n != 0) {
		if (n < 0) {
			ptr = octal(*s++, ptr);
		} else {
			c = wc;
			s += n;
			if (!iswprint(c)) {
				if (c < '\040' && c > 0) {
					/*
					 * assumes ASCII char
					 * translate a control character
					 * into a printable sequence
					 */
					*ptr++ = '^';
					*ptr++ = (c + 0100);
				} else if (c == 0177) {
					/* '\0177' does not work */
					*ptr++ = '^';
					*ptr++ = '?';
				} else {
					/*
					 * unprintable 8-bit byte sequence
					 * assumes all legal multibyte
					 * sequences are
					 * printable
					 */
					ptr = octal(*olds, ptr);
				}
			} else {
				while (n--) {
					*ptr++ = *olds++;
				}
			}
		}
		if (buffd != -1 && ptr >= &bufp[BUFLEN-4]) {
			*ptr = '\0';
			prs(bufp);
			ptr = bufp;
		}
		olds = s;
		if ((n = mbtowc(&wc, (const char *)s, MB_LEN_MAX)) <= 0) {
			(void) mbtowc(NULL, NULL, 0);
			n = 0;
		}
		if (wc == 0)
			n = 0;
	}
	*ptr = '\0';
	prs(bufp);
}


void
/*prull_buff(u_longlong_t lc)*/
prull_buff(lc)
	UIntmax_t	lc;
{
	prs_buff(&numbuf[ulltos(lc)]);
}

void
prn_buff(n)
	int	n;
{
	itos(n);

	prs_buff(numbuf);
}

int
setb(fd)
	int	fd;
{
	int ofd;

	if ((ofd = buffd) == -1) {
		if (bufp+bindex+1 >= brkend) {
			growstak(bufp+bindex+1);
		}
		if (bufp[bindex-1]) {
			bufp[bindex++] = 0;
		}
		endstak(bufp+bindex);
	} else {
		flushb();
	}
	if ((buffd = fd) == -1) {
		bufp = locstak();
	} else {
		bufp = buffer;
	}
	bindex = 0;
	return (ofd);
}
