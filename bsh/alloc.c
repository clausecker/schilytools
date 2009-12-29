/* @(#)alloc.c	1.51 09/10/23 Copyright 1985,1988,1991,1995-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)alloc.c	1.51 09/10/23 Copyright 1985,1988,1991,1995-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 1985,1988,1991,1995-2009 J. Schilling
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

#ifdef	NO_FAST_MALLOC
#undef	FAST_MALLOC
#endif
#ifdef	D_MALLOC
#undef	FAST_MALLOC
#	define	ADEBUG		/* Mit debug Funktionen aprintfree()... */
#	define	XADEBUG		/* Mit Heap Damage Ckeck */
#	define	DXADEBUG
#endif
#ifdef	BSH
#	define	ADEBUG		/* Mit debug Funktionen aprintfree()... */
#	define	XADEBUG		/* Mit Heap Damage Ckeck */
#	define	DXADEBUG
#endif	/* BSH */

#include <schily/mconfig.h>

/*#define	NO_USER_MALLOC*/

#ifndef	HAVE_SBRK
#define	NO_USER_MALLOC
#endif
#ifndef	NO_USER_MALLOC
#define	malloc	__orig_malloc__
#define	calloc	__orig_calloc__
#define	realloc	__orig_realloc__
#define	free	__orig_free__
#define	cfree	__orig_cfree__
#endif

#include <schily/utypes.h>
#include <schily/stdlib.h>

#if defined(ADEBUG) || defined(BSH)
#	include <schily/stdio.h>
#	include <schily/ctype.h>
#endif

#ifdef	DXADEBUG
#	define	XADEBUG
#endif
#ifdef	XADEBUG
#	include <schily/signal.h>
#endif

#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/utypes.h>
#include <schily/align.h>

#ifndef	NO_USER_MALLOC
#undef	malloc
#undef	calloc
#undef	realloc
#undef	free
#undef	cfree
#endif

#include <schily/schily.h>

#ifndef	NO_USER_MALLOC

#define	CHECKFREE
/*#define	MEMRAISE*/
/*#define	ALLOWSMALLINC*/

#ifdef	FAST_MALLOC	/* Fast malloc() disables debugging */
#undef	XADEBUG
#undef	DXADEBUG
#undef	CHECKFREE
#endif

/*
 *	FREE STORAGE PACKAGE
 *
 *	modeled after BCPL package from MIT
 *
 *	Copyright (c) 1985 J. Schilling
 *
 *	Bemerkungen zu den globalen Variablen:
 *
 *	heapbeg		Erster Block im Heap
 *
 *	high		Letzter gueltiger Block im Heap
 *
 *	heapend		Pointer zu fiktivem Block direkt hinter dem Ende des
 *			gueltigen Heaps
 *
 *	avail		Die gesammte Kette der freien Eintraege
 *			nach aufsteigenden Adressen geordnet. Nur avail.sfree
 *			wird benutzt.
 *
 *	last		Ein Platz von dem zuerst versucht wird, zu allozieren
 *			in der Hoffnung, zu verhindern, dasz sich kleine
 *			Stuecke am Anfang des Heaps haeufen. nur last.sfree
 *			wird benutzt.
 *
 *	Mit Debugging:
 *	Niedrige Adressen
 *	-----------------------------------------------------------------------
 *	| struct space *snext
 *	-----------------------------------------------------------------------
 *	| struct space *sfree
 *	-----------------------------------------------------------------------
 *	| ...				Wenn XADEBUG, dann Debug Infos
 *	-----------------------------------------------------------------------
 *	| struct space *store		Erste gueltige Nutzadresse v. malloc()
 *	-----------------------------------------------------------------------
 *	| ...				Weiterer Platz
 *	-----------------------------------------------------------------------
 *	| 				Letze gueltige Nutzadresse				
 *	-----------------------------------------------------------------------
 *	| 0x55				Wenn XADEBUG & size%malign -> Endmarker
 *	-----------------------------------------------------------------------
 *	| ...				Wenn 0x55, evt. Alignement (leer)
 *	-----------------------------------------------------------------------
 *	| ...				evt. Padding bis zum naechsten Block
 *	-----------------------------------------------------------------------
 *	| 0x12345678			Wenn XADEBUG, Debug Magic
 *	-----------------------------------------------------------------------
 *	| size				Wenn XADEBUG, malloc() size Parameter 
 *	-----------------------------------------------------------------------
 *	Hohe Adressen
 *
 *	Ohne Debugging:
 *	Niedrige Adressen
 *	-----------------------------------------------------------------------
 *	| struct space *snext
 *	-----------------------------------------------------------------------
 *	| struct space *sfree
 *	-----------------------------------------------------------------------
 *	| struct space *store		Erste gueltige Nutzadresse v. malloc()
 *	-----------------------------------------------------------------------
 *	| ...				Weiterer Platz
 *	-----------------------------------------------------------------------
 *	| 				Letze gueltige Nutzadresse				
 *	-----------------------------------------------------------------------
 *	| ...				evt. Padding bis zum naechsten Block
 *	-----------------------------------------------------------------------
 *	Hohe Adressen
 *
 *	Allegmeine Hinweise:
 *
 *	Der Heap ist ein groszes Stueck Speicher, dasz auf heapbeg anfaengt
 *	und in Stuecke geteilt ist, die zwischen benutzt und frei wechseln.
 *	Das ist deshalb so, weil alle aufeinander folgenden freien Stuecke
 *	zu einem groszen vereinigt werden. Jedes Stueck hat immer einen
 *	Pointer zu dem naechsten Stueck (snext) um einfach durch den gesammten
 *	Heap zu kommen und die Bestimmung der Groesze jedes
 *	Stueckes (snext - this) ermoeglicht.
 *
 *	Die gesammte Freispeicher Liste beginnt mit avail und benutzt den
 *	ersten benutzbaren Pointer in einem Stueck (das erste Longwort nach
 *	dem snext Pointer) zur Verwaltung der Liste. Dieser Pointer mit
 *	referenzen zu jedem Stueck ist Stueckaddr->sfree.
 *
 *	Zusaetzlich gibt es last, der irgendwo in die Mitte der Freispeicher
 *	Liste zeigt und benutzt wird, um einen freien Block zu finden um die
 *	Moeglichkeit zu verringern, dasz alle kleinen Stuecke am Anfang des
 *	Heaps beginnen, wie es oben beschrieben wurde.
 *
 *	Achtung: Bei SunOS 4.x ist size_t ein signed int. Es ist daher nicht
 *		moeglich dort mehr als maxint zu allozieren, was aber kein
 *		ernsthaftes Problem sein duerfte. Ohne die Verwendung von
 *		size_t ist es aber eine Benutzung unter einer 64 Bit
 *		Architektur nicht moeglich.
 */


/*#define	store		sfree*/
/* XXX mit #define store get realloc nicht (siehe unten) */

typedef struct space {
	struct space *snext;	/* Next storage chunk			*/
	struct space *sfree;	/* Next free storage chunk		*/
#ifdef	XADEBUG
	char	*file;		/* File name for calling function	*/
	int	line;		/* Line number for calling function	*/
#endif
#ifndef	store
	struct space *store;	/* This is where the storage space is	*/
#endif
} SPACE;

#undef	max
#define	max(a, b)	((a) > (b) ? (a) : (b))
#undef	roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#define	chunkaddr(t)	((SPACE *)(((char *)(t))-((char *)&((SPACE *)0)->store)))
#define	chunksize(this)	(((char *)(this)->snext) - ((char *)(this)))
#define	storesize(this)	(((char *)(this)->snext) - ((char *)&(this)->store))

#define	FIRSTINC	1024
#define	MAXINC		8192


LOCAL	SPACE	*heapbeg	= (SPACE *)NULL;
LOCAL	SPACE	*heapend	= (SPACE *)NULL;
LOCAL	SPACE	*high		= (SPACE *)NULL;

LOCAL	SPACE	avail		= { (SPACE *)NULL, (SPACE *)NULL};
LOCAL	SPACE	last		= { (SPACE *)NULL, (SPACE *)NULL};

LOCAL	size_t	meminc		= FIRSTINC;
#ifdef	CHECKFREE
LOCAL	short	checkfree	= TRUE;
#else
LOCAL	short	checkfree	= FALSE;
#endif
#ifdef	MEMRAISE
LOCAL	short	raisenomem	= FALSE;
#endif

/*
 *	Condition names for handlecond()
 */
#ifdef	MEMRAISE
#ifdef	BSH
#	define	nomem	sn_no_mem
extern	char	nomem[];
#else
LOCAL	char	nomem[]		= "no_memory";
#endif	/* BSH */
#endif
LOCAL	char	outofheap[]	= "blk_out_of_heap";
LOCAL	char	notinheap[]	= "blk_not_in_heap";

#ifndef	HAVE_BRK
#define	brk	__brk
EXPORT	int	brk		__PR((void *endds));
#endif
LOCAL	SPACE	*_extcor	__PR((size_t size));
LOCAL	SPACE	*init		__PR((void));
EXPORT	void	free		__PR((void *t));
LOCAL	BOOL	frext		__PR((size_t size));
#ifdef	XADEBUG
EXPORT	void	*dbg_malloc	__PR((size_t size, char *file, int line));
EXPORT	void	*dbg_calloc	__PR((size_t nelem, size_t elsize, char *file, int line));
EXPORT	void	*dbg_realloc	__PR((void *t, size_t size, char *file, int line));
#endif
EXPORT	void	*malloc		__PR((size_t size));
EXPORT	void	*calloc		__PR((size_t nelem, size_t elsize));
EXPORT	void	cfree		__PR((void *t));
EXPORT	void	*realloc	__PR((void *t, size_t size));
#ifndef	BSH
EXPORT	size_t	psize		__PR((char *t));
EXPORT	void	freechecking	__PR((BOOL val));
EXPORT	void	nomemraising	__PR((BOOL val));
#endif	/* BSH */
#ifdef	ADEBUG
EXPORT	void	aprintfree	__PR((FILE * f));
LOCAL	int	afpchar		__PR((FILE * f, int c));
LOCAL	void	aprints		__PR((FILE * f, SPACE *s, char *str));
#ifdef	XADEBUG
LOCAL	void	adbgmark	__PR((SPACE * this, size_t xsize));
LOCAL	BOOL	aisdamaged	__PR((SPACE * this));
LOCAL	void	adbgdamaged	__PR((SPACE * this));
LOCAL	void	aprdamaged	__PR((FILE * f, SPACE *this));
#endif
EXPORT	BOOL	acheckdamage	__PR((void));
LOCAL	void	aprintx		__PR((FILE * f, SPACE *s, long l));
EXPORT	void	aprintlist	__PR((FILE * f, long l));
EXPORT	void	aprintchunk	__PR((FILE * f, long l));
#endif	/* ADEBUG */

EXPORT	void	*get_heapbeg	__PR((void));
EXPORT	void	*get_heapend	__PR((void));

#ifndef	HAVE_BRK
EXPORT int
brk(endds)
	void	*endds;
{
	void		*curend = sbrk((Intptr_t)0);
	Intptr_t	incr;

	incr = ((char *)endds) - ((char *)curend);
	if (sbrk(incr) == (void *)-1)
		return (-1);
	return (0);
}
#endif

/*---------------------------------------------------------------------------
|
|	Erweiterung des Heaps durch neuen zusammenhaengenden Speicher vom
|	System
|
|	Es wird versucht soviel zu bekommen, wie benoetigt wird.
|
|	Jedes mal, wenn neuer Speicher benoetigt wird, wird die Menge
|	verdoppelt bis MAXINC erreicht ist.
|	Eine hoehere Schicht behandelt das Problem, wenn kein Speicher
|	mehr da ist.
|
+---------------------------------------------------------------------------*/
LOCAL SPACE *
_extcor(size)
	register size_t	size;
{
	register	SPACE	*oldend = heapend;
	register	SPACE	*newend;

	if (meminc > size)
		size = meminc;
	if ((size % meminc) != 0)
		size = roundup(size, meminc);

	newend = (SPACE *)((char *)oldend + size);
	if (brk((char *)newend) == -1) {
#ifdef ALLOWSMALLINC
		meminc = FIRSTINC;
		newend = (SPACE *)((char *)oldend + meminc);
		if (brk((char *)newend) == -1)
			return ((SPACE *)NULL);
		else
			heapend = newend;
#else
		return ((SPACE *)NULL);
#endif
	} else {
		heapend = newend;
	}
	if (meminc < MAXINC)
		meminc <<= 1;
	return (oldend);
}

/*---------------------------------------------------------------------------
|
|	Initialisierung der Freispeicherverwaltung
|
+---------------------------------------------------------------------------*/
LOCAL SPACE *
init()
{
	size_t	inc	= 0;
	size_t	pgsize	= 0;

	if (heapbeg != (SPACE *)NULL)
		return (high);

#ifdef	_SC_PAGESIZE
	pgsize = sysconf(_SC_PAGESIZE);
#else
	pgsize = getpagesize();
#endif
	heapbeg = heapend = (SPACE *)sbrk((Intptr_t)0);
	if (!maligned(heapbeg)) {	/* Should never happen		*/
		heapbeg = heapend = (SPACE *)malign(heapbeg);
		if (brk((char *)heapbeg) == -1)
			return ((SPACE *)NULL);
	}
	if (pgsize > 0) {
		inc = pgsize - ((UIntptr_t)heapbeg) % pgsize;
		if (inc < sizeof (SPACE))
			inc += pgsize;
		meminc = 1;		/* damit _extcor() nicht aufrundet */
	}
	if ((high = avail.sfree = last.sfree = _extcor(inc)) != (SPACE *)NULL)
		high->snext = high->sfree = heapend;
	if (pgsize > 0)
		meminc = pgsize;
	return (high);
}

/*---------------------------------------------------------------------------
|
|	free : Ein Stueck wird freigegeben ...
|
|	Ueberprufung des Stuecks:
|		1. Test ob es sich innerhalb des richtigen Adressbereiches
|		   befindet.
|		2. Suchen des Stuecks im Heap, wenn checkfree TRUE ist
|
|	Vereinigung aller zusammenhaengenden Stuecke
|
|	Korrigieren von last und high
|
+---------------------------------------------------------------------------*/
/* VARARGS1 */
EXPORT void
free(t)
	void	*t;
{
	register	SPACE	*this = chunkaddr(t); /* zeigt auf Blockstart*/
	register	SPACE	*prev;
	register	SPACE	*next;

#ifdef	XADEBUG
	(void) acheckdamage();
#endif
	if (t == NULL)
		return;
	if (this < heapbeg || this >= heapend) {
		if (checkfree)
			raisecond(outofheap, 0L);
		return;
	}

	if (checkfree) {
		/*
		 *	Sucht den Block (this) im Heap indem die gesammte
		 *	Freispeicherliste von heapbeg verkettet durch
		 *	snext durchsucht wird.
		 */
		for (next = heapbeg; this > next; next = next->snext)
			;
		if (this != next) {
			raisecond(notinheap, 0L);
		}
	}
#ifdef	XADEBUG
	if (aisdamaged(this)) {
		void	(*osigquit) __PR((int));

		adbgdamaged(this);

		osigquit = signal(SIGQUIT, SIG_DFL);
		sleep(1);
		signal(SIGQUIT, osigquit);
		raisecond("heap damaged", 0L);
	}
#endif

	/*
	 *	Suchen der freien Stuecke vor und nach 'this'
	 */
	if (last.sfree <= this)
		prev = &last;
	else
		prev = &avail;

	for (next = prev->sfree; next <= this; prev = next, next = next->sfree)
		;

	if (prev->snext == this) {
		/*
		 *	prev und this passen zusammen
		 *	sie werden vereinigt
		 */
		prev->snext = this->snext;
		if (this >= high)	/* >= denn sicher ist sicher */
			high = prev;
		this = prev;
	} else {
		/*
		 *	sonst Einhaengen von this in die Kette
		 *	dabei wird ggf. auch avail.sfree korrigiert
		 */
		prev->sfree = this;
		this->sfree = next;
	}

	if (this->snext == next && next < heapend) {
		/*
		 *	this und next passen zusammen
		 *	sie werden vereinigt
		 */
		if (next >= high)
			high = this;
		this->snext = next->snext;
		this->sfree = next->sfree;
	}
	last.sfree = prev;
}

/*---------------------------------------------------------------------------
|
|	Erweitert den Heap
|
+---------------------------------------------------------------------------*/
LOCAL BOOL
frext(size)
			size_t	size;
{
	register	SPACE	*new;
	register	SPACE	*prev;

	if ((new = _extcor(size)) == (SPACE *)NULL)
		return (FALSE);

	new->snext = heapend;
	high->snext = new;
	for (prev = &last; prev->sfree != new; prev = prev->sfree);
	prev->sfree = heapend;
	last.sfree = prev;
#ifdef	XADEBUG
	prev = (struct space *)((char *)new->snext-2*sizeof (struct space *));
	prev->snext = (struct space *)0x12345678;
	prev->sfree = (struct space *)chunksize(new);
#endif
	(void) free((char *)&new->store);
	return (TRUE);
}

#ifdef	XADEBUG
EXPORT void *
dbg_malloc(size, file, line)
	register	size_t	size;
			char	*file;
			int	line;
{
	void		*ret = malloc(size);
	register	SPACE	*this;

	if (ret == 0)
		return (ret);

	this = chunkaddr(ret); /* zeigt auf Blockstart*/
	this->file = file;
	this->line = line;

	return (ret);
}

EXPORT void *
dbg_calloc(nelem, elsize, file, line)
			size_t	nelem;
	register	size_t	elsize;
			char	*file;
			int	line;
{
	void		*ret = calloc(nelem, elsize);
	register	SPACE	*this;

	if (ret == 0)
		return (ret);

	this = chunkaddr(ret); /* zeigt auf Blockstart*/
	this->file = file;
	this->line = line;

	return (ret);
}

EXPORT void *
dbg_realloc(t, size, file, line)
			void	*t;
	register	size_t	size;
			char	*file;
			int	line;
{
	void		*ret = realloc(t, size);
	register	SPACE	*this;

	if (ret == 0)
		return (ret);

	this = chunkaddr(ret); /* zeigt auf Blockstart*/
	this->file = file;
	this->line = line;

	return (ret);
}
#endif

/*---------------------------------------------------------------------------
|
|	Besorgt freien Speicher fuer den Benutzer
|
|	Sucht zuerst in der 'last' Liste, dann in der 'avail' Liste;
|	Dabei wird absichtlich bei last.free begonnen, damit der prev
|	Pointer korrekt initialisiert ist und kein Randproblem auftritt.
|	Wenn kein Speicher verfuegbar ist dann wird der Heap erweitert.
|
+---------------------------------------------------------------------------*/
EXPORT void *
malloc(size)
	register	size_t	size;
{
	register	SPACE	*prev;
	register	SPACE	*this;
	register	SPACE	*new;
	register	size_t	pass;
	register 	size_t	left;
#ifdef	XADEBUG
			size_t	xsize = size;
#endif

	if (heapbeg == (SPACE *)NULL) {
		if (init() == (SPACE *)NULL) {
#ifdef			MEMRAISE
			if (raisenomem)
				raisecond(nomem, 0L);
#endif
			return (NULL);
		}
	}
#ifdef	XADEBUG
	(void) acheckdamage();
#endif

	/*
	 * Die minimale Speichermenge ist struct space aber mindestens
	 * die von auszen gewuenschte Speichermenge + der Anteil in
	 * struct space der vor space->store liegt.
	 */
	size = max(size, sizeof (new->store)) + (int)&((SPACE *)0)->store;
#ifdef	XADEBUG
#ifdef	ALWAYS_55
	/*
	 * Ein Byte fuer 0x55, damit immer ein Endmarker moeglich ist.
	 */
	size += 1;
#endif
	/*
	 * Am Ende dieses Stueckes werden 2 Pointer reserviert.
	 * Der erste Pointer wird mit einem magischen Pointer initialisiert,
	 * der zweite mit der wirklich gewuenschten Speichergroesze.
	 */
	size += 2*sizeof (struct space *);
#endif

	size = (size_t)malign(size); /* runden auf naechsten "Max Size Type" */

	for (pass = 0, prev = last.sfree; ; pass++) {
		if (prev < heapend)
			for (this = prev->sfree; this < heapend;
					prev = this, this = this->sfree) {

				left = chunksize(this);
				if (left < size)
					continue;
				left -= size;

				if (left >= sizeof (SPACE)) {
					new = (SPACE *)((char *)this + size);
					new->snext = this->snext;
					this->snext = new;
					new->sfree = this->sfree;
					if (new > high)
						high = new;
					this->sfree = new;
				}
				/*
				 * dabei wird ggf. auch avail.sfree korrigiert
				 */
				prev->sfree = this->sfree;
				last.sfree = prev;
#ifdef	XADEBUG
				adbgmark(this, xsize);
#endif
				return ((char *) &this->store);
			}
		prev = &avail;
		if (pass > 0) {
			if (!frext(size)) {
#ifdef				MEMRAISE
				if (raisenomem)
					raisecond(nomem, 0L);
#endif
				return (NULL);
			}
		}
	}
}

EXPORT	void *
calloc(nelem, elsize)
	size_t	nelem;
	size_t	elsize;
{
	size_t	size = nelem * elsize;
	char	*p;

	p = malloc(size);
	if (p != NULL)
		fillbytes(p, size, '\0');
	return (p);
}

#define	valign(x, a)	(((char *)(x)) + ((a) - 1 - ((((UIntptr_t)(x))-1)%(a))))
EXPORT void *
valloc(size)
	register	size_t	size;
{
	void	*ret;
	int	pagesize;

#ifdef	_SC_PAGESIZE
	pagesize = sysconf(_SC_PAGESIZE);
#else
	pagesize = getpagesize();
#endif

	ret = malloc((size_t)(size+pagesize));
	if (ret == NULL)
		return (ret);
	ret = valign(ret, pagesize);
	return (ret);
}


EXPORT	void
cfree(t)
	void	*t;
{
	free(t);
}

#ifndef	store
/*---------------------------------------------------------------------------
|
|	Vergroeszert oder verkleinert ein Stueck aus malloc()
|	Liefert einen Pointer auf das eventuell kopierte Stueck
|
+---------------------------------------------------------------------------*/
EXPORT void *
realloc(t, size)
	register void	*t;
		size_t	size;
{
	register char	*n;
		size_t	osize;

#ifdef	XADEBUG
	(void) acheckdamage();
#endif
	if (t == NULL)
		return (malloc(size));

	osize = storesize(chunkaddr(t));

#ifdef	XXX
	(void) free(t);
	n = malloc(size);
	/*
	 * in malloc wird der alte Speicher zerstoert, wenn
	 * der neue Speicher direkt vor dem alten anfaengt und in den
	 * neuen hineinreicht !!!!!
	 */
	if (n == t || n == NULL)
		return (n);
	if (size < osize)
		osize = size;
	(void) movebytes(t, n, osize);
#else
	n = malloc(size);
	if (n != NULL) {
		if (size < osize)
			osize = size;
		(void) movebytes(t, n, osize);
		free(t);
	}
#endif
	return (n);
}
#endif	/* store */


#ifndef	BSH
/*---------------------------------------------------------------------------
|
|	Groesze des allozierten Blocks
|
+---------------------------------------------------------------------------*/
EXPORT size_t
psize(t)
	char	*t;
{
	return (storesize(chunkaddr(t)));
}

/*---------------------------------------------------------------------------
|
|	Funktion zur Umschaltung der Ueberpruefung ob Block belegt ist
|
+---------------------------------------------------------------------------*/
EXPORT void
freechecking(val)
	BOOL	val;
{
	checkfree = val;
}

/*---------------------------------------------------------------------------
|
|	Funktion zur Umschaltung der Freispeicherueberpruefung
|
+---------------------------------------------------------------------------*/
EXPORT void
nomemraising(val)
	BOOL	val;
{
#ifdef	MEMRAISE
	raisenomem = val;
#endif
}
#endif	/* BSH */

#ifndef	BSH
LOCAL	BOOL	ctlc = FALSE;
#else
#include "bsh.h"
#define	REDEFINE_CTYPE		/* Allow to use our local ctype.h */
#include "ctype.h"
EXPORT	void	balloc		__PR((Argvec* vp, FILE ** std, int flag));
#endif	/* BSH */

#ifdef	ADEBUG

EXPORT void
aprintfree(f)
	FILE	*f;
{
	register SPACE	*s;
	register SPACE	*fr;
	register size_t	nbusy;
	register size_t	nfree;

	nbusy = 0;
	nfree = 0;
	for (s = heapbeg, fr = avail.sfree; s < heapend; s = s->snext) {
		while (s < fr) {
			nbusy += chunksize(s);
			s = s->snext;
		}
		nfree += chunksize(s);
		fr = fr->sfree;
	}
	fprintf(f, "heapbeg: %p, end: %p, busy: %lld, free: %lld, total: %ld\n",
			(void *)heapbeg, (void *)heapend, (Llong)nbusy, (Llong)nfree,
			(long)((char *)heapend - (char *)heapbeg));
	fprintf(f, "avail.sfree: %p, last.sfree: %p\n",
			(void *)avail.sfree, (void *)last.sfree);
}

LOCAL int
afpchar(f, c)
	FILE	*f;
	char	c;
{
	return (putc(c, f));
}

LOCAL void
aprints(f, s, str)
		FILE	*f;
	register SPACE	*s;
		char	*str;
{
	fprintf(f, "%6p %7lld\t%6lld\t%s",
			(void *)s, (Llong)(Intptr_t)s,
			(Llong)chunksize(s), str);
}

#ifdef	XADEBUG
LOCAL void
adbgmark(this, xsize)
	register	SPACE	*this;
			size_t	xsize;
{
	register	SPACE	*new;

	new = (struct space *)((char *)this->snext-2*sizeof (struct space *));
	new->snext = (struct space *)0x12345678;
	new->sfree = (struct space *)xsize;
	if (chunksize(this) >
		(xsize + (size_t)&((SPACE *)0)->store
		+ 2*sizeof (struct space *)))
		((char *)&this->store)[xsize] = 0x55;
	this->file = "<unknown-file>";
	this->line = 0;
}

LOCAL BOOL
aisdamaged(this)
	register SPACE	*this;
{
	register SPACE	*next;

	next = (struct space *)((char *)this->snext-2*sizeof (struct space *));

	if (next < heapbeg || next > heapend)
		return (TRUE);

	return ((next->snext != (struct space *)0x12345678) ||
			(chunksize(this) >
			((size_t)next->sfree + (size_t)&((SPACE *)0)->store
			+ 2*sizeof (struct space *)) &&
			((char *)&this->store)[(long)next->sfree] != 0x55));
}

LOCAL void
adbgdamaged(this)
	register SPACE	*this;
{
	register SPACE	*next;

#ifdef	XADEBUG
	(void) acheckdamage();
#endif
	next = (struct space *)((char *)this->snext-2*sizeof (struct space *));
	error("this:  %p ", this);
	error("store: %p ", &this->store);
	error("snext: %p ", next->snext);
	error("chunksz: %lld ", (Llong)chunksize(this));
	error("size: %ld %lX ", (long)next->sfree, (long)next->sfree);
#ifdef	PR_LASTBYTE
	error("lastbyte: %CX\n", ((char *)&this->store)[(long)next->sfree]);
#else
	error("caller: file '%s' line %d ", this->file, this->line);
	error("\n");
#endif
}

LOCAL void
aprdamaged(f, this)
	FILE	*f;
	SPACE	*this;
{
	if (aisdamaged(this))
		afpchar(f, '*');
	else
		afpchar(f, ' ');
}

EXPORT BOOL
acheckdamage()
{
	register SPACE	*s;
	register SPACE	*fr;
		int	ret = FALSE;
	FILE	*f = stderr;

/*fprintf(stderr, ".");*/

	for (s = heapbeg, fr = avail.sfree; !ctlc && s < heapend;
							s = s->snext) {
		while (!ctlc && s < fr) {
			if (aisdamaged(s)) {
				ret = TRUE;
				aprints(f, s, "BUSY");
#ifdef	XADEBUG
				error(" caller: file %s line %d ",
							s->file, s->line);
				aprdamaged(f, s);
#endif
				aprintx(f, s, 1);
				fprintf(f, "\n");
return (TRUE);
			}
			s = s->snext;
		}
		if (fr >= heapend)
			break;
		fr = fr->sfree;
	}
	return (ret);
}
#else

EXPORT BOOL
acheckdamage()
{
	return (FALSE);
}
#endif	/* XADEBUG */

LOCAL void
aprintx(f, s, l)
		FILE	*f;
	register SPACE	*s;
		long	l;
{
	register Uchar	*p;
	register size_t	i;
	register int	n;

	afpchar(f, ' ');

	/* pretty_string benutzt malloc() -> geht nicht */
	p = (Uchar *)&s->store;
	i = storesize(s);
	n = 40;

	while (i-- > 0) {
		if (*p == '\0' && l == 1)
			return;
		if (isprint(*p)) {
			afpchar(f, *p++);
			if (--n < 0)
				return;
			continue;
		}
		if (*p & 0x80) {
			afpchar(f, '~');
			if (--n < 0)
				return;
		}
		afpchar(f, '^');
		if (--n < 0)
			return;
		afpchar(f, (*p++ & 0x7F) ^ 0100);
		if (--n < 0)
			return;
	}
}

EXPORT void
aprintlist(f, l)
	FILE	*f;
	long	l;
{
	register SPACE	*s;
	register SPACE	*fr;

	for (s = heapbeg, fr = avail.sfree; !ctlc && s < heapend;
							s = s->snext) {
		while (!ctlc && s < fr) {
			aprints(f, s, "BUSY");
#ifdef	XADEBUG
			aprdamaged(f, s);
#endif
			if (l)
				aprintx(f, s, l);
			fprintf(f, "\n");
			s = s->snext;
		}
		aprints(f, s, "FREE");
#ifdef	XADEBUG
		afpchar(f, ' ');
#endif
		if (l > 0)
			aprintx(f, s, l);
		fprintf(f, "\n");
		fr = fr->sfree;
	}
}

EXPORT void
aprintchunk(f, l)
	register FILE	*f;
	register long	l;
{
	register SPACE	*s;
	register char	*p;

	s = (SPACE *)malign(l);	/* runden auf naechsten "Max Size Type" */

	if (s < heapbeg || s >= heapend)
		return;

	p = (char *)&s->store;
	l = storesize(s);
	if (l + p > (char *)heapend)
		l = (char *)heapend - p;
	for (; --l >= 0 && !ctlc; p++)
		if ((*p >= ' ' && *p <= '~') || *p == '\n')
			fprintf(f, "%c", *p);
	fprintf(f, "\n");

	p = (char *)s;
	l = chunksize(s);
	if (l + p > (char *)heapend)
		l = (char *)heapend - p;
	for (; --l >= 0 && !ctlc; )
		fprintf(f, "%X ", *p++ & 0xFF);
	fprintf(f, "\n");
}
#endif	/* ADEBUG */

#ifdef	BSH

/* ARGSUSED */
EXPORT void
balloc(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
		long	l;

	if (vp->av_ac > 2) {
		wrong_args(vp, std);
	} else {
#ifdef	ADEBUG
#ifdef	XADEBUG
		(void) acheckdamage();
#endif
		if (vp->av_ac == 2) {
			if (*astol(vp->av_av[1], &l) == '\0') {
				aprintchunk(std[1], l);
			} else {
				l = 0;
				if (streql(vp->av_av[1], "l"))
					l = 1;
				if (streql(vp->av_av[1], "L"))
					l = 2;
				aprintlist(std[1], l);
			}
		}
		aprintfree(std[1]);
#else
		fprintf(std[2], "%s: not compiled with malloc debug.\n", vp->av_av[0]);
#endif	/* ADEBUG */
	}
}
#endif	/* BSH */

EXPORT void *
get_heapbeg()
{
	return (heapbeg);
}

EXPORT void *
get_heapend()
{
	return (heapend);
}

#else /* NO_USER_MALLOC */

#include <schily/stdlib.h>

EXPORT	BOOL	acheckdamage	__PR((void));
EXPORT	void	*get_heapbeg	__PR((void));
EXPORT	void	*get_heapend	__PR((void));

#ifdef	BSH
#include <schily/stdio.h>
#include "bsh.h"

EXPORT	void	balloc		__PR((Argvec* vp, FILE ** std, int flag));

/* VARARGS */
EXPORT void
balloc(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	unimplemented(vp, std);
}
#endif	/* BSH */


EXPORT BOOL
acheckdamage()
{
	return (FALSE);
}

EXPORT void *
get_heapbeg()
{
	return (NULL);
}

EXPORT void *
get_heapend()
{
	return (NULL);
}
#endif	/* NO_USER_MALLOC */
