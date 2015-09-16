/* @(#)abbrev.c	1.64 15/09/12 Copyright 1985-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)abbrev.c	1.64 15/09/12 Copyright 1985-2015 J. Schilling";
#endif
/*
 *	Abbreviation symbol handling
 *
 *	Copyright (c) 1985-2015 J. Schilling
 *
 *	.global & .local alias abbreviations are handled here
 *
 *	Exported functions:
 *		ab_getaltowner(tab)		Get alternate file owner that
 *						is trusted
 *		ab_getaltoname(tab)		Get name of alternate file
 *						owner that is trusted
 *		ab_setaltowner(tab, usrname)	Set alternate file owner that
 *						is trusted
 *		ab_read(tab, fname)		Read new abbrev table from fname
 *		ab_use(tab, fname)		Use new abbrev table from fname
 * 		ab_close(tab)			Shut down abbrev table
 *		ab_delete(tab, name, flags)	Pop or delete abbrev from table
 *						flags & AB_POP allows to
 *						implement the POSIX "unalias -a"
 *						without modifying
 *						.globals/.locals
 *		ab_deleteall(tab, flags)	Pop or delete all entries
 *						flags & AB_POP allows to
 *						implement the POSIX "unalias -a"
 *						without modifying
 *						.globals/.locals
 *		ab_dump(tab, f, flags)		Print all abbrev entries to file
 *						flags & AB_HISTORY sends a copy
 *						to the command history
 *		ab_insert(tab, name, val, flag)	Insert n/v pair to abbrev table
 *						flags & AB_BEGIN defines a begin
 *						alias
 *		ab_list(tab, pat, f, flags)	Print matched entries to file
 *						flags & AB_HISTORY sends a copy
 *						to the command history
 *		ab_push(tab, name, val, flag)	Push n/v pair to abbrev table
 *						flags & AB_BEGIN defines a begin
 *						alias
 *		ab_sname(tab, fname)		Set filename for _ab_output
 *		ab_gname(tab)			Get filename from tab
 *		ab_value(tab, name, seen, flag)	Perform n/v translation for tab
 *						flags & AB_BEGIN looks for begin
 *						alias only
 *
 *	Imported functions:
 *		any_match			from expand.c
 *		append_line			from inputc.c
 *		berror				from bsh.c
 *
 *	Imported vars:
 *		ctlc				from bsh.c
 *		ebadpattern			from str.c
 *		for_ru				.
 *		for_wct				.
 *		sn_badfile			.
 *		sn_badtab			.
 *		sn_no_mem			.
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

#ifdef	BOURNE_SHELL

#include "defs.h"
#undef	tab
#define	LOCAL	static
#define	EXPORT
#include "abbrev.h"
#include <schily/fcntl.h>
#include <schily/stat.h>
#include <schily/pwd.h>
#include <schily/shedit.h>

LOCAL	char	sn_badtab[]	= "bad_astab_number";
LOCAL	char	sn_no_mem[]	= "no_memory";
LOCAL	char	sn_badfile[]	= "bad_sym_file";

/*
 * The Bourne shell does not use stdio, so we need to redefine some functions.
 * Be sure to first #undef things that might be #define'd in schily/schily.h.
 */
#undef	filemopen
#define	filemopen(n, m, c)	open(n, m, c)
#undef	fileopen
#define	fileopen(n, m)	open(n, m, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define	fileread	read
#undef	filestat
#define	filestat	fstat
#define	fclose		close
#define	fflush(f)
#undef	filesize
#define	filesize	ab_fsize
#define	open_failed(f)	(f < 0)

#define	for_wct		(O_WRONLY|O_CREAT|O_TRUNC)
#define	for_ru		(O_RDONLY)

#define	raisecond(n, v)	error(n)
#define	malloc		alloc

#define	ctlc		intrcnt

#ifdef	HAVE_SNPRINTF
	/*
	 * Try to avoid js_snprintf() in the Bourne Shell
	 * to allow to lazyload libschily
	 */
#undef	snprintf
#define	js_snprintf	snprintf
#endif

#else	/* !BOURNE_SHELL */
#include <schily/stdio.h>
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/time.h>
#include <schily/stat.h>
#include <schily/pwd.h>
#include "bsh.h"
#include "abbrev.h"
#include "str.h"
#include "strsubs.h"
#include <schily/patmatch.h>

#define	open_failed(f)	(f == (FILE *)0)
#endif	/* !BOURNE_SHELL */

/*
 * Replacement node entry, one is allocated for each abbrev/alias replacement
 *
 *	ab_name is only set in first node of the ab_push list
 *	ab_name is NULL in further nodes of the ab_push list
 *	ab_value is NULL if the symbol has been deleted
 *	ab_flags & ABF_POP is != 0 if the symbol has been popped but not deleted
 *			ab_value is kept in that case to be able to keep the
 *			content of .globals/.locals
 */
typedef	struct abent	abent_t;

struct abent {
	abent_t	*ab_next;		/* next entry in list		 */
	abent_t *ab_push;		/* next in list of old values	 */
	abent_t *ab_seen;		/* next in list of seen values	 */
	char	*ab_name;		/* abbreviation/alias name	 */
	char	*ab_value;		/* replacement value		 */
	int	ab_flags;		/* various flags		 */
};

/*
 * Definitions for ab_flags:
 */
#define	ABF_BEGIN	1	/* replace only if begin of cmd		   */
#define	ABF_POP		2	/* pop only, but keep in .globals/.locals  */

#define	AB_NULL	(abent_t *)0

/*
 * Table entry, one is allocated for each abbreviation type.
 *
 *	at_ent contains a list of nodes for the related abbreviation table
 */
typedef struct abtab {
	abent_t	*at_ent;		/* first entry in list		 */
	char	*at_fname;		/* name of file for _ab_output() */
	char	*at_blk;		/* start of monolitic malloc()	 */
	char	*at_blkend;		/* end of monolitic malloc()	 */
	time_t	at_mtime;		/* our time stamp for at_fname	 */
	uid_t	at_altowner;		/* alternate permitted file owner */
} abtab_t;

LOCAL	abtab_t	ab_tabs[ABTABS]	= {	{0, 0, 0, 0, 0, (uid_t)-1},
					{0, 0, 0, 0, 0, (uid_t)-1}};

LOCAL	abtab_t	*_ab_down	__PR((abidx_t tab));
LOCAL	abent_t	*_ab_lookup	__PR((abtab_t *ap, char *name, BOOL new));
LOCAL	abent_t	*_ab_newnode	__PR((void));
LOCAL	void	_ab_output	__PR((abtab_t *ap));
LOCAL	void	_ab_print	__PR((abidx_t tab, char *name, FILE_p f,
								int aflags));
LOCAL	void	_ab_dump	__PR((abent_t *np, FILE_p f, int aflags));
LOCAL	void	_ab_list	__PR((abent_t *np, FILE_p f, int aflags));
LOCAL	void	_ab_match	__PR((abent_t *np, FILE_p f, int aflags,
				char *pattern, int *aux, int alt, int *state));
LOCAL	void	_ab_close	__PR((abent_t *np, abidx_t tab));
LOCAL	char	*ab_beginword	__PR((char *p, abtab_t *ap));
LOCAL	char	*ab_endword	__PR((char *p, abtab_t *ap));
LOCAL	char	*ab_endline	__PR((char *p, abtab_t *ap));
LOCAL	BOOL	ab_inblock	__PR((abidx_t tab, char *p));
LOCAL	time_t	ab_statmtime	__PR((struct stat *sp));
LOCAL	time_t	ab_filemtime	__PR((char *fname));
EXPORT	void	ab_read		__PR((abidx_t tab, char *fname));
EXPORT	void	ab_sname	__PR((abidx_t tab, char *fname));
EXPORT	char	*ab_gname	__PR((abidx_t tab));
EXPORT	void	ab_use		__PR((abidx_t tab, char *fname));
EXPORT	void	ab_close	__PR((abidx_t tab));
EXPORT	void	ab_insert	__PR((abidx_t tab, char *name, char *val,
								int aflags));
EXPORT	void	ab_push		__PR((abidx_t tab, char *name, char *val,
								int aflags));
LOCAL	void	_ab_delete	__PR((abent_t *np, abidx_t tab, int aflags));
EXPORT	void	ab_delete	__PR((abidx_t tab, char *name, int aflags));
LOCAL	void	_ab_deleteall	__PR((abent_t *np, abidx_t tab, int aflags));
EXPORT	void	ab_deleteall	__PR((abidx_t tab, int aflags));
EXPORT	char	*ab_value	__PR((abidx_t tab, char *name, void **seen,
								int aflags));
EXPORT	void	ab_dump		__PR((abidx_t tab, FILE_p f, int aflags));
EXPORT	void	ab_list		__PR((abidx_t tab, char *pattern, FILE_p f,
								int aflags));
LOCAL	void	ab_eupdated	__PR((abtab_t *ap));
LOCAL	void	_ab_pr		__PR((abent_t *np, FILE_p f, int aflags));
LOCAL	void	_ab_prposix	__PR((abent_t *np, FILE_p f, int aflags));
LOCAL	void	_ab_phist	__PR((abent_t *np, FILE_p f, int aflags));
#ifdef	BOURNE_SHELL
LOCAL	off_t	filesize	__PR((int f));
LOCAL	BOOL	any_match	__PR((char *s));
#endif

/*
 * Do range check for 'tab' and return abtab_t structure for 'tab'.
 */
LOCAL abtab_t *
_ab_down(tab)
	abidx_t	tab;
{
	if (tab < 0 || tab >= ABTABS)
		raisecond(sn_badtab, 0L);
	return (&ab_tabs[tab]);
}

#ifdef	PROTOTYPES
LOCAL abent_t *
_ab_lookup(abtab_t *ap, char *name, BOOL new)
#else
LOCAL abent_t *
_ab_lookup(ap, name, new)
	abtab_t	*ap;
	char	*name;
	BOOL	new;
#endif
{
	register abent_t	*np;	/* Current node pointer	*/
	register abent_t	*lp;	/* Last node pinter	*/
#ifdef	notdef
	register int		cmp;
#endif

	for (lp = AB_NULL, np = ap->at_ent; np != AB_NULL; ) {
		register char	*s1;
		register char	*s2;

#ifndef notdef
		s1 = name;
		s2 = np->ab_name;
		for (; *s1 == *s2; s1++, s2++) /* viel schneller als strcmp */
			if (*s1 == '\0')
				return (np);
		if (*s1 > *s2)
			break;
#else
		if ((cmp = strcmp(name, np->ab_name)) == 0)
			return (np);
		if (cmp > 0)
			break;
#endif
		lp = np;
		np = np->ab_next;
	}
	if (!new)
		return (AB_NULL);

	np = _ab_newnode();
	np->ab_name = name;
	if (lp == AB_NULL) {			/* make first in chain */
		np->ab_next = ap->at_ent;
		ap->at_ent = np;
	} else {				/* in middle of chain */
		np->ab_next = lp->ab_next;
		lp->ab_next = np;
	}
	return (np);
}


LOCAL abent_t *
_ab_newnode()
{
	register abent_t	*new;

	if ((new = (abent_t *)malloc(sizeof (abent_t))) == AB_NULL)
		raisecond(sn_no_mem, (long)"_ab_newnode");
	new->ab_next = AB_NULL;
	new->ab_push = AB_NULL;
	new->ab_seen = AB_NULL;
	new->ab_name = NULL;
	new->ab_value = NULL;
	new->ab_flags = 0;

	return (new);
}

/*
 * Output the current list of entries to ap->at_fname
 * This funktioon is used to update $HOME/.globals and .locals
 */
LOCAL void
_ab_output(ap)
	register abtab_t *ap;
{
	register FILE_p f;
		time_t	mtime;
		struct stat sb;

#ifdef DEBUG
	berror("updating: %s", ap->at_fname);
#endif
	if (ap->at_fname == NULL)
		return;
	mtime = ab_filemtime(ap->at_fname);
	/*
	 * If ap->at_mtime == 0, the file did not exist for us.
	 */
	if (mtime > ap->at_mtime) {
		ab_eupdated(ap);
		return;
	}
	if (lstat(ap->at_fname, &sb) < 0) /* Check whether a symlink */
		/* EMPTY */;		/* No file yet		   */
	else if (S_ISLNK(sb.st_mode))
		return;

	f = filemopen(ap->at_fname, for_wct, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (open_failed(f)) {
		raisecond(sn_badfile, (long)ap->at_fname);
	} else {
#ifdef	BOURNE_SHELL
		int	oldf = setb(f);	/* Set new fd for prs() */
#endif
		/*
		 *			AB_PERSIST -> no pushed entries
		 *			non interruptable
		 *			no copy into history
		 */
		_ab_dump(ap->at_ent, f, AB_PERSIST);
#ifdef	BOURNE_SHELL
		setb(oldf);		/* Restore fd for prs() */
#endif
		fclose(f);
		ap->at_mtime = ab_filemtime(ap->at_fname);
	}
}

/*
 * Print a single name/value replacement entry to an open file.
 */
LOCAL void
_ab_print(tab, name, f, aflags)
	abidx_t	tab;
	char	*name;
	FILE_p	f;
	int	aflags;		/* should push into History? */
{
	_ab_list(_ab_lookup(_ab_down(tab), name, FALSE), f, aflags);
	fflush(f);
}

/*
 * Dump the whole list.
 * Flags controls whether pushed entries appear in the output, whether
 * interruptable and whether a copy is pushed into the command history.
 */
LOCAL void
_ab_dump(np, f, aflags)
	register abent_t *np;
		FILE_p	f;
		int	aflags;
{
	register abent_t *tp;

	if (np == AB_NULL)
		return;
	_ab_dump(np->ab_next, f, aflags);
	tp = np;
	/*
	 * With AB_PERSIST skip pushed values and only dump the
	 * persistent base entry.
	 */
	if (aflags & AB_PERSIST)
		while (tp->ab_push != AB_NULL)
			tp = tp->ab_push;
	if (!(ctlc && (aflags & AB_INTR)))
		_ab_list(tp, f, aflags);
}

/*
 * The argument "f" is not used in the Bourne Shell
 */
/* ARGSUSED */
LOCAL void
_ab_list(np, f, aflags)
	register abent_t *np;
		FILE_p	f;
		int	aflags;	/* should push into History? */
{
	/*
	 * With AB_PERSIST skip pushed values and only list the
	 * persistent base entry.
	 */
	if (np != AB_NULL && (aflags & AB_PERSIST)) {
		while (np->ab_push != AB_NULL)
			np = np->ab_push;
	}
	if (np != AB_NULL && np->ab_value != NULL &&
	    ((np->ab_flags & ABF_POP) == 0 || aflags & AB_PERSIST)) {
		if ((aflags & AB_POSIX)) {
			_ab_prposix(np, f, aflags);
		} else {
			_ab_pr(np, f, aflags);
		}
#ifdef	INTERACTIVE
		if (aflags & AB_HISTORY) {
			_ab_phist(np, f, aflags);
		}
#endif
	}
}


LOCAL void
_ab_match(np, f, aflags, pattern, aux, alt, state)
	register abent_t *np;
		FILE_p	f;
		int	aflags;		/* should push into History? */
		char	*pattern;
		int	*aux;
		int	alt;
		int	*state;
{
	register abent_t *tp;
#ifndef	BOURNE_SHELL
	register char	*p;
#endif

	if (np == AB_NULL)
		return;
	_ab_match(np->ab_next, f, aflags, pattern, aux, alt, state);
	if (ctlc)
		return;
	tp = np;
	while (tp->ab_push != AB_NULL)
		tp = tp->ab_push;
#ifdef	BOURNE_SHELL
	if (gmatch(tp->ab_name, pattern))
		_ab_list(tp, f, aflags);
#else
	p = (char *)patmatch((unsigned char *)pattern, aux,
		(unsigned char *)tp->ab_name, 0, strlen(tp->ab_name),
		alt, state);
	if (p && *p == '\0')
		_ab_list(tp, f, aflags);
#endif
}

/*
 * Free all symbols associated with this table.
 */
LOCAL void
_ab_close(np, tab)
	register abent_t *np;
	abidx_t	tab;
{
	register abent_t *tp;
	register abent_t *sp;

	while (np != AB_NULL) {
		if (!ab_inblock(tab, np->ab_name))
			free(np->ab_name);
		tp = np;
		np = np->ab_next;		/* remember next ptr now */
		/*
		 * Now free all pushed values including this one.
		 */
		do {
			if (tp->ab_value != NULL &&
			    !ab_inblock(tab, tp->ab_value))
				free(tp->ab_value);
			sp = tp->ab_push;	/* Save push ptr */
			free((char *)tp);
		} while ((tp = sp) != AB_NULL);	/* Use remembered push ptr */
	}
}


/*
 * Find the next begin of word inside the allocated monolitic block.
 */
LOCAL char *
ab_beginword(p, ap)
	register char	*p;
		abtab_t	*ap;
{
	register char	*rend = ap->at_blkend;

	while (p < rend && (*p == ' ' || *p == '\t' || *p == '\n'))
		p++;
	return (p);
}


/*
 * Find the next end of word inside the allocated monolitic block.
 */
LOCAL char *
ab_endword(p, ap)
	register char	*p;
		abtab_t	*ap;
{
	register char	*rend = ap->at_blkend;

	while (p < rend && (*p != ' ' && *p != '\t' && *p != '\n'))
		p++;
	return (p);
}

/*
 * Find the next end of line inside the allocated monolitic block.
 */
LOCAL char *
ab_endline(p, ap)
	register char	*p;
		abtab_t	*ap;
{
	register char	*rend = ap->at_blkend;

	while (p < rend && *p != '\n')
		p++;
	return (p);
}

/*
 * Check whether a character pointer points into the monolitic block
 * that is allocated when the file is read by ab_read().
 */
LOCAL BOOL
ab_inblock(tab, p)
	abidx_t	tab;
	char	*p;
{
	register abtab_t *ap = _ab_down(tab);

	return ((p >= ap->at_blk) && (p <= ap->at_blkend));
}

LOCAL time_t
ab_statmtime(sp)
	struct stat	*sp;
{
	if (sp->st_mtime == (time_t)0)
		sp->st_mtime++;
	return (sp->st_mtime);
}

LOCAL time_t
ab_filemtime(fname)
	char	*fname;
{
	struct stat	sb;

	if (stat(fname, &sb) < 0)
		return ((time_t)0);

	return (ab_statmtime(&sb));
}

/*
 * Get alternate file owner that is trusted in addition to the current
 * shell user.
 */
EXPORT uid_t
ab_getaltowner(tab)
	abidx_t	tab;
{
	register abtab_t *ap = _ab_down(tab);

	return (ap->at_altowner);
}

EXPORT char *
ab_getaltoname(tab)
	abidx_t	tab;
{
	register abtab_t *ap = _ab_down(tab);
	register struct passwd *pw;
static	char oname[12];

	pw = getpwuid(ap->at_altowner);
	endpwent();
	if (pw)
		return (pw->pw_name);
	oname[0] = '\0';

#if	defined(BOURNE_SHELL) && defined(HAVE_SNPRINTF)
	/*
	 * Try to avoid js_snprintf() in the Bourne Shell
	 * to allow to lazyload libschily
	 */
	snprintf(oname, sizeof (oname),
		"%d", ap->at_altowner);
#else
	js_snprintf(oname, sizeof (oname),
		"%d", ap->at_altowner);
#endif
	return (oname);
}

/*
 * Set alternate file owner that is trusted in addition to the current
 * shell user.
 */
EXPORT void
ab_setaltowner(tab, usrname)
	abidx_t	tab;
	char	*usrname;
{
	register abtab_t *ap = _ab_down(tab);
	register struct passwd *pw;
	register char	*p = usrname;

	if (*usrname == '\0') {
		ap->at_altowner = (uid_t)-1;
		return;
	}
	pw = getpwnam(usrname);
	endpwent();

	if (pw) {
		ap->at_altowner = pw->pw_uid;
		return;
	}
	while (*p) {
		if (*p < '0')
			return;
		if (*p++ > '9')
			return;
	}
	ap->at_altowner = atoi(usrname);
}

/*
 * Read file 'fname' and build a new abbreviation/alias replacement table
 */
EXPORT void
ab_read(tab, fname)
	abidx_t	tab;
	char	*fname;
{
	register char	*line;
	register char	*name;
	register char	*val;
		FILE_p	f;
	register abtab_t *ap = _ab_down(tab);
		off_t	fsize;
	register int	beg;
		struct stat sb;

	/*
	 * Make sure that ap->at_fname is NULL to avoid writing back to the
	 * file when calling ab_insert().
	 */
	ab_close(tab);

	if (fname == NULL)
		return;

	if (lstat(fname, &sb) < 0)	/* Check whether a symlink */
		return;			/* No file to open, return */
	if (S_ISLNK(sb.st_mode))
		return;

	f = fileopen(fname, for_ru);
	if (open_failed(f))
		return;

	if (filestat(f, &sb) >= 0) {
		if (geteuid() == sb.st_uid) {
			if (sb.st_mode & (S_IWGRP|S_IWOTH)) {
				fclose(f);
				return;
			}
		} else if ((ap->at_altowner == (uid_t)-1 ||
		    ap->at_altowner != sb.st_uid)) {
			fclose(f);
			return;
		}
		ap->at_mtime = ab_statmtime(&sb);
	}
	fsize = filesize(f);
	if (ap->at_mtime == 0)
		ap->at_mtime = ab_filemtime(fname);
#ifdef DEBUG
	berror("ab_read(%d, %s)-> %d %.24s", tab, fname,
			ap->at_mtime, ctime(&ap->at_mtime));
#endif
	if ((ap->at_blk = malloc((size_t)fsize)) == NULL) {
		raisecond(sn_no_mem, (long)"ab_read");
		fclose(f);
		return;
	}
	ap->at_blkend = ap->at_blk + fsize - 1;
	if (fileread(f, ap->at_blk, (int)fsize) != fsize) {
		free(ap->at_blk);
		ap->at_blk = NULL;
		fclose(f);
		return;
	}
	fclose(f);

	line = ap->at_blk;
	while (line < ap->at_blkend) {
		line = ab_beginword(line, ap);
		if (*line++ != '#') {
			line = ab_endline(line, ap);
			continue;
		}
		if (*line == 'a') {
			beg = 0;
		} else if (*line == 'b') {
			beg = AB_BEGIN;
		} else {
			line = ab_endline(line, ap);
			continue;
		}
		name = line = ab_beginword(ab_endword(line, ap), ap);
		line = ab_endword(line, ap);
		*line++ = '\0';
		val = line = ab_beginword(line, ap);
		line = ab_endline(line, ap);
		*line++ = '\0';
		ab_insert(tab, name, val, beg);
	}
}


/*
 * Set filename to use for _ab_output()
 */
EXPORT void
ab_sname(tab, fname)
	abidx_t	tab;
	char	*fname;
{
	abtab_t *ap = _ab_down(tab);
#ifdef	BOURNE_SHELL
	free(ap->at_fname);	/* Bourne Shell only frees what need free() */
#endif
	ap->at_fname = fname;
}

/*
 * Get filename from tab
 */
EXPORT char *
ab_gname(tab)
	abidx_t	tab;
{
	abtab_t *ap = _ab_down(tab);

	return (ap->at_fname);
}

/*
 * Use new abbrev file
 */
EXPORT void
ab_use(tab, fname)
	abidx_t	tab;
	char	*fname;
{
	ab_read(tab, fname);
	ab_sname(tab, fname);
}

/*
 * Shut down a table, remove all name/value translations from this tab before.
 */
EXPORT void
ab_close(tab)
	abidx_t	tab;
{
	register abtab_t *ap = _ab_down(tab);

	_ab_close(ap->at_ent, tab);
	ap->at_fname = NULL;
	ap->at_mtime = (time_t)0;
	ap->at_ent = AB_NULL;
	if (ap->at_blk != NULL) {
		free(ap->at_blk);
		ap->at_blk = NULL;
	}
}


/*
 * Insert a new name/value pair into an abbreviation/alias table.
 */
EXPORT void
ab_insert(tab, name, val, aflags)
	abidx_t	tab;
	char	*name;
	char	*val;
	int	aflags;
{
		abtab_t	*ap = _ab_down(tab);
	register abent_t *np;

	np = _ab_lookup(ap, name, TRUE);
	if (np->ab_value != NULL && !ab_inblock(tab, np->ab_value))
		free(np->ab_value);
	np->ab_value = val;
	if (aflags & AB_BEGIN)
		np->ab_flags |= ABF_BEGIN;
	else
		np->ab_flags &= ~ABF_BEGIN;
	/*
	 * If this entry has not been pushed on top of old replacements,
	 * update the underlying file storage.
	 */
	if (np->ab_push == AB_NULL)
		_ab_output(ap);
}


/*
 * Push a new name/value pair on top of an abbreviation/alias table entry.
 */
EXPORT void
ab_push(tab, name, val, aflags)
	abidx_t	tab;
	char	*name;
	char	*val;
	int	aflags;
{
		abtab_t		*ap = _ab_down(tab);
	register abent_t	*np;
	register abent_t	*new;

	np = _ab_lookup(ap, name, TRUE);
	new = _ab_newnode();		/* Get space for node to push	*/
	new->ab_name = np->ab_name;	/* Dup to allow to list all	*/
	new->ab_value = np->ab_value;	/* First save old node data	*/
	new->ab_flags = np->ab_flags;
	new->ab_push = np->ab_push;
	np->ab_push = new;		/* Then make pushed node active	*/
	np->ab_value = val;
	if (aflags & AB_BEGIN)
		np->ab_flags |= ABF_BEGIN;
	else
		np->ab_flags &= ~ABF_BEGIN;
}


/*
 * Pop a new name/value pair from the top of an abbreviation/alias table entry.
 * If there is no pushed entry left over, then the whole entry is deleted
 * unless this is a POP operation. A POP on the last entry for that name just
 * sets the ABF_POP flag to be able to keep the content in .globals and .locals
 * Deletion is done by setting ab_value to NULL.
 */
LOCAL void
_ab_delete(np, tab, aflags)
	register abent_t	*np;
	abidx_t	tab;
	int	aflags;
{
	register abent_t	*op;

	if (np != AB_NULL && np->ab_value != NULL) {
		if (np->ab_push != AB_NULL) {	/* If saved old value */
			do {
				op = np->ab_push;
				if (!ab_inblock(tab, np->ab_value))
					free(np->ab_value);
				np->ab_value = op->ab_value; /* Pop top entry */
				np->ab_flags = op->ab_flags;
				np->ab_push = op->ab_push;
				free((char *)op);
			} while (np->ab_push && (aflags & AB_POPALL));
		}

		if (np->ab_push == AB_NULL) {	/* The last definition */
			if (aflags & AB_POP) {
				np->ab_flags |= ABF_POP;
			} else {
				if (!ab_inblock(tab, np->ab_value))
					free(np->ab_value);
				np->ab_value = NULL;
				if ((np->ab_flags & ABF_POP) == 0)
					_ab_output(_ab_down(tab));
			}
		}
	}
}

EXPORT void
ab_delete(tab, name, aflags)
	abidx_t	tab;
	char	*name;
	int	aflags;
{
	abtab_t	*ap = _ab_down(tab);
	abent_t	*np;

	np = _ab_lookup(ap, name, FALSE);
	_ab_delete(np, tab, aflags);
}

LOCAL void
_ab_deleteall(np, tab, aflags)
	register abent_t *np;
		abidx_t	tab;
		int	aflags;
{
	if (np == AB_NULL)
		return;
	_ab_deleteall(np->ab_next, tab, aflags);
	if (!(ctlc && (aflags & AB_INTR)))
		_ab_delete(np, tab, aflags | AB_POPALL);
}

EXPORT void
ab_deleteall(tab, aflags)
	abidx_t	tab;
	int	aflags;
{
	_ab_deleteall(_ab_down(tab)->at_ent, tab, aflags);
}

/*
 * Perform a name/value translation for a named abbreviation/alias table.
 * The seen pointer holds a list of already expanded aliases and is used
 * to avoid endless loops in alias expansion in a POSIX compliant way.
 */
EXPORT char *
ab_value(tab, name, seen, aflags)
	abidx_t	tab;
	char	*name;
	void	**seen;
	int	aflags;			/* lookup begin abbreviations also? */
{
	register abent_t	*np;

	np = _ab_lookup(_ab_down(tab), name, FALSE);
	if (np == AB_NULL)
		return (NULL);
	if (np->ab_flags & ABF_POP)
		return (NULL);
	if ((np->ab_flags & ABF_BEGIN) == 0 || (aflags & AB_BEGIN)) {
		if (seen) {
			register abent_t	*sp = *seen;

			while (sp) {
				if (sp == np)
					return (NULL);
				sp = sp->ab_seen;
			}
			np->ab_seen = *seen;
			*seen = np;
		}
		return (np->ab_value);
	}
	return (NULL);
}


/*
 * Print all name/value replacement entries to an open file.
 */
EXPORT void
ab_dump(tab, f, aflags)
	abidx_t	tab;
	FILE_p	f;
	int	aflags;		/* should push into History? */
{
	/*
	 *	Don't use AB_PERSIST -> output pushed entries
	 *					 be interruptable
	 */
	_ab_dump(_ab_down(tab)->at_ent, f, aflags | AB_INTR);
	fflush(f);
}


/*
 * Print all name/value replacements with matched entries to an open file.
 */
EXPORT void
ab_list(tab, pattern, f, aflags)
		abidx_t	tab;
	register char	*pattern;
		FILE_p	f;
		int	aflags;		/* should push into History? */
{
	register int	*aux = NULL;	/* auxiliary array */
	register int	*state = NULL;	/* state array */
	register int	alt = 0;	/* outermost alternate */
#ifndef	BOURNE_SHELL
	register int	patlen;		/* pattern lenght */
#endif

	if (!any_match(pattern)) {
		_ab_print(tab, pattern, f, aflags);
	} else {
#ifndef	BOURNE_SHELL
		patlen = strlen(pattern);
		aux = (int *)malloc((size_t)patlen * sizeof (int));
		state = (int *)malloc((size_t)(patlen+1) * sizeof (int));
		alt = patcompile((unsigned char *)pattern, patlen, aux);
		if (alt) {
			_ab_match(_ab_down(tab)->at_ent,
					f, aflags, pattern, aux, alt, state);
			fflush(f);
		} else {
			berror("%s", ebadpattern);
		}
		free((char *)aux);
		free((char *)state);
#else
		_ab_match(_ab_down(tab)->at_ent,
				f, aflags, pattern, aux, alt, state);
		fflush(f);
#endif
	}
}

LOCAL void
ab_eupdated(ap)
	abtab_t *ap;
{
#ifdef	BOURNE_SHELL
	gfailure(UC ap->at_fname, "updated by another shell, cannot write back.");
#else
	berror("'%s' was updated by another shell, cannot write back.",
		ap->at_fname);
#endif
}

/*
 * The argument "f" is not used in the Bourne Shell
 */
/* ARGSUSED */
LOCAL void
_ab_pr(np, f, aflags)
	register abent_t	*np;
		FILE_p		f;
		int		aflags;
{
#ifdef	BOURNE_SHELL
	int	len;
#endif

	if (np->ab_value == NULL)
		return;
	if (np->ab_push && (aflags & AB_ALL))
		_ab_pr(np->ab_push, f, aflags);

#ifdef	BOURNE_SHELL
	prc_buff('#');
	if (np->ab_push)
		prc_buff('p');
	prc_buff((np->ab_flags & ABF_BEGIN) ? 'b':'a');
	prc_buff(' ');
	prs_buff(UC np->ab_name);
	len = length(UC np->ab_name);
	do {
		prc_buff(' ');
	} while (len++ <= 8);
	prs_buff(UC np->ab_value);
	prc_buff(NL);
#else
	fprintf(f, "#%s%c %-8s %s\n",
				np->ab_push ? "p":"",
				np->ab_flags & ABF_BEGIN ? 'b':'a',
				np->ab_name, np->ab_value);
#endif
}

/*
 * The argument "f" is not used in the Bourne Shell
 */
/* ARGSUSED */
LOCAL void
_ab_prposix(np, f, aflags)
	register abent_t	*np;
		FILE_p		f;
		int		aflags;
{
	if (np->ab_value == NULL)
		return;
	if (aflags & AB_PARSE) {
		if (np->ab_push && (aflags & AB_ALL))
			_ab_prposix(np->ab_push, f, aflags);

#ifdef	BOURNE_SHELL
		prs_buff(UC "alias ");
		if ((np->ab_flags & ABF_BEGIN) == 0)
			prs_buff(UC "-a ");
		if (np->ab_push)
			prs_buff(UC "-p ");
		if (aflags & AB_PGLOBAL)
			prs_buff(UC "-g ");
		if (aflags & AB_PLOCAL)
			prs_buff(UC "-l ");
#else
		fprintf(f, "alias %s%s%s%s",
				(np->ab_flags & ABF_BEGIN) ? "":"-a ",
				np->ab_push ? "-p ":"",
				(aflags & AB_PGLOBAL) ? "-g ":"",
				(aflags & AB_PLOCAL) ? "-l ":"");
#endif
	}
#ifdef	BOURNE_SHELL
	prs_buff(UC np->ab_name);
	prs_buff(UC "='");
	{	unsigned char *p = UC np->ab_value;
		while (*p) {
			if (*p == '\'')
				prs_buff(UC "'\\'");
			prc_buff(*p++);
		}
	}
	if ((aflags & AB_PARSE) == 0 &&
	    (np->ab_flags & ABF_BEGIN) == 0)
		prs_buff(UC "' # allexpand\n");
	else
		prs_buff(UC "'\n");
#else
	fprintf(f, "'%s=", np->ab_name);
	{	char *p = np->ab_value;
		while (*p) {
			if (*p == '\'')
				fprintf(f, "\\");
			fputc(*p++, f);
		}
	}
	fprintf(f, "'%s\n",
			((np->ab_flags & ABF_BEGIN) == 0) ? " # allexpand":"");
#endif
}

/*
 * The argument "f" is not used in the Bourne Shell
 */
/* ARGSUSED */
LOCAL void
_ab_phist(np, f, aflags)
	register abent_t	*np;
		FILE_p		f;
		int		aflags;
{
#ifdef	BOURNE_SHELL
#define	append_line	shedit_append_line
#endif
	if (np->ab_value == NULL)
		return;
	if (np->ab_push && (aflags & AB_ALL))
		_ab_phist(np->ab_push, f, aflags);

	{	char		buf[8192];
		unsigned int	len;

		js_snprintf(buf, sizeof (buf),
				"#%s%c %-8s %s",
				np->ab_push ? "p":"",
				np->ab_flags & ABF_BEGIN ? 'b':'a',
				np->ab_name, np->ab_value);
		len = strlen(buf);
		append_line(buf, len + 1, len);
	}
}

#ifdef	BOURNE_SHELL
LOCAL off_t
filesize(f)
	int	f;
{
	struct stat	sb;

	if (fstat(f, &sb) < 0)
		return ((off_t)-1);

	return (sb.st_size);
}

LOCAL BOOL
any_match(s)
	register char	*s;
{
	register unsigned char	*rm = UC "[]?*";

	while (*s && !any(*s, rm))
		s++;
	return ((BOOL) *s);
}
#endif
