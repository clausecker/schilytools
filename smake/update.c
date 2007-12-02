/* @(#)update.c	1.103 07/08/19 Copyright 1985, 88, 91, 1995-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)update.c	1.103 07/08/19 Copyright 1985, 88, 91, 1995-2007 J. Schilling";
#endif
/*
 *	Make program
 *	Macro handling / Dependency Update
 *
 *	Copyright (c) 1985, 88, 91, 1995-2007 by J. Schilling
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
#include <schily/types.h>
#include <schily/standard.h>
#include <schily/stdlib.h>		/* for free() */
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/libport.h>
#include "make.h"

EXPORT obj_t	*default_tgt;		/* Current 'make' arg or default tgt */
EXPORT BOOL	found_make;		/* Did we expand the $(MAKE) macro?  */

#define	RTYPE_NONE	-1		/* Undefined type (used to init)    */
#define	RTYPE_DEFAULT	0		/* Rule from .DEFAULT: target	    */
#define	RTYPE_SSUFFIX	1		/* Simple suffix rule		    */
#define	RTYPE_SUFFIX	2		/* Single suffix rule		    */
#define	RTYPE_DSUFFIX	3		/* Double suffix rule		    */
#define	RTYPE_PATTERN	4		/* Pattern matching rule	    */

#define	RTYPE_NEEDFREE	0x1000		/* cmd_t * list needs to be free'd  */

#define	rule_type(t)	((t) & ~RTYPE_NEEDFREE)

EXPORT	void	initchars	__PR((void));
EXPORT	char	*filename	__PR((char * name));
LOCAL	void	copy_dir	__PR((char * name, char *dir, size_t dsize));
LOCAL	char	*get_suffix	__PR((char *name, char *suffix));
LOCAL	void	copy_base	__PR((char *name, char *dir, size_t dsize, char *suffix));
EXPORT	BOOL	isprecious	__PR((obj_t * obj));
EXPORT	BOOL	isphony		__PR((obj_t * obj));
LOCAL	patr_t	*_pattern_rule	__PR((patr_t * prule, char *name));
LOCAL	obj_t	*pattern_rule	__PR((obj_t * obj));
LOCAL	obj_t	*suffix_rule	__PR((obj_t * obj, int *rtypep));
LOCAL	void	suffix_warn	__PR((obj_t * obj));
LOCAL	obj_t	*ssuffix_rule	__PR((obj_t * obj));
LOCAL	obj_t	*default_rule	__PR((obj_t * obj, int *rtypep));
EXPORT	list_t	*list_nth	__PR((list_t * list, int n));
EXPORT	char	*build_path	__PR((int level, char *name, size_t namelen,
						char *path, size_t psize));
LOCAL	void	etoolong	__PR((char *topic, char *name));
LOCAL	void	grant_gbuf	__PR((int size));
LOCAL	void	sub_put		__PR((char *chunk, int size));
LOCAL	void	sub_c_put	__PR((int c));
LOCAL	void	sub_s_put	__PR((char *chunk));
LOCAL	BOOL	sub_arg		__PR((int n, list_t * depends, obj_t * target));
EXPORT	char	*substitute	__PR((char *cmd, obj_t * obj, obj_t * source, char *suffix));
LOCAL	char	*subst		__PR((char *cmd, obj_t * obj, obj_t * source, char *suffix, list_t * depends));
LOCAL	char	*dynmac		__PR((char *cmd, obj_t * obj, obj_t * source, char *suffix, list_t * depends, BOOL  domod));
LOCAL	void	warn_implicit	__PR((obj_t *obj, char *mac, char *exp));
LOCAL	void	extr_filenames	__PR((char *names));
LOCAL	void	extr_dirnames	__PR((char *names));
LOCAL	char	*exp_var	__PR((char end, char *cmd, obj_t * obj, obj_t * source, char *suffix, list_t *depends));
LOCAL	char	*rstr		__PR((char * s1, char * s2));
LOCAL	BOOL	patsub		__PR((char *name, char *f1, char *f2, char *t1, char *t2));
LOCAL	void	patmsub		__PR((char *name, char *f1, char *f2, char *t1, char *t2));
LOCAL	void	parsepat	__PR((char *pat, char **fp1, char **fp2, char **tp1, char **tp2));
EXPORT	char	*shout		__PR((char *cmd));
LOCAL	char	*shsub		__PR((list_t * l, obj_t * obj, obj_t * source, char *suffix, list_t * depends));
LOCAL	void	exp_name	__PR((char * name, obj_t * obj, obj_t * source, char *suffix, list_t * depends, char *));
LOCAL	void	dcolon_time	__PR((obj_t *obj));
LOCAL	date_t	searchobj	__PR((obj_t * obj, int maxlevel, int mode));
LOCAL	obj_t	*patr_src	__PR((char *name, patr_t * prule, int *rtypep, char ** suffixp, cmd_t ** pcmd, int dlev));
LOCAL	obj_t	*suff_src	__PR((char *name, obj_t * rule, int *rtypep, char ** suffixp, cmd_t ** pcmd, int dlev));
LOCAL	obj_t	*one_suff_src	__PR((char *name, char *suffix, cmd_t **pcmd, int dlev));
LOCAL	obj_t	*ssuff_src	__PR((char *name, obj_t * rule, int *rtypep, char ** suffixp, cmd_t ** pcmd, int dlev));
LOCAL	obj_t	*findsrc	__PR((obj_t *obj, obj_t * rule, int *rtypep, char ** suffixp, cmd_t ** pcmd, int dlev));
LOCAL	date_t	default_cmd	__PR((obj_t * obj, char *depname, date_t  deptime, int deplevel, BOOL  must_exist, int dlev));
LOCAL	date_t	make		__PR((obj_t * obj, BOOL lust_exist, int dlev));
EXPORT	BOOL	domake		__PR((char *name));
EXPORT	BOOL	omake		__PR((obj_t * obj, BOOL  must_exist));
EXPORT	BOOL	xmake		__PR((char *name, BOOL  must_exist));

char	chartype[256];

/*
 * Optimise character classification
 */
EXPORT void
initchars()
{
	char	*p;
	int	c;

	p = "@*<0123456789r^?";

	while ((c = *p++) != '\0') {
		chartype[c] |= DYNCHAR;
	}

	p = "0123456789";

	while ((c = *p++) != '\0') {
		chartype[c] |= NUMBER;
	}
}

/*
 * Return last pathname component.
 */
EXPORT char *
filename(name)
	register char	*name;
{
	register char	*fname;

	for (fname = name; *name; )
		if (*name++ == SLASH)
			fname = name;
	return (fname);
}

/*
 * Copy directory component of pathname.
 */
LOCAL void
copy_dir(name, dir, dsize)
	register char	*name;
	register char	*dir;
	register size_t	dsize;
{
	register char	*p = filename(name);
		char	*ns = name;

	if (XDebug > 0)
		error("copy_dir(name:'%s', dir:'%s', dsize: %d) fn: '%s' \n",
				name, dir, dsize, p);
	*dir = '\0';
	if (p == name) {
		if (dsize < 2)
			etoolong("copy directory name", ns);
		*dir++ = '.';
		*dir = '\0';
	} else {
		if (++dsize == 0) {	/* unsigned overflow */
			dsize--;
			while (name < p && dsize-- > 0)
				*dir++ = *name++;
			dsize++;
		} else {
			while (name < p && --dsize > 0)
				*dir++ = *name++;
		}
		if (dsize == 0)
			etoolong("copy directory name", ns);

		*--dir = '\0';		/* POSIX wants the '/' to be removed */
					/* This will make dir(/filename)    */
					/* not usable...		    */
		*dir = '\0';
	}
}

/*
 * Return part after '.' of last pathname component.
 */
LOCAL char *
get_suffix(name, suffix)
	char	*name;
	char	*suffix;
{
	register char	*p;
	register char	*suff = (char *)NULL;

	if (suffix != NULL) {
		p = filename(name);
		suff = rstr(p, suffix);
		if (suff == (char *)NULL) /* No suffix: return end of string */
			suff = &p[strlen(p)];
		return (suff);
	}

	for (p = filename(name); *p; p++)
		if (*p == '.')
			suff = p;
	if (suff == (char *)NULL)	/* No suffix: return end of string */
		suff = p;
	return (suff);
}

/*
 * Copy namebase (everything before '.').
 */
LOCAL void
copy_base(name, dir, dsize, suffix)
	register char	*name;
	register char	*dir;
	register size_t	dsize;
		char	*suffix;
{
	register char	*p = get_suffix(name, suffix);
		char	*ns = name;

	if (++dsize == 0) {	/* unsigned overflow */
		dsize--;
		while (name < p && dsize-- > 0)
			*dir++ = *name++;
		dsize++;
	} else {
		while (name < p && --dsize > 0)
			*dir++ = *name++;
	}
	if (dsize == 0)
		etoolong("copy base name", ns);

	*dir = '\0';
}

/*
 * Return TRUE if 'obj' is in the list of targets that should not be removed.
 */
EXPORT BOOL
isprecious(obj)
	obj_t	*obj;
{
	list_t	*l;

	if (Precious == (obj_t *)NULL)
		return (FALSE);

	for (l = Precious->o_list; l; l = l->l_next)
		if (obj == l->l_obj)
			return (TRUE);
	return (FALSE);
}

/*
 * Return TRUE if 'obj' is in the list of targets that should not be checked
 * aginst existing files. A .PHONY target is asumed to be never up to date,
 * it is not removed in case a signal is received.
 */
EXPORT BOOL
isphony(obj)
	obj_t	*obj;
{
	list_t	*l;

	if (Phony == (obj_t *)NULL)
		return (FALSE);

	for (l = Phony->o_list; l; l = l->l_next)
		if (obj == l->l_obj)
			return (TRUE);
	return (FALSE);
}

/*
 * Find pattern rule for 'name' starting at 'prule' in rules list.
 */
LOCAL patr_t *
_pattern_rule(prule, name)
	register patr_t	*prule;
	register char	*name;
{
	register char	*p;

	if (prule == NULL)
		return ((patr_t *)0);

	if (Debug > 1)
		printf("Searching pattern rule for: %s \n", name);

	for (; prule != NULL; prule = prule->p_next) {
		register char	*np;

		/*
		 * XXX NeXT Step has a buggy strstr(); returns NULL if p == ""
		 */
		p = (char *)prule->p_tgt_prefix;

		if (*p != '\0' && strstr(name, p) != name)
			continue;		/* no matching prefix */

		np = name + prule->p_tgt_pfxlen; /* strip matching prefix */

		if ((p = rstr(np, (char *)prule->p_tgt_suffix)) == NULL)
			continue;

#ifdef	DEBUG
		if (Debug > 1) {
			register cmd_t	*cmd;

			printf("name: %s (%s %% %s): (%s %% %s)\n", name,
				prule->p_tgt_prefix, prule->p_tgt_suffix,
				prule->p_src_prefix, prule->p_src_suffix);

			for (cmd = prule->p_cmd; cmd; cmd = cmd->c_next) {
				printf("\t%s\n", cmd->c_line);
			}
		}
#endif
		break;
	}
	return (prule);
}

/*
 * Find pattern rule for 'obj' ... not yet ready.
 */
LOCAL obj_t *
pattern_rule(obj)
	obj_t	*obj;
{
	/*
	 * XXX Hack for now (cast to obj_t *), should return prule.
	 */
	return ((obj_t *)_pattern_rule(Patrules, obj->o_name));
}

/*
 * Find a POSIX suffix rule.
 *
 * Check if obj has a file name with a default dependency for the
 * corresponding source and a rule to compile it.
 */
LOCAL obj_t *
suffix_rule(obj, rtypep)
	register obj_t	*obj;
		int	*rtypep;
{
	list_t	*l;
	list_t	*l2;
	obj_t	*o;
	char	*suffix;
	char	rulename[TYPICAL_NAMEMAX];	/* Space for two suffixes */
	char	*rule = rulename;
	char	*rp = NULL;
	int	rlen = sizeof (rulename);
	BOOL	found_suffix = FALSE;

	if (Suffixes == NULL)
		return ((obj_t *)0);

	if (Debug > 1)
		printf("Searching double suffix rule for: %s \n", obj->o_name);

	for (l = Suffixes; l; l = l->l_next) {
		suffix = l->l_obj->o_name;
		if (rstr(obj->o_name, suffix)) {	/* may be a suffix */

			found_suffix = TRUE;

			for (l2 = Suffixes; l2; l2 = l2->l_next) {
			again:
				if (snprintf(rule, rlen, "%s%s",
						l2->l_obj->o_name,
							suffix) >= rlen) {
					/*
					 * Expand rule name space.
					 */
					rlen = strlen(suffix) +
						l2->l_obj->o_namelen + 16;
					rule = rp = __realloc(rp, rlen,
							"suffix rule name");
					goto again;
				}
				if ((o = objlook(rule, FALSE)) != NULL && o->o_type == COLON) {
					*rtypep = RTYPE_DSUFFIX;
					if (rp)
						free(rp);
					return (o);
				}
			}
		}
	}
	if (rp)
		free(rp);
	if (found_suffix)
		return ((obj_t *) 0);

	if (Debug > 1)
		printf("Searching single suffix rule for: %s \n", obj->o_name);

	for (l2 = Suffixes; l2; l2 = l2->l_next) {
		rule = l2->l_obj->o_name;
		if ((o = objlook(rule, FALSE)) != NULL && o->o_type == COLON) {
			*rtypep = RTYPE_SUFFIX;
			return (o);
		}
	}
	return ((obj_t *) 0);
}

LOCAL void
suffix_warn(obj)
	obj_t	*obj;
{
	list_t	*l;

	if (obj->o_list == NULL)
		return;

	errmsgno(EX_BAD,
		"WARNING: suffix rule '%s' has superfluous dependency: '",
		obj->o_name);

	for (l = obj->o_list; l; l = l->l_next) {
		error("%s%s",
			l->l_obj->o_name,
			l->l_next?" ":"");
	}
	error("'.\n");
}

/*
 * Find a simple suffix rule.
 *
 * Check if obj has a file name with a default dependency for the
 * corresponding source and a rule to compile it.
 */
LOCAL obj_t *
ssuffix_rule(obj)
	register obj_t	*obj;
{
	register obj_t *rule;
		char	*ext;

	if (!SSuffrules)
		return ((obj_t *)0);

	if (Debug > 1)
		printf("Searching simple-suffix rule for: %s \n", obj->o_name);

	ext = get_suffix(obj->o_name, (char *)0); /* Use '.' (dot) suffix only*/

	if (ext[0] == '\0') {
		if (obj->o_list == (list_t *)NULL) {
			ext = "\"\"";		/* obj has no suffix: use "" */
		} else {
			return ((obj_t *)NULL);	/* obj has dependency list   */
		}
	}
	rule = ssufflook(ext, FALSE);
	if (rule == (obj_t *)NULL ||		/* no default rules known   */
	    rule->o_list == (list_t *)NULL ||	/* no source suffix list    */
	    rule->o_cmd == (cmd_t *)NULL)	/* no commands defined	    */
		return ((obj_t *)NULL);
	return (rule);
}

/*
 * Check if a default rules exists for the target.
 */
LOCAL obj_t *
default_rule(obj, rtypep)
	obj_t	*obj;
	int	*rtypep;
{
	obj_t *rule;

#ifdef	NO_SLASH_IMPLICIT
	if (strchr(obj->o_name, SLASH) != NULL) {
		if (Debug > 3)
			error("%s has slash, no implicit dependency searched.\n",
							obj->o_name);
		/*
		 * XXX We need to check if this is a good idea.
		 */
		rule = (obj_t *)NULL;
	}
#endif
	rule = pattern_rule(obj);
	if (rule) {
		*rtypep = RTYPE_PATTERN;
		return (rule);
	}

	rule = suffix_rule(obj, rtypep);
	if (rule) {
		return (rule);
	}

	rule = ssuffix_rule(obj);
	if (rule) {
		*rtypep = RTYPE_SSUFFIX;
		return (rule);
	}

	/*
	 * XXX Must exits wichtig ??
	 */
	*rtypep = RTYPE_DEFAULT;
	rule = Deflt;	/* .DEFAULT:	*/

	if (rule == (obj_t *)NULL || rule->o_cmd == (cmd_t *)NULL)
		return ((obj_t *)NULL);
	return ((obj_t *)rule->o_cmd);	/* XXX Hack for now, should return drule */
}

/*
 * Return the nth element of a list.
 */
EXPORT list_t *
list_nth(list, n)
	register list_t *list;
		int	n;
{
	for (; list; list = list->l_next)
		if (--n < 0)
			return (list);
	return ((list_t *)NULL);
}

/*
 * Create a new file name from name and the n'th directory in SearchList.
 * SearchList lists sourcedirs before objdirs, starting with
 * n = 0 for '.' and n = 1 for ObjDir.
 *
 * Returns:
 *		NULL	No search path at "level"
 *		name	level points to empty dirname
 *		path	New path in space provided py "path"
 *		other	Allocated new path
 */
EXPORT char *
build_path(level, name, namelen, path, psize)
	int	level;
	char	*name;
	size_t	namelen;
	char	*path;
	size_t	psize;
{
	list_t *lp;
	char	*dirname = (char *)NULL;
	register int n = level;

	if (n <= 1) {
		if (level == OBJLEVEL) {
			dirname = ObjDir;
			namelen += slashlen + ObjDirlen;
		}
	} else if (level != MAXLEVEL) {
		if ((lp = list_nth(SearchList, n - 2)) == (list_t *)NULL)
			return ((char *)NULL);
		dirname = lp->l_obj->o_name;
		namelen += slashlen + lp->l_obj->o_namelen;
	}
	if (dirname == (char *)NULL)
		return (name);

	if (namelen >= psize) {
		psize = namelen + 1;
		path = __realloc(NULL, psize, "build path name");
	}
	n = snprintf(path, psize, "%s%s%s", dirname, slash, name);
	if (n >= psize)
		etoolong("build path name", name);

	return (path);
}

LOCAL void
etoolong(topic, name)
	char	*topic;
	char	*name;
{
	comerrno(EX_BAD, "String too long, could not %s for '%s'.\n",
			topic, name);
	/* NOTREACHED */
}

/*
 * The growable buffer (gbuf) defines a string with the following layout
 *      "xxxxxxxxxxxxxxxCxxxxxxxxxxxxxxx________"
 *      ^               ^               ^               ^
 *      |               |               |               |
 *      gbuf            textp           sub_ptr         gbufend
 *      textp points to a string that is currently been worked on,
 *	sub_ptr is the write pointer.
 */
static char *sub_ptr = (char *)NULL;

LOCAL void
grant_gbuf(size)
	int	size;
{
	while (sub_ptr + size >= gbufend)
		sub_ptr = growgbuf(sub_ptr);
}

/*
 * Put a string bounded by size into the growable buffer.
 */
LOCAL void
sub_put(chunk, size)
	char	*chunk;
	int	size;
{
	grant_gbuf(size);
	movebytes(chunk, sub_ptr, size);
	sub_ptr += size;
}

/*
 * Put a single character into the growable buffer.
 */
LOCAL void
sub_c_put(c)
	int	c;
{
	grant_gbuf(1);
	*sub_ptr++ = c & 0xFF;
}

/*
 * Put a string bounded by strlen() into the growable buffer.
 */
LOCAL void
sub_s_put(chunk)
	char	*chunk;
{
	sub_put(chunk, strlen(chunk));
}

/*
 * Put one arg into the growable buffer.
 *
 * It target is nonzero, check in addition if the target
 * depends on the date of the currently selected obj too.
 *
 * Return FALSE if no more list elements are available.
 */
LOCAL BOOL
sub_arg(n, depends, target)
	int	n;
	list_t	*depends;
	obj_t	*target;
{
	register obj_t	*obj;
		char	arg[TYPICAL_NAMEMAX];
		char	*argp;

	if ((depends = list_nth(depends, n)) == (list_t *)NULL)
		return (FALSE);

	/*
	 * $0 is not available if no implicit source is present!
	 * Just skip it.
	 */
	if ((obj =
	    depends->l_obj) == (obj_t *)NULL)
		return (TRUE);

	/*
	 * It the target does not yet exist, target->o_date is set
	 * to RECURSETIME. We need to make sure that newtime in the
	 * dependencies (here obj) is considered > target->o_date.
	 */
	if (target != NULL &&
	    VALIDTIME(target->o_date) && target->o_date > obj->o_date) {
		return (TRUE);
	}
	if ((argp = build_path(obj->o_level, obj->o_name, obj->o_namelen,
						arg, sizeof (arg))) != NULL) {
		sub_s_put(argp);
		if (argp != obj->o_name && argp != arg) {
			free(argp);
		}
	} else {
		sub_s_put(obj->o_name);
	}
	return (TRUE);
}

/*
 * Do macro substitution. Substitution is done in the growable buffer buf.
 * The buffer is used as stack to allow recursive substitution.
 */
EXPORT char *
substitute(cmd, obj, source, suffix)
	register char	*cmd;
		obj_t	*obj;
		obj_t	*source;
		char	*suffix;
{
		list_t	depends;

	found_make = FALSE;			/* we did not expand $(MAKE) */
	depends.l_obj = source;			/* define implicit source $< */
	depends.l_next = obj->o_list;

	sub_ptr = gbuf;
	return (subst(cmd, obj, source, suffix, &depends));
}

static int	depth = 0;			/* Keep track of recursion   */

/*
 * Substitute macros.
 */
		/* source wird eigentlich nicht gebraucht */
LOCAL char *
subst(cmd, obj, source, suffix, depends)
	register char	*cmd;
		obj_t	*obj;
		obj_t	*source;
		char	*suffix;
		list_t	*depends;
{
		char	*sp = sub_ptr;
		char	*sb = gbuf;
	register char	*p;
		char	name[2];

	if (++depth > 100)
		comerrno(EX_BAD, "Recursion in macro '%s'.\n", cmd);

	name[1] = '\0';
	while ((p = strchr(cmd, '$')) != NULL) {
		sub_put(cmd, p - cmd);
		cmd = ++p;
		switch (*cmd++) {

		default:
			if (chartype[*(Uchar *)p] & DYNCHAR) {
				cmd = dynmac(p, obj, source, suffix, depends, FALSE);
				continue;
			}
			name[0] = cmd[-1];
			exp_name(name, obj, source, suffix, depends, Nullstr);
			break;
		case '\0':
			*sub_ptr = '\0';
			/*
			 * No need to update 'sb' as we exit here...
			 */
			if (sb != gbuf)
				sp = gbuf + (sp - sb);
			comerrno(EX_BAD,
				"Fatal error '$' at end of string '%s$'\n",
									sp);
			/* NOTREACHED */
		case '$':
			sub_c_put('$');
			break;
		case '(':
			cmd = exp_var(')', cmd, obj, source, suffix, depends);
			break;
		case '{':
			cmd = exp_var('}', cmd, obj, source, suffix, depends);
			break;
		}
	}
	sub_s_put(cmd);
	*sub_ptr = '\0';
	depth--;
	if (sb != gbuf)
		sp = gbuf + (sp - sb);
	return (sp);
}

/*
 * Substitute dynamic macros.
 */
LOCAL char *
dynmac(cmd, obj, source, suffix, depends, domod)
	char	*cmd;
	obj_t	*obj;
	obj_t	*source;
	char	*suffix;
	list_t	*depends;
	BOOL	domod;
{
	int	num;
	char	_base[TYPICAL_NAMEMAX];
	char	*base = _base;
	char	*bp = NULL;
	size_t	blen = sizeof (_base);
	char	*sp = sub_ptr;
	char	*sb = gbuf;
	char	*sp1;
	char	*sb1;
	register char	*p = cmd;

	switch (*cmd++) {

	default:
		return (cmd);

	case '@':
		if (obj->o_flags & F_DCOLON)		/* Is a ::@ target */
			sub_s_put(obj->o_node->o_name);	/* $@ -> full target name */
		else
			sub_s_put(obj->o_name);	/* $@ -> full target name */
		break;
	case '*':
		if (obj->o_namelen >= blen) {
			blen = obj->o_namelen + 1;
			base = bp = __realloc(bp, blen, "base name");
		}
		if (suffix == NULL) {
			copy_base(obj->o_name, base, blen, suffix);
			if (!nowarn("$*"))
				warn_implicit(obj, "$*", base);
		}
#ifdef	used_to_be_in_former_versions
		copy_base(filename(obj->o_name), base, blen, suffix);
#endif
		if (ObjDir || SearchList) {	/* May be removed in 2010 */

			if (obj->o_name != filename(obj->o_name) &&
			    objlook("VPATH", FALSE) == NULL)
				error(
				"WARNING: Old: convert $* from '%s' -> '%s'\n",
					obj->o_name, filename(obj->o_name));
		}
		copy_base(obj->o_name, base, blen, suffix);
		sub_s_put(base);		/* $* -> target name base */
		break;
	case '<':
		if (depends->l_obj == NULL && !nowarn("$<"))
			warn_implicit(obj, "$<", "");
		sub_arg(0, depends, (obj_t *)0); /* $< -> implicit source  */
		break;
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		cmd = astoi(p, &num);		/* $1 -> first dependency */
		sub_arg(num, depends, (obj_t *)0); /* $0 -> implicit source  */
		break;
	case 'r':
		/*
		 * $r0 -> all dependencies + implicit source
		 * $r1 -> all dependencies
		 */
		sp1 = sub_ptr;
		sb1 = gbuf;
		cmd = astoi(cmd, &num);
		while (sub_arg(num++, depends, (obj_t *)0)) {
			if (sb1 != gbuf)
				sp1 = gbuf + (sp1 - sb1);
			if (sp1 != sub_ptr)	/* Add only if nonempty arg */
				sub_c_put(' ');
			sp1 = sub_ptr;
			sb1 = gbuf;
		}
		if (sb != gbuf)
			sp = gbuf + (sp - sb);
		if (sp != sub_ptr && sub_ptr[-1] == ' ')
			sub_ptr--;		/* Kill last space */
		break;
	case '^':
		sp1 = sub_ptr;
		sb1 = gbuf;
		num = 1;			/* $^ -> all dependencies */
		while (sub_arg(num++, depends, (obj_t *)0)) {
			if (sb1 != gbuf)
				sp1 = gbuf + (sp1 - sb1);
			if (sp1 != sub_ptr)	/* Add only if nonempty arg */
				sub_c_put(' ');
			sp1 = sub_ptr;
			sb1 = gbuf;
		}
		if (sb != gbuf)
			sp = gbuf + (sp - sb);
		if (sp != sub_ptr && sub_ptr[-1] == ' ')
			sub_ptr--;		/* Kill last space */
		break;
	case '?':
		sp1 = sub_ptr;
		sb1 = gbuf;
		num = 1;			/* $? -> outdated dependencies*/
		while (sub_arg(num++, depends, obj)) {
			if (sb1 != gbuf)
				sp1 = gbuf + (sp1 - sb1);
			if (sp1 != sub_ptr)	/* Add only if nonempty arg */
				sub_c_put(' ');
			sp1 = sub_ptr;
			sb1 = gbuf;
		}
		if (sb != gbuf)
			sp = gbuf + (sp - sb);
		if (sp != sub_ptr && sub_ptr[-1] == ' ')
			sub_ptr--;		/* Kill last space */
		break;
	}
	*sub_ptr = '\0';
	if (bp)
		free(bp);
	if (!domod)
		return (cmd);
	if (sb != gbuf)
		sp = gbuf + (sp - sb);
	if (*cmd == 'F') {
		extr_filenames(sp);		/* 'sp' must be in gbuf */
		return (++cmd);
	}
	if (*cmd == 'D') {
		extr_dirnames(sp);		/* 'sp' must be in gbuf */
		return (++cmd);
	}
	return (cmd);
}

LOCAL void
warn_implicit(obj, mac, exp)
	obj_t	*obj;
	char	*mac;
	char	*exp;
{
	errmsgno(EX_BAD,
	"WARNING: requesting implicit dynmac '%s' for explicit target '%s'\n",
			mac, obj->o_name);
	errmsgno(EX_BAD,
		"WARNING: expanding implicit dynmac  '%s' to '%s'\n",
			mac, exp);
	errmsgno(EX_BAD,
		"WARNING: Current working directory:  '%s', Makefile '%s'\n",
				curwdir(), MakeFileNames[obj->o_fileindex]);
}

/*
 * Extract the filename parts from a string that contains a list of names.
 *
 * The parameter 'names' is expected to be on the 'growable buffer'.
 * If we ever need to use extr_filenames() otherwise, we need to add a boolean
 * parameter that tells extr_filenames() whether 'names' needs to be corrected
 * if 'gbuf' did change or not.
 */
LOCAL void
extr_filenames(names)
	char	*names;
{
	char	*p;
	char	*np;
	char	*s;
	char	*sp;
	char	*sb;

	sp = ++sub_ptr;
	sb = gbuf;
	/*
	 * Make sure that gbuf has enough room for the copy.
	 */
	grant_gbuf(sub_ptr - names);
	if (sb != gbuf) {
		sp = gbuf + (sp - sb);
		names = gbuf + (names - sb);
	}
	for (np = p = names, s = sp; np && *np; p = np) {
		np = strchr(np, ' ');
		if (np)
			*np++ = '\0';
		p = filename(p);
		while (*p)
			*s++ = *p++;
		if (np)
			*s++ = ' ';
	}
	*s = '\0';
	/*
	 * Now copy down from uppe part of gbuf.
	 */
	for (s = names, p = sp; *p; )
		*s++ = *p++;
	*s = '\0';
	sub_ptr = s;
}

/*
 * Extract the directory parts from a string that contains a list of names.
 *
 * The parameter 'names' is expected to be on the 'growable buffer'.
 * If we ever need to use extr_dirnames() otherwise, we need to add a boolean
 * parameter that tells extr_dirnames() whether 'names' needs to be corrected
 * if 'gbuf' did change or not.
 */
LOCAL void
extr_dirnames(names)
	char	*names;
{
	char	*p;
	char	*np;
	char	*s;
	char	*sp;
	char	*sb;
	char	_base[TYPICAL_NAMEMAX];
	char	*base = _base;
	char	*bp = NULL;
	size_t	blen = sizeof (_base);
	size_t	len;

	sp = ++sub_ptr;
	sb = gbuf;
	/*
	 * Make sure that gbuf has enough room for the copy.
	 */
	grant_gbuf(sub_ptr - names);
	if (sb != gbuf) {
		sp = gbuf + (sp - sb);
		names = gbuf + (names - sb);
	}
	for (np = p = names, s = sp; np && *np; p = np) {
		np = strchr(np, ' ');
		if (np) {
			*np++ = '\0';
			len = np - p;
		} else {
			len = strlen(p) + 1;
		}
		if (len > blen) {
			blen = len + 32;	/* Add some reserve */
			base = bp = __realloc(bp, blen, "dir base");
		}
		copy_dir(p, base, blen);
		p = base;
		while (*p)
			*s++ = *p++;
		if (np)
			*s++ = ' ';
	}
	*s = '\0';
	if (bp)
		free(bp);
	/*
	 * Now copy down from upper part of gbuf.
	 */
	for (s = names, p = sp; *p; )
		*s++ = *p++;
	*s = '\0';
	sub_ptr = s;
}

#define	white(c)	((c) == ' ' || (c) == '\t')

/*
 * Expand a macro.
 * As the replacement may be a suffix rule or a pattern rule too,
 * we first must get the basic name the macro referres to.
 */
#ifdef	PROTOTYPES
LOCAL char *
exp_var(
	register char	end,
	char		*cmd,
	obj_t		*obj,
	obj_t		*source,
	char		*suffix,
	list_t		*depends)
#else
LOCAL char *
exp_var(end, cmd, obj, source, suffix, depends)
	register char	end;
		char	*cmd;
		obj_t	*obj;
		obj_t	*source;
		char	*suffix;
		list_t	*depends;
#endif
{
		char	_name[TYPICAL_NAMEMAX];
		char	*name = _name;
		char	*nep = &_name[sizeof (_name) - 2];
		char	*np = NULL;
		size_t	nlen = sizeof (_name);
		char	_pat[TYPICAL_NAMEMAX];
		char	*pat = _pat;
		char	*pep = &_pat[sizeof (_pat) - 2];
		char	*pp = NULL;
		size_t	plen = sizeof (_pat);
	register char	beg = cmd[-1];
	register char	*s = cmd;
	register char	*rname = name;
	register char	ch;
	register int	nestlevel = 0;
		BOOL	funccall = FALSE;

/*error("end: %c cmd: %.50s\n", end, cmd);*/
	pat[0] = '\0';
	while ((ch = *s) != '\0' && ch != ':' && !white(ch)) {
		if (ch == beg)
			nestlevel++;
		if (ch == end)
			nestlevel--;
		if (nestlevel < 0) {
/*printf("name: %s\n", name);*/
			break;
		}
		if (rname >= nep) {
			nlen += TYPICAL_NAMEMAX*2;
			name = __realloc(np, nlen, "macro name");
			if (np == NULL) {
				/*
				 * Copy old content
				 */
				strlcpy(name, _name, sizeof (_name));
				rname = name + (rname - _name);
			} else {
				rname = name + (rname - np);
			}
			np = name;
			nep = &name[nlen - 2];
		}
		*rname++ = *s++;
	}
	*rname = '\0';

	if (ch == ' ')
		funccall = TRUE;

	if (*s != end && *s != ':' && *s != ' ') {
		comerrno(EX_BAD, "Missing '%c' in macro call '%s'\n", end, name);
		/* NOTREACHED */
/*		return (cmd);*/
	}

	if (*s == ':' || *s == ' ') {
		rname = pat;
		if (funccall) {
			while (*s && white(*s))
				s++;
		} else {
			s++;
		}
		while ((ch = *s) != '\0') {
			if (ch == beg)
				nestlevel++;
			if (ch == end)
				nestlevel--;
			if (nestlevel < 0) {
/*printf("name: %s\n", name);*/
				break;
			}
			if (rname >= pep) {
				plen += TYPICAL_NAMEMAX*2;
				pat = __realloc(pp, plen, "macro pattern");
				if (pp == NULL) {
					/*
					 * Copy old content
					 */
					strlcpy(pat, _pat, sizeof (_pat));
					rname = pat + (rname - _pat);
				} else {
					rname = pat + (rname - pp);
				}
				pp = pat;
				pep = &pat[plen - 2];
			}
			*rname++ = *s++;
		}
		*rname = '\0';

		if (nestlevel >= 0)
			comerrno(EX_BAD, "Missing '%c' in macro call '%s%c%s'\n",
					end, name, funccall?' ':':', pat);
	}
	if (*s)
		s++;

	if (name[0] == 'M' && streql(name, "MAKE"))
		found_make = TRUE;

	/*
	 * If the name of the macro contains a '$', resursively expand it.
	 * We need to check if we should rather expand anything between the
	 * brackets (e.g {...}) however, this may fail to expand long lists.
	 * See also comment in exp_name() regarding USE_SUBPAT
	 */
	if (strchr(name, '$')) {
		char	*sp = sub_ptr;
		char	*sb = gbuf;
		char	*s2;

		*sub_ptr++ = '\0';

		s2 = subst(name, obj, source, suffix, depends);
		if (sb != gbuf)
			sp = gbuf + (sp - sb);
		sub_ptr = sp;
		if (*s2) {
			if (strlcpy(name, s2, nlen) >= nlen) {
				nlen = strlen(s2) +1;
				name = np = __realloc(np, nlen, "macro name");
				if (strlcpy(name, s2, nlen) >= nlen)
					etoolong("copy macro content", s2);
			}
		}
	}

	if (funccall) {
		/*
		 * GNU type macro functions will go here.
		 */
		goto out;
	}
/*printf("name: '%s' pat: '%s'\n", name, pat);*/
	exp_name(name, obj, source, suffix, depends, pat);
out:
	if (np)
		free(np);
	if (pp)
		free(pp);
	return (s);
}

/*
 * Check if s1 ends in strings s2
 */
LOCAL char *
rstr(s1, s2)
	char	*s1;
	char	*s2;
{
	int	l1;
	int	l2;

	l1 = strlen(s1);
	l2 = strlen(s2);
	if (l2 > l1)
		return ((char *)NULL);

	if (streql(&s1[l1 - l2], s2))
		return (&s1[l1 - l2]);
	return ((char *)NULL);
}

/*
 * Substitute a pattern:
 * 1)	select the part of 'name' that is surrounded by f1 & f2
 * 2a)	output the selected part from 'name' and t1 (suffix)
 * 2b)	output t2 -
 *	if t2 contains '%' substitute the selected part of 'name'
 *
 * The parameter 'name' is expected to be on the 'growable buffer'.
 * If we ever need to use patsub() otherwise, we need to add a boolean
 * parameter that tells patsub() whether 'name' needs to be corrected
 * if 'gbuf' did change or not.
 */
LOCAL BOOL
patsub(name, f1, f2, t1, t2)
	char	*name;
	char	*f1;
	char	*f2;
	char	*t1;
	char	*t2;
{
	int	l;
	char	*p;
	char	*sb = gbuf;

/*	printf("name: '%s' f1: '%s' f2: '%s' t1: '%s' t2: '%s'\n", name, f1, f2, t1, t2);*/

	/*
	 * Make sure $(VAR:%=PREFIX/%) does not expand if $VAR is empty.
	 */
	if (*name == '\0' && *f1 == '\0' && *f2 == '\0')
		return (FALSE);
	/*
	 * XXX NeXT Step has a buggy strstr(); returns NULL if f1 == ""
	 * XXX f1 is guaranteed to be != NULL
	 */
	if (*f1 != '\0' && strstr(name, f1) != name)
		return (FALSE);		/* no matching prefix */

	name += strlen(f1);		/* strip matching prefix */

	if ((p = rstr(name, f2)) == NULL)
		return (FALSE);		/* no matching suffix */

	l = p - name;
	if (t1 != NULL) {		/* This is a suffix rule */
		grant_gbuf(l);		/* Grow gbuf before sub_put() */
		if (sb != gbuf) {
			name = gbuf + (name - sb);
			sb = gbuf;
		}
		sub_put(name, l);	/* 'name' is on gbuf... */
		p = t1;
	} else {			/* This is a pattern rule */
		p = t2;
	}
	while (*p) {
		if (*p == '%') {
			p++;
			grant_gbuf(l);	/* Grow gbuf before sub_put() */
			if (sb != gbuf) {
				name = gbuf + (name - sb);
				sb = gbuf;
			}
			sub_put(name, l); /* 'name' is on gbuf... */
		} else {
			sub_c_put(*p++);
		}
	}
	return (TRUE);
}

#ifdef	needed
/*
 * Do pattern substitution in a macro that may only contain one word.
 *
 * The parameter 'name' is expected to be on the 'growable buffer'.
 * If we ever need to use patssub() otherwise, we need to add a boolean
 * parameter that tells patssub() whether 'name' needs to be corrected
 * if 'gbuf' did change or not.
 */
LOCAL void
patssub(name, f1, f2, t1, t2)
	char	*name;
	char	*f1;
	char	*f2;
	char	*t1;
	char	*t2;
{
	char	*osp = name;
	char	*sp = sub_ptr;
	char	*sb = gbuf;

	*sub_ptr++ = '\0';

	if (!patsub(name, f1, f2, t1, t2)) {
		sub_ptr = sp;
		return;
	}
	*sub_ptr = '\0';
	sub_ptr = osp;
	if (sb != gbuf) {
		sp = gbuf + (sp - sb);
		sub_ptr = gbuf + (sub_ptr - sb);
	}
	sub_s_put(&sp[1]);
}
#endif

/*
 * Do pattern substitution in a macro that may contain multiple words.
 * Subtitution is applied separately to each word.
 *
 * The parameter 'name' is expected to be on the 'growable buffer'.
 * If we ever need to use patmsub() otherwise, we need to add a boolean
 * parameter that tells patmsub() whether 'name' needs to be corrected
 * if 'gbuf' did change or not.
 */
LOCAL void
patmsub(name, f1, f2, t1, t2)
	char	*name;
	char	*f1;
	char	*f2;
	char	*t1;
	char	*t2;
{
	char	*osp = name;
	char	*sp = sub_ptr;
	char	*sb = gbuf;
	char	*b;
	char	c;

	*sub_ptr++ = '\0';

	do {
		b = name;
		while (*b != '\0' && !white(*b))
			b++;
		if ((c = *b) != '\0')
			*b++ = '\0';
		else
			b = NULL;

/*error("name '%s'\n", name);*/
		if (!patsub(name, f1, f2, t1, t2)) {
			char	*n = name;

			grant_gbuf(strlen(name));	/* Grow gbuf before */
			if (sb != gbuf)
				n = gbuf + (n - sb);
			sub_s_put(n);			/* 'n' is on gbuf... */
		}
		if (sb != gbuf) {
			sp = gbuf + (sp - sb);
			osp = gbuf + (osp - sb);
			name = gbuf + (name - sb);
			if (b != NULL)
				b = gbuf + (b - sb);
			sb = gbuf;
		}

		if (b) {
			sub_c_put(c);
			while (*b != '\0' && white(*b))
				sub_c_put(*b++);
		}
		name = b;
	} while (b);
	*sub_ptr = '\0';
	sub_ptr = osp;
	if (sb != gbuf) {
		sp = gbuf + (sp - sb);
		sub_ptr = gbuf + (sub_ptr - sb);
	}
	sub_s_put(&sp[1]);
}

/*
 * Parse a pattern and divide pattern into parts.
 *
 * Check if this is a suffix rule or a pattern rule.
 *
 * If this is a suffix rule (suf1=suf2), tp2 will point to a NULL pointer,
 * if this is a pattern rule (pref1%suf1=pref1%suf2) tp1 will point to NULL.
 */
LOCAL void
parsepat(pat, fp1, fp2, tp1, tp2)
	char	*pat;
	char	**fp1;
	char	**fp2;
	char	**tp1;
	char	**tp2;
{
	char	*f1;
	char	*f2;
	char	*t1;
	char	*t2;

	t1 = strchr(pat, '=');
	if (t1 == NULL)
		comerrno(EX_BAD, "'=' missing in macro substitution.\n");
	*t1++ = '\0';

	f2 = strchr(pat, '%');		/* Find '%' in from patttern */
	if (f2 != NULL) {
		*f2++ = '\0';
		f1 = pat;
	} else {
		f2 = pat;
		f1 = Nullstr;
	}
	if (f1 == pat) {		/* This is a pattern rule */
		t2 = t1;
		t1 = NULL;
	} else {			/* This is a suffix rule */
		t2 = NULL;
	}
	*fp1 = f1;
	*fp2 = f2;
	*tp1 = t1;
	*tp2 = t2;
/*	printf("f1: '%s' f2: '%s' t1: '%s' t2: '%s'\n", f1, f2, t1, t2);*/
}

#if	!defined(HAVE_POPEN) && defined(HAVE__POPEN)
#	define	popen	_popen
#	define	HAVE_POPEN
#endif
#if	!defined(HAVE_PCLOSE) && defined(HAVE__PCLOSE)
#	define	pclose	_pclose
#	define	HAVE_PCLOSE
#endif
/*
 * Call shell command and capture the outpout in the growable buffer.
 */
EXPORT char *
shout(cmd)
	char	*cmd;
{
	FILE	*f;
	int	c;
	char	*sptr = sub_ptr;
	char	*sbuf = gbuf;

	if (sub_ptr == NULL)
		sptr = sub_ptr = gbuf;

	f = popen(cmd, "r");
	if (f == NULL)
		comerr("Cannot popen '%s'.\n", cmd);

	while ((c = getc(f)) != EOF) {
		if (c == '\t' || c == '\n')
			c = ' ';
		sub_c_put(c);
	}
	if ((c = pclose(f)) != 0)
		comerr("Shell returns %d from command line.\n", c);
	*sub_ptr = '\0';
	if (sbuf != gbuf)
		sptr = gbuf + (sptr - sbuf);
	return (sptr);
}

/*
 * Substitute a shell command output
 */
LOCAL char *
shsub(l, obj, source, suffix, depends)
	register list_t *l;
		obj_t	*obj;
		obj_t	*source;
		char	*suffix;
		list_t  *depends;
{
	char	*sptr = sub_ptr;
	char	*sbuf = gbuf;

	if (sub_ptr == NULL)
		sptr = sub_ptr = gbuf;

	for (;;) {
		subst(l->l_obj->o_name, obj, source, suffix, depends);
		if ((l = l->l_next) != NULL)
			sub_c_put(' ');
		else
			break;
	}
	*sub_ptr = '\0';
	if (sbuf != gbuf)
		sptr = gbuf + (sptr - sbuf);
	sub_ptr = sptr;

	return (shout(sptr));
}

/*
 * Expand a macro for which the name is explicitely known.
 */
LOCAL void
exp_name(name, obj, source, suffix, depends, pat)
		char	*name;
		obj_t	*obj;
		obj_t	*source;
		char	*suffix;
		list_t  *depends;
		char	*pat;
{
	register list_t	*l = NULL;
		obj_t	*o = NULL;
		BOOL	ispat;
		char	epat[TYPICAL_NAMEMAX*2];	/* Was NAMEMAX */
		char	*epa = epat;
		char	*ep = NULL;
		char	*f1, *f2;
		char	*t1, *t2;
		char	*sp;
		char	*sb;

	if ((chartype[*(Uchar *)name] & DYNCHAR) == 0 ||
			(*name == 'r' && (chartype[((Uchar *)name)[1]] & NUMBER) == 0)) {
		/*
		 * Allow dynamic macros to appear in $() or ${} too.
		 */
		o = objlook(name, FALSE);
		if (o == (obj_t *)NULL) {
			/*
			 * Check for $O -> ObjDir (speudo dyn) macro.
			 * $O is a speudo dyn macro as we allow to overwrite it
			 * in order get full POSIX make compliance.
			 */
			if (name[0] == 'O' && name[1] == '\0') {
				/*
				 * If $O has not been overwritten, use ObjDir.
				 */
				if (ObjDir == NULL) {
					sp = sub_ptr;
					sb = gbuf;
					sub_c_put('.');
					*sub_ptr = '\0';
					if (pat[0] != '\0') {
						char	*p;
						p = subst(pat, obj, source,
							suffix, depends);
						if ((sub_ptr - p) >=
						    sizeof (epat)) {
							epa = ep = __realloc(ep,
								sub_ptr - p + 1,
								"pattern content");
						}
						strcpy(epa, p);
						/*
						 * Free space from subpat()
						 */
						sub_ptr = p;
						parsepat(epa, &f1, &f2, &t1, &t2);
						if (sb != gbuf)
							sp = gbuf + (sp - sb);
						/*
						 * patmsub() expects first
						 * parameter to be in 'gbuf'
						 */
						patmsub(sp, f1, f2, t1, t2);
					}
				} else {
					exp_name(".OBJDIR", obj, source, suffix, depends, pat);
				}
			}
			if (ep)
				free(ep);
			return;
		}
		if ((l = o->o_list) == (list_t *)NULL)
			return;
	}
	if (streql(pat, "sh")) {
		/*
		 * Allow SunPRO make compatible shell expansion as $(NAME:sh)
		 */
		if (o != NULL) {
			shsub(o->o_list, obj, source, suffix, depends);
		} else {
			/*
			 * Process dynamic macros that appear in $() or ${}.
			 */
			sp = sub_ptr;
			sb = gbuf;
			dynmac(name, obj, source, suffix, depends, TRUE);
/*error("expanded1: '%s'\n", sp);*/
			if (sb != gbuf)
				sp = gbuf + (sp - sb);
			sub_ptr = sp;
			(void) shout(sp);
		}
		return;
	}
	ispat = pat[0] != '\0';
	if (ispat) {
		char	*p = subst(pat, obj, source, suffix, depends);

		if ((sub_ptr - p) >= sizeof (epat)) {
			epa = ep = __realloc(ep, sub_ptr - p + 1,
						"pattern content");
		}
		strcpy(epa, p);
		sub_ptr = p;	/* Free space from subpat() */
		parsepat(epa, &f1, &f2, &t1, &t2);
	}

	if (o == NULL) {
		/*
		 * Process dynamic macros that appear in $() or ${}.
		 */
		sp = sub_ptr;
		sb = gbuf;
		dynmac(name, obj, source, suffix, depends, TRUE);
/*error("expanded1: '%s'\n", sp);*/
		if (ispat) {
			if (sb != gbuf)
				sp = gbuf + (sp - sb);
			/*
			 * patmsub() expects first parameter to be in 'gbuf'
			 */
			patmsub(sp, f1, f2, t1, t2);
/*error("expanded2: '%s'\n", sp);*/
		}
		if (ep)
			free(ep);
		return;
	}
	for (;;) {
		sp = sub_ptr;
		sb = gbuf;
		subst(l->l_obj->o_name, obj, source, suffix, depends);
/*error("expanded1: '%s'\n", sp);*/
		if (ispat) {
			if (sb != gbuf)
				sp = gbuf + (sp - sb);
			/*
			 * patmsub() expects first parameter to be in 'gbuf'
			 */
			patmsub(sp, f1, f2, t1, t2);
/*error("expanded2: '%s'\n", sp);*/
		}
		if ((l = l->l_next) != NULL)
			sub_c_put(' ');
		else
			break;
/*error("expanded3: '%s'\n", sp);*/
	}
	if (ep)
		free(ep);
}

LOCAL void
dcolon_time(obj)
	register obj_t	*obj;
{
#ifdef	__really_needed__
	if (VALIDTIME(obj->o_node->o_date))
		return;
#endif

#ifdef	Do_not_use_o_node
	name = strchr(obj->o_name, '@');
	if (name == NULL)
		return;
	o = objlook(++name, FALSE);
#endif

	obj->o_node->o_date = obj->o_date;
	obj->o_node->o_level = obj->o_level;
}

/*
 * Find a file in .SEARCHLIST
 * .SEARCHLIST is a list of of src/obj dir pairs
 * Before .SEARCHLIST is searched, look for "." and .OBJDIR
 */
LOCAL date_t
searchobj(obj, maxlevel, mode)
	register obj_t	*obj;
		int	maxlevel;
		int	mode;
{
		char	name[TYPICAL_NAMEMAX];
		char	*namep = NULL;
		char	*oname = NULL;
	register int	level = MAXLEVEL;
		date_t	filedate;


	name[0] = '\0';			/* for clean debug print */

	if ((maxlevel & 1) == 0)	/* Source has lower level	*/
		maxlevel += 1;		/* include OBJ directory level	*/

	if (obj->o_date == PHONYTIME) {
		filedate = PHONYTIME;

	} else if (isphony(obj)) {
		obj->o_date = filedate = PHONYTIME;

#ifdef	NO_SEARCHOBJ_SLASH_IN_NAME
	/*
	 * Earlier versions of smake did allow slashes anywhere in path names
	 * and being searched for. A common practice in VPATH usage requires us
	 * to allow slashes inside a path name again.
	 */
	} else if (strchr(obj->o_name, SLASH) != (char *)NULL) {
#else
	/*
	 * If an object file name starts with a slash, is is an absolute path
	 * name so do not search.
	 */
	} else if (obj->o_name[0] == SLASH) {
#endif
		/*
		 * Do not search for pathnames.
		 */
		filedate = gftime(obj->o_name);
	} else for (level = 0, filedate = NOTIME; level <= maxlevel; level++) {
		if (level & 1 ? mode & SOBJ : mode & SSRC) {
			oname = obj->o_name;
			if ((obj->o_flags & F_DCOLON) != 0) {
				oname = obj->o_node->o_name;
				if ((namep = build_path(level,
						obj->o_node->o_name,
						obj->o_node->o_namelen,
						name, sizeof (name))) == NULL)
					break;
			} else if ((namep = build_path(level, obj->o_name,
						obj->o_namelen,
						name, sizeof (name))) == NULL)
				break;
			if ((filedate = gftime(namep)) != NOTIME)
				break;
			if (level == WDLEVEL && ObjDir == (char *)NULL) {
				/*
				 * Working dir has just been searched don't
				 * look again if no .OBJDIR has been defined.
				 */
				level++;
			}
		}
	}
	if (filedate == NOTIME || filedate == PHONYTIME)
		level = MAXLEVEL;

	obj->o_date = filedate;
	obj->o_level = level;
	if (obj->o_flags & F_DCOLON)
		dcolon_time(obj);

	if (Debug > 2) {
		error("search(%s, %d, %s) = %s %s %d\n",
				obj->o_name, maxlevel, searchtype(mode),
				prtime(filedate), namep, level);
	}
	if (namep != NULL && namep != oname && namep != name) {
		free(namep);
	}
	if (filedate == PHONYTIME) {
		return (NOTIME);
	}
	return (filedate);
}

/*
 * Find a source file for a given object file, use pattern matching rules.
 */
LOCAL obj_t *
patr_src(name, prule, rtypep, suffixp, pcmd, dlev)
	char	*name;
	patr_t	*prule;
	int	*rtypep;
	char	**suffixp;
	cmd_t	**pcmd;
	int	dlev;
{
		char	*xname = name;
	register obj_t	*source;
		obj_t	newsource;
		char	_sourcename[TYPICAL_NAMEMAX];
		char	*sourcename = _sourcename;
		char	*sp = NULL;
		size_t	slen = sizeof (_sourcename);
		char	_pat[TYPICAL_NAMEMAX];
		char	*pat = _pat;
		char	*pp = NULL;
		size_t	plen = sizeof (_pat);
		char	*p;
		size_t	len;

again:
	*suffixp = (char *)prule->p_tgt_suffix;

	p = (char *)prule->p_tgt_prefix;
	xname += prule->p_tgt_pfxlen;	/* strip matching prefix */
/*error("name: '%s'\n", xname);*/
	p = rstr(xname, (char *)prule->p_tgt_suffix);
/*error("p: '%s'\n", p);*/
	len = p - xname + 1;
	if (len > plen) {
		plen = len;
		pat = pp = __realloc(pp, plen, "pattern content");
	}
	strlcpy(pat, xname, len);
/*error("pat: '%s' len %d strlen %d\n", pat, len, strlen(pat));*/
	/*
	 * "len" contains the Null Byte
	 */
	if ((len = prule->p_src_pfxlen + len + prule->p_src_suflen) > slen) {
		slen = len;
		sourcename = sp = __realloc(sp, slen, "pattern source name");
	}
	if ((len = snprintf(sourcename, slen, "%s%s%s",
			prule->p_src_prefix, pat,
			prule->p_src_suffix)) >= slen) {
		etoolong("copy pattern source name", sourcename);
		/* NOTREACHED */
	}
/*error("sourcename: '%s'\n", sourcename);*/

	newsource.o_name = sourcename;
	newsource.o_namelen = len;
	newsource.o_date = NOTIME;
	newsource.o_type = 0;
	newsource.o_flags = 0;
	newsource.o_level = MAXLEVEL;
	newsource.o_fileindex = 0;
	newsource.o_list = (list_t *)NULL;
	newsource.o_cmd = (cmd_t *)NULL;
	newsource.o_node = (obj_t *)NULL;

	if (prule->p_flags & PF_TERM)
		newsource.o_flags |= F_TERM;

	if ((source = objlook(sourcename, FALSE)) != (obj_t *)NULL) {
		make(source, TRUE, dlev);	/* source found, update it  */
		/* XXX make() return value ??? */

	} else if (make(&newsource, FALSE,
			dlev) != NOTIME) {	/* found or made source */
		source = objlook(sourcename, TRUE);
		source->o_date = newsource.o_date;
		source->o_level = newsource.o_level;
		if (source->o_flags & F_DCOLON)
			dcolon_time(source);

	} else {
		/*
		 * This is a replicated search for additional matches for the
		 * same obj.
		 */
		prule = _pattern_rule(prule->p_next, name);
		if (prule != NULL)
			goto again;

		*pcmd = (cmd_t *)NULL;
		source = (obj_t *)0;
		goto out;
	}

	*pcmd = prule->p_cmd;
out:
	if (sp)
		free(sp);
	if (pp)
		free(pp);
	return (source);
}

/*
 * Find a source file for a given object file, use POSIX suffix rules.
 * Loop over all possible source names for all possible target suffixes.
 */
LOCAL obj_t *
suff_src(name, rule, rtypep, suffixp, pcmd, dlev)
	char	*name;
	obj_t	*rule;
	int	*rtypep;
	char	**suffixp;
	cmd_t	**pcmd;
	int	dlev;
{
	register list_t	*suf;		/* List of possible suffixes */
	register obj_t	*source;
	register char	*suffix = NULL;
		BOOL	found_suffix = FALSE;

	for (suf = Suffixes; suf; suf = suf->l_next) {
		suffix = suf->l_obj->o_name;
		if (rstr(name, suffix) == NULL)	/* may not be a suffix */
			continue;

		found_suffix = TRUE;
		*suffixp = suffix;
		if ((source = one_suff_src(name, suffix, pcmd, dlev)) != NULL)
			return (source);
	}
	if (found_suffix) {
		*pcmd = (cmd_t *)NULL;
		return ((obj_t *)0);
	}
	/*
	 * XXX We should never come here.
	 */
	errmsgno(EX_BAD, "XXX Expected double suffix rule but found none.\n");
	return (one_suff_src(name, Nullstr, pcmd, dlev));
}

/*
 * Find a source file for a given object file, use POSIX suffix rules.
 * Loop over all possible source names for one given target suffix.
 */
LOCAL obj_t *
one_suff_src(name, suffix, pcmd, dlev)
	char	*name;
	char	*suffix;
	cmd_t	**pcmd;
	int	dlev;
{
	register list_t	*suf;		/* List of possible suffixes */
	register obj_t	*o;
	register obj_t	*source;
		obj_t	newsource;
		char	_sourcename[TYPICAL_NAMEMAX];
		char	*sourcename = _sourcename;
		char	*sp = NULL;
		size_t	slen = sizeof (_sourcename);
		char	*endbase;
		char	_rulename[TYPICAL_NAMEMAX];
		char	*rulename = _rulename;
		char	*rp = NULL;
		size_t	rlen = sizeof(_rulename);
		size_t	endlen;
		size_t	sourcelen;
		size_t	len;

	if ((len = strlen(name)) >= slen) {
		slen = len + 16;		/* Add space for '\0' and suf */
		sourcename = sp = __realloc(sp, slen,
					"suffix source name");
	}
	copy_base(name, sourcename, slen, suffix);
	endbase = sourcename + strlen(sourcename);
	endlen = slen - (endbase - sourcename);

	newsource.o_name = sourcename;
	newsource.o_namelen = sourcelen = strlen(sourcename);
	newsource.o_date = NOTIME;
	newsource.o_type = 0;
	newsource.o_flags = 0;
	newsource.o_level = MAXLEVEL;
	newsource.o_fileindex = 0;
	newsource.o_list = (list_t *)NULL;
	newsource.o_cmd = (cmd_t *)NULL;
	newsource.o_node = (obj_t *)NULL;

	for (suf = Suffixes; suf; suf = suf->l_next) {
	again:
		if (snprintf(rulename, rlen,
					"%s%s",
					suf->l_obj->o_name,
					suffix) >= rlen) {
			/*
			 * Expand rule name space.
			 */
			rlen = strlen(suffix) + suf->l_obj->o_namelen + 16;
			rulename = rp = __realloc(rp, rlen, "suffix rule name");
			goto again;

		}

		if ((o = objlook(rulename, FALSE)) == NULL || o->o_type != COLON)
			continue;

		if (o->o_list)
			suffix_warn(o);

		if (Debug > 1)
			error("Trying %s suffix rule '%s' for: %s (suffix: '%s')\n",
				*suffix?"double":"single", rulename, name, suffix);

		if (suf->l_obj->o_namelen >= endlen) {
			slen = sourcelen + suf->l_obj->o_namelen + 1;
			sourcename = __realloc(sp, slen,
						"suffix source name");
			if (sp == NULL) {
				/*
				 * Copy old content
				 */
				strlcpy(sourcename, _sourcename, sourcelen+1);
			}
			newsource.o_name = sp = sourcename;
			endbase = sourcename + sourcelen;
			endlen = slen - (endbase - sourcename);
		}
		if (strlcpy(endbase, suf->l_obj->o_name, endlen) >= endlen)
			etoolong("build suffix source name", sourcename);
		newsource.o_namelen = sourcelen + suf->l_obj->o_namelen;
		if ((source = objlook(sourcename, FALSE)) != (obj_t *)NULL) {
			make(source, TRUE, dlev); /* source known, update it */
			/* XXX make() return value ??? */

			*pcmd = o->o_cmd;
			if (sp)
				free(sp);
			if (rp)
				free(rp);
			return (source);
		}
		if (make(&newsource, FALSE,
				dlev) != NOTIME) { /* found/made source */
			source = objlook(sourcename, TRUE);
			source->o_date = newsource.o_date;
			source->o_level = newsource.o_level;
			if (source->o_flags & F_DCOLON)
				dcolon_time(source);

			*pcmd = o->o_cmd;
			if (sp)
				free(sp);
			if (rp)
				free(rp);
			return (source);
		}
	}
	if (sp)
		free(sp);
	if (rp)
		free(rp);
	*pcmd = (cmd_t *)NULL;
	return ((obj_t *)0);
}

/*
 * Find a source file for a given object file, use simple suffix rules.
 */
LOCAL obj_t *
ssuff_src(name, rule, rtypep, suffixp, pcmd, dlev)
	char	*name;
	obj_t	*rule;
	int	*rtypep;
	char	**suffixp;
	cmd_t	**pcmd;
	int	dlev;
{
	register list_t	*suf = rule->o_list;	/* List of possible suffixes */
	register cmd_t	*cmd = rule->o_cmd;	/* List of implicit commands */
		cmd_t	*ncmd;			/* for allocated new command */
	register obj_t	*source;
		obj_t	newsource;
		char	_sourcename[TYPICAL_NAMEMAX];
		char	*sourcename = _sourcename;
		char	*sp = NULL;
		size_t	slen = sizeof (_sourcename);
		char	*endbase;
		size_t	endlen;
		size_t	sourcelen;
		size_t	len;

	*suffixp = get_suffix(name, (char *)0);	/* Use '.' (dot) suffix only */

	if ((len = strlen(name)) >= slen) {
		slen = len + 16;		/* Add space for '\0' and suf */
		sourcename = sp = __realloc(sp, slen,
					"simple suffix source name");
	}
	copy_base(name, sourcename, slen, (char *)NULL);
	endbase = sourcename + strlen(sourcename);
	endlen = slen - (endbase - sourcename);

	newsource.o_name = sourcename;
	newsource.o_namelen = sourcelen = strlen(sourcename);
	newsource.o_date = NOTIME;
	newsource.o_type = 0;
	newsource.o_flags = 0;
	newsource.o_level = MAXLEVEL;
	newsource.o_fileindex = 0;
	newsource.o_list = (list_t *)NULL;
	newsource.o_cmd = (cmd_t *)NULL;
	newsource.o_node = (obj_t *)NULL;

	do {
		if (suf->l_obj->o_namelen >= endlen) {
			slen = sourcelen + suf->l_obj->o_namelen + 1;
			sourcename = __realloc(sp, slen,
						"simple suffix source name");
			if (sp == NULL) {
				/*
				 * Copy old content
				 */
				strlcpy(sourcename, _sourcename, sourcelen+1);
			}
			newsource.o_name = sp = sourcename;
			endbase = sourcename + sourcelen;
			endlen = slen - (endbase - sourcename);
		}
		if (strlcpy(endbase, suf->l_obj->o_name, endlen) >= endlen)
			etoolong("build simple suffix source name", sourcename);
		newsource.o_namelen = sourcelen + suf->l_obj->o_namelen;
		if ((source = objlook(sourcename, FALSE)) != (obj_t *)NULL) {
			make(source, TRUE, dlev); /* source known, update it */
			/* XXX make() return value ??? */
			break;
		}
		if (make(&newsource, FALSE,
				dlev) != NOTIME) { /* found/made source */
			source = objlook(sourcename, TRUE);
			source->o_date = newsource.o_date;
			source->o_level = newsource.o_level;
			if (source->o_flags & F_DCOLON)
				dcolon_time(source);

			break;
		}
	} while ((suf = suf->l_next) != (list_t *)NULL &&
				(cmd = cmd->c_next) != (cmd_t *)NULL);
	if (sp)
		free(sp);
	if (source) {
		ncmd = (cmd_t *)fastalloc(sizeof (cmd_t));
		ncmd->c_next = (cmd_t *)NULL;
		ncmd->c_line = cmd->c_line;
		*pcmd = ncmd;
		*rtypep |= RTYPE_NEEDFREE;
	} else {
		*pcmd = (cmd_t *)NULL;
	}
	return (source);
}

LOCAL obj_t	*BadObj;
/*
 * Find a source file for a given object file.
 */
LOCAL obj_t *
findsrc(obj, rule, rtypep, suffixp, pcmd, dlev)
	obj_t	*obj;
	obj_t	*rule;
	int	*rtypep;
	char	**suffixp;
	cmd_t	**pcmd;
	int	dlev;
{
	int	rtype = *rtypep;

	if (BadObj == NULL) {
		BadObj = objlook("BAD OBJECT", TRUE);
		BadObj->o_date = BADTIME;
		BadObj->o_type = ':';
	}
	*suffixp = (char *)NULL;
	*pcmd = (cmd_t *)NULL;

	if (obj->o_flags & F_TERM)
		return ((obj_t *)0);

	switch (rtype) {

	case RTYPE_PATTERN:
		return (patr_src(obj->o_name, (patr_t *)rule,
					rtypep, suffixp, pcmd, dlev));

	case RTYPE_DSUFFIX:
		return (suff_src(obj->o_name, rule,
					rtypep, suffixp, pcmd, dlev));

	case RTYPE_SUFFIX:
		*suffixp = Nullstr;
		return (one_suff_src(obj->o_name, Nullstr, pcmd, dlev));

	case RTYPE_SSUFFIX:
		return (ssuff_src(obj->o_name, rule,
					rtypep, suffixp, pcmd, dlev));

	case RTYPE_DEFAULT:
		*pcmd = (cmd_t *)rule;
		return (obj);		/* This causes $< to be == $@ */

	default:
		break;
	}
	errmsgno(EX_BAD, "Impossible default rule type %X for '%s'.\n",
			rtype,
			obj->o_name);
	return (BadObj);
}

/*
 * Process a target with no explicit command list.
 */
LOCAL date_t
default_cmd(obj, depname, deptime, deplevel, must_exist, dlev)
	register obj_t	*obj;
		char	*depname;
		date_t	deptime;
		int	deplevel;
		BOOL	must_exist;
		int	dlev;
{
	obj_t	*rule = (obj_t *)NULL;
	obj_t	*source;
	cmd_t	*cmd;
	cmd_t	*cp;
	char	*suffix;
	int	rtype = RTYPE_NONE;


	if (dlev <= 0) {
		errmsgno(EX_BAD,
			"Recursion in default rule for '%s'.\n",
			obj->o_name);
		return (NOTIME);
	}

	if (((obj->o_flags & F_TERM) == 0) &&
	    ((rule = default_rule(obj, &rtype)) == (obj_t *)NULL)) {
		/*
		 * Found no default dependency rule for this target.
		 * This is most likely a placeholder target where no
		 * related file exists. As we did not found rules,
		 * we do not run commands and thus do not propagate deptime.
		 */
		if (obj->o_list != (list_t *)NULL) {
			obj->o_level = deplevel;
			return (NOTIME);
#ifdef	notdef
			/*
			 * This caused "all is up to date" messages.
			 */
			return (deptime);
#endif
		}
		/*
		 * Found no rule to make target.
		 */
		if (obj == default_tgt) {
			/*
			 * For intermediate placeholder targets like FORCE:
			 * that have no prerequisites, do not create an error.
			 */
			if (obj->o_list == (list_t *)NULL)
				return (NOTIME);
			errmsgno(EX_BAD, "'%s' does not say how to make '%s'.\n",
				MakeFileNames[obj->o_fileindex], obj->o_name);
			return (BADTIME);
		}
		/*
		 * Check if it is a source.
		 */
		if (searchobj(obj, MAXLEVEL, SSRC) != NOTIME)
			return (obj->o_date);
		if (!must_exist)
			return (NOTIME);
		/*
		 * For intermediate placeholder targets like FORCE:
		 * that have no prerequisites, do not create an error.
		 */
		if (obj->o_list == (list_t *)NULL)
			return (NOTIME);

		errmsgno(EX_BAD, "'%s' does not exist.\n", obj->o_name);
		return (BADTIME);
	}
	if ((source =
		findsrc(obj, rule, &rtype, &suffix, &cmd,
						dlev)) == (obj_t *)NULL) {

		/* Probably a obj file name. */
		if (searchobj(obj, MAXLEVEL, ObjSearch) != NOTIME)
			return (obj->o_date);

		if (Debug > 2)
			error("Cannot find source for: %s->time: %s %s(type: %X)\n",
				obj->o_name, prtime(obj->o_date),
				must_exist?"but must ":"", obj->o_type);
		if (!must_exist)
			return (NOTIME);
		/*
		 * A target must not exist, a dependency must.
		 */
		if (basetype(obj->o_type) == COLON)
			return (NOTIME);
		if (!NoWarn)
			errmsgno(EX_BAD,
				"Can't find any source for '%s'.\n",
							obj->o_name);
		if (NSflag)
			return (NOTIME);	/* -N specified */
		return (BADTIME);
	}

	if (source->o_date == BADTIME)
		goto badtime;

	if (source->o_date > deptime) {	/* target depends on this source too */
		depname = source->o_name;
		deptime = source->o_date;
	}
	if (source->o_level < deplevel)
		deplevel = source->o_level;
	if (deptime == BADTIME)		/* Errors occured cannot make source.*/
		goto badtime;

	if (deptime >= newtime ||		/* made some targets    */
	    !searchobj(obj, deplevel, SALL) ||	/* found no obj file    */
	    deptime > obj->o_date) {		/* target is out of date */

		if (Debug > 0)
			error("Default: %s is out of date to: %s%s\n",
						obj->o_name, depname,
						isphony(obj)?" and .PHONY:":"");
		if (Tflag) {
			if (!isphony(obj) && !touch_file(obj->o_name))
				goto badtime;
		} else for (cp = cmd; cp; cp = cp->c_next) {
			if (docmd(substitute(cp->c_line, obj, source, suffix), obj)) {
				goto badtime;
			}
		}
		obj->o_date = newtime;
		obj->o_level = WDLEVEL;
		if (obj->o_flags & F_DCOLON)
			dcolon_time(obj);

	}
	if ((rtype & RTYPE_NEEDFREE) != 0 && cmd != NULL)
		fastfree((char *)cmd, sizeof (*cmd));

	if (!Tflag && ObjDir != (char *)NULL && obj->o_level == WDLEVEL) {
		if (Debug > 3) {
			list_t	*l = list_nth(SearchList, source->o_level-2);

			error("%s: obj->o_level: %d %s: source->o_level: %d '%s'\n",
					obj->o_name, obj->o_level,
					source->o_name, source->o_level,
					l ? l->l_obj->o_name:Nullstr);
		}
		obj->o_level = source->o_level;
		if ((source->o_level & 1) == 0)	/* Source has lower level  */
			obj->o_level += 1;	/* use corresponding ObjDir*/

		if (!move_tgt(obj)) {		/* move target into ObjDir */
			obj->o_level = WDLEVEL;
			return (BADTIME);	/* move failed */
		}
/*		obj->o_level = OBJLEVEL;*/
	}
	return (obj->o_date);
badtime:
	if ((rtype & RTYPE_NEEDFREE) != 0 && cmd != NULL)
		fastfree((char *)cmd, sizeof (*cmd));

	return (BADTIME);
}

/*
 * Do all commands that are required to make the current target up to date.
 *
 * If any step fails, return BADTIME. This will stop dependent commands but
 * all other commands, not related to the failed target may be made.
 *
 * If must_exist is FALSE, return value 0 if no source file exists.
 * In this case, findsrc will try other suffixes in the implicit rules.
 */
LOCAL date_t
make(obj, must_exist, dlev)
	register obj_t	*obj;
		BOOL	must_exist;
		int	dlev;
{
		char	*depname = 0;
		date_t	deptime = NOTIME;
		date_t	mtime;
		short	deplevel = MAXLEVEL;
	register list_t	*l;
	register obj_t	*dep;
	register cmd_t	*cmd;

	if (Debug > 1)
		error("%sing %s\n", must_exist?"Mak":"Check", obj->o_name);

	if (obj->o_date != 0) {
		if (Debug > 2)
			error("make: %s->time: %s\n",
				obj->o_name, prtime(obj->o_date));
		/*
		 * We have been here before!
		 */
		if (obj->o_date == RECURSETIME)
			comerrno(EX_BAD, "Recursion in dependencies for '%s'.\n",
				obj->o_name);
		return (obj->o_date);
	}
	obj->o_date = RECURSETIME;	/* mark obj "we have been here" */
	if (obj->o_flags & F_DCOLON)
		dcolon_time(obj);

	/*
	 * Loop through the list of dependencies for obj, but do not try
	 * to "make" the names that appear as content of a NAME=val type macro.
	 */
	if (obj->o_type != EQUAL)
	for (l = obj->o_list; l; l = l->l_next) {
		dep = l->l_obj;
		mtime = make(dep, TRUE, dlev);
		if (!Kflag && mtime == BADTIME) {
			deptime = BADTIME;
			break;
		}


		if (dep->o_date > deptime) {
			/*
			 * Remember the date of the newest
			 * object in the list.
			 */
			depname = dep->o_name;
			deptime = dep->o_date;
		}
		if (dep->o_level < deplevel) {
			/*
			 * Remember the highest index in the search path
			 * for all objects in the list.
			 */
			deplevel = dep->o_level;
		}
	}
	if (deptime == BADTIME) {	/* Errors occured cannot make target.*/
		obj->o_date = BADTIME;
		if (obj->o_flags & F_DCOLON)
			dcolon_time(obj);

		return (obj->o_date);
	}

	if ((obj->o_type != DCOLON) &&		/* If final :: target */
	    (obj->o_flags & F_DCOLON) == 0 &&	/* If no intermediate target */
	    obj->o_cmd == (cmd_t *)NULL) {	/* Find default commands */
		obj->o_date = default_cmd(obj, depname, deptime,
							deplevel, must_exist,
							dlev < 0?16:dlev-1);
		if (obj->o_flags & F_DCOLON)
			dcolon_time(obj);
		/*
		 * Fake sucsess for targets listed in the makefile that have
		 * no exlicit commands no explicit dependencies (prerequisites)
		 * and where we * could not find implicit dependencies.
		 * These intermediate placeholde targets look similar to FORCE:
		 */
		if (obj->o_list == NULL && obj->o_date == NOTIME && must_exist)
			obj->o_date = newtime;

		if (obj->o_flags & F_DCOLON)
			dcolon_time(obj);

		return (obj->o_date);
	}

	/*
	 * There is an explicit command list for the current target.
	 */
	if (Debug > 2)
		error("deptime: %s\n", prtime(deptime));
	if (deptime >= newtime ||			/* made some targets */
	    !searchobj(obj, deplevel, ObjSearch) ||	/* found no obj file */
	    deptime > obj->o_date) {			/* tgt is out of date*/

		if (Debug > 0)
			error("Make:    %s is out of date to: %s%s\n",
						obj->o_name, depname,
						isphony(obj)?" and .PHONY:":"");
		if (Tflag) {
			if (!isphony(obj) && !touch_file(obj->o_name)) {

				obj->o_date = BADTIME;
				if (obj->o_flags & F_DCOLON)
					dcolon_time(obj);

				return (obj->o_date);
			}
		} else for (cmd = obj->o_cmd; cmd; cmd = cmd->c_next) {
			if (docmd(substitute(cmd->c_line, obj, (obj_t *)NULL, (char *)NULL),
									obj)) {

				obj->o_date = BADTIME;
				if (obj->o_flags & F_DCOLON)
					dcolon_time(obj);
				return (obj->o_date);
			}
		}
		if (obj->o_level == MAXLEVEL)
			searchobj(obj, deplevel, SALL);
		obj->o_date = newtime;
		if (obj->o_flags & F_DCOLON)
			dcolon_time(obj);
	}
	return (obj->o_date);
}

/*
 * This is the interface function that is to be called from main()
 * If called with a NULL pointer, it checks for the default target first,
 * otherwise the named target is made.
 */
EXPORT BOOL
domake(name)
	char	*name;
{
	date_t	mtime;

	if (name) {
		default_tgt = objlook(name, TRUE);
		if (Debug > 0)
			error("Target:  %s\n", name);
	} else if (default_tgt) {
		name = default_tgt->o_name;
		if (Debug > 0)
			error("DTarget:  %s\n", name);
	}
	if (default_tgt) {
		mtime = make(default_tgt, TRUE, -1);
		if (mtime == BADTIME) {
			errmsgno(EX_BAD, "Couldn't make '%s'.\n", name);
			if (Debug > 0)
				error("Current working directory:  %s\n", curwdir());
			return (FALSE);
		}
		if (mtime != NOTIME && mtime != newtime) {
			/*
			 * Nothing to do.
			 */
			if (!Qflag)
				errmsgno(EX_BAD, "'%s' is up to date.\n", name);
			return (TRUE);
		}
		if (Qflag)
			return (FALSE);
		return (TRUE);
	}
	errmsgno(EX_BAD, "No default Command defined.\n");
	usage(1);
	return (FALSE);	/* keep lint happy */
}

/*
 * Try to make target 'obj'.
 * Return TRUE, if target could be made.
 *
 * omake and xmake are used to make intermediate targets with no direct
 * dependency (e.g. .INIT and included files).
 */
EXPORT BOOL
omake(obj, must_exist)
	obj_t	*obj;
	BOOL	must_exist;
{
	date_t	mtime;

	if (obj == (obj_t *)NULL)
		return (TRUE);

	if (Debug > 2)
		error("xmake: %s->time: %s\n",
			obj->o_name, prtime(obj->o_date));
	mtime = make(obj, must_exist, -1);

	if (Debug > 2)
		error("xmake: %s\n", prtime(mtime));
	if (mtime == BADTIME) {
/*		errmsgno(EX_BAD, "Couldn't make '%s'.\n", obj->o_name);*/
		return (FALSE);
	}
	return (TRUE);
}

/*
 * First do an objlook(name), then make it using omake.
 */
EXPORT BOOL
xmake(name, must_exist)
	char	*name;
	BOOL	must_exist;
{
	obj_t	*o;

	o = objlook(name, TRUE);
	return (omake(o, must_exist));
}
