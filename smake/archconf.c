/* @(#)archconf.c	1.22 09/01/06 Copyright 1996-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)archconf.c	1.22 09/01/06 Copyright 1996-2009 J. Schilling";
#endif
/*
 *	Make program
 *	Architecture autoconfiguration support
 *
 *	Copyright (c) 1996-2009 by J. Schilling
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
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/schily.h>
#include "make.h"

#ifdef	NO_SYSINFO
#	ifdef	HAVE_SYS_SYSTEMINFO_H
#		undef	HAVE_SYS_SYSTEMINFO_H
#	endif
#endif

EXPORT	void	setup_arch	__PR((void));
LOCAL	BOOL	do_uname	__PR((void));
LOCAL	BOOL	do_sysinfo	__PR((void));
LOCAL	BOOL	do_sysctl	__PR((void));
LOCAL	void	do_gethostname	__PR((void));
LOCAL	void	do_defs		__PR((void));
LOCAL	void	do_archheuristics __PR((void));
LOCAL	void	archcvt		__PR((char *));
#if defined(HAVE_SYS_SYSTEMINFO_H) || defined(HAVE_SYS_SYSCTL_H)
LOCAL	void	unblank		__PR((char *));
#endif

/*
 * External interface to archconf.c. The code tries its best to get the
 * architecture definitions by calling sysinfo(), uname() or by heuristic.
 */
EXPORT void
setup_arch()
{
	if (!do_sysinfo() &&		/* Call sysinfo(2): preferred	*/
			!do_uname()) {	/* Call uname(2): still OK	*/
		/*
		 * If nothing else helps, at least try to set up $(MAKE_HOST)
		 * and some other macros that may be retrieved from
		 * CPP definitions.
		 */
		do_gethostname();	/* Try to get host name		*/
		do_defs();		/* Evaluate CPP definitions	*/
	}
	do_sysctl();

	do_archheuristics();
}

#ifdef	HAVE_UNAME		/* NeXT Step has sys/utsname but not uname() */
#include <sys/utsname.h>

/*
 * This is the interface to the syscall uname(2).
 * Note that the system call is done directly and not by calling the uname
 * shell command. If your system returns wrong information with the uname(2)
 * call, you need to fix it here.
 */
LOCAL BOOL
do_uname()
{
	struct	utsname	un;

	if (uname(&un) < 0) {
		errmsg("Cannot get host arch (uname).\n");
		return (FALSE);
	}
#ifdef	__comment__
	printf("sysname: %s nodename: %s release: %s version: %s machine: %s\n",
	un.sysname,
	un.nodename,
	un.release,
	un.version,
	un.machine);
#endif
	archcvt(un.sysname);
	archcvt(un.release);
	archcvt(un.version);
	archcvt(un.machine);

	define_var("MAKE_OS", un.sysname);		/* uname -s */
	define_var("MAKE_HOST", un.nodename);		/* uname -n */
	define_var("MAKE_OSREL", un.release);		/* uname -r */
	define_var("MAKE_OSVERSION", un.version);	/* uname -v */
	define_var("MAKE_MACH", un.machine);		/* uname -m */

#ifdef	HAVE_UTSNAME_PROCESSOR
	archcvt(un.processor);
	define_var("MAKE_ARCH", un.processor);		/* uname -p */
#endif
#ifdef	HAVE_UTSNAME_SYSNAME_HOST
	archcvt(un.sysname_host);
	define_var("MAKE_HOST_OS", un.sysname_host);	/* uname -Hs */
#endif
#ifdef	HAVE_UTSNAME_RELEASE_HOST
	archcvt(un.release_host);
	define_var("MAKE_HOST_OSREL", un.release_host);	/* uname -Hr */
#endif
#ifdef	HAVE_UTSNAME_VERSION_HOST
	archcvt(un.version_host);
	define_var("MAKE_HOST_OSVERSION", un.version_host); /* uname -Hv */
#endif

	return (TRUE);
}
#else

/*
 * Dummy for platforms that don't implement uname(2).
 */
LOCAL BOOL
do_uname()
{
	return (FALSE);
}
#endif

#ifdef	HAVE_SYS_SYSTEMINFO_H
#include <sys/systeminfo.h>

/*
 * sysinfo(2) is the preferred way to request architecture information.
 * Unfortunately, it is only present on SVr4 compliant systems.
 */
LOCAL BOOL
do_sysinfo()
{
	char	nbuf[NAMEMAX];
	BOOL	ret = TRUE;

#ifdef	SI_SYSNAME
	if (sysinfo(SI_SYSNAME, nbuf, sizeof (nbuf)) < 0) {
		ret = FALSE;
	} else {
		archcvt(nbuf);
		define_var("MAKE_OS", nbuf);		/* uname -s */
	}
#else
	ret = FALSE;
#endif

#ifdef	SI_HOSTNAME
	if (sysinfo(SI_HOSTNAME, nbuf, sizeof (nbuf)) < 0) {
		ret = FALSE;
	} else {
		define_var("MAKE_HOST", nbuf);		/* uname -n */
	}
#else
	ret = FALSE;
#endif

#ifdef	SI_RELEASE
	if (sysinfo(SI_RELEASE, nbuf, sizeof (nbuf)) < 0) {
		ret = FALSE;
	} else {
		archcvt(nbuf);
		define_var("MAKE_OSREL", nbuf);		/* uname -r */
	}
#else
	ret = FALSE;
#endif

#ifdef	SI_VERSION
	if (sysinfo(SI_VERSION, nbuf, sizeof (nbuf)) < 0) {
		ret = FALSE;
	} else {
		archcvt(nbuf);
		define_var("MAKE_OSVERSION", nbuf);	/* uname -v */
	}
#else
	ret = FALSE;
#endif

#ifdef	SI_MACHINE
	if (sysinfo(SI_MACHINE, nbuf, sizeof (nbuf)) < 0) {
		ret = FALSE;
	} else {
		archcvt(nbuf);
		define_var("MAKE_MACH", nbuf);		/* uname -m */
	}
#else
	ret = FALSE;
#endif

#ifdef	SI_ARCHITECTURE
	if (sysinfo(SI_ARCHITECTURE, nbuf, sizeof (nbuf)) >= 0) {
		archcvt(nbuf);
		define_var("MAKE_ARCH", nbuf);		/* uname -p */
	}
#endif

#ifdef	SI_PLATFORM
	if (sysinfo(SI_PLATFORM, nbuf, sizeof (nbuf)) >= 0) {
		unblank(nbuf);
		define_var("MAKE_MODEL", nbuf);		/* uname -i */
	}
#endif

#ifdef	SI_HW_PROVIDER
	if (sysinfo(SI_HW_PROVIDER, nbuf, sizeof (nbuf)) >= 0) {
		unblank(nbuf);
/*		archcvt(nbuf);*/
		define_var("MAKE_BRAND", nbuf);
	}
#endif

#ifdef  SI_HW_SERIAL
	if (sysinfo(SI_HW_SERIAL, nbuf, sizeof (nbuf)) >= 0) {
/*		archcvt(nbuf);*/
		define_var("MAKE_HWSERIAL", nbuf);
	}
#endif

#ifdef  SI_SRPC_DOMAIN
	if (sysinfo(SI_SRPC_DOMAIN, nbuf, sizeof (nbuf)) >= 0) {
		define_var("MAKE_DOMAIN", nbuf);
	}
#endif

#ifdef  SI_ISALIST
	if (sysinfo(SI_ISALIST, nbuf, sizeof (nbuf)) >= 0) {
		define_var("MAKE_ISALIST", nbuf);
	}
#endif

	return (ret);
}
#else

/*
 * Dummy for platforms that don't implement sysinfo(2).
 */
LOCAL BOOL
do_sysinfo()
{
	return (FALSE);
}
#endif

#ifdef	HAVE_SYS_SYSCTL_H
#include <schily/types.h>
#include <schily/param.h>
#include <sys/sysctl.h>

#ifdef	HAVE_MACH_MACHINE_H
#include <mach/machine.h>
#endif
#ifdef	IS_MACOS_X
#include <mach-o/arch.h>
#endif

/*
 * See #ifdef statement below in unblank()w
 */
LOCAL BOOL
do_sysctl()
{
#if	defined(HW_MODEL) || defined(HW_MACHINE_ARCH)
	obj_t	*o;
	char	nbuf[NAMEMAX];
	size_t	len;
	int	mib[2];
#endif

#if	defined(HW_MODEL)
	o = objlook("MAKE_MODEL", FALSE);
	if (o == NULL) {
		mib[0] = CTL_HW;
		mib[1] = HW_MODEL;
		len = sizeof (nbuf);
		if (sysctl(mib, 2, nbuf, &len, 0, 0) >= 0) {
			unblank(nbuf);
			define_var("MAKE_MODEL", nbuf);
		}
	}
#endif	/* defined(HW_MODEL) */

#if	defined(HW_MACHINE_ARCH)
	o = objlook("MAKE_ARCH", FALSE);
	if (o == NULL) {
		mib[0] = CTL_HW;
		mib[1] = HW_MACHINE_ARCH;
		len = sizeof (nbuf);
		if (sysctl(mib, 2, nbuf, &len, 0, 0) >= 0) {
			archcvt(nbuf);
			define_var("MAKE_ARCH", nbuf);
		}
#ifdef	IS_MACOS_X
		/*
		 * Mac OS X fails with HW_MACHINE_ARCH
		 */
		else {
			char			*name = NULL;
			cpu_type_t		cputype = 0;
			NXArchInfo const	*ai;

			len = sizeof (cputype);
			if (sysctlbyname("hw.cputype", &cputype, &len,
								NULL, 0) == 0 &&
			    (ai = NXGetArchInfoFromCpuType(cputype,
					    CPU_SUBTYPE_MULTIPLE)) != NULL) {
				strlcpy(nbuf, (char *)ai->name, sizeof (nbuf));
				archcvt(nbuf);
				name = nbuf;
			}
			if (cputype == CPU_TYPE_POWERPC &&
			    name != NULL && strncmp(name, "ppc", 3) == 0) {
				name = "powerpc";
			}
			if (name != NULL)
				define_var("MAKE_ARCH", name);
		}
#endif	/* IS_MACOS_X */
	}
#endif	/* defined(HW_MACHINE_ARCH) */
	return (TRUE);
}
#else
/*
 * Dummy for platforms that don't implement sysctl().
 */
LOCAL BOOL
do_sysctl()
{
	return (FALSE);
}
#endif

#ifdef	HAVE_GETHOSTNAME

/*
 * Don't care for systems that implement a similar functionality in
 * sysinfo(2) or uname(2). This function is only called if
 * sysinfo(2) or uname(2) do not exists.
 */
LOCAL void
do_gethostname()
{
	char	nbuf[NAMEMAX];

	if (gethostname(nbuf, sizeof (nbuf)) < 0)
		return;

	define_var("MAKE_HOST", nbuf);
}
#else

/*
 * Dummy for platforms that don't implement gethostname(2).
 */
LOCAL void
do_gethostname()
{
}
#endif

/*
 * Try to retrieve some information from CPP definitions.
 */
LOCAL void
do_defs()
{
#ifdef	IS_MSDOS
	define_var("MAKE_OS", "msdos");
#define	FOUND_OS
#endif
#ifdef	IS_TOS
	define_var("MAKE_OS", "tos");
#define	FOUND_OS
#endif
#ifdef	IS_MAC
	define_var("MAKE_OS", "macos");
#define	FOUND_OS
#endif
#ifdef	__NeXT__
	define_var("MAKE_OS", "nextstep");
#define	FOUND_OS
	define_var("MAKE_OSREL", "3.0");
	define_var("MAKE_OSVERSION", "1");
#ifdef	__ARCHITECTURE__
	define_var("MAKE_MACH", __ARCHITECTURE__);
#define	FOUND_MACH
#endif
#endif	/* __NeXT__ */
#ifdef	__MINGW32__
	define_var("MAKE_OS", "mingw32_nt");
#define	FOUND_OS
#endif

/*
 * We need MAKE_OS to allow compilation with the Schily Makefile System
 */
#if !defined(FOUND_OS)
	define_var("MAKE_OS", "unknown");
#define	FOUND_OS
#endif

#if !defined(FOUND_MACH) && defined(__mc68010)
	define_var("MAKE_MACH", "mc68010");
#define	FOUND_MACH
#endif
#if !defined(FOUND_MACH) && defined(__mc68000)
	define_var("MAKE_MACH", "mc68000");
#define	FOUND_MACH
#endif
#if !defined(FOUND_MACH) && defined(__i386)
	define_var("MAKE_MACH", "i386");
#define	FOUND_MACH
#endif
#if !defined(FOUND_MACH) && defined(__sparc)
	define_var("MAKE_MACH", "sparc");
#define	FOUND_MACH
#endif
}

/*
 * Do some additional heuristic for systems that are already known
 * but may need some more macro definitions for completion.
 */
LOCAL void
do_archheuristics()
{
	list_t	*l;
	char	*name;
 
	/*
	 * Try to define global processor architecture
	 */
	l = objlist("MAKE_ARCH");
	if (l == NULL) {
		l = objlist("MAKE_MACH");
		if (l != NULL) {
			name = l->l_obj->o_name;
			if (strstr(name, "sun3"))
				define_var("MAKE_ARCH", "mc68020");
			if (strstr(name, "sun4"))
				define_var("MAKE_ARCH", "sparc");
			if (strstr(name, "i86pc"))
				define_var("MAKE_ARCH", "i386");
		}
	}

	/*
	 * Try to define global machine architecture
	 */
	l = objlist("MAKE_MACH");
	if (l != NULL) {
		name = l->l_obj->o_name;

		if (strstr(name, "sun3"))
			define_var("MAKE_M_ARCH", "sun3");
		if (strstr(name, "sun4"))
			define_var("MAKE_M_ARCH", "sun4");
		if (strstr(name, "i86pc"))
			define_var("MAKE_M_ARCH", "i86pc");
	}

	l = objlist("MAKE_OS");
	if (l != NULL &&
/*				streql(l->l_obj->o_name, "SunOS")) {*/
				streql(l->l_obj->o_name, "sunos")) {
		l = objlist("MAKE_OSREL");
		if (l != NULL && l->l_obj->o_name[0] >= '5')
/*			define_lvar("MAKE_OSDEFS", "-D__SOL2 -D__SVR4");*/
			define_var("MAKE_OSDEFS", "-D__SVR4");
	}
}

/*
 * Alle Namen in Kleinbuchstaben wandeln,
 * '/' in '-' wandeln.
 * '\\' in '-' wandeln.
 * ' ' in '-' wandeln.
 */
#include <ctype.h>

LOCAL void
archcvt(p)
	register char	*p;
{
	register Uchar	c;

	while ((c = (Uchar)*p) != '\0') {
		if (c == '/')
			*p = '-';
		if (c == '\\')
			*p = '-';
		if (c == ' ')
			*p = '-';
		if (isupper(c))
			*p = tolower(c);
		p++;
	}
}

#if defined(HAVE_SYS_SYSTEMINFO_H) || \
	(defined(HAVE_SYS_SYSCTL_H) && defined(HW_MODEL))	/* See do_sysctl() */
LOCAL void
unblank(p)
	register char	*p;
{
	register char	c;

	while ((c = *p) != '\0') {
		if (c == ' ')
			*p = '-';
		p++;
	}
}
#endif
