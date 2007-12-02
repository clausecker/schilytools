/* @(#)macro.c	1.38 06/09/26 Copyright 1984-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)macro.c	1.38 06/09/26 Copyright 1984-2004 J. Schilling";
#endif
/*
 *	The macro package for VED
 *
 *	Copyright (c) 1984-2004 J. Schilling
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

/*
 * If a macro is found, it is loaded into 'macstr' and then read from 'macstr'
 * and executed character by character.
 * A maximum recursion depth of M_DEPTH macros can be executed at a time.
 *
 * The following external routines are available:
 *
 *	macro_init	- Init the package and load the macro file.
 *	vxmac		- Look for a macro and push it on execution stack.
 *			  Also sets 'mflag' as an intication that 'gmacro()'
 *			  should be used to read further characters.
 *	vmac		- Load the temporary macro on the execution stack
 *	gmacro		- Get the next character from the macro execution stack
 *			  Returns '0' and resets 'mflag' at the end of a
 *			  translation.
 *	vemac		- Edit the macro file from VED and re-init macro pakage
 *	macro_reinit	- Check if the macro package should be re-initialized.
 *			  Do the re-initialization if needed.
 *
 * The global 'mflag' is used in io.c to know whether the input should be
 * taken from the macro execution stack.
 */

#include "ved.h"
#include <schily/string.h>
#include <schily/errno.h>
#ifndef	JOS
#	include <pwd.h>
#endif

#define	M_NAMELEN	256
#define	M_STRINGLEN	8192
#define	M_DEPTH		10

extern	int	mflag;

LOCAL	int	mac_init = 0;
LOCAL	int	mac_level = 0;

typedef	struct	{
	char	*m_str;
	char	*m_start;
	int	m_mult;
} macs_t;

LOCAL	macs_t	macs[M_DEPTH];

LOCAL	char	*macstr;
LOCAL	char	*mac_start;
LOCAL	char	*home;

typedef struct m_node mac_t;

struct m_node {
	mac_t	*m_next;
	char	*m_name;
	char	*m_string;
};

LOCAL	mac_t	*first_macro;

EXPORT	char *	myhome		__PR((void));
EXPORT	void	macro_init	__PR((ewin_t *wp));
EXPORT	void	vxmac		__PR((ewin_t *wp));
EXPORT	void	vmac		__PR((ewin_t *wp));
EXPORT	int	gmacro		__PR((void));
EXPORT	void	vemac		__PR((ewin_t *wp));
EXPORT	void	macro_reinit	__PR((ewin_t *wp));
LOCAL	void	add_node	__PR((char *mn, char *ms));
LOCAL	char *	get_macro	__PR((ewin_t *wp, int c));

/*
 * Try to get the user's home directory.
 */
EXPORT char *
myhome()
{
#ifndef	JOS
	struct	passwd	*pw;
#else
		char	*fields[7];
	extern	char	*getuname();
#endif

	if (home)
		return (home);

	if ((home = getenv("HOME")) == NULL) {
#ifdef	JOS
		if (findline("/etc/passwd", ':',
					getuname(getuid()), 0, fields, 6) == 1)
			home = fields[5];
#else
		if ((pw = getpwuid(getuid())) != NULL)
			home = pw->pw_dir;
#endif
		else
			home = "/tmp";
	}
	return (home);
}

/*
 * Initialize the macro package and load the macro file.
 */
EXPORT void
macro_init(wp)
	ewin_t	*wp;
{
	register FILE	*f;
	register int	state;	/* 0 == name, 1 == string, 2 == comment */
	register int	n;
	register char	*s;
	register char	lc;
	register int	c;
		char	fname[1024];
		char	m_name[M_NAMELEN + 1];
		char	m_string[M_STRINGLEN + 1];

	first_macro = 0;

	snprintf(fname, sizeof (fname), "%s%s", myhome(), "/.vedmac");
	if ((f = fileopen(fname, "rb")) == (FILE *) NULL) {
		if (geterrno() == ENOENT)
			return;
		errmsg("Can not open '%s'.\r\n", fname);
		sleep(1);
		return;
	}

	state = 0; n = M_NAMELEN; lc = 0;
	s = m_name;
	for (;;) {
		c = getc(f);
		if (c == EOF)
			break;

		if (state == 0 && c == '\n') {	/* empty line / ln without : */
			if (s != m_name) {
				*s = '\0';
				writeerr(wp, "%.8s Bad macro name.", m_name);
				sleep(1);
				lc = 0;
				n = M_NAMELEN;
				s = m_name;
			}
			continue;
		}

		if (n-- < 0) {
			*s = '\0';
			writeerr(wp, "%.8s: Bad macro form.", m_name);
			sleep(1);
			/*
			 * Eat rest of line.
			 */
			while ((c = getc(f)) != EOF) {
				if (c == '\n')
					break;
			}
			lc = 0; s = m_name; n = M_NAMELEN; state = 0;
			continue;
		}

		if (state == 2) {		/* comment */
			if (c == '\n') {
				lc = 0; s = m_name; n = M_NAMELEN; state = 0;
			}
			continue;
		}
		if (c == ':') {
			if (lc != '\\') {
				*s = 0;
				if (state == 0)
					s = m_string;
				else
					add_node(m_name, m_string);
				n = M_STRINGLEN; state++;
				continue;
			} else {
				s--; *s++ = lc = (char)c;
			}
		} else {
			*s++ = lc = (char)c;
		}
	}
	fclose(f);
}

/*
 * Look for macro and push it on the macro execution stack if found.
 */
EXPORT void
vxmac(wp)
	ewin_t	*wp;
{
	char	*mac;

	wp->eflags &= ~COLUPDATE;
	if (mac_level >= M_DEPTH) {
		/*
		 * Overflow on recursion depth.
		 */
		writeerr(wp, "MACRO ABORTED");
		sleep(1);
		return;
	}
	if ((mac = get_macro(wp, wp->lastch)) == NULL) {
		ringbell();
	} else {
		macs[mac_level].m_str    = macstr;
		macs[mac_level].m_start  = mac_start;
		macs[mac_level++].m_mult = mflag;
		mac_start = macstr = mac;
		mflag = wp->curnum;
	}
}

/*
 * Execute the temporary macro (set by ESC : macro ... )
 */
EXPORT void
vmac(wp)
	ewin_t	*wp;
{
	extern char	mstr[];

	wp->eflags &= ~COLUPDATE;
	if (mstr[0] == '\0') {
		ringbell();
	} else {
		macs[mac_level].m_str    = macstr;
		macs[mac_level].m_start  = mac_start;
		macs[mac_level++].m_mult = mflag;
		mac_start = macstr = mstr;
		mflag = wp->curnum;
	}
}

/*
 * Get the next character from the macro replacement string.
 * Handle recursion properly.
 */
EXPORT int
gmacro()
{
	char	c;

	if ((c = *macstr++) == 0) {
		mflag--;
		if (mflag > 0) {
			macstr = mac_start;
			c = *macstr++;
		} else if (mac_level > 0) {
			macstr	  = macs[--mac_level].m_str;
			mac_start = macs[mac_level].m_start;
			mflag	  = macs[mac_level].m_mult;
			if (mac_level)
				return (gmacro());
		}
	}
	return ((Uchar)c);
}

/*
 * Execute "ved -e $(HOME)/.vedmac"
 * Remember to re-init the macro package later to reflect changes.
 */
/* ARGSUSED */
EXPORT void
vemac(wp)
	ewin_t	*wp;
{
	static	char	command[129];

	mflag = 1;
	/*
	 * XXX What will happen if we have EBCDIC ?
	 * XXX \015 was ^M before we port to Mac OS X Darwin
	 */
	snprintf(command, sizeof (command), "ved -e %s%s",
							home, "/.vedmac\015n");
	macstr = command;
	mac_init = 1;
}

/*
 * RE-initialize the macro package if needed.
 * Called from execcmds.c
 */
EXPORT void
macro_reinit(wp)
	ewin_t	*wp;
{
	if (mac_init == 1) {
		macro_init(wp);
		mac_init = 0;
	}
}

/*
 * Add a new macro to the list of known macros.
 */
LOCAL void
add_node(mn, ms)
	char	*mn;
	char	*ms;
{
	register mac_t	*np;
	register mac_t	*tn;
	register mac_t	*last;
	register int	cmp;
	register int	ln;
	register int	ls;

	/*
	 * First create and init new macro node.
	 */
	tn = (mac_t *) malloc(sizeof (*tn));
	if (tn == (mac_t *)NULL)
		return;
	ln = strlen(mn);
	ls = strlen(ms);
	if ((tn->m_name = malloc(ln+1)) == NULL) {
		free(tn);
		return;
	}
	if ((tn->m_string = malloc(ls+1)) == NULL) {
		free(tn->m_name);
		free(tn);
		return;
	}
	*movebytes(mn, tn->m_name, ln) = '\0';
	*movebytes(ms, tn->m_string, ls) = '\0';
	tn->m_next = NULL;

	if (first_macro == NULL) {
		first_macro = tn;
		return;
	}

	/*
	 * Insert new macro in order.
	 */
	np = last = first_macro;
	for (; ; np = np->m_next) {

		if (np == NULL) {
			/*
			 * Append to end of list
			 */
			last->m_next = tn;
			return;
		}

		cmp = strcmp(mn, np->m_name);

		if (cmp == 0) {
			/*
			 * Macro is already defined
			 */
			free(tn->m_name);
			free(tn->m_string);
			free(tn);
			return;
		}
		if (cmp < 0) {
			if (first_macro == np) {
				/*
				 * Make it the first in list.
				 */
				tn->m_next = first_macro;
				first_macro = tn;
				return;
			} else {
				/*
				 * Insert in list
				 */
				last->m_next = tn;
				tn->m_next = np;
				return;
			}
		}
		last = np;
	}
}

/*
 * Do a lookup for a macro.
 * Return the mapped string on success, else return NULL.
 */
LOCAL char *
get_macro(wp, c)
	ewin_t	*wp;
	Uchar	c;
{
		char	m_name[M_NAMELEN + 1];
	register mac_t	*tn;
	register int	i;
	register char	*cp;
	register char	*name;

	cp = name = m_name;
	*cp++ = c;
	tn = first_macro;
	for (i = 0; i < M_NAMELEN; i++) {
		*cp = '\0';
		for (;;) {
			if (tn == NULL)
				return (NULL);
			if (strcmp(name, tn->m_name) == 0)
				return (tn->m_string);
			if (strstr(tn->m_name, name) == tn->m_name)
				break;
			tn = tn->m_next;
		}
		*cp++ = gchar(wp);
	}
	return (NULL);
}
