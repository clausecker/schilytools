/* @(#)evops.c	1.35 09/07/28 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)evops.c	1.35 09/07/28 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	bsh environment section
 *
 *	Copyright (c) 1984-2009 J. Schilling
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

#include <schily/stdio.h>
#include <schily/param.h>	/* Include various defs needed with some OS */
#include "bsh.h"
#include "str.h"
#include "strsubs.h"
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/limits.h>
#include <schily/systeminfo.h>
#include <schily/utsname.h>
#include <schily/locale.h>


#define	EVAINC	5

extern	char	**evarray;
extern	unsigned evasize;
extern	int	evaent;
extern	BOOL	noslash;
extern	char	*user;
extern	char	*hostname;

EXPORT	char	*getcurenv	__PR((char *name));
LOCAL	char	*getval		__PR((char *name, char *ev));
EXPORT	void	ev_init		__PR((char **evp));
EXPORT	void	ev_insert	__PR((char *val));
EXPORT	void	ev_ins		__PR((char *val));
EXPORT	void	ev_delete	__PR((char *name));
EXPORT	void	ev_inc		__PR((void));
EXPORT	void	ev_list		__PR((FILE * fp));
EXPORT	BOOL	ev_eql		__PR((char *name, char *tval));
EXPORT	void	inituser	__PR((void));
EXPORT	void	inithostname	__PR((void));
EXPORT	void	initprompt	__PR((void));
LOCAL	void	setprompt	__PR((char *p, int nr));
LOCAL	void	set_noslash	__PR((char *s));
EXPORT	BOOL	ev_set_locked	__PR((char *val));
LOCAL	BOOL	ev_is_locked	__PR((char *s));
LOCAL	void	ev_check	__PR((char *what));
#ifdef	USE_LOCALE
LOCAL	BOOL	ev_neql		__PR((char *name, char *ev));
#endif
LOCAL	void	ev_locale	__PR((char *name));

/*
 * Get the value for a enrivonment variable name.
 * Check against current environment and not against
 * the global default 'environ'.
 */
EXPORT char *
getcurenv(name)
	char	*name;
{
	register char **p = evarray;
	register char *p2;

	while (*p != NULL) {
		if ((p2 = getval(name, *p)) != NULL)
			return (p2);
		p++;
	}
	return (NULL);
}

/*
 * Check a name against a specific environment array entry.
 * If name matches, return value, else return NULL.
 */
LOCAL char *
getval(name, ev)
	register char *name;
	register char *ev;
{
	for (;;) {
		if (*name != *ev) {
			if (*ev == '=' && *name == '\0')
					return (++ev);
			return (NULL);
		}
		if (*name == '\0')		/* *name == *ev == '\0' */
			return (NULL);
		name++;
		ev++;
	}
}

/*
 * Initialize the current environment from the global 'environ'.
 */
EXPORT void
ev_init(evp)
	register char *evp[];
{
	register int i;

	ev_inc();			/* ev_insert() benutzt evarray */

	for (i = 0; evp[i] != NULL; i++) {
		if (strchr(evp[i], '=') == NULL) {
			error("WARNING: ev_init() Missing '=' in env[%d] '%s'\n",
							i, evp[i]);
		}
		ev_insert(makestr(evp[i]));
	}
}

/*
 * Insert a new entry into the current environment.
 * Check for permission before doing the actual insert.
 */
EXPORT void
ev_insert(val)
	char *val;
{
#ifdef DEBUG
	printf("ev_insert('%s')\n", val);
#endif /* DEBUG */
	if (ev_set_locked(val)) {
		free(val);
		ex_status = 1;
		return;
	}
	ev_ins(val);
}

/*
 * Insert a new entry into the current environment.
 * Don't check for permission before doing the actual insert.
 * Handle variables with special meaning.
 */
EXPORT void
ev_ins(val)
	register char *val;
{
	int	i;
	char	*p;

	if (!(p = strchr(val, '='))) {
		free(val);
		ex_status = 1;
		return;
	}
	p++;
#ifdef DEBUG
	printf("value = '%s'\n", p);
#endif /* DEBUG */
	if (getval(slashname, val))
		set_noslash(p);
#ifdef INTERACTIVE
	if (getval(histname, val))
		chghistory(p);
#else
	if (getval(histname, val))
		if (!sethistory(p))
			return;
#endif
	if (getval(promptname, val))
		setprompt(p, 0);
	if (getval(prompt2name, val))
		setprompt(p, 1);
	if (getval(mchkname, val)) {
		if (!toint(gstd, p, &i))
			return;
		if (i < 0) {
			berror("Bad number '%s'.", p);
		}
		mailcheck = i;
	}

	for (i = 0; i < evaent; i++) {
		if (streqln(val, evarray[i], p-val)) {
			free(evarray[i]);
			evarray[i] = val;
			ev_locale(val);
			return;
		}
	}
	if ((evaent+1) >= evasize)
		ev_inc();
	evarray[evaent++] = val;
	evarray[evaent] = NULL;
	ev_check("ev_ins()");
	ev_locale(val);
}

/*
 * Delete an entry from the current environment.
 * Don't check for permission before doing the actual insert.
 * Handle variables with special meaning.
 */
EXPORT void
ev_delete(name)
	register char *name;
{
	register int i;

	for (i = 0; i < evaent; i++) {
		if (getval(name, evarray[i])) {
			if (ev_is_locked(name)) {
				berror("Can't delete '%s'. Variable is locked.", name);
				ex_status = 1;
				return;
			}
			if (streql(name, slashname))
				set_noslash(nullstr);
#ifdef INTERACTIVE
			if (streql(name, histname))
				chghistory("0");
#else
			if (streql(name, histname))
				sethistory("0");
#endif
			if (streql(name, mchkname))
				mailcheck = 600;

			free(evarray[i]);
			for (; i < evaent; i++)
				evarray[i] = evarray[i+1];
			evarray[i] = NULL;
			--evaent;

			ev_check("ev_idelete()");
			ev_locale(name);

			return;
		}
	}
	berror("Can't delete '%s' Variable does not exist.", name);
	ex_status = 1;
}

/*
 * Enhance the size of the current environment array.
 */
EXPORT void
ev_inc()
{
	if (evarray == (char **)NULL)
		evarray = (char **)malloc(EVAINC*sizeof (char *));
	else
		evarray = (char **)realloc(evarray,
					(evasize+EVAINC)*sizeof (char *));
	if (evarray == (char **)NULL)
		berror("%s", sn_no_mem);
	evarray[evasize] = NULL;
	evasize += EVAINC;
}

/*
 * Print the content of the complete current environment array
 * on File Pointer 'fp' to allow stdout redirection.
 */
EXPORT void
ev_list(fp)
	register FILE	*fp;
{
	register int	i;

	for (i = 0; i < evaent; i++)
		fprintf(fp, "%s\n", evarray[i]);

	ev_check("ev_list()");
}

/*
 * Check the current environment array against name and tval.
 */
EXPORT BOOL
ev_eql(name, tval)
	char	*name;
	char	*tval;
{
	char	*val;

	if ((val = getcurenv(name)) != NULL)
		return (streql(val, tval));
	return (FALSE);
}

EXPORT void
inituser()
{
	char	*un;

	if (!getcurenv(username)) {
		if (getcurenv(Elogname)) {
			ev_insert(concat(username, eql, getcurenv(Elogname), (char *)NULL));
		} else {
			un = getuname(geteuid());
			ev_insert(concat(username, eql, un, (char *)NULL));
			free(un);
		}
	}
	user = makestr(getcurenv(username));
	if (!getcurenv(Elogname))
		ev_insert(concat(Elogname, eql, user, (char *)NULL));
}

EXPORT void
inithostname()
{
#if defined(HAVE_SYS_SYSTEMINFO_H) && defined(SI_HOSTNAME)

#ifndef	SYS_NMLN		/* SYS_NMLN is in limits.h, missing on MiNT */
#define	SYS_NMLN	257
#endif
	char	host[SYS_NMLN];
	int	ret;

	ret = sysinfo(SI_HOSTNAME, host, sizeof (host));
	if (ret < 0 || ret > sizeof (host))
		hostname = makestr(nullstr);
	else
		hostname = makestr(host);
#else
# if	defined(HAVE_GETHOSTNAME)
#if	!defined(MAXHOSTNAMELEN) && defined(MAXGETHOSTNAME)
#define	MAXHOSTNAMELEN	MAXGETHOSTNAME			/* XXX DJGPP -> libport ??? */
#endif
	char	host[MAXHOSTNAMELEN];

	if (gethostname(host, sizeof (host)) < 0)
		hostname = makestr(nullstr);
	else
		hostname = makestr(host);
# else
	struct utsname	unm;

	if (uname(&unm) < 0)
		hostname = makestr(nullstr);
	else
		hostname = makestr(unm.nodename);
# endif
#endif
}

/*
 * Set prompt according to uid
 */
EXPORT void
initprompt()
{
	ev_insert(concat(promptname, eql, hostname, hostname[0] ? " " : nullstr,
				geteuid() ? user : "admin", "> ", (char *)NULL));
#ifdef	INTERACTIVE
	ev_insert(concat(prompt2name, eql, MOREPROMPT, (char *)NULL));
#else	/* INTERACTIVE */
	ev_insert(concat(prompt2name, eql, EDITPROMPT, (char *)NULL));
#endif	/* INTERACTIVE */
}

/*
 * Set prompt to value of p
 */
LOCAL void
setprompt(p, nr)
	char	*p;
	int	nr;
{
	if (prompts[nr])
		free(prompts[nr]);
	prompts[nr] = makestr(p);
}

LOCAL void
set_noslash(s)
	char	*s;
{
	noslash = streql(s, off);
}

EXPORT BOOL
ev_set_locked(val)
	char	*val;
{
	if (ev_is_locked(val)) {
		berror("Can't set environment '%s'. Variable is locked.", val);
		return (TRUE);
	}
	return (FALSE);
}

/*
 * Check if environment variable is in locklist.
 */
LOCAL BOOL
ev_is_locked(s)
	register char	*s;
{
	register char	*lockstr;
	register char	*c;
	register int	len;

	if (!(lockstr = getcurenv(evlockname)))
		return (FALSE);
	if (streql(lockstr, on))
		return (TRUE);
	c = strchr(s, '=');
	if (!c)
		len = strlen(s);
	else
		len = c - s;
	while (*lockstr) {
		if ((c = strchr(lockstr, ':')) == NULL)
			return (strlen(lockstr) == len && streqln(s, lockstr, len));
		if ((c - lockstr) == len && streqln(s, lockstr, len))
			return (TRUE);
		lockstr = ++c;
	}
	return (FALSE);
}

LOCAL void
ev_check(what)
	char	*what;
{
	int	i;

	for (i = 0; evarray[i] != NULL; i++) {
		if (i >= evaent)
			error("WARNING: %s No NULL in env[%d] '%s'\n", what, i, evarray[i]);
		if (strchr(evarray[i], '=') == NULL)
			error("WARNING: %s Missing '=' in env[%d] '%s'\n", what, i, evarray[i]);
	}
	if (i != evaent)
		error("WARNING: %s i %d evaent %d\n", what, i, evaent);
}

#ifdef	USE_LOCALE
LOCAL BOOL
ev_neql(name, ev)
	register char *name;
	register char *ev;
{
	for (;;) {
		if (*name != *ev) {
			if (*ev == '=' && *name == '\0')
					return (TRUE);
			return (FALSE);
		}
		if (*name == '\0')		/* *name == *ev == '\0' */
			return (TRUE);
		name++;
		ev++;
	}
}
#endif

LOCAL void
ev_locale(name)
	char	*name;
{
#ifdef	USE_LOCALE

		char	**esav;
	extern	char	**environ;

	if (name[0] != 'L')
		return;

	if (!(ev_neql("LC_ALL", name) ||
	    ev_neql("LC_CTYPE", name) ||
	    ev_neql("LC_COLLATE", name) ||
	    ev_neql("LC_NUMERIC", name) ||
	    ev_neql("LC_MESSAGES", name) ||
	    ev_neql("LC_MONETARY", name) ||
	    ev_neql("LC_TIME", name) ||
	    ev_neql("LANG", name)))
		return;

	/*
	 * Do not call setlocale() before inituser() was called which is an
	 * indication that our local environment was initialized.
	 */
	if (user == NULL)
		return;

	/*
	 * Let getenv() simulate the behaviour of getcurenv().
	 * This allows us to use the libc setlocale() for bsh.
	 */
	esav = environ;
	environ = evarray;

	if (setlocale(LC_ALL, "") == NULL)
		error("Bad locale '%s'.\n", name);

	environ = esav;
#endif
}
