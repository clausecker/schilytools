/* @(#)fifo.h	1.40 20/07/22 Copyright 1989-2020 J. Schilling */
/*
 *	Definitions for a "fifo" that uses
 *	shared memory between two processes
 *
 *	Copyright (c) 1989-2020 J. Schilling
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

#ifndef	_FIFO_H
#define	_FIFO_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_STANDARD_H
#include <schily/standard.h>
#endif
#ifndef	_BITSTRING_H
#include "bitstring.h"
#endif

#if	defined(HAVE_OS_H) && \
	defined(HAVE_CLONE_AREA) && defined(HAVE_CREATE_AREA) && \
	defined(HAVE_DELETE_AREA)
#include <OS.h>
#define	HAVE_BEOS_AREAS	/* BeOS/Zeta */
#endif
#if	!defined(HAVE_SMMAP) && !defined(HAVE_USGSHM) && \
	!defined(HAVE_DOSALLOCSHAREDMEM) && !defined(HAVE_BEOS_AREAS)
#undef	FIFO			/* We cannot have a FIFO on this platform */
#endif
#if	!defined(HAVE_FORK)
#undef	FIFO			/* We cannot have a FIFO on this platform */
#endif

/*
 * Statistics data that needs to be in shared memory
 */
typedef	struct	{
	BOOL	reading;	/* true if currently reading from tape	    */
	int	swapflg;	/* -1: init, 0: FALSE, 1: TRUE		    */
	int	hdrtype;	/* Hdrtype used for read/write		    */
	int	volno;		/* Volume #				    */
	int	nblocks;	/* Blocksize for each transfer in TBLOCK    */
	long	blocksize;	/* Blocksize for each transfer in bytes	    */
	long	lastsize;	/* Size of last transfer (for backtape)	    */
	Llong	eofblock;	/* EOF block # on Volume		    */
	Llong	blocks;		/* Full blocks transfered on Volume	    */
	Llong	parts;		/* Bytes fom partial transferes on Volume   */
	Llong	Tblocks;	/* Total blocks transfered		    */
	Llong	Tparts;		/* Total Bytes fom partial transferes	    */
	off_t	cur_size;	/* The size of the current file (multivol)  */
	off_t	cur_off;	/* The off for the current file (multivol)  */
	off_t	old_size;	/* The size for the last block write	    */
	off_t	old_off;	/* The off for the last block wrte	    */
} m_stats;

/*
 * Shared data used to control the FIFO.
 *
 * This structure is at the start of the shared memory segment.
 *
 * If the first character in the comment is a P, this is modified by the put
 * side.
 *
 * If the first character in the comment is a G, this is modified by the get
 * side.
 *
 * In order to avoid the need for semaphores to control the change of values
 * in this structure, members marked with "P", are only modified by the
 * put side of the FIFO and members marked with "G" are only modified by the
 * get side of the FIFO.
 *
 * Members marked with "P-" are set by the put side and reset by the get side.
 * Members marked with "G-" are set by the get side and reset by the put side.
 * Since this reset happens while the "set" side did already decide to wait
 * and the reset happens just before the other side decided to wake up the
 * the first side, this is not a problem.
 */
#define	V	volatile
typedef struct {
	char	* V putptr;	/* P  put pointer within shared memory	    */
	char	* V getptr;	/* G  get pointer within shared memory	    */
	char	*base;		/*    fifobase within shared memory segment */
	char	*end;		/*    end of real shared memory segment	    */
	long	size;		/*    fifosize within shared memory segment */
	long	ibs;		/*    input transfer size		    */
	long	obs;		/*    output transfer size		    */
	long	rsize;		/*    restsize between head struct and .base */
	V unsigned long	icnt;	/* P  input count (incremented on each put) */
	V unsigned long	ocnt;	/* G  output count (incremented on each get) */
	V char	iblocked;	/* P- input  (put side) is blocked	    */
	V char	oblocked;	/* G- output (get side) is blocked	    */
	V char	mayoblock;	/* G output (get side) may set oblocked	    */
	V char	m1;		/*    Semaphore claimed by newvolhdr()	    */
	V char	m2;		/*    Semaphore claimed by cr_file()	    */
	V char	chreel;		/*    Semaphore claimed by startvol()	    */
	V char	reelwait;	/* P- input (put side) is blocked on "chreel" */
	V char	eflags;		/*    fifo exit flags			    */
	V char	pflags;		/*    fifo put flags			    */
	V char	gflags;		/*    fifo get flags			    */
				/*    2 or 6 bytes of padding		    */
	V int	ferrno;		/*    errno from fifo background process    */
	long	hiw;		/*    highwater mark			    */
	long	low;		/*    lowwater mark			    */
	int	gp[2];		/*    sync pipe for get process		    */
	int	pp[2];		/*    sync pipe for put process		    */
	V long	puts;		/*    fifo put count statistic		    */
	V long	gets;		/*    fifo get get statistic		    */
	V long	empty;		/*    fifo was empty count statistic	    */
	V long	full;		/*    fifo was full count statistic	    */
	V long	maxfill;	/*    max # of bytes in fifo		    */
	V long	moves;		/*    # of moves of residual bytes	    */
	V Llong	mbytes;		/*    # of residual bytes moved		    */
	m_stats	stats;		/*    statistics			    */
	bitstr_t *bmap;		/*    Bitmap used to store TCB positions    */
	long	bmlast;		/*    Last bits # in use in above Bitmap    */
	GINFO	ginfo;		/*    To share GINFO for P.1-2001 'g' headers */
} m_head;
#undef	V

#define	gpin	gp[0]		/* get pipe in  */
#define	gpout	gp[1]		/* get pipe out */
#define	ppin	pp[0]		/* put pipe in  */
#define	ppout	pp[1]		/* put pipe out */

#define	FIFO_AMOUNT(p)	((p)->icnt - (p)->ocnt)

/*
 * The FIFO flags are used only inside fifo.c
 *
 * gflags:
 * FIFO_MERROR	 set by the get side
 * FIFO_IWAIT	 set by the put side before startup, reset by the get side
 * FIFO_I_CHREEL set by the get side, reset by the put side with get waiting
 *
 * pflags:
 * FIFO_MEOF	 set and reset by the put side
 * FIFO_O_CHREEL set by the put side, reset by the get side with put waiting
 *
 * eflags:
 * FIFO_EXIT	 set by the side that decided to abort the program
 * FIFO_EXERRNO	 set by the side that decided to abort the program
 *
 * If we ever need more than 8 bits, we need to use a larger data type.
 */
#define	FIFO_MERROR	0x001	/* G error on input (get side)	*/

#define	FIFO_IWAIT	0x010	/* G input (put side) wait after first record */
#define	FIFO_I_CHREEL	0x020	/* G change in tape reel if fifo gets empty  */

#define	FIFO_MEOF	0x001	/* P EOF on input (put side)	*/
#define	FIFO_O_CHREEL	0x002	/* P change out tape reel if fifo gets empty */

#define	FIFO_EXIT	0x001	/* E exit() on non tape side	*/
#define	FIFO_EXERRNO	0x002	/* E errno from non tape side	*/

#ifdef	FIFO
/*
 * Critical section handling for multi volume support.
 *
 * This code is used to protect access to stat->cur_size & stat->cur_off
 * when preparing the multi volume header. Calling blocking code from
 * within a critical section is not permitted. For this reason, and because
 * this only may block when a media change is done, the delay time is not
 * important or critical.
 */

#define	fifo_enter_critical()	if (use_fifo) { \
				extern  m_head  *mp;		\
								\
				while (mp->m1)			\
					usleep(100000);		\
				mp->m2 = TRUE;			\
			}

#define	fifo_leave_critical()	if (use_fifo) { \
				extern  m_head  *mp;		\
								\
				mp->m2 = FALSE;			\
			}

#define	fifo_lock_critical()	if (use_fifo) { \
				extern  m_head  *mp;		\
								\
				mp->m1 = TRUE;			\
				while (mp->m2)			\
					usleep(100000);		\
			}

#define	fifo_unlock_critical()	if (use_fifo) { \
				extern  m_head  *mp;		\
								\
				mp->m1 = FALSE;			\
			}
#else
#define	fifo_enter_critical()
#define	fifo_leave_critical()
#define	fifo_lock_critical()
#define	fifo_unlock_critical()
#endif

/*
 * we always need this external variable.
 */
extern	BOOL	use_fifo;

#endif /* _FIFO_H */
