/* @(#)acl_unix.c	1.49 14/03/31 Copyright 2001-2014 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)acl_unix.c	1.49 14/03/31 Copyright 2001-2014 J. Schilling";
#endif
/*
 *	ACL get and set routines for unix like operating systems.
 *
 *	Copyright (c) 2001-2014 J. Schilling
 *
 *	There are currently two basic flavors of ACLs:
 *
 *	Flavor 1: UFS/POSIX draft
 *
 *	The Solaric UFS ACLs that have been developed between 1990 and 1994.
 *	These ACLs have been made available as extensions to NFSv2 and NFSv3.
 *	A related POSIX.1e draft has been devloped between 1995 and 1997
 *	and withdrawn in October 1997.
 *
 *	This implementation currently supports Solaris UFS ACLs and the
 *	withdrawn POSIX.1e draft that is available on Free BSD, Linux and some
 *	other platforms since aprox. year 2000, using a common text format.
 *	Thanks to Andreas Gruenbacher <ag@bestbits.at> for the first
 *	implementation that supports the withdrawn POSIX draft ACLs.
 *
 *	As True64 does not like ACL "mask" entries and this version of the
 * 	ACL code does not generate "mask" entries on True64, ACL support for
 *	True64 is currently broken. You cannot read back archives created
 *	on true64.
 *
 *	Flavor 2: NFSv4
 *
 *	The NTFS ACLs that have been standardized as NFSv4 ACLs with
 *	NFSv4 in year 2000. NFSv4 ACLs are the native ACLs on ZFS,
 *	NTFS, NFSv4 and CIFS.
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


#include <schily/mconfig.h>
#ifdef	USE_ACL

/*
 * We may support UFS ACLs on UnixWare if we have the source for
 * libsec from Solaris present.
 */
#ifdef	OWN_ACLTEXT
#if	defined(UNIXWARE) && defined(HAVE_ACL)
#	define	HAVE_SUN_ACL
#	define	HAVE_ANY_ACL
#endif
#endif
/*
 * HAVE_ANY_ACL currently includes HAVE_POSIX_ACL and HAVE_SUN_ACL.
 * This definition must be in sync with the definition in star_unix.c
 * As USE_ACL is used in star.h, we are not allowed to change the
 * value of USE_ACL before we did include star.h or we may not include
 * star.h at all.
 * HAVE_HP_ACL is currently not included in HAVE_ANY_ACL.
 */
#	ifndef	HAVE_ANY_ACL
#	undef	USE_ACL		/* Do not try to get or set ACLs */
#	endif
#endif

#include <schily/stdio.h>
#include <schily/errno.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include <schily/standard.h>
#include <schily/stdlib.h>	/* Needed for Solaris ACL code (malloc/free) */
#include <schily/unistd.h>
#include <schily/dirent.h>
#include <schily/string.h>
#include <schily/stat.h>
#include <schily/schily.h>
#include <schily/idcache.h>
#include "starsubs.h"
#include "checkerr.h"
#include "pathname.h"

#ifdef	USE_ACL

#ifdef	HAVE_SYS_ACL_H
#	include <sys/acl.h>
#endif
#ifdef	HAVE_NFSV4_ACL
#ifdef	HAVE_ACLUTILS_H
#	include <aclutils.h>
#else
extern	char	*acl_strerror	__PR((int));
extern	int	acl_type	__PR((acl_t *));
#endif
#endif

/*
 * Define some things that either are missing or defined in a different way
 * on SCO UnixWare
 */
#if	defined(UNIXWARE)
typedef struct acl	aclent_t;
#endif
#ifndef	GETACL
#define	GETACL	ACL_GET
#endif
#ifndef	SETACL
#define	SETACL	ACL_SET
#endif
#ifndef	GETACLCNT
#define	GETACLCNT	ACL_CNT
#endif
#ifndef	MIN_ACL_ENTRIES
#define	MIN_ACL_ENTRIES	NACLBASE
#endif

#define	ROOT_UID	0

extern	BOOL	nochown;
extern	BOOL	numeric;

/*
 * XXX acl_access_text/acl_default_text are a bad idea. (see xheader.c)
 * XXX Note that in 'dirmode' dir ACLs get hosed because getinfo() is
 * XXX called for the directory before the directrory content is written
 * XXX and the directory itself is archived after the dir content.
 *
 * Solaris allows a maximum of 1024 ACL entries (for both POSIX and NFSv4)
 * Win-DOS allows a maximum of ~1820 ACL entries based on internal ACL size 65k
 *
 * A POSIX draft ACL text entry is max: "group::r--:#," (12+name+id)
 * which is 12 + 32 + 12 == 56 Bytes.
 * A NFSv4 ACL text entry is max: "group::r-------------:f------:allow:#,"
 * (38+name+id) which is 38 + 32 + 12 == 82 Bytes.
 *
 * We thus need to be prepared to have 56 kB of POSIX draft ACL text and
 * to have up to 82..146 kB of NFSv4 ACL text.
 */
LOCAL	pathstore_t	acl_access_text;
LOCAL	pathstore_t	acl_default_text;
#ifdef	HAVE_NFSV4_ACL
LOCAL	pathstore_t	acl_ace_text;
#endif

/*
 * The following three functions are implemented for any ACL support
 * but with different content.
 */
EXPORT	void	opt_acl		__PR((void));
EXPORT	BOOL	get_acls	__PR((FINFO *info));
EXPORT	void	set_acls	__PR((FINFO *info));

/*
 * Functions specific to the withdrawn POSIX.1e draft
 */
#ifdef	HAVE_POSIX_ACL
LOCAL	BOOL	acl_to_info	__PR((char *name, int type, pathstore_t *acltext));
LOCAL	BOOL	acl_add_ids	__PR((char *name, pathstore_t *infopath, char *acltext));
#endif

/*
 * Functions specific to the UFS ACL implementation
 */
#ifdef	HAVE_SUN_ACL
#ifndef	HAVE_NFSV4_ACL
LOCAL	BOOL	get_ufs_acl	__PR((FINFO *info, aclent_t **aclpp, int *aclcountp));
#endif
LOCAL	char	*acl_add_ids	__PR((char *dst, char *from, char *end, int *sizep));
#endif

/*
 * The following functions exist in one single implementation that operates
 * on the data archived by star for UFS ACLs and the withdrawn POSIX.1e draft.
 */
LOCAL	char	*base_acl	__PR((mode_t mode));
LOCAL	void	acl_check_ids	__PR((char *acltext, char *infotext, BOOL is_nfsv4));


#ifdef HAVE_POSIX_ACL	/* The withdrawn POSIX.1e draft */

#define	DID_OPT_ACL
EXPORT void
opt_acl()
{
	printf(" acl-POSIX.1e-draft");
}

/*
 * Get the access control list for a file and convert it into the format
 * used by star.
 * This is the implementation specific variant to the withdrawn POSIX.1e draft.
 */
EXPORT BOOL
get_acls(info)
	register FINFO	*info;
{
	info->f_acl_access = NULL;
	info->f_acl_default = NULL;
	info->f_acl_ace = NULL;

	/*
	 * Symlinks don't have ACLs
	 */
	if (is_symlink(info))
		return (TRUE);

	if (!acl_to_info(info->f_sname, ACL_TYPE_ACCESS, &acl_access_text))
		return (FALSE);
	if (*acl_access_text.ps_path != '\0') {
		info->f_xflags |= XF_ACL_ACCESS;
		info->f_acl_access = acl_access_text.ps_path;
	}
	if (!is_dir(info))
		return (TRUE);
	if (!acl_to_info(info->f_sname, ACL_TYPE_DEFAULT, &acl_default_text))
		return (FALSE);
	if (*acl_default_text.ps_path != '\0') {
		info->f_xflags |= XF_ACL_DEFAULT;
		info->f_acl_default = acl_default_text.ps_path;
	}
	return (TRUE);
}

/*
 * This is specific to the withdrawn POSIX.1e draft.
 */
LOCAL BOOL
acl_to_info(name, type, acltext)
	char		*name;
	int		type;
	pathstore_t	*acltext;
{
	acl_t	acl;
	char	*text, *c;
	int entries = 1;

	if (acltext->ps_size == 0)
		init_pspace(PS_EXIT, acltext);
	acltext->ps_path[0] = '\0';
	if ((acl = acl_get_file(name, type)) == NULL) {
		register int err = geterrno();
#ifdef	ENOTSUP
		/*
		 * This FS does not support ACLs.
		 */
		if (err == ENOTSUP)
			return (TRUE);
#endif
#ifdef	ENOSYS
		if (err == ENOSYS)
			return (TRUE);
#endif
		if (!errhidden(E_GETACL, name)) {
			if (!errwarnonly(E_GETACL, name))
				xstats.s_getaclerrs++;
			errmsg("Cannot get %sACL for '%s'.\n",
				type == ACL_TYPE_DEFAULT ? "default ":"", name);
			(void) errabort(E_GETACL, name, TRUE);
		}
		return (FALSE);
	}
	seterrno(0);
	text = acl_to_text(acl, NULL);
	acl_free(acl);
	if (text == NULL) {
		if (geterrno() == 0)
			seterrno(EX_BAD);
		if (!errhidden(E_BADACL, name)) {
			if (!errwarnonly(E_BADACL, name))
				xstats.s_badacl++;
			errmsg(
			    "Cannot convert %sACL entries to text for '%s'.\n",
				type == ACL_TYPE_DEFAULT ? "default ":"", name);
			(void) errabort(E_BADACL, name, TRUE);
		}
		return (FALSE);
	}

	/* remove trailing newlines */
	c = strrchr(text, '\0');
	while (c > text && *(c-1) == '\n')
		*(--c) = '\0';

	/* remove comment fields */
	c = text;
	while ((c = strchr(c, '#')) != NULL) {
		char *d = c, *e;

		while (c > text && strchr(" \t\r", *(c-1)))
			c--;
		if (c == d) {
			/* No whitespace before '#': assume it's no comment. */
			c++;
			continue;
		}
		while (*d && *d != '\n')
			d++;
		e = c;
		while ((*e++ = *d++) != '\0')
			;
	}

	/* count fields */
	for (c = text; *c != '\0'; c++) {
		if (*c == '\n') {
			*c = ',';
			entries++;
		}
	}
	if ((entries > 3) || /* > 4 on Solaris? */
	    (type == ACL_TYPE_DEFAULT && entries >= 3)) {
		if ((entries * 56) > acltext->ps_size)
			grow_pspace(PS_EXIT, acltext, entries * 56 + 2);
		if (acltext->ps_size < 2)
			return (FALSE);
		if (!acl_add_ids(name, acltext, text)) {
			acl_free((acl_t)text);
			return (FALSE);
		}
	}
	/*
	 * XXX True64 prints a compile time warning when we use
	 * XXX acl_free(text) but it is standard...
	 * XXX we need to check whether we really have to call
	 * XXX free() instead of acl_free() if we are on True64.
	 * XXX Cast the string to acl_t to supress the warning.
	 */
	acl_free((acl_t)text);
	return (TRUE);
}

/*
 * This is specific to the withdrawn POSIX.1e draft.
 */
LOCAL BOOL
acl_add_ids(name, infopath, acltext)
	char		*name;
	pathstore_t	*infopath;
	char		*acltext;
{
	int	size = infopath->ps_size - 2;
	char	*infotext = infopath->ps_path;
	ssize_t	len;
	char	*token;
	uid_t	uid;
	gid_t	gid;

	/*
	 * Add final nul to guarantee that the string is nul terminated.
	 * infopath->ps_size is granted to be at least 2 when we come here.
	 */
	infotext[size] = '\0';

	token = strtok(acltext, ",\n\r");
	while (token) {
		strlcpy(infotext, token, size);
		len = strlen(token);
		infotext += len;
		size -= len;
		if (size < 0)
			break;

		if (strncmp(token, "user:", 5) == 0 &&
		    strchr(":,\n\r", token[5]) == NULL) {
			char *username = &token[5], *c = username+1;

			while (strchr(":,\n\r", *c) == NULL)
				c++;
			*c = '\0';
			/* check for all-numeric user name */
			while (c > username && isdigit(*(c-1)))
				c--;
			if (c > username &&
			    ic_uidname(username, c-username, &uid)) {
				len = js_snprintf(infotext, size,
					":%lld", (Llong)uid);
				infotext += len;
				size -= len;
			}
		} else if (strncmp(token, "group:", 6) == 0 &&
			    strchr(":,\n\r", token[6]) == NULL) {
			char *groupname = &token[6], *c = groupname+1;

			while (strchr(":,\n\r", *c) == NULL)
				c++;
			*c = '\0';
			/* check for all-numeric group name */
			while (c > groupname && isdigit(*(c-1)))
				c--;
			if (c > groupname &&
			    ic_gidname(groupname, c-groupname, &gid)) {
				len = js_snprintf(infotext, size,
					":%lld", (Llong)gid);
				infotext += len;
				size -= len;
			}
		}
		if (size > 0) {
			*infotext++ = ',';
			*infotext = '\0';	/* Always null terminate */
			size--;
		}

		token = strtok(NULL, ",\n\r");
	}
	if (size >= 0) {
		*(--infotext) = '\0';		/* Remove tailing ',' */
	} else {
		if (!errhidden(E_BADACL, name)) {
			if (!errwarnonly(E_BADACL, name))
				xstats.s_badacl++;
			errmsgno(EX_BAD,
			"Cannot convert ACL entries (string too long) for '%s'.\n",
						name);
			(void) errabort(E_BADACL, name, TRUE);
		}
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Use ACL info from archive to set access control list for the file if needed.
 * This is the implementation specific variant to the withdrawn POSIX.1e draft.
 */
EXPORT void
set_acls(info)
	register FINFO	*info;
{
	char		acltext[PATH_MAX+1];
	pathstore_t	aclps;
	acl_t		acl;

	aclps.ps_path = acltext;
	aclps.ps_size = PATH_MAX;
	if (info->f_xflags & XF_ACL_ACCESS) {
		ssize_t	len = strlen(info->f_acl_access) + 2;

		if (len > aclps.ps_size) {
			aclps.ps_path = NULL;
			aclps.ps_size = 0;
			grow_pspace(PS_EXIT, &aclps, len);
			if (aclps.ps_size <= len) {
				free_pspace(&aclps);
				return;
			}
		}
		acl_check_ids(aclps.ps_path, info->f_acl_access, FALSE);
	} else {
		/*
		 * We may need to delete an inherited ACL.
		 */
		strcpy(aclps.ps_path,  base_acl(info->f_mode));
	}
	if ((acl = acl_from_text(aclps.ps_path)) == NULL) {
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsg("Cannot convert ACL '%s' to internal format for '%s'.\n",
					aclps.ps_path, info->f_name);
			(void) errabort(E_BADACL, info->f_name, TRUE);
		}
	} else {
		if (acl_set_file(info->f_name, ACL_TYPE_ACCESS, acl) < 0) {
			/*
			 * XXX What should we do if errno is ENOTSUP/ENOSYS?
			 */
			if (!errhidden(E_SETACL, info->f_name)) {
				if (!errwarnonly(E_SETACL, info->f_name))
					xstats.s_setacl++;
				errmsg("Cannot set ACL '%s' for '%s'.\n",
					aclps.ps_path, info->f_name);
				(void) errabort(E_SETACL, info->f_name, TRUE);
			}

#ifdef	__needed__
			/* Fall back to chmod */
			/* XXX chmod has already been done! */
			chmod(info->f_name, info->f_mode);
#endif
		}
		acl_free(acl);
	}

	/*
	 * Only directories can have Default ACLs
	 */
	if (!is_dir(info)) {
		if (aclps.ps_path != acltext)
			free_pspace(&aclps);
		return;
	}

	if (info->f_xflags & XF_ACL_DEFAULT) {
		ssize_t	len = strlen(info->f_acl_access) + 2;

		if (len > aclps.ps_size) {
			if (aclps.ps_path == acltext) {
				aclps.ps_path = NULL;
				aclps.ps_size = 0;
			}
			grow_pspace(PS_EXIT, &aclps, len);
			if (aclps.ps_size <= len) {
				free_pspace(&aclps);
				return;
			}
		}
		acl_check_ids(aclps.ps_path, info->f_acl_default, FALSE);
	} else {
		aclps.ps_path[0] = '\0';
#ifdef	HAVE_ACL_DELETE_DEF_FILE
		/*
		 * FreeBSD does not like acl_from_text("")
		 */
		if (acl_delete_def_file(info->f_name) < 0) {
			/*
			 * XXX What should we do if errno is ENOTSUP/ENOSYS?
			 */
			if (!errhidden(E_SETACL, info->f_name)) {
				if (!errwarnonly(E_SETACL, info->f_name))
					xstats.s_setacl++;
				errmsg("Cannot remove default ACL from '%s'.\n",
								info->f_name);
				(void) errabort(E_SETACL, info->f_name, TRUE);
			}
		}
		if (aclps.ps_path != acltext)
			free_pspace(&aclps);
		return;
#endif
	}
	if ((acl = acl_from_text(aclps.ps_path)) == NULL) {
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsg("Cannot convert default ACL '%s' to internal format for '%s'.\n",
					aclps.ps_path, info->f_name);
			(void) errabort(E_BADACL, info->f_name, TRUE);
		}
	} else {
		if (acl_set_file(info->f_name, ACL_TYPE_DEFAULT, acl) < 0) {
			/*
			 * XXX What should we do if errno is ENOTSUP/ENOSYS?
			 */
			if (!errhidden(E_SETACL, info->f_name)) {
				if (!errwarnonly(E_SETACL, info->f_name))
					xstats.s_setacl++;
				errmsg("Cannot set default ACL '%s' for '%s'.\n",
					aclps.ps_path, info->f_name);
				(void) errabort(E_SETACL, info->f_name, TRUE);
			}
		}
		acl_free(acl);
	}
	if (aclps.ps_path != acltext)
		free_pspace(&aclps);
}

#endif  /* HAVE_POSIX_ACL The withdrawn POSIX.1e draft */

#ifdef	HAVE_SUN_ACL	/* The UFS ACL implementation */

#ifndef	ACL_IS_DIR	/* We did not include sys/acl_impl.h */
#define	ACE_T	1
#endif
#ifndef	ACL_SID_FMT	/* Solaris 10 has no support for this */
#define	ACL_SID_FMT	0
#endif

#define	DID_OPT_ACL
EXPORT void
opt_acl()
{
	printf(" acl-POSIX.1e-draft");
#ifdef	HAVE_NFSV4_ACL
	printf(" acl-NFSv4");
#endif
}

/*
 * Get the access control list for a file and convert it into the format
 * used by star.
 * This is the implementation specific variant to Solaris UFS ACLs
 * and Solaris NFSV4 ACLs.
 */
EXPORT BOOL
get_acls(info)
	register FINFO	*info;
{
#ifdef	HAVE_NFSV4_ACL
		int		acltype;
		acl_t		*aclp;
#else
		int		aclcount;
		aclent_t	*aclp;
#endif
	register char		*acltext;
	register char		*ap;
	register char		*dp;
	register char		*cp;
	register char		*ep;
		int		asize;
		int		dsize;

	info->f_acl_access = NULL;
	info->f_acl_default = NULL;
	info->f_acl_ace = NULL;

	/*
	 * Symlinks don't have ACLs
	 */
	if (is_symlink(info))
		return (TRUE);

#ifdef	HAVE_NFSV4_ACL
	if (acl_get(info->f_name, ACL_NO_TRIVIAL, &aclp) < 0) {
		if (!errhidden(E_GETACL, info->f_name)) {
			if (!errwarnonly(E_GETACL, info->f_name))
				xstats.s_getaclerrs++;
			errmsg("Cannot get ACL entries for '%s'.\n",
							info->f_name);
			(void) errabort(E_GETACL, info->f_name, TRUE);
		}
		return (FALSE);
	}
	if (aclp == NULL)		/* Only trivial ACL entries */
		return (TRUE);

	acltype = acl_type(aclp);
	seterrno(0);
	acltext = acl_totext(aclp, acltype == ACE_T?
			ACL_COMPACT_FMT | ACL_APPEND_ID | ACL_SID_FMT : 0);
	acl_free(aclp);			/* acl_t * needs acl_free() */
#else
	if (!get_ufs_acl(info, &aclp, &aclcount))
		return (FALSE);
	if (aclp == NULL)		/* Only trivial ACL entries */
		return (TRUE);

	seterrno(0);
	acltext = acltotext(aclp, aclcount);
	free(aclp);			/* aclent_t * needs free() */
#endif

	if (acltext == NULL) {
		if (geterrno() == 0)
			seterrno(EX_BAD);
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsg("Cannot convert ACL entries to text for '%s'.\n",
								info->f_name);
			(void) errabort(E_BADACL, info->f_name, TRUE);
		}
		return (FALSE);
	}
#ifdef	ACL_DEBUG
	error("acltext '%s'\n", acltext);
#endif

#ifdef	HAVE_NFSV4_ACL
	if (acltype == ACE_T) {
		if (strcpy_pspace(PS_EXIT, &acl_ace_text, acltext) < 0) {
			free(acltext);
			return (FALSE);
		}
		free(acltext);
		info->f_xflags |= XF_ACL_ACE;
		info->f_acl_ace = acl_ace_text.ps_path;
		return (TRUE);
	}
#endif

	/*
	 * Estimate the ACl text size after adding IDs.
	 * The minimal UFS ACL entry length is 10 bytes.
	 * If we add the numerical ID string, this would become 22 bytes.
	 */
	asize = strlen(acltext);
	asize = (asize+9)/10 * 22 + 2;

	grow_pspace(PS_EXIT, &acl_access_text, asize);
	grow_pspace(PS_EXIT, &acl_default_text, asize);
	ap = acl_access_text.ps_path;
	dp = acl_default_text.ps_path;
	asize = acl_access_text.ps_size;
	dsize = acl_default_text.ps_size;

	for (cp = acltext; *cp; cp = ep) {
		if (*cp == ',')
			cp++;
		ep = strchr(cp, ',');
		if (ep == NULL)
			ep = strchr(cp, '\0');

		if (*cp == 'd' && strncmp(cp, "default", 7) == 0) {
			cp += 7;
			dp = acl_add_ids(dp, cp, ep, &dsize);
			if (dp == NULL)
				break;
		} else {
			ap = acl_add_ids(ap, cp, ep, &asize);
			if (ap == NULL)
				break;
		}
	}
	free(acltext);

	if (ap == NULL || dp == NULL) {
		acl_access_text.ps_path[0] = '\0';
		acl_default_text.ps_path[0] = '\0';
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsgno(EX_BAD, "Cannot convert ACL entries (string too long) for '%s'.\n",
							info->f_name);
			(void) errabort(E_BADACL, info->f_name, TRUE);
		}
		return (FALSE);
	}

	if (ap > acl_access_text.ps_path && ap[-1] == ',')
		--ap;
	*ap = '\0';
	if (dp > acl_default_text.ps_path && dp[-1] == ',')
		--dp;
	*dp = '\0';

	if (*acl_access_text.ps_path != '\0') {
		info->f_xflags |= XF_ACL_ACCESS;
		info->f_acl_access = acl_access_text.ps_path;
	}
	if (*acl_default_text.ps_path != '\0') {
		info->f_xflags |= XF_ACL_DEFAULT;
		info->f_acl_default = acl_default_text.ps_path;
	}

#ifdef	ACL_DEBUG
error("access:  '%s'\n", acl_access_text.ps_path);
error("default: '%s'\n", acl_default_text.ps_path);
#endif

	return (TRUE);
}

#ifndef	HAVE_NFSV4_ACL
LOCAL BOOL
get_ufs_acl(info, aclpp, aclcountp)
	register FINFO	*info;
		aclent_t	**aclpp;
		int		*aclcountp;
{
		int		aclcount;
		aclent_t	*aclp;

	*aclpp = NULL;
#ifdef	HAVE_ST_ACLCNT
	aclcount = info->f_aclcnt;	/* UnixWare */
#else
	if ((aclcount = acl(info->f_sname, GETACLCNT, 0, NULL)) < 0) {
#ifdef	ENOSYS
		if (geterrno() == ENOSYS)
			return (TRUE);
#endif
		if (!errhidden(E_GETACL, info->f_name)) {
			if (!errwarnonly(E_GETACL, info->f_name))
				xstats.s_getaclerrs++;
			errmsg("Cannot get ACL count for '%s'.\n",
							info->f_name);
			(void) errabort(E_GETACL, info->f_name, TRUE);
		}
		return (FALSE);
	}
#endif
#ifdef	ACL_DEBUG
	error("'%s' acl count %d\n", info->f_name, aclcount);
#endif
	if (aclcount <= MIN_ACL_ENTRIES) {
		/*
		 * This file has only the traditional UNIX access list.
		 * This case includes a filesystem that does not support ACLs
		 * like the tmpfs.
		 */
		return (TRUE);
	}
	if ((aclp = (aclent_t *)malloc(sizeof (aclent_t) * aclcount)) == NULL) {
		if (!errhidden(E_GETACL, info->f_name)) {
			if (!errwarnonly(E_GETACL, info->f_name))
				xstats.s_getaclerrs++;
			errmsg("Cannot malloc ACL buffer for '%s'.\n",
							info->f_name);
			(void) errabort(E_GETACL, info->f_name, TRUE);
		}
		return (FALSE);
	}
	if (acl(info->f_sname, GETACL, aclcount, aclp) < 0) {
		if (!errhidden(E_GETACL, info->f_name)) {
			if (!errwarnonly(E_GETACL, info->f_name))
				xstats.s_getaclerrs++;
			errmsg("Cannot get ACL entries for '%s'.\n",
							info->f_name);
			(void) errabort(E_GETACL, info->f_name, TRUE);
		}
		return (FALSE);
	}
	*aclpp = aclp;
	*aclcountp = aclcount;
	return (TRUE);
}
#endif	/* HAVE_NFSV4_ACL */

/*
 * Convert Solaris UFS ACL text into POSIX draft ACL text and add numerical
 * user/group ids.
 * This is specific to Solaris UFS ACLs.
 *
 * Solaris uses only one colon in the ACL text format for "other" and "mask".
 * Solaris ACL text is:	"user::rwx,group::rwx,mask:rwx,other:rwx"
 * while POSIX text is:	"user::rwx,group::rwx,mask::rwx,other::rwx"
 */
LOCAL char *
acl_add_ids(dst, from, end, sizep)
	char	*dst;
	char	*from;
	char	*end;
	int	*sizep;
{
	register char	*cp = from;
	register char	*ep = end;
	register char	*np = dst;
	register int	size = *sizep;
	register int	amt;
		uid_t	uid;
		gid_t	gid;

	if (cp[0] == 'u' &&
	    strncmp(cp, "user:", 5) == 0) {
		if (size <= (ep - cp +1)) {
			*sizep = 0;
			return (NULL);
		}
		size -= ep - cp +1;
		strncpy(np, cp, ep - cp +1);
		np += ep - cp + 1;

		cp += 5;
		ep = strchr(cp, ':');
		if (ep)
			*ep = '\0';
		if (*cp) {
			if (ic_uidname(cp, 1000, &uid)) {
				if (np[-1] == ',') {
					--np;
					size++;
				}
				amt = js_snprintf(np, size,
					":%lld,", (Llong)uid);
				np += amt;
				size -= amt;
			}
		}
		if (ep)
			*ep = ':';

	} else if (cp[0] == 'g' &&
	    strncmp(cp, "group:", 6) == 0) {
		if (size <= (ep - cp +1)) {
			*sizep = 0;
			return (NULL);
		}
		size -= ep - cp +1;
		strncpy(np, cp, ep - cp + 1);
		np += ep - cp + 1;

		cp += 6;
		ep = strchr(cp, ':');
		if (ep)
			*ep = '\0';
		if (*cp) {
			if (ic_gidname(cp, 1000, &gid)) {
				if (np[-1] == ',') {
					--np;
					size++;
				}
				amt = js_snprintf(np, size,
					":%lld,", (Llong)gid);
				np += amt;
				size -= amt;
			}
		}
		if (ep)
			*ep = ':';

	} else if (cp[0] == 'm' &&
	    strncmp(cp, "mask:", 5) == 0) {
		cp += 4;
		if (size < 5) {
			*sizep = 0;
			return (NULL);
		}
		/*
		 * Add one additional ':' to the string for POSIX compliance.
		 */
		strcpy(np, "mask:");
		np += 5;
		if (size <= (ep - cp +1)) {
			*sizep = 0;
			return (NULL);
		}
		strncpy(np, cp, ep - cp + 1);
		np += ep - cp + 1;

	} else if (cp[0] == 'o' &&
	    strncmp(cp, "other:", 6) == 0) {
		cp += 5;
		if (size < 6) {
			*sizep = 0;
			return (NULL);
		}
		/*
		 * Add one additional ':' to the string for POSIX compliance.
		 */
		strcpy(np, "other:");
		np += 6;
		if (size <= (ep - cp +1)) {
			*sizep = 0;
			return (NULL);
		}
		strncpy(np, cp, ep - cp + 1);
		np += ep - cp + 1;
	}
	if (size <= 0) {
		size = 0;
		np = NULL;
	}
	*sizep = size;
	return (np);
}

/*
 * Convert ACL info from archive into Sun's format and set access control list
 * for the file if needed.
 * This is the implementation specific variant to Solaris UFS ACLs
 * and Solaris NFSV4 ACLs.
 */
EXPORT void
set_acls(info)
	register FINFO	*info;
{
#ifdef	HAVE_NFSV4_ACL
	int		err;
	acl_t		*aclp;
#else
	int		aclcount;
	aclent_t	*aclp;
#endif
	char		aclbuf[8192];
	pathstore_t	aclps;
	BOOL		no_acl = FALSE;

	aclps.ps_path = aclbuf;
	aclps.ps_size = sizeof (aclbuf) -2;

	aclbuf[0] = '\0';
	if (info->f_xflags & XF_ACL_ACCESS) {
		ssize_t	len = strlen(info->f_acl_access) + 2;

		if (len > aclps.ps_size) {
			aclps.ps_path = NULL;
			aclps.ps_size = 0;
			grow_pspace(PS_EXIT, &aclps, len);
			if (aclps.ps_size <= len) {
				free_pspace(&aclps);
				return;
			}
			aclps.ps_path[0] = '\0';
		}
		acl_check_ids(aclps.ps_path, info->f_acl_access, FALSE);
	}
	if (info->f_xflags & XF_ACL_DEFAULT) {
			char		acltext[PATH_MAX+1];
			pathstore_t	acldps;
		register char	*cp;
		register char	*dp;
		register char	*ep;
			ssize_t	dlen;

		/* count fields */
		for (dlen = 1, cp = info->f_acl_default; *cp != '\0'; cp++) {
			if (*cp == ',')
				dlen++;
		}
		dlen = dlen * 7;			/* strlen("default") */
		dlen += cp - info->f_acl_default;	/* +strlen(acldeflt) */
		dlen += 2;				/* Nul */
		acldps.ps_path = acltext;
		acldps.ps_size = sizeof (acltext);
		if (dlen > acldps.ps_size) {
			acldps.ps_path = NULL;
			acldps.ps_size = 0;
			grow_pspace(PS_EXIT, &acldps, dlen);
			if (acldps.ps_size <= dlen) {
				free_pspace(&acldps);
				return;
			}
		}
		acl_check_ids(acldps.ps_path, info->f_acl_default, FALSE);

		aclps.ps_tail = strlen(aclps.ps_path);
		if ((aclps.ps_tail + dlen) > aclps.ps_size) {
			char	*op = aclps.ps_path;

			if (aclps.ps_path == acltext) {
				aclps.ps_path = NULL;
				aclps.ps_size = 0;
			}
			grow_pspace(PS_EXIT, &aclps, aclps.ps_tail + dlen);
			if (aclps.ps_size <= dlen) {
				free_pspace(&aclps);
				return;
			}
			if (op == aclbuf)
				strcpy(aclps.ps_path, aclbuf);
		}
		dp = aclps.ps_path + aclps.ps_tail;
		if (dp > aclps.ps_path)
			*dp++ = ',';
		for (cp = acldps.ps_path; *cp; cp = ep) {
			/*
			 * XXX Eigentlich muesste man hier bei den Eintraegen
			 * XXX "mask" und "other" jeweils ein ':' beseitigten
			 * XXX aber es sieht so aus, als ob es bei Solaris 9
			 * XXX auch funktionert wenn man das nicht tut.
			 * XXX Nach Solaris 7 "libsec" Source kann es nicht
			 * XXX mehr funktionieren wenn man das ':' beseitigt.
			 * XXX Moeglicherweise ist das der Grund warum
			 * XXX Solaris immer Probleme mit den ACLs hatte.
			 */
			if (*cp == ',')
				cp++;
			ep = strchr(cp, ',');
			if (ep == NULL)
				ep = strchr(cp, '\0');
			strcpy(dp, "default");
			dp += 7;
			strncpy(dp, cp, ep - cp + 1);
			dp += ep - cp + 1;
		}
	}
	if (info->f_xflags & XF_ACL_ACE) {
		ssize_t	len = strlen(info->f_acl_ace) + 2;

		if (aclps.ps_path[0] != '\0' && !errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsgno(EX_BAD,
			"Both POSIX draft ACL '%s' and NFSv4 ACL %s present for '%s'.\n",
				aclps.ps_path,
				info->f_acl_access,
				info->f_name);
			(void) errabort(E_BADACL, info->f_name, TRUE);
		}
		if (aclps.ps_path != aclbuf)
			free_pspace(&aclps);
#ifdef	HAVE_NFSV4_ACL
		aclps.ps_path = aclbuf;
		aclps.ps_size = sizeof (aclbuf) -2;
		if (len > aclps.ps_size) {
			aclps.ps_path = NULL;
			aclps.ps_size = 0;
			grow_pspace(PS_EXIT, &aclps, len);
			if (aclps.ps_size <= len) {
				free_pspace(&aclps);
				return;
			}
		}
		acl_check_ids(aclps.ps_path, info->f_acl_ace, TRUE);
#else
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsgno(EX_BAD,
			"Cannot set NFSv4 ACLs for '%s' on this platform.\n",
				info->f_name);
			(void) errabort(E_BADACL, info->f_name, TRUE);
		}
		return;
#endif
	}
#ifdef	ACL_DEBUG
	error("aclbuf: '%s'\n", aclps.ps_path);
#endif

	if (aclps.ps_path[0] == '\0') {
		/*
		 * We may need to delete an inherited ACL.
		 */
		strcpy(aclps.ps_path, base_acl(info->f_mode));
		no_acl = TRUE;
	} else if (streql(aclps.ps_path, base_acl(info->f_mode))) {
		no_acl = TRUE;
	}

#ifdef	HAVE_NFSV4_ACL
	if ((err = acl_fromtext(aclps.ps_path, &aclp)) != 0) {
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsgno(EX_BAD,
			"%s. Cannot convert ACL '%s' to internal format for '%s'.\n",
				acl_strerror(err),
				aclps.ps_path, info->f_name);
			(void) errabort(E_BADACL, info->f_name, TRUE);
		}
	} else {
		if (acl_set(info->f_name, aclp) < 0) {
			BOOL	no_error = FALSE;

			if (no_acl) {
				acl_t	*xaclp;

				/*
				 * This should catch the ENOSYS case which
				 * happens e.g. if the target is a socket.
				 */
				if (acl_get(info->f_name,
					    ACL_NO_TRIVIAL, &xaclp) >= 0) {
					if (xaclp == NULL)
						no_error = TRUE;
					else
						acl_free(xaclp);
				}
			}
			if (!no_error && !errhidden(E_SETACL, info->f_name)) {
				if (!errwarnonly(E_SETACL, info->f_name))
				errmsg("Cannot set ACL '%s' for '%s'.\n",
					aclps.ps_path, info->f_name);
					xstats.s_setacl++;
				(void) errabort(E_SETACL, info->f_name, TRUE);
			}
		}
		acl_free(aclp);	/* acl_t * needs acl_free() */
	}
#else
	seterrno(0);
	if ((aclp = aclfromtext(aclps.ps_path, &aclcount)) == NULL) {
		if (geterrno() == 0)
			seterrno(EX_BAD);
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsg("Cannot convert ACL '%s' to internal format for '%s'.\n",
				aclps.ps_path, info->f_name);
			(void) errabort(E_BADACL, info->f_name, TRUE);
		}
	} else {
		if (acl(info->f_name, SETACL, aclcount, aclp) < 0) {
			BOOL	no_error = FALSE;
			/*
			 * XXX What should we do if errno is ENOSYS?
			 */
			if (no_acl) {
				int	aclcnt;

				/*
				 * This should catch the ENOSYS case which
				 * happens e.g. if the target is a socket.
				 */
				aclcnt = acl(info->f_name, GETACLCNT, 0, NULL);
				if (aclcnt <= MIN_ACL_ENTRIES)
					no_error = TRUE;
			}
			if (!no_error && !errhidden(E_SETACL, info->f_name)) {
				if (!errwarnonly(E_SETACL, info->f_name))
				errmsg("Cannot set ACL '%s' for '%s'.\n",
					aclps.ps_path, info->f_name);
					xstats.s_setacl++;
				(void) errabort(E_SETACL, info->f_name, TRUE);
			}
		}
		free(aclp);	/* aclent_t * needs free() */
	}
#endif

	if (aclps.ps_path != aclbuf)
		free_pspace(&aclps);
}

#endif	/* HAVE_SUN_ACL The UFS ACL implementation */

/*
 * Convert UNIX standard mode bits into base ACL
 */
LOCAL char *
#ifdef	PROTOTYPES
base_acl(mode_t mode)
#else
base_acl(mode)
	mode_t	mode;
#endif
{
	static char _acltxt[] = "user::***,group::***,other::***";

	_acltxt[ 6] = (mode & 0400) ? 'r' : '-';
	_acltxt[ 7] = (mode & 0200) ? 'w' : '-';
	_acltxt[ 8] = (mode & 0100) ? 'x' : '-';
	_acltxt[17] = (mode & 0040) ? 'r' : '-';
	_acltxt[18] = (mode & 0020) ? 'w' : '-';
	_acltxt[19] = (mode & 0010) ? 'x' : '-';
	_acltxt[28] = (mode & 0004) ? 'r' : '-';
	_acltxt[29] = (mode & 0002) ? 'w' : '-';
	_acltxt[30] = (mode & 0001) ? 'x' : '-';

	return (_acltxt);
}

/*
 * If we are in -numeric mode, we replace the user and group names by the
 * user and group numbers from our internal format.
 *
 * If we are in non numeric mode, we check whether a user name or group name
 * is present on our current system. It the user/group name is known, then we
 * remove the numeric value from out internal format. If the user/group name
 * is not known, then we replace the name by the numeric value.
 */
LOCAL void
acl_check_ids(acltext, infotext, is_nfsv4)
	char	*acltext;
	char	*infotext;
	BOOL	is_nfsv4;
{
	char	entry_buffer[PATH_MAX];		/* A single entry */
	char	*token = strtok(infotext, ",");

	if (!token)
		return;

	while (token) {
		char *x = strchr(token, '#');
		if (x != NULL) {
			/*
			 * Cut off any "[ \t]#" at the end of the token.
			 */
			while (x > token+1 && strchr(" \t", *(x-1)) != NULL)
				x--;
			*x = '\0';
		}
		if (strncmp(token, "user:", 5) == 0 &&
		    strchr(":,", token[5]) == NULL) {
			char *username = &token[5], *c = username;
			char *perms, *auid;
			char *flags, *type;
			uid_t	udummy;
			/* uidname does not check for NULL! */

			/* check for damaged user names with spaces */
			if (strchr(username, ':') == NULL) {
				/*
				 * Looks like a damaged user name that had
				 * spaces in it, like "Joe,User". Repair.
				 */
				char *unexpected_sep = strchr(username, '\0');

				if (strtok(NULL, ",")) {
					*unexpected_sep = ' ';
					continue;
				}
			}

			/* user name */
			if (*username != ':') {
				c++;
				while (strchr(":,", *c) == NULL)
					c++;
			}
			if (*c)
				*c++ = '\0';

			/* permissions */
			perms = c;
			while (strchr(":,", *c) == NULL)
				c++;
			if (*c)
				*c++ = '\0';

			if (is_nfsv4) {
				/* inheritance flags */
				flags = c;
				while (strchr(":,", *c) == NULL)
					c++;
				if (*c)
					*c++ = '\0';

				/* access type */
				type = c;
				while (strchr(":,", *c) == NULL)
					c++;
				if (*c)
					*c++ = '\0';
			}

			/* identifier */
			auid = c;
			while (strchr(":,", *c) == NULL)
				c++;
			if (*c)
				*c++ = '\0';

			/*
			 * XXX We use strlen(username)+1 to tell uidname not
			 * XXX to stop comparing before the end of the
			 * XXX username has been reached. Otherwise "joe" and
			 * XXX "joeuser" would be recognized as identical.
			 */
			if (*auid && (numeric ||
			    !ic_uidname(username, strlen(username)+1, &udummy)))
				username = auid;
			if (is_nfsv4) {
				js_snprintf(entry_buffer, PATH_MAX,
					"user:%s:%s:%s:%s",
					username, perms, flags, type);
			} else {
				js_snprintf(entry_buffer, PATH_MAX,
					"user:%s:%s",
					username, perms);
			}
			token = entry_buffer;

		} else if (strncmp(token, "group:", 6) == 0 &&
		    strchr(",", token[6]) == NULL) {
			char *groupname = &token[6], *c = groupname;
			char *perms, *agid;
			char *flags, *type;
			gid_t	gdummy;
			/* gidname does not check for NULL! */

			/* check for damaged group names with spaces */
			if (strchr(groupname, ':') == NULL) {
				/*
				 * Looks like a damaged group name that had
				 * spaces in it, like "Domain,Users". Repair.
				 */
				char *unexpected_sep = strchr(groupname, '\0');

				if (strtok(NULL, ",")) {
					*unexpected_sep = ' ';
					continue;
				}
			}

			/* group name */
			if (*groupname != ':') {
				c++;
				while (strchr(":,", *c) == NULL)
					c++;
			}
			if (*c)
				*c++ = '\0';

			/* permissions */
			perms = c;
			while (strchr(":,", *c) == NULL)
				c++;
			if (*c)
				*c++ = '\0';

			if (is_nfsv4) {
				/* inheritance flags */
				flags = c;
				while (strchr(":,", *c) == NULL)
					c++;
				if (*c)
					*c++ = '\0';

				/* access type */
				type = c;
				while (strchr(":,", *c) == NULL)
					c++;
				if (*c)
					*c++ = '\0';
			}

			/* identifier */
			agid = c;
			while (strchr(":,", *c) == NULL)
				c++;
			if (*c)
				*c++ = '\0';

			/*
			 * XXX We use strlen(groupname)+1 to tell gidname not
			 * XXX to stop comparing before the end of the
			 * XXX groupname has been reached. Otherwise "joe" and
			 * XXX "joeuser" would be compared as identical.
			 */
			if (*agid && (numeric ||
			    !ic_gidname(groupname, strlen(groupname)+1, &gdummy)))
				groupname = agid;
			if (is_nfsv4) {
				js_snprintf(entry_buffer, PATH_MAX,
					"group:%s:%s:%s:%s",
					groupname, perms, flags, type);
			} else {
				js_snprintf(entry_buffer, PATH_MAX,
					"group:%s:%s",
					groupname, perms);
			}
			token = entry_buffer;
		}
		if (*token != '\0') {
			strcpy(acltext, token);
			acltext += strlen(token);
			*acltext++ = ',';
		}

		token = strtok(NULL, ",");
	}
	*(--acltext) = '\0';
}

#endif  /* USE_ACL */

#ifndef	DID_OPT_ACL
EXPORT void
opt_acl()
{
}
#endif
