/* @(#)hashcmd.c	1.6 15/10/13 Copyright 1986-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)hashcmd.c	1.6 15/10/13 Copyright 1986-2015 J. Schilling";
#endif
/*
 *	Commands dealing with #<letter> commands
 *
 *	Copyright (c) 1986-2015 J. Schilling
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

#ifdef	DO_HASHCMDS

#include "defs.h"
#include "hashtab.h"
#include "abbrev.h"

#undef	tolower
#define	tolower(c)	(((c) >= 'A' && (c) <= 'Z') ? (c) + ('a'-'A') : (c))

EXPORT	abidx_t	deftab		= GLOBAL_AB;	/* Use .globals by default */
LOCAL	int	delim;

EXPORT	void	hashcmd		__PR((void));
LOCAL	void	abballusage	__PR((void));
LOCAL	void	abbusage	__PR((int cmdc));
LOCAL	int	nextch		__PR((void));
LOCAL	int	skipwhite	__PR((void));
LOCAL	void	eatline		__PR((void));
LOCAL	char	*pstring	__PR((int spc));
LOCAL	char	*nameok		__PR((char *n));
LOCAL	void	herror		__PR((char *s));

/*
 * Parse and execute a hashmark command
 */
EXPORT void
hashcmd()
{
	int	cmdc;
	char	*name;
	char	*val;
	int	bflg = 0;
	int	histflg = 0;
	abidx_t	tab = deftab;

	cmdc = nextch();				/* First skip '#' */
	skipwhite();
	if (!isatty(standin->fdes) && (space(cmdc) || eolchar(delim)))
		cmdc = ' ';				/* # in script: ign. */
	else
		cmdc = tolower(delim);
	if (cmdc != ' ' && eolchar(delim)) {
		prversion();
		return;
	} else if (cmdc == 'b') {
		bflg = AB_BEGIN;
	}

	if (cmdc == '!') {
		nextch();
	} else if (cmdc != ' ') {
		nextch();
		while (!(space(delim) || eolchar(delim))) {
			delim = tolower(delim);
			switch (delim) {

			case 'g':
				tab = GLOBAL_AB;
				break;
			case 'l':
				tab = LOCAL_AB;
				break;
			case 'b':
				if (cmdc == 'p') {
					bflg = AB_BEGIN;
					break;
				}
				goto err;
			case 'a':
				if (cmdc == 'p') {
					bflg = 0;
					break;
				}
				goto err;
			case 'h':
				if (cmdc == 'l') {
					histflg = AB_HISTORY;
					break;
				}
			default:
			err:
				herror("Bad modifier");
				abbusage(cmdc);
				eatline();
				return;
			}
			nextch();
		}
	}
	skipwhite();
	name = pstring(TRUE);		/* Get next word */
	if (name != NULL && eq(name, "-help")) {
		free(name);
		abbusage(cmdc);
		eatline();
		return;
	}
	switch (cmdc) {

	case 'a':
	case 'b':
	case 'p':
		if (nameok(name)) {
			skipwhite();
			val = pstring(FALSE);
			if (val == NULL)
				val = (char *)make(UC nullstr);
			if (cmdc == 'p')
				ab_push(tab, name, val, bflg);
			else
				ab_insert(tab, name, val, bflg);
		} else {
			abbusage(cmdc);
		}
		break;
	case 'd':
		if (nameok(name)) {
			do {
				if (name == NULL)
					break;
				ab_delete(tab, name, AB_NOFLAG);
				free(name);
				skipwhite();
			} while ((name = pstring(TRUE)) != NULL);
		} else {
			abbusage(cmdc);
		}
		break;
	case 'l':
		if (name == NULL) {
			ab_dump(tab, STDOUT_FILENO, histflg);
		} else {
			do {
				ab_list(tab, name, STDOUT_FILENO, histflg);
				free(name);
				skipwhite();
			} while ((name = pstring(TRUE)) != NULL);
		}
		flushb();
		break;
	case 's':
		deftab = tab;
		prs_buff(_gettext("Default: "));
		prs_buff(UC(deftab == GLOBAL_AB?globalname:localname));
		prc_buff(NL);
		flushb();
		break;
	case '?':
	case 'h':
		abballusage();
		break;
	case '!':	/* This shell always ignores #! */
	case ' ':
		break;	/* Kommentar */
		/*
		 * We do not implement all commands from the original concept
		 * in the UNOS command interpreter. The original did support
		 * the following additional commands:
		 *
		 * #!	Manage other interpreters in scripts at user level.
		 * #e	re-execute the parsed tree from the last command.
		 * #q	quit the shell.
		 * #v	switch on/off command verbosity similar to "set -x".
		 * #x	manage the environemt variables.
		 *
		 * So keep in mind that the command characters from the list
		 * above should not be used for future extensions.
		 */
	default:
		herror("Unknown command");
		abballusage();
		break;
	}
	eatline();
}

LOCAL void
abballusage()
{
	register int	i;
	int	save_fd = setb(STDERR_FILENO);

	for (i = 0; abbtab[i].a_c != '\0'; i++) {
		prs_buff(UC abbtab[i].a_msg);
		prc_buff(NL);
	}
	flushb();
	(void) setb(save_fd);
}

LOCAL void
abbusage(cmdc)
	register int	cmdc;
{
	register int i;

	for (i = 0; abbtab[i].a_c != '\0'; i++) {
		if (abbtab[i].a_c == cmdc) {
			int	save_fd = setb(STDERR_FILENO);

			prs_buff(_gettext("Usage: "));
			prs_buff(_gettext(abbtab[i].a_msg));
			prc_buff(NL);
			flushb();
			(void) setb(save_fd);
			break;
		}
	}
	if (abbtab[i].a_c == '\0') {
		int	save_fd = setb(STDERR_FILENO);

		prs_buff(_gettext("Unknown command."));
		prc_buff(NL);
		flushb();
		(void) setb(save_fd);
		abballusage();
	}
}

LOCAL int
nextch()
{
	return (delim = readwc());
}

LOCAL int
skipwhite()
{
	while (delim && space(delim))
		nextch();
	return (delim);
}

LOCAL void
eatline()
{
	while (!eolchar(delim))
		nextch();
}

LOCAL char *
pstring(spc)
	int	spc;
{
	char	buf[1024];
	char	*p = buf;

	if (eolchar(delim))
		return (NULL);

	while (!eolchar(delim)) {
		if (spc && space(delim))
			break;
		if (p >= &buf[sizeof (buf) - MULTI_BYTE_MAX - 1]) {
			herror("Argument too long");
			eatline();
			return (NULL);
		}
		p = (char *)movstr(readw(delim), UC p);
		nextch();
	}
	*p = '\0';
	return ((char *)make(UC buf));
}

LOCAL char *
nameok(n)
	char	*n;
{
	if (n == NULL)
		herror("Missing alias name");
	return (n);
}

LOCAL void
herror(s)
	char	*s;
{
	int	save_fd = setb(STDERR_FILENO);

	prs_buff(_gettext(s));
	prc_buff('.');
	prc_buff(NL);
	flushb();
	(void) setb(save_fd);
}
#endif	/* DO_HASHCMDS */
