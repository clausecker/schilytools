/* @(#)fifo.h	1.31 12/01/01 Copyright 1989-2012 J. Schilling */
/*
 *	Definitions for a "fifo" that uses
 *	shared memory between two processes
 *
 *	Copyright (c) 1989-2012 J. Schilling
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
#	define	HAVE_BEOS_AREAS	/* BeOS/Zeta */
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
 * Shared data used to control the FIFO
 */
typedef struct {
	char	*putptr;	/* put pointer within shared memory	    */
	char	*getptr;	/* get pointer within shared memory	    */
	char	*base;		/* base of fifo within shared memory segment */
	char	*end;		/* end of real shared memory segment	    */
	int	size;		/* size of fifo within shared memory segment */
	int	ibs;		/* input transfer size			    */
	int	obs;		/* output transfer size			    */
	int	rsize;		/* rest size between head struct and .base  */
	unsigned long	icnt;	/* input count (incremented on each put)    */
	unsigned long	ocnt;	/* output count (incremented on each get)   */
	char	iblocked;	/* input  (put side) is blocked		    */
	char	oblocked;	/* output (get side) is blocked		    */
	char	m1;		/* Semaphore claimed by newvolhdr()	    */
	char	m2;		/* Semaphore claimed by cr_file()	    */
	char	chreel;		/* Semaphore claimed by startvol()	    */
	char	reelwait;	/* input  (put side) is blocked on "chreel" */
	int	hiw;		/* highwater mark			    */
	int	low;		/* lowwater mark			    */
	int	flags;		/* fifo flags				    */
	int	ferrno;		/* errno from fifo background process	    */
	int	gp[2];		/* sync pipe for get process		    */
	int	pp[2];		/* sync pipe for put process		    */
	int	puts;		/* fifo put count statistic		    */
	int	gets;		/* fifo get get statistic		    */
	int	empty;		/* fifo was empty count statistic	    */
	int	full;		/* fifo was full count statistic	    */
	int	maxfill;	/* max # of bytes in fifo		    */
	int	moves;		/* # of moves of residual bytes		    */
	Llong	mbytes;		/* # of residual bytes moved		    */
	m_stats	stats;		/* statistics				    */
	bitstr_t *bmap;		/* Bitmap used to store TCB positions	    */
	int	bmlast;		/* Last bits # in use in above Bitmap	    */
	GINFO	ginfo;		/* To share GINFO for P.1-2001 'g' headers  */
} m_head;

#define	gpin	gp[0]		/* get pipe in  */
#define	gpout	gp[1]		/* get pipe out */
#define	ppin	pp[0]		/* put pipe in  */
#define	ppout	pp[1]		/* put pipe out */

#define	FIFO_AMOUNT(p)	((p)->icnt - (p)->ocnt)

#define	FIFO_FULL	0x004	/* fifo is full			*/
#define	FIFO_MEOF	0x008	/* EOF on input (put side)	*/
#define	FIFO_MERROR	0x010	/* error on input (put side)	*/
#define	FIFO_EXIT	0x020	/* exit() on non tape side	*/
#define	FIFO_EXERRNO	0x040	/* errno from non tape side	*/

#define	FIFO_IWAIT	0x200	/* input (put side) waits after first record */
#define	FIFO_I_CHREEL	0x400	/* change input tape reel if fifo gets empty */
#define	FIFO_O_CHREEL	0x800	/* change output tape reel if fifo gets empty */

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
extern	BOOL	use_fifo;

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

#endif /* _FIFO_H */
