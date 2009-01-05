/* @(#)abbrev.c	1.30 08/12/20 Copyright 1985-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)abbrev.c	1.30 08/12/20 Copyright 1985-2008 J. Schilling";
#endif
/*
 *	Abbreviation symbol handling
 *
 *	Copyright (c) 1985-2008 J. Schilling
 *
 *	.global & .local alias abbreviations are handled here
 *
 *	Exported functions:
 *		ab_read(tab, fname)		Read new abbrev table from fname
 *		ab_use(tab, fname)		Use new abbrev table from fname
 * 		ab_close(tab)			Shut down abbrev table
 *		ab_delete(tab, name)		Pop or delect abbrev from table
 *		ab_dump(tab, f, histflag)	Print all abbrev entries to file
 *		ab_insert(tab, name, val, beg)	Insert n/v pair to abbrev table
 *		ab_list(tab, pat, f, histflag)	Print matched entries to file
 *		ab_push(tab, name, val, beg)	Push n/v pair to abbrev table
 *		ab_sname(tab, fname)		Set filename for _ab_output
 *		ab_value(tab, name, beg)	Perform n/v translation for tab
 *
 *	Imported functions:
 *		any_match			from expand.c
 *		append_line			from inputc.c
 *		berror				from bsh.c
 *
 *	Imported vars:
 *		ctlc				from bsh.c
 *		ebadpattern			from str.c
 *		for_ru				.
 *		for_wct				.
 *		sn_badfile			.
 *		sn_badtab			.
 *		sn_no_mem			.
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

#include <schily/mconfig.h>
#include <stdio.h>
#include "bsh.h"
#include "abbrev.h"
#include "str.h"
#include "strsubs.h"
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/patmatch.h>

/*
 * Replacement node entry, one is allocated for each abbrev/alias replacement
 *
 *	ab_name is only set in first node of the ab_push list
 *	ab_name is NULL in further nodes of the ab_push list
 *	ab_value is NULL if the symbol has been deleted
 */
typedef	struct abent	abent_t;

struct abent {
	abent_t	*ab_next;		/* next entry in list		 */
	abent_t *ab_push;		/* next in list of old values	 */
	char	*ab_name;		/* abbreviation/alias name	 */
	char	*ab_value;		/* replacement value		 */
	BOOL	ab_begin;		/* replace only if begin of cmd	 */
};

#define	AB_NULL	(abent_t *)0

/*
 * Table entry, one is allocated for each abbreviation type.
 *
 *	at_ent contains a list of nodes for the related abbreviation table
 */
typedef struct abtab {
	abent_t	*at_ent;		/* first entry in list		 */
	char	*at_fname;		/* name of file for _ab_output() */
	char	*at_blk;		/* start of monolitic malloc()	 */
	char	*at_blkend;		/* end of monolitic malloc()	 */
} abtab_t;

LOCAL	abtab_t	ab_tabs[ABTABS]	= { {0, 0, 0, 0}};

LOCAL	abtab_t	*_ab_down	__PR((abidx_t tab));
LOCAL	abent_t	*_ab_lookup	__PR((abtab_t *ap, char *name, BOOL new));
LOCAL	abent_t	*_ab_newnode	__PR((void));
LOCAL	void	_ab_output	__PR((abtab_t *ap));
LOCAL	void	_ab_print	__PR((abidx_t tab, char *name, FILE *f, BOOL histflg));
LOCAL	void	_ab_dump	__PR((abent_t *np, FILE *f, BOOL pushed, BOOL intrflg, BOOL histflg));
LOCAL	void	_ab_list	__PR((abent_t *np, FILE *f, BOOL histflg));
LOCAL	void	_ab_match	__PR((abent_t *np, FILE *f, BOOL histflg, char *pattern, int *aux, int alt, int *state));
LOCAL	void	_ab_close	__PR((abent_t *np, abidx_t tab));
LOCAL	char	*ab_beginword	__PR((char *p, abtab_t *ap));
LOCAL	char	*ab_endword	__PR((char *p, abtab_t *ap));
LOCAL	char	*ab_endline	__PR((char *p, abtab_t *ap));
LOCAL	BOOL	ab_inblock	__PR((abidx_t tab, char *p));
EXPORT	void	ab_read		__PR((abidx_t tab, char *fname));
EXPORT	void	ab_sname	__PR((abidx_t tab, char *fname));
EXPORT	void	ab_use		__PR((abidx_t tab, char *fname));
EXPORT	void	ab_close	__PR((abidx_t tab));
EXPORT	void	ab_insert	__PR((abidx_t tab, char *name, char *val, BOOL beg));
EXPORT	void	ab_push		__PR((abidx_t tab, char *name, char *val, BOOL beg));
EXPORT	void	ab_delete	__PR((abidx_t tab, char *name));
EXPORT	char	*ab_value	__PR((abidx_t tab, char *name, BOOL beg));
EXPORT	void	ab_dump		__PR((abidx_t tab, FILE *f, BOOL histflg));
EXPORT	void	ab_list		__PR((abidx_t tab, char *pattern, FILE *f, BOOL histflg));

/*
 * Do range check for 'tab' and return abtab_t structure for 'tab'.
 */
LOCAL abtab_t *
_ab_down(tab)
	abidx_t	tab;
{
	if (tab < 0 || tab >= ABTABS)
		raisecond(sn_badtab, 0L);
	return (&ab_tabs[tab]);
}


LOCAL abent_t *
_ab_lookup(ap, name, new)
	abtab_t	*ap;
	char	*name;
	BOOL	new;
{
	register abent_t	*np;	/* Current node pointer	*/
	register abent_t	*lp;	/* Last node pinter	*/
#ifdef	notdef
	register int		cmp;
#endif

	for (lp = AB_NULL, np = ap->at_ent; np != AB_NULL; ) {
		register char	*s1;
		register char	*s2;

#ifndef notdef
		s1 = name;
		s2 = np->ab_name;
		for (; *s1 == *s2; s1++, s2++) /* viel schneller als strcmp */
			if (*s1 == '\0')
				return (np);
		if (*s1 > *s2)
			break;
#else
		if ((cmp = strcmp(name, np->ab_name)) == 0)
			return (np);
		if (cmp > 0)
			break;
#endif
		lp = np;
		np = np->ab_next;
	}
	if (!new)
		return (AB_NULL);

	np = _ab_newnode();
	np->ab_name = name;
	if (lp == AB_NULL) {			/* make first in chain */
		np->ab_next = ap->at_ent;
		ap->at_ent = np;
	} else {				/* in middle of chain */
		np->ab_next = lp->ab_next;
		lp->ab_next = np;
	}
	return (np);
}


LOCAL abent_t *
_ab_newnode()
{
	register abent_t	*new;

	if ((new = (abent_t *)malloc(sizeof (abent_t))) == AB_NULL)
		raisecond(sn_no_mem, (long)"_ab_newnode");
	new->ab_next = AB_NULL;
	new->ab_push = AB_NULL;
	new->ab_name = NULL;
	new->ab_value = NULL;
	new->ab_begin = FALSE;

	return (new);
}


LOCAL void
_ab_output(ap)
	register abtab_t *ap;
{
	register FILE *f;

#ifdef DEBUG
	berror("updating: %s", ap->at_fname);
#endif
	if (ap->at_fname == NULL)
		return;
	if ((f = fileopen(ap->at_fname, for_wct)) == (FILE *)0) {
		raisecond(sn_badfile, (long)ap->at_fname);
	} else {
		_ab_dump(ap->at_ent, f, FALSE, FALSE, FALSE);
		fclose(f);
	}
}

/*
 * Print a single name/value replacement entry to an open file.
 */
LOCAL void
_ab_print(tab, name, f, histflg)
	abidx_t	tab;
	char	*name;
	FILE	*f;
	BOOL	histflg;		/* should push into History */
{
	_ab_list(_ab_lookup(_ab_down(tab), name, FALSE), f, histflg);
	fflush(f);
}

LOCAL void
_ab_dump(np, f, pushed, intrflg, histflg)
	register abent_t *np;
		FILE	*f;
		BOOL	pushed;		/* should dump pushed values */
		BOOL	intrflg;	/* should be interuptable */
		BOOL	histflg;	/* should push into History */
{
	register abent_t *tp;

	if (np == AB_NULL)
		return;
	_ab_dump(np->ab_next, f, pushed, intrflg, histflg);
	tp = np;
	if (!pushed)
		while (tp->ab_push != AB_NULL)
			tp = tp->ab_push;
	if (!(ctlc && intrflg))
		_ab_list(tp, f, histflg);
}


LOCAL void
_ab_list(np, f, histflg)
	register abent_t *np;
		FILE	*f;
		BOOL	histflg;	/* should push into History */
{
	char		buf[512];
	unsigned int	len;

	if (np != AB_NULL && np->ab_value != NULL) {
		fprintf(f, "#%c %-8s %s\n", np->ab_begin?'b':'a',
					np->ab_name, np->ab_value);
#ifdef	INTERACTIVE
		if (histflg) {
			sprintf(buf, "#%c %-8s %s", np->ab_begin?'b':'a',
					np->ab_name, np->ab_value);
			len = strlen(buf);
			append_line(buf, len + 1, len);
		}
#endif
	}
}


LOCAL void
_ab_match(np, f, histflg, pattern, aux, alt, state)
	register abent_t *np;
		FILE	*f;
		BOOL	histflg;	/* should push into History */
		char	*pattern;
		int	*aux;
		int	alt;
		int	*state;
{
	register abent_t *tp;
	register char	*p;

	if (np == AB_NULL)
		return;
	_ab_match(np->ab_next, f, histflg, pattern, aux, alt, state);
	if (ctlc)
		return;
	tp = np;
	while (tp->ab_push != AB_NULL)
		tp = tp->ab_push;
	p = (char *)patmatch((unsigned char *)pattern, aux,
		(unsigned char *)tp->ab_name, 0, strlen(tp->ab_name), alt, state);
	if (p && *p == '\0')
		_ab_list(tp, f, histflg);
}

/*
 * Free all symbols associated with this table.
 */
LOCAL void
_ab_close(np, tab)
	register abent_t *np;
	abidx_t	tab;
{
	register abent_t *tp;
	register abent_t *sp;

	while (np != AB_NULL) {
		if (!ab_inblock(tab, np->ab_name))
			free(np->ab_name);
		tp = np;
		np = np->ab_next;		/* remember next ptr now */
		/*
		 * Now free all pushed values including this one.
		 */
		do {
			if (tp->ab_value != NULL && !ab_inblock(tab, tp->ab_value))
				free(tp->ab_value);
			sp = tp->ab_push;	/* Save push ptr */
			free((char *)tp);
		} while ((tp = sp) != AB_NULL);	/* Use remembered push ptr */
	}
}


/*
 * Find the next begin of word inside the allocated monolitic block.
 */
LOCAL char *
ab_beginword(p, ap)
	register char	*p;
		abtab_t	*ap;
{
	register char	*rend = ap->at_blkend;

	while (p < rend && (*p == ' ' || *p == '\t' || *p == '\n'))
		p++;
	return (p);
}


/*
 * Find the next end of word inside the allocated monolitic block.
 */
LOCAL char *
ab_endword(p, ap)
	register char	*p;
		abtab_t	*ap;
{
	register char	*rend = ap->at_blkend;

	while (p < rend && (*p != ' ' && *p != '\t' && *p != '\n'))
		p++;
	return (p);
}

/*
 * Find the next end of line inside the allocated monolitic block.
 */
LOCAL char *
ab_endline(p, ap)
	register char	*p;
		abtab_t	*ap;
{
	register char	*rend = ap->at_blkend;

	while (p < rend && *p != '\n')
		p++;
	return (p);
}

/*
 * Check whether a character pointer points into the monolitic block
 * that is allocated when the file is read by ab_read().
 */
LOCAL BOOL
ab_inblock(tab, p)
	abidx_t	tab;
	char	*p;
{
	register abtab_t *ap = _ab_down(tab);

	return ((p >= ap->at_blk) && (p <= ap->at_blkend));
}

/*
 * Read file 'fname' and build a new abbreviation/alias replacement table
 */
EXPORT void
ab_read(tab, fname)
	abidx_t	tab;
	char	*fname;
{
	register char	*line;
	register char	*name;
	register char	*val;
		FILE	*f;
	register abtab_t *ap = _ab_down(tab);
		off_t	fsize;
	register BOOL	beg;

	/*
	 * Make sure that ap->at_fname is NULL to avoid writing back to the
	 * file when calling ab_insert().
	 */
	ab_close(tab);

	if ((f = fileopen(fname, for_ru)) == (FILE *)NULL) {
		ap->at_blk = NULL;
		return;
	}
	fsize = filesize(f);
	if ((ap->at_blk = malloc((size_t) fsize)) == NULL) {
		raisecond(sn_no_mem, (long)"ab_read");
		fclose(f);
		return;
	}
	ap->at_blkend = ap->at_blk + fsize - 1;
	if (fileread(f, ap->at_blk, (int)fsize) != fsize) {
		free(ap->at_blk);
		ap->at_blk = NULL;
		fclose(f);
		return;
	}
	fclose(f);

	line = ap->at_blk;
	while (line < ap->at_blkend) {
		line = ab_beginword(line, ap);
		if (*line++ != '#') {
			line = ab_endline(line, ap);
			continue;
		}
		if (*line == 'a') {
			beg = FALSE;
		} else if (*line == 'b') {
			beg = TRUE;
		} else {
			line = ab_endline(line, ap);
			continue;
		}
		name = line = ab_beginword(ab_endword(line, ap), ap);
		line = ab_endword(line, ap);
		*line++ = '\0';
		val = line = ab_beginword(line, ap);
		line = ab_endline(line, ap);
		*line++ = '\0';
		ab_insert(tab, name, val, beg);
	}
}


/*
 * Set filename to use for _ab_output()
 */
EXPORT void
ab_sname(tab, fname)
	abidx_t	tab;
	char	*fname;
{
	abtab_t *ap = _ab_down(tab);

	ap->at_fname = fname;
}

/*
 * Use new abbrev file
 */
EXPORT void
ab_use(tab, fname)
	abidx_t	tab;
	char	*fname;
{
	ab_read(tab, fname);
	ab_sname(tab, fname);
}

/*
 * Shut down a table, remove all name/value translations from this tab before.
 */
EXPORT void
ab_close(tab)
	abidx_t	tab;
{
	register abtab_t *ap = _ab_down(tab);

	_ab_close(ap->at_ent, tab);
	ap->at_fname = NULL;
	ap->at_ent = AB_NULL;
	if (ap->at_blk != NULL) {
		free(ap->at_blk);
		ap->at_blk = NULL;
	}
}


/*
 * Insert a new name/value pair into an abbreviation/alias table.
 */
EXPORT void
ab_insert(tab, name, val, beg)
	abidx_t	tab;
	char	*name;
	char	*val;
	BOOL	beg;
{
		abtab_t	*ap = _ab_down(tab);
	register abent_t *np;

	np = _ab_lookup(ap, name, TRUE);
	if (np->ab_value != NULL && !ab_inblock(tab, np->ab_value))
		free(np->ab_value);
	np->ab_value = val;
	np->ab_begin = beg;
	/*
	 * If this entry has not been pushed on top of old replacements,
	 * update the underlying file storage.
	 */
	if (np->ab_push == AB_NULL)
		_ab_output(ap);
}


/*
 * Push a new name/value pair on top of an abbreviation/alias table entry.
 */
EXPORT void
ab_push(tab, name, val, beg)
	abidx_t	tab;
	char	*name;
	char	*val;
	BOOL	beg;
{
		abtab_t		*ap = _ab_down(tab);
	register abent_t	*np;
	register abent_t	*new;

	np = _ab_lookup(ap, name, TRUE);
	new = _ab_newnode();		/* Get space for node to push	*/
	new->ab_value = np->ab_value;	/* First save old node data	*/
	new->ab_begin = np->ab_begin;
	new->ab_push = np->ab_push;
	np->ab_push = new;		/* Then make pushed node active	*/
	np->ab_value = val;
	np->ab_begin = beg;
}


/*
 * Pop a new name/value pair from the top of an abbreviation/alias table entry.
 * If there is no pushed entry left over, then the whole entry is deleted.
 * Deletion is done by setting ab_value to NULL.
 */
EXPORT void
ab_delete(tab, name)
	abidx_t	tab;
	char	*name;
{
		abtab_t		*ap = _ab_down(tab);
	register abent_t	*np;
	register abent_t	*op;

	np = _ab_lookup(ap, name, FALSE);
	if (np != AB_NULL && np->ab_value != NULL) {
		if (!ab_inblock(tab, np->ab_value))
			free(np->ab_value);
		np->ab_value = NULL;
		if (np->ab_push != AB_NULL) {	    /* If saved old value */
			op = np->ab_push;
			np->ab_value = op->ab_value; /* Pop top entry */
			np->ab_begin = op->ab_begin;
			np->ab_push = op->ab_push;
			free((char *)op);
		} else {
			_ab_output(ap);
		}
	}
}


/*
 * Perform a name/value translation for a named abbreviation/alias table.
 */
EXPORT char *
ab_value(tab, name, beg)
	abidx_t	tab;
	char	*name;
	BOOL	beg;			/* lookup begin abbreviations also */
{
	register abent_t	*np;

	np = _ab_lookup(_ab_down(tab), name, FALSE);
	if (np != AB_NULL && (np->ab_begin == FALSE || beg == TRUE))
		return (np->ab_value);
	else
		return (NULL);
}


/*
 * Print all name/value replacement entries to an open file.
 */
EXPORT void
ab_dump(tab, f, histflg)
	abidx_t	tab;
	FILE	*f;
	BOOL	histflg;		/* should push into History */
{
	_ab_dump(_ab_down(tab)->at_ent, f, TRUE, TRUE, histflg);
	fflush(f);
}


/*
 * Print all name/value replacements with matched entries to an open file.
 */
EXPORT void
ab_list(tab, pattern, f, histflg)
		abidx_t	tab;
	register char	*pattern;
		FILE	*f;
		BOOL	histflg;	/* should push into History */
{
	register int	*aux;		/* auxiliary array */
	register int	*state;		/* state array */
	register int	patlen;		/* pattern lenght */
	register int	alt;		/* outermost alternate */

	if (!any_match(pattern)) {
		_ab_print(tab, pattern, f, histflg);
	} else {
		patlen = strlen(pattern);
		aux = (int *)malloc((size_t)patlen * sizeof (int));
		state = (int *)malloc((size_t)(patlen+1) * sizeof (int));
		alt = patcompile((unsigned char *)pattern, patlen, aux);
		if (alt) {
			_ab_match(_ab_down(tab)->at_ent,
					f, histflg, pattern, aux, alt, state);
			fflush(f);
		} else {
			berror(ebadpattern);
		}
		free((char *)aux);
		free((char *)state);
	}
}
