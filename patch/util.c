/* @(#)util.c	1.25 15/05/16 2011-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)util.c	1.25 15/05/16 2011-2015 J. Schilling";
#endif
/*
 *	Copyright (c) 1986 Larry Wall
 *	Copyright (c) 2011-2015 J. Schilling
 *
 *	This program may be copied as long as you don't try to make any
 *	money off of it, or pretend that you wrote it.
 */

#define	EXT	extern
#include "common.h"
#undef	EXT
#define	EXT
#include "util.h"
#include <schily/varargs.h>
#include <schily/errno.h>

static	Llong	fetchtime	__PR((char *t));
static	void	sig_exit	__PR((int signo));

/* Rename a file, copying it if necessary. */

int
move_file(from, to)
	char	*from;
	char	*to;
{
	char bakname[512];
	char *s;
	int i;
	int fromfd;

	/* to stdout? */

	if (from && strEQ(to, "-")) {
#ifdef DEBUGGING
		if (debug & 4)
			say(_("Moving %s to stdout.\n"), from);
#endif
		fromfd = open(from, 0);
		if (fromfd < 0) {
			fatal(_("patch: internal error, can't reopen %s\n"),
				from);
		}
		/*
		 * For file copy operations, we use the minimal buf size that
		 * is better aligned.
		 */
		while ((i = read(fromfd, buf, BUFFERSIZE)) > 0)
			if (write(STDOUT_FILENO, buf, i) != i)
				fatal(_("patch: write failed\n"));
		Close(fromfd);
		return (0);
	}

	if (origprae) {
		Strcpy(bakname, origprae);
		Strcat(bakname, to);
	} else {
		Strcpy(bakname, to);
		Strcat(bakname, origext?origext:ORIGEXT);
	}
	if (do_backup &&
	    stat(to, &file_stat) >= 0) {	/* output file exists */
		dev_t to_device = file_stat.st_dev;
		ino_t to_inode  = file_stat.st_ino;
		char *simplename = bakname;

		for (s = bakname; *s; s++) {
			if (*s == '/')
				simplename = s+1;
		}
		/* find a backup name that is not the same file */
		file_stat.st_ctime = (time_t)0;
		while (stat(bakname, &file_stat) >= 0 &&
			to_device == file_stat.st_dev &&
			to_inode == file_stat.st_ino) {
			if (file_stat.st_ctime >= starttime) /* mult patches */
				goto backup_done;
			for (s = simplename; *s && !islower(*s); s++) {
				;
				/* LINTED */
			}
			if (*s)
				*s = toupper(*s);
			else
				Strcpy(simplename, simplename+1);
		}
		if (file_stat.st_ctime >= starttime) /* mult patches */
			goto backup_done;
		while (unlink(bakname) >= 0) {	/* while() is for Eunice */
			;
			/* LINTED */
		}
#ifdef DEBUGGING
		if (debug & 4)
			say(_("Moving %s to %s.\n"), to, bakname);
#endif
#ifdef	HAVE_RENAME
		if (rename(to, bakname) < 0) {
			say(_("patch: can't backup %s, output is in %s\n"),
			    to, from);
			return (-1);
		}
#else
		if (link(to, bakname) < 0) {
			say(_("patch: can't backup %s, output is in %s\n"),
			    to, from);
			return (-1);
		}
		while (unlink(to) >= 0) {
			;
			/* LINTED */
		}
#endif
	}
backup_done:
	if (from == NULL) {
		if (!do_backup) {
			if (debug & 4)
				say(_("Removing file %s\n"), to);
			if (unlink(to) != 0 && errno != ENOENT)
				fatal(_("Can't remove file %s\n"), to);
		}
		return (0);
	}
#ifdef DEBUGGING
	if (debug & 4)
		say(_("Moving %s to %s.\n"), from, to);
#endif
#ifdef	HAVE_RENAME
	if (rename(from, to) < 0) {		/* different file system? */
#else
	if (link(from, to) < 0) {		/* different file system? */
#endif
		int tofd;

		tofd = creat(to, 0666);
		if (tofd < 0) {
			say(_("patch: can't create %s, output is in %s.\n"),
			    to, from);
			return (-1);
		}
		fromfd = open(from, 0);
		if (fromfd < 0) {
			fatal(_("patch: internal error, can't reopen %s\n"),
				from);
		}
		/*
		 * For file copy operations, we use the minimal buf size that
		 * is better aligned.
		 */
		while ((i = read(fromfd, buf, BUFFERSIZE)) > 0)
			if (write(tofd, buf, i) != i)
				fatal(_("patch: write failed\n"));
		Close(fromfd);
		Close(tofd);
	}
	Unlink(from);
	return (0);
}

void
removedirs(path)
	char	*path;
{
	char	*p = strrchr(path, '/');

	if (p == NULL)
		return;
	while (p > path) {
		*p = '\0';
		if (rmdir(path) == 0 && verbose)
			say(_("Removed empty directory %s\n"),
				path);
		*p = '/';
		while (--p > path) {
			if (*p == '/')
				break;
		}
	}
}

/* Copy a file. */

void
copy_file(from, to)
	char	*from;
	char	*to;
{
	int tofd;
	int fromfd;
	int i;

	tofd = creat(to, 0666);
	if (tofd < 0)
		fatal(_("patch: can't create %s.\n"), to);
	fromfd = open(from, 0);
	if (fromfd < 0)
		fatal(_("patch: internal error, can't reopen %s\n"), from);
	/*
	 * For file copy operations, we use the minimal buf size that
	 * is better aligned.
	 */
	while ((i = read(fromfd, buf, BUFFERSIZE)) > 0)
		if (write(tofd, buf, i) != i)
			fatal(_("patch: write (%s) failed\n"), to);
	Close(fromfd);
	Close(tofd);
}

/* Allocate a unique area for a string. */

char *
savestr(s)
	char *s;
{
	char *rv;

	if (!s)
		s = "Oops";
	rv = strdup(s);
	if (rv == Nullch) {
		if (using_plan_a) {
			out_of_mem = TRUE;
		} else {
			fatal(_("patch: out of memory (savestr)\n"));
			/* NOTREACHED */
		}
	}
	return (rv);
}

/* Vanilla terminal output (buffered). */

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
say(const char *fmt, ...)
#else
EXPORT void
say(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
#ifdef	USE_VPRINTF
	(void) vfprintf(stderr, fmt, args);
#else
	(void) js_fprintf(stderr, "%r", fmt, args);
#endif
	va_end(args);
	Fflush(stderr);
}

/* Terminal output, pun intended. */

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
fatal(const char *fmt, ...)
#else
EXPORT void
fatal(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
#ifdef	USE_VPRINTF
	(void) vfprintf(stderr, fmt, args);
#else
	(void) js_fprintf(stderr, "%r", fmt, args);
#endif
	va_end(args);
	my_exit(EXIT_FAIL);
}


/* Get a response from the user, somehow or other. */

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
ask(const char *fmt, ...)
#else
EXPORT void
ask(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	ttyfd;
	int	r;
	bool	tty2 = isatty(STDERR_FILENO);

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
#ifdef	USE_VPRINTF
	(void) vsprintf(buf, fmt, args);
#else
	(void) js_sprintf(buf, "%r", fmt, args);
#endif
	va_end(args);
	Fflush(stderr);
	write(STDERR_FILENO, buf, strlen(buf));
	if (tty2) {			/* might be redirected to a file */
		r = read(STDERR_FILENO, buf, bufsize);
	} else if (isatty(STDOUT_FILENO)) {
					/* this may be new file output */
		Fflush(stdout);
		write(STDOUT_FILENO, buf, strlen(buf));
		r = read(STDOUT_FILENO, buf, bufsize);
	} else if ((ttyfd = open("/dev/tty", 2)) >= 0 && isatty(ttyfd)) {
					/* might be deleted or unwriteable */
		write(ttyfd, buf, strlen(buf));
		r = read(ttyfd, buf, bufsize);
		Close(ttyfd);
	} else if (isatty(STDIN_FILENO)) {
					/* this is probably patch input */
		Fflush(stdin);
		write(STDIN_FILENO, buf, strlen(buf));
		r = read(STDIN_FILENO, buf, bufsize);
	} else {			/* no terminal at all--default it */
		buf[0] = '\n';
		r = 1;
	}
	if (r <= 0)
		buf[0] = 0;
	else
		buf[r] = '\0';
	if (!tty2)
		say("%s", buf);
}

/* How to handle certain events when not in a critical region. */

void
set_signals(reset)
	int	reset;
{
	static RETSIGTYPE (*hupval) __PR((int)), (*intval) __PR((int));

	if (!reset) {
#ifdef	SIGHUP
		hupval = signal(SIGHUP, SIG_IGN);
		if (hupval != SIG_IGN)
			hupval = (RETSIGTYPE(*) __PR((int)))sig_exit;
#endif
		intval = signal(SIGINT, SIG_IGN);
		if (intval != SIG_IGN)
			intval = (RETSIGTYPE(*) __PR((int)))sig_exit;
	}
#ifdef	SIGHUP
	Signal(SIGHUP, hupval);
#endif
	Signal(SIGINT, intval);
}

static void
sig_exit(signo)
	int	signo;
{
	my_exit(EXIT_SIGNAL);
}

/* How to handle certain events when in a critical region. */

void
ignore_signals()
{
#ifdef	SIGHUP
	Signal(SIGHUP, SIG_IGN);
#endif
	Signal(SIGINT, SIG_IGN);
}

/* Make sure we'll have the directories to create a file. */

#ifndef	_SCHILY_SCHILY_H
#ifdef	PROTOTYPES
int
makedirs(char *filename, mode_t mode, bool striplast)
#else
int
makedirs(filename, mode, striplast)
	char *filename;
	mode_t mode;
	bool striplast;
#endif
{
	char tmpbuf[256];
	char *s = tmpbuf;
	char *dirv[20];
	int i;
	int dirvp = 0;

	while (*filename) {
		if (*filename == '/') {
			filename++;
			dirv[dirvp++] = s;
			*s++ = '\0';
		} else {
			*s++ = *filename++;
		}
	}
	*s = '\0';
	dirv[dirvp] = s;
	if (striplast)
		dirvp--;
	if (dirvp < 0)
		return (0);
#if 1
	for (i = 0; i <= dirvp; i++) {
		mkdir(tmpbuf, mode);
#if 0
			S_IRUSR|S_IWUSR|S_IXUSR
			|S_IRGRP|S_IWGRP|S_IXGRP
			|S_IROTH|S_IWOTH|S_IXOTH);
#endif
		*dirv[i] = '/';
	}
#else
	strcpy(buf, "mkdir");
	s = buf;
	for (i = 0; i <= dirvp; i++) {
		while (*s) s++;
		*s++ = ' ';
		strcpy(s, tmpbuf);
		*dirv[i] = '/';
	}
	system(buf);
#endif
	return (0);
}
#endif	/* _SCHILY_SCHILY_H */

/* Make filenames more reasonable. */

char *
fetchname(at, strip_leading, assume_exists, isnulldate)
	char *at;
	int strip_leading;
	int assume_exists;
	bool *isnulldate;
{
	char *s;
	char *name;
	char *t;
	char tmpbuf[200];

	if (isnulldate)
		*isnulldate = FALSE;

	if (!at)
		return (Nullch);
	s = savestr(at);
	for (t = s; isspace(*t); t++) {
		;
		/* LINTED */
	}
	name = t;
#ifdef DEBUGGING
	if (debug & 128)
		say(_("fetchname %s %d %d\n"),
			name, strip_leading, assume_exists);
#endif
	for (; *t && !isspace(*t); t++)
		if (*t == '/')
			if (--strip_leading >= 0)
				name = t+1;
	*t = '\0';
	if (strEQ(name, "/dev/null")) {	/* files can be created by diffing */
		if (isnulldate)		/* against /dev/null. */
			*isnulldate = TRUE;
		return (Nullch);
	}
	if (isnulldate)
		*isnulldate = fetchtime(t) == 0;
	if (name != s && *s != '/') {
		name[-1] = '\0';
		if (stat(s, &file_stat) && file_stat.st_mode & S_IFDIR) {
			name[-1] = '/';
			name = s;
		}
	}
	name = savestr(name);
	Sprintf(tmpbuf, "RCS/%s", name);
	free(s);
	if (stat(name, &file_stat) < 0 && !assume_exists) {
		Strcat(tmpbuf, RCSSUFFIX);
		if (stat(tmpbuf, &file_stat) < 0 &&
		    stat(tmpbuf+4, &file_stat) < 0) {
			Sprintf(tmpbuf, "SCCS/%s%s", SCCSPREFIX, name);
			if (stat(tmpbuf, &file_stat) < 0 &&
			    stat(tmpbuf+5, &file_stat) < 0) {
				free(name);
				name = Nullch;
			}
		}
	}
	return (name);
}

static Llong
fetchtime(t)
	char	*t;
{
	Llong	d = -1;
	struct tm tm;
	int	n;
	int	h;
	int	m;

	for (++t; *t != '\0' && isspace(*t); t++)
		;
	n = sscanf(t, "%4d-%2d-%2d %2d:%2d:%2d",
			&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
			&tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	tm.tm_year -= 1900;
	tm.tm_mon -= 1;

	if (n != 6)
		return (d);

	if (strlen(t) <= 19)
		return (d);

	t += 19;
	for (; *t != '\0' && !isspace(*t); t++)
		;
	for (; *t != '\0' && isspace(*t); t++)
		;
	n = sscanf(t, "%3d%2d", &h, &m);
	if (n != 2)
		return (d);
	if (h < -24 || h > 25)
		return (d);
	if (m < 0 || m > 59)
		return (d);
	d = mklgmtime(&tm);
	m += 60 * h;
	m *= 60;
	d -= m;
	return (d);
}
