/* @(#)sgrow.c	1.16 21/08/20 Copyr 1985-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)sgrow.c	1.16 21/08/20 Copyr 1985-2021 J. Schilling";
#endif
/*
 *	Check stack growing response on a machine
 *
 *	Copyright (c) 1985-2021 J. Schilling
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
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/utypes.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/nlsdefs.h>

char *options = "help,version";

EXPORT	int	main	__PR((int ac, char ** av));
LOCAL	Intptr_t _ret	__PR((Intptr_t i));
LOCAL	Intptr_t grow	__PR((int i));
LOCAL	void	usage	__PR((int ret));

EXPORT int
main(ac, av)
	int  ac;
	char *av[];
{
	int 	i;
	char	* const *cav = av;
	int 	cac   = ac;
	BOOL	help  = FALSE;
	BOOL	prversion = FALSE;

	save_args(ac, av);

	(void) setlocale(LC_ALL, "");

#ifdef  USE_NLS
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "sgrow"		/* Use this only if it weren't */
#endif
	{ char	*dir;
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#if defined(PROTOTYPES) && defined(INS_BASE)
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
	}
#endif 	/* USE_NLS */

	cac--, ++cav;
	if (getallargs(&cac, &cav, options, &help, &prversion) == -1) {
		errmsgno(EX_BAD, "Bad option: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);

	if (prversion) {
		gtprintf("Sgrow release %s (%s-%s-%s) Copyright (C) 1985-2021 %s\n",
				"1.16",
				HOST_CPU, HOST_VENDOR, HOST_OS,
				_("Jörg Schilling"));
		exit(0);
	}

	cac = ac, cav = av;
	cac--, ++cav;
	if (getfiles(&cac, &cav, options) == 0) {
		errmsgno(EX_BAD, "Missing Pagecount.\n");
		usage(EX_BAD);
	}
	if (*astoi(cav[0], &i) != '\0') {
		errmsgno(EX_BAD, "not a number: %s.\n", av[1]);
		usage(EX_BAD);
	}
	gtprintf("growing %d * 4k Bytes = %d kBytes.\n", i, i*4);
	grow(i);
	gtprintf("End.\n");
	return (0);
}

/*
 * Hide the fact that we like to return the address of a local variable
 * from the function grow().
 */
LOCAL Intptr_t
_ret(i)
	Intptr_t	i;
{
	return (i);
}

LOCAL Intptr_t
grow(i)
	register int	i;
{
	char s[0x1000-(sizeof (int) + sizeof (char *) + sizeof (char *) + sizeof (int))];
			/* arg i	ret addr	fp		saved reg */

	if (i % 10 == 0) {
		putchar('.'); fflush(stdout);
	}

	if (--i <= 0)
		return (-1);
	grow(i);
	return (_ret((Intptr_t)s));
}

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:   sgrow #\n");
	error("\n         # is n * Pagesize (4k Bytes)\n");
	exit(ret);
}
