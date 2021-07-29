/* @(#)cap.c	1.59 21/07/22 Copyright 2000-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cap.c	1.59 21/07/22 Copyright 2000-2021 J. Schilling";
#endif
/*
 *	termcap		a TERMCAP compiler
 *
 *	The termcap database is an ASCII representation of the data
 *	so people may believe that there is no need for a compiler.
 *	Syntax checks and unification however are a property of compilers.
 *	We check for correct data types, output all entries in a unique
 *	order and recode all strings with the same escape notation.
 *	This is needed in to compare two entries and it makes life easier.
 *
 *	Copyright (c) 2000-2021 J. Schilling
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
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/termcap.h>
#include <schily/getargs.h>
#define	SCHILY_PRINT
#include <schily/schily.h>

#define	TBUF	2048

typedef struct {
	char	*tc_name;	/* Termcap name */
	char	*tc_iname;	/* Terminfo name */
	char	*tc_var;	/* Curses Variable name */
	char	*tc_comment;	/* Explanation */
	int	tc_flags;
} clist;

/*
 * Definitions for tc_flags
 */
#define	C_BOOL		0x01	/* This is a boolean entry */
#define	C_INT		0x02	/* This is a numeric entry */
#define	C_STRING	0x04	/* This is a string entry */
#define	C_TC		0x08	/* This is a tc= string entry */
#define	C_PAD		0x10	/* This rentry requires padding */
#define	C_PADN		0x20	/* Padding based on affect count */
#define	C_PARM		0x40	/* Padding based on affect count */
#define	C_OLD		0x100	/* This is an old termcap only entry */
#define	C_CURIOUS	0x200	/* This is a curious termcap entry */

/*
 * The list of capabilities for the termcap command.
 * This contains the Termcap Name, the Terminfo Name and a Comment.
 */
LOCAL clist caplist[] = {
#include "caplist.c"
};

LOCAL	int	ncaps = sizeof (caplist) / sizeof (caplist[0]);

LOCAL	BOOL	nodisabled = FALSE;
LOCAL	BOOL	nounknown = FALSE;
LOCAL	BOOL	nowarn = FALSE;
LOCAL	BOOL	dooctal = FALSE;
LOCAL	BOOL	docaret = FALSE;
LOCAL	BOOL	gnugoto = FALSE;
LOCAL	BOOL	oneline = FALSE;

#ifdef	HAVE_SETVBUF
LOCAL	char	obuf[4096];
#else
#ifdef	HAVE_SETVBUF
LOCAL	char	obuf[BUFSIZ];
#endif
#endif

LOCAL	void	init_clist	__PR((void));
LOCAL	char *	tskip		__PR((char *ep));
LOCAL	char *	tfind		__PR((char *ep, char *ent));
LOCAL	void	dumplist	__PR((void));
LOCAL	void	usage		__PR((int ex));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	void	checkentries	__PR((char *tname, int *slenp));
LOCAL	char	*type2str	__PR((int type));
LOCAL	void	checkbad	__PR((char *tname, char *unknown, char *disabled));
LOCAL	void	outcap		__PR((char *tname, char *unknown, char *disabled, BOOL obsolete_last));
LOCAL	char *	checkgoto	__PR((char *tname, char *ent, char *cm, int col, int line));
LOCAL	char *	checkquote	__PR((char *tname, char *s));
LOCAL	char *	quote		__PR((char *s));
LOCAL	char *	requote		__PR((char *s));
LOCAL	void	compile_ent	__PR((char *tname, BOOL	do_tc, BOOL obsolete_last));
LOCAL	void	read_names	__PR((char *fname, BOOL	do_tc, BOOL obsolete_last));

/*
 * Initialize the tc_flags struct member in "caplist".
 */
LOCAL void
init_clist()
{
	int	i;
	int	flags = 0;
	int	flags2;

	for (i = 0; i < ncaps; i++) {
		flags2 = 0;

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
			/*
			 * OBSOLETE is used together with BOOL, INT or STRING
			 */
			flags |= C_OLD;
		}
		if (streql(caplist[i].tc_var, "CURIOUS")) {
			/*
			 * CURIOUS is a special tag for the tc= entry.
			 */
			flags &= ~C_OLD;
			flags |= C_CURIOUS;
		}
		if (caplist[i].tc_comment[0] != '\0') {
			char	*p = caplist[i].tc_comment;

			p += strlen(p) - 1;

			if (*p == ')' && strchr("NP*", p[-1]) != NULL) {
				while (strchr("NP*", *--p) != NULL) {
					if (*p == 'N')
						flags2 |= C_PARM;
					if (*p == 'P')
						flags2 |= C_PAD;
					if (*p == '*')
						flags2 |= C_PADN;
				}
			}
		}
		caplist[i].tc_flags = flags | flags2;
	}
}


/*
 * Skip past next ':'.
 * If there are two consecutive ':', the returned pointer may point to ':'.
 *
 * A copy from the local function libxtermcap:tgetent.c:tskip()
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

/*
 * A copy from the local function libxtermcap:tgetent.c:tfind()
 */
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

/*
 * Dump the all entries from "caplist".
 * Skip all special entries.
 */
LOCAL void
dumplist()
{
	int	i;
	int	j;
	char	parms[8];

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

		parms[0] = '\0';
		j = 0;
		if (caplist[i].tc_flags & C_PARM)
			parms[j++] = 'N';
		if (caplist[i].tc_flags & C_PAD)
			parms[j++] = 'P';
		if (caplist[i].tc_flags & C_PADN)
			parms[j++] = '*';
		parms[j] = '\0';


		l = strlen(caplist[i].tc_var);
		printf("{\"%s\",	\"%s\",%s\"%s\"},%s	/*%c%c%-3s %s */\n",
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
			parms,
			caplist[i].tc_comment);
	}
}

LOCAL void
usage(ex)
	int	ex;
{
	error("Usage: %s\n", get_progname());
	error("Options:\n");
	error("-e		use termcap entry from TERMCAP environment if present\n");
	error("-help		print this help\n");
	error("-version	print version number\n");
	error("-dumplist	dump internal capability list\n");
	error("-inorder	print caps in order, else print outdated caps last\n");
	error("-noinorder	switch off -inorder, print unknown and outdated caps last\n");
	error("-dooctal	prefer '\\003' before '^C' when creating escaped strings\n");
	error("-docaret	prefer '^M' before '\\r' when creating escaped strings\n");
	error("if=name		input file for termcap compiling\n");
	error("-gnugoto	allow GNU tgoto() format extensions '%%C' and '%%m'.\n");
	error("-nodisabled	do not output disabled termcap entries\n");
	error("-nounknown	do not output unkonwn termcap entries\n");
	error("-nowarn		do not warn about problems that could be fixed\n");
	error("-oneline	output termcap entries in a single line\n");
	error("-s		Output commands to set and export TERM and TERMCAP.\n");
	error("-tc		follow tc= entries and generate cumulative output\n");
	error("-v		increase verbosity level\n");
	error("With if= name, -inorder is default\n");
	exit(ex);
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	*const *cav;
	char	*tbuf;			/* Termcap buffer */
	char	unknown[5*TBUF];	/* Buffer fuer "unknown" Entries */
	char	disabled[5*TBUF];	/* Buffer fuer :..xx: Entries */
	char	*tname = getenv("TERM");
	char	*tcap = getenv("TERMCAP");
	int	slen = 0;
	int	fullen;
	int	strippedlen;
	BOOL	help = FALSE;
	BOOL	prvers = FALSE;
	BOOL	dodump = FALSE;
	BOOL	inorder = FALSE;
	BOOL	noinorder = FALSE;
	BOOL	sflag = FALSE;
	int	verbose = 0;
	BOOL	do_tc = FALSE;
	BOOL	useenv = FALSE;
	char	*infile = NULL;

	save_args(ac, av);

#ifdef	HAVE_SETVBUF
	setvbuf(stdout, obuf, _IOFBF, sizeof (obuf));
#else
#ifdef	HAVE_SETVBUF
	setbuf(stdout, obuf);
#endif
#endif
	init_clist();

	cac = ac;
	cav = av;
	cac--, cav++;
	if (getallargs(&cac, &cav, "help,version,e,dumplist,inorder,noinorder,oneline,v+,s,tc,if*,nodisabled,nounknown,nowarn,dooctal,docaret,gnugoto",
				&help, &prvers,
				&useenv,
				&dodump, &inorder, &noinorder,
				&oneline,
				&verbose,
				&sflag,
				&do_tc,
				&infile,
				&nodisabled, &nounknown,
				&nowarn, &dooctal, &docaret,
				&gnugoto) < 0) {
		errmsgno(EX_BAD, "Bad option '%s'\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prvers) {
		printf("termcap %s %s (%s-%s-%s)\n\n", "1.59", "2021/07/22",
				HOST_CPU, HOST_VENDOR, HOST_OS);
		printf("Copyright (C) 2000-2021 Jörg Schilling\n");
		printf("This is free software; see the source for copying conditions.  There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}

	if (dodump) {
		dumplist();
		exit(0);
	}

	if (infile)
		inorder = TRUE;
	if (noinorder)
		inorder = FALSE;


	if (!useenv && tcap && *tcap != '/')
		*tcap = '\0';

	if (infile) {
		char	env[2048];

		if (tcap)
			*tcap = '\0';
		if (tname)
			*tname = '\0';
		snprintf(env, sizeof (env), "TERMPATH=%s", infile);
		putenv(env);
		read_names(infile, do_tc, !inorder);
		exit(0);
	}

	/*
	 * Check existence & unstripped Termcap len
	 *
	 * TCF_NO_TC	Don't follow tc= entries
	 * TCF_NO_SIZE	Don't get actual ttysize (li#/co#)
	 * TCF_NO_STRIP	Don't strip down termcap buffer
	 *
	 * If called with the -s option, this is the only place where we
	 * retrieve the termcap entry, so use the default settings in this case.
	 */
	if (!sflag)
		tcsetflags((do_tc?0:TCF_NO_TC)|TCF_NO_SIZE|TCF_NO_STRIP);
	if (tgetent(NULL, tname) != 1)
		comerr("no term '%s' found\n", tname);
	tbuf = tcgetbuf();
	fullen = strlen(tbuf);

	if (sflag) {
		char	*p = getenv("SHELL");
		int	is_csh = FALSE;

		if (p) {
			int	len = strlen(p);

			if (len >= 3 && streql(&p[len-3], "csh"))
				is_csh = TRUE;
		}
		if (is_csh) {
			/*
			 * csh
			 */
			printf("set noglob;\n");
			printf("setenv TERM %s;\n", tname);
			printf("setenv TERMCAP '%s';\n", tbuf);
			printf("unset noglob;\n");
		} else {
			/*
			 * Bourne Shell and compatible
			 */
			printf("export TERMCAP TERM;\n");
			printf("TERM=%s;\n", tname);
			printf("TERMCAP='%s';\n", tbuf);
		}
		exit(0);
	}

	/*
	 * Get Stripped len
	 */
	tcsetflags((do_tc?0:TCF_NO_TC)|TCF_NO_SIZE);
	tgetent(NULL, tname);
	tbuf = tcgetbuf();
	strippedlen = strlen(tbuf);

	if (verbose > 0)
		checkentries(tname, &slen);

	if (verbose > 1) {
		printf("tbuf: '%s'\n", tbuf);
		printf("full tbuf len: %d stripped tbuf len: %d\n", fullen, strippedlen);
		printf("string length: %d\n", slen);
	}

	checkbad(tname, unknown, disabled);
	outcap(tname, unknown, disabled, !inorder);

	return (0);
}

/*
 * Check entries for correct type and print them
 */
LOCAL void
checkentries(tname, slenp)
	char	*tname;
	int	*slenp;
{
	char	stbuf[10*TBUF];	/* String buffer zum zaehlen */
	char	*sbp;		/* String buffer Ende */
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

	b = strlen(tbuf);
	if (b > 2*TBUF) {
		error("%s: termcap entry too long (%d bytes).\n", tname, b);
		return;
	}

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
				char	*n = tskip(p2);

				p = &p2[1];
				n--;			/* Backstep from ':' */
				if ((n - p) >= TBUF) {
					error("NOTICE(%s). string '%.*s' too long.\n",
					tname, (int)(n - p2)+2, p2-2);
					break;
				}
				strlcpy(sbp, p, n-p+1);
				p = sbp;	/* The tc= parameter */
			} else
				break;
		} else {
			char	*e = tfind(tbuf, caplist[i].tc_name);
			char	*n = tskip(e);

			/*
			 * e and n are always != NULL.
			 */
			p = NULL;
			if (*e == '=' && (n - e) > 1024) {
				/*
				 * Warn if the string is > 1024, but let up to
				 * 4096 bytes appear to be able to count the
				 * real length.
				 */
				error("NOTICE(%s). string '%.*s' longer than %d.\n",
					tname, (int)(n - e)+1, e-2, 1024);
			}
			if (*e != '=') {
				/*
				 * Not a string type entry.
				 */
				/* EMPTY */
				;
			} else if ((n - e) > 4*TBUF) {
				error("NOTICE(%s). string '%.*s' too long.\n",
					tname, (int)(n - e)+1, e-2);
			} else if ((sbp - stbuf) > 4*TBUF) {
				error(
				"NOTICE(%s). '%.*s' string table overflow.\n",
					tname, (int)(n - e)+1, e-2);
			} else {
				p = tgetstr(caplist[i].tc_name, &sbp);
			}
		}
		if (caplist[i].tc_name[0] == 'm' &&
		    caplist[i].tc_name[1] == 'a' &&
		    (caplist[i].tc_flags & C_STRING) == 0) {
			/*
			 * The "ma" entry exists as numeric and string
			 * capability. We are currently not looking for
			 * the string capability, so ignore this entry.
			 */
			/* EMPTY */
			;
		} else if (p) {
			/*
			 * XXX Option zum Deaktivieren des Quoten
			 */
			p = quote(p);
			printf("'%s' -> '%s'", caplist[i].tc_name, p);
			printf("		%s", caplist[i].tc_comment);

			if ((caplist[i].tc_flags & C_STRING) == 0)
				printf(" -> WARNING: TYPE mismatch");
			printf("\n");
			if (i == ncaps -1)
				i--;
			continue;
		}
		b = tgetnum(caplist[i].tc_name);

		if (caplist[i].tc_name[0] == 'm' &&
		    caplist[i].tc_name[1] == 'a' &&
		    (caplist[i].tc_flags & C_INT) == 0) {
			/*
			 * The "ma" entry exists as numeric and string
			 * capability. We are currently not looking for
			 * the numeric capability, so ignore this entry.
			 */
			/* EMPTY */
			;
		} else if (b >= 0) {
			printf("'%s' -> %d", caplist[i].tc_name, b);
			printf("		%s", caplist[i].tc_comment);
			if ((caplist[i].tc_flags & C_INT) == 0)
				printf(" -> WARNING: TYPE mismatch");
			printf("\n");
			continue;
		}

		b = tgetflag(caplist[i].tc_name);
		printf("'%s' -> %s", caplist[i].tc_name, b?"TRUE":"FALSE");
		printf("		%s", caplist[i].tc_comment);
		if ((caplist[i].tc_flags & C_BOOL) == 0)
			printf(" -> WARNING: TYPE mismatch");
		printf("\n");
		continue;

	}
	if (slenp)
		*slenp = sbp - stbuf;
}

LOCAL char *
type2str(type)
	int	type;
{
	switch (type & 0x07) {

	case C_INT:	return ("INT");
	case C_STRING:	return ("STRING");
	case C_BOOL:	return ("BOOL");
	default:	return ("<unknown>");
	}
}

/*
 * Check for bad termcap entries
 */
LOCAL void
checkbad(tname, unknown, disabled)
	char	*tname;
	char	*unknown;
	char	*disabled;
{
	char	ent[3];		/* Space to hold termcap entry */
	char	*up;		/* Unknown buffer Ende */
	char	*dp;		/* Disabled buffer Ende */
	int	rb;		/* Fuer bool/int Werte */
	char	*rp;		/* Fuer string Werte */
	BOOL	found;		/* Correct type already found */
	int	i;
	char	*p;
	char	*p2;
	char	*xp;
	char	*tbuf = tcgetbuf();
	BOOL	out_tty = isatty(STDOUT_FILENO);

	p = tskip(tbuf);

	i = strlen(tbuf);
	if (i > 2*TBUF) {
		printf("# BAD(%s). Skipping long entry (%d bytes): '%s'\n",
					tname,
					i,
					tbuf);
		if (!out_tty)
		error("BAD(%s). Skipping long entry (%d bytes): '%.*s'\n",
					tname,
					i,
					(int)(p - tbuf - 1), tbuf);
		return;
	}

	up = unknown;
	dp = disabled;
	while (*p) {
		while (*p == ':')
			p = tskip(p);
		if (*p == '\0')
			break;
		/*
		 * Avoid to warn about bad termcap entries as long as we
		 * implement support for the terminfo escape '\:'.
		 */
		if (p > (tbuf+1) && p[-1] == ':' && p[-2] == '\\') {
			p = tskip(p);
			continue;
		}
		if (p[1] == '\0' ||
		    p[1] == ':') {
			if (p[0] == '\t' ||	/* This is an indented line */
			    p[0] == ' ') {	/* also ignore single space */
				p = tskip(p);
				continue;
			}
			printf("# NOTICE(%s). Short entry (':%c%s') removed\n",
				tname, p[0], p[1]?":":"");
			if (!out_tty)
			error("NOTICE(%s). Short entry (':%c%s') removed\n",
				tname, p[0], p[1]?":":"");
			p = tskip(p);
			continue;
		}
		checkquote(tname, p);
		if (p[2] != ':' && p[2] != '@' && p[2] != '#' && p[2] != '=') {
			p2 = tskip(p);
			if (p[0] == ' ' || p[0] == '\t') {
				printf("# BAD(%s). Skipping %sblank entry: '%.*s'\n",
							tname,
							(p[1] == ' ' || p[1] == '\t')?
							"":"partially ",
							(int)(p2 - p - 1), p);
				if (!out_tty)
				error("BAD(%s). Skipping %sblank entry: '%.*s'\n",
							tname,
							(p[1] == ' ' || p[1] == '\t')?
							"":"partially ",
							(int)(p2 - p - 1), p);
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
				printf("# NOTICE(%s). Disabled entry: '%.*s'\n",
							tname, (int)(p2 - p - 1), p);
				if (!out_tty)
				error("NOTICE(%s). Disabled entry: '%.*s'\n",
							tname, (int)(p2 - p - 1), p);
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
					tname, p[2], p[0], p[1], p[2], (int)(p2 - xp - 1), xp);
			if (!out_tty)
			error("BAD(%s). Illegal entry (3rd char '%c' for '%c%c%c'): '%.*s'\n",
					tname, p[2], p[0], p[1], p[2], (int)(p2 - p - 1), p);
			p = tskip(p);
			continue;
		}
		if ((p[0] == ' ' || p[0] == '\t') &&
		    (p[1] == ' ' || p[1] == '\t')) {
			p2 = tskip(p);
			printf("# BAD(%s). Skipping blank entry: '%.*s'\n",
						tname, (int)(p2 - p - 1), p);
			if (!out_tty)
			error("BAD(%s). Skipping blank entry: '%.*s'\n",
							tname, (int)(p2 - p - 1), p);
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

			if (caplist[i].tc_name[0] == 'm' &&
			    caplist[i].tc_name[1] == 'a' &&
			    (caplist[i].tc_flags & C_STRING) == 0) {
				/*
				 * We found the "ma" entry in the numeric
				 * variant. If this was a string cap, continue
				 * searching for the matching caplist entry.
				 * The "ma=" entry is past "ma#" in our list.
				 */
				if (p[2] == '=')
					continue;
			}

			if (streql(ent, caplist[i].tc_name))
				break;
		}
		if (i == ncaps) {
			p2 = tskip(p);
			printf("# NOTICE(%s). Unknown entry ('%s'): '%.*s'\n",
						tname, ent, (int)(p2 - p - 1), p);
			if (!out_tty)
			error("NOTICE(%s). Unknown entry ('%s'): '%.*s'\n",
						tname, ent, (int)(p2 - p - 1), p);
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
		} else if ((caplist[i].tc_flags & (C_STRING|C_PARM)) == (C_STRING|C_PARM)) {

			if (p[2] == '=') {
				char	buf[5*TBUF];
				char	*bp = buf;
				char	*val = tdecode(&p[3], &bp);

				checkgoto(tname, ent, val, 0, 0);
			}
		}

		if (i == ncaps) {
			/*
			 * This is an unknown entry that has already been
			 * handled above. We cannot check the type for unknown
			 * entries, so continue to check the other entries.
			 */
			p = tskip(p);
			continue;
		}
		found = FALSE;
		rp = tgetstr(caplist[i].tc_name, NULL);
		if (rp)
			found = TRUE;
		if (caplist[i].tc_name[0] == 'm' &&
		    caplist[i].tc_name[1] == 'a' &&
		    (caplist[i].tc_flags & C_STRING) == 0) {
			/* EMPTY */
			;
		} else if (rp) {
			if ((caplist[i].tc_flags & C_STRING) == 0) {
				p2 = tskip(p);
				printf("# BAD(%s). Type mismatch '%s' in '%.*s' is STRING should be %s\n",
					tname, ent, (int)(p2 - p - 1), p, type2str(caplist[i].tc_flags));
			}
		}
		if (rp) {		/* tgetstr() return was malloc()ed */
			free(rp);
			rp = NULL;
		}
		rb = tgetnum(caplist[i].tc_name);
		if (rb >= 0)
			found = TRUE;
		if (caplist[i].tc_name[0] == 'm' &&
		    caplist[i].tc_name[1] == 'a' &&
		    (caplist[i].tc_flags & C_INT) == 0) {
			/* EMPTY */
			;
		} else if (rb >= 0) {
			if ((caplist[i].tc_flags & C_INT) == 0) {
				p2 = tskip(p);
				printf("# BAD(%s). Type mismatch '%s' in '%.*s' is INT should be %s\n",
					tname, ent, (int)(p2 - p - 1), p, type2str(caplist[i].tc_flags));
			}
		}

		if (!found && p[2] != '@') {
			rb = tgetflag(caplist[i].tc_name);
			if ((caplist[i].tc_flags & C_BOOL) == 0) {
				p2 = tskip(p);
				if (rb == 0 && p[2] != '@') {
					printf("# NOTICE(%s). Canceled entry '%s@' followed by '%.*s' expected type %s\n",
						tname, ent, (int)(p2 - p - 1), p, type2str(caplist[i].tc_flags));
				} else {
					printf("# BAD(%s). Type mismatch '%s' in '%.*s' is BOOL should be %s\n",
						tname, ent, (int)(p2 - p - 1), p, type2str(caplist[i].tc_flags));
				}
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

LOCAL int	itotype[6] = { 0, C_BOOL, C_INT, C_STRING, 0, C_TC|C_STRING };
#ifdef	DEBUG
LOCAL char	*itoname[6] = { "0", "C_BOOL", "C_INT", "C_STRING", "C_UNKNOWN", "C_TC" };
#endif

LOCAL void
outcap(tname, unknown, disabled, obsolete_last)
	char	*tname;
	char	*unknown;
	char	*disabled;
	BOOL	obsolete_last;	/* obsolete_last == !inorder */
{
	char	stbuf[10*TBUF];	/* String buffer zum zaehlen */
	char	line[10*TBUF];	/* Fuer Einzelausgabe */
	char	*sbp;		/* String buffer Ende */
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


	b = strlen(tbuf);
	if (b > 2*TBUF) {
		return;
	}

	p = strchr(tbuf, ':');
	if (p)
		i = p - tbuf;
	else
		i = strlen(tbuf);
	printf("%.*s:", i, tbuf);	/* Print Terminal name/label */
	llen = i + 1;

	sbp = stbuf;
	flags = 0;
	p2 = tbuf;

	/*
	 * If obsolete_last is FALSE, then this loop goes from j=1..5,
	 * else, there is only one loop with j=-1 as the loop stops with j==0.
	 */
	for (j = obsolete_last ? -1:1; j != 0 && j <= 5; j++)
	for (i = 0; i < ncaps; i++) {
#ifdef	DEBUG
		flush();
		error("caplist[%d]->'%c%c' (%s)->'%.10s' j=%d\n",
			i, caplist[i].tc_name[0], caplist[i].tc_name[1],
			itoname[j], tfind(tbuf, caplist[i].tc_name), j);
#endif
		/*
		 * Skip meta entries.
		 */
		if (caplist[i].tc_name[0] == '-' &&
		    caplist[i].tc_name[1] == '-')
			continue;

		/*
		 * Print unknown entries in order at the end of the respective
		 * block. Do not mark inline them and requote the strings.
		 */
		if (j >= 1 && j <= 3 && i == ncaps -1) {
			char	*px = unknown;
			int	t = itotype[j];
			char	*val = NULL;	/* Keep GCC happy */

			while (*px) {
				pe = px;
				px = tskip(pe);

				if (t == C_BOOL) {
					if (pe[2] != ':' && pe[2] != '@')
						continue;
				} else if (t == C_INT) {
					if (pe[2] != '#')
						continue;
				} else {
					if (pe[2] != '=')
						continue;
					val = requote(&pe[3]);
				}
				p = pe;
				curlen = px - pe;
				if (p[2] == '=')
					curlen = strlen(val) + 4;
				if (curlen <= 0)
					break;
				if (flags != t && !oneline) {
					printf("\\\n\t:");
					llen = 9;
					flags = t;
				}
				if ((llen > 9) && ((llen + curlen) >= 79 &&
				   !oneline)) {
					printf("\\\n\t:");
					llen = 9 + curlen;
				} else {
					llen += curlen;
				}
				if (p[2] == '=')
					printf("%.2s=%.*s:", p, curlen, val);
				else
					printf("%.*s", curlen, p);
			}
		}
		/*
		 * If j < 0, sort order is order from caplist array
		 * and thus obsolete entries appear past non obsolete entries.
		 *
		 * If j > 0, sort order is BOOL -> INT -> STRING
		 */
		if (j > 0 && (caplist[i].tc_flags & (C_BOOL|C_INT|C_STRING|C_TC)) != itotype[j])
			continue;

		if ((caplist[i].tc_flags & (C_BOOL|C_INT|C_STRING)) == 0) {
			if (streql(caplist[i].tc_var, "unknown")) {
				if (j > 0)	/* Printed in order before */
					continue;
				if (unknown[0] == '\0')
					continue;
				if (nounknown)
					continue;

				if ((llen > 9 || flags == 0) && !oneline)
					printf("\\\n\t:");
				if (!oneline) {
					printf("%s%s",
						"..UNKNOWN:\\\n\t:",
						unknown);
				}
				llen = 99;
			} else {
				if (disabled[0] == '\0')
					continue;
				if (nodisabled)
					continue;

				if ((llen > 9 || flags == 0) && !oneline)
					printf("\\\n\t:");
				if (!oneline) {
					printf("%s%s",
						"..DISABLED:\\\n\t:",
						disabled);
				}
				llen = 99;
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
			/*
			 * tfind() first calls tskip() but the last printed
			 * entry may be just before the "tc=" enty and did
			 * already call p2 = tskip(pe). In this case, we must
			 * not call tfind().
			 */
			if (strncmp(p2, "tc=", 3) == 0)
				p2 = &p2[2];
			else
				p2 = tfind(p2, "tc");
			if (p2 == NULL)
				break;				/* End of loop */
			if (*p2 == '=') {
				p = &p2[1];
				strcpy(sbp, p);
				if ((p = strchr(sbp, ':')) != NULL)
					*p = '\0';
				p = sbp;
			} else {
				break;				/* End of loop */
			}
			curlen = sprintf(line, "%s=%s:", caplist[i].tc_name, p);
			i--;
			goto printit;
		} else {
			p = tgetstr(caplist[i].tc_name, &sbp);
		}

		if (p) {
			if (caplist[i].tc_flags & C_INT) {	/* check type for "ma" */
				if (caplist[i].tc_name[0] == 'm' &&
				    caplist[i].tc_name[1] == 'a') {
					/*
					 * The "ma" entry exists as numeric and
					 * string capability. We are currently
					 * not looking for the string
					 * capability, so try to check for int.
					 */
					goto trynum;
				}
			}

			if (caplist[i].tc_flags & C_STRING) {	/* check type for "ma" */
				curlen = sprintf(line, "%s=%s:", caplist[i].tc_name, quote(p));
				goto printit;
			} else {
				p2 = tskip(pe);
				error("%s: Illegal entry '%s' '%.*s' (should not be a string)\n",
						tname, caplist[i].tc_name,
						(int)(p2 - pe - 1), pe);
				continue;
			}
		}
trynum:
		b = tgetnum(caplist[i].tc_name);
		if (b >= 0) {
			if (caplist[i].tc_flags & C_INT) {	/* check type for "ma" */
				curlen = sprintf(line, "%s#%d:", caplist[i].tc_name, b);
				goto printit;
			} else {
				p2 = tskip(pe);
				if (caplist[i].tc_name[0] == 'm' &&
				    caplist[i].tc_name[1] == 'a') {
					/*
					 * The "ma" entry exists as numeric and
					 * string capability. We are currently
					 * not looking for the string
					 * capability. We will check the string
					 * entry later in the caplist[i] loop.
					 */
					continue;
				}
				error("%s: Illegal entry '%s' '%.*s' (should not be a number)\n",
						tname, caplist[i].tc_name,
						(int)(p2 - pe - 1), pe);
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
						(int)(p2 - pe - 1), pe);
				continue;
			}
		}
		p2 = tskip(pe);
		if (caplist[i].tc_flags & C_INT) {	/* check type for "ma" */
			if (caplist[i].tc_name[0] == 'm' &&
			    caplist[i].tc_name[1] == 'a' &&
			    *pe == '=') {
				/*
				 * This entry will be checked later.
				 * The "ma=" entry is past "ma#" in our list.
				 */
				continue;
			}
		}
		error("%s: Illegal entry '%s' '%.*s'\n",
						tname, caplist[i].tc_name,
						(int)(p2 - pe - 1), pe);
		continue;

printit:
		if (!oneline &&
		    flags != (caplist[i].tc_flags & (C_BOOL|C_INT|C_STRING|C_TC))) {
			printf("\\\n\t:");
			llen = 9;
			flags = caplist[i].tc_flags & (C_BOOL|C_INT|C_STRING|C_TC);
		}

		/*
		 * If j > 0, sort order is BOOL -> INT -> STRING
		 * Do not print the OBSOLETE header in this case.
		 */
		if (!oneline &&
		    j < 0 && (caplist[i].tc_flags & C_OLD) != 0) {
			if (!didobsolete) {
				if (llen > 9)
					printf("\\\n\t:");
				if (!oneline) {
					printf("..OBSOLETE:\\\n\t:");
				}
				llen = 9;
				didobsolete = TRUE;
			}
		}

/*error("line: '%s', llen: %d curlen: %d sum: %d\n", line, llen, curlen, llen + curlen);*/
		p = line;
		curlen = strlen(p);
		if ((llen > 9) && ((llen + curlen) >= 79) && !oneline) {
			printf("\\\n\t:");
			llen = 9 + curlen;
		} else {
			llen += curlen;
		}
		printf("%s", p);
	}
	printf("\n");
	flush();
}


#define	OBUF_SIZE	80

/*
 * Perform string preparation/conversion for cursor addressing.
 * The string cm contains a format string.
 *
 * A copy from the local function libxtermcap:tgoto.c:tgoto()
 */
LOCAL char *
checkgoto(tname, ent, cm, col, line)
	char	*tname;
	char	*ent;
	char	*cm;
	int	col;
	int	line;
{
	static	char	outbuf[OBUF_SIZE];	/* Where the output goes to */
		char	xbuf[10];		/* for %. corrections	    */
	register char	*op = outbuf;
	register char	*p = cm;
	register int	c;
	register int	val = line;
		int	usecol = 0;
		BOOL	out_tty = isatty(STDOUT_FILENO);
		BOOL	hadbad = FALSE;

	if (p == 0) {
badfmt:
		/*
		 * Be compatible to 'vi' in case of bad format.
		 */
		return ("OOPS");
	}
	xbuf[0] = 0;
	while ((c = *p++) != '\0') {
		if ((op + 5) >= &outbuf[OBUF_SIZE])
			goto overflow;

		if (c != '%') {
			*op++ = c;
			continue;
		}
		switch (c = *p++) {

		case '%':		/* %% -> %			*/
					/* This is from BSD		*/
			*op++ = c;
			continue;

		case 'd':		/* output as printf("%d"...	*/
					/* This is from BSD (use val)	*/
			if (val < 10)
				goto onedigit;
			if (val < 100)
				goto twodigits;
			/*FALLTHROUGH*/

		case '3':		/* output as printf("%03d"...	*/
					/* This is from BSD (use val)	*/
			if (val >= 1000) {
				*op++ = '0' + (val / 1000);
				val %= 1000;
			}
			*op++ = '0' + (val / 100);
			val %= 100;
			/*FALLTHROUGH*/

		case '2':		/* output as printf("%02d"...	*/
					/* This is from BSD (use val)	*/
		twodigits:
			*op++ = '0' + val / 10;
		onedigit:
			*op++ = '0' + val % 10;
		nextparam:
			usecol ^= 1;
		setval:
			val = usecol ? col : line;
			continue;

		case 'C': 		/* For c-100: print quotient of	*/
					/* value by 96, if nonzero,	*/
					/* then do like %+.		*/
					/* This is from GNU (use val)	*/
			if (!gnugoto)
				goto badchar;
			if (val >= 96) {
				*op++ = val / 96;
				val %= 96;
			}
			/*FALLTHROUGH*/

		case '+':		/* %+x like %c but add x before	*/
					/* This is from BSD (use val)	*/
			val += *p++;
			/*FALLTHROUGH*/

		case '.':		/* output as printf("%c" but...	*/
					/* This is from BSD (use val)	*/
			if (usecol || UP)  {
				/*
				 * We assume that backspace works and we don't
				 * need to test for BC too.
				 *
				 * If you did not call stty tabs while termcap
				 * is used you will get other problems, so we
				 * exclude tab from the execptions.
				 */
				while (val == 0 || val == '\004' ||
					/* val == '\t' || */ val == '\n') {

					strcat(xbuf,
						usecol ? (BC?BC:"\b") : UP);
					val++;
				}
			}
			*op++ = val;
			goto nextparam;

		case '>':		/* %>xy if val > x add y	*/
					/* This is from BSD (chng state)*/

			if (val > *p++)
				val += *p++;
			else
				p++;
			continue;

		case 'B':		/* convert to BCD char coding	*/
					/* This is from BSD (chng state)*/

			val += 6 * (val / 10);
			continue;

		case 'D':		/* weird Delta Data conversion	*/
					/* This is from BSD (chng state)*/

			val -= 2 * (val % 16);
			continue;

		case 'i':		/* increment row/col by one	*/
					/* This is from BSD (chng state)*/
			col++;
			line++;
			val++;
			continue;

		case 'm':		/* xor both parameters by 0177	*/
					/* This is from GNU (chng state)*/
			if (!gnugoto)
				goto badchar;
			col ^= 0177;
			line ^= 0177;
			goto setval;

		case 'n':		/* xor both parameters by 0140	*/
					/* This is from BSD (chng state)*/
			col ^= 0140;
			line ^= 0140;
			goto setval;

		case 'r':		/* reverse row/col		*/
					/* This is from BSD (chng state)*/
			usecol = 1;
			goto setval;

		default:
		badchar:
			printf("# BAD(%s). Bad format '%%%c' in '%s=%s'\n", tname, c, ent, quote(cm));
			if (!out_tty)
			error("BAD(%s). Bad format '%%%c' in '%s=%s'\n", tname, c, ent, quote(cm));
			hadbad = TRUE;
/*			goto badfmt;*/
		}
	}
	/*
	 * append to output if there is space...
	 */
	if ((op + strlen(xbuf)) >= &outbuf[OBUF_SIZE]) {
overflow:
		printf("# BAD(%s). Buffer overflow in '%s=%s'\n", tname, ent, quote(cm));
		if (!out_tty)
		error("BAD(%s). Buffer overflow in '%s=%s'\n", tname, ent, quote(cm));
		return ("OVERFLOW");
	}
	if (hadbad)
		goto badfmt;

	for (p = xbuf; *p; )
		*op++ = *p++;
	*op = '\0';
	return (outbuf);
}

LOCAL	char	_quotetab[]	= "E^^\\\\n\nr\rt\tb\bf\f";

#define	isoctal(c)	((c) >= '0' && (c) <= '7')

LOCAL char *
checkquote(tname, s)
	char	*tname;
	char	*s;
{
static			char	out[5*TBUF];
			char	nm[16];
			int	i;
	register	Uchar	c;
	register	Uchar	*ep = (Uchar *)s;
	register	Uchar	*bp;
	register	Uchar	*tp;
			char	*p;
			BOOL	out_tty = isatty(STDOUT_FILENO);

	out[0] = '\0';
	if (s[0] == ' ' || s[0] == '\t')
		return (out);

	ep = (Uchar *)strchr(s, '=');
	if (ep == NULL)
		return (out);
	i = ep - (Uchar *)s;
	ep++;
	p = tskip(s);
	if (ep > (Uchar *)p)
		return (out);

	strlcpy(nm, s, sizeof (nm));
	if (i < sizeof (nm))
		nm[i] = '\0';

	bp = (Uchar *)out;

	for (; (c = *ep++) && c != ':'; *bp++ = c) {
		if (c == '^') {
			c = *ep++;
			if (c == '\0') {
				printf(
				"# NOTICE(%s). ^ quoting followed by no character in '%s'\n",
							tname, s);
				if (!out_tty)
				error("NOTICE(%s). ^ quoting followed by no character in '%s'\n",
							tname, s);
				break;
			}
			c &= 0x1F;
		} else if (c == '\\') {
			c = *ep++;
			if (c == '\0') {
				printf(
				"# NOTICE(%s). \\ quoting followed by no character in '%s'\n",
							tname, s);
				if (!out_tty)
				error("NOTICE(%s). \\ quoting followed by no character in '%s'\n",
							tname, s);
				break;
			}
			if (isoctal(c)) {
				for (c -= '0', i = 3; --i > 0 && isoctal(*ep); ) {
					c <<= 3;
					c |= *ep++ - '0';
				}
				if (c == 0) {
					char	*p2 = tskip(s);
					int	len;
					int	pos = (char *)ep - s;

					len = p2 - s - (*p2?1:0);
					pos -= 4-i;
					if (!nowarn && nm[0] != '.') {
					printf("# NOTICE(%s). NULL char (fixed) in entry ('%s') at abs position %d in '%.*s'\n",
							tname, nm, pos, len, s);
					if (!out_tty)
					error("NOTICE(%s). NULL char (fixed) in entry ('%s') at abs position %d in '%.*s'\n",
							tname, nm, pos, len, s);
					}
				}
#ifdef	__checkoctal__
				if (i > 0) {
					char	*p2 = tskip(s);
					int	len;
					int	pos = (char *)ep - s;

					len = p2 - s - (*p2?1:0);
					printf(
					"# NOTICE(%s). Nonoctal char '%c' in entry ('%s') at position %d (abs %d) in '%.*s'\n",
							tname, *ep, nm, 4-i, pos, len, s);
					if (!out_tty)
					error("NOTICE(%s). Nonoctal char '%c' in entry ('%s') at position %d (abs %d) in '%.*s'\n",
							tname, *ep, nm, 4-i, pos, len, s);
				}
#endif
			} else {
				for (tp = (Uchar *)_quotetab; *tp; tp++) {
					if (*tp++ == c) {
						c = *tp;
						break;
					}
				}
				/*
				 * Terminfo quotes not in termcap:
				 * \a	->	'^G'
				 * \e	->	'^['
				 * \:	->	':'
				 * \,	->	','
				 * \s	->	' '
				 * \l	->	'\n'
				 */
				if (*tp == '\0') {
					char	*p2 = tskip(s);
					int	len;
					int	pos = (char *)&ep[-1] - s;

					len = p2 - s - (*p2?1:0);
					if (!nowarn) {
					printf("# NOTICE(%s). Badly quoted char '\\%c' %sin ('%s') at abs position %d in '%.*s'\n",
							tname, c, strchr("ae:,sl", c)?"(fixed) ":"", nm, pos, len, s);
					if (!out_tty)
					error("NOTICE(%s). Badly quoted char '\\%c' %sin ('%s') at abs position %d in '%.*s'\n",
							tname, c, strchr("ae:,sl", c)?"(fixed) ":"", nm, pos, len, s);
					}
				}
			}
		}
	}
	*bp++ = '\0';
	return (out);
}

LOCAL char *
quote(s)
	char	*s;
{
static	char	out[10*TBUF];
	char	*p1;
	char	*p2;
	unsigned char	c;

	for (p1 = s, p2 = out; *p1; ) {
		c = *p1++;
		if (c == 033) {		/* ESC -> \E */
			*p2++ = '\\';
			*p2++ = 'E';
		} else if (c == '\\') {	/* \ -> \\ */
			*p2++ = '\\';
			*p2++ = '\\';
		} else if (c == '^') {	/* ^ -> \^ */
			*p2++ = '\\';
			*p2++ = '^';
		} else if (!docaret && c == '\r') {	/* CR -> \r */
			*p2++ = '\\';
			*p2++ = 'r';
		} else if (!docaret && c == '\n') {	/* NL -> \n */
			*p2++ = '\\';
			*p2++ = 'n';
		} else if (!docaret && c == '\t') {	/* TAB -> \t */
			*p2++ = '\\';
			*p2++ = 't';
		} else if (!docaret && c == '\b') {	/* BS -> \b */
			*p2++ = '\\';
			*p2++ = 'b';
		} else if (!docaret && c == '\f') {	/* FF -> \f */
			*p2++ = '\\';
			*p2++ = 'f';
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
 *   A number of escape sequences are  provided  in  the  string-
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
	char	buf[5*TBUF];
	char	*bp = buf;

	tdecode(s, &bp);
	return (quote(buf));
}

LOCAL void
compile_ent(tname, do_tc, obsolete_last)
	char	*tname;
	BOOL	do_tc;
	BOOL	obsolete_last;
{
	char	unknown[5*TBUF];	/* Buffer fuer "unknown" Entries */
	char	disabled[5*TBUF];	/* Buffer fuer :..xx: Entries */

	tcsetflags((do_tc?0:TCF_NO_TC)|TCF_NO_SIZE|TCF_NO_STRIP);
	if (tgetent(NULL, tname) != 1)
		return;
/*	tbuf = tcgetbuf();*/
/*	strippedlen = strlen(tbuf);*/

/*	checkentries(tname, &slen);*/

/*	printf("tbuf: '%s'\n", tbuf);*/
/*printf("full tbuf len: %d stripped tbuf len: %d\n", fullen, strippedlen);*/
/*printf("string length: %d\n", slen);*/

	checkbad(tname, unknown, disabled);
	outcap(tname, unknown, disabled, obsolete_last);
}


#define	TRDBUF	8192
#define	TMAX	1024
#define	TINC	1

LOCAL void
read_names(fname, do_tc, obsolete_last)
	char	*fname;
	BOOL	do_tc;
	BOOL	obsolete_last;
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
	tfd = open(fname, O_RDONLY);
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
				free(tbuf);
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
				compile_ent(name, do_tc, obsolete_last);
				tcsetflags(TCF_NO_TC|TCF_NO_SIZE);
			} else {
				/*
				 * This line is not a termcap entry
				 */
				printf("%s\n", tbuf);
			}
		} else {
			/*
			 * This line is comment
			 */
			printf("%s\n", tbuf);
		}
		ep = bp;
	}
}
