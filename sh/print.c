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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)print.c	1.18	06/06/16 SMI"
#endif

/*
 * Copyright 2008-2016 J. Schilling
 *
 * @(#)print.c	1.33 16/01/04 2008-2016 J. Schilling
 */
#ifdef	SCHILY_INCLUDES
#include <schily/mconfig.h>
#endif
#ifndef lint
static	UConst char sccsid[] =
	"@(#)print.c	1.33 16/01/04 2008-2016 J. Schilling";
#endif

/*
 * UNIX shell
 *
 */

#ifdef	SCHILY_INCLUDES
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
#define		NUMBUFLEN	21	/* big enough for 64 bits */
unsigned char numbuf[NUMBUFLEN+1];	/* Add one for sign */

static unsigned char buffer[BUFLEN];
static unsigned char *bufp = buffer;
static int bindex = 0;
static int buffd = 1;

	void	prp	__PR((void));
	void	prs	__PR((unsigned char *as));
	void	prc	__PR((unsigned char c));
	void	prwc	__PR((wchar_t c));
	void	prt	__PR((long t));
	void	prtv	__PR((struct timeval *tp, int digs, int lf));
	void	prn	__PR((int n));
static	void	_itos	__PR((unsigned int n, char *out, size_t	outlen));
	void	itos	__PR((int n));
	void	sitos	__PR((int n));
	int	stoi	__PR((unsigned char *icp));
	int	ltos	__PR((long n));

static int ulltos	__PR((UIntmax_t n));
	void flushb	__PR((void));
	void unprs_buff	__PR((int));
	void prc_buff	__PR((unsigned char c));
	int  prs_buff	__PR((unsigned char *s));
static unsigned char *octal __PR((unsigned char c, unsigned char *ptr));
	void prs_cntl	__PR((unsigned char *s));
	void prull_buff	__PR((UIntmax_t lc));
	void prn_buff	__PR((int n));
	int setb	__PR((int fd));


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
clock2tv(t, tp)
	clock_t		t;
	struct timeval	*tp;
{
	int _hz = HZ;	/* HZ may be a macro to a sysconf() call */

	tp->tv_sec = t / _hz;
	tp->tv_usec = t % _hz;
	if (_hz <= 1000000)
		tp->tv_usec *= 1000000 / _hz;
	else
		tp->tv_usec /= _hz / 1000000;
}

void
prt(t)
	long	t;	/* t is time in clock ticks, not seconds */
{
	struct timeval	tv;

	clock2tv(t, &tv);
	prtv(&tv, 3, TRUE);
}

static int	divs[7] = { 1000000, 100000, 10000, 1000, 100, 10, 1 };

void
prtv(tp, digs, lf)
	struct timeval	*tp;
	int		digs;
	int		lf;	/* Long format */
{
	int s, hr, min, sec, frac;

	if (digs < 0)
		digs = 3;
	if (digs > 6)
		digs = 6;
	frac = tp->tv_usec / divs[digs];
	s = tp->tv_sec;
	if (lf) {
		sec = s % 60;	/* Pure seconds		*/
		s /= 60;	/* s now holds minutes	*/
		min = s % 60;	/* Pure minutes		*/
		if (lf == 'l')
			min = s;
		hr = 0;

		if ((lf != 'l') && (hr = s / 60) != 0) {
			prn_buff(hr);
			prc_buff(lf == ':' ? ':':'h');
		}
		if (lf == 'l' || hr > 0 || min > 0) {
			if (lf == ':' && min < 10 && hr > 0)
				prc_buff('0');
			prn_buff(min);
			prc_buff(lf == ':' ? ':':'m');
		}
	} else {
		sec = s;
	}
	if (lf == ':' && sec < 10 && tp->tv_sec >= 60)
		prc_buff('0');
	prn_buff(sec);
	if (digs > 0) {
		prc_buff('.');
		itos(frac+1000000);
		prs_buff(numbuf+7-digs);
	}
	if (lf != FALSE && lf != ':')
		prc_buff('s');
}

void
prn(n)
	int	n;
{
	itos(n);

	prs(numbuf);
}

/*
 * Convert unsigned int into "out" buffer.
 */
static void
_itos(n, out, outlen)
	unsigned int	n;
	char	*out;
	size_t	outlen;
{
	unsigned char buf[NUMBUFLEN];
	unsigned char *abuf = &buf[NUMBUFLEN-1];
	unsigned int d;

	*--abuf = (unsigned char)'\0';

	do {
		 *--abuf = (unsigned char)('0' + n - 10 * (d = n / 10));
	} while ((n = d) != 0);

	strncpy(out, (char *)abuf, outlen);
}

/*
 * Convert int into numbuf as if it was an unsigned.
 */
void
itos(n)
	int	n;
{
	_itos(n, (char *)numbuf, sizeof (numbuf));
}

/*
 * Convert signed int into numbuf.
 */
void
sitos(n)
	int	n;
{
	char *np = (char *)numbuf;

	if (n < 0) {
		*np++ = '-';
		_itos(-n, np, sizeof (numbuf) -1);
		return;
	}
	_itos(n, (char *)numbuf, sizeof (numbuf));
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
stosi(icp)
	unsigned char	*icp;
{
	int	sign = 1;

	if (*icp == '-') {
		sign = -1;
		icp++;
	}
	return (sign * stoi(icp));
}

/*
 * Convert signed long
 */
int
sltos(n)
	long	n;
{
	if (n < 0) {
		int i;

		i = ltos(-n);
		numbuf[--i] = '-';
		return (i);
	}
	return (ltos(n));
}

int
ltos(n)
	long	n;
{
	int i;

	numbuf[NUMBUFLEN-1] = '\0';
	for (i = NUMBUFLEN-2; i >= 0; i--) {
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
	numbuf[NUMBUFLEN-1] = '\0';
	for (i = NUMBUFLEN-2; i >= 0; i--) {
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

void
unprs_buff(amt)
	int	amt;
{
	if (!bindex)
		return;
	bindex -= amt;
	if (bindex < 0)
		bindex = 0;
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
		if (buffd == -1) {
			if (bufp+bindex+1 >= brkend) {
				bufp = growstak(bufp+bindex+1);
				bufp -= bindex + 1;
			}
		} else if (bindex + 1 >= BUFLEN) {
			flushb();
		}

		bufp[bindex++] = c;
	} else {
		flushb();
		write(buffd, &c, 1);
	}
}

int
prs_buff(s)
	unsigned char	*s;
{
	int len = length(s) - 1;

	if (buffd == -1) {
		if (bufp+bindex+len >= brkend) {
			bufp = growstak(bufp+bindex+len);
			bufp -= bindex + len;
		}
	} else if (bindex + len >= BUFLEN) {
		flushb();
	}

	if (buffd != -1 && len >= BUFLEN) {
		write(buffd, s, len);
		return (0);
	} else {
		movstr(s, &bufp[bindex]);
		bindex += len;
		return (len);
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
prull_buff(lc)
	UIntmax_t	lc;
{
	prs_buff(&numbuf[ulltos(lc)]);
}

void
prl_buff(l)
	long	l;
{
	prs_buff(&numbuf[ltos(l)]);
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
			bufp = growstak(bufp+bindex+1);
			bufp -= bindex + 1;
		}
		if (bufp[bindex-1]) {
			bufp[bindex++] = 0;
		}
		bufp = endstak(bufp+bindex);
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
