/* @(#)strsubs.h	1.8 04/06/18 Copyright 1986-2004 J. Schilling */
/*
 *	header for string support routines
 *
 *	Copyright (c) 1986-2004 J. Schilling
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

extern	char	*makestr	__PR((char *s));
extern	char	*concat		__PR((char *, ...));
extern	char	*concatv	__PR((char **s));
extern	int	streql		__PR((const char *s1, const char *s2));
extern	int	streqln		__PR((char *s1, char *s2, int n));
extern	char	*strindex	__PR((char *x, char *y));
extern	int	strbeg		__PR((char *x, char *y));
extern	int	wordeql		__PR((char *s1, char *s2));
extern	char	*quote_string	__PR((char *s, char *spec));
extern	char	*pretty_string	__PR((unsigned char *s));
extern	char	*fbasename	__PR((char *n));
