/* @(#)cut.c	1.20 15/06/13 Copyright 1985-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)cut.c	1.20 15/06/13 Copyright 1985-2015 J. Schilling";
#endif
/*
 *	Cut files into fields
 *
 *	Copyright (c) 1985-2015 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* For sys/types.h to make off_t available */
#include <schily/inttypes.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>

#define	NO_NUMBER	-1	/* No number found */

#define	CUT_NONE	0	/* No cut type yet */
#define	CUT_FIELDS	1	/* Cut by fields */
#define	CUT_COLS	2	/* Cut by columns */

#define	MAX_FIELDS	1024	/* Max # of fields */

LOCAL	char inp_delim = '\t';	/* The input delimiter */
LOCAL	char outp_delim = '\t';	/* The output delimiter */
LOCAL	BOOL no_odelim = FALSE;	/* Whether to suppress output delimiter */
LOCAL	BOOL sup_nodel = FALSE;	/* Suppress lines without delimiters */
LOCAL	char *line;
LOCAL	size_t linesize;

/*
 * A parsed entry takes 2-3 entries in this array.
 * The number of processable fields thus depends on the actual list.
 * We can process 341..512 fields from the command line.
 *
 * 0	is a group delimiter.
 * +#	followed by 0 is a single entry.
 * +# #	followed by 0 is a range #-# entry.
 * -#	followed by 0 means all entries from # to the end.
 */
LOCAL	int  pr_fields[MAX_FIELDS+4];

LOCAL	void	usage		__PR((int exitcode));
EXPORT	int	main		__PR((int ac, char ** av));
LOCAL	int	parse_range	__PR((int cut_type, char * digstr));
LOCAL	int	get_num		__PR((int cut_type, char ** dgpp));
LOCAL	void	cut_fields	__PR((FILE *fp, int nfields));
LOCAL	void	cut_cols	__PR((FILE *fp, int nfields));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	cut [d=C][od=C][-nod][c=range][f=range][-s] file\n");
	error("\td=C\t\tSet input delimiter.\n");
	error("\tod=C\t\tSet output delimiter.\n");
	error("\tc=range\t\tSpecify column range.\n");
	error("\tf=range\t\tSpecify field range.\n");
	error("\t-nod\t\tSuppress output delimiter.\n");
	error("\t-s\t\tSuppress lines without input delimiter.\n");
	error("\t-help\t\tPrint this help.\n");
	error("\t-version\tPrint version information and exit.\n");
	error("	Cannot have both c and f flags.\n");
	error("	Input is taken from standard in if no file is given.\n");
	exit(exitcode);
}

EXPORT int
main(ac, av)
	int	ac;
	char	**av;
{
	int	nfields = 0;
	char	*options = "d?,od?,nod,c*,f*,s,help,version";
	BOOL	help = FALSE;
	BOOL	prvers = FALSE;
	int	cut_type = CUT_NONE;
	char	*c_range = NULL;
	char	*f_range = NULL;
	FILE	*fp = NULL;	/* Keep silly gcc happy */
	int	cac;
	char * const * cav;

	save_args(ac, av);
	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, options,
		    &inp_delim, &outp_delim,
		    &no_odelim,
		    &c_range, &f_range, &sup_nodel, &help, &prvers) < 0) {
		errmsgno(EX_BAD, "Bad flag: '%s'\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);

	if (prvers) {
		/* CSTYLED */
		printf("Cut release %s (%s-%s-%s) Copyright (C) 1985-2015 Jörg Schilling\n",
				"1.20",
				HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	if (c_range && f_range) {
		errmsgno(EX_BAD,
			"Can't have both character and field specified.\n");
		usage(EX_BAD);
	}

	if (c_range) {
		cut_type = CUT_COLS;
		nfields = parse_range(cut_type, c_range);
	} else if (f_range) {
		cut_type = CUT_FIELDS;
		nfields = parse_range(cut_type, f_range);
	} else {
		errmsgno(EX_BAD, "No character or field specified.\n");
		usage(EX_BAD);
	}

	cac = ac;
	cav = av;
	if (getfiles(&cac, &cav, options) <= 0) {
		fp = stdin;
	} else {
		if ((fp = fileopen(cav[0], "r")) == NULL)
			comerr("Can't open '%s'.\n", cav[0]);
	}


	if (cut_type == CUT_FIELDS)
		cut_fields(fp, nfields);
	else if (cut_type == CUT_COLS)
		cut_cols(fp, nfields);
	else
		comerrno(EX_BAD, "Internal failure.\n");

	return (0);
}

/*
 * Parse field/column spec into pr_fields[] array.
 *
 * 0	is a group delimiter.
 * +#	followed by 0 is a single entry.
 * +# #	followed by 0 is a range #-# entry.
 * -#	followed by 0 means all entries from # to the end.
 *
 * Returns the number of int entries in the pr_fields[] array.
 */
LOCAL int
parse_range(cut_type, digstr)
	int	cut_type;
	char	*digstr;
{
	register int nfields = 1, i, a;
	int	b;

	for (i = 1; i < sizeof (pr_fields) / sizeof (pr_fields[0]); i++)
		pr_fields[i] = 0;		/* Fill with group delimter */

	do {
		if (nfields > MAX_FIELDS)
			break;
		if ((a = get_num(cut_type, &digstr)) == NO_NUMBER) {
			if (*digstr == '-') {		/* leading minus */
				++digstr;
				if ((a = get_num(cut_type, &digstr)) ==
				    NO_NUMBER) {
					comerrno(EX_BAD, "Missing operand.\n");
				}

				if (*digstr == ',')
					++digstr;

				pr_fields[nfields++] = 1;
				pr_fields[nfields++] = a;
				pr_fields[nfields++] = 0;
			} else {
				comerrno(EX_BAD,
					"Illegal char:'%c'\n", *digstr);
			}
		} else {
			if (*digstr == ',') {		/* # followed by ',' */
				pr_fields[nfields++] = a;
				pr_fields[nfields++] = 0;
				++digstr;
			} else if (*digstr == '-') {
				++digstr;
				if ((b = get_num(cut_type, &digstr)) ==
				    NO_NUMBER) {
					pr_fields[nfields++] = -a;
					pr_fields[nfields++] = 0;
				} else {
					if (a <= b) {
						pr_fields[nfields++] = a;
						pr_fields[nfields++] = b;
						pr_fields[nfields++] = 0;
					} else {
						comerrno(EX_BAD,
						    "Reversed limits.\n");
					}
				}
				if (*digstr == ',')
					++digstr;
			} else if (*digstr == '\0') {
				pr_fields[nfields++] = a;
				break;
			} else {
				comerrno(EX_BAD,
					"Can't parse '%c'.\n", *digstr);
			}
		}
	} while (*digstr != '\0');

	if (nfields > MAX_FIELDS)
		comerrno(EX_BAD, "Too large range definitions.\n");

	/*
	 * Nfields must be decremented first, as it is the next entry to fill.
	 * Do not count group separator (0) for our return code.
	 * The list in pr_fields[] is always 0 terminated.
	 */
	if (--nfields > 1 && pr_fields[nfields] == 0)
		nfields--;
	return (nfields);
}

#define	isdigit(c)	('0' <= (c) && (c) <= '9')

LOCAL int
get_num(cut_type, dgpp)
	int	cut_type;
	char	**dgpp;
{
	int	ret = 0;
	int	imax;
	int	multmax;
	unsigned char	c;

	c = **dgpp;
	if (!isdigit(c))
		return (NO_NUMBER);

	imax = TYPE_MAXVAL(int);
	multmax = TYPE_MAXVAL(int) / 10;

	while (isdigit(c)) {
		if (ret > multmax)
			comerrno(EX_BAD, "Number too large.\n");
		ret *= 10;
		c -= '0';
		if (c > (imax - ret))
			comerrno(EX_BAD, "Number too large.\n");
		ret += c;
		c = *++*dgpp;
	}

	if (ret == 0) {
		comerrno(EX_BAD, "No zeroth %s.\n",
				(cut_type == CUT_FIELDS?"field":"column"));
	}

	return (ret);
}

/*
 * Cut a file based on fields.
 */
LOCAL void
cut_fields(fp, nfields)
	FILE	*fp;
	int	nfields;
{
	BOOL	has_delim;
	BOOL	printed;	/* Whether we need to add delimiter to output */
	char	*bfp;
	int fidx[MAX_FIELDS+1];	/* Add one as we start at index 1. */
	int i;
	int j, k, l;

	while ((j = fgetaline(fp, &line, &linesize)) > 0) {
		bfp = line;
		if (*bfp == '\0' && ! sup_nodel) {
			putchar('\n');
			continue;
		}
		if (line[j-1] == '\n') {
			line[--j] = '\0';
		}


		has_delim = FALSE;		/* No delim yet on this line */
		fidx[1] = 0;			/* First is always the same */
		for (i = 2; i <= MAX_FIELDS; i++) { /* now add second ... n */
			for (; *bfp != inp_delim && *bfp != '\0'; bfp++)
				;

			if (*bfp == inp_delim)
				has_delim = TRUE;

			if (*bfp == '\0') {	/* No delimiter */
				i--;		/* so reduce fields */
				break;		/* and stop scanning */
			}

			*bfp++ = '\0';		/* Replace field separator */
			fidx[i] = bfp - line;	/* remember field offset */

			if (*bfp == '\0')	/* End of line */
				break;		/* so stop scanning */
		}


		if (!has_delim) {
			if (!sup_nodel)
				printf("%s\n", line);

			continue;
		}

		printed = FALSE;
		/*
		 * Check all parsed entries in pr_fields[].
		 */
		for (j = 1; j <= nfields; j++) {
			if (pr_fields[j] == 0) /* Skip group separator */
				continue;

			if (pr_fields[j] < 0) {	/* < 0: from here to the end */
				for (k = -pr_fields[j]; k <= i; k++) {
					if (printed && !no_odelim)
							putchar(outp_delim);
					printf("%s", &line[fidx[k]]);
					printed = TRUE;
				}
				continue;
			}

			if (pr_fields[j] > i)	/* Field is not in this line */
				continue;

			k = pr_fields[j];
			l = pr_fields[++j];
			if (l == 0)
				l = k;
			if (l > i)
				l = i;
			for (; k <= l; k++) {
				if (printed && !no_odelim)
					putchar(outp_delim);
				printf("%s", &line[fidx[k]]);
				printed = TRUE;
			}
		}
		putchar('\n');
	}
}

/*
 * Cut a file based on columns.
 */
LOCAL void
cut_cols(fp, nfields)
	FILE	*fp;
	int	nfields;
{
	register int	 i;
	register ssize_t len;
	register ssize_t col;

	while ((len = fgetaline(fp, &line, &linesize)) > 0) {
		if (line[len-1] == '\n') {
			line[--len] = '\0';
		}
		for (i = 1; i <= nfields; i++) {
			col = pr_fields[i];
			if (col == 0)		/* Skip group separator */
				continue;
			if (col > 0) {
				ssize_t	end = pr_fields[++i];
				col--;
				if (col < len) {
					if (end == 0)
						putchar(line[col]);
					else
						printf("%.*s",
						    end-col, &line[col]);
				}
			} else {
				col++;
				if (-col < len)
					printf("%s", &line[-col]);
			}
		}
		putchar('\n');
	}
}
