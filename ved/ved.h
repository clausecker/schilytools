/* @(#)ved.h	1.38 07/03/08 Copyright 1984, 85, 86, 88, 89, 97, 2000-2004 J. Schilling */
/*
 *	Main include file for VED
 *
 *	Copyright (c) 1984, 85, 86, 88, 89, 97, 2000-2004 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/standard.h>
#include <schily/schily.h>

#define	C	(char *)
#define	UC	(unsigned char *)
#define	CP	(char **)
#define	UCP	(unsigned char **)
#define	CPP	(char ***)
#define	UCPP	(unsigned char ***)

typedef	unsigned char	echar_t;

/*#define	USE_LONGLONG_POS*/

#if	defined(HAVE_LARGEFILES) || defined(USE_LONGLONG_POS)
/*
 * do not use long long to allow compilation on non long long
 * aware systems.
 */
/*typedef	long long	epos_t;*/
/*typedef	long long	ecnt_t;*/
/*
 * Rather use the "best effort" long long "Llong"
 */
typedef	Llong		epos_t;
typedef	Llong		ecnt_t;
#else
typedef	long		epos_t;
typedef	long		ecnt_t;
#endif

#ifdef	TESTLONGLONG
enum nicht { blau, besoffen };

/*typedef	long		epos_t;*/
/*typedef	Llong		epos_t;*/
/*typedef	double		epos_t;*/
/*typedef	char		epos_t;*/
/*typedef	enum nicht	epos_t;*/

/*typedef	double	ecnt_t;*/
/*typedef	Llong	ecnt_t;*/
#endif

/*
 * We do out own buffering for outout to the screen to make ved
 * fast on all operating systems independent of the I/O implementation.
 * Our output buffer size should be more than the size of a usual screen.
 * As most lines are shorter that 40 characters, a bufsize of 4096 chars
 * will be sufficient for a 100 line display.
 */
#define	BFSIZE	8192

#ifdef	BFSIZE
typedef struct {
	short	_count;
	Uchar	_buf[BFSIZE];
	Uchar	*_ptr;
} iobuf_t;

extern	iobuf_t	_bb;

#ifdef	putchar
#	undef	putchar
#endif
#ifdef	JOS
#	define	putchar(c) {if (--_bb._count >= 0) *_bb._ptr++ = (c); \
					else _bflush(c); }
#else
#	define	putchar(c) (--_bb._count >= 0 ? *_bb._ptr++ = (c): _bflush(c))
#endif

#ifdef	flush
#	undef	flush
#endif
#define	flush()	_bufflush()
#endif	/* BFSIZE */

#ifdef	JOS
#	define	ENOENT	ENOFILE
#endif

#define	SP	0x20
#define	TAB	0x09
#define	DEL	0x7F
#define	SP8	0xA0
#define	DEL8	0xFF

#define	TABNONE	255				/* No table		*/
#define	TABFIRST CTAB				/* The first table	*/
#define	CTAB	0				/* The chartable	*/
#define	ESCTAB	1				/* The ESC table	*/
#define	ALTTAB	2				/* The ALT table	*/
#define	AESCTAB	3				/* The ALT-ESC table	*/
#define	XTAB	4				/* The X table		*/
#define	TABLAST	XTAB				/* The last table number */
#define	NCTAB	(TABLAST+1)			/* The number of tables	*/

/*
 * XXX Only correct on a two's complement machine.
 */
#define	MAXULONG	(~((unsigned long)0))
#define	MAXLONG		(MAXULONG >> 1)

#define	min(x, y)	(((x) < (y)) ? (x) : (y))
#define	max(x, y)	(((x) < (y)) ? (y) : (x))
/*#define	abs(x)	(((x) > 0) ? (x) : (-x))*/

#define	TMPNSIZE	28	/* Size of temporary filename buffers	*/
#define	NAMESIZE	256	/* Size of several temporary buffers	*/
#define	FNAMESIZE	1024	/* Size of edited filename		*/

#ifdef	JOS
#	define	DEFSHELL	"/bin/command"
#	define	HELPFILE UC "/doc/cmds/ved.help"
#else
#	define	DEFSHELL	"/bin/sh"
#	define	HELPFILE UC "/opt/schily/man/help/ved.help"
#endif

/*
 * A pair of horizontal and vertical position.
 */
typedef struct {
	int	hp;
	int	vp;
} cpos_t;

typedef struct {
	/*
	 * Current Window status.
	 */
	epos_t	dot;		/* The byte offset of the cursor position   */
	epos_t	eof;		/* The byte offset of the last character + 1 */
	epos_t	mark;		/* The byte offset of the selection mark    */
	epos_t	window;		/* The byte offset of the window start	    */
	epos_t	ewindow;	/* The byte offset of the window end	    */

	int	eflags;		/* XXX					    */
	BOOL	markvalid;	/* if true: there is a mark		    */
	BOOL	overstrikemode;	/* if true: overwrite in favor of insert    */
	BOOL	visible;	/* if true: make TAB, CR, EOL, EOF visible  */
	BOOL	dosmode;	/* if true: supress '\r' in \r\n sequence   */
	BOOL	raw8;		/* if true: do not map non acsii 8bit chars */
	BOOL	markwrap;	/* if true: mark linewraps with a '\\' at eol */

	int	pmargin;	/* Max Abstand vom oberen/unteren Rand	    */
	int	tabstop;	/* Breite eines 'tabs'			    */
	int	optline;	/* Vorzugszeile fuer Window - Adjust	    */

	int	wrapmargin;	/* autowrap based on margin from linelen    */
	int	maxlinelen;	/* autowrap based on linelen		    */
	BOOL	autoindent;	/* whether to indent next line like current */

	void	*bhead;		/* Head of the linked list of used headers  */
	void	*gaplink;	/* Pointer to the header where the gap starts */
	epos_t	gap;		/* Byte offset of first char after the gap   */
	int	gapoff;		/* Offset within the header the gap starts in */

				/* Three variables used by findpos() only   */
	void	*winlink;	/* Header containing the start of the window */
	epos_t	winpos;		/* Byte offset of the current window start  */
	epos_t	winoff;		/* Byte offset for start of data in winlink */

	Uchar	*curfile;	/* current file name			    */
	FILE	*curfp;		/* current file pointer (if file is locked) */
	long	curftime;	/* last modification time of current file   */
	int	curfd;		/* fd used to hold writelock to current file */
	Uchar	lastch;		/* a global copy of the last read char	    */
	BOOL	magic;		/* wether to search in 'magic' mode	    */
	long	modflg;		/* number of modifications since last save  */
	ecnt_t	curnum;		/* mult # fot next edit command		    */
	ecnt_t	number;		/* curnum is copied from this mult # master */
	int	column;		/* the remembered not mapped hp of the cursor */
	/*
	 * Global stuff.
	 */
	int	psize;		/* page size of the screen		    */
	int	llen;		/* line length of the screen		    */
} ewin_t;

extern	cpos_t	cursor;		/* position of cursor in window hp not mapped */
extern	cpos_t	cpos;		/* real pos of cursor on screen hp mapped   */

#define	COLUPDATE	0x0001	/* if set: update 'column' to current hp    */
#define	SAVENUM		0x0002	/* if set: keep value in 'number'	    */
#define	SAVEDEL		0x0004	/* if set: keep next del in delete buffer   */
#define	KEEPDEL		0x0010	/* if set: keep content of delete buffer    */
#define	DELDONE		0x0020	/* if set: last command filled delete buffer */

#define	FREADONLY	0x10000	/* if set: forced readonly on current file  */
#define	FNOLOCK		0x20000	/* if set: could not lock file		    */
#define	FLOCKLOCAL	0x40000	/* if set: lock to file is local only	    */

#ifdef	notused
#define	MARKVALID	0x0008	/* if set: there is a mark		    */
#define	OVERSTRIKE	0x0100	/* if set: overwrite in favor of insert	    */
#define	VISIBLE		0x0200	/* if set: make TAB, CR, EOL, EOF visible   */
#define	DOSMODE		0x0400	/* if set: supress '\r' in \r\n sequence    */
#define	RAW8		0x0800	/* if set: do not map non acsii 8bit chars  */
#define	MARKWRAP	0x1000	/* if set: mark linewraps with a '\\' at eol */
#endif

#define	LOCK_OK		2	/* A networkwide file lock is present	    */
#define	LOCK_LOCAL	1	/* The lock is local only because of flock() */
#define	LOCK_CANNOT	0	/* The system does not support to lock	    */
#define	LOCK_ALREADY	-1	/* The file is already locked		    */


extern	int	pid;		/* process id used for unique tmp file names */

extern	int	mflag;		/* if > 0 : take characters from macro	    */


/*
 * Used by ved.c and io.c
 */
extern	char	ved_version[];

/*
 * Used by ved.c and *cmds.c
 */
extern	FILE	*takefile;	/* FILE * of current take buffer	    */
extern	epos_t	takesize;	/* file size of current take buffer	    */
extern	Uchar	takename[TMPNSIZE]; /* file name of current take buffer	    */

extern	FILE	*delfile;	/* FILE * of delete buffer		    */
extern	epos_t	delsize;	/* file size of delete buffer		    */
extern	Uchar	delname[TMPNSIZE]; /* file name of delete buffer	    */

extern	FILE	*rubfile;	/* FILE * of rubout buffer		    */
extern	epos_t	rubsize;	/* file size of rubout buffer		    */
extern	Uchar	rubname[TMPNSIZE]; /* file name of rubout buffer	    */

extern	int	ReadOnly;	/* if > 0 : do not allow to write back mods */
extern	int	nobak;		/* Es wird kein ___.bak angelegt	    */
extern	int	nolock;		/* Es wird kein record locking versucht	    */
extern	int	noedtmp;	/* Kein .vedtmp.* erzeugen		    */
extern	int	recover;	/* altes File reparieren		    */
extern	BOOL	autodos;	/* wp->dosmode aus isdos() bestimmen	    */
extern	int	nfiles;		/* Anzahl der zu editierenden Files	    */
extern	int	fileidx;	/* Index des editierten Files		    */
extern	Uchar	**files;	/* Array der zu editierenden Files	    */

/*
 * Used by io.c and screen.c
 */
extern	BOOL	markon;		/* if true: we are just printing the mark    */

/*
 * Used by ved.c, message.c and cmdline.c
 */
extern	BOOL	updatemsg;	/* infostr is temporary			    */
extern	BOOL	updateerr;	/* errorstr is temporary		    */
extern	BOOL	updatesysline;	/* whole systemline should be redrawn	    */

/*
 * Used by ved.c and executecmds.c
 */
extern Uchar execname[TMPNSIZE]; /* file name of execute buffer		    */

/*
 * Used by ved.c and filecmds.c
 */
extern Uchar curfname[FNAMESIZE]; /* global file name storage space	    */

/*
 * Used by tags.c and coloncmdsds.c
 */
extern int taglength;		/* Maxnimum number of chars to compare	    */
extern Uchar tags[];		/* List of files to search for tags	    */

/*
 * Make function prototypes available for all.
 */
#include "func.h"
