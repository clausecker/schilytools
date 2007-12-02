/* @(#)getargs.h	1.18 07/07/02 Copyright 1985-2007 J. Schilling */
/*
 *	Definitions for getargs()/getallargs()/getfiles()
 *
 *	Copyright (c) 1985-2007 J. Schilling
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

#ifndef	_SCHILY_GETARGS_H
#define	_SCHILY_GETARGS_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Return values for get*args()/get*files()
 *
 * This package calls options "flags", they are returned from get*args().
 *
 * Note that NOTAFILE is not returned by the interface functions.
 * NOTAFILE is however used as return code from the user's callback functions
 * to signal that the current arg may be an option the callback function does
 * not know and definitely is no file type argument.
 *
 * General rules for the return code of the interface functions
 * get*args()/get*files():
 *
 *	> 0		A file type argument was found
 *	  0		All arguments have been parsed
 *	< 0		An error occured
 *
 * Flag and file arg processing should be terminated after getting a return
 * code <= 0.
 */
#define	FLAGDELIM	  2	/* "--" stopped flag processing	*/
#define	NOTAFLAG	  1	/* Not a flag type argument	*/
#define	NOARGS		  0	/* No more args			*/
#define	BADFLAG		(-1)	/* Not a valid flag argument	*/
#define	BADFMT		(-2)	/* Error in format string	*/
#define	NOTAFILE	(-3)	/* Seems to be a flag type	*/

typedef	int	(*getargfun)	__PR((const char *__arg, void *__valp));
typedef	int	(*getpargfun)	__PR((const char *__arg, void *__valp, int *__pac, char *const **__pav, const char *__opt));

#define	NO_ARGFUN	(getargpfun)0

struct ga_flags {
	const char	*ga_format;
	void		*ga_arg;
	getpargfun	ga_funcp;
};

struct ga_props {
	UInt32_t	ga_flags;
	UInt32_t	ga_oflags;
	size_t		ga_size;
};

#define	GA_NO_PROPS	(struct ga_props *)0

/*
 * Definitions for ga_flags
 */
#define	GAF_DEFAULT		0x00	/* The default behavior		    */
#define	GAF_NO_PLUS		0x01	/* Options may not start with '+'   */
#define	GAF_NO_EQUAL		0x02	/* Disallow '=' between opt and val */
#define	GAF_NEED_DASH		0x04	/* Need dash before (-name=val),    */
					/*  name=val is not allowed	    */
#define	GAF_DELIM_DASHDASH	0x08	/* "--" stops even get?allargs()    */

/*
 * POSIX does not allow options in the form "+option", "-option=value" or
 * "option=value".
 *
 * GAF_NO_PLUS		do not allow options to start with a '+'
 * GAF_NO_EQUAL		do not allow options to contain '=' between name & val
 */
#define	GAF_POSIX_DEFAULT	(GAF_NO_PLUS | GAF_NO_EQUAL)

/*
 * Keep in sync with schily.h
 */
extern	int	getallargs __PR((int *, char * const**, const char *, ...));
extern	int	getargs __PR((int *, char * const**, const char *, ...));
extern	int	getfiles __PR((int *, char * const**, const char *));
extern	char	*getargerror __PR((int));

/*
 * The new list versions of the functions need struct ga_props and thus need
 * getargs.h
 */
extern	int	getlallargs __PR((int *, char * const**, struct ga_props *, const char *, ...));
extern	int	getlargs __PR((int *, char * const**, struct ga_props *, const char *, ...));
extern	int	getlfiles __PR((int *, char * const**, struct ga_props *, const char *));
extern	int	_getarginit __PR((struct ga_props *, size_t, UInt32_t));

#define	getarginit(p, f)	_getarginit(p, sizeof (struct ga_props), f)

/*
 * The vector versions of the functions need struct ga_flags and thus need
 * getargs.h
 */
extern	int	getvallargs __PR((int *, char * const**, struct ga_flags *, struct ga_props *));
extern	int	getvargs __PR((int *, char * const**, struct ga_flags *, struct ga_props *));
extern	int	getvfiles __PR((int *, char * const**, struct ga_flags *, struct ga_props *));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_GETARGS_H */
