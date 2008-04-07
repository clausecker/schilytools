/* @(#)acl_unix.c	1.37 08/04/06 Copyright 2001-2008 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)acl_unix.c	1.37 08/04/06 Copyright 2001-2008 J. Schilling";
#endif
/*
 *	ACL get and set routines for unix like operating systems.
 *
 *	Copyright (c) 2001-2008 J. Schilling
 *
 *	This implementation currently supports POSIX.1e and Solaris ACLs.
 *	Thanks to Andreas Gruenbacher <ag@bestbits.at> for the first POSIX ACL
 *	implementation.
 *
 *	As True64 does not like ACL "mask" entries and this version of the
 * 	ACL code does not generate "mask" entries on True64, ACl support for
 *	True64 is currently broken. You cannot read back archives created
 *	on true64.
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
#ifdef	USE_ACL

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

#include <stdio.h>
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

#ifdef	USE_ACL

#ifdef	HAVE_SYS_ACL_H
#	include <sys/acl.h>
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
 */
LOCAL char acl_access_text[PATH_MAX+1];
LOCAL char acl_default_text[PATH_MAX+1];

EXPORT	void	opt_acl		__PR((void));
EXPORT	BOOL	get_acls	__PR((FINFO *info));
EXPORT	void	set_acls	__PR((FINFO *info));

#ifdef	HAVE_POSIX_ACL
LOCAL	BOOL	acl_to_info	__PR((char *name, int type, char *acltext));
LOCAL	BOOL	acl_add_ids	__PR((char *name, char *infotext, char *acltext));
#endif

#ifdef	HAVE_SUN_ACL
LOCAL	char	*acl_add_ids	__PR((char *dst, char *from, char *end, int *sizep));
#endif

LOCAL	char	*base_acl	__PR((mode_t mode));
LOCAL	void	acl_check_ids	__PR((char *acltext, char *infotext));


#ifdef HAVE_POSIX_ACL

#define	DID_OPT_ACL
EXPORT void
opt_acl()
{
	printf(" acl");
}

/*
 * Get the access control list for a file and convert it into the format
 * used by star.
 */
EXPORT BOOL
get_acls(info)
	register FINFO	*info;
{
	info->f_acl_access = NULL;
	info->f_acl_default = NULL;

	/*
	 * Symlinks don't have ACLs
	 */
	if (is_symlink(info))
		return (TRUE);

	if (!acl_to_info(info->f_name, ACL_TYPE_ACCESS, acl_access_text))
		return (FALSE);
	if (*acl_access_text != '\0') {
		info->f_xflags |= XF_ACL_ACCESS;
		info->f_acl_access = acl_access_text;
	}
	if (!is_dir(info))
		return (TRUE);
	if (!acl_to_info(info->f_name, ACL_TYPE_DEFAULT, acl_default_text))
		return (FALSE);
	if (*acl_default_text != '\0') {
		info->f_xflags |= XF_ACL_DEFAULT;
		info->f_acl_default = acl_default_text;
	}
	return (TRUE);
}

LOCAL BOOL
acl_to_info(name, type, acltext)
	char	*name;
	int	type;
	char	*acltext;
{
	acl_t	acl;
	char	*text, *c;
	int entries = 1;

	acltext[0] = '\0';
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
/*	free(text);*/
	acl_free((acl_t)text);
	return (TRUE);
}

LOCAL BOOL
acl_add_ids(name, infotext, acltext)
	char	*name;
	char	*infotext;
	char	*acltext;
{
	int	size = PATH_MAX;
	int	len;
	char	*token;
	uid_t	uid;
	gid_t	gid;

	/*
	 * Add final nul to guarantee that the string is nul terminated.
	 */
	infotext[PATH_MAX] = '\0';

	token = strtok(acltext, ",\n\r");
	while (token) {
		strncpy(infotext, token, size);
		infotext += strlen(token);
		size -= strlen(token);
		if (size < 0)
			size = 0;

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
			size--;
		}

		token = strtok(NULL, ",\n\r");
	}
	if (size >= 0) {
		*(--infotext) = '\0';
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
 */
EXPORT void
set_acls(info)
	register FINFO	*info;
{
	char	acltext[PATH_MAX+1];
	acl_t	acl;

	if (info->f_xflags & XF_ACL_ACCESS) {
		acl_check_ids(acltext, info->f_acl_access);
	} else {
		/*
		 * We may need to delete an inherited ACL.
		 */
		strcpy(acltext,  base_acl(info->f_mode));
	}
	if ((acl = acl_from_text(acltext)) == NULL) {
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsg("Cannot convert ACL '%s' to internal format for '%s'.\n",
					acltext, info->f_name);
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
					acltext, info->f_name);
				(void) errabort(E_SETACL, info->f_name, TRUE);
			}

			/* Fall back to chmod */
/* XXX chmod has already been done! */
/*			chmod(info->f_name, info->f_mode);*/
		}
		acl_free(acl);
	}

	/*
	 * Only directories can have Default ACLs
	 */
	if (!is_dir(info))
		return;

	if (info->f_xflags & XF_ACL_DEFAULT) {
		acl_check_ids(acltext, info->f_acl_default);
	} else {
		acltext[0] = '\0';
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
		return;
#endif
	}
	if ((acl = acl_from_text(acltext)) == NULL) {
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsg("Cannot convert default ACL '%s' to internal format for '%s'.\n",
					acltext, info->f_name);
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
					acltext, info->f_name);
				(void) errabort(E_SETACL, info->f_name, TRUE);
			}
		}
		acl_free(acl);
	}
}

#endif  /* HAVE_POSIX_ACL */

#ifdef	HAVE_SUN_ACL	/* Solaris */

#define	DID_OPT_ACL
EXPORT void
opt_acl()
{
	printf(" acl");
}

/*
 * Get the access control list for a file and convert it into the format
 * used by star.
 */
EXPORT BOOL
get_acls(info)
	register FINFO	*info;
{
		int		aclcount;
		aclent_t	*aclp;
	register char		*acltext;
	register char		*ap;
	register char		*dp;
	register char		*cp;
	register char		*ep;
		int		asize;
		int		dsize;

	info->f_acl_access = NULL;
	info->f_acl_default = NULL;

	/*
	 * Symlinks don't have ACLs
	 */
	if (is_symlink(info))
		return (TRUE);

#ifdef	HAVE_ST_ACLCNT
	aclcount = info->f_aclcnt;
#else
	if ((aclcount = acl(info->f_name, GETACLCNT, 0, NULL)) < 0) {
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
	if (acl(info->f_name, GETACL, aclcount, aclp) < 0) {
		if (!errhidden(E_GETACL, info->f_name)) {
			if (!errwarnonly(E_GETACL, info->f_name))
				xstats.s_getaclerrs++;
			errmsg("Cannot get ACL entries for '%s'.\n",
							info->f_name);
			(void) errabort(E_GETACL, info->f_name, TRUE);
		}
		return (FALSE);
	}
	seterrno(0);
	acltext = acltotext(aclp, aclcount);
	free(aclp);
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

	ap = acl_access_text;
	dp = acl_default_text;
	asize = PATH_MAX;
	dsize = PATH_MAX;

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
	if (ap == NULL || dp == NULL) {
		acl_access_text[0] = '\0';
		acl_default_text[0] = '\0';
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsgno(EX_BAD, "Cannot convert ACL entries (string too long) for '%s'.\n",
							info->f_name);
			(void) errabort(E_BADACL, info->f_name, TRUE);
		}
		return (FALSE);
	}

	if (ap > acl_access_text && ap[-1] == ',')
		--ap;
	*ap = '\0';
	if (dp > acl_default_text && dp[-1] == ',')
		--dp;
	*dp = '\0';

	if (*acl_access_text != '\0') {
		info->f_xflags |= XF_ACL_ACCESS;
		info->f_acl_access = acl_access_text;
	}
	if (*acl_default_text != '\0') {
		info->f_xflags |= XF_ACL_DEFAULT;
		info->f_acl_default = acl_default_text;
	}

#ifdef	ACL_DEBUG
error("access:  '%s'\n", acl_access_text);
error("default: '%s'\n", acl_default_text);
#endif

	return (TRUE);
}

/*
 * Convert Solaris ACL text into POSIX ACL text and add numerical user/group
 * ids.
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
 */
EXPORT void
set_acls(info)
	register FINFO	*info;
{
	int		aclcount;
	aclent_t	*aclp;
	char		acltext[PATH_MAX+1];
	char		aclbuf[8192];
	BOOL		no_acl = FALSE;

	aclbuf[0] = '\0';
	if (info->f_xflags & XF_ACL_ACCESS) {
		acl_check_ids(aclbuf, info->f_acl_access);
	}
	if (info->f_xflags & XF_ACL_DEFAULT) {
		register char *cp;
		register char *dp;
		register char *ep;

		acl_check_ids(acltext, info->f_acl_default);

		dp = aclbuf + strlen(aclbuf);
		if (dp > aclbuf)
			*dp++ = ',';
		for (cp = acltext; *cp; cp = ep) {
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
#ifdef	ACL_DEBUG
	error("aclbuf: '%s'\n", aclbuf);
#endif

	if (aclbuf[0] == '\0') {
		/*
		 * We may need to delete an inherited ACL.
		 */
		strcpy(aclbuf, base_acl(info->f_mode));
		no_acl = TRUE;
	} else if (streql(aclbuf, base_acl(info->f_mode))) {
		no_acl = TRUE;
	}

	seterrno(0);
	if ((aclp = aclfromtext(aclbuf, &aclcount)) == NULL) {
		if (geterrno() == 0)
			seterrno(EX_BAD);
		if (!errhidden(E_BADACL, info->f_name)) {
			if (!errwarnonly(E_BADACL, info->f_name))
				xstats.s_badacl++;
			errmsg("Cannot convert ACL '%s' to internal format for '%s'.\n",
				aclbuf, info->f_name);
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
					aclbuf, info->f_name);
					xstats.s_setacl++;
				(void) errabort(E_SETACL, info->f_name, TRUE);
			}
		}
		free(aclp);
	}
}

#endif	/* HAVE_SUN_ACL Solaris */

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
 * If we are in -numeric mode, we replace the user and groups names by the
 * user and group numbers from our internal format.
 *
 * If we are in non numeric mode, we check whether a user name or group name
 * is present on our current system. It the user/group name is known, then we
 * remove the numeric value from out internal format. If the user/group name
 * is not known, then we replace the name by the numeric value.
 */
LOCAL void
acl_check_ids(acltext, infotext)
	char	*acltext;
	char	*infotext;
{
	char	entry_buffer[PATH_MAX];
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
			js_snprintf(entry_buffer, PATH_MAX, "user:%s:%s",
				username, perms);
			token = entry_buffer;

		} else if (strncmp(token, "group:", 6) == 0 &&
		    strchr(",", token[6]) == NULL) {
			char *groupname = &token[6], *c = groupname;
			char *perms, *agid;
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
			js_snprintf(entry_buffer, PATH_MAX, "group:%s:%s",
				groupname, perms);
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
