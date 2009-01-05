/* @(#)acltext.c	1.13 08/12/22 2005-2008 J. Schilling from SMI */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)acltext.c	1.13 08/12/22 2005-2008 J. Schilling from SMI";
#endif
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only.
 * See the file CDDL.Sun.txt in this distribution or
 * http://www.opensolaris.org/license/ for details.
 */

#include <schily/mconfig.h>
#if	defined(HAVE_SUN_ACL) && !defined(IS_CYGWIN)

#pragma ident	"@(#)acltext.c	1.11	02/10/02 SMI"
/*LINTLIBRARY*/

#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <schily/errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/acl.h>
#include <schily/schily.h>

/*
 * acltotext() converts each ACL entry to look like this:
 *
 *    entry_type:uid^gid^name:perms
 *
 * The maximum length of entry_type is 14 ("defaultgroup::" and
 * "defaultother::") hence ENTRYTYPELEN is set to 14.
 *
 * The max length of a uid^gid^name entry (in theory) is 8, hence we use
 * LOGNAME_MAX.
 *
 * The length of a perms entry is 4 to allow for the comma appended to each
 * to each acl entry.  Hence PERMS is set to 4.
 */
/*
 * Warning: LOGNAME_MAX is 8 but sizeof (struct utmpx.ut_user) is 32
 * 8 is also not sufficient to hold a int32_t uid_t and we may even need
 * it to be able to hold a int64_t uid_t which needs 20 chars.
 */
#if	LOGNAME_MAX < 32
#define	_LOGNAME_MAX_	32
#else
#define	_LOGNAME_MAX_	LOGNAME_MAX
#endif

#define	ENTRYTYPELEN	14
#define	PERMS		4
#define	ACL_ENTRY_SIZE	(ENTRYTYPELEN + _LOGNAME_MAX_ + PERMS)

struct dynaclstr {
	size_t bufsize;		/* current size of aclexport */
	char *aclexport;
};

static char *strappend(char *, char *);
static char *convert_perm(char *, o_mode_t);
static int increase_length(struct dynaclstr *, size_t);
static int isnumeric(char *);

#define	FREE	free(aclp);\
		free(allocp)

/*
 * Convert internal acl representation to external representation.
 *
 * The length of a non-owning user name or non-owning group name ie entries
 * of type DEF_USER, USER, DEF_GROUP or GROUP, can exceed LOGNAME_MAX.  We
 * thus check the length of these entries, and if greater than LOGNAME_MAX,
 * we realloc() via increase_length().
 *
 * The LOGNAME_MAX, ENTRYTYPELEN and PERMS limits are otherwise always
 * adhered to.
 */
char *
acltotext(aclent_t *aclp, int aclcnt)
{
	char		*aclexport;
	char		*where;
	struct group	*groupp;
	struct passwd	*passwdp;
	struct dynaclstr *dstr;
	int		i, rtn;
	size_t		excess = 0;

	if (aclp == NULL)
		return (NULL);
	if ((dstr = malloc(sizeof (struct dynaclstr))) == NULL)
		return (NULL);
	dstr->bufsize = aclcnt * ACL_ENTRY_SIZE;
	if ((dstr->aclexport = malloc(dstr->bufsize)) == NULL)
		return (NULL);
	*dstr->aclexport = '\0';
	where = dstr->aclexport;

	for (i = 0; i < aclcnt; i++, aclp++) {
		switch (aclp->a_type) {
		case DEF_USER_OBJ:
		case USER_OBJ:
			if (aclp->a_type == USER_OBJ)
				where = strappend(where, "user::");
			else
				where = strappend(where, "defaultuser::");
			where = convert_perm(where, aclp->a_perm);
			break;
		case DEF_USER:
		case USER:
			if (aclp->a_type == USER)
				where = strappend(where, "user:");
			else
				where = strappend(where, "defaultuser:");
			passwdp = getpwuid(aclp->a_id);
			if (passwdp == (struct passwd *)NULL) {
				/* put in uid instead */
				/* XXX better use %j and intmax_t */
				(void) sprintf(where, "%lld",
							(long long)aclp->a_id);
			} else {
				excess = strlen(passwdp->pw_name) -
								_LOGNAME_MAX_;
				if (excess > 0) {
					rtn = increase_length(dstr, excess);
					if (rtn == 1)
						/* reset where */
						where = dstr->aclexport +
							strlen(dstr->aclexport);
					else
						return (NULL);
				}
				where = strappend(where, passwdp->pw_name);
			}
			where = strappend(where, ":");
			where = convert_perm(where, aclp->a_perm);
			break;
		case DEF_GROUP_OBJ:
		case GROUP_OBJ:
			if (aclp->a_type == GROUP_OBJ)
				where = strappend(where, "group::");
			else
				where = strappend(where, "defaultgroup::");
			where = convert_perm(where, aclp->a_perm);
			break;
		case DEF_GROUP:
		case GROUP:
			if (aclp->a_type == GROUP)
				where = strappend(where, "group:");
			else
				where = strappend(where, "defaultgroup:");
			groupp = getgrgid(aclp->a_id);
			if (groupp == (struct group *)NULL) {
				/* put in gid instead */
				/* XXX better use %j and intmax_t */
				(void) sprintf(where, "%lld",
							(long long)aclp->a_id);
			} else {
				excess = strlen(groupp->gr_name) -
								_LOGNAME_MAX_;
				if (excess > 0) {
					rtn = increase_length(dstr, excess);
					if (rtn == 1)
						/* reset where */
						where = dstr->aclexport +
							strlen(dstr->aclexport);
					else
						return (NULL);
				}
				where = strappend(where, groupp->gr_name);
			}
			where = strappend(where, ":");
			where = convert_perm(where, aclp->a_perm);
			break;
		case DEF_CLASS_OBJ:
		case CLASS_OBJ:
			if (aclp->a_type == CLASS_OBJ)
				where = strappend(where, "mask:");
			else
				where = strappend(where, "defaultmask:");
			where = convert_perm(where, aclp->a_perm);
			break;
		case DEF_OTHER_OBJ:
		case OTHER_OBJ:
			if (aclp->a_type == OTHER_OBJ)
				where = strappend(where, "other:");
			else
				where = strappend(where, "defaultother:");
			where = convert_perm(where, aclp->a_perm);
			break;
		default:
			free(dstr->aclexport);
			free(dstr);
			return (NULL);

		}
		if (i < aclcnt - 1)
			where = strappend(where, ",");
	}
	aclexport = dstr->aclexport;
	free(dstr);
	return (aclexport);
}

/*
 * Convert external acl representation to internal representation.
 * The accepted syntax is: <acl_entry>[,<acl_entry>]*[,]
 * The comma at the end is not prescribed by the man pages.
 * But it is needed not to break the old programs.
 */
aclent_t *
aclfromtext(char *aclstr, int *aclcnt)
{
	char		*fieldp;
	char		*tp;
	char		*nextp;
	char		*allocp;
	char		*aclimport;
	int		entry_type;
	int		id;
	int		len;
	o_mode_t	perm;
	aclent_t	*tmpaclp;
	aclent_t	*aclp;
	struct group	*groupp;
	struct passwd	*passwdp;

	*aclcnt = 0;
	aclp = NULL;

	if (! aclstr)
		return (NULL);

	len = strlen(aclstr);

	if ((aclimport = allocp = strdup(aclstr)) == NULL) {
		(void) fprintf(stderr, "malloc() failed\n");
		return (NULL);
	}

	if (aclimport[len - 1] == ',')
		aclimport[len - 1] = '\0';

	for (; aclimport; ) {
		/* make sure id is always set up */
		id = -1;

		/* look for an ACL entry */
		tp = strchr(aclimport, ',');
		if (tp == NULL) {
			nextp = NULL;
		} else {
			*tp = '\0';
			nextp = tp + 1;
		}

		*aclcnt += 1;

		/*
		 * get additional memory:
		 * can be more efficient by allocating a bigger block
		 * each time.
		 */
		if (*aclcnt > 1)
			tmpaclp = (aclent_t *)realloc(aclp,
			    sizeof (aclent_t) * (*aclcnt));
		else
			tmpaclp = (aclent_t *)malloc(sizeof (aclent_t));
		if (tmpaclp == NULL) {
			free(allocp);
			if (aclp)
				free(aclp);
			return (NULL);
		}
		aclp = tmpaclp;
		tmpaclp = aclp + (*aclcnt - 1);

		/* look for entry type field */
		tp = strchr(aclimport, ':');
		if (tp == NULL) {
			FREE;
			return (NULL);
		} else
			*tp = '\0';
		if (strcmp(aclimport, "user") == 0) {
			if (*(tp+1) == ':')
				entry_type = USER_OBJ;
			else
				entry_type = USER;
		} else if (strcmp(aclimport, "group") == 0) {
			if (*(tp+1) == ':')
				entry_type = GROUP_OBJ;
			else
				entry_type = GROUP;
		} else if (strcmp(aclimport, "other") == 0)
			entry_type = OTHER_OBJ;
		else if (strcmp(aclimport, "mask") == 0)
			entry_type = CLASS_OBJ;
		else if (strcmp(aclimport, "defaultuser") == 0) {
			if (*(tp+1) == ':')
				entry_type = DEF_USER_OBJ;
			else
				entry_type = DEF_USER;
		} else if (strcmp(aclimport, "defaultgroup") == 0) {
			if (*(tp+1) == ':')
				entry_type = DEF_GROUP_OBJ;
			else
				entry_type = DEF_GROUP;
		} else if (strcmp(aclimport, "defaultmask") == 0)
			entry_type = DEF_CLASS_OBJ;
		else if (strcmp(aclimport, "defaultother") == 0)
			entry_type = DEF_OTHER_OBJ;
		else {
			FREE;
			return (NULL);
		}

		/* look for user/group name */
		if (entry_type != CLASS_OBJ && entry_type != OTHER_OBJ &&
		    entry_type != DEF_CLASS_OBJ &&
		    entry_type != DEF_OTHER_OBJ) {
			fieldp = tp + 1;
			tp = strchr(fieldp, ':');
			if (tp == NULL) {
				FREE;
				return (NULL);
			} else
				*tp = '\0';
			if (fieldp != tp) {
				/*
				 * The second field could be empty. We only care
				 * when the field has user/group name.
				 */
				if (entry_type == USER ||
				    entry_type == DEF_USER) {
					struct passwd	pw;

					/*
					 * The reentrant interface getpwnam_r()
					 * is uncommitted and subject to
					 * change. Use the friendlier interface
					 * getpwnam().
					 */
					if (isnumeric(fieldp)) {
						uid_t	uid;


						errno = 0;
						uid = (uid_t)strtol(fieldp,
								NULL, 10);
						if (errno == 0) {
							passwdp = &pw;
							passwdp->pw_uid = uid;
						} else {
							passwdp = NULL;
						}
					} else {
						passwdp = getpwnam(fieldp);
					}
					if (passwdp == NULL) {
						(void) fprintf(stderr,
						"user %s not found\n", fieldp);
						id = UID_NOBODY; /* nobody */
					} else {
						id = passwdp->pw_uid;
					}
				} else if (entry_type == GROUP ||
					    entry_type == DEF_GROUP) {
					struct group	gr;

					if (isnumeric(fieldp)) {
						gid_t	gid;

						errno = 0;
						gid = (gid_t)strtol(fieldp,
								NULL, 10);
						if (errno == 0) {
							groupp = &gr;
							groupp->gr_gid = gid;
						} else {
							groupp = NULL;
						}
					} else {
						groupp = getgrnam(fieldp);
					}
					if (groupp == NULL) {
						(void) fprintf(stderr,
						"group %s not found\n",
						fieldp);
						/* no group? */
						id = GID_NOBODY;
					} else {
						id = groupp->gr_gid;
					}
				} else {
					(void) fprintf(stderr,
					"acl import errors\n");
					FREE;
					return (NULL);
				}
			} else {
				/*
				 * The second field is empty.
				 * Treat it as undefined (-1)
				 */
				id = -1;
			}
		} else {
			/*
			 * Let's not break the old applications
			 * that use mask::rwx, other::rwx format,
			 * though they violate the man pages.
			 */
			if (*(tp + 1) == ':')
				*++tp = 0;
		}

		/* next field: permission */
		fieldp = tp + 1;
		if (strlen(fieldp) != 3) {
			/*  not "rwx" format */
			FREE;
			return (NULL);
		} else {
			char	s[] = "rwx";
			int	mask = 0x04;
			int	i;
			perm = 0;

			for (i = 0; i < 3; i++, mask /= 2) {
				if (fieldp[i] == s[i])
					perm |= mask;
				else if (fieldp[i] != '-') {
					FREE;
					return (NULL);
				}
			}
		}

		tmpaclp->a_type = entry_type;
		tmpaclp->a_id = id;
		tmpaclp->a_perm = perm;
		aclimport = nextp;
	}
	free(allocp);
	return (aclp);
}

static char *
strappend(char *where, char *newstr)
{
	(void) strcat(where, newstr);
	return (where + strlen(newstr));
}

static char *
convert_perm(char *where, o_mode_t perm)
{
	if (perm & 04)
		where = strappend(where, "r");
	else
		where = strappend(where, "-");
	if (perm & 02)
		where = strappend(where, "w");
	else
		where = strappend(where, "-");
	if (perm & 01)
		where = strappend(where, "x");
	else
		where = strappend(where, "-");
	/* perm is the last field */
	return (where);
}

/*
 * Callers should check the return code as this routine may change the string
 * pointer in dynaclstr.
 */
static int
increase_length(struct dynaclstr *dacl, size_t increase)
{
	char *tptr;
	size_t newsize;

	newsize = dacl->bufsize + increase;
	tptr = realloc(dacl->aclexport, newsize);
	if (tptr != NULL) {
		dacl->aclexport = tptr;
		dacl->bufsize = newsize;
		return (1);
	} else
		return (0);
}

static int
isnumeric(char *p)
{
	int	c;

	while ((c = *p++) != '\0') {
		if (!isdigit(c))
			return (0);
	}
	return (1);
}

#endif	/* defined(HAVE_SUN_ACL) && !defined(IS_CYGWIN) */
