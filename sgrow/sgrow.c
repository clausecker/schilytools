/* @(#)sgrow.c	1.9 08/04/22 Copyr 1985 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)sgrow.c	1.9 08/04/22 Copyr 1985 J. Schilling";
#endif
/*
 *	Check stack growung response on a machine
 *
 *	Copyright (c) 1985 J. Schilling
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

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/schily.h>

char *options = "help,version";

EXPORT	int	main	__PR((int ac, char ** av));
LOCAL	int	grow	__PR((int i));
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
	cac--, ++cav;
	if (getallargs(&cac, &cav, options, &help, &prversion) == -1) {
		errmsgno(EX_BAD, "Bad option: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);

	if (prversion) {
		printf("Sgrow release %s (%s-%s-%s) Copyright (C) 1985-2007 Jörg Schilling\n",
				"1.9",
				HOST_CPU, HOST_VENDOR, HOST_OS);
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
	printf("growing %d * 4k Bytes = %d kBytes.\n", i, i*4);
	grow(i);
	printf("End.\n");
	return (0);
}

LOCAL int
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
	return ((int)s);
}

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:   sgrow #\n");
	error("\n         # is n * Pagesize (4k Bytes)\n");
	exit(ret);
}
