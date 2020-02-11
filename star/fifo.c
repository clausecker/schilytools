/* @(#)fifo.c	1.108 20/02/05 Copyright 1989, 1994-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fifo.c	1.108 20/02/05 Copyright 1989, 1994-2020 J. Schilling";
#endif
/*
 *	A "fifo" that uses shared memory between two processes
 *
 *	s*wakeup() 2nd parameter character:
 *
 *		t	fifo_iwait() wake up put side to start read Tape change
 *		d	fifo_owake() wake up get side if mp->oblocked == TRUE
 *		e	fifo_oflush() wake up get side if EOF
 *		T	fifo_owait() wake up put side after write Tape change
 *		s	fifo_iwake() wake up put side if mp->iblocked == TRUE
 *		S	fifo_resume() wake up put side after reading first blk
 *		n	fifo_chitape() wake up put side to start wrt Tape chng
 *		N	fifo_chotape()	wake up get side if mp->oblocked == TRUE
 *		R	fifo_reelwake() wake up put side if mp->reelwait == TRUE
 *
 *	If you ever see a hang in the fifo code, you need to report a stack
 *	trace for both processes (including the line number of the hanging call
 *	to swait()) and the relevant state of struct m_head.
 *
 *	This can be done by sending SIGRTMAX to both star processes via:
 *		kill -RTMAX <pid1>
 *		kill -RTMAX <pid2>
 *	and then to report the output (from stderr).
 *
 *	Copyright (c) 1989, 1994-2020 J. Schilling
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

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* includes <sys/types.h> */
#include <schily/fcntl.h>
#include <schily/signal.h>
#include <schily/standard.h>
#include <schily/errno.h>

#ifdef	FIFO

#if !defined(USE_MMAP) && !defined(USE_USGSHM)
#ifdef	HAVE_SMMAP
#define	USE_MMAP
#endif
#endif
#if defined(HAVE_SMMAP) && defined(USE_MMAP)
#include <schily/mman.h>
#endif
#include "star.h"
#include "starsubs.h"
#include "fifo.h"	/* #undef FIFO may happen here */
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/libport.h>	/* getpagesize() */

#ifndef	HAVE_SMMAP
#undef	USE_MMAP
#ifdef	FIFO
#define	USE_USGSHM	/* SYSV shared memory is the default */
#endif
#endif

#ifdef	HAVE_DOSALLOCSHAREDMEM	/* This is for OS/2 */
#undef	USE_MMAP
#undef	USE_USGSHM
#ifdef	FIFO
#define	USE_OS2SHM
#endif
#endif

#ifdef	HAVE_BEOS_AREAS		/* This is for BeOS/Zeta */
#undef	USE_MMAP
#undef	USE_USGSHM
#undef	USE_OS2SHM
#ifdef	FIFO
#define	USE_BEOS_AREAS
#endif
#endif

#define	HANG_DEBUG
#ifdef DEBUG
#ifndef	HANG_DEBUG
#define	HANG_DEBUG
#endif
#define	EDEBUG(a)	if (debug) error a
#else
#define	EDEBUG(a)
#endif
#ifdef	HANG_DEBUG
#include <schily/string.h>
#endif

	/*
	 * roundup(x, y), x needs to be unsigned or x+y non-negative.
	 */
#undef	roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

char	*buf;
m_head	*mp;
int	buflen;

extern	BOOL	multivol;
extern	BOOL	debug;
extern	BOOL	shmflag;
extern	BOOL	no_stats;
extern	BOOL	lowmem;
extern	long	fs;
extern	long	bs;
extern	long	ibs;
extern	long	obs;
extern	int	hiw;
extern	int	low;
extern	BOOL	copyflag;
extern	long	chdrtype;

extern	m_stats	*stats;
extern	int	pid;

extern	GINFO	*gip;

long	ibs;
long	obs;
int	hiw;
int	low;

LOCAL	int	waitchan;	/* Current waiting channel for debugging */

EXPORT	void	initfifo	__PR((void));
LOCAL	void	fifo_setparams	__PR((void));
EXPORT	void	fifo_ibs_shrink	__PR((int newsize));
EXPORT	void	runfifo		__PR((int ac, char *const *av));
#ifdef	HANG_DEBUG
LOCAL	char	*prgflags	__PR((int f, char *fbuf));
LOCAL	char	*preflags	__PR((int f, char *fbuf));
LOCAL	char	*prpflags	__PR((int f, char *fbuf));
#endif
EXPORT	void	fifo_prmp	__PR((int sig));
EXPORT	void	fifo_stats	__PR((void));
LOCAL	int	swait		__PR((int f, int chan));
LOCAL	int	swakeup		__PR((int f, int c));
EXPORT	int	fifo_amount	__PR((void));
EXPORT	int	fifo_iwait	__PR((int amount));
EXPORT	void	fifo_owake	__PR((int amount));
EXPORT	void	fifo_oflush	__PR((void));
EXPORT	void	fifo_oclose	__PR((void));
EXPORT	int	fifo_owait	__PR((int amount));
EXPORT	void	fifo_iwake	__PR((int amt));
EXPORT	void	fifo_reelwake	__PR((void));
EXPORT	void	fifo_resume	__PR((void));
EXPORT	void	fifo_sync	__PR((int size));
EXPORT	void	fifo_chitape	__PR((void));
EXPORT	void	fifo_chotape	__PR((void));
LOCAL	void	do_in		__PR((void));
LOCAL	void	do_out		__PR((void));
LOCAL	void	fbit_nclear	__PR((bitstr_t *name, int startb, int stopb));
#ifdef	USE_MMAP
LOCAL	char	*mkshare	__PR((int size));
#endif
#ifdef	USE_USGSHM
LOCAL	char	*mkshm		__PR((int size));
#endif
#ifdef	USE_OS2SHM
LOCAL	char	*mkos2shm	__PR((int size));
#endif
#ifdef	USE_BEOS_AREAS
LOCAL	char	*mkbeosshm	__PR((int size));
LOCAL	void	beosshm_child	__PR((void));
#endif

/*
 * Initialize the FIFO.
 *
 * This is called from the buffer administration at start up time.
 * It just allocates the FIFO buffer and initializes the data structures.
 */
EXPORT void
initfifo()
{
extern	BOOL	cflag;
	int	pagesize;
	int	addsize;

	if (obs == 0)
		obs = bs;
	if (fs == 0) {
#if	defined(sun) && defined(mc68000)
		fs = 1*1024*1024;
#else
#if defined(__linux) && !defined(USE_MMAP)
		fs = 4*1024*1024;
#else
#if defined(ultrix)
		fs = 1*1024*1024;
#else
		fs = 8*1024*1024;
#endif
#endif
#endif
		if (lowmem)
			fs = 1*1024*1024;
	}
	if (fs < bs + obs)
		fs = bs + obs;
	/*
	 * We need at least 2x obs for the visible part of the FIFO to allow
	 * the inout and the output act independently.
	 */
	if (fs < 2*obs)
		fs = 2*obs;
	fs = roundup(fs, obs);
	pagesize = getpagesize();
	if (pagesize <= 0)
		pagesize = TBLOCK;

	/*
	 * 'addsize' covers the invisible part of the FIFO.
	 * The start of the real shared memory carries m_head, followed by
	 * the space needed for moving residual bytes "down" before the
	 * official start of the vivible part of the FIFO.
	 * This is needed to grant that the FIFO output may always be done
	 * in "obs" chunks to grant a unique tape output block size.
	 * If we ony need to support CPIO to be fed into a tar inout, it
	 * would be sufficient to add TBLOCK size. If we like to support
	 * auto split multi volume archives, we need to count on QIC drives
	 * that are unblocked and write less than a TAR "tape block" on EOT.
	 * In this case we may need to move anything < obs "down" before the
	 * official start of the FIFO.
	 * We reserve at least 300 bytes for the GINFO string table.
	 */
#ifdef	CPIO_ONLY
	addsize = sizeof (m_head) + 300 + TBLOCK;
#else
	addsize = sizeof (m_head) + 300 + obs;
#endif
	if (multivol && cflag)
		addsize += bitstr_size(fs/TBLOCK);
	addsize = roundup(addsize, pagesize);

	buflen = roundup(fs, pagesize) + addsize;
	/* CSTYLED */
	EDEBUG(("bs: %ld obs: %ld fs: %ld buflen: %d addsize: %d bitstr size: %ld\n", bs, obs, fs, buflen, addsize, bitstr_size(fs/TBLOCK)));
	/*
	 * llitos() overshoots by one space (' ') in cpio mode, add pagesize.
	 * See also fifo_setparams().
	 */
	buflen += pagesize;

#if	defined(USE_MMAP) && defined(USE_USGSHM)
	if (shmflag)
		buf = mkshm(buflen);
	else
		buf = mkshare(buflen);
#else
#if	defined(USE_MMAP)
	buf = mkshare(buflen);
#endif
#if	defined(USE_USGSHM)
	buf = mkshm(buflen);
#endif
#if	defined(USE_OS2SHM)
	buf = mkos2shm(buflen);
#endif
#if	defined(USE_BEOS_AREAS)
	buf = mkbeosshm(buflen);
#endif
#endif
	if (buf == NULL) {
		comerrno(EX_BAD,
			"Cannot get shared memory, fifo missconfigured?\n");
	}
	mp = (m_head *)buf;
	fillbytes(buf, addsize, '\0');	/* We init the complete add. space */
	stats = &mp->stats;
	stats->hdrtype = chdrtype;
	mp->base = &buf[addsize];
	mp->iblocked = FALSE;
	mp->oblocked = FALSE;
	mp->mayoblock = FALSE;
	/*
	 * Note that R'est'size is used to store shareable strings from the
	 * 'g'lobal P-1.2001 headers.
	 */
#ifdef	CPIO_ONLY
	mp->rsize = addsize - sizeof (m_head) - TBLOCK;
#else
	mp->rsize = addsize - sizeof (m_head) - obs;
#endif
	if (multivol && cflag) {
		/*
		 * Subtract bitmap size from restsize and set up bitmap pointer
		 */
		mp->bmlast = (fs/TBLOCK) - 1;
		mp->rsize -= bitstr_size(fs/TBLOCK);
		mp->bmap  = (bitstr_t *)
				(((char *)(&(&mp->ginfo)[1])) + mp->rsize);
	}
	gip = &mp->ginfo;

	fifo_setparams();

	if (pipe(mp->gp) < 0)
		comerr("Cannot create get pipe\n");
	if (pipe(mp->pp) < 0)
		comerr("Cannot create put pipe\n");

#ifdef	F_SETFD
	/*
	 * Set close on exec() flag so the compress program
	 * or other programs will not inherit our pipes.
	 */
	fcntl(mp->gp[0], F_SETFD, FD_CLOEXEC);
	fcntl(mp->gp[1], F_SETFD, FD_CLOEXEC);
	fcntl(mp->pp[0], F_SETFD, FD_CLOEXEC);
	fcntl(mp->pp[1], F_SETFD, FD_CLOEXEC);
#endif

	mp->putptr = mp->getptr = mp->base;
	fifo_prmp(0);			/* Print FIFO information with -debug */
	{
		/* Temporary until all modules know about mp->xxx */
		extern int	bufsize;
		extern char	*bigbuf;
		extern char	*bigptr;

		bufsize = mp->size;
		bigptr = bigbuf = mp->base;
	}
#ifdef	HANG_DEBUG
#ifdef	SIGRTMAX
	if (signal(SIGRTMAX, SIG_IGN) != SIG_IGN)
		set_signal(SIGRTMAX, fifo_prmp);
#endif
#endif
}

LOCAL void
fifo_setparams()
{
	if (mp == NULL) {
		comerrno(EX_BAD, "Panic: NULL fifo parameter structure.\n");
		/* NOTREACHED */
		return;
	}
	mp->end = &mp->base[fs];
	fillbytes(mp->end, 10, 'U');	/* Mark llitos() overshoot reserve. */
	mp->size = fs;
	mp->ibs = ibs;
	mp->obs = obs;
	if (mp->bmap)
		mp->bmlast = (fs/TBLOCK) - 1;

	if (hiw)
		mp->hiw = hiw;
	else
		mp->hiw = mp->size / 3 * 2;
	if (low)
		mp->low = low;
	else
		mp->low = mp->size / 3;
	if (mp->low < mp->obs)
		mp->low = mp->obs;
	if (mp->low > mp->hiw)
		mp->low = mp->hiw;

	if (ibs == 0 || mp->ibs > mp->size)
		mp->ibs = mp->size;
}

EXPORT void
fifo_ibs_shrink(newsize)
	int	newsize;
{
	ibs = newsize;
	fs = (fs/newsize)*newsize;
	fifo_setparams();
}

/*
 * fork() and run FIFO background process in forked child.
 *
 * The main star process is always in the foreground.
 * The tape process is always in the background.
 *
 * Tape -> fifo -> star
 * star -> fifo -> star	-copy	flag
 * star -> fifo -> Tape	-c	flag
 *
 * While reading, the star process is the "get" process.
 * While writing, the star process is the "put" process.
 */
EXPORT void
runfifo(ac, av)
	int		ac;
	char	*const *av;
{
	extern	BOOL	cflag;

	signal(SIGPIPE, SIG_IGN);

	if ((pid = fork()) < 0)
		comerr("Cannot fork.\n");

#ifdef USE_OS2SHM
	if (pid == 0)
		DosGetSharedMem(buf, 3);	/* PAG_READ|PAG_WRITE */
#endif
#ifdef	USE_BEOS_AREAS
	if (pid == 0)
		beosshm_child();
#endif

#ifdef	FIFO_STANDALONE
	if (pid != 0) {
		EDEBUG(("Get prozess: pid: %d\n", pid));
		/* Get Prozess */
		(void) close(mp->gpout);
		(void) close(mp->ppin);
		do_out();
	} else {
		EDEBUG(("Put prozess: pid: %d\n", pid));
		/* Put Prozess */
		(void) close(mp->gpin);
		(void) close(mp->ppout);
		do_in();
	}
	return;
#else
	if ((pid != 0) ^ cflag) {
		EDEBUG(("Get prozess: cflag: %d pid: %d\n", cflag, pid));
		/* Get Prozess */
		(void) close(mp->gpout);
		(void) close(mp->ppin);
	} else {
		EDEBUG(("Put prozess: cflag: %d pid: %d\n", cflag, pid));
		/* Put Prozess */
		(void) close(mp->gpin);
		(void) close(mp->ppout);
	}

	if (pid == 0) {
		/*
		 * The background process
		 */
		if (copyflag) {
			mp->ibs = mp->size;
			mp->obs = mp->size;

			copy_create(ac, av);
		} else if (cflag) {	/* In create mode .... */
			/*
			 * FIFO -> Tape (Get side)
			 */
			mp->ibs = mp->size;
			mp->obs = bs;
			do_out();	/* Write archive in background */
		} else {
			/*
			 * Tape -> FIFO (Put side)
			 */
			mp->gflags |= FIFO_IWAIT;
			mp->ibs = bs;
			mp->obs = mp->size;
			do_in();	/* Extract mode: read archive in bg. */
		}
#ifdef	USE_OS2SHM
		DosFreeMem(buf);
#ifdef	__never__
		sleep(30000);	/* XXX If calling _exit() here the parent */
				/* XXX process seems to be blocked.	  */
				/* XXX This should be fixed soon.	  */
#endif
#endif
		exit(0);
		/* NOTREACHED */
	} else {
		extern	FILE	*tarf;

		if (tarf)
			fclose(tarf);
		/*
		 * Here we return from the FIFO set up to the foreground
		 * TAR process.
		 */
	}
#endif
}

#ifdef	HANG_DEBUG
LOCAL char *
prgflags(f, fbuf)
	int	f;
	char	*fbuf;
{
	char	*p = fbuf;

	*p = '\0';
	if (f)
		*p++ = '\t';

	if (f & FIFO_MERROR) {
		strcpy(p, "FIFO_MERROR");
		p += 11;
	}
	if (f & FIFO_IWAIT) {
		if (p > &fbuf[1])
			*p++ = '|';
		strcpy(p, "FIFO_IWAIT");
		p += 10;
	}
	if (f & FIFO_I_CHREEL) {
		if (p > &fbuf[1])
			*p++ = '|';
		strcpy(p, "FIFO_I_CHREEL");
	}
	return (fbuf);
}

LOCAL char *
preflags(f, fbuf)
	int	f;
	char	*fbuf;
{
	char	*p = fbuf;

	*p = '\0';
	if (f)
		*p++ = '\t';

	if (f & FIFO_EXIT) {
		strcpy(p, "FIFO_EXIT");
		p += 9;
	}
	if (f & FIFO_EXERRNO) {
		if (p > &fbuf[1])
			*p++ = '|';
		strcpy(p, "FIFO_EXERRNO");
	}
	return (fbuf);
}

LOCAL char *
prpflags(f, fbuf)
	int	f;
	char	*fbuf;
{
	char	*p = fbuf;

	*p = '\0';
	if (f)
		*p++ = '\t';

	if (f & FIFO_MEOF) {
		strcpy(p, "FIFO_MEOF");
		p += 9;
	}
	if (f & FIFO_O_CHREEL) {
		if (p > &fbuf[1])
			*p++ = '|';
		strcpy(p, "FIFO_O_CHREEL");
	}
	return (fbuf);
}
#endif

EXPORT void
fifo_prmp(sig)
	int	sig;
{
#ifdef	HANG_DEBUG
	char	fbuf[100];
extern	BOOL	cflag;

	if (sig == 0 && !debug)
		return;

	error("pid:      %ld (%ld) copy %d cflag %d\n",
					(long)getpid(), (long)pid,
					copyflag, cflag);
	error("waitchan: %d\n", waitchan);
	error("putptr:   %p\n", mp->putptr);
	error("getptr:   %p\n", mp->getptr);
	error("base:     %p\n", mp->base);
	error("end:      %p\n", mp->end);
	error("size:     %d\n", mp->size);
	error("ibs:      %d\n", mp->ibs);
	error("obs:      %d\n", mp->obs);
	error("amt:      %ld\n", FIFO_AMOUNT(mp));
	error("icnt:     %ld\n", mp->icnt);
	error("ocnt:     %ld\n", mp->ocnt);
	error("iblocked: %d\n", mp->iblocked);
	error("oblocked: %d\n", mp->oblocked);
	error("mayoblock:%d\n", mp->mayoblock);
	error("m1:       %d\n", mp->m1);
	error("m2:       %d\n", mp->m2);
	error("chreel:   %d\n", mp->chreel);
	error("reelwait: %d\n", mp->reelwait);
	error("eflags:   %2.2X%s\n", mp->eflags, preflags(mp->eflags, fbuf));
	error("pflags:   %2.2X%s\n", mp->pflags, prpflags(mp->pflags, fbuf));
	error("flags:    %2.2X%s\n", mp->gflags, prgflags(mp->gflags, fbuf));
	error("hiw:      %d\n", mp->hiw);
	error("low:      %d\n", mp->low);
	error("puts:     %d\n", mp->puts);
	error("gets:     %d\n", mp->gets);
	error("empty:    %d\n", mp->empty);
	error("full:     %d\n", mp->full);
	error("maxfill:  %d\n", mp->maxfill);
	error("moves:    %d\n", mp->moves);
	error("mbytes:   %lld\n", mp->mbytes);
#ifdef	TEST
	error("wpin:     %d\n", mp->wpin);
	error("wpout:    %d\n", mp->wpout);
	error("rpin:     %d\n", mp->rpin);
	error("rpout:    %d\n", mp->rpout);
#endif
#endif	/* HANG_DEBUG */
}

EXPORT void
fifo_stats()
{
	if (no_stats)
		return;

	errmsgno(EX_BAD, "fifo had %d puts %d gets.\n", mp->puts, mp->gets);
	errmsgno(EX_BAD, "fifo was %d times empty and %d times full.\n",
						mp->empty, mp->full);
	errmsgno(EX_BAD, "fifo held %d bytes max, size was %d bytes\n",
						mp->maxfill, mp->size);
	if (mp->moves) {
		errmsgno(EX_BAD,
			"fifo had %d moves, total of %lld moved bytes\n",
						mp->moves, mp->mbytes);
	}
	if (((mp->pflags & FIFO_MEOF) == 0 &&
	    (mp->eflags & FIFO_EXIT) == 0) ||
	    FIFO_AMOUNT(mp) > 0) {
		errmsgno(EX_BAD, "fifo is %lld%% full (%luk), size %dk.\n",
				(Llong)FIFO_AMOUNT(mp) * (Llong)100 / mp->size,
				FIFO_AMOUNT(mp)/1024, mp->size/1024);
	}
}


/*
 * Semaphore wait
 */
LOCAL int
swait(f, chan)
	int	f;
	int	chan;
{
		int	ret;
		int	err = 0;
	unsigned char	c = 0;

	waitchan = chan;
	seterrno(0);
	do {
		ret = read(f, &c, 1);
	} while (ret < 0 && geterrno() == EINTR);
	waitchan = 0;
	if (ret < 0)
		err = geterrno();

	if ((mp->pflags & FIFO_MEOF) && (ret == 0)) {
		/*
		 * We come here in case that the other process that should send
		 * a wakeup died without sending the wakeup because a context
		 * switch on the waiting process (us) prevented us to set the
		 * wait flag in time. This is not a problem since we wake up
		 * from the EOF condition on the sync pipe. We behave as if we
		 * received a normal wakeup byte.
		 */
#ifdef	FIFO_EOF_DEBUG
extern	BOOL	cflag;
		errmsg("Emulate received EOF wakeup on %s side.\n",
			((pid != 0) ^ cflag)? "get": "put");
#endif
		return ((int)c);
	}
	if (ret <= 0) {
		/*
		 * If pid != 0, this is the foreground process
		 */
		if (ret < 0 || (mp->eflags & FIFO_EXIT) == 0) {
			errmsg(
			"Sync pipe read error pid %d ret %d\n",
				pid, ret);
			errmsg("Ib %d Ob %d e %X p %X g %X chan %d.\n",
				mp->iblocked, mp->oblocked,
				mp->eflags, mp->pflags, mp->gflags,
				chan);
		}
		if ((mp->eflags & FIFO_EXERRNO) != 0) {
			/*
			 * A previous error was seen, keep it.
			 */
			ret = mp->ferrno;
		} else {
			/*
			 * Recent sync pipe read error.
			 * Signal error to forground process.
			 */
			mp->eflags |= FIFO_EXERRNO;
			if (ret == 0) {
				errmsgno(err,
					"Sync pipe EOF error pid %d ret %d\n",
					pid, ret);
			}
			if (err)
				ret = err;
			else
				ret = EX_BAD;
			mp->ferrno = ret;
		}
		exprstats(ret);
		/* NOTREACHED */
	}
	return ((int)c);
}

/*
 * Semaphore wakeup
 */
LOCAL int
swakeup(f, c)
	int	f;
	char	c;
{
	return (write(f, &c, 1));
}

#define	sgetwait(m, w)		swait((m)->gpin, w)	/* Wait in get side */
#define	sgetwakeup(m, c)	swakeup((m)->gpout, (c)) /* Wakeup get side */

#define	sputwait(m, w)		swait((m)->ppin, w)	/* Wait in put side */
#define	sputwakeup(m, c)	swakeup((m)->ppout, (c)) /* Wakeup put side */

/*
 * Return the amount of data from the FIFO available to the reader process.
 */
EXPORT int
fifo_amount()
{
	return (FIFO_AMOUNT(mp));
}


/*
 * Data -> FIFO (Put side)
 *
 * wait until at least amount bytes may be put into the fifo.
 *
 * If we are in "-c reate" mode, it is used by the tar process that reads files
 * If we are in "-x tract" mode, it is used by the process that reads the tape
 */
EXPORT int
fifo_iwait(amount)
	int	amount;
{
	register int	cnt;
	register m_head *rmp = mp;

	if (rmp->chreel) {	/* Block FIFO to allow to change reel */
		EDEBUG(("C"));
		rmp->reelwait = TRUE;
		sputwait(rmp, 1);
	}
	if (rmp->pflags & FIFO_MEOF) {
		EDEBUG(("E"));
		cnt = sputwait(rmp, 2);
		/*
		 * 'n' is from fifo_chitape(), '\0' is the final FIFO shutdown.
		 */
		if (cnt != 'n' &&
		    !((rmp->eflags & FIFO_EXIT) && cnt == '\0')) {
			errmsgno(EX_BAD,
			"Implementation botch: with FIFO_MEOF\n");
#ifdef	HANG_DEBUG
			errmsgno(EX_BAD,
			    "Pid %ld expect 'n' eflags %x pflags %x gflags %x\n",
			    (long)getpid(),
			    rmp->eflags, rmp->pflags, rmp->gflags);
			fifo_prmp(-1);
#endif
			comerrno(EX_BAD,
			"Did not wake up from fifo_chitape() - got '%c'.\n",
				cnt);
		}
		if (rmp->gflags & FIFO_I_CHREEL) {
			changetape(TRUE);
			rmp->gflags &= ~FIFO_I_CHREEL;
			rmp->pflags &= ~FIFO_MEOF;
			EDEBUG(("t"));
			sgetwakeup(rmp, 't');
		} else {
			return (-1);
		}
	}
	while ((cnt = rmp->size - FIFO_AMOUNT(rmp)) < amount) {
		/*
		 * Wait until "amount" data fits into the fifo.
		 */
		if (rmp->gflags & FIFO_MERROR) {
			fifo_stats();
			exit(1);
		}
		rmp->full++;
		rmp->iblocked = TRUE;	/* Wait for space to become free */
		EDEBUG(("i"));
		sputwait(rmp, 3);
	}
	if (cnt > rmp->ibs)
		cnt = rmp->ibs;
	if ((rmp->end - rmp->putptr) < cnt) {
		EDEBUG(("at end: cnt: %d max: %d\n",
						cnt, rmp->end - rmp->putptr));
		cnt = rmp->end - rmp->putptr;
	}
	{
		/* Temporary until all modules know about mp->xxx */
		extern char *bigptr;

		bigptr = rmp->putptr;
	}
	return (cnt);
}


/*
 * Data -> FIFO (Put side)
 *
 * add amount bytes to putcount and wake up get side if necessary
 */
EXPORT void
fifo_owake(amount)
	int	amount;
{
	register m_head *rmp = mp;
	register int	iwait;

	if (amount <= 0)
		return;
	rmp->puts++;
	rmp->putptr += amount;
	rmp->icnt += amount;
	if (rmp->putptr >= rmp->end)
		rmp->putptr = rmp->base;

	/*
	 * If FIFO_IWAIT was set, we always need to call sputwait(rmp, 4); as we
	 * always get a related wakeup. Since fifo_resume() clears that flag
	 * before sending the wakeup, we need to evaluate FIFO_IWAIT only once.
	 */
	iwait = rmp->gflags & FIFO_IWAIT;
	if (rmp->oblocked && (iwait || (FIFO_AMOUNT(rmp) >= rmp->low))) {
		/*
		 * Reset oblocked to make sure we send just one single
		 * weakup event
		 */
		rmp->oblocked = FALSE;
		EDEBUG(("d"));
		sgetwakeup(rmp, 'd');
	}
	/*
	 * This is when the get side of the FIFO examins the data to decide
	 * e.g. whether the data needs to be swapped. If we did send the
	 * wakeup above, we always need to wait for a related wakup that
	 * permits us to continue.
	 */
	if (iwait) {
		EDEBUG(("I"));
		sputwait(rmp, 4);
		/*
		 * Set up shadow properties for this proc.
		 */
		setprops(stats->hdrtype);
	}
}


/*
 * Data -> FIFO (Put side)
 *
 * send EOF condition to get side
 */
EXPORT void
fifo_oflush()
{
	mp->pflags |= FIFO_MEOF;
	/*
	 * Make us immune gainst against delays caused by a context switch at
	 * the other side. usleep(1000) will usually not wait at all but may
	 * yield to a context switch.
	 */
	while (mp->mayoblock && !mp->oblocked)
		usleep(1000);

	if (mp->oblocked) {
		/*
		 * Reset oblocked to make sure we send just one single
		 * weakup event
		 */
		mp->oblocked = FALSE;
		EDEBUG(("e"));
		sgetwakeup(mp, 'e');
	}
}


/*
 * Data -> FIFO (Put side)
 *
 * final close of sync pipe
 */
EXPORT void
fifo_oclose()
{
	/*
	 * Close the pipe that is used to wakeup the Get side that might be
	 * waiting but we did not notice that mp->oblocked was set because
	 * it happened too late for us.
	 * Closing the pipe is an alternate way to wake up the Get side.
	 */
	fifo_exit(0);
}


/*
 * FIFO -> Data (Get side)
 *
 * wait until at least obs bytes may be taken out of fifo
 *
 * If we are in "-c reate" mode, it is used by the process that writes the tape
 * If we are in "-x tract" mode, it is used by the tar process that writes files
 */
EXPORT int
fifo_owait(amount)
	int	amount;
{
	int	c;
	register int	cnt;
	register m_head *rmp = mp;

again:
	/*
	 * We need to check rmp->pflags & FIFO_MEOF first, because FIFO_AMOUNT()
	 * gets updated before FIFO_MEOF.
	 * If we did first check FIFO_AMOUNT(), we could get 0 and a context
	 * switch after that and after being continued we could get FIFO_MEOF,
	 * failing to notice that there was a content update meanwhile.
	 */
	if (rmp->pflags & FIFO_MEOF) {
		cnt = FIFO_AMOUNT(rmp);
		if (cnt == 0) {
			return (cnt);
		}
	}

	/*
	 * We need to check rmp->pflags & FIFO_MEOF first, because FIFO_AMOUNT()
	 * gets updated before FIFO_MEOF.
	 * rmp->mayoblock is used to mark this block to avoid deadlocks.
	 */
	rmp->mayoblock = TRUE;
	if ((rmp->pflags & (FIFO_MEOF|FIFO_O_CHREEL)) == 0) {
		cnt = FIFO_AMOUNT(rmp);
		if (cnt < amount) {
			/*
			 * If a context switch happens here, we may get a EOF
			 * condition while reading from the sync pipe because
			 * the other process did already fill up the last chunk
			 * into the FIFO and called exit(). There may be no way
			 * to detect that we expect a wakeup as rmp->oblocked
			 * may be set after the Put process exited.
			 */
			if (rmp->pflags & (FIFO_MEOF|FIFO_O_CHREEL)) {
				/*
				 * There was a context switch between the flag
				 * check and the amount comutation. The related
				 * delay changed the state.
				 */
				rmp->mayoblock = FALSE;
				goto again;
			}
			rmp->empty++;
			rmp->oblocked = TRUE;
			rmp->mayoblock = FALSE;
			EDEBUG(("o"));
			c = sgetwait(rmp, 5);
		}
	}
	rmp->mayoblock = FALSE;

	if (rmp->pflags & FIFO_O_CHREEL) {
		cnt = FIFO_AMOUNT(rmp);
		if (cnt == 0) {
			changetape(TRUE);
			rmp->pflags &= ~FIFO_O_CHREEL;
			EDEBUG(("T"));
			sputwakeup(mp, 'T');
			goto again;
		}
	}
	cnt = FIFO_AMOUNT(rmp);

	if (rmp->maxfill < cnt)
		rmp->maxfill = cnt;

	if (cnt > rmp->obs)
		cnt = rmp->obs;

	c = rmp->end - rmp->getptr;	/* Compute max. contig. content */
#ifdef	CPIO_ONLY
	if (c < TBLOCK && c < cnt) {	/* XXX Check for c < amount too? */
#else
	if (c < obs && c < cnt) {	/* Check for initial obs val */
#endif
		/*
		 * A left over residual at the end of the FIFO is smaller
		 * than 512 bytes (CPIO_ONLY) or 'obs' (auto-detect multivol
		 * split) and there is additional data at the beginning
		 * of the FIFO.
		 * Move the residual before the beginning of the FIFO to make
		 * the content continuous.
		 */
		char *p;

		p = rmp->base - c;
		movebytes(rmp->getptr, p, c);
		rmp->moves++;
		rmp->mbytes += c;
		rmp->getptr = p;
		c = rmp->end - rmp->getptr;
	}
	/*
	 * If there is more data in the FIFO than from the get ptr to the end
	 * of the FIFO and this is still more than the requested data, reduce
	 * "cnt" to what can be read in a single transfer.
	 */
	if (cnt > c && c >= amount)
		cnt = c;

	if (rmp->getptr + cnt > rmp->end) {
		errmsgno(EX_BAD, "getptr >: %p %p %d end: %p\n",
				(void *)rmp->getptr, (void *)&rmp->getptr[cnt],
				cnt, (void *)rmp->end);
	}
	{
		/* Temporary until all modules know about mp->xxx */
		extern char *bigptr;

		bigptr = rmp->getptr;
	}
	return (cnt);
}


/*
 * FIFO -> Data (Get side)
 *
 * add amount bytes to getcount and wake up put side if necessary
 */
EXPORT void
fifo_iwake(amt)
	int	amt;
{
	register m_head *rmp = mp;

	if (amt <= 0) {
		rmp->gflags |= FIFO_MERROR;
		exit(1);
	}

	rmp->gets++;
	rmp->getptr += amt;
	rmp->ocnt += amt;

	if (rmp->getptr >= rmp->end)
		rmp->getptr = rmp->base;

	/*
	 * If rmp->iblocked is TRUE, the put side definitely waits for space.
	 */
	if (rmp->iblocked && (FIFO_AMOUNT(rmp) <= rmp->hiw)) {
		/*
		 * Reset iblocked to make sure we send just one single
		 * weakup event
		 */
		rmp->iblocked = FALSE;
		EDEBUG(("s"));
		sputwakeup(rmp, 's');
	}
}

/*
 * FIFO -> Data (Get side)
 *
 * Wake up the put side in case it is wating on rmp->reelwait
 */
EXPORT void
fifo_reelwake()
{
	register m_head *rmp = mp;

	if (rmp->reelwait) {
		/*
		 * Reset reelwait to make sure we send just one single
		 * weakup event
		 */
		rmp->reelwait = FALSE;
		EDEBUG(("R"));
		sputwakeup(rmp, 'R');
	}
}

/*
 * FIFO -> Data (Get side)
 *
 * Resume FIFO, this is needed as we now know that we may need to do swap data.
 */
EXPORT void
fifo_resume()
{
	register m_head *rmp = mp;

	if ((rmp->gflags & FIFO_IWAIT) != 0) {
		rmp->gflags &= ~FIFO_IWAIT;
		EDEBUG(("S"));
		sputwakeup(rmp, 'S');
	}
}

/*
 * Data -> FIFO (Put side)
 */
EXPORT void
fifo_sync(size)
	int	size;
{
	register m_head *rmp = mp;
		int	rest = 0;
		int	smax = rmp->end - rmp->putptr;
		int	amt  = FIFO_AMOUNT(rmp);

	if (size) {
		if ((amt % size) != 0)
			rest = size - amt%size;
	} else {
		if ((amt % rmp->obs) != 0)
			rest = rmp->obs - amt%rmp->obs;
	}

	/*
	 * Be careful with the size, as we might have changed mp->obs.
	 */
	if (rest > smax)
		rest = smax;
	fifo_iwait(rest);
	fillbytes(rmp->putptr, rest, '\0');
	fifo_owake(rest);
}

EXPORT int
fifo_errno()
{
	/*
	 * Note that we may be called with fifo not active.
	 */
	if (mp == NULL)
		return (0);

	if ((mp->eflags & FIFO_EXERRNO) != 0)
		return (mp->ferrno);
	return (0);
}

/* ARGSUSED */
EXPORT void
fifo_onexit(err, ignore)
	int	err;
	void	*ignore;
{
	fifo_exit(err);
}

/*
 * Both sides of the FIFO may call this
 */
EXPORT void
fifo_exit(err)
	int	err;
{
	extern	BOOL	cflag;

	/*
	 * Note that we may be called with fifo not active.
	 */
	if (mp == NULL)
		return;

	/*
	 * Tell other side of FIFO to exit().
	 */
	mp->eflags |= FIFO_EXIT;
	if (err != 0) {
		mp->eflags |= FIFO_EXERRNO;
		mp->ferrno = err;
	}

	/*
	 * Wake up other side by closing the sync pipes.
	 */
	if ((pid != 0) ^ cflag) {
		EDEBUG(("Fifo_exit() from get prozess: cflag: %d pid: %d\n", cflag, pid));
		/* Get Prozess */
		(void) close(mp->gpin);
		(void) close(mp->ppout);
	} else {
		EDEBUG(("Fifo_exit() from put prozess: cflag: %d pid: %d\n", cflag, pid));
		/* Put Prozess */
		(void) close(mp->gpout);
		(void) close(mp->ppin);
	}
}

/*
 * FIFO -> Data (Get side)
 *
 * The tar -x/-t process tells the tape process to mount a new volume.
 */
EXPORT void
fifo_chitape()
{
	char	c;

	mp->gflags |= FIFO_I_CHREEL;
	if (mp->pflags & FIFO_MEOF) {
		EDEBUG(("n"));
		sputwakeup(mp, 'n');
	} else {
		comerrno(EX_BAD,
		"Implementation botch: FIFO_MEOF not set in fifo_chitape()\n");
	}
	EDEBUG(("w"));
	c = sgetwait(mp, 6);
	if (c != 't') {
		errmsgno(EX_BAD, "Implementation botch: with FIFO_I_CHREEL\n");
#ifdef	HANG_DEBUG
		errmsgno(EX_BAD,
		    "Pid %ld expect 't' eflags %x pflags %x gflags %x\n",
		    (long)getpid(),
		    mp->eflags, mp->pflags, mp->gflags);
		fifo_prmp(-1);
#endif
		comerrno(EX_BAD,
			"Did not wake up from fifo_iwait() - got '%c'.\n", c);
	}
}

/*
 * Data -> FIFO (Put side)
 *
 * The tar -c process tells the tape process to mount a new volume.
 */
EXPORT void
fifo_chotape()
{
	char	c;

	mp->pflags |= FIFO_O_CHREEL;
	/*
	 * Make us immune gainst against delays caused by a context switch at
	 * the other side. usleep(1000) will usually not wait at all but may
	 * yield to a context switch.
	 */
	while (mp->mayoblock && !mp->oblocked)
		usleep(1000);

	if (mp->oblocked) {
		/*
		 * Reset oblocked to make sure we send just one single
		 * weakup event
		 */
		mp->oblocked = FALSE;
		EDEBUG(("N"));
		sgetwakeup(mp, 'N');
	}
	EDEBUG(("W"));
	c = sputwait(mp, 7);
	if (c != 'T') {
		errmsgno(EX_BAD, "Implementation botch: with FIFO_O_CHREEL\n");
#ifdef	HANG_DEBUG
		errmsgno(EX_BAD,
		    "Pid %ld expect 'T' eflags %x pflags %x gflags %x\n",
		    (long)getpid(),
		    mp->eflags, mp->pflags, mp->gflags);
		fifo_prmp(-1);
#endif
		comerrno(EX_BAD,
			"Did not wake up from fifo_owait() - got '%c'.\n", c);
	}
}

/*
 * Tape -> FIFO (Put/Input side)
 *
 * Tape input process for the FIFO.
 * This process runs in background and fills the FIFO with data from the TAPE.
 * Also wait in fifo_owake() if FIFO_IWAIT and we ware reading the first block.
 */
LOCAL void
do_in()
{
	int	amt;
	int	cnt;
extern	int	tarfindex;

	/*
	 * First start reading to reduce total startup time.
	 */
	cnt = fifo_iwait(mp->ibs);
	amt = readtape(mp->putptr, cnt);
	/*
	 * Wait until the foregound Get/Output process did start to read.
	 * usleep(1000) will usually not wait at all but may yield
	 * to a context switch.
	 */
	for (cnt = 0; cnt < 5000 && mp->oblocked == FALSE; cnt++)
		usleep(1000);
	goto owake;

nextread:
	do {
		cnt = fifo_iwait(mp->ibs);
		amt = readtape(mp->putptr, cnt);
owake:
		fifo_owake(amt);
	} while (amt > 0);

	fifo_oflush();
	if (multivol) {
		/*
		 * In theory, we don't need to test 'multivol' here.
		 * We would die later from EOF in the sync pipe.
		 */
		cnt = fifo_iwait(mp->ibs);	/* Media is changed here */
		if (cnt > 0) {
			int	skip;

			amt = readtape(mp->putptr, cnt);
			while (amt > 0 &&
				!verifyvol(mp->putptr, amt,
							stats->volno, &skip)) {
				changetape(FALSE);
				amt = readtape(mp->putptr, cnt);
			}
			if (skip > 0)
				fifo_iwake(skip*TBLOCK);
			if (amt > 0) {
				fifo_owake(amt);
				goto nextread;
			}
		}
	}
	closetape();
	runnewvolscript(stats->volno+1, tarfindex+1);
}

/*
 * FIFO -> Tape (Get/Output side)
 *
 * Tape output process for the FIFO.
 * This process runs in background and writes to the TAPE using
 * data from the FIFO.
 */
LOCAL void
do_out()
{
	int	cnt;
	int	amt;
extern	int	tarfindex;

	for (;;) {
		cnt = fifo_owait(mp->obs);
		if (cnt == 0)
			break;
nextwrite:
		amt = writetape(mp->getptr, cnt);
		if (amt == -2) {
			changetape(TRUE);
			if ((amt = startvol(mp->getptr, cnt)) <= 0)
				goto nextwrite;
		}
		if (multivol) {
			int	startb = (mp->getptr - mp->base) / TBLOCK;
			int	stopb = startb - 1 + amt / TBLOCK;

			if (mp->getptr < mp->base) {
				stopb = mp->bmlast;
				startb = stopb + 1 - (mp->base - mp->getptr) / TBLOCK;
				if ((mp->getptr + amt) < mp->base)
					stopb = startb - 1 + amt / TBLOCK;
				fbit_nclear(mp->bmap, startb, stopb);
				if ((mp->getptr + amt) < mp->base) {
					startb = stopb = -1;
				} else {
					startb = 0;
					stopb = -1 + (amt - (mp->base - mp->getptr)) / TBLOCK;
				}
			}
			if (startb >= 0)
				fbit_nclear(mp->bmap, startb, stopb);
		}
		fifo_iwake(amt);
	}
	closetape();
	runnewvolscript(stats->volno+1, tarfindex+1);
}

/*
 * Make the macro a function...
 */
LOCAL void
fbit_nclear(name, startb, stopb)
	register bitstr_t *name;
	register int	startb;
	register int	stopb;
{
	bit_nclear(name, startb, stopb);
}

/* -------------------------------------------------------------------------- */
/*
 * Allocate shared memory for the FIFO.
 */

#ifdef	USE_MMAP
LOCAL char *
mkshare(size)
	int	size;
{
	int	f;
	char	*addr;

#ifdef	MAP_ANONYMOUS	/* HP/UX */
	f = -1;
	addr = mmap(0, mmap_sizeparm(size), PROT_READ|PROT_WRITE,
					MAP_SHARED|MAP_ANONYMOUS, f, 0);
#else
	if ((f = open("/dev/zero", O_RDWR)) < 0)
		comerr("Cannot open '/dev/zero'.\n");
	addr = mmap(0, mmap_sizeparm(size), PROT_READ|PROT_WRITE,
					MAP_SHARED, f, 0);
#endif
	if (addr == (char *)-1)
		comerr("Cannot get mmap for %d Bytes on /dev/zero.\n", size);
	if (f >= 0)
		close(f);

	if (debug) {
		errmsgno(EX_BAD,
			"shared memory segment attached at: %p size %d\n",
			(void *)addr, size);
	}

#ifdef	HAVE_MLOCK
	if (getuid() == 0 && mlock(addr, size) < 0)
		errmsg("Cannot lock fifo memory.\n");
#endif

	return (addr);
}
#endif

#ifdef	USE_USGSHM
#include <schily/ipc.h>
#include <schily/shm.h>
LOCAL char *
mkshm(size)
	int	size;
{
	int	id;
	char	*addr;
	/*
	 * Unfortunately, a declaration of shmat() is missing in old
	 * implementations such as AT&T SVr0 and SunOS.
	 * We cannot add this definition here because the return-type
	 * changed on newer systems.
	 *
	 * We will get a warning like this:
	 *
	 * warning: assignment of pointer from integer lacks a cast
	 * or
	 * warning: illegal combination of pointer and integer, op =
	 */
#ifdef	__never__
	extern	char *shmat();
#endif

	if ((id = shmget(IPC_PRIVATE, size, IPC_CREAT|0600)) == -1)
		comerr("shmget failed\n");

	if (debug) {
		errmsgno(EX_BAD,
			"shared memory segment allocated: %d\n", id);
	}

	if ((addr = shmat(id, (char *)0, 0600)) == (char *)-1)
		comerr("shmat failed\n");

	if (debug) {
		errmsgno(EX_BAD,
			"shared memory segment attached at: %p size %d\n",
			(void *)addr, size);
	}

	if (shmctl(id, IPC_RMID, 0) < 0)
		comerr("shmctl failed\n");

#ifdef	SHM_LOCK
	/*
	 * Although SHM_LOCK is standard, it seems that all versions of AIX
	 * ommit this definition.
	 */
	if (getuid() == 0 && shmctl(id, SHM_LOCK, 0) < 0)
		errmsg("shmctl failed to lock shared memory segment\n");
#endif

	return (addr);
}
#endif

#ifdef	USE_OS2SHM
LOCAL char *
mkos2shm(size)
	int	size;
{
	char	*addr;

	/*
	 * The OS/2 implementation of shm (using shm.dll) limits the size of
	 * one shared memory segment to 0x3fa000 (aprox. 4MBytes). Using OS/2
	 * native API we have no such restriction so I decided to use it
	 * allowing fifos of arbitrary size.
	 */
	if (DosAllocSharedMem(&addr, NULL, size, 0X100L | 0x1L | 0x2L | 0x10L))
		comerr("DosAllocSharedMem() failed\n");

	if (debug) {
		errmsgno(EX_BAD,
			"shared memory allocated attached at: %p size %d\n",
			(void *)addr, size);
	}

	return (addr);
}
#endif

#ifdef	USE_BEOS_AREAS
LOCAL	area_id	fifo_aid;
LOCAL	void	*fifo_addr;
LOCAL	char	fifo_name[32];

LOCAL char *
mkbeosshm(size)
	int	size;
{
	snprintf(fifo_name, sizeof (fifo_name), "star FIFO %lld",
		(Llong)getpid());

	fifo_aid = create_area(fifo_name, &fifo_addr,
			B_ANY_ADDRESS,
			size,
			B_NO_LOCK, B_READ_AREA|B_WRITE_AREA);
	if (fifo_addr == NULL) {
		comerrno(fifo_aid,
			"Cannot get create_area for %d Bytes FIFO.\n", size);
	}
	if (debug) {
		errmsgno(EX_BAD,
			"shared memory allocated attached at: %p size %d\n",
			(void *)fifo_addr, size);
	}

	return (fifo_addr);
}

LOCAL void
beosshm_child()
{
	/*
	 * Delete the area created by fork that is copy-on-write.
	 */
	delete_area(area_for(fifo_addr));
	/*
	 * Clone (share) the original one.
	 * The original implementaion used B_ANY_ADDRESS, but newer Haiku
	 * versions implement address randomization that prevents us from
	 * using the pointer in the child. So we noe use B_EXACT_ADDRESS.
	 */
	fifo_aid = clone_area(fifo_name, &fifo_addr,
			B_EXACT_ADDRESS, B_READ_AREA|B_WRITE_AREA,
			fifo_aid);
	if (buf != fifo_addr) {
		comerrno(EX_BAD, "Panic FIFO addr.\n");
		/* NOTREACHED */
	}
}
#endif

#endif	/* FIFO */
