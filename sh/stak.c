/*
 * UNIX shell
 *
 * Stacked-storage allocation.
 *
 * Written by Geoff Collyer
 * Rewritten by J. Schilling to match the SVr4 version of the Bourne Shell
 *
 *
 * This replaces the original shell's stak.c.  This implementation
 * does not rely on memory faults to manage storage.  See ``A Partial
 * Tour Through the UNIX Shell'' for details.  This implementation is
 * newer than the one published in that paper, but differs mainly in
 * just being a little more portable.  In particular, it works on
 * Ultrasparc and Alpha processors, which are insistently 64-bit processors.
 *
 * Maintains a linked stack (of mostly character strings), the top (most
 * recently allocated item) of which is a growing string, which pushstak()
 * inserts into and grows as needed.
 *
 * Each item on the stack consists of a pointer to the previous item
 * (the "word" pointer; stk.topitem points to the top item on the stack),
 * an optional magic number, and the data.  There may be malloc overhead storage
 * on top of this.  Heap storage returned by alloc() lacks the "word" pointer.
 *
 * Pointers returned by these routines point to the first byte of the data
 * in a stack item; users of this module should be unaware of the "word"
 * pointer and the magic number.  To confuse matters, stk.topitem points to the
 * "word" linked-list pointer of the top item on the stack, and the
 * "word" linked-list pointers each point to the corresponding pointer
 * in the next item on the stack.  This all comes to a head in tdystak().
 *
 * Geoff Collyer
 */

/*
 * see also stak.h
 */
#include "defs.h"

/*
 *	Copyright Geoff Collyer 1999-2005
 *
 * @(#)stak.c	2.2 12/04/05 Copyright 2010-2012 J. Schilling
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

#ifndef lint
static	UConst char sccsid[] =
	"@(#)stak.c	2.2 12/04/05 Copyright 2010-2012 J. Schilling";
#endif


#undef free				/* refer to free(3) here */
#define	stakend		brkend		/* SVr4 Shell uses brkend */
#define	USED(x)				/* Built in from Plan-9 compiler */

/*
 * was (10*1024) for testing; must be >= sizeof (struct fileblk) always.
 * must also be >= 2*(CPYSIZ in io.c [often 512])
 */
#undef	BRKINCR
#define	BRKINCR 1024

#ifdef	STAK_DEBUG
#define	TPRS(s)		do { if (Tracemem) prs((unsigned char *)s); } while (0)
#define	TPRN(n)		do { if (Tracemem) prln(n); } while (0)
#define	TPRNN(n)	do { if (Tracemem) prnln(n); } while (0)

#define	STPRS(s)	do { if (Stackdebug) prs((unsigned char *)s); } while (0)
#define	STPRN(n)	do { if (Stackdebug) prln(n); } while (0)
#define	STPRNN(n)	do { if (Stackdebug) prnln(n); } while (0)
#else
#define	TPRS(s)
#define	TPRN(n)
#define	TPRNN(n)

#define	STPRS(s)
#define	STPRN(n)
#define	STPRNN(n)
#endif

enum {
	Tracemem = 0,
	Stackdebug = 0,

	STMAGICNUM = 0x1235,		/* stak item magic */
	HPMAGICNUM = 0x4276,		/* heap item magic */
};

/*
 * to avoid relying on features of the Plan 9 C compiler, these structs
 * are expressed rather awkwardly.
 */
typedef struct stackblk Stackblk;

typedef struct {
	Stackblk	*word;		/* pointer to previous stack item */
	ULlong		magic;		/* force pessimal alignment */
} Stackblkhdr;

struct stackblk {
	Stackblkhdr	h;
	char		userdata[1];
};

typedef struct {
	ULlong		magic;		/* force pessimal alignment */
} Heapblkhdr;

typedef struct {
	Heapblkhdr	h;
	char		userdata[1];
} Heapblk;

typedef struct {
	unsigned char	*base;
	/*
	 * A chain of ptrs of stack blocks that have become covered
	 * by heap allocation.  `tdystak' will return them to the heap.
	 */
	Stackblk	*topitem;
} Stack;

unsigned brkincr = BRKINCR;		/* used in stak.h only */

static Stack	stk;
static char	*stklow;		/* Lowest known addr on stak */
static char	*stkhigh;		/* Highest known addr on stak */

/*
 * Global variables stakbas, staktop, stakbot, stakbsy, brkend see defs.c
 */

static void	*xmalloc	__PR((size_t size));
static void	tossgrowing	__PR((void));
static char	*stalloc	__PR((int size));
static void	grostalloc	__PR((void));
	unsigned char *getstak	__PR((Intptr_t asize));
#ifdef	STAK_DEBUG
static void	prnln		__PR((long));
#endif
static void	prln		__PR((long));
	unsigned char *locstak	__PR((void));
	unsigned char *savstak	__PR((void));
	unsigned char *endstak	__PR((unsigned char *argp));
	void	tdystak		__PR((unsigned char *sav));
static void	debugsav	__PR((unsigned char *sav));
	void	stakchk		__PR((void));
static	unsigned char *__growstak __PR((int incr));
	unsigned char *growstak	__PR((unsigned char *newtop));
	void	addblok		__PR((unsigned reqd));
	void	*alloc		__PR((size_t size));
	void	shfree		__PR((void *ap));
	unsigned char *cpystak	__PR((unsigned char *newstak));
	unsigned char *movstrstak	__PR((unsigned char *a, unsigned char *b));
	unsigned char *memcpystak	__PR((unsigned char *s1, unsigned char *s2, int n));

static void *
xmalloc(size)
	size_t	size;
{
	char	*ret = malloc(size);

	if (ret == NULL)
		return ((void *)ret);

	if (stklow == NULL)
		stklow = ret;
	else if (ret < stklow)
		stklow = ret;

	if ((ret + size) > stkhigh)
		stkhigh = ret + size;

	return ((void *)ret);
}

static void
tossgrowing()				/* free the growing stack */
{
	if (stk.topitem != 0) {		/* any growing stack? */
		Stackblk *nextitem;

		/* verify magic before freeing */
		if (stk.topitem->h.magic != STMAGICNUM) {
			prs((unsigned char *)"tossgrowing: stk.topitem->h.magic ");
			prln((long)stk.topitem->h.magic);
			prs((unsigned char *)"\n");
			error("tossgrowing: bad magic on stack");
		}
		stk.topitem->h.magic = 0;	/* erase magic */

		/*
		 * about to free the ptr to next, so copy it first
		 */
		nextitem = stk.topitem->h.word;

		TPRS("tossgrowing freeing ");
		TPRN((long)stk.topitem);
		TPRS("\n");

		free(stk.topitem);
		stk.topitem = nextitem;
	}
}

static char *
stalloc(size)		/* allocate requested stack space (no frills) */
	int		size;
{
	Stackblk	*nstk;

	TPRS("stalloc allocating ");
	TPRN(sizeof (Stackblkhdr) + size);
	TPRS(" user bytes ");

	if (size < sizeof (Llong))
		size = sizeof (Llong);
	nstk = xmalloc(sizeof (Stackblkhdr) + size);
	if (nstk == 0)
		error(nostack);

	TPRS("@ ");
	TPRN((long)nstk);
	TPRS("\n");

	/* stack this item */
	nstk->h.word = stk.topitem;	/* point back @ old stack top */
	stk.topitem = nstk;		/* make this new stack top */

	nstk->h.magic = STMAGICNUM;	/* add magic number for verification */
	return (nstk->userdata);
}

static void
grostalloc()				/* allocate growing stack */
{
	int	size = BRKINCR;

	/*
	 * fiddle global variables to point into this (growing) stack
	 */
	staktop = stakbot = stk.base = (unsigned char *)stalloc(size);
	stakend = stk.base + size;
}

/*
 * allocate requested stack.
 * movstrstak() assumes that getstak just realloc's the growing stack,
 * so we must do just that.  Grump.
 */
unsigned char *
getstak(asize)
	Intptr_t	asize;
{
	int		staklen;
	unsigned char	*nstk;

	/*
	 * +1 is because stakend points at the last byte of the growing stack
	 */
	staklen = stakend - stk.base;	/* # of usable bytes */

	TPRS("getstak(");
	TPRN(asize);
	TPRS(") calling __growstak(");
	TPRNN(asize - staklen);
	TPRS("):\n");

	/* grow growing stack to requested size */
	nstk = __growstak(asize - staklen);
	grostalloc();			/* allocate new growing stack */
	return (nstk);
}

#ifdef	STAK_DEBUG
static void
prnln(l)
	long	l;
{
	if (l < 0) {
		prs((unsigned char *)"-");
		l = -l;
	}
	prln(l);
}
#endif

static void
prln(l)
	long	l;
{
	prs(&numbuf[ltos(l)]);
}

/*
 * set up stack for local use (i.e. make it big).
 * should be followed by `endstak'
 */
unsigned char *
locstak()
{
	if (stakend - stakbot < BRKINCR) {
		TPRS("locstak calling __growstak(");
		TPRNN(BRKINCR - (stakend - stakbot));
		TPRS("):\n");
		(void) __growstak(BRKINCR - (stakend - stakbot));
	}
	return (stakbot);
}

/*
 * return an address to be used by tdystak later,
 * so it must be returned by getstak because it may not be
 * a part of the growing stack, which is subject to moving.
 */
unsigned char *
savstak()
{
	assert(staktop == stakbot);	/* assert empty stack */
	return (getstak(1));
}

/*
 * tidy up after `locstak'.
 * make the current growing stack a semi-permanent item and
 * generate a new tiny growing stack.
 */
unsigned char *
endstak(argp)
	unsigned char	*argp;
{
	unsigned char *ostk;

	if (argp >= stakend)
		__growstak(argp - stakend);
	*argp++ = 0;				/* terminate the string */
	TPRS("endstak calling __growstak(");
	TPRNN(-(stakend - argp));
	TPRS("):\n");
	ostk = __growstak(-(stakend - argp));	/* reduce growing stack size */
	grostalloc();				/* alloc. new growing stack */

	return (ostk);				/* perm. addr. of old item */
}

/*
 * Try to bring the "stack" back to sav (the address of userdata[0] in some
 * Stackblk, returned by __growstak()), and bring iotemp's stack back to iosav
 * (an old copy of iotemp, which may be zero).
 */
void
tdystak(sav)
	unsigned char	*sav;
{
	Stackblk	*blk = (Stackblk *)NIL;
	struct ionod	*iosav = (struct ionod *)NIL;

	rmtemp(iosav);			/* pop temp files */

	if (sav != 0)
		blk = (Stackblk *)(sav - sizeof (Stackblkhdr));
	if (sav == 0) {
		/* EMPTY */
		STPRS("tdystak(0)\n");
	} else if (blk->h.magic == STMAGICNUM) {
		/* EMPTY */
		STPRS("tdystak(data ptr: ");
		STPRN((long)sav);
		STPRS(")\n");
	} else {
		STPRS("tdystak(garbage: ");
		STPRN((long)sav);
		STPRS(")\n");
		error("tdystak: bad magic in argument");
	}

	/*
	 * pop stack to sav (if zero, pop everything).
	 * stk.topitem points at the ptr before the data & magic.
	 */
	while (stk.topitem != 0 && (sav == 0 || stk.topitem != blk)) {
		debugsav(sav);
		tossgrowing();		/* toss the stack top */
	}
	debugsav(sav);
	STPRS("tdystak: done popping\n");
	grostalloc();			/* new growing stack */
	STPRS("tdystak: exit\n");
}

static void
debugsav(sav)
	unsigned char	*sav;
{
	if (stk.topitem == 0) {
		/* EMPTY */
		STPRS("tdystak: stk.topitem == 0\n");
	} else if (sav != 0 &&
	    stk.topitem == (Stackblk *)(sav - sizeof (Stackblkhdr))) {
		/* EMPTY */
		STPRS("tdystak: stk.topitem == link ptr of arg: ");
		STPRN((long)stk.topitem);
		STPRS("\n");
	} else {
		/* EMPTY */
		STPRS("tdystak: stk.topitem == link ptr of item above arg: ");
		STPRN((long)stk.topitem);
		STPRS("\n");
	}
}

/*
 * Reduce the growing-stack size if possible
 */
void
stakchk()
{
	if (stakend - staktop > 2*BRKINCR) { /* lots of unused stack headroom */
		TPRS("stakchk calling __growstak(");
		TPRNN(-(stakend - staktop - BRKINCR));
		TPRS("):\n");
		(void) __growstak(-(stakend - staktop - BRKINCR));
	}
}

static unsigned char *			/* new address of grown stak */
__growstak(incr)			/* grow the growing stack by incr */
	int		incr;
{
	int		staklen;
	unsigned int	topoff;
	unsigned int	botoff;
	unsigned int	basoff;
	unsigned char	*oldbsy;

	if (stk.topitem == 0)		/* paranoia */
		grostalloc();		/* make a trivial stack */

	/*
	 * paranoia: during realloc, point @ previous item in case of signals
	 */
	oldbsy = (unsigned char *)stk.topitem;
	stk.topitem = stk.topitem->h.word;

	topoff = staktop - oldbsy;
	botoff = stakbot - oldbsy;
	basoff = stk.base - oldbsy;

	/*
	 * stakend points past the last valid byte of the growing stack
	 */
	staklen = stakend + incr - oldbsy;

	if (staklen < sizeof (Stackblkhdr))	/* paranoia */
		staklen = sizeof (Stackblkhdr);

	TPRS("__growstak growing ");
	TPRN((long)oldbsy);
	TPRS(" from ");
	TPRN(stakend - oldbsy);
	TPRS(" bytes; ");

	if (incr < 0) {
		/*
		 * V7 realloc wastes the memory given back when
		 * asked to shrink a block, so we malloc new space
		 * and copy into it in the hope of later reusing the old
		 * space, then free the old space.
		 */
		unsigned char *new = xmalloc((unsigned)staklen);

		if (new == (unsigned char *)NIL)
			error(nostack);
		memcpy(new, oldbsy, staklen);
		free(oldbsy);
		oldbsy = new;
	} else
		/*
		 * get realloc to grow the stack to match the stack top
		 */
		if ((oldbsy = realloc(oldbsy, (unsigned)staklen)) == (unsigned char *)NIL)
			error(nostack);
	TPRS("now @ ");
	TPRN((long)oldbsy);
	TPRS(" of ");
	TPRN(staklen);
	TPRS(" bytes (");
	if (incr < 0) {
		/* EMPTY */
		TPRN(-incr);
		TPRS(" smaller");
	} else {
		/* EMPTY */
		TPRN(incr);
		TPRS(" bigger");
	}
	TPRS(")\n");

	stakend = oldbsy + staklen;	/* see? points at the last byte */
	staktop = oldbsy + topoff;
	stakbot = oldbsy + botoff;
	stk.base = oldbsy + basoff;

	stk.topitem = (Stackblk *)oldbsy;	/* restore after realloc */
	return (stk.base);			/* addr of 1st usable byte */
}

unsigned char *
growstak(newtop)
	unsigned char	*newtop;
{
	UIntptr_t	incr;
	UIntptr_t	newoff = newtop - stakbot;

	incr = (UIntptr_t)round(newtop - brkend + 1, BYTESPERWORD);
	if (brkincr > incr)
		incr = brkincr;
	__growstak(incr);

	return (stakbot + newoff);	/* New value for newtop */
}

/* ARGSUSED reqd */
void
addblok(reqd)				/* called from main at start only */
	unsigned	reqd;
{
	USED(reqd);
	if (stakbot == 0)
		grostalloc();		/* allocate initial arena */
}

/*
 * Heap allocation.
 */
void *
alloc(size)
	size_t	size;
{
	Heapblk	*p = xmalloc(sizeof (Heapblkhdr) + size);

	if (p == (Heapblk *)NIL)
		error(nospace);

	p->h.magic = HPMAGICNUM;

	TPRS("alloc allocated ");
	TPRN(size);
	TPRS(" user bytes @ ");
	TPRN((long)p->userdata);
	TPRS("\n");
	return (p->userdata);
}

/*
 * the shell's private "free" - frees only heap storage.
 * only works on non-null pointers to heap storage
 * (below the data break and stamped with HPMAGICNUM).
 * so it is "okay" for the shell to attempt to free data on its
 * (real) stack, including its command line arguments and environment,
 * or its fake stak.
 * this permits a quick'n'dirty style of programming to "work".
 */
void
shfree(ap)
	void	*ap;
{
	char	*p = ap;
	Heapblk	*blk;

	if (p == 0)			/* Required by POSIX		*/
		return;			/* Ignore any NULL pointer	*/

	if (p < stklow) {		/* Below heap addresses		*/
#ifdef	FREE_DEBUG
		prs((unsigned char *)"free(");
		prln((long)ap);
		prs((unsigned char *)"): arg is below heap.\n");
#endif
		return;
	}
	if (p > stkhigh) {		/* Above heap addresses		*/
#ifdef	FREE_DEBUG
		prs((unsigned char *)"free(");
		prln((long)ap);
		prs((unsigned char *)"): arg is above heap.\n");
#endif
		return;
	}

	blk = (Heapblk *)(p - sizeof (Heapblkhdr));

	TPRS("shfree freeing user data @ ");
	TPRN((long)p);
	TPRS("\n");
	/*
	 * ignore attempts to free non-heap storage
	 */
	if (blk->h.magic == HPMAGICNUM) {
		blk->h.magic = 0;	/* erase magic */
		free(blk);
#ifdef	FREE_DEBUG
	} else if (blk->h.magic == STMAGICNUM) {
		prs((unsigned char *)"free(");
		prln((long)ap);
		prs((unsigned char *)"): arg is from stak.\n");
	} else {
		prs((unsigned char *)"free(");
		prln((long)ap);
		prs((unsigned char *)"): arg is unknown type.\n");
#endif
	}
}

#ifdef	DO_SYSALLOC
void
chkmem()
{
}
#endif

/*--------------------------------------------------------------------------*/
/*
 * The code below has been taken from the historical stak.c
 *
 * Copyright 2008-2012 J. Schilling
 */

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * Copy the string in "x" to the stack and make it semi-permanent.
 * The current local stack is assumed to be empty.
 */
unsigned char *
cpystak(x)
unsigned char	*x;
{
	return(endstak(movstrstak(x, locstak())));
}

/*
 * Append the string in "a" to the string pointed to by "b".
 * "b" must be on the current local stack.
 * Return the address of the nul character at the end of the new string.
 *
 * The stack is kept growable.
 */
unsigned char *
movstrstak(a, b)
	unsigned char	*a;
	unsigned char	*b;
{
	do {
		if (b >= brkend)
			b = growstak(b);
	} while ((*b++ = *a++) != '\0');
	return (--b);
}

/*
 * Append the string in "s2" to the string pointed to by "s1".
 * "s1" must be on the current local stack.
 * Always copy n bytes from s2 to s1.
 * Return "old value" of s1,
 * taking care of that s1 may have been relocated by growstak().
 *
 * The stack is kept growable.
 */
unsigned char *
memcpystak(s1, s2, n)
	unsigned char	*s1;
	unsigned char	*s2;
	int		n;
{
	int amt = n > 0 ? n : 0;

	while (--n >= 0) {
		if (s1 >= brkend)
			s1 = growstak(s1);
		*s1++ = *s2++;
	}
	return (s1 - amt);
}
