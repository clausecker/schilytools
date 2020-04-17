/* @(#)archconf.cc	1.2 20/04/04 Copyright 1996-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)archconf.cc	1.2 20/04/04 Copyright 1996-2020 J. Schilling";
#endif
/*
 *	Make program
 *	Architecture autoconfiguration support
 *
 *	Copyright (c) 1996-2020 by J. Schilling
 */
/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/ctype.h>

#include <schily/hostname.h>
#include <schily/utsname.h>
#include <schily/systeminfo.h>

#include <schily/errno.h>

#define	NAMEMAX		4096	/* Max size of a name POSIX linelen */
#define	Uchar		unsigned char
#define	streql(a, b)	(strcmp((a), (b)) == 0)

#ifdef	DO_ARCHCONF
extern void define_var(const char *name, const char *value);
extern char *get_var(const char *name);

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
#endif

#ifdef __HAIKU__
#include <OS.h>
#endif

#include <mksh/misc.h>
#include <mk/defs.h>

#ifdef	NO_SYSINFO
#ifdef	HAVE_SYS_SYSTEMINFO_H
#undef	HAVE_SYS_SYSTEMINFO_H
#endif
#endif

EXPORT	void	setup_arch(void);
LOCAL	BOOL	do_uname(void);
LOCAL	BOOL	do_sysinfo(void);
LOCAL	BOOL	do_sysctl(void);
LOCAL	BOOL	do_haiku(void);
LOCAL	void	do_gethostname(void);
LOCAL	void	do_defs(void);
LOCAL	void	do_archheuristics(void);
LOCAL	void	archcvt(char *);
#if defined(HAVE_SYS_SYSTEMINFO_H) || \
	(defined(HAVE_SYS_SYSCTL_H) && defined(HW_MODEL)) /* See do_sysctl() */
LOCAL	void	unblank(char *);
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
	do_haiku();

	do_archheuristics();
}

/*
 * NeXT Step has sys/utsname but not uname()
 */
#if	defined(HAVE_UNAME) || defined(__MINGW32__) || defined(_MSC_VER)
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
		warning(gettext("Cannot get host arch (uname): %s"),
		    errmsg(errno));
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
#else
#ifdef	HAVE_UTSNAME_ARCH				/* OpenVMS */
	archcvt(un.arch);
	define_var("MAKE_ARCH", un.arch);		/* uname -p */
#endif
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
#if 0
		archcvt(nbuf);
#endif
		define_var("MAKE_BRAND", nbuf);
	}
#endif

#ifdef  SI_HW_SERIAL
	if (sysinfo(SI_HW_SERIAL, nbuf, sizeof (nbuf)) >= 0) {
#if 0
		archcvt(nbuf);
#endif
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
/*
 * See #ifdef statement below in unblank()
 */
LOCAL BOOL
do_sysctl()
{
#if	defined(HW_MODEL) || defined(HW_MACHINE_ARCH)
	const char	*name;
	char	nbuf[NAMEMAX];
	size_t	len;
	int	mib[2];
#endif

#if	defined(HW_MODEL)
	name = get_var("MAKE_MODEL");
	if (name == NULL) {
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
	name = get_var("MAKE_ARCH");
	if (name == NULL) {
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
			cpu_type_t		cputype = 0;
			NXArchInfo const	*ai;

			name = NULL;
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


#ifdef __HAIKU__
LOCAL BOOL
do_haiku()
{
	char			*archname = "unknown";
	cpu_topology_node_info	root;
	uint32			count = 1;
	status_t		error = get_cpu_topology_info(&root, &count);

	if (error == B_OK && count >= 1) {
		switch (root.data.root.platform) {

		case B_CPU_x86:
			archname = "x86";
			break;

		case B_CPU_x86_64:
			archname = "x86_64";
			break;

		case B_CPU_PPC:
			archname = "ppc";
			break;

		case B_CPU_PPC_64:
			archname = "ppc64";
			break;

		case B_CPU_M68K:
			archname = "m68k";
			break;

		case B_CPU_ARM:
			archname = "arm";
			break;

		case B_CPU_ARM_64:
			archname = "arm64";
			break;

		case B_CPU_ALPHA:
			archname = "alpha";
			break;

		case B_CPU_MIPS:
			archname = "mips";
			break;

		case B_CPU_SH:
			archname = "sh";
			break;

		default:
			archname = "other";
			break;
		}
	}

	define_var("MAKE_ARCH", archname);
	return (TRUE);
}
#else
/*
 * Dummy for platforms that don't implement Haiku get_cpu_topology_info().
 */
LOCAL BOOL
do_haiku()
{
	return (FALSE);
}
#endif

#ifdef	HAVE_GETHOSTNAME

/*
 * Don't care for systems that implement a similar functionality in
 * sysinfo(2) or uname(2). This function is only called if
 * sysinfo(2) or uname(2) do not exist.
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
	char	*name;

	/*
	 * Try to define global processor architecture
	 */
	name = get_var("MAKE_ARCH");		/* uname -p is an extension */
	if (name == NULL) {
		name = get_var("MAKE_MACH");	/* Check uname -m	*/
		if (name != NULL) {
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
	name = get_var("MAKE_MACH");
	if (name != NULL) {
		if (strstr(name, "sun3"))
			define_var("MAKE_M_ARCH", "sun3");
		if (strstr(name, "sun4"))
			define_var("MAKE_M_ARCH", "sun4");
		if (strstr(name, "i86pc"))
			define_var("MAKE_M_ARCH", "i86pc");
	}

	name = get_var("MAKE_OS");
	if (name != NULL) {
		if (streql(name, "os400")) {
			/*
			 * OS400 returns serial number in uname -m
			 */
			if (get_var("MAKE_HWSERIAL") == NULL) {
				name = get_var("MAKE_MACH");
				define_var("MAKE_HWSERIAL", name);
			}
			define_var("MAKE_MACH", "powerpc");
		}
		if (streql(name, "sunos")) {
			name = get_var("MAKE_OSREL");
			if (name != NULL && name[0] >= '5')
				define_var("MAKE_OSDEFS", "-D__SVR4");
		}
	}
}

/*
 * Convert all characters into lower case,
 * convert '/' into '-',
 * convert '\\' into '-',
 * convert ' ' into '-'.
 */
LOCAL void
archcvt(register char *p)
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
	(defined(HAVE_SYS_SYSCTL_H) && defined(HW_MODEL)) /* See do_sysctl() */
/*
 * Convert all spaces into '-'.
 */
LOCAL void
unblank(register char *p)
{
	register char	c;

	while ((c = *p) != '\0') {
		if (c == ' ')
			*p = '-';
		p++;
	}
}
#endif
#endif	/* DO_ARCHCONF */
