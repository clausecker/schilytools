/* @(#)fexec.c	1.32 07/07/01 Copyright 1985, 1995-2007 J. Schilling */
/*
 *	Execute a program with stdio redirection
 *
 *	Copyright (c) 1985, 1995-2007 J. Schilling
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
#include <stdio.h>
#include <schily/standard.h>
#define	fexecl	__nothing_1_	/* prototype in schily/schily.h is wrong */
#define	fexecle	__nothing_2_	/* prototype in schily/schily.h is wrong */
#include <schily/schily.h>
#undef	fexecl
#undef	fexecle
	int fexecl	__PR((const char *, FILE *, FILE *, FILE *, ...));
	int fexecle	__PR((const char *, FILE *, FILE *, FILE *, ...));
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/varargs.h>
#include <schily/errno.h>
#include <schily/fcntl.h>
#include <schily/dirent.h>
#include <schily/maxpath.h>

/*
 * Check whether fexec may be implemented...
 */
#if	defined(HAVE_DUP) && (defined(HAVE_DUP2) || defined(F_DUPFD))


#define	MAX_F_ARGS	16

#if	defined(IS_MACOS_X) && defined(HAVE_CRT_EXTERNS_H)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * We need to fetch the address of "environ" using a hack.
 */
#include <crt_externs.h>
#define	environ	*_NSGetEnviron()
#else
extern	char **environ;
#endif

LOCAL void	 fdcopy __PR((int, int));
LOCAL void	 fdmove __PR((int, int));
LOCAL const char *chkname __PR((const char *, const char *));
LOCAL const char *getpath __PR((char * const *));

#ifdef	PROTOTYPES
EXPORT int
fexecl(const char *name, FILE *in, FILE *out, FILE *err, ...)
#else
EXPORT int
fexecl(name, in, out, err, va_alist)
	char	*name;
	FILE	*in;
	FILE	*out;
	FILE	*err;
	va_dcl
#endif
{
	va_list	args;
	int	ac = 0;
	char	*xav[MAX_F_ARGS];
	char	**av;
	char	**pav;
	char	*p;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, err);
#else
	va_start(args);
#endif
	while (va_arg(args, char *) != NULL)
		ac++;
	va_end(args);

	if (ac < MAX_F_ARGS) {
		pav = av = xav;
	} else {
		pav = av = (char **)malloc((ac+1)*sizeof (char *));
		if (av == 0)
			return (-1);
	}

#ifdef	PROTOTYPES
	va_start(args, err);
#else
	va_start(args);
#endif
	do {
		p = va_arg(args, char *);
		*pav++ = p;
	} while (p != NULL);
	va_end(args);

	ret = fexecv(name, in, out, err, ac, av);
	if (av != xav)
		free(av);
	return (ret);
}

#ifdef	PROTOTYPES
EXPORT int
fexecle(const char *name, FILE *in, FILE *out, FILE *err, ...)
#else
EXPORT int
fexecle(name, in, out, err, va_alist)
	char	*name;
	FILE	*in;
	FILE	*out;
	FILE	*err;
	va_dcl
#endif
{
	va_list	args;
	int	ac = 0;
	char	*xav[MAX_F_ARGS];
	char	**av;
	char	**pav;
	char	*p;
	char	**env;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, err);
#else
	va_start(args);
#endif
	while (va_arg(args, char *) != NULL)
		ac++;
	env = va_arg(args, char **);
	va_end(args);

	if (ac < MAX_F_ARGS) {
		pav = av = xav;
	} else {
		pav = av = (char **)malloc((ac+1)*sizeof (char *));
		if (av == 0)
			return (-1);
	}

#ifdef	PROTOTYPES
	va_start(args, err);
#else
	va_start(args);
#endif
	do {
		p = va_arg(args, char *);
		*pav++ = p;
	} while (p != NULL);
	va_end(args);

	ret = fexecve(name, in, out, err, av, env);
	if (av != xav)
		free(av);
	return (ret);
}

EXPORT int
fexecv(name, in, out, err, ac, av)
	const char *name;
	FILE *in, *out, *err;
	int ac;
	char *av[];
{
	av[ac] = NULL;			/*  force list to be null terminated */
	return (fexecve(name, in, out, err, av, environ));
}

EXPORT int
fexecve(name, in, out, err, av, env)
	const char *name;
	FILE *in, *out, *err;
	char * const av[], * const env[];
{
	char	nbuf[MAXPATHNAME+1];
	char	*np;
	const char *path;
	int	ret;
	int	fin;
	int	fout;
	int	ferr;
#ifndef	JOS
	int	o[3];		/* Old fd's for stdin/stdout/stderr */
	int	f[3];		/* Old close on exec flags for above  */
	int	errsav;

	o[0] = o[1] = o[2] = -1;
	f[0] = f[1] = f[2] = 0;
#endif

	fflush(out);
	fflush(err);
	fin  = fdown(in);
	fout = fdown(out);
	ferr = fdown(err);
#ifdef JOS

	/*
	 * If name contains a pathdelimiter ('/' on unix)
	 * or name is too long ...
	 * try exec without path search.
	 */
	if (find('/', name) || strlen(name) > MAXFILENAME) {
		ret = exec_env(name, fin, fout, ferr, av, env);

	} else if ((path = getpath(env)) == NULL) {
		ret = exec_env(name, fin, fout, ferr, av, env);
		if ((ret == ENOFILE) && strlen(name) <= (sizeof (nbuf) - 6)) {
			strcatl(nbuf, "/bin/", name, (char *)NULL);
			ret = exec_env(nbuf, fin, fout, ferr, av, env);
			if (ret == EMISSDIR)
				ret = ENOFILE;
		}
	} else {
		int	nlen = strlen(name);

		for (;;) {
			np = nbuf;
			/*
			 * JOS always uses ':' as PATH Environ separator
			 */
			while (*path != ':' && *path != '\0' &&
				np < &nbuf[MAXPATHNAME-nlen-2]) {

				*np++ = *path++;
			}
			*np = '\0';
			if (*nbuf == '\0')
				strcatl(nbuf, name, (char *)NULL);
			else
				strcatl(nbuf, nbuf, "/", name, (char *)NULL);
			ret = exec_env(nbuf, fin, fout, ferr, av, env);
			if (ret == EMISSDIR)
				ret = ENOFILE;
			if (ret != ENOFILE || *path == '\0')
				break;
			path++;
		}
	}
	return (ret);

#else	/* JOS */

	if (fin != STDIN_FILENO) {
#ifdef	F_GETFD
		f[0] = fcntl(STDIN_FILENO, F_GETFD, 0);
#endif
		o[0] = dup(STDIN_FILENO);
#ifdef	F_SETFD
		fcntl(o[0], F_SETFD, 1);
#endif
		fdmove(fin, STDIN_FILENO);
	}
	if (fout != STDOUT_FILENO) {
#ifdef	F_GETFD
		f[1] = fcntl(STDOUT_FILENO, F_GETFD, 0);
#endif
		o[1] = dup(STDOUT_FILENO);
#ifdef	F_SETFD
		fcntl(o[1], F_SETFD, 1);
#endif
		fdmove(fout, STDOUT_FILENO);
	}
	if (ferr != STDERR_FILENO) {
#ifdef	F_GETFD
		f[2] = fcntl(STDERR_FILENO, F_GETFD, 0);
#endif
		o[2] = dup(STDERR_FILENO);
#ifdef	F_SETFD
		fcntl(o[2], F_SETFD, 1);
#endif
		fdmove(ferr, STDERR_FILENO);
	}

	/*
	 * If name contains a pathdelimiter ('/' on unix)
	 * or name is too long ...
	 * try exec without path search.
	 */
#ifdef	FOUND_MAXFILENAME
	if (strchr(name, '/') || strlen(name) > (unsigned)MAXFILENAME) {
#else
	if (strchr(name, '/')) {
#endif
		ret = execve(name, av, env);

	} else if ((path = getpath(env)) == NULL) {
		ret = execve(name, av, env);
		if ((geterrno() == ENOENT) && strlen(name) <= (sizeof (nbuf) - 6)) {
			strcatl(nbuf, "/bin/", name, (char *)NULL);
			ret = execve(nbuf, av, env);
		}
	} else {
		int	nlen = strlen(name);

		for (;;) {
			np = nbuf;
			while (*path != PATH_ENV_DELIM && *path != '\0' &&
				np < &nbuf[MAXPATHNAME-nlen-2]) {

				*np++ = *path++;
			}
			*np = '\0';
			if (*nbuf == '\0')
				strcatl(nbuf, name, (char *)NULL);
			else
				strcatl(nbuf, nbuf, "/", name, (char *)NULL);
			ret = execve(nbuf, av, env);
			if (geterrno() != ENOENT || *path == '\0')
				break;
			path++;
		}
	}
	errsav = geterrno();
			/* reestablish old files */
	if (ferr != STDERR_FILENO) {
		fdmove(STDERR_FILENO, ferr);
		fdmove(o[2], STDERR_FILENO);
#ifdef	F_SETFD
		if (f[2] == 0)
			fcntl(STDERR_FILENO, F_SETFD, 0);
#endif
	}
	if (fout != STDOUT_FILENO) {
		fdmove(STDOUT_FILENO, fout);
		fdmove(o[1], STDOUT_FILENO);
#ifdef	F_SETFD
		if (f[1] == 0)
			fcntl(STDOUT_FILENO, F_SETFD, 0);
#endif
	}
	if (fin != STDIN_FILENO) {
		fdmove(STDIN_FILENO, fin);
		fdmove(o[0], STDIN_FILENO);
#ifdef	F_SETFD
		if (f[0] == 0)
			fcntl(STDIN_FILENO, F_SETFD, 0);
#endif
	}
	seterrno(errsav);
	return (ret);

#endif	/* JOS */
}

#ifndef	JOS

LOCAL void
fdcopy(fd1, fd2)
	int	fd1;
	int	fd2;
{
	close(fd2);
#ifdef	F_DUPFD
	fcntl(fd1, F_DUPFD, fd2);
#else
#ifdef	HAVE_DUP2
	dup2(fd1, fd2);
#endif
#endif
}

LOCAL void
fdmove(fd1, fd2)
	int	fd1;
	int	fd2;
{
	fdcopy(fd1, fd2);
	close(fd1);
}

#endif

/*----------------------------------------------------------------------------
|
|	get PATH from env
|
+----------------------------------------------------------------------------*/

LOCAL const char *
getpath(env)
	char	* const *env;
{
	char * const *p = env;
	const char *p2;

	if (p != NULL) {
		while (*p != NULL) {
			if ((p2 = chkname("PATH", *p)) != NULL)
				return (p2);
			p++;
		}
	}
	return (NULL);
}


/*----------------------------------------------------------------------------
|
| Check if name is in environment.
| Return pointer to value name is found.
|
+----------------------------------------------------------------------------*/

LOCAL const char *
chkname(name, ev)
	const char	*name;
	const char	*ev;
{
	for (;;) {
		if (*name != *ev) {
			if (*ev == '=' && *name == '\0')
				return (++ev);
			return (NULL);
		}
		name++;
		ev++;
	}
}

#endif	/* defined(HAVE_DUP) && (defined(HAVE_DUP2) || defined(F_DUPFD)) */
