/* @(#)cap.c	1.19 08/01/10 Copyright 2000-2008 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)cap.c	1.19 08/01/10 Copyright 2000-2008 J. Schilling";
#endif
/*
 *	termcap		a TERMCAP compiler
 *
 *	Copyright (c) 2000-2008 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/termcap.h>
#include <schily/getargs.h>
#include <schily/schily.h>

#define	TBUF	2048

typedef struct {
	char	*tc_name;	/* Termcap name */
	char	*tc_iname;	/* Terminfo name */
	char	*tc_var;	/* Curses Variable name */
	char	*tc_comment;	/* Explanation */
	int	tc_flags;
} clist;

#define	C_BOOL		0x01	/* This is a boolean entry */
#define	C_INT		0x02	/* This is a numeric entry */
#define	C_STRING	0x04	/* This is a string entry */
#define	C_TC		0x08	/* This is a tc= string entry */
#define	C_PAD		0x10	/* This rentry requires padding */
#define	C_PADN		0x20	/* Padding based on affect count */
#define	C_OLD		0x40	/* This is an old termcap only entry */
#define	C_CURIOUS	0x80	/* This is a curious termcap entry */

clist caplist[] = {
#include "caplist.c"
};

int	ncaps = sizeof (caplist) / sizeof (caplist[0]);

LOCAL	BOOL	dooctal = FALSE;

LOCAL	void	init_clist	__PR((void));
LOCAL	char *	tskip		__PR((char *ep));
LOCAL	char *	tfind		__PR((char *ep, char *ent));
LOCAL	void	dumplist	__PR((void));
LOCAL	void	usage		__PR((int ex));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	void	checkentries	__PR((char *tname, int *slenp));
LOCAL	void	checkbad	__PR((char *tname, char *unknown, char *disabled));
LOCAL	void	outcap		__PR((char *tname, char *unknown, char *disabled, BOOL obsolete_last));
LOCAL	char *	quote		__PR((char *s));
LOCAL	char *	requote		__PR((char *s));
LOCAL	void	compile_ent	__PR((char *tname));
LOCAL	void	read_names	__PR((char *fname));


LOCAL void
init_clist()
{
	int	i;
	int	flags = 0;

	for (i = 0; i < ncaps; i++) {
		/*
		 * Process meta entries.
		 */
		if (caplist[i].tc_name[0] == '-' &&
		    caplist[i].tc_name[1] == '-') {
		}

		if (streql(caplist[i].tc_var, "BOOL")) {

			flags &= ~(C_BOOL|C_INT|C_STRING);
			flags |= C_BOOL;
		}
		if (streql(caplist[i].tc_var, "INT")) {

			flags &= ~(C_BOOL|C_INT|C_STRING);
			flags |= C_INT;
		}
		if (streql(caplist[i].tc_var, "STRING")) {

			flags &= ~(C_BOOL|C_INT|C_STRING);
			flags |= C_STRING;
		}
		if (streql(caplist[i].tc_var, "TC")) {

			flags |= C_TC;
		}
		if (streql(caplist[i].tc_var, "COMMENT")) {

			flags &= ~(C_BOOL|C_INT|C_STRING);
			flags &= ~C_OLD;
		}
		if (streql(caplist[i].tc_var, "OBSOLETE")) {

			flags |= C_OLD;
		}
		if (streql(caplist[i].tc_var, "CURIOUS")) {

			flags &= ~C_OLD;
			flags |= C_CURIOUS;
		}

		caplist[i].tc_flags = flags;
	}
}


/*
 * Skip past next ':'.
 * If the are two consecutive ':', the returned pointer may point to ':'.
 */
LOCAL char *
tskip(ep)
	register	char	*ep;
{
	while (*ep) {
		if (*ep++ == ':')
			return (ep);
	}
	return (ep);
}

LOCAL char *
tfind(ep, ent)
	register	char	*ep;
			char	*ent;
{
	register	char	e0 = ent[0];
	register	char	e1 = ent[1];

	for (;;) {
		ep = tskip(ep);
		if (*ep == '\0')
			break;
		if (*ep == ':')
			continue;
		if (e0 != *ep++)
			continue;
		if (*ep == '\0')
			break;
		if (e1 != *ep++)
			continue;
		return (ep);
	}
	return ((char *) NULL);
}

LOCAL void
dumplist()
{
	int	i;

	for (i = 0; i < ncaps; i++) {
		int l;
		/*
		 * Skip meta entries.
		 */
		if (caplist[i].tc_name[0] == '-' &&
		    caplist[i].tc_name[1] == '-')
			continue;
		if (caplist[i].tc_name[0] == '.' &&
		    caplist[i].tc_name[1] == '.')
			continue;

		l = strlen(caplist[i].tc_var);
		printf("{\"%s\",	\"%s\",%s\"%s\"},%s	/*%c%c %s */\n",
			caplist[i].tc_name,
			caplist[i].tc_iname,
			strlen(caplist[i].tc_iname) >= 5 ? "\t":"\t\t",
			caplist[i].tc_var,
			l >= 20 ? "":
			l >= 12 ? "\t":
			l >= 4 ? "\t\t": "\t\t\t",
			(caplist[i].tc_flags & C_BOOL) ? 'B':
			(caplist[i].tc_flags & C_INT) ? 'I':
			(caplist[i].tc_flags & C_STRING) ? 'S': '?',
			(caplist[i].tc_flags & C_OLD) ? 'O': ' ',
			caplist[i].tc_comment);
	}
}

LOCAL void
usage(ex)
	int	ex;
{
	error("Usage: %s\n", get_progname());
	error("Options:\n");
	error("-help		print this help\n");
	error("-version	print version number\n");
	error("-dumplist	dump internal capability list\n");
	error("-inorder	print caps in order, else print outdated caps last\n");
	error("-dooctal	prefer '\\003' before '^C' when creating escaped strings\n");
	error("if=name		input file for termcap compiling\n");
	error("-v		increase verbosity level\n");
	exit(ex);
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	*const *cav;
	char	*tbuf;		/* Termcap buffer */
	char	unknown[TBUF];	/* Buffer fuer "unknown" Entries */
	char	disabled[TBUF];	/* Buffer fuer :..xx: Entries */
	char	*tname = getenv("TERM");
	char	*tcap = getenv("TERMCAP");
	int	slen;
int	fullen;
int	strippedlen;
	BOOL	help = FALSE;
	BOOL	prvers = FALSE;
	BOOL	dodump = FALSE;
	BOOL	inorder = FALSE;
	int	verbose = 0;
	char	*infile = NULL;

	save_args(ac, av);

	init_clist();

	cac = ac;
	cav = av;
	cac--, cav++;
	if (getallargs(&cac, &cav, "help,version,dumplist,inorder,v+,if*,dooctal",
				&help, &prvers,
				&dodump, &inorder, &verbose,
				&infile, &dooctal) < 0) {
		errmsgno(EX_BAD, "Bad option '%s'\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prvers) {
		printf("termcap %s (%s-%s-%s)\n\n", "1.19", HOST_CPU, HOST_VENDOR, HOST_OS);
		printf("Copyright (C) 2000-2008 Jörg Schilling\n");
		printf("This is free software; see the source for copying conditions.  There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}

	if (dodump) {
		dumplist();
		exit(0);
	}

/*#define	__test__*/
#ifdef	__test__
	{ int i;
	for (i = 0; i < ncaps; i++) {
		/*
		 * Skip meta entries.
		 */
		if (caplist[i].tc_name[0] == '-' &&
		    caplist[i].tc_name[1] == '-')
			continue;
		if (caplist[i].tc_name[0] == '.' &&
		    caplist[i].tc_name[1] == '.')
			continue;
		printf("%s,	\"%s\"	\"%s\"\n",
			caplist[i].tc_var,
			caplist[i].tc_iname,
			caplist[i].tc_name
/*			caplist[i].tc_comment,*/
		);
	}
	exit(0);
	}
#endif

	if (tcap && *tcap != '/')
		*tcap = '\0';

	if (infile) {
		char	env[2048];

		*tcap = '\0';
		*tname = '\0';
		snprintf(env, sizeof (env), "TERMPATH=%s", infile);
		putenv(env);
		read_names(infile);
		exit(0);
	}

	/*
	 * Check existence & unstripped Termcap len
	 */
	tcsetflags(TCF_NO_TC|TCF_NO_SIZE|TCF_NO_STRIP);
	if (tgetent(NULL, tname) != 1)
		comerr("no term '%s' found\n", tname);
	tbuf = tcgetbuf();
	fullen = strlen(tbuf);

	/*
	 * Get Stripped len
	 */
	tcsetflags(TCF_NO_TC|TCF_NO_SIZE);
	tgetent(NULL, tname);
	tbuf = tcgetbuf();
	strippedlen = strlen(tbuf);

	checkentries(tname, &slen);

	if (verbose) {
		printf("tbuf: '%s'\n", tbuf);
		printf("full tbuf len: %d stripped tbuf len: %d\n", fullen, strippedlen);
		printf("string length: %d\n", slen);
	}

	checkbad(tname, unknown, disabled);
	outcap(tname, unknown, disabled, !inorder);

	return (0);
}

LOCAL void
checkentries(tname, slenp)
	char	*tname;
	int	*slenp;
{
	char	stbuf[TBUF];	/* String buffer zum zaehlen */
	char	*sbp;		/* Sting buffer Ende */
	int	i;
	int	b;		/* Fuer bool/int Werte */
	char	*p;
	char	*p2;
	char	*pe;
	char	*tbuf = tcgetbuf();

	/*
	 * Print first part of Termcap entry
	 */
	p = strchr(tbuf, ':');
	if (p)
		i = p - tbuf;
	else
		i = strlen(tbuf);
	printf("tbuf: '%.*s'\n", i, tbuf);

	sbp = stbuf;
	p2 = tbuf;
	for (i = 0; i < ncaps; i++) {
		/*
		 * Skip meta entries.
		 */
		if (caplist[i].tc_name[0] == '-' &&
		    caplist[i].tc_name[1] == '-')
			continue;

		if (!(pe = tfind(tbuf, caplist[i].tc_name)))
			continue;

		if ((caplist[i].tc_flags & (C_BOOL|C_INT|C_STRING)) == 0)
			continue;

		if ((caplist[i].tc_flags & C_OLD) != 0) {
			printf("OBSOLETE ");
		}

		if (*pe == '@') {
			printf("'%s' -> %s", caplist[i].tc_name,
				 ((caplist[i].tc_flags & C_STRING) != 0)? "null @" :
				(((caplist[i].tc_flags & C_INT) != 0)? "-1 @" :
				(((caplist[i].tc_flags & C_BOOL) != 0)? "FALSE @" :
				"unknown-@")));
			printf("		%s", caplist[i].tc_comment);
			printf("\n");
			continue;
		}
		if (i == ncaps -1) {
			p2 = tfind(p2, "tc");
			if (p2 == NULL)
				break;
			if (*p2 == '=') {
				p = &p2[1];
				strcpy(sbp, p);
				if ((p = strchr(sbp, ':')) != NULL)
					*p = '\0';
				p = sbp;
			} else
				break;
		} else
			p = tgetstr(caplist[i].tc_name, &sbp);
		if (p) {
			/*
			 * XXX Option zum Deaktivieren des Quoten
			 */
			p = quote(p);
			printf("'%s' -> '%s'", caplist[i].tc_name, p);
			printf("		%s", caplist[i].tc_comment);

			if ((caplist[i].tc_flags & C_STRING) == 0)
				printf(" TYPE missmatch");
			printf("\n");
			if (i == ncaps -1)
				i--;
			continue;
		}
		b = tgetnum(caplist[i].tc_name);
		if (b >= 0) {
			printf("'%s' -> %d", caplist[i].tc_name, b);
			printf("		%s", caplist[i].tc_comment);
			if ((caplist[i].tc_flags & C_INT) == 0)
				printf(" TYPE missmatch");
			printf("\n");
			continue;
		}

		b = tgetflag(caplist[i].tc_name);
		printf("'%s' -> %s", caplist[i].tc_name, b?"TRUE":"FALSE");
		printf("		%s", caplist[i].tc_comment);
		if ((caplist[i].tc_flags & C_BOOL) == 0)
			printf(" TYPE missmatch");
		printf("\n");
		continue;

	}
	if (slenp)
		*slenp = sbp - stbuf;
}

LOCAL void
checkbad(tname, unknown, disabled)
	char	*tname;
	char	*unknown;
	char	*disabled;
{
	char	ent[3];		/* Space to hold termcap entry */
	char	*up;		/* Unknown buffer Ende */
	char	*dp;		/* Disabled buffer Ende */
	int	i;
	char	*p;
	char	*p2;
	char	*xp;
	char	*tbuf = tcgetbuf();
	BOOL	out_tty = isatty(STDOUT_FILENO);

	p = tskip(tbuf);
	up = unknown;
	dp = disabled;
	while (*p) {
		while (*p == ':')
			p = tskip(p);
		if (p[2] != ':' && p[2] != '@' && p[2] != '#' && p[2] != '=') {
			p2 = tskip(p);
			if (p[0] == ' ' || p[0] == '\t') {
				printf("# BAD(%s). Skipping blank entry: '%.*s'\n", tname, p2 - p - 1, p);
				if (!out_tty)
				error("BAD(%s). Skipping blank entry: '%.*s'\n", tname, p2 - p - 1, p);
				p = tskip(p);
				continue;
			}
			if (p[0] == '.') {
				if (strncmp(p, "..DISABLED:", 11) == 0 ||
				    strncmp(p, "..OBSOLETE:", 11) == 0 ||
				    strncmp(p, "..UNKNOWN:", 10) == 0) {
					p = tskip(p);
					continue;
				}
				printf("# NOTICE(%s). Disabled entry: '%.*s'\n", tname, p2 - p - 1, p);
				if (!out_tty)
				error("NOTICE(%s). Disabled entry: '%.*s'\n", tname, p2 - p - 1, p);
				if (1) {
					strncpy(dp, p, p2 - p);
					dp += p2 - p;
				} else {
					strncpy(dp, p, p2 - p);
					dp[p2 - p] = '\0';
					strcpy(dp, requote(dp));
					dp += strlen(dp);
					*dp++ = ':';
				}
				p = tskip(p);
				continue;
			}
			xp = p;
			if (xp > &tbuf[2])
				xp = &p[-2];
			while (xp > tbuf && *xp != ':')
				--xp;
			printf("# BAD(%s). Illegal entry (3rd char '%c' for ':%c%c%c'): '%.*s'\n",
					tname, p[2], p[0], p[1], p[2], p2 - xp - 1, xp);
			if (!out_tty)
			error("BAD(%s). Illegal entry (3rd char '%c' for '%c%c%c'): '%.*s'\n",
					tname, p[2], p[0], p[1], p[2], p2 - p - 1, p);
			p = tskip(p);
			continue;
		}
		strncpy(ent, p, 2);
		ent[2] = '\0';

		for (i = 0; i < ncaps; i++) {
			/*
			 * Skip meta entries.
			 */
			if (caplist[i].tc_name[0] == '-' &&
			    caplist[i].tc_name[1] == '-')
				continue;

			if (streql(ent, caplist[i].tc_name))
				break;
		}
		if (i == ncaps) {
			p2 = tskip(p);
			printf("# NOTICE(%s). Unknown entry ('%s'): '%.*s'\n", tname, ent, p2 - p - 1, p);
			if (!out_tty)
			error("NOTICE(%s). Unknown entry ('%s'): '%.*s'\n", tname, ent, p2 - p - 1, p);
			if (1) {
				strncpy(up, p, p2 - p);
				up += p2 - p;
			} else {
				strncpy(up, p, p2 - p);
				up[p2 - p] = '\0';
				strcpy(up, requote(up));
				up += strlen(up);
				*up++ = ':';
			}
		}
		p = tskip(p);
	}
	*up = '\0';
	*dp = '\0';
	if (unknown[0] != '\0') {
		error("NOTICE(%s). Unknown: '%s'\n", tname, unknown);
	}
	if (disabled[0] != '\0') {
		error("NOTICE(%s). Disabled: '%s'\n", tname, disabled);
	}
}

LOCAL int	itotype[5] = { 0, C_BOOL, C_INT, C_STRING, 0 };

LOCAL void
outcap(tname, unknown, disabled, obsolete_last)
	char	*tname;
	char	*unknown;
	char	*disabled;
	BOOL	obsolete_last;
{
	char	stbuf[TBUF];	/* String buffer zum zaehlen */
	char	line[TBUF];	/* Fuer Einzelausgabe */
	char	*sbp;		/* Sting buffer Ende */
	int	llen;
	int	curlen;
	int	i;
	int	j = 0;
	int	b;		/* Fuer bool/int Werte */
	int	flags;
	char	*p;
	char	*p2;
	char	*pe;
	char	*tbuf = tcgetbuf();
BOOL	didobsolete = FALSE;

	p = strchr(tbuf, ':');
	if (p)
		i = p - tbuf;
	else
		i = strlen(tbuf);
	printf("%.*s:", i, tbuf);
	llen = i + 1;

	sbp = stbuf;
	flags = 0;
	p2 = tbuf;

	for (j = obsolete_last ? -1:1; j != 0 && j <= 4; j++)
	for (i = 0; i < ncaps; i++) {
		/*
		 * Skip meta entries.
		 */
		if (caplist[i].tc_name[0] == '-' &&
		    caplist[i].tc_name[1] == '-')
			continue;

		/*
		 * If j < 0, sort order is order from caplist array
		 * If j > 0, sort order is BOOL -> INT -> STRING
		 */
		if (j > 0 && (caplist[i].tc_flags & (C_BOOL|C_INT|C_STRING)) != itotype[j])
			continue;

		if ((caplist[i].tc_flags & (C_BOOL|C_INT|C_STRING)) == 0) {
			if (streql(caplist[i].tc_var, "unknown")) {
				if (unknown[0] == '\0')
					continue;

				if (llen > 9 || flags == 0)
					printf("\\\n\t:");
				printf("%s%s", "..UNKNOWN:\\\n\t:", unknown);
				llen = 9;
			} else {
				if (disabled[0] == '\0')
					continue;

				if (llen > 9 || flags == 0)
					printf("\\\n\t:");
				printf("%s%s", "..DISABLED:\\\n\t:", disabled);
				llen = 9;
			}
			continue;
		}

		if (!(pe = tfind(tbuf, caplist[i].tc_name)))
			continue;

		if (*pe == '@') {
			curlen = sprintf(line, "%s@:", caplist[i].tc_name);
			goto printit;
		}
		if (i == ncaps -1) {
			p2 = tfind(p2, "tc");
			if (p2 == NULL)
				break;
			if (*p2 == '=') {
				p = &p2[1];
				strcpy(sbp, p);
				if ((p = strchr(sbp, ':')) != NULL)
					*p = '\0';
				p = sbp;
			} else
				break;
			curlen = sprintf(line, "%s=%s:", caplist[i].tc_name, p);
			i--;
			goto printit;
		} else
			p = tgetstr(caplist[i].tc_name, &sbp);
		if (p) {
			if (caplist[i].tc_flags & C_STRING) {	/* check type for "ma" */
				curlen = sprintf(line, "%s=%s:", caplist[i].tc_name, quote(p));
				goto printit;
			} else {
				p2 = tskip(pe);
				error("%s: Illegal entry '%s' '%.*s' (should not be a string)\n",
						tname, caplist[i].tc_name,
						p2 - pe - 1, pe);
				continue;
			}
		}
		b = tgetnum(caplist[i].tc_name);
		if (b >= 0) {
			if (caplist[i].tc_flags & C_INT) {	/* check type for "ma" */
				curlen = sprintf(line, "%s#%d:", caplist[i].tc_name, b);
				goto printit;
			} else {
				p2 = tskip(pe);
				error("%s: Illegal entry '%s' '%.*s' (should not be a number)\n",
						tname, caplist[i].tc_name,
						p2 - pe - 1, pe);
				continue;
			}
		}

		b = tgetflag(caplist[i].tc_name);
		if (b != 0) {
			if (caplist[i].tc_flags & C_BOOL) {
				curlen = sprintf(line, "%s:", caplist[i].tc_name);
				goto printit;
			} else {
				p2 = tskip(pe);
				error("%s: Illegal entry '%s' '%.*s' (should not be a bool)\n",
						tname, caplist[i].tc_name,
						p2 - pe - 1, pe);
				continue;
			}
		}
		p2 = tskip(pe);
		error("%s: Illegal entry '%s' '%.*s'\n", tname, caplist[i].tc_name, p2 - pe - 1, pe);
		continue;

printit:
		if (flags != (caplist[i].tc_flags & (C_BOOL|C_INT|C_STRING|C_TC))) {
			printf("\\\n\t:");
			llen = 9;
			flags = caplist[i].tc_flags & (C_BOOL|C_INT|C_STRING|C_TC);
		}

		/*
		 * If j > 0, sort order is BOOL -> INT -> STRING
		 * Do not print the OBSOLETE header in this case.
		 */
		if (j < 0 && (caplist[i].tc_flags & C_OLD) != 0) {
			if (!didobsolete) {
				if (llen > 9)
					printf("\\\n\t:");
				printf("..OBSOLETE:\\\n\t:");
				llen = 9;
				didobsolete = TRUE;
			}
		}

/*error("line: '%s', llen: %d curlen: %d sum: %d\n", line, llen, curlen, llen + curlen);*/
		p = line;
		curlen = strlen(p);
		if ((llen > 9) && ((llen + curlen) >= 79)) {
			printf("\\\n\t:");
			llen = 9 + curlen;
		} else {
			llen += curlen;
		}
		printf("%s", p);
	}
	printf("\n");
}

LOCAL char *
quote(s)
	char	*s;
{
static	char	out[TBUF];
	char	*p1;
	char	*p2;
	unsigned char	c;

	for (p1 = s, p2 = out; *p1; ) {
		c = *p1++;
		if (c == 033) {		/* ESC -> \E */
			*p2++ = '\\';
			*p2++ = 'E';
		} else if (c == '\r') {	/* CR -> \r */
			*p2++ = '\\';
			*p2++ = 'r';
		} else if (c == '\n') {	/* NL -> \n */
			*p2++ = '\\';
			*p2++ = 'n';
		} else if (c == '\t') {	/* TAB -> \t */
			*p2++ = '\\';
			*p2++ = 't';
		} else if (c == '\b') {	/* BS -> \b */
			*p2++ = '\\';
			*p2++ = 'b';
		} else if (c == '\v') {	/* VT -> \v */
			*p2++ = '\\';
			*p2++ = 'v';
		} else if (c == '\\') {	/* \ -> \\ */
			*p2++ = '\\';
			*p2++ = '\\';
		} else if (c == '^') {	/* ^ -> \^ */
			*p2++ = '\\';
			*p2++ = '^';
		} else if (!dooctal &&
			    c <= 0x1F) {	/* Control C -> ^C */
			*p2++ = '^';
			*p2++ = '@' + c;
		} else if (c == ':' || c <= 0x1F || c >= 0x7F) {
			*p2++ = '\\';
			*p2++ = '0' + (c / 64) % 8;
			*p2++ = '0' + (c / 8) % 8;
			*p2++ = '0' + c % 8;
		} else {
			*p2++ = c;
		}
	}
	*p2 = '\0';
	return (out);
}

/*
 *   valued capabilities for easy encoding of characters there:
 *
 *        \E   maps to ESC
 *        ^X   maps to CTRL-X for any appropriate character X
 *        \n   maps to LINEFEED
 *        \r   maps to RETURN
 *        \t   maps to TAB
 *        \b   maps to BACKSPACE
 *        \f   maps to FORMFEED
 *
 *   Finally, characters may be given as three octal digits after
 *   a  backslash  (for  example,  \123),  and  the  characters ^
 *   (caret) and \ (backslash) may be given as \^ and \\  respec-
 *   tively.
 *
 *   If it is necessary to place a : in a capability it  must  be
 *   escaped in octal as \072.
 *
 *   If it is necessary to place a  NUL  character  in  a  string
 *   capability  it  must be encoded as \200.  (The routines that
 *   deal with termcap use C strings and strip the high  bits  of
 *   the  output  very  late,  so that a \200 comes out as a \000
 *   would.)
 */

LOCAL char *
requote(s)
	char	*s;
{
	char	buf[TBUF];
	char	*bp = buf;

	tdecode(s, &bp);
	return (quote(buf));
}

LOCAL void
compile_ent(tname)
	char	*tname;
{
	char	unknown[TBUF];	/* Buffer fuer "unknown" Entries */
	char	disabled[TBUF];	/* Buffer fuer :..xx: Entries */

	tcsetflags(TCF_NO_TC|TCF_NO_SIZE);
	if (tgetent(NULL, tname) != 1)
		return;
/*	tbuf = tcgetbuf();*/
/*	strippedlen = strlen(tbuf);*/

/*	checkentries(tname, &slen);*/

/*	printf("tbuf: '%s'\n", tbuf);*/
/*printf("full tbuf len: %d stripped tbuf len: %d\n", fullen, strippedlen);*/
/*printf("string length: %d\n", slen);*/

	checkbad(tname, unknown, disabled);
	outcap(tname, unknown, disabled, FALSE);
}


#define	TRDBUF	8192
#define	TMAX	1024
#define	TINC	1

LOCAL void
read_names(fname)
	char	*fname;
{
			char	nbuf[TMAX];
			char	rdbuf[TRDBUF];
			char	*tbuf;
			char	*name = nbuf;
	register	char	*bp;
	register	char	*ep;
	register	char	*rbuf = rdbuf;
	register	char	c;
	register	int	count	= 0;
	register	int	tfd;
			int	nents = 0;
			int	tbufsize = TINC;

	tcsetflags(TCF_NO_TC|TCF_NO_SIZE);

	tbuf = bp = malloc(tbufsize);
	if (bp == NULL)
		comerr("Cannot malloc termcap parsing buffer.\n");


	if (fname == NULL)
		fname = "/etc/termcap";
	tfd = open(fname, 0);
	if (tfd < 0)
		comerr("Cannot open '%s'\n", fname);

	/*
	 * Search TERM entry in one file.
	 */
	ep = bp;
	for (;;) {
		if (--count <= 0) {
			if ((count = read(tfd, rdbuf, sizeof (rdbuf))) <= 0) {
				close(tfd);
				error("Found %d terminal entries.\n", nents);
				error("TBuf size %d.\n", tbufsize);
				return;
			}
			rbuf = rdbuf;
		}
		c = *rbuf++;
		if (c == '\n') {
			if (ep > bp && ep[-1] == '\\') {
				ep--;
				continue;
			}
		} else if (ep >= bp + (tbufsize-1)) {
			tbufsize += TINC;
			if ((bp = realloc(bp, tbufsize)) != NULL) {
				ep = bp + (ep - tbuf);
				tbuf = bp;
				*ep++ = c;
				continue;
			} else {
				comerr("Cannot grow termcap parsing buffer.\n");
			}
		} else {
			*ep++ = c;
			continue;
		}
		*ep = '\0';

		if (tbuf[0] != '\0' && tbuf[0] != '#') {
			char	*p;

/*			printf("NAME: '%.20s'\n", tbuf);*/
			p = strchr(tbuf, '|');
			if (p == &tbuf[2]) {
				++p;
				p = strchr(p, '|');
			}
			if (p == 0)
				p = strchr(tbuf, ':');
			if (p) {
				int amt = p - tbuf;

				nents++;
				if (amt > (sizeof (nbuf)-1))
					amt = sizeof (nbuf)-1;
				strncpy(name, tbuf, amt);
				nbuf[amt] = '\0';
/*				printf("name: %s'\n", name);*/
				compile_ent(name);
			} else {
				printf("%s\n", tbuf);
			}
		} else {
			printf("%s\n", tbuf);
		}
		ep = bp;
	}
}
