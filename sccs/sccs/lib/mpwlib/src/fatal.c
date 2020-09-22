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
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)fatal.c	1.14 20/09/06 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)fatal.c 1.14 20/09/06 J. Schilling"
#endif
/*
 * @(#)fatal.c 1.8 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)fatal.c"
#pragma ident	"@(#)sccs:lib/mpwlib/fatal.c"
#endif
# include	<defines.h>
# include	<fatal.h>
# include	<had.h>

/*
	General purpose error handler.
	Typically, low level subroutines which detect error conditions
	(an open or create routine, for example) return the
	value of calling fatal with an appropriate error message string.
	E.g.,	return(fatal("can't do it"));
	Higher level routines control the execution of fatal
	via the global word Fflags.
	The macros FSAVE and FRSTR in <fatal.h> can be used by higher
	level subroutines to save and restore the Fflags word.
 
	The argument to fatal is a pointer to an error message string.
	The action of this routine is driven completely from
	the "Fflags" global word (see <fatal.h>).
	The following discusses the interpretation of the various bits
	of Fflags.
 
	The FTLMSG bit controls the writing of the error
	message on file descriptor 2.  The message is preceded
	by the string "ERROR: ", unless the global character pointer
	"Ffile" is non-zero, in which case the message is preceded
	by the string "ERROR [<Ffile>]: ".  A newline is written
	after the user supplied message.
 
	If the FTLCLN bit is on, clean_up is called with an
	argument of 0 (see clean.c).
 
	If the FTLFUNC bit is on, the function pointed to by the global
	function pointer "Ffunc" is called with the user supplied
	error message pointer as argument.
	(This feature can be used to log error messages).
 
	The FTLACT bits determine how fatal should return.
	If the FTLJMP bit is on longjmp(Fjmp) is called
       (Fjmp is a global vector, see <setjmp.h>).
 
	If the FTLEXIT bit is on the value of userexit(1) is
	passed as an argument to exit(II)
	(see userexit.c).
 
	If none of the FTLACT bits are on
	(the default value for Fflags is 0), the global word
	"Fvalue" (initialized to -1) is returned.
 
	If all fatal globals have their default values, fatal simply
	returns -1.
*/

int	Fcnt;
int	Fflags;
char	*Ffile;
int	Fvalue = -1;
int	(*Ffunc) __PR((char *));
jmp_buf	Fjmp;
char    *nsedelim = (char *) 0;

	void	set_clean_up	__PR((void (*f)(void)));
static	void	dummy_clean_up	__PR((void));
	void	(*f_clean_up) __PR((void)) = dummy_clean_up;

/* default value for NSE delimiter (currently correct, if NSE ever
 * changes implementation it will have to pass new delimiter as
 * value for -q option)
 */
# define NSE_VCS_DELIM "vcs"
# define NSE_VCS_GENERIC_NAME "<history file>"
static  int     delimlen = 0;

int
efatal(msg)
char *msg;
{
	int	errsav = errno;
	char	*errstr = NULL;

	if (Fflags & FTLMSG) {
#ifdef	SCHILY_BUILD
		errstr = get_progname();
#else
#ifdef	HAVE_GETPROGNAME
		errstr = (char *)getprogname();
#endif
#endif
		if (errstr) {
			write(2, errstr, length(errstr));
			write(2, ": ", 2);
		}

		errno = 0;
		errstr = NULL;
#ifdef	SCHILY_BUILD
		errstr = errmsgstr(errsav);
#else
#ifdef	HAVE_STRERROR
		errstr = strerror(errsav);
		if (errno)
			errstr = NULL;
#endif
#endif
		if (errsav != 0 && errstr != NULL) {
			write(2, errstr, length(errstr));
			write(2, ". ", 2);
		}
	}
	return (fatal(msg));
}

int
fatal(msg)
char *msg;
{
#if	defined(IS_MACOS_X)
/*
 * The Mac OS X static linker is too silly to link in .o files from static libs
 * if only a variable is referenced. The elegant workaround for this bug (using
 * common variables) triggers a different bug in the dynamic linker from Mac OS
 * that is unable to link common variables. This forces us to introduce funcs
 * that need to be called from central places to enforce to link in the vars.
 */
extern	void __mpw __PR((void));

	__mpw();
#endif
	++Fcnt;
	if (Fflags & FTLMSG) {
		write(2,gettext("ERROR"),5);
		if (Ffile) {
			(void) write(2," [",2);
			(void) write(2,Ffile,length(Ffile));
			(void) write(2,"]",1);
		}
		(void) write(2,": ",2);
		(void) write(2,msg,length(msg));
		(void) write(2,"\n",1);
	}
	if (Fflags & FTLCLN)
		(*f_clean_up)();
	if (Fflags & FTLFUNC)
		(*Ffunc)(msg);
	switch (Fflags & FTLACT) {
	case FTLJMP:
		longjmp(Fjmp, 1);
	/*FALLTHRU*/
	case FTLEXIT:
		if (Fflags & FTLVFORK)
			_exit(userexit(1));
		exit(userexit(1));
	/*FALLTHRU*/
	case FTLRET:
		return(Fvalue);
	}
	return(-1);
}

/* if running under NSE, the path to the directory which heads the
 * vcs hierarchy and the "s." is removed from the names of s.files
 *
 * if `vcshist' is true, a generic name for the history file is returned.
 */
 
char *
nse_file_trim(f, vcshist)
        char    *f;
        int     vcshist;
{
        register char   *p;
        char            *q;
        char            *r;
        char            c;
 
        r = f;
        if (HADQ) {
                if (vcshist && Ffile && (0 == strcmp(Ffile, f))) {
                        return NSE_VCS_GENERIC_NAME;
                }
                if (!nsedelim) {
                        nsedelim = NSE_VCS_DELIM;
                }
                if (delimlen == 0) {
                        delimlen = strlen(nsedelim);
                }
                p = f;
                while ((c = *p++) != '\0') {
                        /* find the NSE delimiter path component */
                        if ((c == '/') && (*p == nsedelim[0]) &&
                            (0 == strncmp(p, nsedelim, delimlen)) &&
                            (*(p + delimlen) == '/')) {
                                break;
                        }
                }
                if (c) {
                        /* if found, new name starts with second "/" */
                        p += delimlen;
                        q = strrchr(p, '/');
                        /* find "s." */
                        if (q && (q[1] == 's') && (q[2] == '.')) {
                                /* build the trimmed name.
                                 * <small storage leak here> */
                                q[1] = '\0';
                                r = (char *) malloc((unsigned) (strlen(p) +
                                                strlen(q+3) + 1));
                                (void) strcpy(r, p);
                                q[1] = 's';
                                (void) strcat(r, q+3);
                        }
                }
        }
        return r;
}

int
check_permission_SccsDir(path)
	char	*path;
{
	int  desc;
	char dir_name[MAXPATHLEN];

	strlcpy(dir_name, sname(path), sizeof (dir_name));
	strlcat(dir_name, "/.", sizeof (dir_name));
	if ((desc = open(dir_name, O_RDONLY|O_BINARY)) < 0) {
		if (errno == EACCES) {
			fprintf(stderr, gettext("%s: Permission denied\n"), dname(dir_name));
			++Fcnt;
			return (0);
		}
		return (1);
	}
	close(desc);
	return(1);

}

static void
dummy_clean_up()
{
}

void
set_clean_up(f)
	void (*f) __PR((void));
{
	f_clean_up = f;
}
