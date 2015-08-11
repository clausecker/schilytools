/* @(#)paste.c	1.21 15/08/06 Copyright 1985-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)paste.c	1.21 15/08/06 Copyright 1985-2015 J. Schilling";
#endif
/*
 *	Paste some files together
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
#include <schily/standard.h>
#include <schily/schily.h>

#define	MIN_LINELEN	4096		/* Min line size  */
#define	INCR_LINELEN	4096		/* Increment for line size */
#define	MAX_FILES 	(256-3)		/* Max # of files */

LOCAL	BOOL	eofp[MAX_FILES];	/* Files that did hit EOF */
LOCAL	FILE	*filep[MAX_FILES] = {0}; /* Files to work on */
LOCAL	char	*delim;
LOCAL	int	delimlen;
LOCAL	int	empty;

LOCAL	char	*line;			/* Output line */
LOCAL	size_t	linelen = MIN_LINELEN;
LOCAL	int	linesize = -1;

LOCAL	void	usage	__PR((int exitcode));
EXPORT	int	main	__PR((int ac, char ** av));
LOCAL	void	paste	__PR((int n));
LOCAL	void	spaste	__PR((FILE *f));
LOCAL	int	parsedelim __PR((char *d));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	paste [options] file1...filen\n");
	error("Options:\n");
	error("\td=list\t\tuse 'list' as delimiter instead of 'tab'.\n");
	error("\t-e\t\tdo not output empty lines.\n");
	error("\t-s\t\tpaste lines of one file instead of one line per file.\n");
	error("\twidth=#,w=#\tmaximum output linewidth (default infinite).\n");
	error("\t-help\t\tPrint this help.\n");
	error("\t-version\tPrint version information and exit.\n");
	exit(exitcode);
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	i = 0;			/* Count # of input files */

	char	*options = "help,version,d*,e,s,width#,w#";
	int	help	= 0;
	int	prvers	= 0;
	int	ser	= 0;
	int	cac;
	char	* const *cav;

	save_args(ac, av);
	cac	= --ac;
	cav	= ++av;

	if (getallargs(&cac, &cav, options, &help, &prvers,
					&delim, &empty, &ser,
					&linesize, &linesize) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);

	if (prvers) {
		/* CSTYLED */
		printf("Paste release %s (%s-%s-%s) Copyright (C) 1985-2015 Jörg Schilling\n",
				"1.21",
				HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	if (delim) {
		delimlen = parsedelim(delim);
		if (delimlen == 0)
			comerrno(EX_BAD, "No delimiter.\n");
	} else {
		delim = "\t";
		delimlen = 1;
	}
	cac	= ac;
	cav	= av;
	for (; getfiles(&cac, &cav, options); cac--, cav++) {
		if (i >= MAX_FILES)
			comerrno(EX_BAD, "Cannot paste more than %d files.\n",
				MAX_FILES);
		if (streql(*cav, "-"))
			filep[i++] = stdin;
		else if ((filep[i++] = fileopen(*cav, "r")) == (FILE *)NULL)
			comerr("Cannot open '%s'.\n", *cav);
		if (ser)
			spaste(filep[i-1]);
	}

	if (i == 0) {
		errmsgno(EX_BAD, "No files given.\n");
		usage(EX_BAD);
	}
	if (ser)
		return (0);

	if (linesize > 0)
		linelen = linesize;
	if ((line = malloc(linelen+2)) == NULL)
		comerr("Cannot malloc space for line.\n");

	paste(i);
	return (0);
}

LOCAL void
paste(n)
	int	n;		/* # of files to read from */
{
	int	k;
	register char	*lp;	/* pointer to line */
	register char	*ep;	/* pointer to end of line */
	register int	c;
	register FILE	*fp;
	register int	i;

	for (i = 0; i < n; i++)
		eofp[i] = FALSE;
	ep = &line[linelen];

	for (k = 0; k < n; ) {	/* Stop when all files hit EOF */
		lp = line;
		for (i = 0; i < n; i++) {
			if (!eofp[i]) {		/* No EOF on this file yet? */
				fp = filep[i];
			again:
				while ((c = getc(fp)) != EOF &&
				    c != '\n' && lp < ep)
					*lp++ = c;

				if (lp >= ep) {
					char	*new = NULL;
					static	BOOL didwarn = FALSE;

					if (linesize < 0) {
						/*
						 * Use dynamic line length,
						 */
						linelen += INCR_LINELEN;
						new = realloc(line,
						    linelen+2 + INCR_LINELEN);
					} else {
						didwarn = TRUE;
					}
					if (new == NULL) {
						if (!didwarn)
							errmsg(
					"Cannot realloc space for line.\n");
						didwarn = TRUE;
					} else {
						linelen += INCR_LINELEN;
						lp += new - line;
						ep = &new[linelen];
						line = new;
						goto again;
					}
					while ((c = getc(fp)) != EOF &&
					    c != '\n')
						;
				}
			} else {		/* Avoid EOFing twice	*/
				c = '\n';	/* so pretend eol.	*/
			}

			if (c == EOF) {
				eofp[i] = EOF;
				k++;		/* One file less */
			}

			if (i == (n-1)) {	/* Last file in line? */
				*lp++ = '\n';
				*lp   = '\0';
				break;		/* Quit loop to print line */
			} else {		/* Add field delimiter */
				c = delim[i%delimlen];
				if (c)
					*lp++ = c;
			}
		}

		/*
		 * Print only if at least one file did have a line (k <  n).
		 * With -e, print only lines that have more than delimiters.
		 */
		if (lp-line > n || (!empty && k < n))
			filewrite(stdout, line, lp-line);
	}
}

LOCAL void
spaste(f)
	FILE	*f;
{
	register int	len;
	register int	i;
	register int	c;

	if ((len = fgetaline(f, &line, &linelen)) > 0) {
		for (i = 0; ; i++) {
			if (line[len-1] == '\n')
				len--;
			filewrite(stdout, line, len);
			if ((len = fgetaline(f, &line, &linelen)) <= 0)
				break;
			c = delim[i%delimlen];
			if (c)
				putchar(c);
		}
	}
	putchar('\n');
	fclose(f);
}

LOCAL int
parsedelim(d)
	char	*d;
{
	int	len = 0;
	int	c;
static	char	del[MAX_FILES];

	while (*d) {
		if ((c = *d++) != '\\') {
			del[len++] = c;
		} else {
			switch (c = *d++) {

			case '0':
				c = 0;
				break;
			case 't':
				c = '\t';
				break;
			case 'n':
				c = '\n';
				break;
			}
			del[len++] = c;
		}
		if (len >= MAX_FILES)
			break;
	}
	delim = del;
	return (len);
}
