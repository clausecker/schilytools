/* @(#)rmt.c	1.35 08/12/23 Copyright 1994,2000-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)rmt.c	1.35 08/12/23 Copyright 1994,2000-2008 J. Schilling";
#endif
/*
 *	Remote tape server
 *	Supports both the old BSD format and the new abstract Sun format
 *	which is called RMT V1 protocol.
 *
 *	A client that likes to use the enhanced features of the RMT V1 protocol
 *	needs to send a "I-1\n0\n" request directly after opening the remote
 *	file using the 'O' rmt command.
 *	If the client requests the new protocol, MTIOCTOP ioctl opcodes
 *	in the range 0..7 are mapped to the BSD values to prevent problems
 *	from Linux opcode incompatibility.
 *
 *	The open modes support an abstract string notation found in rmt.c from
 *	GNU. This makes it possible to use more than O_RDONLY|O_WRONLY|O_RDWR
 *	with open(). MTIOCTOP tape ops could be enhanced the same way, but it
 *	seems that the current interface supports all what we need over the
 *	wire.
 *
 *	Copyright (c) 1994,2000-2008 J. Schilling
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

/*#define	FORCE_DEBUG*/

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* includes sys/types.h */
#include <schily/fcntl.h>
#include <schily/stat.h>
#include <schily/string.h>
#ifdef	 HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef	 HAVE_SYS_PARAM_H
#include <sys/param.h>	/* BSD-4.2 & Linux need this for MAXHOSTNAMELEN */
#endif
#include <schily/ioctl.h>
#ifdef	HAVE_SYS_MTIO_H
#include <sys/mtio.h>
#endif
#include <schily/errno.h>
#include <pwd.h>

#include <schily/utypes.h>
#include <schily/standard.h>
#include <schily/deflts.h>
#include <schily/patmatch.h>
#include <schily/schily.h>

#include <netinet/in.h>
#ifdef	HAVE_ARPA_INET_H
#include <arpa/inet.h>		/* BeOS does not have <arpa/inet.h> */
#endif				/* but inet_ntaoa() is in <netdb.h> */
#ifdef	 HAVE_NETDB_H
#include <netdb.h>
#endif

#if (!defined(HAVE_NETDB_H) || !defined(HAVE_SYS_SOCKET_H))
#undef	USE_REMOTE
#endif

EXPORT	int	main		__PR((int argc, char **argv));
#ifdef	USE_REMOTE
LOCAL	void	checkuser	__PR((void));
LOCAL	char	*getpeer	__PR((void));
LOCAL	BOOL	checkaccess	__PR((char *device));
LOCAL	BOOL	strmatch	__PR((char *str, char *pat));
LOCAL	void	dormt		__PR((void));
LOCAL	void	opentape	__PR((void));
LOCAL	int	rmtoflags	__PR((char *omode));
LOCAL	void	closetape	__PR((void));
LOCAL	void	readtape	__PR((void));
LOCAL	void	writetape	__PR((void));
LOCAL	void	ioctape		__PR((int cmd));
#ifdef	HAVE_SYS_MTIO_H
LOCAL	int	rmtmapold	__PR((int cmd));
LOCAL	int	rmtmapnew	__PR((int cmd));
#endif
LOCAL	void	statustape	__PR((int cmd));
LOCAL	void	seektape	__PR((void));
LOCAL	void	doversion	__PR((void));
LOCAL	int	fillrdbuf	__PR((void));
LOCAL	void	tryfillrdbuf	__PR((void));
LOCAL	int	readchar	__PR((char *cp));
LOCAL	void	readbuf		__PR((char *buf, int n));
LOCAL	int	readarg		__PR((char *buf, int n));
LOCAL	char *	preparebuffer	__PR((int size));
LOCAL	int	checktape	__PR((char *device));
LOCAL	BOOL	has_dotdot	__PR((char *name));
LOCAL	void	rmtrespond	__PR((long ret, int err));
LOCAL	void	rmterror	__PR((char *str));

#define	CMD_SIZE	80

LOCAL	char	*username;
LOCAL	char	*peername;

LOCAL	int	tape_fd = -1;

LOCAL	char	*debug_name;
LOCAL	FILE	*debug_file;
LOCAL	BOOL	found_dfltfile;

#define	DEBUG(fmt)			if (debug_file) js_fprintf(debug_file, fmt)
#define	DEBUG1(fmt, a)			if (debug_file) js_fprintf(debug_file, fmt, a)
#define	DEBUG2(fmt, a1, a2)		if (debug_file) js_fprintf(debug_file, fmt, a1, a2)
#define	DEBUG3(fmt, a1, a2, a3)		if (debug_file) js_fprintf(debug_file, fmt, a1, a2, a3)
#define	DEBUG4(fmt, a1, a2, a3, a4)	if (debug_file) js_fprintf(debug_file, fmt, a1, a2, a3, a4)
#endif	/* USE_REMOTE */

EXPORT int
main(argc, argv)
	int	argc;
	char	**argv;
{
	save_args(argc, argv);
#ifndef	USE_REMOTE
	comerrno(EX_BAD, "No remote TAPE support on this platform.\n");
#else
	argc--, argv++;
	if (argc > 0 && strcmp(*argv, "-c") == 0) {
		/*
		 * Skip params in case we have been installed/called as shell.
		 */
		argc--, argv++;
		argc--, argv++;
	}
	/*
	 * If we are running as root (uid 0), the existence of /etc/default/rmt
	 * is required. If our uid is != 0 and there is no /etc/default/rmt
	 * we will only allow to access files in /dev (see below).
	 *
	 * WARNING you are only allowed to change the defaults configuration
	 * filename if you also change the documentation and add a statement
	 * that makes clear where the official location of the file is, why you
	 * did choose a nonstandard location and that the nonstandard location
	 * only refers to inofficial rmt versions.
	 *
	 * I was forced to add this because some people change cdrecord without
	 * rational reason and then publish the result. As those people
	 * don't contribute work and don't give support, they are causing extra
	 * work for me and this way slow down the development.
	 */
	if (defltopen("/etc/default/rmt") < 0) {
		if (geteuid() == 0) {
			rmterror("Remote configuration error: Cannot open /etc/default/rmt");
			exit(EX_BAD);
		}
	} else {
		found_dfltfile = TRUE;
	}
	debug_name = defltread("DEBUG=");	/* Get debug file name */
#ifdef	FORCE_DEBUG
	if (debug_name == NULL && argc <= 0)
		debug_name = "/tmp/RMT";
#endif
#ifdef	NONONO
	/*
	 * Allowing to write arbitrary files may be a security risk.
	 */
	if (argc > 0)
		debug_name = *argv;
#endif

	if (debug_name != NULL)
		debug_file = fopen(debug_name, "w");

	if (argc > 0) {
		if (debug_file == 0) {
			rmtrespond((long)-1, geterrno());
			exit(EX_BAD);
		}
		(void) setbuf(debug_file, (char *)0);
	}
	checkuser();		/* Check if we are called by a bad guy	*/
	peername = getpeer();	/* Get host name of caller		*/
	dormt();
#endif	/* USE_REMOTE */
	return (0);
}

#ifdef	USE_REMOTE
LOCAL void
checkuser()
{
	uid_t	uid = getuid();
	char	*uname;
	struct passwd *pw;

	pw = getpwuid(uid);
	if (pw == NULL)
		goto notfound;

	username = pw->pw_name;

	/*
	 * If no /etc/default/rmt could be found allow general access.
	 */
	if (!found_dfltfile)
		return;

	defltfirst();
	while ((uname = defltnext("USER=")) != NULL) {
		if (strmatch(username, uname))
			return;
	}
notfound:
	rmterror("Illegal user id for RMT server");
	exit(EX_BAD);
}

#ifndef	NI_MAXHOST
#ifdef	MAXHOSTNAMELEN			/* XXX remove this and sys/param.h */
#define	NI_MAXHOST	MAXHOSTNAMELEN
#else
#define	NI_MAXHOST	64
#endif
#endif

LOCAL char *
getpeer()
{
#ifdef	HAVE_GETNAMEINFO
#ifdef	HAVE_SOCKADDR_STORAGE
	struct sockaddr_storage sa;
#else
	char			sa[256];
#endif
#else
	struct	sockaddr sa;
	struct hostent	*he;
#endif
	struct	sockaddr *sap;
	struct	sockaddr_in *s;
	socklen_t	 sasize = sizeof (sa);
static	char		buffer[NI_MAXHOST];

	sap = (struct  sockaddr *)&sa;
	if (getpeername(STDIN_FILENO, sap, &sasize) < 0) {
		int		errsav = geterrno();
		struct stat	sb;

		if (fstat(STDIN_FILENO, &sb) >= 0) {
			if (S_ISFIFO(sb.st_mode)) {
				DEBUG("rmt: stdin is a PIPE\n");
				return ("PIPE");
			}
			DEBUG1("rmt: stdin st_mode %0llo\n", (Llong)sb.st_mode);
		}

		DEBUG1("rmt: peername %s\n", errmsgstr(errsav));
		return ("ILLEGAL_SOCKET");
	} else {
		s = (struct sockaddr_in *)&sa;
#ifdef	AF_INET6
		if (s->sin_family != AF_INET && s->sin_family != AF_INET6) {
#else
		if (s->sin_family != AF_INET) {
#endif
#ifdef	AF_UNIX
			/*
			 * AF_UNIX is not defined on BeOS
			 */
			if (s->sin_family == AF_UNIX) {
				DEBUG("rmt: stdin is a PIPE (UNIX domain socket)\n");
				return ("PIPE");
			}
#endif
			DEBUG1("rmt: stdin NOT_IP socket (sin_family: %d)\n",
							s->sin_family);
			return ("NOT_IP");
		}

#ifdef	HAVE_GETNAMEINFO
		buffer[0] = '\0';
		if (debug_file &&
		    getnameinfo(sap, sasize, buffer, sizeof (buffer), NULL, 0,
		    NI_NUMERICHOST) == 0) {
			DEBUG1("rmt: peername %s\n", buffer);
		}
		buffer[0] = '\0';
		if (getnameinfo(sap, sasize, buffer, sizeof (buffer), NULL, 0,
		    0) == 0) {
			DEBUG1("rmt: peername %s\n", buffer);
			return (buffer);
		}
		return ("CANNOT_MAP_ADDRESS");
#else	/* HAVE_GETNAMEINFO */
#ifdef	HAVE_INET_NTOA
		(void) js_snprintf(buffer, sizeof (buffer), "%s",
						inet_ntoa(s->sin_addr));
#else
		(void) js_snprintf(buffer, sizeof (buffer), "%x",
						s->sin_addr.s_addr);
#endif
		DEBUG1("rmt: peername %s\n", buffer);
		he = gethostbyaddr((char *)&s->sin_addr.s_addr, 4, AF_INET);
		DEBUG1("rmt: peername %s\n", he != NULL ? he->h_name:buffer);
		if (he != NULL)
			return (he->h_name);
		return (buffer);
#endif	/* HAVE_GETNAMEINFO */
	}
}

LOCAL BOOL
checkaccess(device)
	char	*device;
{
	char	*target;
	char	*user;
	char	*host;
	char	*fname;
	char	*p;

	if (peername == NULL)
		return (FALSE);
	defltfirst();
	while ((target = defltnext("ACCESS=")) != NULL) {
		p = target;
		while (*p == '\t')
			p++;
		user = p;
		if ((p = strchr(p, '\t')) != NULL)
			*p++ = '\0';
		else
			continue;
		if (!strmatch(username, user))
			continue;

		while (*p == '\t')
			p++;
		host = p;
		if ((p = strchr(p, '\t')) != NULL)
			*p++ = '\0';
		else
			continue;
		if (!strmatch(peername, host))
			continue;

		fname = p;
		if ((p = strchr(p, '\t')) != NULL)
			*p++ = '\0';

		DEBUG3("ACCESS %s %s %s\n", user, host, fname);

		if (has_dotdot(device))		/* Do not allow ".." in name */
			continue;
		if (!strmatch(device, fname))
			continue;
		return (TRUE);
	}
	return (FALSE);
}

LOCAL BOOL
strmatch(str, pat)
	char	*str;
	char	*pat;
{
	int	*aux;
	int	*state;
	int	alt;
	int	plen;
	char	*p;

	plen = strlen(pat);
	aux = malloc(plen*sizeof (int));
	state = malloc((plen+1)*sizeof (int));
	if (aux == NULL || state == NULL) {
		if (aux) free(aux);
		if (state) free(state);
		return (FALSE);
	}

	if ((alt = patcompile((const unsigned char *)pat, plen, aux)) == 0) {
		/* Bad pattern */
		free(aux);
		free(state);
		return (FALSE);
	}

	p = (char *)patmatch((const unsigned char *)pat, aux,
						(const unsigned char *)str, 0,
						strlen(str), alt, state);
	free(aux);
	free(state);

	if (p != NULL && *p == '\0')
		return (TRUE);
	return (FALSE);
}

LOCAL void
dormt()
{
	char	c;

	while (readchar(&c) == 1) {
		seterrno(0);

		switch (c) {

		case 'O':
			opentape();
			break;
		case 'C':
			closetape();
			break;
		case 'R':
			readtape();
			break;
		case 'W':
			writetape();
			break;
		case 'I':
		case 'i':
			ioctape(c);
			break;
		case 'S':
		case 's':
			statustape(c);
			break;
		case 'L':
			seektape();
			break;
		/*
		 * It would be nice to have something like 'V' for retrieving
		 * Version information. But unfortunately newer BSD rmt version
		 * implement this command in a way that is not useful at all.
		 */
		case 'v':
			doversion();
			break;
		default:
			DEBUG1("rmtd: garbage command %c\n", c);
			rmterror("Garbage command");
			exit(EX_BAD);
		}
	}
	exit(0);
}

LOCAL void
opentape()
{
	char	device[4096];
	char	omode[CMD_SIZE];
	int	omodes;
	int	n;

	if (tape_fd >= 0)
		(void) close(tape_fd);

	n = readarg(device, sizeof (device));
	if (n < 0 || n >= sizeof (device)) {	/* Try to recover */
		readarg(omode, sizeof (omode));	/* honor protocol */
		DEBUG2("rmtd: O %s %s\n", device, omode);
#ifdef	ENAMETOOLONG
		seterrno(ENAMETOOLONG);
#else
		seterrno(EINVAL);
#endif
		goto out;
	}
	readarg(omode, sizeof (omode));
	omodes = rmtoflags(omode);
	if (omodes == -1) {
		/*
		 * Mask off all bits that differ between operating systems.
		 */
		omodes = atoi(omode);
		omodes &= (O_RDONLY|O_WRONLY|O_RDWR);
	}
#ifdef	O_TEXT
	/*
	 * Default to O_BINARY the client may not know that we need it.
	 */
	if ((omodes & O_TEXT) == 0)
		omodes |= O_BINARY;
#endif
	DEBUG2("rmtd: O %s %s\n", device, omode);
	if (!checktape(device)) {
		tape_fd = -1;
		seterrno(EACCES);
	} else {
		tape_fd = open(device, omodes, (mode_t)0666);
	}
out:
	rmtrespond((long)tape_fd, geterrno());
}

LOCAL struct oflags {
	char	*fname;
	int	fval;
} oflags[] = {
	{ "O_RDONLY",	O_RDONLY },
	{ "O_RDWR",	O_RDWR },
	{ "O_WRONLY",	O_WRONLY },
#ifdef	O_TEXT
	{ "O_TEXT",	O_TEXT },
#endif
#ifdef	O_NDELAY
	{ "O_NDELAY",	O_NDELAY },
#endif
#ifdef	O_APPEND
	{ "O_APPEND",	O_APPEND },
#endif
#ifdef	O_SYNC
	{ "O_SYNC",	O_SYNC },
#endif
#ifdef	O_DSYNC
	{ "O_DSYNC",	O_DSYNC },
#endif
#ifdef	O_RSYNC
	{ "O_RSYNC",	O_RSYNC },
#endif
#ifdef	O_NONBLOCK
	{ "O_NONBLOCK",	O_NONBLOCK },
#endif
#ifdef	O_PRIV
	{ "O_PRIV",	O_PRIV },
#endif
#ifdef	O_LARGEFILE
	{ "O_LARGEFILE",O_LARGEFILE },
#endif
#ifdef	O_CREAT
	{ "O_CREAT",	O_CREAT },
#endif
#ifdef	O_TRUNC
	{ "O_TRUNC",	O_TRUNC },
#endif
#ifdef	O_EXCL
	{ "O_EXCL",	O_EXCL },
#endif
#ifdef	O_NOCTTY
	{ "O_NOCTTY",	O_NOCTTY },
#endif
	{ NULL,		0 }
};

LOCAL int
rmtoflags(omode)
	char	*omode;
{
	register char		*p = omode;
	register struct oflags	*op;
	register int		slen;
	register int		nmodes = 0;

	/*
	 * First skip numeric open modes...
	 */
	while (*p != '\0' && *p == ' ')
		p++;
	if (*p != 'O') while (*p != '\0' && *p != ' ')
		p++;
	while (*p != '\0' && *p != 'O')
		p++;
	do {
		if (p[0] != 'O' || p[1] != '_')
			return (-1);

		for (op = oflags; op->fname; op++) {
			slen = strlen(op->fname);
			if ((strncmp(op->fname, p, slen) == 0) &&
			    (p[slen] == '|' || p[slen] == ' ' ||
			    p[slen] == '\0')) {
				nmodes |= op->fval;
				break;
			}
		}
		p = strchr(p, '|');
	} while (p && *p++ == '|');

	return (nmodes);
}

LOCAL void
closetape()
{
	int	ret;
	char	device[CMD_SIZE];

	DEBUG("rmtd: C\n");
	readarg(device, sizeof (device));
	ret = close(tape_fd);
	rmtrespond((long)ret, geterrno());
	tape_fd = -1;
}

LOCAL void
readtape()
{
	int	n;
	long	ret;
	char	*buf;
	char	count[CMD_SIZE];

	readarg(count, sizeof (count));
	DEBUG1("rmtd: R %s\n", count);
	n = atoi(count);		/* Only an int because of setsockopt */
	buf = preparebuffer(n);
	ret = _niread(tape_fd, buf, n);
	rmtrespond(ret, geterrno());
	if (ret >= 0) {
		(void) _nixwrite(STDOUT_FILENO, buf, ret);
	}
}

LOCAL void
writetape()
{
	int	n;
	long	ret;
	char	*buf;
	char	count[CMD_SIZE];

	readarg(count, sizeof (count));
	n = atoi(count);		/* Only an int because of setsockopt */
	DEBUG1("rmtd: W %s\n", count);
	buf = preparebuffer(n);
	readbuf(buf, n);
	ret = _niwrite(tape_fd, buf, n);
	rmtrespond(ret, geterrno());
}

/*
 * Definitions for the new RMT Protocol version 1
 *
 * The new Protocol version tries to make the use
 * of rmtioctl() more portable between different platforms.
 */
#define	RMTIVERSION	-1
#define	RMT_VERSION	1

/*
 * Support for commands beyond MTWEOF..MTNOP (0..7)
 */
#define	RMTICACHE	0
#define	RMTINOCACHE	1
#define	RMTIRETEN	2
#define	RMTIERASE	3
#define	RMTIEOM		4
#define	RMTINBSF	5

#ifndef	HAVE_SYS_MTIO_H
LOCAL void
ioctape(cmd)
	int	cmd;
{
	char	count[CMD_SIZE];
	char	opcode[CMD_SIZE];

	readarg(opcode, sizeof (opcode));
	readarg(count, sizeof (count));
	DEBUG3("rmtd: %c %s %s\n", cmd, opcode, count);
	if (atoi(opcode) == RMTIVERSION) {
		rmtrespond((long)RMT_VERSION, 0);
	} else {
		rmtrespond((long)-1, ENOTTY);
	}
}
#else

LOCAL void
ioctape(cmd)
	int	cmd;
{
	long	ret = 0;
	int	i;
	char	count[CMD_SIZE];
	char	opcode[CMD_SIZE];
	struct mtop mtop;
static	BOOL	version_seen = FALSE;

	readarg(opcode, sizeof (opcode));
	readarg(count, sizeof (count));
	DEBUG3("rmtd: %c %s %s\n", cmd, opcode, count);
	mtop.mt_op = atoi(opcode);
	ret = atol(count);
	mtop.mt_count = ret;
	if (mtop.mt_count != ret) {
		rmtrespond((long)-1, EINVAL);
		return;
	}

	/*
	 * Only Opcodes 0..7 are unique across different architectures.
	 * But as in many cases Linux does not even follow this rule.
	 * If we know that we have been called by a VERSION 1 client,
	 * we may safely assume that the client is not using Linux mapping
	 * but the standard mapping.
	 */
	ret = 0;
	if (cmd == 'I' && version_seen && (mtop.mt_op != RMTIVERSION)) {
		i = rmtmapold(mtop.mt_op);
		if (i < 0) {
			/*
			 * Should we rather give it a chance instead
			 * of aborting the command?
			 */
			rmtrespond((long)-1, EINVAL);
			return;
		}
		mtop.mt_op = i;
	}
	if (cmd == 'i') {
		i = rmtmapnew(mtop.mt_op);
		if (i < 0) {
			ret = -1;
			seterrno(EINVAL);
		} else {
			mtop.mt_op = i;
		}
	}
	DEBUG4("rmtd: %c %d %ld ret: %ld (mapped)\n", cmd, mtop.mt_op,
						(long)mtop.mt_count, ret);
	if (ret == 0) {
		if (mtop.mt_op == RMTIVERSION) {
			/*
			 * Client must retrieve RMTIVERSION directly after
			 * opening the drive using the 'O' rmt command.
			 */
			ret = mtop.mt_count = RMT_VERSION;
			version_seen = TRUE;
		} else {
			ret = ioctl(tape_fd, MTIOCTOP, (char *)&mtop);
		}
	}
	if (ret < 0) {
		rmtrespond(ret, geterrno());
	} else {
		ret = mtop.mt_count;
		rmtrespond(ret, geterrno());
	}
}

/*
 * Map all old /etc/rmt (over the wire) opcodes that should be in range 0..7
 * to numbers that are understood by the local driver.
 * This is needed because Linux does not follow the UNIX conventions.
 */
LOCAL int
rmtmapold(cmd)
	int	cmd;
{
	switch (cmd) {

	case 0:
#ifdef	MTWEOF
		return (MTWEOF);
#else
		return (-1);
#endif

	case 1:
#ifdef	MTFSF
		return (MTFSF);
#else
		return (-1);
#endif

	case 2:
#ifdef	MTBSF
		return (MTBSF);
#else
		return (-1);
#endif

	case 3:
#ifdef	MTFSR
		return (MTFSR);
#else
		return (-1);
#endif

	case 4:
#ifdef	MTBSR
		return (MTBSR);
#else
		return (-1);
#endif

	case 5:
#ifdef	MTREW
		return (MTREW);
#else
		return (-1);
#endif

	case 6:
#ifdef	MTOFFL
		return (MTOFFL);
#else
		return (-1);
#endif

	case 7:
#ifdef	MTNOP
		return (MTNOP);
#else
		return (-1);
#endif

	}
	return (-1);
}

/*
 * Map all new /etc/rmt (over the wire) opcodes from 'i' command
 * to numbers that are understood by the local driver.
 */
LOCAL int
rmtmapnew(cmd)
	int	cmd;
{
	switch (cmd) {

#ifdef	MTCACHE
	case RMTICACHE:		return (MTCACHE);
#endif
#ifdef	MTNOCACHE
	case RMTINOCACHE:	return (MTNOCACHE);
#endif
#ifdef	MTRETEN
	case RMTIRETEN:		return (MTRETEN);
#endif
#ifdef	MTERASE
	case RMTIERASE:		return (MTERASE);
#endif
#ifdef	MTEOM
	case RMTIEOM:		return (MTEOM);
#endif
#ifdef	MTNBSF
	case RMTINBSF:		return (MTNBSF);
#endif
	}
	return (-1);
}
#endif

/*
 * Old MTIOCGET copies a binary version of struct mtget back
 * over the wire. This is highly non portable.
 * MTS_* retrieves ascii versions (%d format) of a single
 * field in the struct mtget.
 * NOTE: MTS_ERREG may only be valid on the first call and
 *	 must be retrived first.
 */
#define	MTS_TYPE	'T'		/* mtget.mt_type */
#define	MTS_DSREG	'D'		/* mtget.mt_dsreg */
#define	MTS_ERREG	'E'		/* mtget.mt_erreg */
#define	MTS_RESID	'R'		/* mtget.mt_resid */
#define	MTS_FILENO	'F'		/* mtget.mt_fileno */
#define	MTS_BLKNO	'B'		/* mtget.mt_blkno */
#define	MTS_FLAGS	'f'		/* mtget.mt_flags */
#define	MTS_BF		'b'		/* mtget.mt_bf */

#ifndef	HAVE_SYS_MTIO_H
LOCAL void
statustape(cmd)
	int	cmd;
{
	char	subcmd;

	if (cmd == 's') {
		if (readchar(&subcmd) != 1)
			return;
		DEBUG2("rmtd: %c%c\n", cmd, subcmd);
	} else {
		DEBUG1("rmtd: %c\n", cmd);
	}
	rmtrespond((long)-1, ENOTTY);
}
#else

LOCAL void
statustape(cmd)
	int	cmd;
{
	int	ret;
	char	subcmd;
	struct mtget mtget;

	/*
	 * Only the first three fields of the struct mtget (mt_type, mt_dsreg
	 * and mt_erreg) are identical on all platforms. The original struct
	 * mtget is 16 bytes. All client implementations except the one from
	 * star will overwrite other data and probably die if the remote struct
	 * mtget is bigger than the local one.
	 * In addition, there are byte order problems.
	 */
	if (cmd == 's') {
		if (readchar(&subcmd) != 1)
			return;
		DEBUG2("rmtd: %c%c\n", cmd, subcmd);
	} else {
		DEBUG1("rmtd: %c\n", cmd);
	}
	ret = ioctl(tape_fd, MTIOCGET, (char *)&mtget);
	if (ret < 0) {
		rmtrespond((long)ret, geterrno());
	} else {
		if (cmd == 's') switch (subcmd) {

#ifdef	HAVE_MTGET_TYPE
		case MTS_TYPE:
			rmtrespond(mtget.mt_type, geterrno());	break;
#endif
#ifdef	HAVE_MTGET_DSREG
		case MTS_DSREG:
			rmtrespond(mtget.mt_dsreg, geterrno());	break;
#endif
#ifdef	HAVE_MTGET_ERREG
		case MTS_ERREG:
			rmtrespond(mtget.mt_erreg, geterrno());	break;
#endif
#ifdef	HAVE_MTGET_RESID
		case MTS_RESID:
			rmtrespond(mtget.mt_resid, geterrno());	break;
#endif
#ifdef	HAVE_MTGET_FILENO
		case MTS_FILENO:
			rmtrespond(mtget.mt_fileno, geterrno()); break;
#endif
#ifdef	HAVE_MTGET_BLKNO
		case MTS_BLKNO:
			rmtrespond(mtget.mt_blkno, geterrno());	break;
#endif
#ifdef	HAVE_MTGET_FLAGS
		case MTS_FLAGS:
			rmtrespond(mtget.mt_flags, geterrno());	break;
#endif
#ifdef	HAVE_MTGET_BF
		case MTS_BF:
			rmtrespond(mtget.mt_bf, geterrno());	break;
#endif
		default:
			rmtrespond((long)-1, EINVAL);		break;
		} else {
			/*
			 * Do not expect that this interface makes any sense.
			 * With UNIX, you may at least trust the first two
			 * struct members, but Linux is completely incompatible
			 */
			ret = sizeof (mtget);
			rmtrespond((long)ret, geterrno());
			(void) _nixwrite(STDOUT_FILENO, (char *)&mtget,
							sizeof (mtget));
		}
	}
}
#endif

LOCAL void
seektape()
{
	off_t	ret;
	char	count[CMD_SIZE];
	char	whence[CMD_SIZE];
	Llong	offset = (Llong)0;
	int	iwhence;

	readarg(count, sizeof (count));
	readarg(whence, sizeof (whence));
	DEBUG2("rmtd: L %s %s\n", count, whence);
	(void) astoll(count, &offset);
	iwhence = atoi(whence);
	switch (iwhence) {

	case 0:	iwhence = SEEK_SET; break;
	case 1:	iwhence = SEEK_CUR; break;
	case 2:	iwhence = SEEK_END; break;
#ifdef	SEEK_DATA
	case 3:	iwhence = SEEK_DATA; break;
#endif
#ifdef	SEEK_HOLE
	case 4:	iwhence = SEEK_HOLE; break;
#endif

	default:
		DEBUG1("rmtd: Illegal lseek() whence %d\n", iwhence);
		rmtrespond((long)-1, EINVAL);
		return;
	}
	ret = (off_t)offset;
	if (ret != offset) {
		DEBUG1("rmtd: Illegal seek offset %lld\n", offset);
		rmtrespond((long)-1, EINVAL);
		return;
	}
	ret = lseek(tape_fd, (off_t)offset, iwhence);
	if ((ret != (off_t)-1) && (sizeof (ret) > sizeof (long))) {
		DEBUG1("rmtd: A %lld\n", (Llong)ret);
		(void) js_snprintf(count, sizeof (count), "A%lld\n",
								(Llong)ret);
		(void) _nixwrite(STDOUT_FILENO, count, strlen(count));
		return;
	}
	rmtrespond((long)ret, geterrno());
}

LOCAL void
doversion()
{
	char	arg[CMD_SIZE];

	readarg(arg, sizeof (arg));	/* We may like to add an arg later */
	DEBUG1("rmtd: v %s\n", arg);
	rmtrespond((long)RMT_VERSION, 0);
}

#define	READB_SIZE	128
LOCAL	char		readb[READB_SIZE];
LOCAL	char		*readbptr;
LOCAL	int		readbcnt;

LOCAL int
fillrdbuf()
{
	readbptr = readb;

	return (readbcnt = _niread(STDIN_FILENO, readb, READB_SIZE));
}

/*
 * This function is used for error recovery, it thus may be slow.
 * We try to fill the read buffer in case there is something to read.
 * We will not block here, if the OS does not support O_NONBLOCK we
 * will just do nothing.
 */
LOCAL void
tryfillrdbuf()
{
#if	defined(F_GETFL) && defined(F_SETFL) && defined(O_NONBLOCK)
	int	fl;

	fl = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, fl|O_NONBLOCK);

	fillrdbuf();

	fcntl(STDIN_FILENO, F_SETFL, fl);
#endif
}

LOCAL int
readchar(cp)
	char	*cp;
{
	if (--readbcnt < 0) {
		if (fillrdbuf() <= 0)
			return (readbcnt);
		--readbcnt;
	}
	*cp = *readbptr++;
	return (1);
}

LOCAL void
readbuf(buf, n)
	register char	*buf;
	register int	n;
{
	register int	i = 0;
	register int	amt;

	if (readbcnt > 0) {
		amt = readbcnt;
		if (amt > n)
			amt = n;
		movebytes(readbptr, buf, amt);
		readbptr += amt;
		readbcnt -= amt;
		i += amt;
	}

	for (; i < n; i += amt) {
		amt = _niread(STDIN_FILENO, &buf[i], n - i);
		if (amt <= 0) {
			DEBUG("rmtd: premature eof\n");
			rmterror("Premature eof");
			exit(EX_BAD);
		}
	}
}

LOCAL int
readarg(buf, n)
	char	*buf;
	int	n;
{
	int	i;
	char	c;

	for (i = 0; i < n; i++) {
		if (readchar(&buf[i]) != 1)
			exit(0);
		if (buf[i] == '\n')
			break;
	}
	if (buf[i] == '\n') {
		buf[i] = '\0';
		return (--i);		/* Do not include null byte */
	}
	buf[n-1] = '\0';

	/*
	 * The following code is for error recovery.
	 * We come here if the client send us too long parameters.
	 * We try to recover from the problem by reading a reasonable
	 * amount of data in hope to find the newline which is the
	 * argument terminator.
	 */
	if (readbcnt <= 0)
		tryfillrdbuf();
	for (i = 0; readbcnt > 0 && i < 10000; i++) {
		if (readchar(&c) != 1)
			exit(0);
		if (c == '\n')
			break;
		if (readbcnt <= 0)
			tryfillrdbuf();
	}
	return (n);
}

LOCAL char *
preparebuffer(size)
	int	size;
{
static	char	*buffer		= 0;
static	int	buffersize	= 0;

	if (buffer != 0 && size <= buffersize)
		return (buffer);
	if (buffer != 0)
		free(buffer);
	buffer = malloc(size);
	if (buffer == 0) {
		DEBUG("rmtd: cannot allocate buffer space\n");
		rmterror("Cannot allocate buffer space");
		exit(EX_BAD);
	}
	buffersize = size;

#ifdef	SO_SNDBUF
	while (size > 512 &&
		setsockopt(STDOUT_FILENO, SOL_SOCKET, SO_SNDBUF,
					(char *)&size, sizeof (size)) < 0)
		size -= 512;
	DEBUG1("rmtd: sndsize: %d\n", size);
#endif
#ifdef	SO_RCVBUF
	while (size > 512 &&
		setsockopt(STDIN_FILENO, SOL_SOCKET, SO_RCVBUF,
					(char *)&size, sizeof (size)) < 0)
		size -= 512;
	DEBUG1("rmtd: rcvsize: %d\n", size);
#endif
	return (buffer);
}

/*
 * If we are not root and there is no /etc/default/rmt
 * we will only allow to access files in /dev.
 * We do this because we may assume that non-root access to files
 * in /dev is only granted if it does not open security holes.
 * Accessing files (e.g. /etc/passwd) is not possible.
 * Otherwise permissions depend on the content of /etc/default/rmt.
 */
LOCAL int
checktape(device)
	char	*device;
{
	if (!found_dfltfile) {
		if (has_dotdot(device))
			return (0);
		if (strncmp(device, "/dev/", 5) == 0)
			return (1);
		return (0);
	}
	return (checkaccess(device));
}

LOCAL BOOL
has_dotdot(name)
	char	*name;
{
	register char	*p = name;

	while (*p) {
		if ((p[0] == '.' && p[1] == '.') &&
		    (p[2] == '/' || p[2] == '\0')) {
			return (TRUE);
		}
		do {
			if (*p++ == '\0')
				return (FALSE);
		} while (*p != '/');
		p++;
		while (*p == '/')	/* Skip multiple slashes */
			p++;
	}
	return (FALSE);
}
LOCAL void
rmtrespond(ret, err)
	long	ret;
	int	err;
{
	char	rbuf[2*CMD_SIZE];

	if (ret >= 0) {
		DEBUG1("rmtd: A %ld\n", ret);
		(void) js_snprintf(rbuf, sizeof (rbuf), "A%ld\n", ret);
	} else {
		DEBUG2("rmtd: E %d (%s)\n", err, errmsgstr(err));
		(void) js_snprintf(rbuf, sizeof (rbuf), "E%d\n%s\n", err,
							errmsgstr(err));
	}
	(void) _nixwrite(STDOUT_FILENO, rbuf, strlen(rbuf));
}

LOCAL void
rmterror(str)
	char	*str;
{
	char	rbuf[2*CMD_SIZE];

	DEBUG1("rmtd: E 0 (%s)\n", str);
	(void) js_snprintf(rbuf, sizeof (rbuf), "E0\n%s\n", str);
	(void) _nixwrite(STDOUT_FILENO, rbuf, strlen(rbuf));
}
#endif	/* USE_REMOTE */
