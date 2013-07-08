/* @(#)buffer.c	1.31 13/06/25 Copyright 1984-2013 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)buffer.c	1.31 13/06/25 Copyright 1984-2013 J. Schilling";
#endif
/*
 *	Virtual storage (buffer) management.
 *
 *	Copyright (c) 1984-2013 J. Schilling
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

/*
 * The paged virtual memory system maintains a list of resident headers each
 * pointing to a buffer that may be either in memory or is swapped out into
 * the 'swapfile'. The number of resident buffers is set with 'initbuf'.
 * There must be at least two resident buffers in order to be able to
 * do a 'splitbuffer' operation.
 *
 * All used headers are in a doubly linked list. All used headers
 * that belong to resident buffers are kept also in a second doubly linked list
 * called the mru/lru list. This list is used to determine the buffer
 * that (as least recently used buffer) is subject of a swap out operation.
 *
 * To make live easier (and avoid always to check for an empty list) at least
 * one header is kept in the used list.
 *
 * Deleted headers are kept in a third list that is used to recycle then.
 */

#include "ved.h"
#include "buffer.h"

LOCAL	headr_t	*delhead;	/* Head of the linked list of deleted headers*/
LOCAL	headr_t	*mru;		/* The most recently used header in memory   */
LOCAL	headr_t	*lru;		/* The least recently used header in memory  */
				/* All resident headers are in a chain going */
				/* from mru to lru			    */

extern	Uchar	swapname[TMPNSIZE];
#ifdef	__needed__
LOCAL	FILE	*readfile;
#endif
LOCAL	FILE	*swapfile;
#ifdef	__needed__
LOCAL	off_t	readfoff;
#endif
LOCAL	off_t	swapfoff;
LOCAL	off_t	swapfend;
LOCAL	int	nbheads;
#ifdef	DEBUG
LOCAL	long	sumsize;
LOCAL	long	sumwrite;
LOCAL	long	sumfull;
#endif
LOCAL	int	maxbuffers = MAXBUFFERS;
LOCAL	int	numbuffers;

EXPORT	void	initbuffers	__PR((ewin_t *wp, int nbuf));
EXPORT	void	termbuffers	__PR((ewin_t *wp));
LOCAL	void	getheader	__PR((ewin_t *wp));
LOCAL	Uchar	*getbuffer	__PR((ewin_t *wp));
LOCAL	BOOL	releasebuffer	__PR((ewin_t *wp, headr_t *linkp));
EXPORT	headr_t	*addbuffer	__PR((ewin_t *wp, headr_t *prev));
EXPORT	headr_t	*deletebuffer	__PR((ewin_t *wp, headr_t *linkp));
EXPORT	void	splitbuffer	__PR((ewin_t *wp, headr_t *linkp, int pos));
EXPORT	void	compressbuffer	__PR((ewin_t *wp, headr_t *linkp));
LOCAL	int	bufswapin	__PR((ewin_t *wp, headr_t *linkp));
#ifdef	__needed__
LOCAL	int	bufreadin	__PR((ewin_t *wp, headr_t *linkp));
#endif
EXPORT	void	readybuffer	__PR((ewin_t *wp, headr_t *linkp));
LOCAL	void	buf_mruhead	__PR((headr_t *linkp));
LOCAL	void	buf_remruhead	__PR((headr_t *linkp));
EXPORT	void	bufdebug	__PR((ewin_t *wp));

/*
 * Init the paged virtual buffers.
 * Nbuf is the number of incore buffers and may be set from the caller,
 * if nbuf is <= 0, use the default value.
 * The swap file file is opened and then a new (needed) buffer is added.
*/
EXPORT void
initbuffers(wp, nbuf)
	ewin_t	*wp;
	int	nbuf;
{
	headr_t	*hp;

	/*
	 * To be able to to a splitbuffer operation without copying into
	 * a temporary buffer, we need at least two incore buffers.
	 */
	if (nbuf == 1)
		nbuf = 2;
	if (nbuf > 0)
		maxbuffers = nbuf;
	/*
	 * Open the swp file for backup memory.
	 */
	if (swapfile == (FILE *)NULL)
		swapfile = tmpfopen(wp, swapname, "ctwrub");

	addbuffer(wp, wp->bhead);
	/*
	 * The first valid position in buffer space is '0'.
	 * Most find operations return a position one behind the actual
	 * position making 'eof' a valid offset.
	 * To allow this to work, we make 'eof' a valid offset by inserting
	 * a dummy space at the end.
	 */
	hp = wp->bhead;
	hp->size = 1;
	hp->buf[0] = ' ';
	nbheads++;
}

/*
 * Close the swapfile and delete it in preparation of an exit()
 */
/* ARGSUSED */
EXPORT void
termbuffers(wp)
	ewin_t	*wp;
{
	if (--nbheads > 0) {
		/*
		 * Wenn mehrere Files unterstützt werden,
		 * dann wird hier der Speicher zurückgegeben.
		 */
		/*EMPTY*/
	}
	if (nbheads == 0) {
		if (swapfile)
			fclose(swapfile);
		if (swapname[0] != '\0')
			unlink(C swapname);
	}

#ifdef	DEBUG
if (sumwrite)cdbg("nwrite: %ld nfull: %ld avgsize: %ld (%ld%%)",
sumwrite, sumfull, sumsize/sumwrite, sumsize/BUFFERSIZE*100/sumwrite);
#endif
}

/*
 * Get a new header.
 * Try to use a recycled header if possible,
 * else get some new headers and give them new swap file positions.
 */
LOCAL void
getheader(wp)
	ewin_t	*wp;
{
	register headr_t *hp;
	static	int	amt = 32;
	register int	i = amt;

	if (amt < 256)
		amt *= 2;

	if ((delhead = (headr_t *) malloc(i*sizeof (headr_t))) == NULL) {
		rsttmodes(wp);
		raisecond("Out of memory", 0);
	}

	hp = delhead;
	hp->next = hp;

	while (--i > 0) {
		hp = hp->next;
		hp->buf = hp->cont = NULL;
		hp->fpos = swapfend;
		hp->size = 0;
		hp->flags = 0;
		hp->next = &hp[1];
		swapfend += BUFFERSIZE;
	}
	hp->next = (headr_t *)0;
}

/*
 * Get a buffer.
 * If we are below the buffer limit, allocate a new one, else try to release
 * one of the buffers currently in use. The buffer to be released in this
 * case is the first in the least recently used chain.
 */
LOCAL Uchar *
getbuffer(wp)
	ewin_t	*wp;
{
	register headr_t *linkp = lru;
		Uchar	*buf;

#ifdef	DEBUG
	cdbg("getbuffer()");
#endif
	if (numbuffers >= maxbuffers) {
		if (linkp == (headr_t *)0) {
			rsttmodes(wp);
			raisecond("Incore buffer lost", 0);
		}
		if (releasebuffer(wp, linkp)) {
			buf = linkp->buf;
			linkp->buf = linkp->cont = NULL;
			linkp->flags &= ~INMEMORY;
			buf_remruhead(linkp);
#ifdef	DEBUG
	cdbg("getbuffer() = %p", (void *)linkp->buf);
#endif
			return (buf);
		}
	}
	if ((buf = (Uchar *) malloc(BUFFERSIZE * sizeof (char))) == 0) {
		rsttmodes(wp);
		raisecond("Out of memory", 0);
	}
	numbuffers++;
#ifdef	DEBUG
	cdbg("getbuffer() = %p", (void *)buf);
#endif
	return (buf);
}

/*
 * Release the buffer associated with a header.
 * If the buffer is modified, back it up to the swapfile before.
 */
LOCAL BOOL
releasebuffer(wp, linkp)
		ewin_t	*wp;
	register headr_t *linkp;
{
#ifdef	DEBUG
	cdbg("releasebuffer(%p)", (void *)linkp);
#endif
	if ((linkp->flags & INMEMORY) == 0)
		return (TRUE);
	if ((linkp->flags & MODIFIED) == 0)
		return (TRUE);
#ifdef	DEBUG
	writecons("write");
#endif
#ifdef	DEBUG
if (linkp->size != BUFFERSIZE)
cdbg("pos: %lld size: %d", (Llong)linkp->fpos, linkp->size);
else sumfull++;
sumwrite++;
sumsize += linkp->size;
#endif
	/*
	 * Try to avoid seeks if possible.
	 */
	if (swapfoff != linkp->fpos)
		lseek(fdown(swapfile), linkp->fpos, SEEK_SET);
	if (writesyserr(wp, swapfile, linkp->cont, linkp->size, swapname) < 0) {
		swapfoff = (off_t)-1;
		return (FALSE);
	}
	swapfoff = linkp->fpos + linkp->size;
	linkp->flags &= ~MODIFIED;
	linkp->flags |= ONSWAP;
	return (TRUE);
}

/*
 * Read a buffer from the swapfile.
 */
LOCAL int
bufswapin(wp, linkp)
		ewin_t	*wp;
	register headr_t *linkp;
{
	/*
	 * Try to avoid seeks if possible.
	 */
	if (swapfoff != linkp->fpos)
		lseek(fdown(swapfile), linkp->fpos, SEEK_SET);
	if (readsyserr(wp, swapfile, linkp->cont, linkp->size, swapname) < 0) {
		swapfoff = (off_t)-1;
		return (-1);
	} else {
		swapfoff = linkp->fpos + linkp->size;
	}
	linkp->flags |= INMEMORY;
	return (0);
}

#ifdef	__needed__
/*
 * Read a buffer from the original file.
 * Used in case that the original file was not read into the swapfile when
 * starting to edit a new file.
 */
LOCAL int
bufreadin(wp, linkp)
		ewin_t	*wp;
	register headr_t *linkp;
{
	/*
	 * Try to avoid seeks if possible.
	 */
	if (readfoff != linkp->fpos)
		lseek(fdown(readfile), linkp->fpos, SEEK_SET);
								/*XXX*/
	if (readsyserr(wp, readfile, linkp->cont, linkp->size, wp->curfile) < 0) {
		readfoff = (off_t)-1;
		return (-1);
	} else {
		readfoff = linkp->fpos + linkp->size;
	}
	linkp->flags |= INMEMORY;
	return (0);
}
#endif

/*
 * Make the buffer for a header ready to use
 * by swapping in the data from the swap file if needed.
 * Make it the most recently used one.
 */
EXPORT void
readybuffer(wp, linkp)
		ewin_t	*wp;
	register headr_t *linkp;
{
#ifdef	DEBUG
	cdbg("readybuffer(%p)", (void *)linkp);
#endif
	if ((linkp->flags & INMEMORY) == 0) {
#ifdef	DEBUG
		writecons("read");
#endif
		linkp->cont = linkp->buf = getbuffer(wp);
		if (linkp->flags & INVALID) {
			linkp->flags &= ~INVALID;
			linkp->flags |= INMEMORY;
		} else {
			bufswapin(wp, linkp);
		}
	}
	buf_mruhead(linkp);
}

/*
 * Add a new header and an associated buffer after 'prev' header.
 * Add the new header to the linked list of the used headers as well as to
 * the mru/lru chain by making it the most recently used one.
 * If 'prev' is NULL, add the new header to the head of the header chain.
 * Getting a new buffer for the new header is handled by getbuffer().
 * If no memory could be allocated, raise a software signal and return NULL.
 */
EXPORT headr_t *
addbuffer(wp, prev)
	ewin_t	*wp;
	headr_t	*prev;
{
	register headr_t *newlinkp = NULL;
	register headr_t *next;
#ifdef	DEBUG
	cdbg("addbuffer(%p)", (void *)prev);
#endif

	if (prev) {
		next = prev->next;
	} else {
		next = wp->bhead;
	}

	if (delhead == (headr_t *)0)
		getheader(wp);

	newlinkp = delhead;
	delhead = newlinkp->next;

if (newlinkp->fpos == (off_t)-1) {
	newlinkp->fpos = swapfend;
	swapfend += BUFFERSIZE;
}

	if ((newlinkp->cont = newlinkp->buf = getbuffer(wp)) == NULL) {
		newlinkp->flags = INVALID;
		newlinkp->next = delhead;
		delhead = newlinkp;
		return ((headr_t *)0);
	}
	newlinkp->flags = (INMEMORY|MODIFIED);	/* make shure to swap out */
						/* at the first time	  */
#ifdef	DEBUG
	cdbg("New Link: %p", (void *)newlinkp);
#endif
	newlinkp->size = 0;
	newlinkp->nextr = newlinkp->prevr = (headr_t *)0;
	newlinkp->next = next;
	if (next)
		next->prev = newlinkp;
	newlinkp->prev = prev;
	if (prev) {
		prev->next = newlinkp;
	} else {
		wp->bhead = newlinkp;
	}
	buf_mruhead(newlinkp);
	return (newlinkp);
}

/*
 * Delete the header 'linkp' and its associated buffer if 'linkp' is not the
 * only header. Remove it from the header chain and from the mru/lru chain.
 * If there was only one header return it, else return 'linkp->prev'.
 */
EXPORT headr_t *
deletebuffer(wp, linkp)
		ewin_t	*wp;
	register headr_t *linkp;
{
	headr_t	*next = linkp->next;
	headr_t	*prev = linkp->prev;
#ifdef	DEBUG
	cdbg("deletebuffer(%p)", (void *)linkp);
#endif

	if (next || prev) {
		/*
		 * 'linkp' is not the only header,
		 * remove it from the mru/lru chain and
		 * from the active header chain.
		 */
		buf_remruhead(linkp);
		if (next) {
			next->prev = prev;
		}
		if (prev) {
			prev->next = next;
		} else {
			wp->bhead = next;
		}
		/*
		 * Free the associated buffer
		 */
		if (linkp->flags & INMEMORY) {
			free(linkp->buf);
			linkp->buf = linkp->cont = NULL;
			numbuffers--;
		}
		/*
		 * Mark as deleted and add header to linked dels
		 */
		linkp->flags = INVALID;
		linkp->next = delhead;
		delhead = linkp;
		clearifwpos(wp, linkp);
		return (prev);
	} else {
		/*
		 * Keep at least one header
		 */
		linkp->size = 0;
		linkp->cont = linkp->buf;
		return (linkp);
	}
}

/*
 * Split a buffer into two buffers starting at offset 'pos' in the buffer.
 * Used by movegap() to prepare insert or delete operations.
 * Pos will be the first unused element in the current buffer after
 * the split is done.
 * If the current buffer ends before pos, we are done,
 * else insert a new buffer and copy the rest to the new buffer.
 * Put a gap at the beginning of the new buffer.
 */
EXPORT void
splitbuffer(wp, linkp, pos)
		ewin_t	*wp;
	register headr_t *linkp;
	int	pos;
{
	register headr_t *newlinkp = NULL;
	register Uchar *from;
	register Uchar *to;
		Uchar *end;
		int newsize = linkp->size - pos;

#ifdef	DEBUG
	cdbg("splitbuffer(%p, %d)", (void *)linkp, pos);
#endif
	if (newsize <= 0)
		return;
	readybuffer(wp, linkp);
	newlinkp = addbuffer(wp, linkp);

	end = newlinkp->buf + BUFFERSIZE;
	newlinkp->size = newsize;
	newlinkp->cont = end - newsize;
	linkp->size = pos;

	from = linkp->cont + pos;
	to = newlinkp->cont;
	if (newsize > 16) {
		movebytes(C from, C to, newsize);
	} else while (to < end) {
		*to++ = *from++;
	}
}

/*
 * Compress the content of a buffer.
 * Remove a leading gap and then try to combine its content with
 * the content of the next buffer.
 * Loop until the next buffer has no leading gap.
 */
EXPORT void
compressbuffer(wp, linkp)
		ewin_t	*wp;
	register headr_t *linkp;
{
	register Uchar *from;
	register Uchar *to;
		Uchar *end;
	register headr_t *next;

again:

#ifdef	DEBUG
	cdbg("compressbuffer(%p)", (void *)linkp);
#endif
	if (linkp == (headr_t *)0)
		return;
	readybuffer(wp, linkp);

	/*
	 * First, remove a leading gap if present.
	 */
	if ((from = linkp->cont) != (to = linkp->buf)) {
		linkp->flags |= MODIFIED;
		if (linkp->size > 16) {
			movebytes(C from, C to, linkp->size);
		} else {
			end = to + linkp->size;
			while (to < end)
				*to++ = *from++;
		}
	}
	linkp->cont = linkp->buf;

	while ((next = linkp->next) != NULL &&
		(linkp->size+next->size <= BUFFERSIZE)) {

		/*
		 * If the buffer from 'linkp' and the next buffer or one of the
		 * following buffers can be combined, do it.
		 */
		readybuffer(wp, next);
		from = next->cont;
		to = linkp->cont + linkp->size;
		linkp->flags |= MODIFIED;
		if (next->size > 16) {
			movebytes(C from, C to, next->size);
		} else {
			end = from + next->size;
			while (from < end)
				*to++ = *from++;
		}
		linkp->size += next->size;

		/*
		 * Delete the now unused buffer.
		 */
		deletebuffer(wp, next);
	}

	/*
	 * If the next buffer has a leading gap, compress it too.
	 */
	if ((next = linkp->next) != NULL && (next->cont != next->buf)) {
		linkp = next;
		goto again;
	}
}

/*
 * Make 'linkp' the most recently used header
 */
LOCAL void
buf_mruhead(linkp)
	register headr_t *linkp;
{
	/*
	 * If linkp is already the most recently used header, we are done.
	 */
	if (mru == linkp)
		return;

	/*
	 * First remove it from the mru/lru chain.
	 * This will also maintain the lru pointer.
	 */
	buf_remruhead(linkp);

	/*
	 * Actually make it the most recently used one.
	 */
	linkp->prevr = mru;
	linkp->nextr = (headr_t *)0;
	if (mru != (headr_t *)0)
		mru->nextr = linkp;
	mru = linkp;

	/*
	 * If the list was empty before,
	 * it is the least recently used header too.
	 */
	if (lru == (headr_t *)0)
		lru = linkp;
}

/*
 * Remove a header from the mru/lru chain.
 * May be called even if the header is not in the mru/lru chain.
 */
LOCAL void
buf_remruhead(linkp)
	register headr_t *linkp;
{
	register headr_t *nextr = linkp->nextr;
	register headr_t *prevr = linkp->prevr;

	if (mru == linkp)
		mru = prevr;
	if (lru == linkp)
		lru = nextr;
	/*
	 * If it is not the first or the last in the chain, remove it.
	 */
	if (nextr) {
		nextr->prevr = prevr;
		linkp->nextr = (headr_t *)0;
	}
	if (prevr) {
		prevr->nextr = nextr;
		linkp->prevr = (headr_t *)0;
	}
}

/*#define	DEBUG*/
#ifdef	DEBUG

/*
 * Print buffer debug statistics
 */
EXPORT void
bufdebug(wp)
	ewin_t *wp;
{
	long	i = 0;
	headr_t	*p = wp->bhead;

	while (p) {
		i++;
		p = p->next;
	}
	p = wp->bhead;
	cdbg("h->size: %d n: %ld", p->size, i);
	if (p->next)
		cdbg("h->next->size: %d", p->next->size);
}

#else

/* ARGSUSED */
EXPORT void
bufdebug(wp)
	ewin_t *wp;
{
}

#endif
