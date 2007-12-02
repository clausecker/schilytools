/* @(#)walk.c	1.5 06/10/22 Copyright 2005-2006 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)walk.c	1.5 06/10/22 Copyright 2005-2006 J. Schilling";
#endif
/*
 *	This file contains the callback code for treewalk() as used
 *	with mkisofs -find.
 *
 *	Copyright (c) 2005-2006 J. Schilling
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
#include <schily/stat.h>
#include <walk.h>
#include <find.h>
#include "mkisofs.h"
#include <schily/schily.h>

#ifdef	USE_FIND
/*
 * The callback function for treewalk()
 */
EXPORT int
walkfunc(nm, fs, type, state)
	char		*nm;
	struct stat	*fs;
	int		type;
	struct WALK	*state;
{
	if (type == WALK_NS) {
		errmsg("Cannot stat '%s'.\n", nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_SLN && (state->walkflags & WALK_PHYS) == 0) {
		errmsg("Cannot follow symlink '%s'.\n", nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_DNR) {
		if (state->flags & WALK_WF_NOCHDIR)
			errmsg("Cannot chdir to '%s'.\n", nm);
		else
			errmsg("Cannot read '%s'.\n", nm);
		state->err = 1;
		return (0);
	}
	if (state->maxdepth >= 0 && state->level >= state->maxdepth)
		state->flags |= WALK_WF_PRUNE;
	if (state->mindepth >= 0 && state->level < state->mindepth)
		return (0);

	if (state->tree == NULL ||
	    find_expr(nm, nm + state->base, fs, state, state->tree)) {
		struct directory	*de;
		char			*p;
		char			*xp;
		struct wargs		*wa = state->auxp;

		de = wa->dir;
		/*
		 * Loop down deeper and deeper until we find the
		 * correct insertion spot.
		 * Canonicalize the filename while parsing it.
		 */
		for (xp = &nm[state->auxi]; xp < nm + state->base; ) {
			do {
				while (xp[0] == '.' && xp[1] == '/')
					xp += 2;
				while (xp[0] == '/')
					xp += 1;
				if (xp[0] == '.' && xp[1] == '.' && xp[2] == '/') {
					if (de && de != root) {
						de = de->parent;
						xp += 2;
					}
				}
			} while ((xp[0] == '/') || (xp[0] == '.' && xp[1] == '/'));
			p = strchr(xp, PATH_SEPARATOR);
			if (p == NULL)
				break;
			*p = '\0';
			if (debug) {
				error("BASE Point:'%s' in '%s : %s' (%s)\n",
					xp,
					de->whole_name,
					de->de_name,
					nm);
			}
			de = find_or_create_directory(de,
				nm,
				NULL, TRUE);
			*p = PATH_SEPARATOR;
			xp = p + 1;
		}

		if (state->base == 0) {
			char	*short_name;
			/*
			 * This is one of the path type arguments to -find
			 */
			short_name = wa->name;
			if (short_name == NULL) {
				short_name = strrchr(nm, PATH_SEPARATOR);
				if (short_name == NULL || short_name < nm) {
					short_name = nm;
				} else {
					short_name++;
					if (*short_name == '\0')
						short_name = nm;
				}
			}
			if (S_ISDIR(fs->st_mode))
				attach_dot_entries(de, fs, fs);
			else
				insert_file_entry(de, nm, short_name, NULL, 0);
		} else {
			int	ret = insert_file_entry(de,
						nm, nm + state->base, fs, 0);

			if (S_ISDIR(fs->st_mode)) {
				int		sret;
				struct stat	parent;

				if (ret == 0) {
					/*
					 * Direcory nesting too deep.
					 */
					state->flags |= WALK_WF_PRUNE;
					return (0);
				}
				sret = stat(".", &parent);
				de = find_or_create_directory(de,
						nm,
						NULL, TRUE);
				attach_dot_entries(de,
						fs,
						sret < 0 ? NULL:&parent);
			}
		}
	}
	return (0);
}
#endif
