/* @(#)dirs.c	1.29 14/04/21 Copyright 1984-2014 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)dirs.c	1.29 14/04/21 Copyright 1984-2014 J. Schilling";
#endif
/*
 *	Directory routines
 *
 *	Copyright (c) 1984-2014 J. Schilling
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
#include <schily/maxpath.h>
#include "bsh.h"
#include "str.h"
#include "node.h"
#include "abbrev.h"
#include "strsubs.h"
#include <schily/string.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>

#include <schily/dirent.h>
#include <schily/maxpath.h>
#include <schily/getcwd.h>

#define	PUSH	0x1
#define	POP	0x2
#define	PRINT	0x10000000

LOCAL	char	*curr_wd	= NULL;
LOCAL	char	*last_wd	= NULL;
LOCAL	Tnode	*dirs		= 0;
LOCAL	BOOL	pwd_done	= FALSE;

LOCAL	void	push_dir	__PR((char *name));
LOCAL	char	*pop_dir	__PR((int offset));
LOCAL	void	init_dirs	__PR((void));
LOCAL	void	update_lwd	__PR((void));
EXPORT	void	update_cwd	__PR((void));
EXPORT	void	bpwd		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bdirs		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bcd		__PR((Argvec * vp, FILE ** std, int flag));
LOCAL	int	cwd		__PR((FILE ** std, char *newdir, BOOL not_to_home, int flg));
LOCAL	int	changedir	__PR((FILE ** std, char *dir, char *cdenv, BOOL locklist, int flg));
LOCAL	int	changewd	__PR((FILE ** std, char *newdir, char *cdenv, BOOL locklist, int flg));
LOCAL	void	pr_dirs		__PR((FILE * f));
LOCAL	void	pr_dir		__PR((FILE * f, char *name, char *home));
LOCAL	BOOL	higher_dir	__PR((char *dir, char *maxdir));
LOCAL	char	*abs_path	__PR((char *rel));
LOCAL	void	shorten		__PR((char *name));

LOCAL void
push_dir(name)
	char	*name;
{
	dirs = allocnode(STRING, (Tnode *)makestr(name), dirs);
}

LOCAL char *
pop_dir(offset)
	register int	offset;
{
	register	int	i = 0;
	register 	struct Tnode *d = dirs;
	register	struct Tnode *prev = dirs;
			char	*name;

	while (d && i++ != offset) {
		prev = d;
		d = d->tn_right.tn_node;
	}
	if (!d)
		return (NULL);
	name = d->tn_left.tn_str;
	if (prev == d)		/* d == dirs */
		dirs = d->tn_right.tn_node;
	else
		prev->tn_right.tn_node = d->tn_right.tn_node;
	free((char *) d);
	return (name);
}

LOCAL void
init_dirs()
{
	char	wd[MAXPATHNAME + 1];
	char	*c_wd = wd;

	if (getcwd(c_wd, MAXPATHNAME) == NULL) {
		if ((c_wd = getcurenv(cwdname)) == NULL)
			c_wd = getcurenv(homename);
	} else {
		pwd_done = TRUE;
	}
	if (c_wd == NULL)
		c_wd = "unknown";
	pop_dir(0);
	push_dir(c_wd);
	update_cwd();
}

LOCAL void
update_lwd()
{
	free(last_wd);
	if (curr_wd)
		last_wd = makestr(curr_wd);
}

EXPORT void
update_cwd()
{
	char	*dir;

	if (dirs == NULL)	/* ist am Anfang nicht initialisiert !! */
		return;

	curr_wd = dirs->tn_left.tn_str;

	/*
	 * ev_ins() (ohne is_locked), wir wollen es immer aendern.
	 */
	dir = getcurenv(cwdname);
	if (dir == NULL || !streql(dir, curr_wd))
		ev_ins(concat(cwdname, eql, curr_wd, (char *)NULL));

	/*
	 * sh / ksh compat
	 */
	dir = getcurenv(pwdname);
	if (dir == NULL || !streql(dir, curr_wd))
		ev_ins(concat(pwdname, eql, curr_wd, (char *)NULL));
}

/*
 * Get and Print current directory
 */
/* ARGSUSED */
EXPORT void
bpwd(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	if (!pwd_done)
		init_dirs();
	fprintf(std[1], "%s\n", curr_wd);
}

/*
 * Print directory Stack
 */
/* ARGSUSED */
EXPORT void
bdirs(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	if (!dirs)
		init_dirs();
	pr_dirs(std[1]);
}

/*
 * Change the current working directory
 */
/* ARGSUSED */
EXPORT void
bcd(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	char	*name = vp->av_av[0];
	char	*newdir = vp->av_av[1];
	int	err;
	int	flg = 0;
	int	offset = 0;
	int	needfree = FALSE;

	if (!dirs)
		init_dirs();
	if (streql(name, "pushd"))
		flg |= PUSH;
	if (streql(name, "popd"))
		flg |= POP;
	if (newdir != NULL && newdir[0] == '-' && newdir[1] == '\0')
		newdir = last_wd;

	if (newdir != NULL) {
		if (newdir[0] == '-') {
			if (!toint(std, ++newdir, &offset))
				return;
			needfree = TRUE;
			if (!(newdir = pop_dir(offset))) {
				fprintf(std[2], "Bad offset.\n");
				ex_status = 1;
				return;
			}
		}
	} else if (flg & POP) {
		if (!dirs->tn_right.tn_node) {
			fprintf(std[2], "Stack empty.\n");
			ex_status = 1;
			return;
		}
		free(pop_dir(0));
		newdir = dirs->tn_left.tn_str;
	} else if ((newdir = getcurenv(homename)) == NULL) {	/* argcount == NULL */
		fprintf(std[2], "Can't get home directory.\n");
		ex_status = 1;
		return;
	}
	if ((err = cwd(std, newdir, vp->av_ac - 1, flg)) == 0) {
		ab_use(LOCAL_AB, localname);
	} else {
		fprintf(std[2], "Can't change to '%s'. %s\n",
						newdir, errstr(err));
		ex_status = 1;
	}
	if (needfree)
		free(newdir);
}

LOCAL int
cwd(std, newdir, not_to_home, flg)
	FILE	*std[];
	char	*newdir;
	BOOL	not_to_home;
	int	flg;
{
	char	*cdenv;
	int	err = 0;
	BOOL	locklist;

	cdenv = getcurenv("CD");
	if (cdenv && not_to_home && streql(cdenv, off))
		err = EACCES;
	locklist = not_to_home && !(!cdenv || streql(cdenv, on));
	if (!err)
		err = changedir(std, newdir, cdenv, locklist, flg);
	return (err);
}

LOCAL int
changedir(std, dir, cdenv, locklist, flg)
	FILE	*std[];
	char	*dir;
	char	*cdenv;
	BOOL	locklist;
	int	flg;
{
			int	err;
			char	*pathlist = getcurenv(cdpathname);
			char	*newdir;
	register	char	*p1;
	register	char	*p2;

	if (!pathlist || dir[0] == '/')
		return (changewd(std, dir, cdenv, locklist, flg));
	p2 = pathlist = makestr(pathlist);
	for (;;) {
		p1 = p2;
		if ((p2 = strchr(p2, ':')) != NULL)
			*p2++ = '\0';
		if (*p1 == '\0')
			newdir = concat(dir, (char *)NULL);
		else
			newdir = concat(p1, slash, dir, (char *)NULL);
		if ((err = changewd(std, newdir, cdenv, locklist,
						 *p1 ?(flg|PRINT): flg)) == 0) {
			break;
		}
		if (err == ENOENT && newdir[0] == '\0')
			break;
		if (err == EACCES || !p2)
			break;
		free(newdir);
	}
	free(newdir);
	free(pathlist);
	return (err);
}

LOCAL int
changewd(std, newdir, cdenv, locklist, flg)
	FILE	*std[];
	char	*newdir;
	char	*cdenv;
	BOOL	locklist;
	int	flg;
{
	int	ret;
	char	*full;

	if (locklist && higher_dir(newdir, cdenv))
		return (EACCES);
	ret = chdir(newdir) < 0 ? geterrno():0;
	if (!ret) {
		pwd_done = FALSE;
		full = abs_path(newdir);
		update_lwd();			/* may change newdir */
		if (!(flg & PUSH))
			free(pop_dir(0));
		push_dir(full);
		update_cwd();
	}
	if (!ret && (flg & PRINT || dirs->tn_right.tn_node))
		pr_dirs(std[1]);
	return (ret);
}

LOCAL void
pr_dirs(f)
	register FILE	*f;
{
	register	struct Tnode *d = dirs;
	register	char	*home;

	home = myhome();
	while (d) {
		pr_dir(f, d->tn_left.tn_str, home);
		d = d->tn_right.tn_node;
	}
	fputc('\n', f);
	if (home)
		free(home);
}

LOCAL void
pr_dir(f, name, home)
	FILE	*f;
	char	*name;
	char	*home;
{
	register	char	*dir;
	register	char	*d;

	dir = d = makestr(name);
	if (home && strbeg(home, dir) && !streql(home, slash)) {
		d += strlen(home) - 1;
		*d = '~';
	}
	fprintf(f, "%s ", d);
	free(dir);
}

LOCAL BOOL
higher_dir(dir, maxdir)
	char 	*dir;
	char	*maxdir;
{
	char	*newdir;
	BOOL	ok;

	newdir = abs_path(dir);
	ok = !streqln(newdir, maxdir, strlen(maxdir));
	free(newdir);
	return (ok);
}

/*
 * Expandiert einen relativen Pfadnamen in eine vollen.
 */
LOCAL char *
abs_path(rel)
	register	char	*rel;
{
	register	char	*full;
	register	char	*ofull;

	if (rel[0] == '/') {
		if (rel[1] == '\0')		  /*  root dir		*/
			return (makestr(rel));
		rel++;
		ofull = full = makestr(nullstr);  /*  Empty name	 */
	} else {
		ofull = full = makestr(curr_wd);  /*  Working directory. */
	}
	for (;;) {
/*		printf("%s / %s\n", full, rel);*/
		full = concat(full, slash, rel, (char *)NULL);
		free(ofull);
		ofull = rel = full;

		for (;;) {
			rel = strchr(++rel, '/');
			if (rel == NULL)
				break;
			if (rel[1] == '/' || rel[1] == '\0') {
				*rel++ = '\0';
							/* /foo//bar = /foo/bar */
				while (rel[0] == '/')
					rel++;
				break;
			}
			if (rel[1] == '.') {
							/* /foo/./bar = /foo/bar */
				if (rel[2] == '/') {
					*rel = '\0';
					rel += 3;
					break;
				}
							/* /foo/. = /foo    */
				if (rel[2] == '\0') {
					*rel = '\0';
					rel += 2;
					break;
				}
				if (rel[2] == '.') {
							/* /foo/../bar = /bar */
					if (rel[3] == '/') {
						*rel = '\0';
						rel += 4;
						shorten(full);
						break;
					}
							/* /foo/bar/.. = /foo */
					if (rel[3] == '\0') {
						*rel = '\0';
						shorten(full);
						break;
					}
				}
			}
		}
		if (rel == NULL || rel[0] == '\0')
			break;
	}
	if (full[1] == '.' && full[2] == '\0') 			/* /. = /   */
		full[1] = '\0';
	return (full);
}

/*
 * Beseitigt die letzte Komponente aus einem Pfadnamen.
 */
LOCAL void
shorten(name)
	register	char	*name;
{
	register	char	*p;

	for (p = name++; *p++ != '\0'; );
	while (p > name)
		if (*--p == '/')
			break;
	*p = '\0';
}
