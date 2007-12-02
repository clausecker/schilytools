/* @(#)change.c	1.34 06/09/26 Copyright 1985, 87-90, 95-99, 2000-2006 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)change.c	1.34 06/09/26 Copyright 1985, 87-90, 95-99, 2000-2006 J. Schilling";
#endif
/*
 *	find pattern and substitute in files
 *
 *	Copyright (c) 1985, 87-90, 95-99, 2000-2006 J. Schilling
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
#include <schily/varargs.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/errno.h>
#include <schily/stat.h>
#	define	FILEDESC	struct stat
#	define	f_modes		st_mode
#	define	fdfileno(fdp)	((fdp)->st_ino)
#	define	fsof(fdp)	((fdp)->st_dev)
#include <signal.h>
#include <schily/standard.h>
#include <schily/patmatch.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/libport.h>

/*
 *	check for same file descriptor
 */
#define	samefd(fd1, fd2)	(fsof(fd1) == fsof(fd2) && fdfileno(fd1) == fdfileno(fd2))


#define	MAXLINE	8196
#define	MAXNAME	1024


int	Vflag = 0;
int	Cflag = 0;
int	Iflag = 0;
int	Nflag = 0;
int	CHcnt = MAXLINE;

char	oline[MAXLINE];
char	newline[MAXLINE];
char	tmpname[MAXNAME];
int	*aux;
int	*state;
FILE	*tty;
FILEDESC ostat;	/* Old stat buf */
FILEDESC cstat;	/* Changed file stat buf */

LOCAL	void	usage	__PR((int excode));
LOCAL	RETSIGTYPE intr	__PR((int signo));
EXPORT	int	main	__PR((int ac, char **av));
LOCAL	int	match	__PR((char *pat, int *auxp, char *linep, int off, int len, int alt));
LOCAL	int	change	__PR((char *name, FILE *ifile, FILE *ofile, char *pat, char *subst, int *auxp, int alt));
LOCAL	int	catsubst __PR((char *linep, char *newp, int start, int end, char *subst, int idx, int max));
LOCAL	int	appchar	__PR((int c, char *buf, int off, int max));
LOCAL	void	showchange __PR((char *name, char *linep, int start, int end, char *subst, int out, int max));
LOCAL	BOOL	yes	__PR((char *form, ...));
LOCAL	void	mktmp	__PR((char *name));
LOCAL	BOOL	mkbak	__PR((char *name));

LOCAL void
usage(excode)
	int	excode;
{
	error("Usage:	change [options] pattern substitution [file1...filen]\n");
	error("Options:\n");
	error("	-v	Display Text before change\n");
	error("	-c	Display Text after change\n");
	error("	-i	Interactive prompting for each change\n");
	error("	-#	Maximum number of changes per line\n");
	error("	-nobak	Do not create filename.bak\n");
	error("	-help	Print this help.\n");
	error("	-version Print version number.\n");
	error("	Standard input will be used if no files given.\n");
	exit(excode);
	/* NOTREACHED */
}

LOCAL RETSIGTYPE
intr(signo)
	int	signo;
{
	if (tmpname[0] != '\0')
		unlink(tmpname);
	exit(signo);
	/* NOTREACHED */

	/*
	 * Was ist wenn RETSIGTYPE == int ?
	 */
}

EXPORT int
main(ac, av)
	int	ac;
	char	**av;
{
	FILE	*file;
	FILE	*tfile;
	char	*pat;
	int	alt;
	char	*subst;
	int	len;
	int	cnt;
	char	*opt = "help,version,v,c,i,#,nobak,n";
	BOOL	help = FALSE;
	BOOL	prversion = FALSE;
	BOOL	closeok;
	int	cac;
	char	* const *cav;

	save_args(ac, av);

	signal(SIGHUP, intr);
	signal(SIGINT, intr);
	signal(SIGTERM, intr);

	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, opt,
					&help, &prversion,
					&Vflag, &Cflag, &Iflag,
					&CHcnt,
					&Nflag, &Nflag) < 0) {
		error("Bad flag: '%s'\n", cav[0]);
		usage(EX_BAD);
	}
	if (help) usage(0);
	if (prversion) {
		printf("Change release %s (%s-%s-%s) Copyright (C) 1985, 87-90, 95-99, 2000-2006 Jörg Schilling\n",
				"1.34",
				HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	if (Iflag) Cflag = 0;

	cac = ac;
	cav = av;
	if (getfiles(&cac, &cav, opt) <= 0) {
		error("No pattern or substitution given.\n");
		usage(EX_BAD);
	}
	pat = cav[0];
	cac--, cav++;
	len = strlen(pat);

	/*
	 * Cygwin32 (and newer Linux versions too) make
	 * stdin/stdout/stderr non constant expressions so we cannot do
	 * loader initialization.
	 *
	 * XXX May this be a problem?
	 */
	tty = stdin;

	aux = malloc(sizeof (int)*len);
	state = malloc(sizeof (int)*(len+1));
	if (aux == NULL || state == NULL)
		comerrno(EX_BAD, "No memory for pattern compiler");

	if ((alt = patcompile((unsigned char *)pat, len, aux)) == 0)
		comerrno(EX_BAD, "Bad pattern '%s'.\n", pat);

	if (getfiles(&cac, &cav, opt) <= 0) {
		error("No substitution given.\n");
		usage(EX_BAD);
	}
	subst = cav[0];
	if (strlen(subst) >= MAXLINE)
		comerrno(EX_BAD, "Substitution pattern too long.\n");
	cac--, cav++;

	if (getfiles(&cac, &cav, opt) <= 0) {
		/*
		 * If no args, change is a filter from stdin to stdout.
		 */
		if (Iflag) {
#ifdef	HAVE_DEV_TTY
			if ((tty = fileopen("/dev/tty", "r")) == NULL)
				comerr("Can't open '/dev/tty'\n");
#else
			tty = stderr;
#endif
		}
		(void) change("", stdin, stdout, pat, subst, aux, alt);
		flush();
	} else for (; getfiles(&cac, &cav, opt) > 0; cac--, cav++) {
		file = fileopen(cav[0], "r");
		if (file == NULL) {
			errmsg("Can't open '%s'.\n", cav[0]);
		} else {
			mktmp(cav[0]);
			if ((tfile = fileopen(tmpname, "wct")) == NULL) {
				errmsg("Can't open '%s'.\n", tmpname);
			} else {
#ifdef	HAVE_FSYNC
				int	err;
				int	scnt;
#endif
				stat(cav[0], &ostat);
				stat(tmpname, &cstat);
				chmod(tmpname, ostat.f_modes);
				cnt = change(cav[0], file, tfile, pat, subst,
							aux, alt);
				fclose(file);

				closeok = TRUE;
				if (fflush(tfile) != 0)
					closeok = FALSE;
#ifdef	HAVE_FSYNC
				err = 0;
				scnt = 0;
				do {
					if (fsync(fdown(tfile)) != 0)
						err = geterrno();

					if (err == EINVAL)
						err = 0;
				} while (err == EINTR && ++scnt < 10);
				if (err != 0)
					closeok = FALSE;
#endif
				if (fclose(tfile) != 0)
					closeok = FALSE;

				if (!closeok && cnt > 0)
					errmsg("Problems flushing outfile for '%s'.\n",
							cav[0]);

				if (closeok && cnt > 0 && mkbak(cav[0]))
					chmod(cav[0], ostat.f_modes);
				else
					unlink(tmpname);
			}
		}
	}
	exit(0);
	/* NOTREACHED */
	return (0);	/* Keep lint happy */
}

/*
 * Return index of first char after matching pattern (if any) in line.
 * If no match, return (-1).
 * Start from position off in linep.
 */
LOCAL int
match(pat, auxp, linep, off, len, alt)
	char	*pat;
	int	*auxp;
	char	*linep;
	int	off;
	int	len;
	int	alt;
{
	char	*p;

	p = (char *)patmatch((unsigned char *)pat, auxp,
					(unsigned char *)linep, off, len, alt, state);
	if (p == NULL)
		return (-1);
	else
		return (p-linep);
}

/*
 * Copy ifile to ofile, replace pattern with substitution.
 */
LOCAL int
change(name, ifile, ofile, pat, subst, auxp, alt)
	char	*name;
	FILE	*ifile;
	FILE	*ofile;
	char	*pat;
	char	*subst;
	int	*auxp;
	int	alt;
{
	register char	*linep	= oline;
	register char	*newp	= newline;
	int	idx;
	int	cnt = 0;
	int	matend;
	int	lastmend;
	int	out;
	int	llen;
	int	changed;

	while ((llen = fgetline(ifile, linep, MAXLINE)) >= 0) {
		if (llen >= MAXLINE-1) {
			errmsgno(EX_BAD, "Input line truncated: %s\n", linep);
			/*
			 * Abort if not in filter mode.
			 */
			if (ofile != stdout)
				return (-1);
		}
		changed = 0;
		out = 0;
		lastmend = -1;
		for (idx = 0; linep[idx] != 0; ) {
			matend = match(pat, auxp, linep, idx, llen, alt);
			if (matend >= 0 && matend != lastmend) {
				lastmend = matend;
				if (changed >= CHcnt) {
					matend = llen;
					out = catsubst(linep, newp, idx, matend, "&", out, MAXLINE);
					break;
				}
				if (Vflag && !changed)
					fprintf(stderr, "%s%s%s\n",
						name, *name?": ":"", linep);
				if (Iflag) {
					showchange(name, linep, idx, matend, subst, out, MAXLINE);
					if (!yes("OK? ")) {
						out = catsubst(linep, newp, idx, matend, "&", out, MAXLINE);
						idx = matend;
						continue;
					}
				}
				out = catsubst(linep, newp, idx, matend, subst, out, MAXLINE);
				cnt++;
				changed++;
			}
			if (matend == -1 || matend == idx)
				out = appchar(linep[idx++], newp, out, MAXLINE);
			else
				idx = matend;
		}
		if (appchar('\0', newp, out, MAXLINE) < 0) {
			errmsgno(EX_BAD, "Output line truncated: %s\n", newp);
			/*
			 * Abort if not in filter mode.
			 */
			if (ofile != stdout)
				return (-1);
		}
		fprintf(ofile, "%s\n", newp);
		if (Cflag && changed)
			fprintf(stderr, "%s%s%s\n", name, *name?": ":"", newp);
	}
	return (cnt);
}

/*
 * Concatenate substitution to current version of new line.
 */
LOCAL int
catsubst(linep, newp, start, end, subst, idx, max)
	char	*linep;
	char	*newp;
	int	start;
	int	end;
	char	*subst;
	int	idx;
	int	max;
{
	int	i;

	while (*subst != '\0') {
		if (*subst == '&') {
			for (i = start; i < end; i++)
				idx = appchar(linep[i], newp, idx, max);
			subst++;
		} else {
			if (*subst == '\\')
				subst++;
			idx = appchar(*subst++, newp, idx, max);
		}
	}
	return (idx);
}

/*
 * Append a character to the buffer, use position off.
 */
LOCAL int
appchar(c, buf, off, max)
	char	c;
	char	*buf;
	int	off;
	int	max;
{
	if (off < 0 || off >= max)
		return (-1);
	buf[off++] = c;
	return (off);
}

LOCAL void
showchange(name, linep, start, end, subst, out, max)
	char	*name;
	char	*linep;
	int	start;
	int	end;
	char	*subst;
	int	out;
	int	max;
{
	char	tmp[MAXLINE];
	char	*newp = tmp;

	movebytes(newline, tmp, max);
	out = catsubst(linep, newp, start, end, subst, out, max);
	if (appchar('\0', newp, out, max) < 0)
		error("Line will be truncated!\n");
	fprintf(stderr, "%s%s%s%s\n", name, *name?": ":"", newp, &linep[end]);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL BOOL
yes(char *form, ...)
#else
LOCAL BOOL
yes(form, va_alist)
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	char	okbuf[10];

#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	fprintf(stderr, "%r", form, args);
	va_end(args);
	flush();
	fgetline(tty, okbuf, sizeof (okbuf));
	if (streql(okbuf, "y") || streql(okbuf, "yes"))
		return (TRUE);
	else
		return (FALSE);
}

LOCAL void
mktmp(name)
	char	*name;
{
	char	*p  = NULL;
	char	*p2 = name;
	char	c = '\0';

	/*
	 * Extract dir from name
	 */
	while (*p2) {
#ifdef	tos
		if (*p2++ == '\\')
#else
		if (*p2++ == '/')
#endif
			p = p2;
	}
	if (p) {
		c = *p;
		*p = '\0';
	} else {
		name  = "";
	}
	js_snprintf(tmpname, sizeof (tmpname), "%sch%llo", name, (Llong)getpid());
	if (p)
		*p = c;
}

LOCAL BOOL
mkbak(name)
	char	*name;
{
	char	bakname[MAXNAME];

	if (!Nflag) {
		/*
		 * Try to create a backup file.
		 */
		if (strlen(name) > (MAXNAME-5) &&
		    strchr(&name[MAXNAME-5], '/') != NULL) {
			errmsgno(EX_BAD, "Cannot backup '%s'; name too long\n", name);
			return (FALSE);
		}

		strncpy(bakname, name, MAXNAME-5);
		bakname[MAXNAME-5] = '\0';
		strcat(bakname, ".bak");

		if (rename(name, bakname) < 0) { /* make cur file .bak	*/

			errmsg("Cannot backup '%s'\n", name);
			return (FALSE);
		}
	}

	if (rename(tmpname, name) < 0) {	/* rename new file	*/
		errmsg("Cannot rename '%s' to '%s'\n", tmpname, name);
		if (!Nflag) {
			/*
			 *try make .bak current again.
			 */
			if (rename(bakname, name) < 0)
				errmsg("Cannot rename backup '%s' back to '%s'\n",
					bakname, name);
		}
		return (FALSE);
	}
	return (TRUE);
}

