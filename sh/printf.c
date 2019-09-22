/* @(#)printf.c	1.19 19/08/29 Copyright 2015-2019 J. Schilling */
#include <schily/mconfig.h>
/*
 *	printf builtin or standalone
 *
 *	Note that in the Bourne Shell builtin variant, this module needs
 *	libschily, but we can use -zlazyload
 *
 *	Libschily is needed because the Bourne Shell does not use stdio
 *	internally and we thus need a printf() like formatter that allows
 *	to use a callback function to output a character. This can be done
 *	using the format() function from libschily.
 *
 *	Copyright (c) 2015-2019 J. Schilling
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

#ifndef	BOURNE_SHELL
#include <defs.h>	/* Allow local defs.h for standalone printf via -I. */
#else
#include "defs.h"
#endif
#ifdef DO_SYSPRINTF

static	UConst char sccsid[] =
	"@(#)printf.c	1.19 19/08/29 Copyright 2015-2019 J. Schilling";

#include <schily/errno.h>
#include <schily/alloca.h>

#ifndef	HAVE_STRTOD
#undef	DO_SYSPRINTF_FLOAT
#endif
#ifdef	NO_FLOATINGPOINT
#undef	DO_SYSPRINTF_FLOAT
#endif

#define	LOCAL	static
#if	!defined(HAVE_DYN_ARRAYS) && !defined(HAVE_ALLOCA)
#define	exit(a)	flushb(); free(fm); return (a)
#else
#define	exit(a)	flushb(); return (a)
#endif

#define	DOTSEEN	1	/* Did encounter a '.' in format    */
#define	FMINUS	2	/* Did encounter a '-' in format    */
#define	IGNSEEN	4	/* Did encounter a '\c' in argument */

const char printfuse[] = "printf format [string ...]";
const char stardigit[]	= "*1234567890";
const char *digit	= &stardigit[1];

LOCAL	int	xprp	__PR((unsigned char *fmt, char *p, int *width));
LOCAL	int	xprc	__PR((unsigned char *fmt, int c, int *width));
LOCAL	int	xpri	__PR((unsigned char *fmt, Intmax_t i, int *width));
LOCAL	Uchar	*intfmt	__PR((unsigned char *fmt, int c));
#ifdef	DO_SYSPRINTF_FLOAT
LOCAL	int	xprd	__PR((unsigned char *fmt, double d, int *width));
#endif
LOCAL	char	*gstr	__PR((unsigned char ***appp));
LOCAL	char	gchar	__PR((unsigned char ***appp));
LOCAL	void	grangechk __PR((unsigned char *p, unsigned char *ep));
LOCAL	Intmax_t strtoc	__PR((unsigned char **epp));
LOCAL	Intmax_t gintmax __PR((unsigned char ***appp));
LOCAL	UIntmax_t guintmax __PR((unsigned char ***appp));
#ifdef	DO_SYSPRINTF_FLOAT
LOCAL	double	gdouble __PR((unsigned char ***appp));
#endif

EXPORT int	bprintf	__PR((const char *form, ...));

/*
 * Low level support for %s format.
 */
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

/*
 * Low level support for %s format.
 */
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

/*
 * Low level support for %[diouxX] format.
 * We need to modify the format for Intmax_t.
 */
LOCAL int
xpri(fmt, i, width)
	unsigned char	*fmt;
	Intmax_t	i;
	int		*width;
{
	switch (*width) {
	case 1: return (bprintf(C fmt, i));
	case 2: return (bprintf(C fmt, width[1], i));
	default: return (bprintf(C fmt, width[1], width[2], i));
	}
}

/*
 * Modify the integer format strings to include the 'j' length modifier
 * as we deal with integers of type maxint_t.
 */
LOCAL unsigned char *
intfmt(fmt, c)
	unsigned char	*fmt;
	int		c;
{
	UIntptr_t	ostakoff = relstak();	/* Current staktop offset */
	char	*p = C movstrstak(fmt, locstak());

	--p;		/* Backspace over format specifier */
	*p++ = 'j';	/* Add maxint_t length modifier	*/
	*p++ = c;	/* Add format specifier again	*/
	staktop = UC p;
	GROWSTAKTOP();
	pushstak('\0');
	staktop = absstak(ostakoff);

	return (stakbot);			/* locstak() returns stakbot */
}

#ifdef	DO_SYSPRINTF_FLOAT
/*
 * Low level support for %[eEfFgG] format.
 */
LOCAL int
xprd(fmt, d, width)
	unsigned char	*fmt;
	double		d;
	int		*width;
{
	switch (*width) {
	case 1: return (bprintf(C fmt, d));
	case 2: return (bprintf(C fmt, width[1], d));
	default: return (bprintf(C fmt, width[1], width[2], d));
	}
}
#endif

/*
 * Fetch string argument
 */
LOCAL char *
gstr(appp)
	unsigned char	***appp;
{
	if (**appp)
		return (C *(*appp)++);
	return (C nullstr);
}

/*
 * Fetch char argument
 */
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

/*
 * Fetch "c type argument
 */
LOCAL Intmax_t
strtoc(epp)
	unsigned char	**epp;
{
	unsigned char	*ep = *epp;
	Intmax_t	val;
	wchar_t		wc;
	size_t		len = strlen(C ep);

	len = mbtowc(&wc, C ep, len);
	if (wc && len > 0)
		ep += len;
	*epp = ep;
	val = wc;

	return (val);
}

/*
 * Fetch Intmax_t argument
 */
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
			ep = &cp[1];
			val = strtoc(&ep);
		} else {
#ifdef	HAVE_STRTOLL
			val = strtoll(C cp, CP &ep, 0);
#else
			ep = UC astoll(C cp, &val);
#endif
		}
		grangechk(cp, ep);
		return ((Intmax_t)val);
	}
	return ((Intmax_t)0);
}

/*
 * Fetch UIntmax_t argument
 */
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
			ep = &cp[1];
			val = strtoc(&ep);
		} else {
#ifdef	HAVE_STRTOULL
			val = strtoull(C cp, CP &ep, 0);
#else
			ep = UC astoull(C cp, &val);
#endif
		}
		grangechk(cp, ep);
		return ((UIntmax_t)val);
	}
	return ((UIntmax_t)0);
}

#ifdef	DO_SYSPRINTF_FLOAT
/*
 * Fetch double argument
 */
LOCAL double
gdouble(appp)
	unsigned char	***appp;
{
	if (**appp) {
		unsigned char	*cp = *(*appp)++;
		unsigned char	*ep;
		double		val;

		errno = 0;
		if (*cp == '"' || *cp == '\'') {
			ep = &cp[1];
			val = strtoc(&ep);
		} else {
			val = strtod(C cp, CP &ep);
		}
		grangechk(cp, ep);
		return ((double)val);
	}
	return ((double)0.0);
}
#endif

#ifndef	BOURNE_SHELL
int
main(argc, argv)
	int	argc;
	char	**argv;
{
	/*
	 * Need to set this in the standalone version
	 */
	(void) setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "SYS_TEST"	/* Use this only if it weren't */
#endif
	(void) textdomain(TEXT_DOMAIN);

	return (sysprintf(argc, UCP argv));
}
#endif	/* BOURNE_SHELL */

int
sysprintf(argc, argv)
	int	argc;
	unsigned char	**argv;
{
	int		ind = optskip(argc, argv, printfuse);
	unsigned char	*fmt;
	unsigned char	*per;
	unsigned char	*cp;
	unsigned char	**oargv;	/* old argv	*/
	unsigned char	**dargv;	/* $ argv	*/
	unsigned char	**eargv;	/* end argv	*/
	unsigned char	**fargv;	/* format argv	*/
	unsigned char	**maxargv;	/* maxused argv	*/
	unsigned char	*av0 = *argv;
	int		len = strlen((ind < 0 || ind >= argc) ? \
					C argv[0] : C argv[ind]) + 1;
	wchar_t		wc;
#ifdef	HAVE_DYN_ARRAYS
	unsigned char	fm[len];
#else
#ifdef	HAVE_ALLOCA
	unsigned char	*fm = alloca(len);
#else
	unsigned char	*fm = malloc(len);
#endif
#endif
	unsigned char	*pfmt;

	if (ind < 0)		/* Test for missing fmt is below */
		return (ERROR);

	argc -= ind;
	argv += ind;
	fmt = *argv++;
	oargv = argv;
	eargv = &argv[argc-1];
	if (!fmt) {
		/*
		 * No args: give usage.
		 */
		gfailure((unsigned char *)usage, printfuse);
		return (ERROR);
	}
	do {
		maxargv = dargv = argv;

		(void) mbtowc(NULL, NULL, 0);
		for (cp = fmt; *cp; cp++) {
			int		width[3];
			int		*widthp = &width[1];
			int		pflags;
			int		fldwidth;
			int		signif;
			Llong		val;
			unsigned char	fc;
			unsigned char	*p;


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
			fldwidth = pflags = 0;
			signif = -1;
			per = cp++;		/* Start of new printf format */
			pfmt = fm;
			*pfmt++ = '%';

			len = strspn(C cp, digit);
			if (len > 0 && cp[len] == '$') {
				int	n = atoi(C cp);

				if (--n < 0) {
					bfailure(av0, "illegal n$ format: ",
						--cp);
					exit(ERROR);
				}
				if (argv + n > eargv)
					dargv = eargv;
				else
					dargv = &argv[n];
				fargv = dargv;
				if (dargv > maxargv)
					maxargv = dargv;
				cp += len + 1;	/* skip n$ */
			} else {
				fargv = NULL;
			}

			p = cp;
			cp += strspn(C cp, "+- #0"); /* Skip flag characters  */
			while (p < cp) {
				*pfmt++ = *p;
				if (*p++ == '-')
					pflags |= FMINUS;
			}
			p = NULL;
			if (*cp == '*') {
				len = strspn(C ++cp, digit);
				if (len > 0 && cp[len] == '$') {
					int	n = atoi(C cp);

					if (--n < 0 || fargv == NULL) {
						bfailure(av0,
							"illegal n$ format: ",
							per);
						exit(ERROR);
					}
					if (argv + n > eargv)
						dargv = eargv;
					else
						dargv = &argv[n];
					cp += len + 1;	/* skip n$ */
				} else if (fargv != NULL) {
					bfailure(av0, "illegal n$ format: ",
						per);
					exit(ERROR);
				}
				*widthp++ = fldwidth = gintmax(&dargv);
				*pfmt++ = '*';
				if (dargv > maxargv)
					maxargv = dargv;
			} else {
#ifdef	HAVE_STRTOLL
				val = strtoll(C cp, NULL, 10);
#else
				(void) astollb(C cp, &val, 10);
#endif
				fldwidth = val;
				p = cp;
			}
			if (fldwidth < 0) {
				pflags |= FMINUS;
				fldwidth = -fldwidth;
			}
			cp += strspn(C cp, stardigit); /* Skip fldwitdh  */
			if (p) {
				while (p < cp)
					*pfmt++ = *p++;
			}
			if (*cp == '.') {
				pflags |= DOTSEEN;
				cp++;
				*pfmt++ = '.';
				p = NULL;
				if (*cp == '*') {
					len = strspn(C ++cp, digit);
					if (len > 0 && cp[len] == '$') {
						int	n = atoi(C cp);

						if (--n < 0 || fargv == NULL) {
							bfailure(av0,
							"illegal n$ format: ",
							per);
							exit(ERROR);
						}
						if (argv + n > eargv)
							dargv = eargv;
						else
							dargv = &argv[n];
						cp += len + 1;	/* skip n$ */
					} else if (fargv != NULL) {
						bfailure(av0,
							"illegal n$ format: ",
							per);
						exit(ERROR);
					}
					*widthp++ = signif = gintmax(&dargv);
					*pfmt++ = '*';
					if (dargv > maxargv)
						maxargv = dargv;
				} else {
#ifdef	HAVE_STRTOLL
					val = strtoll(C cp, NULL, 10);
#else
					(void) astollb(C cp, &val, 10);
#endif
					signif = val;
					p = cp;
				}
				cp += strspn(C cp, stardigit); /* Sk prec */
				if (p) {
					while (p < cp)
						*pfmt++ = *p++;
				}
			}
			width[0] = widthp - width;
			if ((fc = *cp) == '\0') {
				len = cp - per;
				cp = per;
				goto outc;
			}
			*pfmt++ = fc;
			*pfmt = '\0';
			if (fargv)
				dargv = fargv;

			switch (fc) {
			case 'b': {
				UIntptr_t	ostakoff = relstak();
				int		pre = 0;
				int		post = 0;

				/*
				 * We cannot simply forward to printf() here as
				 * we need to support "%b" "\0" and printf()
				 * does not handle nul bytes.
				 */
				staktop = locstak();
				p = UC gstr(&dargv);
				for (; *p; p++) {
					if ((len = mbtowc(&wc, (char *)p,
							MB_LEN_MAX)) <= 0) {
						(void) mbtowc(NULL, NULL, 0);
						len = 1;
						wc = *p;
					}
					if (signif >= 0 &&
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
				staktop = absstak(ostakoff);
				if (pflags & IGNSEEN) {
					exit(0);
				}
				break;
			}
			case 'c': {
				char c = gchar(&dargv);

				xprc(fm, c, width);
				break;
			}
			case 's': {
				p = UC gstr(&dargv);

				xprp(fm, C p, width);
				break;
			}
			case 'd':
			case 'i': {
				Intmax_t	i = gintmax(&dargv);
				unsigned char	*fp = intfmt(fm, fc);

				xpri(fp, i, width);
				break;
			}
			case 'o':
			case 'u':
			case 'x':
			case 'X': {
				Intmax_t	i = guintmax(&dargv);
				unsigned char	*fp = intfmt(fm, fc);

				xpri(fp, i, width);
				break;
			}
#ifdef	DO_SYSPRINTF_FLOAT
#ifdef	HAVE_PRINTF_A
			case 'a': case 'A':
#endif
			case 'e': case 'E':
			case 'f': case 'F':
			case 'g': case 'G': {
				double	d = gdouble(&dargv);

				xprd(fm, d, width);
				break;
			}
#endif
			default:
				bfailure(av0,
					"unknown format specifier: ", cp);
				exit(ERROR);
			}
			if (dargv > maxargv)
				maxargv = dargv;
		}
		argv = maxargv;
	/*
	 * If the format consumed any argument, loop over all arguments until
	 * all of them have been useed.
	 */
	} while (oargv != argv && *argv);
	exit(exitval);
}

#ifndef	bprintf
#include <schily/varargs.h>

/* VARARGS1 */
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
	cnt = format((void (*)__PR((char, void *)))prc_buff, NULL,
			form, args);
	va_end(args);
	return (cnt);
}
#endif	/* bprintf */

#endif /* DO_SYSBUILTIN */
