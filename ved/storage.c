/* @(#)storage.c	1.48 07/03/08 Copyright 1984-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)storage.c	1.48 07/03/08 Copyright 1984-2007 J. Schilling";
#endif
/*
 *	Storage management based on the paged virtual memory
 *	provided by buffer.c
 *
 *	Copyright (c) 1984-2007 J. Schilling
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

/*
 * The basic buffer operations:
 *
 *	insertion, deletion, loading/storing from/to a file and extracting
 *
 * are handled in this package. Findpos() gives direct buffer access for
 * a given buffer offset. All operations are based on:
 *
 *	dot	- The current corsor position offset in the buffer
 *
 *	eof	- The offset that comes directly after the last valid
 *		  character in the buffer, 'eof' is the number of valid
 *		  characters in the buffer. The 'eof' position contains a
 *		  space charcter to be able to access this position.
 *
 *	gap	- The offset in the buffer memory where the gap is. It
 *		  has been prepared for insertions or deletions by
 *		  splitting the actual buffer at the 'gap' position.
 *		  The 'gap' position is maintained by the insertion
 *		  and deletion routines.
 *
 * To speed up findpos() on very long files, we maintain an 'anker' that
 * is at or close before the start or the current screen window. This allows
 * findpos() to work relative to this 'anker'. The variables that are
 * used to maintain this 'anker' are 'winlink', 'winpos' and 'winoff'.
 */

/*
 * Include code to speed up findpos().
 */
/*#define	FASTPOS*/
/*
 * Include code to time the fastpos code.
 */
/*#define	TIMEPOS*/
/*
 * Include code for extended fastpos debugging.
 */
/*#define	XPOSDEBUG*/

#include "ved.h"
#include "buffer.h"
#ifdef	TIMEPOS
#	include <schily/time.h>	/* Nur für Zeitmessungen */
#endif
#include <schily/errno.h>

#define	wp_gaplink(p)	((headr_t *)((p)->gaplink))
#define	wp_winlink(p)	((headr_t *)((p)->winlink))

EXPORT	void	insert		__PR((ewin_t *wp, Uchar* str, long size));
EXPORT	void	delete		__PR((ewin_t *wp, epos_t size));
EXPORT	void	rubout		__PR((ewin_t *wp, epos_t size));
EXPORT	BOOL	loadfile	__PR((ewin_t *wp, Uchar* filename, BOOL newdefault));
EXPORT	BOOL	savefile	__PR((ewin_t *wp, epos_t begin, epos_t end, FILE * f, char *name));
EXPORT	BOOL	backsavefile	__PR((ewin_t *wp, epos_t begin, epos_t end, FILE * f, char *name));
EXPORT	void	getfile		__PR((ewin_t *wp, FILE * f, epos_t size, char *name));
EXPORT	void	backgetfile	__PR((ewin_t *wp, FILE * f, epos_t size, char *name));
LOCAL	void	reversebuffer	__PR((Uchar* from, Uchar* to, long size));
EXPORT	BOOL	isdos		__PR((ewin_t *wp));
EXPORT	int	extract		__PR((ewin_t *wp, epos_t begin, Uchar *str, int size));
EXPORT	int	extr_line	__PR((ewin_t *wp, epos_t begin, char *str, int size));
EXPORT	int	retractline	__PR((ewin_t *wp, epos_t begin, char *str, int size));
LOCAL	void	movegap		__PR((ewin_t *wp, epos_t pos));
EXPORT	void	clearifwpos	__PR((ewin_t *wp, headr_t *this));
EXPORT	void	clearwpos	__PR((ewin_t *wp));
EXPORT	void	backwpos	__PR((ewin_t *wp));
EXPORT	void	findwpos	__PR((ewin_t *wp, epos_t new));
EXPORT	void	findpos		__PR((ewin_t *wp, epos_t pos, headr_t ** returnlinkp, int *returnoff));
#ifdef	FASTPOS
#ifdef	CHECKPOS
LOCAL	void	ckfindpos	__PR((ewin_t *wp, epos_t pos, headr_t ** returnlinkp, int *returnoff));
#endif
LOCAL	void	rfindpos	__PR((ewin_t *wp, epos_t pos, headr_t ** returnlinkp, int *returnoff));
#endif

/*
 * Insert 'size' characters at dot.
 * Then put the dot after the inserted characters.
 */
EXPORT void
insert(wp, str, size)
		ewin_t	*wp;
	register Uchar	*str;
	register long	size;
{
	register Uchar	*to;
	register long 	n = size;
	register int	amount;

	if (n <= 0)
		return;

	movegap(wp, wp->dot);

#ifdef	FASTPOS
	if (wp->winlink && wp->dot <= wp->winoff)
		clearwpos(wp);
#endif

	if (wp->markvalid && wp->mark == wp->dot) {
		resetmark(wp);
		wp->markvalid = 1;
	}

	while (n > 0) {
		/*
		 * There is no leading gap after a movegap() call.
		 */
		amount = min(n, BUFFERSIZE - wp_gaplink(wp)->size);
		to = wp_gaplink(wp)->cont + wp_gaplink(wp)->size;
		wp_gaplink(wp)->flags |= MODIFIED;
		wp_gaplink(wp)->size += amount;
		n -= amount;
		if (amount > 16) {
			movebytes(C str, C to, amount);
			str += amount;
		} else {
			while (amount--)
				*to++ = *str++;
		}
		if (n)
			wp->gaplink = addbuffer(wp, wp_gaplink(wp));
	}
	wp->eof += size;
	wp->gap += size;
	if (wp->markvalid && wp->mark > wp->dot)
		wp->mark += size;
	wp->dot += size;
}

/*
 * Delete 'size' characters after the dot.
 * Leave 'dot' at the old position.
 */
EXPORT void
delete(wp, size)
	ewin_t	*wp;
	epos_t	size;
{
	register headr_t *next;
	register epos_t	n;
	register int	amount;

#ifdef	DEBUG
	cdbg("delete(%lld)", (Llong)size);
#endif
	if (size > wp->eof-wp->dot)
		size = wp->eof-wp->dot;
	n = size;
	if (n <= 0)
		return;

	movegap(wp, wp->dot);

	while (n > 0) {
		if ((next = wp_gaplink(wp)->next) == NULL)	/* Paranoia */
			break;
#ifdef	FASTPOS
		if (next == wp->winlink)
			clearwpos(wp);
#endif
/*		readybuffer(wp, next);*/
		amount = min(n, next->size);
		n -= amount;
		next->size -= amount;
		next->cont += amount;
		/*
		 * We are modifying next->cont and thus must set the MODIFIED
		 * flag in the header. This is needed because we swap out from
		 * linkp->cont and remove any leading gap when we swap in.
		 * If we don't want to set the MODIFIED flag here, we must
		 * always swap out the complete buffer and restore the old
		 * leading gap and the linkp->cont offset when swapping in
		 * again.
		 */
		next->flags |= MODIFIED;
		if (n)
			deletebuffer(wp, next);
	}
	wp->eof -= size;
	if (wp->markvalid) {
		if (wp->mark >= wp->dot && wp->mark < wp->dot+size) {
			setmark(wp, wp->dot);
		} else if (wp->mark >= wp->dot+size) {
			wp->mark -= size;
		}
	}
#ifdef	DEBUG
	cdbg("delete() done");
#endif
}

/*
 * Delete 'size' characters before the dot.
 * Then move the dot back 'size' characters.
 */
EXPORT void
rubout(wp, size)
	ewin_t	*wp;
	epos_t	size;
{
	register epos_t	n;
		epos_t	savedot = wp->dot;
	register int	amount;

	if (size > wp->dot)
		size = wp->dot;
	n = size;
	if (n <= 0)
		return;

	movegap(wp, wp->dot);

	while (n > 0) {
		if (! wp_gaplink(wp)->next)	/* Paranoia */
			break;
#ifdef	FASTPOS
		if (wp_gaplink(wp) == wp->winlink)
			clearwpos(wp);
#endif
		amount = min(n, wp_gaplink(wp)->size);
		n -= amount;
		wp_gaplink(wp)->size -= amount;
		if (n)
			wp->gaplink = deletebuffer(wp, wp_gaplink(wp));
/*		readybuffer(wp, wp_gaplink(wp));*/
	}
	wp->dot -= size;
	wp->eof -= size;
	wp->gap -= size;
	if (wp->markvalid) {
		if (wp->mark < savedot && wp->mark >= wp->dot) {
			setmark(wp, wp->dot);
		} else if (wp->mark >= savedot) {
			wp->mark -= size;
		}
	}
}

/*
 * Load the content of a file at dot position.
 */
EXPORT BOOL
loadfile(wp, filename, newdefault)
	ewin_t	*wp;
	Uchar	*filename;
	BOOL	newdefault;	/* if true: this is the currenlty edited file*/
{
	FILE	*infile;
	Uchar	str[BUFSIZ < 8192 ? 8192 : BUFSIZ];
	long	size;
#if	defined(IS_CYGWIN) || defined(__DJGPP__)
	BOOL	retry = FALSE;
#endif

	if (newdefault) {
		wp->eflags &= ~(FREADONLY|FNOLOCK);
		if (wp->curfd >= 0)
			close(wp->curfd);
		wp->curfd = -1;
		if (wp->curfp != NULL)
			fclose(wp->curfp);
		if (!ReadOnly && writable(filename))
			lockfile(wp, C filename);
	}
	if ((infile = fileopen(C filename, "rub")) == (FILE *) NULL) {
		if (geterrno() == ENOENT) {
			if (newdefault && ReadOnly == 0) {
				defaulterr(wp, UC "NEW FILE");
			} else {
				writeerr(wp, "FILE DOES NOT EXIST");
			}
		} else {
			write_errno(wp, "CAN'T OPEN FILE");
		}
		return (FALSE);
	}
	file_raise(infile, FALSE);
#if	defined(IS_CYGWIN) || defined(__DJGPP__)
again:
#endif
	while ((size = readsyserr(wp, infile, str, sizeof (str), UC "FILE")) > 0) {
		insert(wp, str, size);
	}
	if (size < 0) {
#if	defined(IS_CYGWIN) || defined(__DJGPP__)
		/*
		 * On Cygwin, we cannot lock correctly.
		 * Locks are on a fd base not on a process base.
		 * In addition, we may not even read from a diffrerent fd
		 * if we wold a write lock on another fd.
		 */
		if (newdefault && !retry && wp->eof == 0 &&
		    wp->curfd >= 0 && geterrno() == EACCES) {
			/*
			 * If read did not work and we did nothing
			 * until now, this is a Cygwin locking problem
			 * and we must destroy the locking.
			 */
			close(wp->curfd);
			wp->curfd = -1;
			retry = TRUE;
			goto again;
		}
#endif
		fclose(infile);
		return (FALSE);
	}
#if	defined(IS_CYGWIN) || defined(__DJGPP__)
	if (newdefault && !retry && wp->curfd >= 0) {
		write_errno(wp, "LOCK WORKED!");
		sleep(2);
	}
#ifdef	NONOONO
	if (newdefault && wp->curfd < 0) {
		if (!ReadOnly && writable(C filename))
			lockfile(wp, C filename);
	}
#endif
#endif
	if (wp->curfd >= 0) {
		/*
		 * Due to a conceptional bug of lockf()/fcntl(f, F_SETLK)
		 * we will loose the lock if we close any fd associated
		 * with the loked file.
		 */
		wp->curfp = infile;
	} else {
		fclose(infile);
	}
	return (TRUE);
}

/*
 * Save a part of the current buffer into a file.
 */
EXPORT BOOL
savefile(wp, begin, end, f, name)
	ewin_t	*wp;
	epos_t	begin;
	epos_t	end;
	FILE	*f;
	char	*name;
{
	register headr_t *linkp = wp->bhead;
		headr_t *passlinkp;
	register int	amount;
		int	pos = 0;
		int	nextsize;
		Uchar	*from;

#ifdef	JOS
	getpid();
#else
	seterrno(0);		/* Damit write_errno korrekt arbeitet */
#endif

	if (end >= wp->eof)
		end = wp->eof;
	if (begin >= end)
		return (FALSE);
	if (f == (FILE *) NULL)
		return (FALSE);

	findpos(wp, begin, &passlinkp, &pos);
	linkp = passlinkp;
	from = linkp->cont + pos;
	nextsize = linkp->size - pos;

	while (begin < end) {
		amount = min(end-begin, nextsize);
		if (writesyserr(wp, f, from, amount, UC name) != amount)
			return (FALSE);
		begin += amount;
		if (begin < end) {
			linkp = linkp->next;
			readybuffer(wp, linkp);
			from = linkp->cont;
			nextsize = linkp->size;
		}
	}
	return (fflush(f) != EOF);
}

/*
 * Save a part of the current buffer into a file, write the buffer backwards.
 */
EXPORT BOOL
backsavefile(wp, begin, end, f, name)
	ewin_t	*wp;
	epos_t	begin;
	epos_t	end;
	FILE	*f;
	char	*name;
{
	register headr_t *linkp;
		headr_t *passlinkp;
	register int	amount;
		Uchar	buf[BUFFERSIZE];
	int pos = 0;
	int nextsize;
	register int	n;
		Uchar	*from;
		Uchar	*to;

#ifdef	JOS
	getpid();
#else
	seterrno(0);		/* Damit write_errno korrekt arbeitet */
#endif

	if (end > wp->eof)
		end = wp->eof;
	if (end == 0)
		return (FALSE);
	if (begin >= end)
		return (FALSE);
	if (f == (FILE *) NULL)
		return (FALSE);

	findpos(wp, end-1, &passlinkp, &pos);
	linkp = passlinkp;
	from = linkp->cont + pos;
	nextsize = pos + 1;

	while (end > begin) {
		amount = min(end-begin, nextsize);
		end -= amount;
		n = amount;
		to = buf;
		while (--n >= 0) {
			*to++ = *from--;
		}
		if (writesyserr(wp, f, buf, amount, UC name) != amount)
			return (FALSE);
		if (end > begin) {
			linkp = linkp->prev;
			readybuffer(wp, linkp);
			from = linkp->cont + linkp->size - 1;
			nextsize = linkp->size;
		}
	}
	return (fflush(f) != EOF);
}

/*
 * Get the content of a file and insert it 'curnum' times at dot position.
 * The dot position remains at the old position.
 */
EXPORT void
getfile(wp, f, size, name)
	ewin_t	*wp;
	FILE	*f;
	epos_t	size;
	char	*name;
{
	Uchar	buf[BUFSIZ];
	epos_t 	left;
	int	this;
	long	result;
	register ecnt_t	n = wp->curnum;
	epos_t	save = wp->dot;

	if (size <= 0)
		return;

	while (--n >= 0) {
		lseek(fdown(f), (off_t)0, SEEK_SET);
		for (left = size; left > 0; ) {
			if (left > sizeof (buf))
				this = sizeof (buf);
			else
				this = (int)left;
			if ((result = readsyserr(wp, f, buf, this, UC name)) < 0)
				break;
			insert(wp, buf, result);
			left -= result;
		}
	}
	dispup(wp, wp->dot, save);
	wp->dot = save;
	modified(wp);
}

/*
 * Get the content of a file and insert it backwards 'curnum' times at
 * dot position.
 * Then put the dot after the inserted characters.
 */
EXPORT void
backgetfile(wp, f, size, name)
	ewin_t	*wp;
	FILE	*f;
	epos_t 	size;
	char	*name;
{
	Uchar	buf[BUFSIZ];
	Uchar	rbuf[BUFSIZ];
	epos_t	left;
	int	this;
	long	result;
	register ecnt_t	n = wp->curnum;
	epos_t	save = wp->dot;

	if (size <= 0)
		return;

	while (--n >= 0) {
		for (left = size; left > 0; ) {
			if (left > sizeof (buf))
				this = sizeof (buf);
			else
				this = (int)left;
			left -= this;
			lseek(fdown(f), (off_t)left, SEEK_SET);
			if ((result = readsyserr(wp, f, buf, this, UC name)) < 0)
				break;
			reversebuffer(buf, rbuf, result);
			insert(wp, rbuf, result);
		}
	}
	dispup(wp, wp->dot, save);
	modified(wp);
}

/*
 * Reverse the content of a buffer while copying it to another location
 */
LOCAL void
reversebuffer(from, to, size)
	register Uchar	*from;
	register Uchar	*to;
		long	size;
{
	to += (size-1);
	while (--size >= 0)
		*to-- = *from++;
}

/*
 * Look into the first block of the buffer memory and find out whether
 * it is using DOS newlines.
 */
EXPORT BOOL
isdos(wp)
	ewin_t	*wp;
{
	register headr_t *linkp;
		headr_t *passlinkp;
		int	pos;
		int	linksize;
		Uchar	*from;
	register Uchar	*cp;
		int	dosnlcnt = 0;

	if (wp->eof <= (epos_t)0)
		return (FALSE);

	findpos(wp, (epos_t)0, &passlinkp, &pos);
	linkp = passlinkp;
	linksize = linkp->size - pos;
	cp = from = linkp->cont + pos;

	while (--linksize >= 0) {
		register Uchar	c;

		c = *cp++;
		if (c == '\0')			/* Is binary, cannot be DOS */
			return (FALSE);
		if (c == '\n') {
			if ((cp - from) < 2)	/* First char is Newline    */
				return (FALSE);

			if (cp[-2] == '\r') {
				dosnlcnt++;
				continue;
			}
			if (dosnlcnt > 0) {
				error(
				"Non DOS newline at %d after (line %d).\r\n",
				linkp->size - pos - linksize - 1, dosnlcnt+1);
			}
			return (FALSE);		/* Found '\n' without '\r'  */
		}
	}
	return (dosnlcnt > 0);
}

/*
 * Extract a portion of the buffer memory.
 * Fill in up to 'size' characters and return the amount of characters read.
 * NOTE: a NULL byte is added to the extracted string so it must be able
 *	 to hold up to size + 1 characters.
 */
EXPORT int
extract(wp, begin, str, size)
	ewin_t	*wp;
	epos_t	begin;
	Uchar	*str;
	int	size;
{
	register headr_t *linkp;
		headr_t *passlinkp;
		int	pos;
		int	savesize = size;
		int	amount;
		int	linksize;
		Uchar	*out = str;
		Uchar	*from;

/*	if (begin > wp->eof) {*/
	if (begin >= wp->eof) {
		*out = 0;
		return (0);
	}
	findpos(wp, begin, &passlinkp, &pos);
	linkp = passlinkp;
	linksize = linkp->size - pos;
	from = linkp->cont + pos;

	while (size) {
		amount = min(size, linksize);
		movebytes(C from, C out, (int)amount);
		size -= amount;
		out += amount;
		if (size) {
			linkp = linkp->next;
			if (! linkp)
				break;
			readybuffer(wp, linkp);
			linksize = linkp->size;
			from = linkp->cont;
		}
	}
	amount = savesize - size;
	str[amount] = '\0';
	return (amount);
}

/*
 * Extract a portion of the buffer memory,
 * stop after extracting a new-line character.
 * Fill in up to 'size' characters and return the amount of characters read.
 * NOTE: a NULL byte is added to the extracted string so it must be able
 *	 to hold up to size + 1 characters.
 */
EXPORT int
extr_line(wp, begin, str, size)
	ewin_t	*wp;
	epos_t	begin;
	char	*str;
	int	size;
{
	register headr_t *linkp;
		headr_t *passlinkp;
		int	pos;
		int	length = 0;
		int	amount;
		int	linksize;
		Uchar	*from;

/*	if (begin > wp->eof) {*/
	if (begin >= wp->eof) {
		*str = 0;
		return (0);
	}
	findpos(wp, begin, &passlinkp, &pos);
	linkp = passlinkp;
	linksize = linkp->size - pos;
	from = linkp->cont + pos;

	while (size) {
		amount = min(size, linksize);
		while (amount-- > 0) {
			size--, length++;
			if ((*str++ = *from++) == '\n') {
				size = 0;
				break;
			}
		}
		if (size) {
			linkp = linkp->next;
			if (! linkp)
				break;
			readybuffer(wp, linkp);
			linksize = linkp->size;
			from = linkp->cont;
		}
	}
	*str = '\0';
	return (length);
}

/*
 * Extract a portion of the buffer memory, after searching backwards for
 * the previous new-line, stop after extracting a new-line character.
 * Fill in up to 'size' characters and return the amount of characters read.
 * The 'begin' character position is not extracted to make it complementary
 * to 'extr_line' and allow symmetrical behavior in search functions.
 * The new-line that was found before 'begin' is not included.
 * NOTE: a NULL byte is added to the extracted string so it must be able
 *	 to hold up to size + 1 characters.
 */
EXPORT int
retractline(wp, begin, str, size)
	ewin_t	*wp;
	epos_t	begin;			/* Start searching backw. from here */
	char	*str;
	int	size;
{
	epos_t newpos;

	if (begin == 0) {
		*str = 0;
		return (0);
	}
	if (begin == 1)
		return (extr_line(wp, (epos_t)0, str, min(size, 1)));
	newpos = srevchar(wp, begin-2, '\n') + 1;
	if (newpos >= wp->eof)
		newpos = 0;
	if (begin-newpos > size)
		newpos = begin-size;
	return (extr_line(wp, newpos, str, (int)min(size, begin-newpos)));
}

/*
 * Move the gap to a new position.
 * First remove the current gap with compressbuffer().
 * Then find the header where the new gap position is in and compress the
 * buffer in case it has a leading gap. This helps us not to waste too much
 * space with partially filled buffers.
 * If the new gap position is at the end of the buffer, we are done.
 * If not, we must split the buffer at the new gap position.
 * When we are done, the gap is at the end if the current buffer.
 */
LOCAL void
movegap(wp, pos)
	ewin_t	*wp;
	epos_t	pos;
{
#ifdef	DEBUG
	cdbg("movegap(%lld) gaplink: %p", (Llong)pos, (void *)wp_gaplink(wp));
#endif
	if (pos == wp->gap && wp_gaplink(wp) != (headr_t *)0)
		return;
	if (wp_gaplink(wp))
		compressbuffer(wp, wp_gaplink(wp));
	findpos(wp, wp->gap = pos, (headr_t **)&wp->gaplink, &wp->gapoff);
	if (wp_gaplink(wp)->buf != wp_gaplink(wp)->cont)
		compressbuffer(wp, wp_gaplink(wp));
	if (wp_gaplink(wp)->size > wp->gapoff)
		splitbuffer(wp, wp_gaplink(wp), wp->gapoff);
}

/*
 * If we are removing the buffer that is pointed to by winlink, we
 * must invalidate winlink.
 */
EXPORT void
clearifwpos(wp, this)
	ewin_t	*wp;
	headr_t		*this;
{
#ifdef	FASTPOS
	if (this == wp->winlink)
		clearwpos(wp);
#endif
}

#ifdef	FASTPOS

/*
 * Invalidate winlink.
 *
 * 'winlink' wird ungültig wenn:
 *	1)	der Header entfernt wird auf den 'winlink' zeigt
 *	2)	vor 'winoff' eingefügt oder gelöscht wird
 */
EXPORT void
clearwpos(wp)
	ewin_t	*wp;
{
#ifdef	XPOSDEBUG
	writeerr(wp, "winlink: %p winpos: %lld winoff: %lld",
		wp->winlink, (Llong)wp->winpos, (Llong)wp->winoff);
	sleep(1);
#endif
	/*
	 * Das Neuberechnen dauert bei 200000 Headern mit einer Sparcstation-2
	 * ca. 0,25 Sekunden. Das entspricht bei der aktuellen Buffersize
	 * einer Datei von 1,5 GB.
	 * Gibt es einen besseren Weg als loeschen?
	 */
	wp->winlink = NULL;
	wp->winpos = 0L;
	wp->winoff = 0L;
}

/*
 * Try to move winpos/winlink backwards.
 */
/* ARGSUSED */
EXPORT void
backwpos(wp)
	ewin_t	*wp;
{
	/*
	 * Das geht nicht!!! Es ist hier nur als Anregung.
	 */
#ifdef	used
	if (winlink(wp)->prev != NULL) {
		wp->winlink = winlink(wp)->prev;
		wp->winpos -= winlink(wp)->size;
	}
	wp->winoff = 0L;
#endif
}

/*
 * Find current window start position.
 */
EXPORT void
findwpos(wp, new)
	ewin_t	*wp;
	epos_t	new;
{
	int	xp;

	findpos(wp, new, (headr_t **)&wp->winlink, &xp);
	wp->winpos = new;	/* The position of the start of the window   */
	wp->winoff = new - xp;	/* The position of the first char in winlink */
}

#else	/* FASTPOS */

/*
 * Find current window start position (dummy).
 */
EXPORT void
findwpos(new)
	epos_t	new;
{
}

#endif	/* FASTPOS */

/*
 * Find link pointer and offset in found buffer for 'pos'.
 */
EXPORT void
findpos(wp, pos, returnlinkp, returnoff)
		ewin_t	*wp;
	register epos_t	pos;
	headr_t		**returnlinkp;	/* To return found link header */
	int		*returnoff;	/* To return found link offset */
{
	register headr_t	*linkp = wp->bhead;
#ifdef	FASTPOS
	epos_t	SAVE = pos;
#ifdef	TIMEPOS
	struct timeval t1, t2;

	gettimeofday(&t1, 0);
#endif	/* TIMEPOS */
#endif	/* FASTPOS */

/*cdbg("findpos(%d) caller %x", pos, getcaller());*/
	if (pos > wp->eof)
		pos = wp->eof;

#ifdef	FASTPOS
	if (wp->eof < wp->winoff)
		clearwpos(wp);

#ifdef	XPOSDEBUG
	if (wp->winlink) {
		if (((headr_t *)wp->winlink)->flags & INVALID)
			cdbg("winoff: %lld INVALID", (Llong)wp->winoff);
		if (((headr_t *)wp->winlink)->next == 0) {
			cdbg("winoff: %lld winoff+winlink->size: %lld eof: %lld",
				(Llong)wp->winoff,
				(Llong)wp->winoff+((headr_t *)wp->winlink)->size,
				(Llong)wp->eof);
		}
	}
#endif
	if (wp->winlink) {
		if (pos >= wp->winoff) {
			linkp = wp->winlink;
			pos -= wp->winoff;
		} else {
			rfindpos(wp, pos, returnlinkp, returnoff);
#ifdef	CHECKPOS
			goto checkpos;
#else
			return;
#endif
		}
	}
#endif	/* FASTPOS */
	/*
	 * Find header that contains 'pos'
	 */
	while (linkp->size <= pos) {
		pos -= linkp->size;
		linkp = linkp->next;
		if (linkp == 0 || linkp->flags & INVALID) {		/* Paranoia */
			rsttmodes(wp);
#ifdef	FASTPOS
			error("findpos: winlink: %p winpos: %lld winoff: %lld pos: %lld eof: %lld\n",
				(void *)wp->winlink,
				(Llong)wp->winpos, (Llong)wp->winoff,
				(Llong)SAVE, (Llong)wp->eof);
			error("findpos: winlink->next: %p\n", (void *)wp_winlink(wp)->next);
			if (linkp)
				error("linkp: %p linkp->flags = %X\n", (void *)linkp, linkp->flags);
#endif
			error("pos: %lld linkp: %p\n", (Llong)pos, (void *)linkp);
			error("\nBAD POSITION TO BE FOUND\n");
			flushprot(wp);
			abort();
		}
	}
	readybuffer(wp, linkp);
/*cdbg("findpos() returnoff: %d linkp: %X", pos, linkp);*/
	*returnlinkp = linkp;
	*returnoff = (int)pos;
#ifdef	FASTPOS
#ifdef	CHECKPOS
checkpos:
	;
#endif
#ifdef	TIMEPOS
	gettimeofday(&t2, 0);
	t2.tv_sec -= t1.tv_sec;
	t2.tv_usec -= t1.tv_usec;
	while (t2.tv_usec < 0) {
		t2.tv_usec += 1000000;
		t2.tv_sec -= 1;
	}
	if (t2.tv_usec)
		cdbg("time: %d.%06d", t2.tv_sec, t2.tv_usec);
#endif	/* TIMEPOS */
#ifdef	CHECKPOS
#ifdef	TIMEPOS
	gettimeofday(&t1, 0);
#endif
	/*
	 * Uncomment to force a difference that needs to be found.
	 */
/*	(*returnoff)++;*/
	ckfindpos(wp, SAVE, returnlinkp, returnoff);
#ifdef	TIMEPOS
	gettimeofday(&t2, 0);
	t2.tv_sec -= t1.tv_sec;
	t2.tv_usec -= t1.tv_usec;
	while (t2.tv_usec < 0) {
		t2.tv_usec += 1000000;
		t2.tv_sec -= 1;
	}
	if (t2.tv_usec)
		cdbg("Time: %d.%06d", t2.tv_sec, t2.tv_usec);
#endif	/* TIMEPOS */
#endif	/* CHECKPOS */
#endif	/* FASTPOS */
}

#ifdef	FASTPOS
#ifdef	CHECKPOS
/*
 * Find link pointer and offset in found buffer for 'pos'.
 * This is the old version that may be called after the new one and
 * checks for diffs.
 */
LOCAL void
ckfindpos(wp, pos, returnlinkp, returnoff)
		ewin_t	*wp;
	register epos_t	pos;
	headr_t		**returnlinkp;	/* To return found link header */
	int		*returnoff;	/* To return found link offset */
{
	register headr_t	*linkp = wp->bhead;

/*cdbg("findpos(%d) caller %x", pos, getcaller());*/
	if (pos > wp->eof)
		pos = wp->eof;

	/*
	 * Find header that contains 'pos'
	 */
	while (linkp->size <= pos) {
		pos -= linkp->size;
		linkp = linkp->next;
		if (linkp == 0 || linkp->flags & INVALID) {		/* Paranoia */
			rsttmodes(wp);
			error("pos: %d linkp: %X\n", pos, linkp);
			error("\nBAD POSITION TO BE FOUND\n");
			flushprot(wp);
			abort();
		}
	}
	readybuffer(wp, linkp);
/*cdbg("ckfindpos() returnoff: %d linkp: %X", pos, linkp);*/
	if (*returnlinkp != linkp || *returnoff != pos) {
		cdbg("DIFF: %X o%X %d o%d caller: %X", *returnlinkp, linkp, *returnoff, pos,
			getcaller());
		writeerr(wp, "DIFF: %X o%X %d o%d", *returnlinkp, linkp, *returnoff, pos); sleep(10);
	}
	*returnlinkp = linkp;
	*returnoff = (int)pos;
}
#endif	/* CHECKPOS */

/*
 * Find link pointer and offset in found buffer for 'pos'.
 * This is a version that only counts backwards from winlink.
 */
LOCAL void
rfindpos(wp, pos, returnlinkp, returnoff)
		ewin_t	*wp;
	register epos_t pos;
	headr_t		**returnlinkp;	/* To return found link header */
	int		*returnoff;	/* To return found link offset */
{
	register headr_t	*linkp;
	register epos_t		linkpos;
epos_t	SAVE = pos;

#ifdef	TEST_FROM_EOF
	linkp = wp->bhead;
	while (linkp->next)
		linkp = linkp->next;
	linkpos = wp->eof + 1;			/* XXX see buffer.c/initbufs() */
	linkpos -= linkp->size;
#else
	linkp = wp->winlink;
	linkpos = wp->winoff;
#endif
/*	cdbg("last: %X", linkp);*/

	while (linkpos > pos) {
		linkp = linkp->prev;
		if (linkp == 0 || linkp->flags & INVALID) {		/* Paranoia */
			rsttmodes(wp);
#ifdef	FASTPOS
			error("rfindpos: winlink: %p winpos: %lld winoff: %lld pos: %lld eof: %lld\n",
				(void *)wp->winlink,
				(Llong)wp->winpos, (Llong)wp->winoff,
				(Llong)SAVE, (Llong)wp->eof);
			if (linkp)
				error("linkp: %p linkp->flags = %X\n", (void *)linkp, linkp->flags);
#endif
			error("pos: %lld linkp: %p\n", (Llong)pos, (void *)linkp);
			error("\nBAD POSITION TO BE FOUND\n");
			flushprot(wp);
			abort();
		}
		linkpos -= linkp->size;
	}
	readybuffer(wp, linkp);

/*	cdbg("pos: %d linkp: %X linksz: %d lpos: %d loff: %d next: %X",*/
/*	pos, linkp, linkp->size, linkpos, pos - linkpos, linkp->next);*/

	pos -= linkpos;
	*returnlinkp = linkp;
	*returnoff = (int)pos;
}
#endif	/* FASTPOS */
