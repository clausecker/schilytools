/* @(#)shcall.h	1.4 18/08/01 Copyright 2009-2018 J. Schilling */
/*
 *	Abstraction from shcall.h
 *
 *	Copyright (c) 2009-2018 J. Schilling
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

#ifndef _SCHILY_SHCALL_H
#define	_SCHILY_SHCALL_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef	_SCHILY_INTTYPES_H
#include <schily/inttypes.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	__sqfun_t_defined
typedef	int	(*sqfun_t)	__PR((void *arg));
#define	__sqfun_t_defined
#endif

#ifndef	__cbfun_t_defined
typedef	int	(*cbfun_t)	__PR((int ac, char  **argv));
#define	__cbfun_t_defined
#endif

#ifndef	__squit_t_defined

typedef struct {
	sqfun_t	quitfun;	/* Function to query for shell signal quit   */
	void	*qfarg;		/* Generic arg for shell builtin quit fun    */
	Int32_t	flags;		/* Flags to identify data beyond qfarg	    */
	cbfun_t	callfun;	/* Callback function for -call		    */
	void	*__reserved[16]; /* For future extensions		    */
} squit_t;

#define	SQ_CALL	0x01		/* Use call feature */	

#define	__squit_t_defined
#endif

typedef	int	(*shcall)	__PR((int ac, char **av, char **ev,
					FILE *std[3], squit_t *__quit));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_SHCALL_H */
