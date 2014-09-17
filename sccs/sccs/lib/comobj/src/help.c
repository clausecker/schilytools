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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2014 J. Schilling
 *
 * @(#)help.c	1.18 14/08/10 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)help.c 1.18 14/08/10 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)help.c"
#pragma ident	"@(#)sccs:lib/comobj/help.c"
#endif
#include	<defines.h>
#include	<i18n.h>
#include	<ccstypes.h>

/*
 *	Routine to locate helpful info in an ascii file.
 *
 *	I18N changes are as follows:
 *
 *	First determine the appropriate directory to search for help text
 *	files corresponding to the user's locale.  Consistent with the
 *	setlocale() routine algorithm, environment variables are here
 *	searched in the following order of descending priority: LC_ALL,
 *	LC_MESSAGES, LANG.  The first one found to be defined is used
 *	as the locale for help messages.  If this directory does in fact
 *	not exist, a fallback to the (default) "C" locale is done.
 *
 *	The file to be searched is determined from the argument. If the
 *	argument does not contain numerics, the search
 *	will be attempted on '/usr/ccs/lib/help/cmds', with the search key
 *	being the whole argument. This is used for SCCS command help.
 *
 *	If the argument begins with non-numerics but contains
 *	numerics (e.g, zz32) the file /usr/ccs/lib/help/helploc
 *	will be checked for a file corresponding to the non numeric prefix,
 *	That file will then be seached for the mesage. If
 *	/usr/ccs/lib/help/helploc
 *	does not exist or the prefix is not found there the search will
 *	be attempted on '/usr/ccs/lib/help/<non-numeric prefix>',
 *	(e.g., /usr/ccs/lib/help/zz), with the search key
 *	being <remainder of arg>, (e.g., 32).
 *	If the argument is all numeric, or if the file as
 *	determined above does not exist, the search will be attempted on
 *	'/usr/ccs/lib/help/default' with the search key being
 *	the entire argument.
 *	In no case will more than one search per argument be performed.
 *
 *	File is formatted as follows:
 *
 *		* comment
 *		* comment
 *		-str1
 *		text
 *		-str2
 *		text
 *		* comment
 *		text
 *		-str3
 *		text
 *
 *	The "str?" that matches the key is found and
 *	the following text lines are printed.
 *	Comments are ignored.
 */

#define	HELPLOC		"/ccs/lib/help/helploc"
#define	DEFAULT_LOCALE	"C"			/* Default English. */

static int	findprt __PR((FILE *f, char *p, char *locale));
static int	lochelp __PR((char *ky, char *fi, size_t fisize));


int
sccsfatalhelp(msg)
	char	*msg;
{
	char *p;
	char *pend;
	char xtrmsg[64];

	if (getenv("SCCS_NO_HELP"))
		return (0);
	/*
	 * Find end of string.
	 */
	p = msg;
	while (*p)
		p++;
	/*
	 * If the string does not end in ')', this message does not contain a
	 * SCCS error code - just return and do nothing.
	 */
	if (*(--p) != ')')
		return (1);
	/*
	 * Valid SCCS error codes and in a number.
	 */
	if (p <= msg || p[-1] < '0' || p[1] > '9')
		return (1);
	/*
	 * Copy over the SCCS error code and call findprt().
	 */
	pend = p;
	while (p > msg && *--p != '(')
		;
	if (pend - p > 50)
		return (1);
	p++;
	pend = xtrmsg;
	while (*p != '\0' && *p != ')')
		*pend++ = *p++;
	*pend = '\0';
	return (sccshelp(stderr, xtrmsg));
}

int
sccshelp(f, msg)
	FILE	*f;
	char	*msg;
{
	char	*locale = NULL; /* User's locale. */

#ifdef	LC_MESSAGES
	/*
	 * Get the locale value for the LC_MESSAGES category.
	 */
	locale = setlocale(LC_MESSAGES, (char *)0);
#endif
	/*
	 * This will be used to set the path to retrieve the appropriate
	 * help files in "/.../help.d/locale/<locale>".
	 */
	if (locale == NULL)
		locale = DEFAULT_LOCALE;

	return (findprt(f, msg, locale));
}


static int
findprt(f, p, locale)
	FILE	*f;
	char	*p;	/* "p" is user specified error code. */
	char	*locale;
{
	register char *q;
	register char *q2 = NULL;
	char	key[150];
	char	line[MAXLINE+1];
	char	hfile[max(8192, PATH_MAX+1)];
	FILE	*iop;
	char	*dftfile = NOGETTEXT("/default");
	char	*helpdir = NOGETTEXT("/ccs/lib/help/locale/");
	char	help_dir[max(8192, PATH_MAX+1)]; /* Directory to search for. */

	if ((int) size(p) > 50)
		return (1);

	/*
	 * Set directory to search for SCCS help text, not general
	 * message text wrapped with "gettext()".
	 */
	strlcpy(help_dir, sccs_insbase?sccs_insbase:"/usr", sizeof (help_dir));
	strlcat(help_dir, helpdir, sizeof (help_dir));
	strlcat(help_dir, locale, sizeof (help_dir));

	/*
	 * If the help directory does not exist, set default locale to English.
	 */
	if (stat(help_dir, &Statbuf) != 0) {
		locale = DEFAULT_LOCALE;
		strlcpy(help_dir, sccs_insbase?sccs_insbase:"/usr",
						sizeof (help_dir));
		strlcat(help_dir, helpdir, sizeof (help_dir));
		strlcat(help_dir, locale, sizeof (help_dir));
	}

	q = p;

	while (*q && !numeric(*q))
		q++;

	if (*q != '\0') {		/* first char alpha, then numeric */
		q2 = q;			/* check whether alpha follows	  */
		while (*q2 && numeric(*q2))	/* the numeric part	  */
			q2++;
		if (*q2 == '\0')
			q2 = NULL;
	}

	if (*q == '\0') {		/* all alphabetics */
		strlcpy(key, p, sizeof (key));
		snprintf(hfile, sizeof (hfile), "%s%s",
				help_dir, NOGETTEXT("/cmds"));
		if (!exists(hfile)) {
			snprintf(hfile, sizeof (hfile), "%s%s",
				help_dir, dftfile);
		}

	} else if (q == p) {		/* first char numeric */
			strlcpy(key, p, sizeof (key));
			snprintf(hfile, sizeof (hfile), "%s%s",
				help_dir, dftfile);

	} else {				/* first char alpha, then num */
		strlcpy(key, p, sizeof (key));	/* key used as temporary */
		*(key + (q - p)) = '\0';
		if (!lochelp(key, hfile, sizeof (hfile)))
			snprintf(hfile, sizeof (hfile), "%s%s%s", help_dir,
			    NOGETTEXT("/"), key);
		else
			cat(hfile, hfile, NOGETTEXT("/"), locale,
			    NOGETTEXT("/"), key, (char *)0);
		strlcpy(key, q, sizeof (key));

		if (!exists(hfile) && q2) {	/* "rcs2sccs" found?	*/
			strlcpy(key, p, sizeof (key));
			snprintf(hfile, sizeof (hfile), "%s%s",
				help_dir, NOGETTEXT("/cmds"));
		}
		if (!exists(hfile)) {
			strlcpy(key, p, sizeof (key));
			snprintf(hfile, sizeof (hfile), "%s%s%s",
				sccs_insbase?sccs_insbase:"/usr",
				helpdir, dftfile);
		}
	}

	if ((iop = fopen(hfile, NOGETTEXT("r"))) == NULL)
		return (1);

	/*
	 * Now read file, looking for key.
	 */
	while ((q = fgets(line, sizeof (line)-1, iop)) != NULL) {
		repl(line, '\n', '\0');		/* replace newline char */
		if (line[0] == '-' && equal(&line[1], key))
			break;
	}

	if (q == NULL) {	/* endfile? */
		fclose(iop);
		if ((Fflags & FTLFUNC) == 0) {
			/*
			 * We have not been called via a callback from fatal().
			 * We thus may fall fatal() again without causing an
			 * endless recursion.
			 */
			snprintf(SccsError, sizeof (SccsError),
				gettext("Key '%s' not found (he1)"), p);
			fatal(SccsError);
		}
		return (1);
	}

	fprintf(f, "\n%s:\n", p);

	while (fgets(line, sizeof (line)-1, iop) != NULL && line[0] == '-')
		;

	do {
		if (line[0] != '*')
			fprintf(f, "%s", line);
	} while (fgets(line, sizeof (line)-1, iop) != NULL && line[0] != '-');

	fclose(iop);
	return (0);
}

/*
 * lochelp finds the file which contains the help messages.
 * if none found returns 0.  If found, as a side effect, lochelp
 * modifies the actual second parameter passed to lochelp to contain
 * the file name of the found file (pointed to by the automatic
 * variable fi).
 */
static int
lochelp(ky, fi, fisize)
	char *ky, *fi; /* ky is key  fi is found file name */
	size_t	fisize;
{
	FILE *fp;
	char locfile[MAXLINE + 1];
	char *hold;

	strlcpy(locfile, sccs_insbase?sccs_insbase:"/usr", sizeof (locfile));
	strlcat(locfile, HELPLOC, sizeof (locfile));
	if (!(fp = fopen(locfile, "r"))) {
		/* no lochelp file */
		return (0);
	}
	while (fgets(locfile, sizeof (locfile)-1, fp) != NULL) {
		hold = (char *)strtok(locfile, "\t ");
		if (!(strcmp(ky, hold)))
		{
			hold = (char *)strtok(0, "\n");
			strlcpy(fi, hold, fisize); /* copy file name to fi */
			fclose(fp);
			return (1); /* entry found */
		}
	}
	fclose(fp);
	return (0); /* no entry found */
}
