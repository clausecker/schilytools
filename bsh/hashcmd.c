/* @(#)hashcmd.c	1.31 15/08/08 Copyright 1986-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)hashcmd.c	1.31 15/08/08 Copyright 1986-2015 J. Schilling";
#endif
/*
 *	bsh - Commands dealing with #<letter> commands
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

#include <schily/stdio.h>
#include "bsh.h"
#include "node.h"
#include "str.h"
#include "strsubs.h"
#include "abbtab.h"
#include "abbrev.h"
#include "ctype.h"
#include <schily/stdlib.h>
#include <schily/fcntl.h>		/* To get F_GETFD/F_SETFD */

EXPORT	abidx_t	deftab		= GLOBAL_AB;	/* Use .globals by default */

extern	int	delim;
extern	int	ttyflg;
extern	int	MVERSION;
extern	int	mVERSION;
extern	char	dVERSION[];
extern	char	*initav0;
extern	char	*cmdfname;
extern	Tnode	*lastcmd;	/* Used by ancient #e command */

EXPORT	void	hashcmd		__PR((FILE ** std));
LOCAL	void	shcmd		__PR((FILE ** std, char *name));
LOCAL	void	abballusage	__PR((FILE ** std));
LOCAL	void	abbusage	__PR((FILE ** std, int cmd));
LOCAL	char	*nameok		__PR((char *n));

/*
 * Parse and execute a hashmark command
 */
EXPORT void
hashcmd(std)
	FILE	*std[];
{
	int	cmd;
	char	*name;
	char	*name2;
	char	*val;
	int	bflg = 0;
	int	delflg = 0;
	int	histflg = 0;
	abidx_t	tab = deftab;

	quote();
	cmd = nextch();						/* First skip '#' */
	skipwhite();
	if (!ttyflg && (iswhite(cmd) || argend(nl, 0)))		/* Kommentar */
		cmd = ' ';
	else
		cmd = tolower(delim);
	if (cmd != ' ' && argend(nl, 0)) {
		fprintf(std[2], relmsg, MVERSION, mVERSION, dVERSION, HOST_CPU, HOST_VENDOR, HOST_OS);

		unquote();
		return;
	} else if (cmd == 'b') {
		bflg = AB_BEGIN;
	}

	if (cmd == '!') {
		nextch();
	} else if (cmd != ' ') {
		nextch();
		while (!argend(spaces, 0)) {
			delim = tolower(delim);
			switch (delim) {

			case 'g':
				tab = GLOBAL_AB;
				break;
			case 'l':
				tab = LOCAL_AB;
				break;
			case 'b':
				if (cmd == 'p') {
					bflg = AB_BEGIN;
					break;
				}
				goto err;
			case 'a':
				if (cmd == 'p') {
					bflg = 0;
					break;
				}
				goto err;
			case 'd':
				if (cmd == 'x')
					delflg++;
				break;
			case 'h':
				if (cmd == 'l') {
					histflg = AB_HISTORY;
					break;
				}
			default:
			err:
				berror("%s", ebadmodifier);
				abbusage(std, cmd);
				eatline();
				return;
			}
			nextch();
		}
	}
	skipwhite();
	name = pstring(spaces, 0);	/* Get next word */
	if (name != NULL && streql(name, helpname)) {
		free(name);
		abbusage(std, cmd);
		eatline();
		return;
	}
	switch (cmd) {

	case 'a':
	case 'b':
	case 'p':
		if (nameok(name)) {
			skipwhite();
			val = pstring(nl, 0);
			if (val == NULL)
				val = makestr(nullstr);
			if (cmd == 'p')
				ab_push(tab, name, val, bflg);
			else
				ab_insert(tab, name, val, bflg);
		}
		else
			abbusage(std, cmd);
		break;
	case 'd':
		if (nameok(name)) {
			do {
				if (name == NULL)
					break;
				ab_delete(tab, name, AB_NOFLAG);
				free(name);
				skipwhite();
			} while ((name = pstring(spaces, 0)) != NULL);
		}
		else
			abbusage(std, cmd);
		break;
	case 'e':
		eatline();
		execute(lastcmd, 0, gstd);
		return;
	case 'l':
		if (name == NULL) {
			ab_dump(tab, std[1], histflg);
		} else {
			do {
				ab_list(tab, name, std[1], histflg);
				free(name);
				skipwhite();
			} while ((name = pstring(spaces, 0)) != NULL);
		}
		break;
	case 'v':
		if (name != NULL) {
			if (streql(name, on)) {
				vflg = TRUE;
			} else if (streql(name, off)) {
				vflg = FALSE;
			} else {
				berror(ebadopt, "v", name);
				abbusage(std, cmd);
			}
		}
		else
			fprintf(std[1], "Verbose %s.\n", vflg?on:off);
		break;
	case 's':
		deftab = tab;
		fprintf(std[1], "Default: %s\n",
				deftab == GLOBAL_AB?
				globalname:localname);
		break;
	case '?':
	case 'h':
		abballusage(std);
		break;
	case 'q':
		delim = EOF;
		break;
	case 'x':
		if (name == NULL) {
			ev_list(std[1]);
		} else {
			if (!strchr(name, '=')) {
				if (delflg) {
					ev_delete(name);
				} else if ((val = getcurenv(name)) != NULL) {
					fprintf(std[1], "%s=%s\n", name, val);
					break;
				}
			} else {
				val = pstring(nl, 0);
				name2 = concat(name, val, (char *)NULL);
				free(name);
				if (val)
					free(val);
				ev_insert(name2);
			}
		}
		break;
	case '!':
		if (ttyflg) {
			berror("%s", eshonly);
			abbusage(std, cmd);
			break;
		}
		if (name != NULL && streql(fbasename(name), fbasename(initav0))) {
			break;
		}
		if (cmdfname != NULL)	/* Can do this only on commandfiles */
			shcmd(std, name);
		break;
	case ' ':
		break;	/* Kommentar */
	default:
		berror("%s", enocmd);
		abballusage(std);
		break;
	}
	eatline();
}

LOCAL void
shcmd(std, name)
	FILE	*std[];
	char	*name;
{
	register int	i;
	register Tnode	*lst;
	register Tnode	*lp;
		Argvec	*vp;

	lst = lp = allocnode(STRING, (Tnode *) name, (Tnode *) NULL);
	while ((lp->tn_right.tn_node = pword()) != NULL)
		lp = lp->tn_right.tn_node;
	vav[0] = cmdfname;
	for (i = 0; i < vac; i++) {
		lp->tn_right.tn_node = allocnode(STRING, (Tnode *) vav[i],
							(Tnode *) NULL);
		lp = lp->tn_right.tn_node;
	}

#ifndef	F_SETFD
	/*
	 * If we canno set the close on exec() flag, we need
	 * to close the files manually.
	 */
	fclose(cmdfp);
	if (protfile != (FILE *)NULL)
		fclose(protfile);
#endif
	vp = scan(lst);
	start(vp, std, 0);	/* no return */
}

LOCAL void
abballusage(std)
	register FILE	*std[];
{
	register int	i;

	for (i = 0; abbtab[i].a_c != '\0'; i++) {
		fprintf(std[2], "%s\n", abbtab[i].a_msg);
	}
}

LOCAL void
abbusage(std, cmd)
		FILE	*std[];
	register int	cmd;
{
	register int i;

	for (i = 0; abbtab[i].a_c != '\0'; i++) {
		if (abbtab[i].a_c == cmd) {
			fprintf(std[2], "%s%s\n", usage, abbtab[i].a_msg);
			break;
		}
	}
	if (abbtab[i].a_c == '\0') {
		fprintf(std[2], "%s\n", enocmd);
		abballusage(std);
	}
}

LOCAL char *
nameok(n)
	char	*n;
{
	if (n == NULL)
		berror("%s", emissabbrev);
	return (n);
}
