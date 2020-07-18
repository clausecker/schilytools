/* @(#)fifo_main.c	1.3 20/07/08 Copyright 1989, 2019-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fifo_main.c	1.3 20/07/08 Copyright 1989, 2019-2020 J. Schilling";
#endif
/*
 *	Copyright (c) 1989, 2019-2020 J. Schilling
 *
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/limits.h>
#include <schily/stat.h>
#include <schily/signal.h>
#include <schily/wait.h>
#include <schily/errno.h>
#include <schily/nlsdefs.h>
#include <schily/schily.h>
#include "star.h"
#include "starsubs.h"
#include "fifo.h"

#ifndef	PIPE_BUF
#define	PIPE_BUF	4096
#endif

/* BEGIN CSTYLED */
char	options[] =
"help,version,debug,no-statistics,bs&,ibs&,obs&,fs&";
/* END CSTYLED */

LOCAL	void	usage		__PR((int exitcode));
EXPORT	void	set_signal	__PR((int sig, RETSIGTYPE (*handler)(int)));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	int	getenum		__PR((char *arg, long *valp));
EXPORT	void	exprstats	__PR((int ret));
EXPORT	void	setprops	__PR((long htype));
EXPORT	void	changetape	__PR((BOOL donext));
EXPORT	void	closetape	__PR((void));
EXPORT	void	runnewvolscript	__PR((int volno, int nindex));
EXPORT	long	startvol	__PR((char *buf, long amount));
EXPORT	BOOL	verifyvol	__PR((char *buf, int amt, int volno,
								int *skipp));
LOCAL	BOOL	ispipe		__PR((int));

BOOL	debug	  = FALSE;		/* -debug has been specified	*/
long	fs;				/* FIFO size			*/
long	bs;				/* TAPE block size (bytes)	*/
extern	long	ibs;
extern	long	obs;
int	tarfindex;			/* Current index in list	*/
BOOL	multivol = FALSE;		/* -multivol specified		*/
long	chdrtype  = H_UNDEF;		/* command line hdrtype		*/
BOOL	cflag	  = FALSE;		/* -c has been specified	*/
BOOL	copyflag  = FALSE;		/* -copy has been specified	*/
BOOL	lowmem	  = FALSE;		/* -lowmem use less memory	*/
BOOL	no_stats  = FALSE;		/* -no-statistics specified	*/
m_stats	*stats;
int	pid;
extern	m_head	*mp;

GINFO	_ginfo;				/* Global (volhdr) information	*/
GINFO	_grinfo;			/* Global read information	*/
GINFO	*gip  = &_ginfo;		/* Global information pointer	*/
GINFO	*grip = &_grinfo;		/* Global read info pointer	*/

long	bufsize	= 0;		/* Available buffer size */
char	*bigbuf	= NULL;
char	*bigptr	= NULL;

LOCAL void
usage(exitcode)
	int	exitcode;
{
	gterror("Usage:	fifo [options]\n");
	gterror("Options:\n");
	gterror("	fs=#	set fifo size to #\n");
	gterror("	bs=#	set buffer size to #\n");
	gterror("	ibs=#	set input buffer size to #\n");
	gterror("	obs=#	set output buffer size to #\n");
	gterror("	-no-statistics	do not print fifo statistics\n");
	gterror("	-help	print this help\n");
	gterror("	-debug	print additional debug messages\n");
	gterror("	-version print version information and exit\n");

	exit(exitcode);
}

EXPORT void
set_signal(sig, handler)
	int		sig;
	RETSIGTYPE	(*handler)	__PR((int));
{
#if	defined(HAVE_SIGPROCMASK) && defined(SA_RESTART)
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	(void) sigaction(sig, &sa, (struct sigaction *)0);
#else
#ifdef	HAVE_SIGSETMASK
	struct sigvec	sv;

	sv.sv_mask = 0;
	sv.sv_handler = handler;
	sv.sv_flags = 0;
	(void) sigvec(sig, &sv, (struct sigvec *)0);
#else
	(void) signal(sig, handler);
#endif
#endif
}


EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	*const *cav;
	BOOL	help = FALSE;
	BOOL	prvers = FALSE;

	save_args(ac, av);

	(void) setlocale(LC_ALL, "");

	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, options, &help, &prvers,
			&debug, &no_stats,
			getenum, &bs,
			getenum, &ibs,
			getenum, &obs,
			getenum, &fs) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	cac = ac;
	cav = av;
	if (getfiles(&cac, &cav, options) > 0)
		comerrno(EX_BAD, "Too many args.\n");

	if (help)
		usage(0);
	if (prvers) {
		/* BEGIN CSTYLED */
		printf("fifo %s %s (%s-%s-%s)\n\n", "2.0", "2020/07/08", HOST_CPU, HOST_VENDOR, HOST_OS);
		printf("Copyright (C) 1989, 2019-2020 Jörg Schilling\n");
		printf("This is free software; see the source for copying conditions.  There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		/* END CSTYLED */
		exit(0);
	}
	if (bs == 0)
		bs = 512;
	if (ibs == 0) {
		if (ispipe(STDIN_FILENO))
			ibs = PIPE_BUF;
		else
			ibs = bs;
	}
	if (obs == 0) {
		if (ispipe(STDOUT_FILENO))
			obs = PIPE_BUF;
		else
			obs = bs;
	}
	initfifo();
	fifo_prmp(0);
	on_comerr((void(*)__PR((int, void *)))fifo_stats, (void *)0);
	runfifo(ac, av);
	if (pid != 0) {
		wait(0);
		fifo_stats();
	} else {
		exit(0);
	}
	exit(fifo_errno());
}

LOCAL int
getenum(arg, valp)
	char	*arg;
	long	*valp;
{
	int ret = getnum(arg, valp);

	if (ret != 1)
		errmsgno(EX_BAD, "Badly formed number '%s'.\n", arg);
	return (ret);
}

EXPORT void
exprstats(ret)
	int	ret;
{
#if 0
	prstats();
	checkerrs();
	if (use_fifo)
		fifo_exit(ret);
#endif
	exit(ret);
}

EXPORT void
setprops(htype)
	long	htype;
{
}

EXPORT void
changetape(donext)
	BOOL	donext;
{
}

EXPORT void
closetape()
{
}

EXPORT void
runnewvolscript(volno, nindex)
	int	volno;
	int	nindex;
{
}

EXPORT long
startvol(buf, amount)
	char	*buf;		/* The original buffer address		*/
	long	amount;		/* The related requested transfer count	*/
{
	return (1);
}

EXPORT BOOL
verifyvol(buf, amt, volno, skipp)
	char	*buf;
	int	amt;
	int	volno;
	int	*skipp;
{
	return (TRUE);
}

LOCAL BOOL
ispipe(f)
	int	f;
{
	struct stat	sb;

	if (fstat(f, &sb) < 0)
		return (FALSE);

	if (S_ISFIFO(sb.st_mode))
		return (TRUE);

	return (FALSE);
}

EXPORT ssize_t
readtape(buf, amount)
	char	*buf;
	size_t	amount;
{
	register ssize_t	ret;
		int		oerrno = geterrno();

	while ((ret = read(0, buf, amount)) < 0) {
		if (geterrno() == EINTR) {
			seterrno(oerrno);
			continue;
		}
	}
	return (ret);
}

EXPORT ssize_t
writetape(buf, amount)
	char	*buf;
	size_t	amount;
{
	register ssize_t	ret;
		int		oerrno = geterrno();

	while ((ret = write(1, buf, amount)) < 0) {
		if (geterrno() == EINTR) {
			seterrno(oerrno);
			continue;
		}
	}
	return (ret);
}
