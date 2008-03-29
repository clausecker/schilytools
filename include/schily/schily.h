/* @(#)schily.h	1.69 08/02/16 Copyright 1985-2007 J. Schilling */
/*
 *	Definitions for libschily
 *
 *	This file should be included past:
 *
 *	schily/mconfig.h / config.h
 *	schily/standard.h
 *	stdio.h
 *	stdlib.h	(better use schily/stdlib.h)
 *	unistd.h	(better use schily/unistd.h) needed f. LARGEFILE support
 *	schily/string.h
 *	sys/types.h
 *
 *	If you need stdio.h, you must include it before schily/schily.h
 *
 *	NOTE: If you need ctype.h and did not include stdio.h you need to
 *	include ctype.h past schily/schily.h as OpenBSD does not follow POSIX
 *	and defines EOF in ctype.h
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

#ifndef _SCHILY_SCHILY_H
#define	_SCHILY_SCHILY_H

#ifndef _SCHILY_STANDARD_H
#include <schily/standard.h>
#endif
#ifndef _SCHILY_CCOMDEFS_H
#include <schily/ccomdefs.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_INCL_SYS_TYPES_H) || defined(off_t)
#	ifndef	FOUND_OFF_T
#	define	FOUND_OFF_T
#	endif
#endif
#if	defined(_INCL_SYS_TYPES_H) || defined(size_t)
#	ifndef	FOUND_SIZE_T
#	define	FOUND_SIZE_T
#	endif
#endif
#if	defined(_MSC_VER) && !defined(_SIZE_T_DEFINED)
#	undef	FOUND_SIZE_T
#endif

#ifdef	__never_def__
/*
 * It turns out that we cannot use the folloginw definition because there are
 * some platforms that do not behave application friendly. These are mainly
 * BSD-4.4 based systems (which #undef a definition when size_t is available.
 * We actually removed this code because of a problem with QNX Neutrino.
 * For this reason, it is important not to include <sys/types.h> directly but
 * via the Schily SING include files so we know whether it has been included
 * before we come here.
 */
#if	defined(_SIZE_T)	|| defined(_T_SIZE_)	|| defined(_T_SIZE) || \
	defined(__SIZE_T)	|| defined(_SIZE_T_)	|| \
	defined(_GCC_SIZE_T)	|| defined(_SIZET_)	|| \
	defined(__sys_stdtypes_h) || defined(___int_size_t_h) || defined(size_t)

#ifndef	FOUND_SIZE_T
#	define	FOUND_SIZE_T	/* We already included a size_t definition */
#endif
#endif
#endif	/* __never_def__ */

#if	defined(HAVE_LARGEFILES)
#	define	_fcons		_fcons64
#	define	fdup		fdup64
#	define	fileluopen	fileluopen64
#	define	fileopen	fileopen64
#	define	filemopen	filemopen64
#	define	filepos		filepos64
#	define	filereopen	filereopen64
#	define	fileseek	fileseek64
#	define	filesize	filesize64
#	define	filestat	filestat64
#	define	_openfd		_openfd64
#endif

#ifdef	EOF	/* stdio.h has been included */

extern	int	_cvmod __PR((const char *, int *, int *));
extern	FILE	*_fcons __PR((FILE *, int, int));
extern	FILE	*fdup __PR((FILE *));
#if	!defined(fdown) || defined(PROTOTYPES)
/*
 * We cannot declare fdown() with K&R in case that fdown() has been #define'd
 */
extern	int	fdown __PR((FILE *));
#endif
extern	int	fexecl __PR((const char *, FILE *, FILE *, FILE *,
							const char *, ...));
extern	int	fexecle __PR((const char *, FILE *, FILE *, FILE *,
							const char *, ...));
		/* 6th arg not const, fexecv forces av[ac] = NULL */
extern	int	fexecv __PR((const char *, FILE *, FILE *, FILE *, int,
							char **));
extern	int	fexecve __PR((const char *, FILE *, FILE *, FILE *,
					char * const *, char * const *));
extern	int	fspawnv __PR((FILE *, FILE *, FILE *, int, char * const *));
extern	int	fspawnl __PR((FILE *, FILE *, FILE *,
					const char *, const char *, ...));
extern	int	fspawnv_nowait __PR((FILE *, FILE *, FILE *,
					const char *, int, char *const*));
extern	int	fgetline __PR((FILE *, char *, int));
extern	int	fgetstr __PR((FILE *, char *, int));
extern	int	file_getraise __PR((FILE *));
extern	void	file_raise __PR((FILE *, int));
extern	int	fileclose __PR((FILE *));
extern	FILE	*fileluopen __PR((int, const char *));
extern	FILE	*fileopen __PR((const char *, const char *));
#ifdef	_INCL_SYS_TYPES_H
extern	FILE	*filemopen __PR((const char *, const char *, mode_t));
#endif
#ifdef	FOUND_OFF_T
extern	off_t	filepos __PR((FILE *));
#endif
extern	int	fileread __PR((FILE *, void *, int));
extern	int	ffileread __PR((FILE *, void *, int));
extern	FILE	*filereopen __PR((const char *, const char *, FILE *));
#ifdef	FOUND_OFF_T
extern	int	fileseek __PR((FILE *, off_t));
extern	off_t	filesize __PR((FILE *));
#endif
#ifdef	S_IFMT
extern	int	filestat __PR((FILE *, struct stat *));
#endif
extern	int	filewrite __PR((FILE *, void *, int));
extern	int	ffilewrite __PR((FILE *, void *, int));
extern	int	flush __PR((void));
extern	int	fpipe __PR((FILE **));
/*extern	int	fprintf __PR((FILE *, const char *, ...)) __printflike__(2, 3);*/
extern	int	getbroken __PR((FILE *, char *, char, char **, int));
extern	int	ofindline __PR((FILE *, char, const char *, int,
							char **, int));
extern	int	peekc __PR((FILE *));

#ifdef	__never_def__
/*
 * We cannot define this or we may get into problems with DOS based systems.
 */
extern	int	spawnv __PR((FILE *, FILE *, FILE *, int, char * const *));
extern	int	spawnl __PR((FILE *, FILE *, FILE *,
					const char *, const char *, ...));
extern	int	spawnv_nowait __PR((FILE *, FILE *, FILE *,
					const char *, int, char *const*));
#endif	/* __never_def__ */
#endif	/* EOF */

#ifdef	_SCHILY_UTYPES_H
typedef struct gnmult {
	char	key;
	Llong	mult;
} gnmult_t;

extern	int	getllnum __PR((char *arg, Llong *lvalp));
extern	int	getxnum	 __PR((char *arg, long *valp, gnmult_t *mult));
extern	int	getllxnum __PR((char *arg, Llong *lvalp, gnmult_t *mult));

extern	int	getlltnum  __PR((char *arg, Llong *lvalp));
extern	int	getxtnum  __PR((char *arg, time_t *valp, gnmult_t *mult));
extern	int	getllxtnum __PR((char *arg, Llong *lvalp, gnmult_t *mult));
#endif
extern	int	getnum	__PR((char *arg, long *valp));
#ifdef	_SCHILY_TYPES_H
extern	int	gettnum	__PR((char *arg, time_t *valp));
#endif

#ifdef	EOF			/* stdio.h has been included */
#ifdef	_INCL_SYS_TYPES_H
/*
 * getperm() flags:
 */
#define	GP_NOX		0	/* This is not a dir and 'X' is not valid */
#define	GP_DOX		1	/* 'X' perm character is valid		  */
#define	GP_XERR		2	/* 'X' perm characters are invalid	  */
#define	GP_FPERM	4	/* TRUE if we implement find -perm	  */

extern	int	getperm	__PR((FILE *f, char *perm, char *opname, \
				mode_t *modep, int smode, int flag)); 
#endif
#endif

extern	int	_niread __PR((int, void *, int));
extern	int	_niwrite __PR((int, void *, int));
extern	int	_nixread __PR((int, void *, int));
extern	int	_nixwrite __PR((int, void *, int));
extern	int	_openfd __PR((const char *, int));
extern	int	on_comerr __PR((void (*fun)(int, void *), void *arg));
/*PRINTFLIKE1*/
extern	void	comerr __PR((const char *, ...)) __printflike__(1, 2);
/*PRINTFLIKE2*/
extern	void	comerrno __PR((int, const char *, ...)) __printflike__(2, 3);
/*PRINTFLIKE1*/
extern	int	errmsg __PR((const char *, ...)) __printflike__(1, 2);
/*PRINTFLIKE2*/
extern	int	errmsgno __PR((int, const char *, ...)) __printflike__(2, 3);
#ifdef	FOUND_SIZE_T
/*PRINTFLIKE3*/
extern	int	serrmsg __PR((char *, size_t, const char *, ...)) __printflike__(3, 4);
/*PRINTFLIKE4*/
extern	int	serrmsgno __PR((int, char *, size_t, const char *, ...)) __printflike__(4, 5);
#endif
extern	void	comexit	__PR((int));
extern	char	*errmsgstr __PR((int));

#ifdef	EOF	/* stdio.h has been included */
/*PRINTFLIKE2*/
extern	void	fcomerr		__PR((FILE *, const char *, ...)) __printflike__(2, 3);
/*PRINTFLIKE3*/
extern	void	fcomerrno	__PR((FILE *, int, const char *, ...)) __printflike__(3, 4);
/*PRINTFLIKE2*/
extern	int	ferrmsg		__PR((FILE *, const char *, ...)) __printflike__(2, 3);
/*PRINTFLIKE3*/
extern	int	ferrmsgno	__PR((FILE *, int, const char *, ...)) __printflike__(3, 4);
#ifdef	_SCHILY_VARARGS_H
/*PRINTFLIKE4*/
extern	int	_comerr		__PR((FILE *, int, int, const char *, va_list));
#endif
#endif

/*PRINTFLIKE1*/
extern	int	error __PR((const char *, ...)) __printflike__(1, 2);
extern	char	*fillbytes __PR((void *, int, char));
extern	char	*findbytes __PR((const void *, int, char));
extern	int	findline __PR((const char *, char, const char *,
							int, char **, int));
extern	int	getline __PR((char *, int));
extern	int	getstr __PR((char *, int));
extern	int	breakline __PR((char *, char, char **, int));
extern	int	getallargs __PR((int *, char * const**, const char *, ...));
extern	int	getargs __PR((int *, char * const**, const char *, ...));
extern	int	getfiles __PR((int *, char * const**, const char *));
extern	char	*astoi __PR((const char *, int *));
extern	char	*astol __PR((const char *, long *));
extern	char	*astolb __PR((const char *, long *, int base));
#ifdef	_SCHILY_UTYPES_H
extern	char	*astoll __PR((const char *, Llong *));
extern	char	*astollb __PR((const char *, Llong *, int));
extern	char	*astoull __PR((const char *, Ullong *));
extern	char	*astoullb __PR((const char *, Ullong *, int));
#endif

/*extern	void	handlecond __PR((const char *, SIGBLK *, int(*)(const char *, long, long), long));*/
/*extern	void	unhandlecond __PR((SIGBLK *));*/

extern	int		patcompile __PR((const unsigned char *, int, int *));
extern	unsigned char	*patmatch __PR((const unsigned char *, const int *,
					const unsigned char *, int, int, int, int[]));
extern	unsigned char	*patlmatch __PR((const unsigned char *, const int *,
					const unsigned char *, int, int, int, int[]));

/*extern	int	printf __PR((const char *, ...)) __printflike__(1, 2);*/
extern	char	*movebytes __PR((const void *, void *, int));

extern	void	save_args __PR((int, char **));
extern	int	saved_ac __PR((void));
extern	char	**saved_av __PR((void));
extern	char	*saved_av0 __PR((void));
#ifndef	seterrno
extern	int	seterrno __PR((int));
#endif
extern	void	set_progname __PR((const char *));
extern	char	*get_progname __PR((void));

extern	void	setfp __PR((void * const *));
extern	int	wait_chld __PR((int));		/* for fspawnv_nowait() */
extern	int	geterrno __PR((void));
extern	void	raisecond __PR((const char *, long));
#ifdef	FOUND_SIZE_T
/*
 * We currently cannot define this here because there IXIX has a definition
 * than violates the standard.
 */
#ifndef	HAVE_SNPRINTF
/*PRINTFLIKE3*/
extern	int	snprintf __PR((char *, size_t, const char *, ...)) __printflike__(3, 4);
#endif
#endif
/*extern	int	sprintf __PR((char *, const char *, ...)); ist woanders falsch deklariert !!!*/
extern	char	*strcatl __PR((char *, ...));
extern	int	streql __PR((const char *, const char *));
#ifdef	va_arg
extern	int	format __PR((void (*)(char, long), long, const char *, va_list));
#else
extern	int	format __PR((void (*)(char, long), long, const char *, void *));
#endif

extern	int	ftoes __PR((char *, double, int, int));
extern	int	ftofs __PR((char *, double, int, int));

#ifdef	EOF	/* stdio.h has been included */
/*PRINTFLIKE2*/
extern	int	js_fprintf	__PR((FILE *, const char *, ...)) __printflike__(2, 3);
/*PRINTFLIKE1*/
extern	int	js_printf	__PR((const char *, ...)) __printflike__(1, 2);
#ifdef	FOUND_SIZE_T
/*PRINTFLIKE3*/
extern	int	js_snprintf	__PR((char *, size_t, const char *, ...)) __printflike__(3, 4);
#endif
/*PRINTFLIKE2*/
extern	int	js_sprintf	__PR((char *, const char *, ...)) __printflike__(2, 3);
#endif	/* EOF */

extern	void	swabbytes	__PR((void *, int));
extern	char	**getmainfp	__PR((void));
extern	char	**getavp	__PR((void));
extern	char	*getav0		__PR((void));
extern	void	**getfp		__PR((void));
extern	int	flush_reg_windows __PR((int));
extern	int	cmpbytes	__PR((const void *, const void *, int));
extern	int	cmpnullbytes	__PR((const void *, int));

#ifdef	nonono
#if	defined(HAVE_LARGEFILES)
/*
 * To allow this, we need to figure out how to do autoconfiguration for off64_t
 */
extern	FILE	*_fcons64	__PR((FILE *, int, int));
extern	FILE	*fdup64		__PR((FILE *));
extern	FILE	*fileluopen64	__PR((int, const char *));
extern	FILE	*fileopen64	__PR((const char *, const char *));
#ifdef	FOUND_OFF_T
extern	off64_t	filepos64	__PR((FILE *));
#endif
extern	FILE	*filereopen64	__PR((const char *, const char *, FILE *));
#ifdef	FOUND_OFF_T
extern	int	fileseek64	__PR((FILE *, off64_t));
extern	off64_t	filesize64	__PR((FILE *));
#endif
#ifdef	S_IFMT
extern	int	filestat64	__PR((FILE *, struct stat *));
#endif
extern	int	_openfd64	__PR((const char *, int));
#endif
#endif

#ifdef	SCHILY_PRINT
#undef	fprintf
#define	fprintf		js_fprintf
#undef	printf
#define	printf		js_printf
#undef	snprintf
#define	snprintf	js_snprintf
#undef	sprintf
#define	sprintf		js_sprintf
#else
#ifndef	HAVE_SNPRINTF
#undef	snprintf
#define	snprintf	js_snprintf
#endif
#endif


#ifdef	FOUND_SIZE_T
extern	void	*js_malloc	__PR((size_t size, char *msg));
extern	void	*js_realloc	__PR((void *ptr, size_t size, char *msg));
#endif
extern	char	*js_savestr	__PR((const char *s));

#ifdef	_SCHILY_JMPDEFS_H

/*
 * Special valued for the "jmp" parameter.
 */
#define	JM_EXIT		((sigjmps_t *)-1) /* Call comexit(errno) instead of siglongjmp() */
#define	JM_RETURN	((sigjmps_t *)0)  /* Return instead of calling siglongjmp() */

#ifdef	FOUND_SIZE_T
extern	void	*js_jmalloc	__PR((size_t size, char *msg, sigjmps_t *jmp));
extern	void	*js_jrealloc	__PR((void *ptr, size_t size, char *msg, sigjmps_t *jmp));
#endif
extern	char	*js_jsavestr	__PR((const char *s, sigjmps_t *jmp));

#ifdef	EOF	/* stdio.h has been included */
#ifdef	FOUND_SIZE_T
extern	void	*js_fjmalloc	__PR((FILE *f, size_t size, char *msg, sigjmps_t *jmp));
extern	void	*js_fjrealloc	__PR((FILE *f, void *ptr, size_t size, char *msg, sigjmps_t *jmp));
#endif
extern	char	*js_fjsavestr	__PR((FILE *f, const char *s, sigjmps_t *jmp));
#endif	/* EOF */
#endif	/* _SCHILY_JMPDEFS_H */

#define	__malloc	js_malloc
#define	__realloc	js_realloc
#define	__savestr	js_savestr
#define	__jmalloc	js_jmalloc
#define	__jrealloc	js_jrealloc
#define	__jsavestr	js_jsavestr
#define	__fjmalloc	js_fjmalloc
#define	__fjrealloc	js_fjrealloc
#define	__fjsavestr	js_fjsavestr

#ifdef	__cplusplus
}
#endif

#if defined(_JOS) || defined(JOS)
#	ifndef	_SCHILY_JOS_IO_H
#	include <schily/jos_io.h>
#	endif
#endif

#endif	/* _SCHILY_SCHILY_H */
