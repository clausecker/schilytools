/* @(#)abbrev.h	1.17 12/06/10 Copyright 1985-2012 J. Schilling */
/*
 *	Copyright (c) 1985-2012 J. Schilling
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

typedef int abidx_t;

#ifdef	BOURNE_SHELL
#define	FILE_p		int
#else
#define	FILE_p		FILE *
#endif


#define	ABTABS		2	/* .globals & .locals			   */

#define	GLOBAL_AB	0	/* The idx for the '.globals' abbrev./aliases */
#define	LOCAL_AB	1	/* The idx for the '.locals' abbrev./aliases */

/*
 * Function flags:
 */
#define	AB_NOFLAG	0	/* No flags				*/
#define	AB_BEGIN	1	/* A begin only abbreviation		*/
#define	AB_POP		2	/* POP rather than permanently delete	*/
#define	AB_POPALL	4	/* POP all definitions for an alias	*/
#define	AB_PERSIST	8	/* Dump persistent, not pushed values	*/
#define	AB_ALL		16	/* Dump all values			*/
#define	AB_HISTORY	32	/* Enter entry in command line history	*/
#define	AB_INTR		64	/* Loops are interruptable via ^C	*/
#define	AB_POSIX	128	/* Output in POSIX "name=value" format	*/
#define	AB_PARSE	256	/* Output "alias name=value"		*/
#define	AB_PGLOBAL	512	/* Add "-g" with output from AB_PARSE	*/
#define	AB_PLOCAL	1024	/* Add "-l" with output from AB_PARSE	*/

extern	void	ab_read		__PR((abidx_t tab, char *fname));
extern	void	ab_sname	__PR((abidx_t tab, char *fname));
extern	char	*ab_gname	__PR((abidx_t tab));
extern	void	ab_use		__PR((abidx_t tab, char *fname));
extern	void	ab_close	__PR((abidx_t tab));
extern	void	ab_insert	__PR((abidx_t tab, char *name, char *val,
								int aflags));
extern	void	ab_push		__PR((abidx_t tab, char *name, char *val,
								int aflags));
extern	void	ab_delete	__PR((abidx_t tab, char *name, int aflags));
extern	void	ab_deleteall	__PR((abidx_t tab, int aflags));
extern	char	*ab_value	__PR((abidx_t tab, char *name, void **seen,
								int aflags));
extern	void	ab_dump		__PR((abidx_t tab, FILE_p file, int aflags));
extern	void	ab_list		__PR((abidx_t tab, char *pattern, FILE_p file,
								int aflags));
