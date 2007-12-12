/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
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
 * This file contains modifications Copyright 2006-2007 J. Schilling
 *
 * @(#)dofile.c	1.5 07/02/17 J. Schilling
 */
#if defined(sun) || defined(__GNUC__)

#ident "@(#)dofile.c 1.5 07/02/17 J. Schilling"
#endif
/*
 * @(#)dofile.c 1.12 06/12/12
 */

#ident	"@(#)dofile.c"
#ident	"@(#)sccs:lib/comobj/dofile.c"
# include	<defines.h>
# include	<schily/dirent.h>

char	had_dir;
char	had_standinp;

/*
 * Macro to skip the following names: "", ".", "..".
 */
#define	dot_dotdot(n)	((n)[(n)[0] != '.' ? 0 : (n)[1] != '.' ? 1 : 2] == '\0')

void
do_file(p,func,check_file)
register char *p;
void (*func) __PR((char *));
int check_file;
{
	extern char *Ffile;
	int fd;
	char str[FILESIZE];
	char ibuf[FILESIZE];
	DIR	*dirf;
	struct dirent *dir[2];

	if ((p[0] == '-' ) && (!p[1])) {
		/* this is make sure that the arguements starting with
		** a hyphen are handled as regular files and stdin
		** is used for accepting file names when a hyphen is
		** not followed by any characters.
		*/
		had_standinp = 1;
		while (fgets(ibuf, sizeof (ibuf), stdin) != NULL) {
			size_t	l;

			l = strlen(ibuf) - 1;
			if (l >= 0 && ibuf[l] == '\n')
				ibuf[l] = '\0';

			if (exists(ibuf) && (Statbuf.st_mode & S_IFMT) == S_IFDIR) {
				had_dir = 1;
				Ffile = ibuf;
				if((dirf = opendir(ibuf)) == NULL)
					return;
				while ((dir[0] = readdir(dirf)) != NULL) {
					if (dot_dotdot(dir[0]->d_name))
						continue;
#ifdef	HAVE_DIRENT_D_INO
					if(dir[0]->d_ino == 0) continue;
#endif
					sprintf(str,"%s/%s",ibuf,dir[0]->d_name);
					if(sccsfile(str)) {
					   if (check_file && (fd=open(str, O_RDONLY|O_BINARY)) < 0) {
					      errno = 0;
					   } else {
					        if (check_file) close(fd);
						Ffile = str;
						(*func)(str);
					   } 
					}
				}
				closedir(dirf);
			}
			else if (sccsfile(ibuf)) {
				if (check_file && (fd=open(ibuf, O_RDONLY|O_BINARY)) < 0) {
				   errno = 0;
				} else {
				   if (check_file) close(fd);
				   Ffile = ibuf;
				   (*func)(ibuf);
				}   
			}
		}
	}
	else if (exists(p) && (Statbuf.st_mode & S_IFMT) == S_IFDIR) {
		had_dir = 1;
		Ffile = p;
		if (!check_permission_SccsDir(p)) {
			return;
		}
		if((dirf = opendir(p)) == NULL)
			return;
		while ((dir[0] = readdir(dirf)) != NULL) {
			if (dot_dotdot(dir[0]->d_name))
				continue;
#ifdef	HAVE_DIRENT_D_INO
			if(dir[0]->d_ino == 0) continue;
#endif
			sprintf(str,"%s/%s",p,dir[0]->d_name);
			if(sccsfile(str)) {
			   if (check_file && (fd=open(str, O_RDONLY|O_BINARY)) < 0) {
			      errno = 0;
			   } else {
			      if (check_file) close(fd);
			      Ffile = str;
			      (*func)(str);
			   }	
			}
		}
		closedir(dirf);
	}
	else {
		if (strlen(p) < sizeof(str)) {
			strcpy(str, p);
		} else {
			strncpy(str, p, sizeof(str));
		}
		if (!check_permission_SccsDir(dname(str))) {
			return;
		}
		Ffile = p;
		(*func)(p);
	}
}
