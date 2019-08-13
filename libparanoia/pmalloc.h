/* @(#)pmalloc.h	1.1 04/02/20 Copyright 2004 J. Schilling */
/*
 *	Paranoia malloc() functions
 *
 *	Copyright (c) 2004 J. Schilling
 */
/*@@C@@*/

#ifndef	_PMALLOC_H
#define	_PMALLOC_H

extern	void	_pfree		__PR((void *ptr));
extern	void	*_pmalloc	__PR((size_t size));
extern	void	*_pcalloc	__PR((size_t nelem, size_t elsize));
extern	void	*_prealloc	__PR((void *ptr, size_t size));

#endif	/* _PMALLOC_H */
