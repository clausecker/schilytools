/* @(#)printf.c	1.4 16/07/23 Copyright 2015-2016 J. Schilling */
#include <schily/mconfig.h>
/*
 *	printf builtin
 *
 *	Note that this module needs libschily, but we can use -zlazyload
 *
 *	Libschily is needed because the Bourne Shell does not use stdio
 *	internally and we thus need a printf() like formatter that allows
 *	to use a callback function to output a character. This can be done
 *	using the format() function from libschily.
 *
 *	Copyright (c) 2015-2016 J. Schilling
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

#include "defs.h"
#ifdef DO_SYSPRINTF

static	UConst char sccsid[] =
	"@(#)printf.c	1.4 16/07/23 Copyright 2015-2016 J. Schilling";

#include <schily/errno.h>

#define	LOCAL	static
#define	exit(a)	flushb(); return (a)

#define	DOTSEEN	1
#define	FMINUS	2
#define	IGNSEEN	4

const char printfuse[] = "printf format [string ...]";

LOCAL	int	xprp	__PR((unsigned char *fmt, char *p, int *width));
LOCAL	int	xprc	__PR((unsigned char *fmt, int c, int *width));
LOCAL	int	xpri	__PR((unsigned char *fmt, Intmax_t i, int *width));
LOCAL	char	*gstr	__PR((unsigned char ***appp));
LOCAL	char	gchar	__PR((unsigned char ***appp));
LOCAL	void	grangechk __PR((unsigned char *p, unsigned char *ep));
LOCAL	Intmax_t gintmax __PR((unsigned char ***appp));
LOCAL	UIntmax_t guintmax __PR((unsigned char ***appp));

EXPORT int	bprintf	__PR((const char *form, ...));

LOCAL int
xprp(fmt, p, width)
	unsigned char	*fmt;
	char		*p;
	int		*width;
{
	switch (*width) {
	case 1: return (bprintf(C fmt, p));
	case 2: return (bprintf(C fmt, width[1], p));
	default: return (bprintf(C fmt, width[1], width[2], p));
	}
}

LOCAL int
xprc(fmt, c, width)
	unsigned char	*fmt;
	int		c;
	int		*width;
{
	switch (*width) {
	case 1: return (bprintf(C fmt, c));
	case 2: return (bprintf(C fmt, width[1], c));
	default: return (bprintf(C fmt, width[1], width[2], c));
	}
}

LOCAL int
xpri(fmt, i, width)
	unsigned char	*fmt;
	Intmax_t	i;
	int		*width;
{
	unsigned char	*ostaktop = staktop;
	char	*p = C movstrstak(fmt, locstak());
	char	c = *--p;

	*p++ = 'j';	/* Add maxint_t length modifier */
	*p++ = c;
	staktop = UC p;
	GROWSTAKTOP();
	pushstak('\0');
	staktop = ostaktop;

	switch (*width) {
	case 1: return (bprintf(C stakbot, i));
	case 2: return (bprintf(C stakbot, width[1], i));
	default: return (bprintf(C stakbot, width[1], width[2], i));
	}
}

LOCAL char *
gstr(appp)
	unsigned char	***appp;
{
	if (**appp)
		return (C *(*appp)++);
	return (C nullstr);
}

LOCAL char
gchar(appp)
	unsigned char	***appp;
{
	if (**appp)
		return (**(*appp)++);
	return (0);
}

LOCAL void
grangechk(p, ep)
	unsigned char	*p;
	unsigned char	*ep;
{
	unsigned char	*av0 = UC "printf";

	if (p == ep) {
		bfailure(av0, "expected numeric value: ", p);
	} else if (*ep) {
		bfailure(av0, "not completely converted: ", p);
	}
	if (errno == ERANGE) {
		bfailure(av0, "range error: ", p);
	}
}

LOCAL Intmax_t
gintmax(appp)
	unsigned char	***appp;
{
	if (**appp) {
		unsigned char	*cp = *(*appp)++;
		unsigned char	*ep;
		Llong		val;

		errno = 0;
		if (*cp == '"' || *cp == '\'') {
			ep = ++cp;
			val = *ep;
			if (val)
				ep++;
		} else {
			ep = UC astoll(C cp, &val);
		}
		grangechk(cp, ep);
		return ((Intmax_t)val);
	}
	return ((Intmax_t)0);
}

LOCAL UIntmax_t
guintmax(appp)
	unsigned char	***appp;
{
	if (**appp) {
		unsigned char	*cp = *(*appp)++;
		unsigned char	*ep;
		ULlong		val;

		errno = 0;
		if (*cp == '"' || *cp == '\'') {
			ep = ++cp;
			val = *ep;
			if (val)
				ep++;
		} else {
			ep = UC astoull(C cp, &val);
		}
		grangechk(cp, ep);
		return ((UIntmax_t)val);
	}
	return ((UIntmax_t)0);
}

int
sysprintf(argc, argv)
	int	argc;
	unsigned char	**argv;
{
	int		ind = optskip(argc, argv, printfuse);
	unsigned char	*fmt;
	unsigned char	*per;
	unsigned char	*cp;
	unsigned char	**oargv;
	unsigned char	*av0 = *argv;
	int		len;
	wchar_t		wc;

	if (ind < 0)
		return (ERROR);

	argc -= ind;
	argv += ind;
	fmt = *argv++;
	oargv = argv;
	if (!fmt) {
		/*
		 * No args: give usage.
		 */
		gfailure((unsigned char *)usage, printfuse);
		return (ERROR);
	}
	do {
		(void) mbtowc(NULL, NULL, 0);
		for (cp = fmt; *cp; cp++) {
			int		width[3];
			int		*widthp = &width[1];
			int		pflags;
			int		fldwidth;
			int		signif;
			Llong		val;
			unsigned char	sc;
			unsigned char	fc;

			if ((len = mbtowc(&wc, (char *)cp,
					MB_LEN_MAX)) <= 0) {
				(void) mbtowc(NULL, NULL, 0);
				prc_buff(*cp);
				continue;
			}
			if (wc == '\\') {
				unsigned char	cc;

				cp = escape_char(cp, &cc, FALSE);
				if (cp == NULL) {
					exit(exitval);
				}
				prc_buff(cc);
				continue;
			} else if (wc == '%' && cp[len] == '%') {
				cp += len;
				prc_buff(wc);
				continue;
			} else if (wc != '%') {
	outc:
				for (; len > 0; len--)
					prc_buff(*cp++);
				cp--;
				continue;
			}
			fldwidth = signif = pflags = 0;
			per = cp++;		/* Start of new printf format */
			cp += strspn(C cp, "+- #0"); /* Skip flag characters  */
			{
				unsigned char	*p;

				for (p = &per[1]; p < cp; ) {
					if (*p++ == '-')
						pflags |= FMINUS;
				}
			}
			if (*cp == '*') {
				*widthp++ = fldwidth = gintmax(&argv);
			} else {
				(void) astoll(C cp, &val);
				fldwidth = val;
			}
			if (fldwidth < 0) {
				pflags |= FMINUS;
				fldwidth = -fldwidth;
			}
			cp += strspn(C cp, "*1234567890"); /* Skip fldwitdh  */
			if (*cp == '.') {
				pflags |= DOTSEEN;
				cp++;
				if (*cp == '*') {
					*widthp++ = signif = gintmax(&argv);
				} else {
					(void) astoll(C cp, &val);
					signif = val;
				}
				cp += strspn(C cp, "*1234567890"); /* Sk prec */
			}
			width[0] = widthp - width;
			if ((fc = *cp++) == '\0') {
				cp = per;
				goto outc;
			}
			sc = *cp;
			*cp = '\0';
			switch (fc) {
			case 'b': {
				unsigned char *ostaktop = staktop;
				unsigned char *p = UC gstr(&argv);
				int		pre = 0;
				int		post = 0;

				/*
				 * We cannot simply forward to printf() here as
				 * we need to support "%b" "\0" and printf()
				 * does not handle nul bytes.
				 */
				staktop = locstak();
				for (; *p; p++) {
					if ((len = mbtowc(&wc, (char *)p,
							MB_LEN_MAX)) <= 0) {
						(void) mbtowc(NULL, NULL, 0);
						len = 1;
						wc = *p;
					}
					if (signif > 0 &&
					    (staktop - stakbot + len) > signif)
						break;
					if (wc == '\\') {
						unsigned char	cc;

						p = escape_char(p, &cc, TRUE);
						if (p == NULL) {
							pflags |= IGNSEEN;
							break;
						}
						GROWSTAKTOP();
						pushstak(cc);
					} else {
						for (; len > 0; len--) {
							GROWSTAKTOP();
							pushstak(*p++);
						}
						p--;
					}
				}
				if ((staktop - stakbot) < fldwidth) {
					if (pflags & FMINUS) {
						post = fldwidth -
						    (staktop - stakbot);
					} else {
						pre = fldwidth -
						    (staktop - stakbot);
					}
				}
				while (--pre >= 0)
					prc_buff(' ');
				for (p = stakbot; p < staktop; )
					prc_buff(*p++);
				while (--post >= 0)
					prc_buff(' ');
				staktop = ostaktop;
				if (pflags & IGNSEEN) {
					exit(0);
				}
				break;
			}
			case 'c': {
				char c = gchar(&argv);

				xprc(per, c, width);
				break;
			}
			case 's': {
				char *p = gstr(&argv);

				xprp(per, p, width);
				break;
			}
			case 'd':
			case 'i': {
				Intmax_t	i = gintmax(&argv);

				xpri(per, i, width);
				break;
			}
			case 'o':
			case 'u':
			case 'x':
			case 'X': {
				Intmax_t	i = guintmax(&argv);

				xpri(per, i, width);
				break;
			}
			default:
				bfailure(av0,
					"unknown format specifier: ", --cp);
				*cp = sc;
				exit(ERROR);
			}
			*cp-- = sc;
		}
	/*
	 * If the format consumed any argument, loop over all arguments until
	 * all of them have been useed.
	 */
	} while (oargv != argv && *argv);
	exit(exitval);
}

#include <schily/varargs.h>

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
bprintf(const char *form, ...)
#else
EXPORT int
bprintf(form, va_alist)
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	int	cnt;

#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	/*
	 * Note that this requires libschily, but on Solaris we can
	 * use lazy linking via -zlazyload and avoid to link agaist
	 * libschily as long as the printf(1) builtin is not used.
	 */
	cnt = format((void (*)__PR((char, long)))prc_buff, (long)NULL,
			form, args);
	va_end(args);
	return (cnt);
}

#endif /* DO_SYSBUILTIN */
