/* @(#)fifo.c	1.72 09/11/28 Copyright 1989, 1994-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fifo.c	1.72 09/11/28 Copyright 1989, 1994-2009 J. Schilling";
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
 *
 *	Copyright (c) 1989, 1994-2009 J. Schilling
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

/*#define	DEBUG*/

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* includes <sys/types.h> */
#include <schily/libport.h>	/* getpagesize() */
#include <schily/fcntl.h>
#include <schily/standard.h>
#include <schily/errno.h>
#include "star.h"
#include "fifo.h"	/* #undef FIFO may happen here */
#include <schily/schily.h>
#include "starsubs.h"

#ifdef	FIFO

#if !defined(USE_MMAP) && !defined(USE_USGSHM)
#define	USE_MMAP
#endif
#if defined(HAVE_SMMAP) && defined(USE_MMAP)
#include <schily/mman.h>
#endif

#ifndef	HAVE_SMMAP
#	undef	USE_MMAP
#	define	USE_USGSHM	/* SYSV shared memory is the default */
#endif

#ifdef	HAVE_DOSALLOCSHAREDMEM	/* This is for OS/2 */
#	undef	USE_MMAP
#	undef	USE_USGSHM
#	define	USE_OS2SHM
#endif

#ifdef	HAVE_BEOS_AREAS		/* This is for BeOS/Zeta */
#	undef	USE_MMAP
#	undef	USE_USGSHM
#	undef	USE_OS2SHM
#	define	USE_BEOS_AREAS
#endif

#ifdef DEBUG
#define	EDEBUG(a)	if (debug) error a
#else
#define	EDEBUG(a)
#endif

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

EXPORT	void	initfifo	__PR((void));
LOCAL	void	fifo_setparams	__PR((void));
EXPORT	void	fifo_ibs_shrink	__PR((int newsize));
EXPORT	void	runfifo		__PR((int ac, char *const *av));
LOCAL	void	prmp		__PR((void));
EXPORT	void	fifo_stats	__PR((void));
LOCAL	int	swait		__PR((int f));
LOCAL	int	swakeup		__PR((int f, int c));
EXPORT	int	fifo_amount	__PR((void));
EXPORT	int	fifo_iwait	__PR((int amount));
EXPORT	void	fifo_owake	__PR((int amount));
EXPORT	void	fifo_oflush	__PR((void));
EXPORT	int	fifo_owait	__PR((int amount));
EXPORT	void	fifo_iwake	__PR((int amt));
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
		fs = 8*1024*1024;
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
	mp = (m_head *)buf;
	fillbytes(buf, addsize, '\0');	/* We init the complete add. space */
	stats = &mp->stats;
	stats->hdrtype = chdrtype;
	mp->base = &buf[addsize];
	mp->iblocked = FALSE;
	mp->oblocked = FALSE;
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
		mp->bmap  = (bitstr_t *)(((char *)(&(&mp->ginfo)[1])) + mp->rsize);
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
	 * or other programs will not inherit out pipes.
	 */
	fcntl(mp->gp[0], F_SETFD, 1);
	fcntl(mp->gp[1], F_SETFD, 1);
	fcntl(mp->pp[0], F_SETFD, 1);
	fcntl(mp->pp[1], F_SETFD, 1);
#endif

	mp->putptr = mp->getptr = mp->base;
	prmp();
	{
		/* Temporary until all modules know about mp->xxx */
		extern int	bufsize;
		extern char	*bigbuf;
		extern char	*bigptr;

		bufsize = mp->size;
		bigptr = bigbuf = mp->base;
	}
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

/*---------------------------------------------------------------------------
|
| Der eigentliche star Prozess ist immer im Vordergrund.
| Der Band Prozess ist immer im Hintergrund.
|
| Band -> fifo -> star
| star -> fifo -> star	-copy	flag
| star -> fifo -> Band	-c	flag
|
| Beim Lesen ist der star Prozess der get Prozess.
| Beim Schreiben ist der star Prozess der put Prozess.
|
+---------------------------------------------------------------------------*/

EXPORT void
runfifo(ac, av)
	int		ac;
	char	*const *av;
{
	extern	BOOL	cflag;

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

	if ((pid != 0) ^ cflag) {
		EDEBUG(("Get prozess: cflag: %d pid: %d\n", cflag, pid));
		/* Get Prozess */
		close(mp->gpout);
		close(mp->ppin);
	} else {
		EDEBUG(("Put prozess: cflag: %d pid: %d\n", cflag, pid));
		/* Put Prozess */
		close(mp->gpin);
		close(mp->ppout);
	}

	if (pid == 0) {
		/*
		 * The background process
		 */
		if (copyflag) {
			mp->ibs = mp->size;
			mp->obs = mp->size;

			copy_create(ac, av);
		} else if (cflag) {
			mp->ibs = mp->size;
			mp->obs = bs;
			do_out();
		} else {
			mp->flags |= FIFO_IWAIT;
			mp->ibs = bs;
			mp->obs = mp->size;
			do_in();
		}
#ifdef	USE_OS2SHM
		DosFreeMem(buf);
#ifdef	__needed__
		sleep(30000);	/* XXX If calling _exit() here the parent process seems to be blocked */
				/* XXX This should be fixed soon */
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
}

LOCAL void
prmp()
{
#ifdef	DEBUG
	if (!debug)
		return;
#ifdef	TEST
	error("putptr: %p\n", mp->putptr);
	error("getptr: %p\n", mp->getptr);
#endif
	error("base:  %p\n", mp->base);
	error("end:   %p\n", mp->end);
	error("size:  %d\n", mp->size);
	error("ibs:   %d\n", mp->ibs);
	error("obs:   %d\n", mp->obs);
	error("amt:   %ld\n", FIFO_AMOUNT(mp));
	error("hiw:   %d\n", mp->hiw);
	error("low:   %d\n", mp->low);
	error("flags: %X\n", mp->flags);
#ifdef	TEST
	error("wpin:  %d\n", mp->wpin);
	error("wpout: %d\n", mp->wpout);
	error("rpin:  %d\n", mp->rpin);
	error("rpout: %d\n", mp->rpout);
#endif
#endif	/* DEBUG */
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
	if ((mp->flags & (FIFO_MEOF|FIFO_EXIT)) == 0 || FIFO_AMOUNT(mp) > 0) {
		errmsgno(EX_BAD, "fifo is %lld%% full (%luk), size %dk.\n",
				(Llong)FIFO_AMOUNT(mp) * (Llong)100 / mp->size,
				FIFO_AMOUNT(mp)/1024, mp->size/1024);
	}
}


/*---------------------------------------------------------------------------
|
| Semaphore wait
|
+---------------------------------------------------------------------------*/
LOCAL int
swait(f)
	int	f;
{
		int	ret;
	unsigned char	c;

	seterrno(0);
	do {
		ret = read(f, &c, 1);
	} while (ret < 0 && geterrno() == EINTR);
	if (ret < 0 || (ret == 0 && pid)) {
		/*
		 * If pid != 0, this is the foreground process
		 */
		if ((mp->flags & FIFO_EXIT) == 0)
			errmsg("Sync pipe read error on pid %d flags 0x%X.\n", pid, mp->flags);
		if ((mp->flags & FIFO_EXERRNO) != 0)
			ret = mp->ferrno;
		else
			ret = 1;
		exprstats(ret);
		/* NOTREACHED */
	}
	if (ret == 0) {
		/*
		 * this is the background process!
		 */
		exit(0);
	}
	return ((int)c);
}

/*---------------------------------------------------------------------------
|
| Semaphore wakeup
|
+---------------------------------------------------------------------------*/
LOCAL int
swakeup(f, c)
	int	f;
	char	c;
{
	return (write(f, &c, 1));
}

#define	sgetwait(m)		swait((m)->gpin)
#define	sgetwakeup(m, c)	swakeup((m)->gpout, (c))

#define	sputwait(m)		swait((m)->ppin)
#define	sputwakeup(m, c)	swakeup((m)->ppout, (c))

EXPORT int
fifo_amount()
{
	return (FIFO_AMOUNT(mp));
}


/*---------------------------------------------------------------------------
|
| wait until at least amount bytes may be put into the fifo
|
+---------------------------------------------------------------------------*/
EXPORT int
fifo_iwait(amount)
	int	amount;
{
	register int	cnt;
	register m_head *rmp = mp;

	if (rmp->flags & FIFO_MEOF) {
		EDEBUG(("E"));
		cnt = sputwait(rmp);
		if (cnt != 'n') {
			errmsgno(EX_BAD,
			"Implementation botch: with FIFO_MEOF\n");
			comerrno(EX_BAD,
			"Did not wake up from fifo_chitape() - got '%c'.\n",
				cnt);
		}
		if (rmp->flags & FIFO_I_CHREEL) {
			changetape(TRUE);
			rmp->flags &= ~FIFO_I_CHREEL;
			rmp->flags &= ~FIFO_MEOF;
			EDEBUG(("t"));
			sgetwakeup(rmp, 't');
		} else {
			return (-1);
		}
	}
	while ((cnt = rmp->size - FIFO_AMOUNT(rmp)) < amount) {
		if (rmp->flags & FIFO_MERROR) {
			fifo_stats();
			exit(1);
		}
		rmp->full++;
		rmp->iblocked = TRUE;
		EDEBUG(("i"));
		sputwait(rmp);
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


/*---------------------------------------------------------------------------
|
| add amount bytes to putcount and wake up get side if necessary
|
+---------------------------------------------------------------------------*/
EXPORT void
fifo_owake(amount)
	int	amount;
{
	register m_head *rmp = mp;

	if (amount <= 0)
		return;
	rmp->puts++;
	rmp->putptr += amount;
	rmp->icnt += amount;
	if (rmp->putptr >= rmp->end)
		rmp->putptr = rmp->base;

	if (rmp->oblocked &&
			((rmp->flags & FIFO_IWAIT) ||
					(FIFO_AMOUNT(rmp) >= rmp->low))) {
		rmp->oblocked = FALSE;
		EDEBUG(("d"));
		sgetwakeup(rmp, 'd');
	}
	if ((rmp->flags & FIFO_IWAIT)) {
		EDEBUG(("I"));
		sputwait(rmp);
		/*
		 * Set up shadow properties for this proc.
		 */
		setprops(stats->hdrtype);
	}
}


/*---------------------------------------------------------------------------
|
| send EOF condition to get side
|
+---------------------------------------------------------------------------*/
EXPORT void
fifo_oflush()
{
	mp->flags |= FIFO_MEOF;
	if (mp->oblocked) {
		mp->oblocked = FALSE;
		EDEBUG(("e"));
		sgetwakeup(mp, 'e');
	}
}

/*---------------------------------------------------------------------------
|
| wait until at least obs bytes may be taken out of fifo
|
+---------------------------------------------------------------------------*/
EXPORT int
fifo_owait(amount)
	int	amount;
{
	int	c;
	register int	cnt;
	register m_head *rmp = mp;

again:
	cnt = FIFO_AMOUNT(rmp);
	if (cnt == 0 && (rmp->flags & FIFO_MEOF))
		return (cnt);

	if (cnt < amount && (rmp->flags & (FIFO_MEOF|FIFO_O_CHREEL)) == 0) {
		rmp->empty++;
		rmp->oblocked = TRUE;
		EDEBUG(("o"));
		c = sgetwait(rmp);
		cnt = FIFO_AMOUNT(rmp);
	}
	if (cnt == 0 && (rmp->flags & FIFO_O_CHREEL)) {
		changetape(TRUE);
		rmp->flags &= ~FIFO_O_CHREEL;
		EDEBUG(("T"));
		sputwakeup(mp, 'T');
		goto again;
	}

	if (rmp->maxfill < cnt)
		rmp->maxfill = cnt;

	if (cnt > rmp->obs)
		cnt = rmp->obs;

	c = rmp->end - rmp->getptr;
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


/*---------------------------------------------------------------------------
|
| add amount bytes to getcount and wake up put side if necessary
|
+---------------------------------------------------------------------------*/
EXPORT void
fifo_iwake(amt)
	int	amt;
{
	register m_head *rmp = mp;

	if (amt <= 0) {
		rmp->flags |= FIFO_MERROR;
		exit(1);
	}

	rmp->gets++;
	rmp->getptr += amt;
	rmp->ocnt += amt;

	if (rmp->getptr >= rmp->end)
		rmp->getptr = rmp->base;

	if ((FIFO_AMOUNT(rmp) <= rmp->hiw) && rmp->iblocked) {
		rmp->iblocked = FALSE;
		EDEBUG(("s"));
		sputwakeup(rmp, 's');
	}
}

EXPORT void
fifo_resume()
{
	register m_head *rmp = mp;

	if ((rmp->flags & FIFO_IWAIT) != 0) {
		rmp->flags &= ~FIFO_IWAIT;
		EDEBUG(("S"));
		sputwakeup(rmp, 'S');
	}
}

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

	if ((mp->flags & FIFO_EXERRNO) != 0)
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
	mp->flags |= FIFO_EXIT;
	if (err != 0) {
		mp->flags |= FIFO_EXERRNO;
		mp->ferrno = err;
	}

	/*
	 * Wake up other side by closing the sync pipes.
	 */
	if ((pid != 0) ^ cflag) {
		EDEBUG(("Fifo_exit() from get prozess: cflag: %d pid: %d\n", cflag, pid));
		/* Get Prozess */
		close(mp->gpin);
		close(mp->ppout);
	} else {
		EDEBUG(("Fifo_exit() from put prozess: cflag: %d pid: %d\n", cflag, pid));
		/* Put Prozess */
		close(mp->gpout);
		close(mp->ppin);
	}
}

EXPORT void
fifo_chitape()
{
	char	c;

	mp->flags |= FIFO_I_CHREEL;
	if (mp->flags & FIFO_MEOF) {
		EDEBUG(("n"));
		sputwakeup(mp, 'n');
	} else {
		comerrno(EX_BAD,
		"Implementation botch: FIFO_MEOF not set in fifo_chitape()\n");
	}
	EDEBUG(("w"));
	c = sgetwait(mp);
}

EXPORT void
fifo_chotape()
{
	char	c;

	mp->flags |= FIFO_O_CHREEL;
	if (mp->oblocked) {
		mp->oblocked = FALSE;
		EDEBUG(("N"));
		sgetwakeup(mp, 'N');
	}
	EDEBUG(("W"));
	c = sputwait(mp);
}

/*
 * Tape -> FIFO
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

nextread:
	do {
		cnt = fifo_iwait(mp->ibs);
		amt = readtape(mp->putptr, cnt);
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
}

/*
 * FIFO -> Tape
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

#ifdef	USE_MMAP
LOCAL char *
mkshare(size)
	int	size;
{
	int	f;
	char	*addr;

#ifdef	MAP_ANONYMOUS	/* HP/UX */
	f = -1;
	addr = mmap(0, mmap_sizeparm(size), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, f, 0);
#else
	if ((f = open("/dev/zero", O_RDWR)) < 0)
		comerr("Cannot open '/dev/zero'.\n");
	addr = mmap(0, mmap_sizeparm(size), PROT_READ|PROT_WRITE, MAP_SHARED, f, 0);
#endif
	if (addr == (char *)-1)
		comerr("Cannot get mmap for %d Bytes on /dev/zero.\n", size);
	if (f >= 0)
		close(f);

	if (debug) errmsgno(EX_BAD, "shared memory segment attached at: %p size %d\n",
				(void *)addr, size);

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
/*	extern	char *shmat();*/

	if ((id = shmget(IPC_PRIVATE, size, IPC_CREAT|0600)) == -1)
		comerr("shmget failed\n");

	if (debug) errmsgno(EX_BAD, "shared memory segment allocated: %d\n", id);

	if ((addr = shmat(id, (char *)0, 0600)) == (char *)-1)
		comerr("shmat failed\n");

	if (debug) errmsgno(EX_BAD, "shared memory segment attached at: %p size %d\n",
				(void *)addr, size);

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
	 * The OS/2 implementation of shm (using shm.dll) limits the size of one shared
	 * memory segment to 0x3fa000 (aprox. 4MBytes). Using OS/2 native API we have
	 * no such restriction so I decided to use it allowing fifos of arbitrary size.
	 */
	if (DosAllocSharedMem(&addr, NULL, size, 0X100L | 0x1L | 0x2L | 0x10L))
		comerr("DosAllocSharedMem() failed\n");

	if (debug) errmsgno(EX_BAD, "shared memory allocated attached at: %p size %d\n",
				(void *)addr, size);

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
	if (debug) errmsgno(EX_BAD, "shared memory allocated attached at: %p size %d\n",
				(void *)fifo_addr, size);

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
	 */
	fifo_aid = clone_area(fifo_name, &fifo_addr,
			B_ANY_ADDRESS, B_READ_AREA|B_WRITE_AREA,
			fifo_aid);
	if (buf != fifo_addr) {
		comerrno(EX_BAD, "Panic FIFO addr.\n");
		/* NOTREACHED */
	}
}
#endif

#endif	/* FIFO */
