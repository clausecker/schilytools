/* @(#)find_tok.h	1.8 18/08/20 Copyright 2004-2018 J. Schilling */
/*
 *	Copyright (c) 2004-2018 J. Schilling
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

#ifndef	_FIND_TOK_H
#define	_FIND_TOK_H

#define	OPEN	0	/* (				*/
#define	CLOSE	1	/* )				*/
#define	LNOT	2	/* !				*/
#define	AND	3	/* a				*/
#define	LOR	4	/* o				*/
#define	ATIME	5	/* -atime			*/
#define	CTIME	6	/* -ctime			*/
#define	DEPTH	7	/* -depth			*/
#define	EXEC	8	/* -exec			*/
#define	FOLLOW	9	/* -follow	POSIX Extension	*/
#define	FSTYPE	10	/* -fstype	POSIX Extension	*/
#define	GROUP	11	/* -group			*/
#define	INUM	12	/* -inum	POSIX Extension	*/
#define	LINKS	13	/* -links			*/
#define	LOCL	14	/* -local	POSIX Extension	*/
#define	LS	15	/* -ls		POSIX Extension	*/
#define	MODE	16	/* -mode	POSIX Extension	*/
#define	MOUNT	17	/* -mount	POSIX Extension	*/
#define	MTIME	18	/* -mtime			*/
#define	NAME	19	/* -name			*/
#define	NEWER	20	/* -newer			*/
#define	NOGRP	21	/* -nogroup			*/
#define	NOUSER	22	/* -nouser			*/
#define	OK_EXEC	23	/* -ok				*/
#define	PERM	24	/* -perm			*/
#define	PRINT	25	/* -print			*/
#define	PRINTNNL 26	/* -printnnl	POSIX Extension	*/
#define	PRUNE	27	/* -prune			*/
#define	SIZE	28	/* -size			*/
#define	TIME	29	/* -time	POSIX Extension	*/
#define	TYPE	30	/* -type			*/
#define	USER	31	/* -user 			*/
#define	XDEV	32	/* -xdev			*/
#define	PATH	33	/* -path	POSIX Extension	*/
#define	LNAME	34	/* -lname	POSIX Extension	*/
#define	PAT	35	/* -pat		POSIX Extension	*/
#define	PPAT	36	/* -ppat	POSIX Extension	*/
#define	LPAT	37	/* -lpat	POSIX Extension	*/
#define	PACL	38	/* -ack		POSIX Extension	*/
#define	XATTR	39	/* -xattr	POSIX Extension	*/
#define	LINKEDTO 40	/* -linkedto	POSIX Extension	*/
#define	NEWERAA	41	/* -neweraa	POSIX Extension	*/
#define	NEWERAC	42	/* -newerac	POSIX Extension	*/
#define	NEWERAM	43	/* -neweram	POSIX Extension	*/
#define	NEWERAT	44	/* -newerat	POSIX Extension	*/
#define	NEWERCA	45	/* -newerca	POSIX Extension	*/
#define	NEWERCC	46	/* -newercc	POSIX Extension	*/
#define	NEWERCM	47	/* -newercm	POSIX Extension	*/
#define	NEWERCT	48	/* -newerct	POSIX Extension	*/
#define	NEWERMA	49	/* -newerma	POSIX Extension	*/
#define	NEWERMC	50	/* -newermc	POSIX Extension	*/
#define	NEWERMM	51	/* -newermm	POSIX Extension	*/
#define	NEWERMT	52	/* -newermt	POSIX Extension	*/
#define	SPARSE	53	/* -sparse	POSIX Extension	*/
#define	LTRUE	54	/* -true	POSIX Extension	*/
#define	LFALSE	55	/* -false	POSIX Extension	*/
#define	MAXDEPTH 56	/* -maxdepth	POSIX Extension	*/
#define	MINDEPTH 57	/* -mindepth	POSIX Extension	*/
#define	HELP	58	/* -help	POSIX Extension	*/
#define	CHOWN	59	/* -chown	POSIX Extension	*/
#define	CHGRP	60	/* -chgrp	POSIX Extension	*/
#define	CHMOD	61	/* -chmod	POSIX Extension	*/
#define	DOSTAT	62	/* -dostat	POSIX Extension	*/
#define	INAME	63	/* -iname	POSIX Extension	*/
#define	ILNAME	64	/* -ilname	POSIX Extension	*/
#define	IPATH	65	/* -ipath	POSIX Extension	*/
#define	IPAT	66	/* -ipat	POSIX Extension	*/
#define	IPPAT	67	/* -ippat	POSIX Extension	*/
#define	ILPAT	68	/* -ilpat	POSIX Extension	*/
#define	AMIN	69	/* -amin	POSIX Extension	*/
#define	CMIN	70	/* -cmin	POSIX Extension	*/
#define	MMIN	71	/* -mmin	POSIX Extension	*/
#define	PRINT0	72	/* -print0	POSIX Extension	*/
#define	FPRINT	73	/* -fprint	POSIX Extension	*/
#define	FPRINTNNL 74	/* -fprintnnl	POSIX Extension	*/
#define	FPRINT0	75	/* -fprint0	POSIX Extension	*/
#define	FLS	76	/* -fls		POSIX Extension	*/
#define	EMPTY	77	/* -empty	POSIX Extension	*/
#define	READABLE 78	/* -readable	POSIX Extension	*/
#define	WRITABLE 79	/* -writable	POSIX Extension	*/
#define	EXECUTABLE 80	/* -executable	POSIX Extension	*/
#define	EXECDIR	81	/* -execdir	POSIX Extension	*/
#define	OK_EXECDIR 82	/* -okdir	POSIX Extension	*/
#define	CALL	83	/* -call	POSIX Extension	*/
#define	CALLDIR	84	/* -calldir	POSIX Extension	*/
#define	ENDPRIM	85	/* End of primary list		*/
#define	EXECPLUS 86	/* -exec			*/
#define	EXECDIRPLUS 87	/* -execdir			*/
#define	ENDTLIST 88	/* End of token list		*/

#define	tokennames	_find_tokennames

#ifdef	TOKEN_NAMES
LOCAL	char	*tokennames[] = {
	"(",		/* 0 OPEN			*/
	")",		/* 1 CLOSE			*/
	"!",		/* 2 LNOT			*/
	"a",		/* 3 AND			*/
	"o",		/* 4 LOR			*/
	"atime",	/* 5 ATIME			*/
	"ctime",	/* 6 CTIME			*/
	"depth",	/* 7 DEPTH			*/
	"exec",		/* 8 EXEC			*/
	"follow",	/* 9 FOLLOW	POSIX Extension	*/
	"fstype",	/* 10 FSTYPE	POSIX Extension	*/
	"group",	/* 11 GROUP			*/
	"inum",		/* 12 INUM	POSIX Extension	*/
	"links",	/* 13 LINKS			*/
	"local",	/* 14 LOCL	POSIX Extension	*/
	"ls",		/* 15 LS	POSIX Extension	*/
	"mode",		/* 16 MODE	POSIX Extension	*/
	"mount",	/* 17 MOUNT	POSIX Extension	*/
	"mtime",	/* 18 MTIME			*/
	"name",		/* 19 NAME			*/
	"newer",	/* 20 NEWER			*/
	"nogroup",	/* 21 NOGRP			*/
	"nouser",	/* 22 NOUSER			*/
	"ok",		/* 23 OK_EXEC			*/
	"perm",		/* 24 PERM			*/
	"print",	/* 25 PRINT			*/
	"printnnl",	/* 26 PRINTNNL	POSIX Extension	*/
	"prune",	/* 27 PRUNE			*/
	"size",		/* 28 SIZE			*/
	"time",		/* 29 TIME	POSIX Extension	*/
	"type",		/* 30 TYPE			*/
	"user",		/* 31 USER			*/
	"xdev",		/* 32 XDEV			*/
	"path",		/* 33 PATH	POSIX Extension	*/
	"lname",	/* 34 LNAME	POSIX Extension	*/
	"pat",		/* 35 PAT	POSIX Extension	*/
	"ppat",		/* 36 PPAT	POSIX Extension	*/
	"lpat",		/* 37 LPAT	POSIX Extension	*/
	"acl",		/* 38 PACL	POSIX Extension	*/
	"xattr",	/* 39 XATTR	POSIX Extension	*/
	"linkedto",	/* 40 LINKEDTO	POSIX Extension	*/
	"neweraa",	/* 41 NEWERAA	POSIX Extension	*/
	"newerac",	/* 42 NEWERAC	POSIX Extension	*/
	"neweram",	/* 43 NEWERAM	POSIX Extension	*/
	"newerat",	/* 44 NEWERAT	POSIX Extension	*/
	"newerca",	/* 45 NEWERCA	POSIX Extension	*/
	"newercc",	/* 46 NEWERCC	POSIX Extension	*/
	"newercm",	/* 47 NEWERCM	POSIX Extension	*/
	"newerct",	/* 48 NEWERCT	POSIX Extension	*/
	"newerma",	/* 49 NEWERMA	POSIX Extension	*/
	"newermc",	/* 50 NEWERMC	POSIX Extension	*/
	"newermm",	/* 51 NEWERMM	POSIX Extension	*/
	"newermt",	/* 52 NEWERMT	POSIX Extension	*/
	"sparse",	/* 53 SPARSE	POSIX Extension	*/
	"true",		/* 54 LTRUE	POSIX Extension	*/
	"false",	/* 55 LFALSE	POSIX Extension	*/
	"maxdepth",	/* 56 MAXDEPTH	POSIX Extension	*/
	"mindepth",	/* 57 MINDEPTH	POSIX Extension	*/
	"help",		/* 58 HELP	POSIX Extension	*/
	"chown",	/* 59 CHOWN	POSIX Extension	*/
	"chgrp",	/* 60 CHGRP	POSIX Extension	*/
	"chmod",	/* 61 CHMOD	POSIX Extension	*/
	"dostat",	/* 62 DOSTAT	POSIX Extension	*/
	"iname",	/* 63 INAME	POSIX Extension	*/
	"ilname",	/* 64 ILNAME	POSIX Extension	*/
	"ipath",	/* 65 IPATH	POSIX Extension	*/
	"ipat",		/* 66 IPAT	POSIX Extension	*/
	"ippat",	/* 67 IPPAT	POSIX Extension	*/
	"ilpat",	/* 68 ILPAT	POSIX Extension	*/
	"amin",		/* 69 AMIN	POSIX Extension	*/
	"cmin",		/* 70 CMIN	POSIX Extension	*/
	"mmin",		/* 71 MMIN	POSIX Extension	*/
	"print0",	/* 72 PRINT0	POSIX Extension	*/
	"fprint",	/* 73 FPRINT	POSIX Extension	*/
	"fprintnnl",	/* 74 FPRINTNNL	POSIX Extension	*/
	"fprint0",	/* 75 FPRINT0	POSIX Extension	*/
	"fls",		/* 76 FLS	POSIX Extension	*/
	"empty",	/* 77 EMPTY	POSIX Extension	*/
	"readable",	/* 78 READABLE	POSIX Extension	*/
	"writable",	/* 79 WRITABLE	POSIX Extension	*/
	"executable",	/* 80 EXECUTABLE POSIX Extension */
	"execdir",	/* 81 EXECDIR	POSIX Extension */
	"okdir",	/* 82 OK_EXECDIR POSIX Extension */
	"call",		/* 83 CALL	POSIX Extension */
	"calldir",	/* 84 CALLDIR	POSIX Extension */
	0,		/* 85 End of primary list	*/
	"exec",		/* 86 Map EXECPLUS -> "exec"	*/
	"execdir",	/* 87 Map EXECDIRPLUS -> "execdir" */
	0		/* 88 End of list		*/
};
#define	NTOK	((sizeof (tokennames) / sizeof (tokennames[0])) - 1)

#else	/* TOKEN_NAMES */

#define	NTOK	ENDTLIST

#endif	/* TOKEN_NAMES */

#endif	/* _FIND_TOK_H */
