/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
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
 * Copyright 2006-2018 J. Schilling
 *
 * @(#)dofile.c	1.13 18/04/29 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)dofile.c 1.13 18/04/29 J. Schilling"
#endif
/*
 * @(#)dofile.c 1.12 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)dofile.c"
#pragma ident	"@(#)sccs:lib/comobj/dofile.c"
#endif
#include	<defines.h>
#include	<schily/dirent.h>

char	had_dir;
char	had_standinp;

/*
 * Macro to skip the following names: "", ".", "..".
 */
#define	dot_dotdot(n)	((n)[(n)[0] != '.' ? 0 : (n)[1] != '.' ? 1 : 2] == '\0')

void
do_file(p, func, check_file, need_sdot)
register char *p;
void (*func) __PR((char *));
int check_file;		/* Check whether file is readable */
int need_sdot;		/* Skip non s. files */
{
	extern char *Ffile;
	int fd;
	char str[FILESIZE];
	char ibuf[FILESIZE];
	DIR	*dirf;
	struct dirent *dir[2];

	had_standinp = 0;
	had_dir = 0;

	if ((p[0] == '-') && (!p[1])) {
		/*
		 * this is make sure that the arguements starting with
		 * a hyphen are handled as regular files and stdin
		 * is used for accepting file names when a hyphen is
		 * not followed by any characters.
		 */
		had_standinp = 1;
		while (fgets(ibuf, sizeof (ibuf), stdin) != NULL) {
			size_t	l;

			l = strlen(ibuf);
			if (l > 0 && ibuf[l-1] == '\n')
				ibuf[l-1] = '\0';

			had_dir = 0;
			if (exists(ibuf) && (Statbuf.st_mode & S_IFMT) == S_IFDIR) {
				had_dir = 1;
				Ffile = ibuf;
				if ((dirf = opendir(ibuf)) == NULL)
					return;
				while ((dir[0] = readdir(dirf)) != NULL) {
					if (dot_dotdot(dir[0]->d_name))
						continue;
#ifdef	HAVE_DIRENT_D_INO
					if (dir[0]->d_ino == 0)
						continue;
#endif
					snprintf(str, sizeof (str),
						"%s/%s", ibuf, dir[0]->d_name);
					if (!need_sdot) {
						Ffile = str;
						(*func)(str);
					} else if (sccsfile(str)) {
					    if (check_file && (fd = open(str, O_RDONLY|O_BINARY)) < 0) {
						errno = 0;
					    } else {
						if (check_file)
							close(fd);
						Ffile = str;
						(*func)(str);
					    }
					}
				}
				closedir(dirf);
			} else if (!need_sdot) {
				Ffile = ibuf;
				(*func)(ibuf);
			} else if (sccsfile(ibuf)) {
				if (check_file && (fd = open(ibuf, O_RDONLY|O_BINARY)) < 0) {
				    errno = 0;
				} else {
				    if (check_file)
					close(fd);
				    Ffile = ibuf;
				    (*func)(ibuf);
				}
			}
		}
	} else if (exists(p) && (Statbuf.st_mode & S_IFMT) == S_IFDIR) {
		had_dir = 1;
		Ffile = p;
		if (!check_permission_SccsDir(p)) {
			return;
		}
		if ((dirf = opendir(p)) == NULL)
			return;
		while ((dir[0] = readdir(dirf)) != NULL) {
			if (dot_dotdot(dir[0]->d_name))
				continue;
#ifdef	HAVE_DIRENT_D_INO
			if (dir[0]->d_ino == 0)
				continue;
#endif
			snprintf(str, sizeof (str),
				"%s/%s", p, dir[0]->d_name);
			if (!need_sdot) {
				Ffile = str;
				(*func)(str);
			} else if (sccsfile(str)) {
			    if (check_file && (fd = open(str, O_RDONLY|O_BINARY)) < 0) {
				errno = 0;
			    } else {
				if (check_file)
					close(fd);
				Ffile = str;
				(*func)(str);
			    }
			}
		}
		closedir(dirf);
	} else {
		strlcpy(str, p, sizeof (str));
		if (!check_permission_SccsDir(dname(str))) {
			return;
		}
		Ffile = p;
		(*func)(p);
	}
}
