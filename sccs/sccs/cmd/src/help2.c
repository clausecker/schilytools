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
 * Copyright 2006-2015 J. Schilling
 *
 * @(#)help2.c	1.19 15/02/02 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)help2.c 1.19 15/02/02 J. Schilling"
#endif
/*
 * @(#)help2.c 1.10 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)help2.c"
#pragma ident	"@(#)sccs:cmd/help2.c"
#endif
#include	<defines.h>
#include	<i18n.h>
#include	<ccstypes.h>

/*
 *	Program to locate helpful info in an ascii file.
 *	The program accepts a variable number of arguments.
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
 *
 *	If the argument is omitted, the program requests it.
 */


	int	main __PR((int argc, char **argv));
static char *	ask __PR((void));


int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	register int	i;
		int	numerrs = 0;
#ifdef	PROTOTYPES
	char default_locale[] = NOGETTEXT("C"); /* Default English. */
#else
	char *default_locale = NOGETTEXT("C"); /* Default English. */
#endif
	char *helpdir = NOGETTEXT("/ccs/lib/help/locale/");
	char help_dir[200]; /* Directory to search for help text. */
	char *locale = NULL; /* User's locale. */


	/*
	 * Set locale for all categories.
	 */
	setlocale(LC_ALL, NOGETTEXT(""));

	sccs_setinsbase(INS_BASE);

#ifdef	LC_MESSAGES
	/*
	 * Returns the locale value for the LC_MESSAGES category.  This
	 * will be used to set the path to retrieve the appropriate
	 * help files in "/.../help.d/locale/<locale>".
	 */
	locale = setlocale(LC_MESSAGES, NOGETTEXT(""));
	if (locale == NULL) {
		locale = default_locale;
	}
#else
	locale = default_locale;
#endif

	/*
	 * Set directory to search for general l10n SCCS messages.
	 */
#ifdef	PROTOTYPES
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	    NOGETTEXT(INS_BASE "/ccs/lib/locale/"));
#else
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	    NOGETTEXT("/usr/ccs/lib/locale/"));
#endif
	(void) textdomain(NOGETTEXT("SUNW_SPRO_SCCS"));

	Fflags = FTLMSG;

	/*
	 * Set directory to search for SCCS help text, not general
	 * message text wrapped with "gettext()".
	 */
	strlcpy(help_dir, sccs_insbase?sccs_insbase:"/usr", sizeof (help_dir));
	strlcat(help_dir, helpdir, sizeof (help_dir));
	strlcat(help_dir, locale, sizeof (help_dir));

	/*
	 * The text of the printf statement below should not be wrapped
	 * with gettext().  Since we don't know what the locale is, we
	 * don't know how to get the proper translation text.
	 */
	if (stat(help_dir, &Statbuf) != 0) { /* Does help directory exist? */
		printf(
		    NOGETTEXT("Unrecognized locale... setting to English\n"));
	}

	if (argc == 1) {
		char	*he = ask();
		if (*he == '\0') {
			numerrs += sccshelp(stdout, "intro");
		} else {
			numerrs += sccshelp(stdout, he);
		}
	} else {
		for (i = 1; i < argc; i++)
			numerrs += sccshelp(stdout, argv[i]);
	}
	return ((numerrs == (argc-1)) ? 1 : 0);
}

static char *
ask()
{
	static char resp[51];
		FILE	*iop;

	iop = stdin;

	printf(gettext("Enter the message number or SCCS command name: "));
	fgets(resp, sizeof (resp), iop);
	return (repl(resp, '\n', '\0'));
}
