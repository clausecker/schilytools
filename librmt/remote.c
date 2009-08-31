/*#define	USE_REMOTE*/
/*#define	USE_RCMD_RSH*/
/*#define	NO_LIBSCHILY*/
/* @(#)remote.c	1.72 09/08/24 Copyright 1990-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)remote.c	1.72 09/08/24 Copyright 1990-2009 J. Schilling";
#endif
/*
 *	Remote tape client interface code
 *
 *	Copyright (c) 1990-2009 J. Schilling
 *
 *	TOTO:
 *		Signal handler for SIGPIPE
 *		check rmtaborted for exit() / clean abort of connection
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

/*#undef	USE_REMOTE*/
/*#undef	USE_RCMD_RSH*/

#if !defined(HAVE_FORK) || !defined(HAVE_SOCKETPAIR) || !defined(HAVE_DUP2)
#undef	USE_RCMD_RSH
#endif
/*
 * We may work without getservbyname() if we restructure the code not to
 * use the port number if we only use _rcmdrsh().
 */
#if !defined(HAVE_GETSERVBYNAME)
#undef	USE_REMOTE				/* Cannot get rcmd() port # */
#endif
#if (!defined(HAVE_NETDB_H) || !defined(HAVE_RCMD)) && !defined(USE_RCMD_RSH)
#undef	USE_REMOTE				/* There is no rcmd() */
#endif

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/ioctl.h>
#include <schily/socket.h>
#include <schily/errno.h>
#include <schily/signal.h>
#include <schily/netdb.h>
#include <schily/pwd.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/mtio.h>
#include <schily/librmt.h>
#include <schily/schily.h>
#include <schily/ctype.h>
#include <schily/priv.h>

#if	defined(SIGDEFER) || defined(SVR4)
#define	signal	sigset
#endif

/*
 * On Cygwin, there are no privilleged ports.
 * On UNIX, rcmd() uses privilleged port that only work for root.
 */
#if	defined(IS_CYGWIN) || defined(__MINGW32__)
#define	privport_ok()	(1)
#else
#ifdef	HAVE_SOLARIS_PPRIV
#define	privport_ok()	ppriv_ok()
#else
#define	privport_ok()	(geteuid() == 0)
#endif
#endif


#ifdef	NO_LIBSCHILY

/* >>>>>>>>>> Begin compatibility code for use without libschily <<<<<<<<<<< */
/*
 * This code is needed for applications that like to use librmt but don't
 * link against libschily.
 *
 * A working vfprintf(), snprintf(), strerror(), memmove() and atoll() is
 * needed.
 * For this reason, the portability in this compilation mode is limited.
 */
#include <schily/varargs.h>
#define	geterrno()		(errno)
#define	seterrno(err)		(errno = (err))
#define	js_snprintf		snprintf
#define	_niread			_lniread
#define	_niwrite		_lniwrite
#define	movebytes(f, t, n)	memmove((t), (f), (n))
#define	astoll(s, np)		{*np = atoll(s); }
#define	comerrno(e, s)		{_errmsgno(e, s); rmt_exit(e); }
#define	errmsgno		_errmsgno
#define	errmsgstr		strerror

LOCAL	int _niread	__PR((int f, void *buf, int count));
LOCAL	int _niwrite	__PR((int f, void *buf, int count));
LOCAL	int errmsgno	__PR((int, const char *, ...));

LOCAL int
_niread(f, buf, count)
	int	f;
	void	*buf;
	int	count;
{
	int ret;

	while ((ret = read(f, buf, count)) < 0 && geterrno() == EINTR)
		;
	return (ret);
}

LOCAL int
_niwrite(f, buf, count)
	int	f;
	void	*buf;
	int	count;
{
	int ret;

	while ((ret = write(f, buf, count)) < 0 && geterrno() == EINTR)
		;
	return (ret);
}

	/*
	 * On UNIX errno is a small non-negative number, so we assume that
	 * negative values cannot be a valid errno and don't print the error
	 * string in this case. Note that this macro does not work on BeOS.
	 */
#define	silent_error(e)		((e) < 0)

/* VARARGS2 */
LOCAL int
#ifdef	PROTOTYPES
errmsgno(int err, const char *msg, ...)
#else
errmsgno(err, msg, va_alist)
	int	err;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;
	char	errbuf[20];
	char	*errnam;

	ret = fprintf(stderr, "librmt: ");
	if (ret < 0)
		return (ret);

	if (!silent_error(err)) {
		errnam = errmsgstr(err);
		if (errnam == NULL) {
			(void) js_snprintf(errbuf, sizeof (errbuf),
						"Error %d", err);
			errnam = errbuf;
		}
		ret = fprintf(stderr, "%s. ", errnam);
		if (ret < 0)
			return (ret);
	}

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret = vfprintf(stderr, msg, args);
	va_end(args);
	return (ret);
}

/* >>>>>>>>>>> End compatibility code for use without libschily <<<<<<<<<<<< */

#endif	/* NO_LIBSCHILY */

#define	CMD_SIZE	80

LOCAL	BOOL	rmt_debug;
LOCAL	int	(*rmt_errmsgno)		__PR((int, const char *, ...))	= errmsgno;
LOCAL	void	(*rmt_exit)		__PR((int))			= exit;

EXPORT	void	rmtinit			__PR((int (*errmsgn)(int, const char *, ...),
						void (*eexit)(int)));
EXPORT	int	rmtdebug		__PR((int dlevel));
EXPORT	char	*rmtfilename		__PR((char *name));
EXPORT	char	*rmthostname		__PR((char *hostname, int hnsize, char *rmtspec));
EXPORT	int	rmtgetconn		__PR((char *host, int trsize, int excode));

#ifdef	USE_REMOTE
LOCAL	void	rmtabrt			__PR((int sig));
LOCAL	BOOL	okuser			__PR((char *name));
LOCAL	void	rmtoflags		__PR((int fmode, char *cmode));
EXPORT	int	rmtopen			__PR((int fd, char *fname, int fmode));
EXPORT	int	rmtclose		__PR((int fd));
EXPORT	int	rmtread			__PR((int fd, char *buf, int count));
EXPORT	int	rmtwrite		__PR((int fd, char *buf, int count));
EXPORT	off_t	rmtseek			__PR((int fd, off_t offset, int whence));
EXPORT	int	rmtioctl		__PR((int fd, int cmd, int count));
LOCAL	int	rmtmapold		__PR((int cmd));
LOCAL	int	rmtmapnew		__PR((int cmd));
LOCAL	Llong	rmtxgstatus		__PR((int fd, int cmd));
LOCAL	int	rmt_v1_status		__PR((int fd, struct rmtget *mtp));
LOCAL	int	rmt_v0_status		__PR((int fd, struct mtget *mtp));
EXPORT	int	rmxtstatus		__PR((int fd, struct rmtget *mtp));
EXPORT	int	rmtstatus		__PR((int fd, struct mtget *mtp));
LOCAL	Llong	rmtcmd			__PR((int fd, char *name, char *cbuf));
LOCAL	void	rmtsendcmd		__PR((int fd, char *name, char *cbuf));
LOCAL	int	rmtfillrdbuf		__PR((int fd));
LOCAL	int	rmtreadchar		__PR((int fd, char *cp));
LOCAL	int	rmtreadbuf		__PR((int fd, char *buf, int count));
LOCAL	int	rmtgetline		__PR((int fd, char *line, int count));
LOCAL	Llong	rmtgetstatus		__PR((int fd, char *name));
LOCAL	int	rmtaborted		__PR((int fd));
EXPORT	void	_rmtg2mtg		__PR((struct mtget *mtp, struct rmtget *rmtp));
EXPORT	int	_mtg2rmtg		__PR((struct rmtget *rmtp, struct mtget *mtp));
#ifdef	USE_RCMD_RSH
LOCAL	int	_rcmdrsh		__PR((char **ahost, int inport,
						const char *locuser,
						const char *remuser,
						const char *cmd,
						const char *rsh));
#ifdef	HAVE_SOLARIS_PPRIV
LOCAL	BOOL	ppriv_ok		__PR((void));
#endif
#endif

#endif

EXPORT void
rmtinit(errmsgn, eexit)
	int	(*errmsgn) __PR((int, const char *, ...));
	void	(*eexit)   __PR((int));
{
	rmt_errmsgno = errmsgn;
	if (rmt_errmsgno == (int (*)  __PR((int, const char *, ...)))0)
		rmt_errmsgno = errmsgno;

	rmt_exit = eexit;
	if (rmt_exit == (void (*) __PR((int)))0)
		rmt_exit = exit;
}

EXPORT int
rmtdebug(dlevel)
	int	dlevel;
{
	int	odebug = rmt_debug;

	rmt_debug = dlevel;
	return (odebug);
}

EXPORT char *
rmtfilename(name)
	char	*name;
{
	char	*ret;

	if (name[0] == '/')
		return (NULL);		/* Absolut pathname cannot be remote */
	if (name[0] == '.') {
		if (name[1] == '/' || (name[1] == '.' && name[2] == '/'))
			return (NULL);	/* Relative pathname cannot be remote*/
	}
	if ((ret = strchr(name, ':')) != NULL) {
		if (name[0] == ':') {
			/*
			 * This cannot be a remote filename as the host part
			 * has zero length.
			 */
			return (NULL);
		}
		ret++;	/* Skip the colon. */
	}
	return (ret);
}

EXPORT char *
rmthostname(hostname, hnsize, rmtspec)
		char	*hostname;
	register int	hnsize;
		char	*rmtspec;
{
	register int	i;
	register char	*hp;
	register char	*fp;
	register char	*remfn;

	if ((remfn = rmtfilename(rmtspec)) == NULL) {
		hostname[0] = '\0';
		return (NULL);
	}
	remfn--;
	for (fp = rmtspec, hp = hostname, i = 1;
			fp < remfn && i < hnsize; i++) {
		*hp++ = *fp++;
	}
	*hp = '\0';
	return (hostname);
}

#ifdef	USE_REMOTE

EXPORT int
rmtgetconn(host, trsize, excode)
	char	*host;		/* The host name to connect to		    */
	int	trsize;		/* Max transfer size for SO_SNDBUF/SO_RCVBUF */
	int	excode;		/* If != 0 use value to exit() in this func  */
{
	static	struct servent	*sp = 0;
	static	struct passwd	*pw = 0;
		char		*name = "root";
		char		*p;
		char		*rmt;
		char		*rsh;
		int		rmtsock;
		char		*rmtpeer;
		char		rmtuser[128];


	signal(SIGPIPE, rmtabrt);
	if (sp == 0) {
		sp = getservbyname("shell", "tcp");
		if (sp == 0) {
			rmt_errmsgno(EX_BAD, "shell/tcp: unknown service\n");
			if (excode)
				rmt_exit(excode);
			rmt_exit(EX_BAD);
			return (-2);		/* exit function did not exit*/
		}
		pw = getpwuid(getuid());
		if (pw == 0) {
			rmt_errmsgno(EX_BAD, "who are you? No passwd entry found.\n");
			if (excode)
				rmt_exit(excode);
			rmt_exit(EX_BAD);
			return (-2);		/* exit function did not exit*/
		}
	}
	if ((p = strchr(host, '@')) != NULL) {
		size_t d = p - host;

		if (d > sizeof (rmtuser))
			d = sizeof (rmtuser);
		js_snprintf(rmtuser, sizeof (rmtuser), "%.*s",
							(int)d, host);
		if (! okuser(rmtuser)) {
			if (excode)
				rmt_exit(excode);
			rmt_exit(EX_BAD);
			return (-2);		/* exit function did not exit*/
		}
		name = rmtuser;
		host = &p[1];
	} else {
		name = pw->pw_name;
	}
	if (rmt_debug)
		rmt_errmsgno(EX_BAD, "locuser: '%s' rmtuser: '%s' host: '%s'\n",
						pw->pw_name, name, host);
	rmtpeer = host;

	if ((rmt = getenv("RMT")) == NULL)
		rmt = "/etc/rmt";
	rsh = getenv("RSH");

#ifdef	USE_RCMD_RSH
	if (!privport_ok() || rsh != NULL)
		rmtsock = _rcmdrsh(&rmtpeer, (unsigned short)sp->s_port,
					pw->pw_name, name, rmt, rsh);
	else
#endif
#ifdef	HAVE_RCMD
		rmtsock = rcmd(&rmtpeer, (unsigned short)sp->s_port,
					pw->pw_name, name, rmt, 0);
#else
		rmtsock = _rcmdrsh(&rmtpeer, (unsigned short)sp->s_port,
					pw->pw_name, name, rmt, rsh);
#endif

	if (rmtsock < 0)
		return (-1);


#ifdef	SO_SNDBUF
	while (trsize > 512 &&
		setsockopt(rmtsock, SOL_SOCKET, SO_SNDBUF,
				(char *)&trsize, sizeof (trsize)) < 0) {
		trsize -= 512;
	}
	if (rmt_debug)
		rmt_errmsgno(EX_BAD, "sndsize: %d\n", trsize);
#endif
#ifdef	SO_RCVBUF
	while (trsize > 512 &&
		setsockopt(rmtsock, SOL_SOCKET, SO_RCVBUF,
				(char *)&trsize, sizeof (trsize)) < 0) {
		trsize -= 512;
	}
	if (rmt_debug)
		rmt_errmsgno(EX_BAD, "rcvsize: %d\n", trsize);
#endif

	return (rmtsock);
}

LOCAL void
rmtabrt(sig)
	int	sig;
{
	rmtaborted(-1);
}

/*
 * XXX Is such a function really needed?
 * XXX A similar function appeared on FreeBSD with a
 * XXX misterious change to dump(8)
 * XXX N.B. The FreeBSD function excludes '_' in addition.
 */
LOCAL BOOL
okuser(name)
	char	*name;
{
	register char	*p;
	register Uchar	c;

	for (p = name; *p; ) {
		c = *p++;
		if (!isascii(c) || !(isalnum(c) || c == '-')) {
			rmt_errmsgno(EX_BAD, "invalid user name %s\n", name);
			return (FALSE);
		}
	}
	return (TRUE);
}

LOCAL void
rmtoflags(fmode, cmode)
	int	fmode;
	char	*cmode;
{
	register char	*p;
	register int	amt;
	register int	maxcnt = CMD_SIZE;

	switch (fmode & O_ACCMODE) {

	case O_RDONLY:	p = "O_RDONLY";	break;
	case O_RDWR:	p = "O_RDWR";	break;
	case O_WRONLY:	p = "O_WRONLY";	break;

	default:	p = "Cannot Happen";
	}
	amt = js_snprintf(cmode, maxcnt, "%s", p); if (amt < 0) return;
	p = cmode;
	p += amt;
	maxcnt -= amt;
#ifdef	O_TEXT
	if (fmode & O_TEXT) {
		amt = js_snprintf(p, maxcnt, "|O_TEXT"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_NDELAY
	if (fmode & O_NDELAY) {
		amt = js_snprintf(p, maxcnt, "|O_NDELAY"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_APPEND
	if (fmode & O_APPEND) {
		amt = js_snprintf(p, maxcnt, "|O_APPEND"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_SYNC
	if (fmode & O_SYNC) {
		amt = js_snprintf(p, maxcnt, "|O_SYNC"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_DSYNC
	if (fmode & O_DSYNC) {
		amt = js_snprintf(p, maxcnt, "|O_DSYNC"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_RSYNC
	if (fmode & O_RSYNC) {
		amt = js_snprintf(p, maxcnt, "|O_RSYNC"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_NONBLOCK
	if (fmode & O_NONBLOCK) {
		amt = js_snprintf(p, maxcnt, "|O_NONBLOCK"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_PRIV
	if (fmode & O_PRIV) {
		amt = js_snprintf(p, maxcnt, "|O_PRIV"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_LARGEFILE
	if (fmode & O_LARGEFILE) {
		amt = js_snprintf(p, maxcnt, "|O_LARGEFILE"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_CREAT
	if (fmode & O_CREAT) {
		amt = js_snprintf(p, maxcnt, "|O_CREAT"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_TRUNC
	if (fmode & O_TRUNC) {
		amt = js_snprintf(p, maxcnt, "|O_TRUNC"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_EXCL
	if (fmode & O_EXCL) {
		amt = js_snprintf(p, maxcnt, "|O_EXCL"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
#ifdef	O_NOCTTY
	if (fmode & O_NOCTTY) {
		amt = js_snprintf(p, maxcnt, "|O_NOCTTY"); if (amt < 0) return;
		p += amt;
		maxcnt -= amt;
	}
#endif
}

EXPORT int
rmtopen(fd, fname, fmode)
	int	fd;
	char	*fname;
	int	fmode;
{
	char	cbuf[4096+CMD_SIZE];
	char	cmode[CMD_SIZE];
	int	ret;

	/*
	 * Convert all fmode bits into the symbolic fmode.
	 * only send the lowest 2 bits in numeric mode as it would be too
	 * dangerous because the apropriate bits differ between different
	 * operating systems.
	 */
	rmtoflags(fmode, cmode);
	ret = js_snprintf(cbuf, sizeof (cbuf), "O%s\n%d %s\n",
				fname, fmode & O_ACCMODE, cmode);
	if (ret < 0 || ret >= sizeof (cbuf)) {
#ifdef	ENAMETOOLONG
		seterrno(ENAMETOOLONG);
#else
		seterrno(EINVAL);
#endif
		return (-1);
	}
	ret = rmtcmd(fd, "open", cbuf);
	if (ret < 0)
		return (ret);

	/*
	 * Tell the rmt server that we are aware of Version 1 commands.
	 */
	(void) rmtioctl(fd, RMTIVERSION, 0);

	return (ret);
}

EXPORT int
rmtclose(fd)
	int	fd;
{
	return (rmtcmd(fd, "close", "C\n"));
}

EXPORT int
rmtread(fd, buf, count)
	int	fd;
	char	*buf;
	int	count;
{
	char	cbuf[CMD_SIZE];
	int	n;
	int	amt = 0;

	n = js_snprintf(cbuf, CMD_SIZE, "R%d\n", count);
	if (n < 0 || n >= CMD_SIZE) {				/* Paranoia */
		seterrno(EINVAL);
		return (-1);
	}
	n = rmtcmd(fd, "read", cbuf);
	if (n < 0)
		return (-1);

	/*
	 * Nice idea from disassembling Solaris ufsdump...
	 */
	if (n > count) {
		rmt_errmsgno(EX_BAD,
			"rmtread: expected response size %d, got %d\n",
			count, n);
		rmt_errmsgno(EX_BAD,
			"This means the remote rmt daemon is not compatible.\n");
		return (rmtaborted(fd));
		/*
		 * XXX Should we better abort (exit) here?
		 */
	}
	amt = rmtreadbuf(fd, buf, n);

	return (amt);
}

EXPORT int
rmtwrite(fd, buf, count)
	int	fd;
	char	*buf;
	int	count;
{
	char	cbuf[CMD_SIZE];
	int	n;

	n = js_snprintf(cbuf, CMD_SIZE, "W%d\n", count);
	if (n < 0 || n >= CMD_SIZE) {				/* Paranoia */
		seterrno(EINVAL);
		return (-1);
	}
	rmtsendcmd(fd, "write", cbuf);
	if (_niwrite(fd, buf, count) != count)
		rmtaborted(fd);
	return (rmtgetstatus(fd, "write"));
}

EXPORT off_t
rmtseek(fd, offset, whence)
	int	fd;
	off_t	offset;
	int	whence;
{
	char	cbuf[CMD_SIZE];
	int	n;

	switch (whence) {

	case SEEK_SET: whence = 0; break;
	case SEEK_CUR: whence = 1; break;
	case SEEK_END: whence = 2; break;
#ifdef	SEEK_DATA
	case SEEK_DATA: whence = 3; break;
#endif
#ifdef	SEEK_HOLE
	case SEEK_HOLE: whence = 4; break;
#endif

	default:
		seterrno(EINVAL);
		return (-1);
	}

	n = js_snprintf(cbuf, CMD_SIZE, "L%lld\n%d\n", (Llong)offset, whence);
	if (n < 0 || n >= CMD_SIZE) {				/* Paranoia */
		seterrno(EINVAL);
		return (-1);
	}
	return ((off_t)rmtcmd(fd, "seek", cbuf));
}

EXPORT int
rmtioctl(fd, cmd, count)
	int	fd;
	int	cmd;
	int	count;
{
	char	cbuf[CMD_SIZE];
	char	c = 'I';
	int	rmtversion = RMT_NOVERSION;
	int	i;

	if (cmd != RMTIVERSION)
		rmtversion = rmtioctl(fd, RMTIVERSION, 0);

	if (cmd >= 0 && (rmtversion == RMT_VERSION)) {
		/*
		 * Opcodes 0..7 are unique across different architectures.
		 * But as in many cases Linux does not even follow this rule.
		 * If we know that we are calling a VERSION 1 client, we may
		 * safely assume that the client is not using Linux mapping
		 * but the standard mapping.
		 */
		i = rmtmapold(cmd);
		if (cmd <= 7 && i  < 0) {
			/*
			 * We cannot map the current command but it's value is
			 * within the range 0..7. Do not send it over the wire.
			 */
			seterrno(EINVAL);
			return (-1);
		}
		if (i >= 0)
			cmd = i;
	}
	if (cmd > 7 && (rmtversion == RMT_VERSION)) {
		i = rmtmapnew(cmd);
		if (i >= 0) {
			cmd = i;
			c = 'i';
		}
	}

	i = js_snprintf(cbuf, CMD_SIZE, "%c%d\n%d\n", c, cmd, count);
	if (i < 0 || i >= CMD_SIZE) {				/* Paranoia */
		seterrno(EINVAL);
		return (-1);
	}
	return (rmtcmd(fd, "ioctl", cbuf));
}

/*
 * Map all old opcodes that should be in range 0..7 to numbers /etc/rmt expects
 * This is needed because Linux does not follow the UNIX conventions.
 */
LOCAL int
rmtmapold(cmd)
	int	cmd;
{
	switch (cmd) {

#ifdef	MTWEOF
	case  MTWEOF:	return (0);
#endif

#ifdef	MTFSF
	case MTFSF:	return (1);
#endif

#ifdef	MTBSF
	case MTBSF:	return (2);
#endif

#ifdef	MTFSR
	case MTFSR:	return (3);
#endif

#ifdef	MTBSR
	case MTBSR:	return (4);
#endif

#ifdef	MTREW
	case MTREW:	return (5);
#endif

#ifdef	MTOFFL
	case MTOFFL:	return (6);
#endif

#ifdef	MTNOP
	case MTNOP:	return (7);
#endif
	}
	return (-1);
}

/*
 * Map all new opcodes that should be in range above 7 to the
 * values expected by the 'i' command of /etc/rmt.
 */
LOCAL int
rmtmapnew(cmd)
	int	cmd;
{
	switch (cmd) {

#ifdef	MTCACHE
	case MTCACHE:	return (RMTICACHE);
#endif

#ifdef	MTNOCACHE
	case MTNOCACHE:	return (RMTINOCACHE);
#endif

#ifdef	MTRETEN
	case MTRETEN:	return (RMTIRETEN);
#endif

#ifdef	MTERASE
	case MTERASE:	return (RMTIERASE);
#endif

#ifdef	MTEOM
	case MTEOM:	return (RMTIEOM);
#endif

#ifdef	MTNBSF
	case MTNBSF:	return (RMTINBSF);
#endif
	}
	return (-1);
}

/*
 * Get one (single) member of struct mtget from remote
 */
LOCAL Llong
rmtxgstatus(fd, cmd)
	int	fd;
	char	cmd;
{
	char	cbuf[CMD_SIZE];
	int	n;

			/* No newline */
	n = js_snprintf(cbuf, CMD_SIZE, "s%c", cmd);
	if (n < 0 || n >= CMD_SIZE) {				/* Paranoia */
		seterrno(EINVAL);
		return (-1);
	}
	seterrno(0);
	return (rmtcmd(fd, "extended status", cbuf));
}

LOCAL int
rmt_v1_status(fd, mtp)
	int		fd;
	struct  rmtget	*mtp;
{
	mtp->mt_xflags	= 0;

	mtp->mt_erreg	= rmtxgstatus(fd, MTS_ERREG); /* must be first */
	if (geterrno() == 0)
		mtp->mt_xflags |= RMT_ERREG;

	mtp->mt_type	= rmtxgstatus(fd, MTS_TYPE);
	if (geterrno() == 0)
		mtp->mt_xflags |= RMT_TYPE;

	mtp->mt_dsreg	= rmtxgstatus(fd, MTS_DSREG);
	if (geterrno() == 0)
		mtp->mt_xflags |= RMT_DSREG;

	mtp->mt_resid	= rmtxgstatus(fd, MTS_RESID);
	if (geterrno() == 0)
		mtp->mt_xflags |= RMT_RESID;

	mtp->mt_fileno	= rmtxgstatus(fd, MTS_FILENO);
	if (geterrno() == 0)
		mtp->mt_xflags |= RMT_FILENO;

	mtp->mt_blkno	= rmtxgstatus(fd, MTS_BLKNO);
	if (geterrno() == 0)
		mtp->mt_xflags |= RMT_BLKNO;

	mtp->mt_flags	= rmtxgstatus(fd, MTS_FLAGS);
	if (geterrno() == 0)
		mtp->mt_xflags |= RMT_FLAGS;

	mtp->mt_bf	= rmtxgstatus(fd, MTS_BF);
	if (geterrno() == 0)
		mtp->mt_xflags |= RMT_BF;

	if (mtp->mt_xflags == 0)
		return (-1);

	return (0);
}

LOCAL int
rmt_v0_status(fd, mtp)
	int		fd;
	struct  mtget	*mtp;
{
	register int i;
	register char *cp;
	char	c;
	int	n;

				/* No newline */
	if ((n = rmtcmd(fd, "status", "S")) < 0)
		return (-1);

	/*
	 * From disassembling Solaris ufsdump, they seem to check
	 * only if (n > sizeof (mts)).
	 */
	if (n != sizeof (struct mtget)) {
		rmt_errmsgno(EX_BAD,
			"rmtstatus: expected response size %d, got %d\n",
			(int)sizeof (struct mtget), n);
		rmt_errmsgno(EX_BAD,
			"This means the remote rmt daemon is not compatible.\n");
		/*
		 * XXX should we better abort here?
		 */
	}

	for (i = 0, cp = (char *)mtp; i < sizeof (struct mtget); i++)
		*cp++ = 0;
	for (i = 0, cp = (char *)mtp; i < n; i++) {
		/*
		 * Make sure to read all bytes because we otherwise
		 * would confuse the protocol. Do not copy more
		 * than the size of our local struct mtget.
		 */
		if (rmtreadchar(fd, &c) != 1)
			return (rmtaborted(fd));

		if (i < sizeof (struct mtget))
			*cp++ = c;
	}
	/*
	 * The GNU remote tape lib tries to swap the structure based on the
	 * value of mt_type. While this makes sense for UNIX, it will not
	 * work if one system is running Linux. The Linux mtget structure
	 * is completely incompatible (mt_type is long instead of short).
	 */
	return (n);
}

EXPORT int
rmtxstatus(fd, mtp)
	int		fd;
	struct  rmtget	*mtp;
{
	struct  mtget	mtget;

	if (rmtioctl(fd, RMTIVERSION, 0) == RMT_VERSION)
		return (rmt_v1_status(fd, mtp));

	if (rmt_v0_status(fd, &mtget) < 0)
		return (-1);

	if (_mtg2rmtg(mtp, &mtget) < 0)
		return (-1);
	return (0);
}

EXPORT int
rmtstatus(fd, mtp)
	int		fd;
	struct  mtget	*mtp;
{
	struct  rmtget	rmtget;
	int	ret = -1;

	if (rmtioctl(fd, RMTIVERSION, 0) == RMT_VERSION) {
		ret = rmt_v1_status(fd, &rmtget);
		if (ret < 0)
			return (ret);
	} else {
		if (rmt_debug)
			rmt_errmsgno(EX_BAD, "Retrieving mt status from old server.\n");
		return (rmt_v0_status(fd, mtp));
	}

	_rmtg2mtg(mtp, &rmtget);
	return (ret);
}

LOCAL Llong
rmtcmd(fd, name, cbuf)
	int	fd;
	char	*name;
	char	*cbuf;
{
	rmtsendcmd(fd, name, cbuf);
	return (rmtgetstatus(fd, name));
}

LOCAL void
rmtsendcmd(fd, name, cbuf)
	int	fd;
	char	*name;
	char	*cbuf;
{
	int	buflen = strlen(cbuf);

	seterrno(0);
	if (_niwrite(fd, cbuf, buflen) != buflen)
		rmtaborted(fd);
}

#define	READB_SIZE	128
LOCAL	char		readb[READB_SIZE];
LOCAL	char		*readbptr;
LOCAL	int		readbcnt;

LOCAL int
rmtfillrdbuf(fd)
	int	fd;
{
	readbptr = readb;

	return (readbcnt = _niread(fd, readb, READB_SIZE));
}

LOCAL int
rmtreadchar(fd, cp)
	int	fd;
	char	*cp;
{
	if (--readbcnt < 0) {
		if (rmtfillrdbuf(fd) <= 0)
			return (readbcnt);
		--readbcnt;
	}
	*cp = *readbptr++;
	return (1);
}

LOCAL int
rmtreadbuf(fd, buf, count)
	register int	fd;
	register char	*buf;
	register int	count;
{
	register int	amt = 0;
	register int	cnt;

	if (readbcnt > 0) {
		cnt = readbcnt;
		if (cnt > count)
			cnt = count;
		movebytes(readbptr, buf, cnt);
		readbptr += cnt;
		readbcnt -= cnt;
		amt += cnt;
	}
	while (amt < count) {
		if ((cnt = _niread(fd, &buf[amt], count - amt)) <= 0) {
			return (rmtaborted(fd));
		}
		amt += cnt;
	}
	return (amt);
}

LOCAL int
rmtgetline(fd, line, count)
	int	fd;
	char	*line;
	int	count;
{
	register char	*cp;

	for (cp = line; cp < &line[count]; cp++) {
		if (rmtreadchar(fd, cp) != 1)
			return (rmtaborted(fd));

		if (*cp == '\n') {
			*cp = '\0';
			return (cp - line);
		}
	}
	if (rmt_debug)
		rmt_errmsgno(EX_BAD, "Protocol error (in rmtgetline).\n");
	return (rmtaborted(fd));
}

LOCAL Llong
rmtgetstatus(fd, name)
	int	fd;
	char	*name;
{
	char	cbuf[CMD_SIZE];
	char	code;
	Llong	number;

	rmtgetline(fd, cbuf, sizeof (cbuf));
	code = cbuf[0];
	astoll(&cbuf[1], &number);

	if (code == 'E' || code == 'F') {
		rmtgetline(fd, cbuf, sizeof (cbuf));
		if (code == 'F')	/* should close file ??? */
			rmtaborted(fd);
		if (rmt_debug)
			rmt_errmsgno(number, "Remote status(%s): %lld '%s'.\n",
							name, number, cbuf);
		seterrno(number);
		return ((Llong)-1);
	}
	if (code != 'A') {
		/* XXX Hier kommt evt Command not found ... */
		if (rmt_debug) {
			rmt_errmsgno(EX_BAD, "Protocol error (got %s).\n",
									cbuf);
		}
		return (rmtaborted(fd));
	}
	return (number);
}

LOCAL int
rmtaborted(fd)
	int	fd;
{
	if (rmt_debug)
		rmt_errmsgno(EX_BAD, "Lost connection to remote host ??\n");
	/* if fd >= 0 */
	/* close file */
	if (geterrno() == 0) {
		/*
		 * BSD used EIO but EPIPE is better for something like
		 * sdd -noerror
		 */
		seterrno(EPIPE);
	}
	/*
	 * BSD uses exit(X_ABORT) == 3, we return(-1) and let the caller decide
	 */
	return (-1);
}

#else	/* USE_REMOTE */

/* ARGSUSED */
EXPORT int
rmtgetconn(host, trsize, excode)
	char	*host;		/* The host name to connect to		    */
	int	trsize;		/* Max transfer size for SO_SNDBUF/SO_RCVBUF */
	int	excode;		/* If != 0 use value to exit() in this func  */
{
	rmt_errmsgno(EX_BAD, "Remote tape support not present.\n");

#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}

/* ARGSUSED */
EXPORT int
rmtopen(fd, fname, fmode)
	int	fd;
	char	*fname;
	int	fmode;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}

/* ARGSUSED */
EXPORT int
rmtclose(fd)
	int	fd;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}

/* ARGSUSED */
EXPORT int
rmtread(fd, buf, count)
	int	fd;
	char	*buf;
	int	count;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}

/* ARGSUSED */
EXPORT int
rmtwrite(fd, buf, count)
	int	fd;
	char	*buf;
	int	count;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}

/* ARGSUSED */
EXPORT off_t
rmtseek(fd, offset, whence)
	int	fd;
	off_t	offset;
	int	whence;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}

/* ARGSUSED */
EXPORT int
rmtioctl(fd, cmd, count)
	int	fd;
	int	cmd;
	int	count;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}

/* ARGSUSED */
EXPORT int
rmtxstatus(fd, mtp)
	int		fd;
	struct  rmtget	*mtp;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}

/* ARGSUSED */
EXPORT int
rmtstatus(fd, mtp)
	int		fd;
	struct  mtget	*mtp;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}

#endif	/* USE_REMOTE */



EXPORT void
_rmtg2mtg(mtp, rmtp)
	struct  mtget	*mtp;
	struct  rmtget	*rmtp;
{
#ifdef	HAVE_MTGET_TYPE
	if (rmtp->mt_xflags & RMT_TYPE)
		mtp->mt_type   = rmtp->mt_type;
	else
		mtp->mt_type   = 0;
#endif
#ifdef	HAVE_MTGET_DSREG
	if (rmtp->mt_xflags & RMT_DSREG)
		mtp->mt_dsreg  = rmtp->mt_dsreg;
	else
		mtp->mt_dsreg  = 0;
#endif
#ifdef	HAVE_MTGET_ERREG
	if (rmtp->mt_xflags & RMT_ERREG)
		mtp->mt_erreg  = rmtp->mt_erreg;
	else
		mtp->mt_erreg  = 0;
#endif
#ifdef	HAVE_MTGET_RESID
	if (rmtp->mt_xflags & RMT_RESID)
		mtp->mt_resid  = rmtp->mt_resid;
	else
		mtp->mt_resid  = 0;
#endif
#ifdef	HAVE_MTGET_FILENO
	if (rmtp->mt_xflags & RMT_FILENO)
		mtp->mt_fileno	= rmtp->mt_fileno;
	else
		mtp->mt_fileno	= -1;
#endif
#ifdef	HAVE_MTGET_BLKNO
	if (rmtp->mt_xflags & RMT_BLKNO)
		mtp->mt_blkno  = rmtp->mt_blkno;
	else
		mtp->mt_blkno	= -1;
#endif
#ifdef	HAVE_MTGET_FLAGS
	if (rmtp->mt_xflags & RMT_FLAGS)
		mtp->mt_flags	= rmtp->mt_flags;
	else
		mtp->mt_flags	= 0;
#endif
#ifdef	HAVE_MTGET_BF
	if (rmtp->mt_xflags & RMT_BF)
		mtp->mt_bf	= rmtp->mt_bf;
	else
		mtp->mt_bf	= 0;
#endif
}

EXPORT int
_mtg2rmtg(rmtp, mtp)
	struct  rmtget	*rmtp;
	struct  mtget	*mtp;
{
	rmtp->mt_xflags	= 0;

#ifdef	HAVE_MTGET_TYPE
	rmtp->mt_xflags	|= RMT_TYPE;
	rmtp->mt_type	 = mtp->mt_type;
#else
	rmtp->mt_type	 = 0;
#endif
#ifdef	HAVE_MTGET_DSREG
	rmtp->mt_xflags	|= RMT_DSREG;
	rmtp->mt_dsreg	 = mtp->mt_dsreg;
#else
	rmtp->mt_dsreg	 = 0;
#endif
#ifdef	HAVE_MTGET_ERREG
	rmtp->mt_xflags	|= RMT_ERREG;
	rmtp->mt_erreg	 = mtp->mt_erreg;
#else
	rmtp->mt_erreg	 = 0;
#endif
#ifdef	HAVE_MTGET_RESID
	rmtp->mt_xflags	|= RMT_RESID;
	rmtp->mt_resid	 = mtp->mt_resid;
#else
	rmtp->mt_resid	 = 0;
#endif
#ifdef	HAVE_MTGET_FILENO
	rmtp->mt_xflags	|= RMT_FILENO;
	rmtp->mt_fileno	 = mtp->mt_fileno;
#else
	rmtp->mt_fileno	 = -1;
#endif
#ifdef	HAVE_MTGET_BLKNO
	rmtp->mt_xflags	|= RMT_BLKNO;
	rmtp->mt_blkno	 = mtp->mt_blkno;
#else
	rmtp->mt_blkno	 = -1;
#endif
#ifdef	HAVE_MTGET_FLAGS
	rmtp->mt_xflags	|= RMT_FLAGS;
	rmtp->mt_flags	 = mtp->mt_flags;
#else
	rmtp->mt_flags	 = 0;
#endif
#ifdef	HAVE_MTGET_BF
	rmtp->mt_xflags |= RMT_BF;
	rmtp->mt_bf	 = mtp->mt_bf;
#else
	rmtp->mt_bf	 = 0;
#endif
	if (rmtp->mt_xflags == 0)
		return (-1);

	rmtp->mt_xflags	|= RMT_COMPAT;
	return (0);
}

/*--------------------------------------------------------------------------*/
#ifdef	USE_REMOTE
#ifdef	USE_RCMD_RSH
/*
 * If we make a separate file for libschily, we would need these include files:
 *
 * socketpair():	sys/types.h + sys/socket.h
 * dup2():		schily/unistd.h (hat auch sys/types.h)
 * strrchr():		schily/string.h
 *
 * and make sure that we use sigset() instead of signal() if possible.
 */
#include <schily/wait.h>
LOCAL int
_rcmdrsh(ahost, inport, locuser, remuser, cmd, rsh)
	char		**ahost;
	int		inport;		/* port is ignored */
	const char	*locuser;
	const char	*remuser;
	const char	*cmd;
	const char	*rsh;
{
	struct passwd	*pw;
	int	pp[2];
	int	pid;

	if (rsh == 0)
		rsh = "rsh";

	/*
	 * Verify that 'locuser' is present on local host.
	 */
	if ((pw = getpwnam(locuser)) == NULL) {
		rmt_errmsgno(EX_BAD, "Unknown user: %s\n", locuser);
		return (-1);
	}
	/* XXX Check the existence for 'ahost' here? */

	/*
	 * rcmd(3) creates a single socket to be used for communication.
	 * We need a bi-directional pipe to implement the same interface.
	 * On newer OS that implement bi-directional we could use pipe(2)
	 * but it makes no sense unless we find an OS that implements a
	 * bi-directional pipe(2) but no socketpair().
	 */
	if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, pp) == -1) {
		rmt_errmsgno(geterrno(), "Cannot create socketpair.\n");
		return (-1);
	}

	pid = fork();
	if (pid < 0) {
		return (-1);
	} else if (pid == 0) {
		const char	*p;
		const char	*av0;
		int		xpid;

		(void) close(pp[0]);
		if (dup2(pp[1], 0) == -1 ||	/* Pipe becomes 'stdin'  */
		    dup2(0, 1) == -1) {		/* Pipe becomes 'stdout' */

			rmt_errmsgno(geterrno(), "dup2 failed.\n");
			_exit(EX_BAD);
			/* NOTREACHED */
		}
		(void) close(pp[1]);		/* We don't need this anymore*/

		/*
		 * Become 'locuser' to tell the rsh program the local user id.
		 */
		if (getuid() != pw->pw_uid &&
		    setuid(pw->pw_uid) == -1) {
			rmt_errmsgno(geterrno(), "setuid(%lld) failed.\n",
							(Llong)pw->pw_uid);
			_exit(EX_BAD);
			/* NOTREACHED */
		}
		if (getuid() != geteuid() &&
#ifdef	HAVE_SETREUID
		    setreuid(-1, pw->pw_uid) == -1) {
#else
#ifdef	HAVE_SETEUID
		    seteuid(pw->pw_uid) == -1) {
#else
		    setuid(pw->pw_uid) == -1) {
#endif
#endif
			errmsg("seteuid(%lld) failed.\n",
							(Llong)pw->pw_uid);
			_exit(EX_BAD);
			/* NOTREACHED */
		}

		/*
		 * Fork again to completely detach from parent
		 * and avoid the need to wait(2).
		 */
		if ((xpid = fork()) == -1) {
			rmt_errmsgno(geterrno(),
				"rcmdsh: fork to lose parent failed.\n");
			_exit(EX_BAD);
			/* NOTREACHED */
		}
		if (xpid > 0) {
			_exit(0);
			/* NOTREACHED */
		}

		/*
		 * Always use remote shell programm (even for localhost).
		 * The client command may call getpeername() for security
		 * reasons and this would fail on a simple pipe.
		 */


		/*
		 * By default, 'rsh' handles terminal created signals
		 * but this is not what we like.
		 * For this reason, we tell 'rsh' to ignore these signals.
		 * Ignoring these signals is important to allow 'star' / 'sdd'
		 * to e.g. implement SIGQUIT as signal to trigger intermediate
		 * status printing.
		 *
		 * For now (late 2002), we know that the following programs
		 * are broken and do not implement signal handling correctly:
		 *
		 *	rsh	on SunOS-5.0...SunOS-5.9
		 *	ssh	from ssh.com
		 *	ssh	from openssh.org
		 *
		 * Sun already did accept a bug report for 'rsh'. For the ssh
		 * commands we need to send out bug reports. Meanwhile it could
		 * help to call setsid() if we are running under X so the ssh
		 * X pop up for passwd reading will work.
		 */
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
#ifdef	SIGTSTP
		signal(SIGTSTP, SIG_IGN); /* We would not be able to continue*/
#endif

		av0 = rsh;
		if ((p = strrchr(rsh, '/')) != NULL)
			av0 = ++p;
		execlp(rsh, av0, *ahost, "-l", remuser, cmd, (char *)NULL);

		rmt_errmsgno(geterrno(), "execlp '%s' failed.\n", rsh);
		_exit(EX_BAD);
		/* NOTREACHED */
	} else {
		(void) close(pp[1]);
		/*
		 * Wait for the intermediate child.
		 * The real 'rsh' program is completely detached from us.
		 */
		wait(0);
		return (pp[0]);
	}
	return (-1);	/* keep gcc happy */
}

#ifdef	HAVE_SOLARIS_PPRIV

LOCAL BOOL
ppriv_ok()
{
	priv_set_t	*privset;
	BOOL		net_privaddr = FALSE;


	if ((privset = priv_allocset()) == NULL) {
		return (FALSE);
	}
	if (getppriv(PRIV_EFFECTIVE, privset) == -1) {
		priv_freeset(privset);
		return (FALSE);
	}
	if (priv_ismember(privset, PRIV_NET_PRIVADDR)) {
		net_privaddr = TRUE;
	}
	priv_freeset(privset);

	return (net_privaddr);
}
#endif	/* HAVE_SOLARIS_PPRIV */

#endif	/* USE_RCMD_RSH */
#endif	/* USE_REMOTE */
