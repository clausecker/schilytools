/* @(#)make.h	1.92 10/10/06 Copyright 1985, 87, 91, 1995-2010 J. Schilling */
/*
 *	Definitions for make.
 *	Copyright (c) 1985, 87, 91, 1995-2010 by J. Schilling
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

#ifndef	_SCHILY_UTYPES_H
#include <schily/utypes.h>	/* Includes sys/types.h	*/
#endif
#ifndef	_SCHILY_STANDARD_H
#include <schily/standard.h>	/* #defines BOOL	*/
#endif

/*
 * XXX if sizeof(date_t) is < sizeof(time_t) there may be problems
 */
typedef unsigned long	date_t;

/*
 * XXX It may be a good idea to make "newtime" > RECURSETIME as
 * XXX the date of unknown targets is set to RECURSETIME before
 * XXX it is made.
 *
 * If we add new entries here, we need to also add a related entry
 * to the function prtime().
 */
#define	NOTIME		((date_t)0)	/* Obj not (yet) found   */
#define	BADTIME		((date_t)-1)	/* Obj could not be made */
#define	RECURSETIME	((date_t)-2)	/* Obj depends on itself */
#define	MAKETIME	((date_t)-3)	/* Obj is currently made */
#define	PHONYTIME	((date_t)-4)	/* Obj is Phony		 */

/*
 * We may need to check whether PHONYTIME is a "valid" time...
 */
#define	VALIDTIME(t)	((t) > NOTIME && \
			(t) < PHONYTIME)

					/* NOTE: MAXLEVEL must be odd */
#define	MAXLEVEL	255		/* Obj is not yet searched for */
#define	WDLEVEL		0		/* Obj is in working dir (".") */
#define	OBJLEVEL	1		/* Obj is in .OBJDIR */

#define	SSRC		1		/* Search in source directories	*/
#define	SOBJ		2		/* Search in obj directories */
#define	SALL		(SSRC | SOBJ)	/* Search in source/obj directories */

#ifdef	pdp11
#define	NAMEMAX		512		/* Max size of a name	*/
#else
#define	NAMEMAX		4096		/* Max size of a name POSIX linelen */
#endif
#define	TYPICAL_NAMEMAX	128		/* Namelen that avoids malloc() */

/*
 * one unique element is used for each target or member in a dependency list
 * also used for macros and macro definition lists
 */
typedef struct obj {
	struct	obj	*o_left;	/* Left next node in binary tree    */
	struct	obj	*o_right;	/* Right next node in binary tree   */
	struct	list	*o_list;	/* List of dependencies for target  */
	struct	cmd	*o_cmd;		/* List of commands for this target */
		char	*o_name;	/* Name of this target		    */
	struct	obj	*o_node;	/* Auxilliary node pointer	    */
		date_t	o_date;		/* Current date for this target	    */
		size_t	o_namelen;	/* strlen(o_name)		    */
		short	o_type;		/* Type of node			    */
		short	o_flags;	/* Flags for this node		    */
		short	o_level;	/* Obj level this target was found  */
		short	o_fileindex;	/* Makefile idx for this definition */
} obj_t;

#define	F_READONLY	1		/* Prevents overwriting the value   */
#define	F_CMDLINE	2		/* From commandline		    */
#define	F_EXPORT	4		/* Export to environment	    */
#define	F_MULTITARGET	8		/* Multiple targets for one rule    */
#define	F_DCOLON	16		/* Intermediate :: object	    */
#define	F_TERM		32		/* Object is source of a TERM rule  */
#define	F_PERCENT	64		/* o_name has % in pattern dep list */
#define	F_PATRULE	128		/* Pattern rule pointer in o_node   */
#define	F_NEWNODE	256		/* This node was just created	    */

/*
 * list element, used to build dependency lists from unique obj elements
 */
typedef struct list {
	struct	list	*l_next;	/* Next entry in dependency list    */
	struct	obj	*l_obj;		/* Obj structure for this entry	    */
} list_t;

/*
 * element for commands that are used to update a target
 * one allocated for each command line
 */
typedef struct cmd {
	struct	cmd	*c_next;	/* Next command for this target	    */
		char	*c_line;	/* Command line for this element    */
} cmd_t;

/*
 * Element used to describe pattern rules (rules that contain a '%' sign)
 * format is:
 *	target: source	(a%b: [ c%d ...])
 *		cmdlist
 */
typedef struct patrule {
	struct	patrule	*p_next;	/* Next pattern rule		    */
	struct	cmd	*p_cmd;		/* List of commands for this rule   */
	struct	obj	*p_name;	/* Node for complete target name    */
	struct	list	*p_list;	/* List of dependencies for rule    */
	int		p_flags;	/* Flags related to this rule	    */
#ifdef	xxx
	struct	obj	*p_tgt_prefix;	/*				    */
	struct	obj	*p_tgt_suffix;	/*				    */
	struct	obj	*p_src_prefix;	/*				    */
	struct	obj	*p_src_suffix;	/*				    */
#else
	char		*p_tgt_prefix;	/* "a" (the string before the '%')  */
	char		*p_tgt_suffix;	/* "b" (the string after the '%')   */
	char		*p_src_prefix;	/* "c" (the string before the '%')  */
	char		*p_src_suffix;	/* "d" (the string after the '%')   */
	size_t		p_tgt_pfxlen;
	size_t		p_tgt_suflen;
	size_t		p_src_pfxlen;
	size_t		p_src_suflen;
#endif
} patr_t;

/*
 * Definitions for Patrule Flags
 */
#define	PF_TERM	0x01			/* This is a terminator rule	    */

/*
 * node types
 *
 * Note that NWARN cannot be in o_flags as there may be no associated node.
 */
#define	EQUAL	'='
#define	COLON	':'
#define	SEMI	';'
#define	ADDMAC	('=' | ('+'<<8)) /* +=	 */
#define	ASSIGN	('=' | (':'<<8)) /* :=	 */
#define	DCOLON	(':' | (':'<<8)) /* ::	 */
#define	SHVAR	0x1001		 /* :sh= */

#define	NWARN	0x4000		/* We did warn already on this node */

#define	basetype(x)	((x) & 0xFF)
#define	ntype(x)	((x) & 0x3FFF)

/*
 * make.c
 */
extern	void	usage		__PR((int exitcode));
extern	void	setup_dotvars	__PR((void));
extern	char	*searchtype	__PR((int mode));
extern	void	doexport	__PR((char *));
extern	void	dounexport	__PR((char *));
extern	int	docmd		__PR((char * cmd, obj_t * obj));
extern	BOOL	move_tgt	__PR((obj_t * from));
extern	BOOL	touch_file	__PR((char * name));
extern	date_t	gftime		__PR((char * file));
extern	Llong	gfileid		__PR((char * file));
extern	char	*prtime		__PR((date_t  date));
extern	char	*curwdir	__PR((void));

/*
 * archconf.c
 */
extern	void	setup_arch	__PR((void));

/*
 * readfile.c
 */
extern	char	*peekrdbuf	__PR((void));
extern	char	*getrdbuf	__PR((void));
extern	int	getrdbufsize	__PR((void));
extern	void	setincmd	__PR((BOOL isincmd));
extern	void	getch		__PR((void));
extern	int	peekch		__PR((void));
extern	void	skipline	__PR((void));
extern	void	readstring	__PR((char * str, char * strname));
extern	void	readfile	__PR((char * name, BOOL  must_exist));
extern	void	doinclude	__PR((char * name, BOOL  must_exist));
extern	void	makeincs	__PR((void));

/*
 * parse.c
 */
extern	void	parsefile	__PR((void));
extern	char	*get_var	__PR((char *name));
extern	void	define_var	__PR((char *name, char *val));
extern	list_t	*cvtvpath	__PR((list_t *l));
extern	BOOL	is_inlist	__PR((char *objname, char *name));
extern	BOOL	nowarn		__PR((char *name));
extern	obj_t	*objlook	__PR((char * name, BOOL  create));
extern	list_t	*objlist	__PR((char * name));
extern	obj_t	*ssufflook	__PR((char * name, BOOL  create));
extern	BOOL	check_ssufftab	__PR((void));
extern	void	printtree	__PR((void));
#ifdef	EOF
extern	void	probj		__PR((FILE *f, obj_t * o, int type, int dosuff));
#endif
extern	void	prtree		__PR((void));

/*
 * update.c
 */
extern	void	initchars	__PR((void));
extern	char	*filename	__PR((char * name));
extern	BOOL	isprecious	__PR((obj_t * obj));
extern	BOOL	isphony		__PR((obj_t * obj));
extern	list_t	*list_nth	__PR((list_t * list, int n));
extern	char	*build_path	__PR((int level, char * ename, size_t namelen,
						char * path, size_t psize));
extern	char	*substitute	__PR((char * cmd, obj_t * obj, obj_t * source,
							char *suffix));
extern	char	*shout		__PR((char * cmd));
extern	BOOL	domake		__PR((char * name));
extern	BOOL	omake		__PR((obj_t * obj, BOOL  must_exist));
extern	BOOL	xmake		__PR((char * name, BOOL  must_exist));

/*
 * memory.c
 */
#undef	___realloc
extern	void	*___realloc	__PR((void *ptr, size_t size, char *msg));
#ifdef	DEBUG
extern	void	prmem		__PR((void));
#endif
extern	char	*fastalloc	__PR((unsigned int size));
extern	void	fastfree	__PR((char *p, unsigned int size));
extern	void	freelist	__PR((list_t *l));
extern	char	*strsave		__PR((char *p));
extern	char	*initgbuf	__PR((int size));
extern	char	*growgbuf	__PR((char *p));


/*
 * Command line options.
 */
extern	BOOL	Kflag;		/* Continue on unrelated targets	*/
extern	BOOL	NSflag;		/* Ignore no Source on dependency	*/
extern	BOOL	Sflag;		/* Do not print command lines on exec.	*/
extern	BOOL	Tflag;		/* Touch files instead of make.		*/
extern	BOOL	Qflag;		/* If up to date exit (0)		*/
extern	int	Debug;		/* Print reson for rebuild		*/
extern	int	XDebug;		/* Print extended debug info		*/
extern	int	Dmake;		/* Display makefile			*/
extern	BOOL	Prdep;		/* Print include dependendy		*/
extern	BOOL	No_Warn;	/* Don't print warning Messages.	*/
extern	BOOL	Do_Warn;	/* Print extra warnings			*/
extern	char   **MakeFileNames;	/* List of pathnames of the Makefiles.	*/
extern	int	Mfileindex;	/* Current index in MakeFileNames.	*/

/*
 * Various common variables.
 */
extern	BOOL	posixmode;	/* We found a .POSIX target		    */
extern	int	Mflags;
extern	char	*ObjDir;	/* .OBJDIR: pathname Target destination dir */
extern	int	ObjDirlen;	/* strlen(.OBJDIR)			    */
extern	int	ObjSearch;	/* .OBJSEARCH: searchtype for explicit rules */
extern	list_t	*SearchList;	/* .SEARCHLIST: list of src/obj dir pairs   */
extern	list_t	*Suffixes;	/* .SUFFIXES: list of suffixes (POSIX)	    */
extern	BOOL	SSuffrules;	/* Found any simple suffix rules	    */
extern	obj_t	*Init;		/* .INIT: command to execute at startup	    */
extern	obj_t	*Done;		/* .DONE: command do execute on success	    */
extern	obj_t	*Failed;	/* .FAILED: command to execute on failure   */
extern	obj_t	*IncludeFailed;	/* .INCLUDE_FAILED: cmd to execute if missing */
extern	obj_t	*Deflt;		/* .DEFAULT: command to execute if no rule  */
extern	obj_t	*Precious;	/* .PRECIOUS: list of targets not to remove */
extern	obj_t	*Phony;		/* .PHONY: list of false targets, no check  */
extern	obj_t	*default_tgt;	/* Current or Default target		    */
extern	date_t	curtime;	/* current fime				    */
extern	date_t	newtime;	/* Special time newer than all	XXX	    */
extern	char	*gbuf;		/* Global growable buffer		    */
extern	char	*gbufend;	/* Current end of growable buffer	    */
extern	int	gbufsize;	/* Current size of buffer (bufend - buf)    */
extern	BOOL	found_make;	/* Did we expand the $(MAKE) macro?	    */

extern	int	lastc;		/* last input character			    */
extern	int	firstc;		/* first character in line		    */
extern	char	*mfname;	/* name of current make file		    */
extern	int	lineno;		/* current line number			    */
extern	int	col;		/* current column			    */

extern	char	Nullstr[];	/* global empty string			    */
extern	obj_t	*NullObj;	/* global empty string obj		    */
extern	char	slash[];	/* string holding path delimiter	    */
extern	int	slashlen;	/* strlen(slash)			    */
#define	SLASH	PATH_DELIM

extern	patr_t	*Patrules;
extern	patr_t	**pattail;

extern	char	chartype[256];

/*
 * Definitiond for chartype[] bits:
 */
#define	DYNCHAR	0x01
#define	NUMBER	0x02

/*
 * Definitions for Makefile Index values
 * The values need to be kept in sync with initmakefiles() in make.c
 */
#define	MF_IDX_IMPLICIT	0		/* Implicit rules		*/
#define	MF_IDX_ENVIRON	1		/* Environment strings		*/
#define	MF_IDX_MAKEFILE	2		/* Default make file		*/
#define	MF_IDX_BASE	MF_IDX_MAKEFILE	/* The base for -f makefiles	*/
