/* @(#)calc.c	1.24 19/04/28 Copyright 1985-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)calc.c	1.24 19/04/28 Copyright 1985-2019 J. Schilling";
#endif
/*
 *	Simples Taschenrechnerprogramm
 *
 *	Copyright (c) 1985-2019 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/utypes.h>
#include <schily/standard.h>
#include <schily/schily.h>
#ifdef	FERR_DEBUG
#include <schily/termios.h>
#endif

#define	LLEN	100

#define	LSHIFT	1000
#define	RSHIFT	1001

LOCAL	void	usage	__PR((int));
EXPORT	int	main	__PR((int, char **));
LOCAL	void	prdig	__PR((int));
LOCAL	void	prlldig	__PR((Llong));
LOCAL	void	kommentar __PR((void));
LOCAL	int	xbreakline __PR((char *, char *, char **, int));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage: calc [options]\n");
	error("Options:\n");
	error("	-help	Print this help.\n");
	error("	-version Print version number.\n");
	kommentar();
	exit(exitcode);
	/* NOTREACHED */
}

EXPORT int
main(ac, av)
	int	ac;
	char	**av;
{
	int	cac;
	char	* const* cav;
	BOOL	help = FALSE;
	BOOL	prversion = FALSE;
	char	eingabe[LLEN+1];
	char	*argumente[8];
	int	i;
	int	op = 0;
	char	*opstr;
	Llong	arg1;
	Llong	arg2;
	Llong	ergebnis;
	Llong	rest = (Llong)0;
	int	iarg1;
	int	iarg2;
	int	iergebnis;
	int	irest = 0;

	save_args(ac, av);
	cac = --ac;
	cav = ++av;
	if (getallargs(&cac, &cav, "help,version", &help, &prversion) < 0) {
		errmsgno(EX_BAD, "Bad Option %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prversion) {
		printf("Calc release %s %s (%s-%s-%s) Copyright (C) 1985, 89-91, 1996, 2000-2019 Jörg Schilling\n",
				"1.24", "2019/04/28",
				HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	putchar('?'); flush();
	while (getline(eingabe, LLEN) >= 0 && !streql(eingabe, "quit")) {

		opstr = eingabe;
		while (*opstr == ' ' || *opstr == '\t')
			opstr++;

		/*
		 * optional Kommentarausgabe
		 */
		if (streql(opstr, "help")) {
			kommentar();
			putchar('?'); flush();
			continue;
		}

		/*
		 * Test des Formats und der Argumente
		 */
		i = xbreakline(opstr, " \t", argumente, 5);
		if (*argumente[i-1] == '\0')
			i--;
		switch (i) {

		case 1:
			if (*astoll(argumente[0], &ergebnis) != '\0') {
				printf("'%s' ist keine Zahl!\n?", argumente[0]);
				continue;
			}
			iergebnis	= (int)ergebnis;
			goto print;
		case 2:
			op = *argumente[0];
			if (op != '!' && op != '~') {
				printf("Unzulässiger Operator für: ");
				printf("'op argument1'\n?");
				continue;
			}
			if (*astoll(argumente[1], &arg1) != '\0') {
				printf("'%s' ist keine Zahl!\n?", argumente[1]);
				continue;
			}
			break;
		case 3:
			if (*astoll(argumente[0], &arg1) != '\0') {
				printf("'%s' ist keine Zahl!\n?", argumente[0]);
				continue;
			}
			if (*astoll(argumente[2], &arg2) != '\0') {
				printf("'%s' ist keine Zahl!\n?", argumente[2]);
				continue;
			}
			break;

		default:
			printf("Die Eingabe hat nicht das richtige Format: ");
			printf("'argument1 op argument2'\n?");
			continue;
		}

		/*
		 * Test der Operationssymbole
		 */
		op = 0;
		opstr = argumente[1];
		if (i == 2)
			opstr = argumente[0];

		if (streql(opstr, "<<")) {
			op = LSHIFT;
		} else if (streql(opstr, ">>")) {
			op = RSHIFT;
		} else if (opstr[1] != '\0') {
			printf("Operationssymbole sind einstellig. Falsche Eingabe!\n?");
			continue;
		} else if (!op) {
			op = *opstr;
		}

		i = 0;
		iergebnis	= (int)ergebnis;
		iarg1		= (int)arg1;
		iarg2		= (int)arg2;

		switch (op) {

		case '+':
			ergebnis = arg1 + arg2;
			iergebnis = iarg1 + iarg2;
			break;
		case '-':
			ergebnis = arg1 - arg2;
			iergebnis = iarg1 - iarg2;
			break;
		case '*':
			ergebnis = arg1 * arg2;
			iergebnis = iarg1 * iarg2;
			break;
		case LSHIFT:
			ergebnis = (Ullong)arg1 << arg2;
			iergebnis = (unsigned)iarg1 << iarg2;
			break;
		case RSHIFT:
			ergebnis = (Ullong)arg1 >> arg2;
			iergebnis = (unsigned)iarg1 >> iarg2;
			break;
		case '^':
			ergebnis = arg1 ^ arg2;
			iergebnis = iarg1 ^ iarg2;
			break;
		case '&':
			ergebnis = arg1 & arg2;
			iergebnis = iarg1 & iarg2;
			break;
		case '|':
			ergebnis = arg1 | arg2;
			iergebnis = iarg1 | iarg2;
			break;
		case '!':
			ergebnis = !arg1;
			iergebnis = !iarg1;
			break;
		case '~':
			ergebnis = ~arg1;
			iergebnis = ~iarg1;
			break;
		case '%':
		case '/': if (arg2 == 0) {
				printf("Division durch Null ist unzulaessig.\n?");
				i = 1;
				break;
			} else {
				/*
				 * 9223372036854775808 /  322122547200
				 * liefert eine Integer(32) Division durch 0
				 */
				ergebnis = arg1 / arg2;
				rest = arg1 % arg2;
				if (iarg2 == 0) {
					/*
					 * Alle unteren 32 Bit sind 0
					 * Division durch Null verhindern.
					 */
					iergebnis = irest = 0;
				} else {
					iergebnis = iarg1 / iarg2;
					irest = iarg1 % iarg2;
				}

				if (op == '%') {
					ergebnis = rest;
					iergebnis = irest;
				}
				break;
			}

		default:
			printf("Unzulaessiger Operator!\n?");
			i = 1;
			break;
		}
		if (i == 1)
			continue;

print:
		/*
		 * Ausgabe
		 */
		prdig(iergebnis);
		if (op == '/') {
			printf("\nRest (dezimal): %d\n", irest);
			prdig(irest);
		}
		putchar('\n');

		prlldig(ergebnis);
		if (op == '/') {
			printf("\nRest (dezimal): %lld\n", rest);
			prlldig(rest);
		}
		putchar('\n');

		putchar('?'); flush();
	}
	if (ferror(stdin)) {
#ifdef	FERR_DEBUG
		pid_t	pgrp;
		ioctl(STDIN_FILENO, TIOCGPGRP, (char *)&pgrp);
		errmsg("Read error on stdin. pid %ld pgrp %ld tty pgrp %ld\n",
			(long)getpid(), (long)getpgid(0), (long)pgrp);
#else
		errmsg("Read error on stdin.\n");
#endif
	}
	return (0);
}

LOCAL void
prdig(n)
	int	n;
{
	register int	i;

	printf(" = %d   %u   0%o   0x%x\n = ", n, n, n, n);
	if (n < 0)
		putchar('1');
	else
		putchar('0');
	for (i = 1; i <= 31; i++) {
		/*
		 * New compilers like to make left shifting signed vars illegal
		 */
		n = (int)(((unsigned int)n) << 1);
		if (n < 0)
			putchar('1');
		else
			putchar('0');
		if (i%4 == 3)
			putchar(' ');
	}
}

LOCAL void
prlldig(n)
	Llong	n;
{
	register int	i;

	printf(" = %lld   %llu   0%llo   0x%llx\n = ", n, n, n, n);
	if (n < 0)
		putchar('1');
	else
		putchar('0');
	for (i = 1; i <= 63; i++) {
		/*
		 * New compilers like to make left shifting signed vars illegal
		 */
		n = (Llong)(((ULlong)n) << 1);
		if (n < 0)
			putchar('1');
		else
			putchar('0');
		if (i%4 == 3)
			putchar(' ');
	}
}

LOCAL void
kommentar()
{
	error("                    Taschenrechnerprogramm\n");
	error("                    ======================\n");
	error("Das Programm wird verlassen durch die Eingabe: 'QUIT'\n");
	error("Es kann jeweils eine binäre Operation aus {+,-,*,/,%%,<<,>>,^,&,|}\n");
	error("oder eine unäre Operation aus {~,!} ausgeführt werden. Eine einzelne\n");
	error("Zahl wird wie eine unäre Operation behandelt. Als Eingabe sind nur\n");
	error("integer-Werte zugelassen, aber es ist egal, ob sie dezimal, oktal\n");
	error("oder hexadezimal kodiert sind; auch verschiedene Kodierungen in einer\n");
	error("Rechnung sind zulässig. Die Ausgabe erfolgt dezimal, oktal,\n");
	error("hexadezimal und binär in zwei Darstellungen: 32 Bit sowie 64 Bit.\n");
	error("'/' liefert zusätzlich den Rest (dezimal) bei ganzzahliger Division.\n");
	error("\n");
	error("***************************************************************\n");
	error("* Die Eingabe muss folgendes Format haben:                    *\n");
	error("*          'argument1 op argument2'  oder                     *\n");
	error("*          'op argument'             oder                     *\n");
	error("*          'argument'                                         *\n");
	error("***************************************************************\n");
}

/*--------------------------------------------------------------------------*/
#include <schily/string.h>

LOCAL int
xbreakline(buf, delim, array, len)
		char	*buf;
	register char	*delim;
	register char	*array[];
	register int	len;
{
	register char	*bp = buf;
	register char	*dp;
	register int	i;
	register int	found;

	for (i = 0, found = 1; i < len; i++) {
		for (dp = bp; *dp != '\0' && strchr(delim, *dp) == NULL; dp++)
			;

		array[i] = bp;
		if (*dp != '\0' && strchr(delim, *dp) != NULL) {
			*dp++ = '\0';
			found++;
		}
		while (*dp != '\0' && strchr(delim, *dp) != NULL)
			dp++;
		bp = dp;
	}
	if (found > len)
		found = len;
	return (found);
}
