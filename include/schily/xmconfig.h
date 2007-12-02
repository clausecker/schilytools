/* @(#)xmconfig.h	1.34 06/10/21 Copyright 1995-2006 J. Schilling */
/*
 *	This file either includes the manual generated
 *	static definitions for a machine configuration.
 *
 *	Copyright (c) 1995-2006 J. Schilling
 *
 *	Use only cpp instructions.
 *
 *	NOTE: SING: (Schily Is Not Gnu)
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

#ifndef _SCHILY_XMCONFIG_H
#define	_SCHILY_XMCONFIG_H

/*
 * This is the only static (hand crafted) set of machine configuration
 * left over. We are trying to avoid even this in future.
 */


#ifdef __cplusplus
extern "C" {
#endif

#if defined(VMS)
#	define	PROTOTYPES
#	define	HAVE_OSDEF	/* prevent later definitions to overwrite current */
#	define	__NOT_SVR4__	/* Not a real SVR4 implementation */

#	define	HAVE_UNISTD_H
/*  #	define	HAVE_SYS_TIME_H	*/
#	define	HAVE_FCNTL_H
#	define	HAVE_USLEEP
#	if __DECC_VER >= 60000000
#		define HAVE_INTTYPES_H
#	endif /* __DECC_VER >= 60000000 */
#	define SIZEOF_CHAR 1
#	define SIZEOF_UNSIGNED_CHAR SIZEOF_CHAR
#	define SIZEOF_SHORT_INT 2
#	define SIZEOF_UNSIGNED_SHORT_INT SIZEOF_SHORT_INT
#	define SIZEOF_INT 4
#	define SIZEOF_UNSIGNED_INT SIZEOF_INT
#	define SIZEOF_LONG_INT 4
#	define SIZEOF_UNSIGNED_LONG_INT SIZEOF_LONG_INT
#	ifdef __VAX
#		define SIZEOF_LONG_LONG 4
#	else /* def __VAX */
#	endif /* def __VAX [else] */
#	define SIZEOF_UNSIGNED_LONG_LONG SIZEOF_LONG_LONG
#	define SIZEOF_CHAR_P 4

#	include <types.h>
#	ifndef __OFF_T
		typedef int off_t;
#	endif /* ndef __OFF_T */

#	if defined(__alpha)
#		define HOST_CPU	"Alpha"
#	elif defined(__ia64)
#		define HOST_CPU	"IA64"
#	elif defined(__vax)
#		define HOST_CPU	"VAX"
#	else
#		define HOST_CPU	"VAX(?)"
#	endif
#	define HOST_VENDOR "HP"
#	define HOST_OS "VMS/OpenVMS"
#	define RETSIGTYPE   void
/*#	define	HAVE_ELF    */
/*#	define	HAVE_COFF   */
/*#	define	HAVE_AOUT   */
#	define	HAVE_STDARG_H
/*#	define	HAVE_VA_COPY	*/
#	define	HAVE_STDLIB_H
#	define	HAVE_STRING_H
#	define	HAVE_STDC_HEADERS
#	define	STDC_HEADERS
/*#	define	HAVE_LONGLONG	*/

#	define	HAVE_UNISTD_H
#	define	HAVE_FCNTL_H
#	define	HAVE_DIRENT_H
/*#	define	HAVE_SYS_DIR_H	*/
/*#	define	HAVE_NDIR_H */
/*#	define	HAVE_SYS_NDIR_H	*/
/*#	define	HAVE_TERMIOS_H	*/
/*#	define	HAVE_TERMIO_H	*/
/*#	define	HAVE_SYS_TIME_H	*/
/*#	define	HAVE_UTIMES */
/*#	define	HAVE_UTIME_H	*/
#	define	HAVE_WAIT_H
/*#	define	HAVE_SYS_WAIT_H	*/
/*#	define	HAVE_SYS_SYSTEMINFO_H	*/
/*#	define	HAVE_SYS_UTSNAME_H  */
/*#	define	HAVE_SYS_PRIOCNTL_H */
/*#	define	HAVE_SYS_RTPRIOCNTL_H	*/
/*#	define	HAVE_SYS_MTIO_H	*/
/*#	define	HAVE_MTGET_DSREG    */
/*#	define	HAVE_MTGET_RESID    */
/*#	define	HAVE_MTGET_FILENO   */
/*#	define	HAVE_MTGET_BLKNO    */
/*#	define	MAJOR_IN_MKDEV	*/
/*#	define	MAJOR_IN_SYSMACROS  */
/*#	define	HAVE_USG_STDIO	*/
/*#	define	HAVE_DTOA   */
#	define	HAVE_GETCWD
#	define	HAVE_STRERROR
#	define	HAVE_MEMMOVE
#	define	HAVE_MMAP
/*#	define	HAVE_SMMAP  */
/*#	define	HAVE_USGSHM */
/*#	define	HAVE_USGSEM */
/*#	define	HAVE_MSEM   */
/*#	define	HAVE_LDSTUB */
/*#	define	HAVE_XCHG   */
#	define	HAVE_FLOCK
/*#	define	HAVE_FCHDIR */
/*#	define	HAVE_STATVFS	*/
/*#	define	HAVE_QUOTA  */
/*#	define	HAVE_YP	*/
/*#	define	HAVE_SHADOW */
/*#	define	HAVE_SETREUID	*/
/*#	define	HAVE_SETRESUID	*/
/*#	define	HAVE_SETEUID	*/
/*#	define	HAVE_LCHOWN */
/*#	define	HAVE_PROCFS */
/*#	define	HAVE_PROCFS2	*/
/*#	define	HAVE_SIGINFO	*/
/*#	define	HAVE_WAIT3  */
/*#	define	HAVE_WAITPID	*/
/*#	define	HAVE_UNION_WAIT	*/
#	define	HAVE_GETHOSTNAME
/*#	define	HAVE_STREAMS	*/
/*#	define	HAVE_STRPTYS	*/
/*#	define	HAVE_POLL   */
#	define	HAVE_SELECT
/*#	define	HAVE_TIRPC  */
/*#	define	GID_T		gid_t	*/
#	define	USLEEPRETURN_T	uint
/*#	define	USLEEPRETURN_T	void	*/
/*#	define	USLEEPISVOID	*/
/*#	define	HAVE_GETAV0 */
/*#	define	HAVE_SCANSTACK	*/

/* 2005-11-22 SMS.  Enabled some above.  Added some below. */
#	define	HAVE_ECVT
#	define	HAVE_FCVT
#	define	HAVE_GCVT
#	define	HAVE_NICE 1
#	define	HAVE_SELECT
#	define	HAVE_STRCASECMP
#	if __CRTL_VER >= 70312000
#		define HAVE_SNPRINTF
#	endif /* __CRTL_VER >= 70312000 */
#	define	NICE_DECR -8

/* 2005-03-14 SMS.  Need VMS-specific open() parameters. */
#	define _OPENFD_SRC	/* Use VMS-specific _openfd() function.*/
#	define _openfd openfd_vms	/* This one. */
#	define O_BINARY 0x0004	/* DOS-like value.  Implies "ctx=bin". */

#	define HAVE_C_BIGENDIAN
#	define HAVE_C_BITFIELDS
#	define BITFIELDS_LTOH

/* 2006-09-14 SMS.  Various things for libfind. */
#	define fork fork_dummy
#	ifndef _POSIX_ARG_MAX
#		define _POSIX_ARG_MAX 4096	/* Probably needed. */
#	endif /* ndef _POSIX_ARG_MAX */
#	define HAVE_DECL_STAT
#	define HAVE_DECL_LSTAT

/* 2006-09-17 SMS. */
#	if __CRTL_VER >= 70000000
#		define HAVE_GETPAGESIZE
#	endif /* __CRTL_VER >= 70000000 */

	extern pid_t fork_dummy(void);

#endif /* defined(VMS) */


#ifdef __cplusplus
}
#endif

#endif /* _SCHILY_XMCONFIG_H */
