/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Copyright 2010 J. Schilling
 *
 * Be very careful with modifications as the code has been optimized
 * to fit into a single page on Solaris.
 *
 * @(#)isaexec.c	1.5 10/12/29 J. Schilling
 */

#if defined(sun)
#pragma ident	"@(#)isaexec.c 1.5 10/12/29 J. Schilling"
#pragma ident	"@(#)isaexec.c	1.3	05/06/08 SMI"
#endif

#include <schily/stdio.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/nlsdefs.h>
#include <schily/systeminfo.h>

int	main __PR((int argc, char **argv, char **envp));

/*ARGSUSED*/
int
main(argc, argv, envp)
	int	argc;
	char	**argv;
	char	**envp;
{
	const char *execname;
	const char *fname;
	char *isalist;
	ssize_t isalen;
	char *pathname;
	ssize_t len;
	char scratch[1];
	char *str;

#if	defined(USE_NLS)
#if !defined(TEXT_DOMAIN)		/* Should be defined by cc -D */
#define	TEXT_DOMAIN	"isaexec"	/* Use this only if it wasn't */
#endif
	(void) setlocale(LC_ALL, "");
	(void) textdomain(TEXT_DOMAIN);
#endif

	/*
	 * Get the executable name. On Solaris, we have getexecname()
	 * that retrives the reliable name from the kernel from the
	 * ELF aux vector.
	 */
#ifdef	HAVE_GETEXECNAME
	if ((execname = getexecname()) == NULL) {
		(void) fprintf(stderr,
		    gettext("%s: getexecname() failed\n"),
		    argv[0]);
		return (1);
	}
#else
	execname = argv[0];
#endif

	/*
	 * Get the base name from execname.
	 */
	fname = strrchr(execname, '/');
	fname = (fname != NULL) ? (fname+1) : execname;

	/*
	 * Get the isa list.
	 */
#ifdef	SI_ISALIST
	if ((isalen = sysinfo(SI_ISALIST, scratch, 1)) == -1 ||
	    (isalist = malloc(isalen)) == NULL ||
	    sysinfo(SI_ISALIST, isalist, isalen) == -1) {
		goto cannotfind;
	}
#else
	isalist = scratch;
	*isalist = '\0';
	isalen = 1;
#endif

	/*
	 * Allocate a path name buffer.  The sum of the lengths of the
	 * execname and isalist strings is guaranteed to be big enough.
	 */
	len = strlen(execname) + isalen;
	if ((pathname = malloc(len)) == NULL) {
		(void) fprintf(stderr,
		    gettext("%s: isaexec(\"%s\") failed\n"),
		    argv[0], fname);
		return (1);
	}

	/*
	 * Copy exec name and retrieve the directory name component len.
	 */
	pathname[0] = '\0';
	(void) strcat(pathname, execname);
	len = fname - execname;

	/*
	 * For each name in the isa list, look for an executable file
	 * with the given file name in the corresponding subdirectory.
	 */
	str = strtok(isalist, " ");
	if (str) do {
		pathname[len] = '\0';
		(void) strcat(pathname+len, str);
		(void) strcat(pathname+len, "/");
		(void) strcat(pathname+len, fname);
		if (access(pathname, X_OK) == 0) {
			/*
			 * File exists and is marked executable.  Attempt
			 * to execute the file from the subdirectory,
			 * using the user-supplied argv and envp.
			 */
			(void) execve(pathname, argv, envp);
			(void) fprintf(stderr,
			    gettext("%s: isaexec(\"%s\") failed\n"),
			    argv[0], pathname);
		}
	} while ((str = strtok(NULL, " ")) != NULL);

cannotfind:
	(void) fprintf(stderr,
	    gettext("%s: cannot find/execute \"%s\" in ISA subdirectories\n"),
	    argv[0], fname);

	return (1);
}
