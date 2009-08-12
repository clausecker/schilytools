/* @(#)inet.h	1.2 09/08/04 Copyright 2009 J. Schilling */
/*
 *	Inet abstraction
 *
 *	Copyright (c) 2009 J. Schilling
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

#ifndef	_SCHILY_INET_H
#define	_SCHILY_INET_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_ARPA_INET_H
#ifndef	_INCL_ARPA_INET_H
#include <arpa/inet.h>
#define	_INCL_ARPA_INET_H
#endif
#else	/* !HAVE_ARPA_INET_H */

/*
 * BeOS does not have <arpa/inet.h>
 * but inet_ntaoa() is in <netdb.h>
 */
#ifdef	HAVE_NETDB_H
#ifndef	_INCL_NETDB_H
#include <netdb.h>
#define	_INCL_NETDB_H
#endif
#endif	/* HAVE_NETDB_H */

#endif	/* !HAVE_ARPA_INET_H */

#endif	/* _SCHILY_INET_H */
