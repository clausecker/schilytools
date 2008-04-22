/* @(#)parse.c	1.89 08/04/19 Copyright 1985 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)parse.c	1.89 08/04/19 Copyright 1985 J. Schilling";
#endif
/*
 *	Make program
 *	Parsing routines
 *
 *	Copyright (c) 1985 by J. Schilling
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
#include <schily/varargs.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* include sys/types.h for schily/schily.h */
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/ccomdefs.h>
#include "make.h"

#ifdef	pdp11
#define	MAXOBJS		64	/* Max # of targets in target list. */
#else
#define	MAXOBJS		512	/* Max # of targets in target list. */
#endif


EXPORT	void	parsefile	__PR((void));
LOCAL	void	define_obj	__PR((obj_t * obj, int n, int objcnt, int type, list_t * dep, cmd_t * cmd));
LOCAL	obj_t	*define_dcolon	__PR((obj_t *obj));
LOCAL	void	listappend	__PR((obj_t *obj, list_t *dep));
LOCAL	void	define_patrule	__PR((obj_t * obj, list_t * dep, cmd_t * cmd, int type));
LOCAL	void	print_patrules	__PR((FILE *f));
LOCAL	obj_t	*check_ssuffrule __PR((obj_t *obj, list_t *dep));
EXPORT	char	*get_var	__PR((char *name));
EXPORT	void	define_var	__PR((char *name, char *val));
LOCAL	int	getobjname	__PR((void));
LOCAL	obj_t	*getobj		__PR((void));
LOCAL	int	getname		__PR((int type));
LOCAL	obj_t	*getnam		__PR((int type));
LOCAL	int	getln		__PR((void));
LOCAL	list_t	**listcat	__PR((obj_t * obj, list_t ** tail));
LOCAL	list_t	**mklist	__PR((char *line, list_t ** tail, BOOL doexpand));
LOCAL	list_t	*cplist		__PR((list_t * l));
LOCAL	list_t	**exp_list	__PR((obj_t * o, list_t ** tail));
LOCAL	list_t	*getlist	__PR((int *typep));
LOCAL	BOOL	is_shvar	__PR((obj_t ** op, int *typep, list_t *** tailp));
LOCAL	list_t	*getshvar	__PR((int *typep));
LOCAL	cmd_t	*getcmd		__PR((void));
LOCAL	int	exp_ovec	__PR((obj_t ** ovec, int objcnt));
LOCAL	int	read_ovec	__PR((obj_t ** ovec, int *typep));
EXPORT	list_t	*cvtvpath	__PR((list_t *l));
EXPORT	BOOL	nowarn		__PR((char *name));
LOCAL	void	warn		__PR((char *, ...)) __printflike__(1, 2);
LOCAL	void	exerror		__PR((char *, ...)) __printflike__(1, 2);
LOCAL	obj_t	*_objlook	__PR((obj_t *table[], char *name, BOOL create));
EXPORT	obj_t	*objlook	__PR((char *name, BOOL create));
EXPORT	list_t	*objlist	__PR((char *name));
EXPORT	obj_t	*ssufflook	__PR((char *name, BOOL create));
EXPORT	BOOL	check_ssufftab	__PR((void));
LOCAL	void	prvar		__PR((obj_t * p));
LOCAL	void	ptree		__PR((obj_t * p, int n));
EXPORT	void	printtree	__PR((void));
EXPORT	void	probj		__PR((FILE *f, obj_t * o, int type));
EXPORT	void	prtree		__PR((void));
LOCAL	char	*typestr	__PR((int type));
LOCAL	void	printobj	__PR((FILE *f, obj_t ** ovec, int objcnt, int type, list_t * deplist, cmd_t * cmdlist));

/*
 *	parse the dependency file
 *
 *	<obj1> {<obj2> ...} : <dependency list>
 *	{	<cmdlist>}
 *
 *	<macro>= <macro value>
 *
 *	<macro> += <macro value>
 *
 *	<macro> :sh= <shell command>
 */
EXPORT void
parsefile()
{
	int	i, objcnt;
	int	type;
	obj_t	*ovec[MAXOBJS];
	list_t	*deplist;
	cmd_t	*cmdlist;

	if (DoWarn)
		error("Parsing file '%s'\n", mfname);

	if (Dmake > 0)
		error(">>>>>>>>>>>>>>>> Reading makefile '%s'\n", mfname);

	lineno = 1;
	col = 0;
	getch();
	while (lastc != EOF) {
		if (lastc == '\t') {
			/*
			 * Skip leading white space that follows a TAB.
			 */
			while (lastc != EOF && (lastc == ' ' || lastc == '\t'))
				getch();
			/*
			 * Abort if such a line is not empty or contains more
			 * than only a comment. This has the side effect that
			 * '^<TAB> include' will not work anymore.
			 */
			if (lastc != '\n') {
				exerror(
				"Unexpected <TAB> character followed by nonblank '%c' <%o>",
					lastc, lastc);
			}
			getch();	/* Eat up new line character */
			continue;	/* Continue with next line */
		}
		if ((objcnt = read_ovec(ovec, &type)) == 0)
			continue;

		if (objcnt < 0) {
			errmsgno(EX_BAD, "Objcount: %d Objects: ", -objcnt);
			for (i = 0; i < -objcnt; i++)
				error("'%s' ", ovec[i]->o_name);
			error("\n");
			exerror("Too many target items");
		}

#ifdef	XXX	/* Begin new code for include */
printf("objcnt: %d ovec[0]: %s lastc: '%c'\n", objcnt, ovec[0]->o_name, lastc);
for (i = 0; i < objcnt; i++)
	printf("name[%d] = %s ", i, ovec[i]->o_name);
printf("\n");
#endif
		if (lastc == '\n') {
			if (streql(ovec[0]->o_name, "include") ||
				streql(ovec[0]->o_name, "-include")) {
				for (i = 1; i < objcnt; i++) {
					doinclude(ovec[i]->o_name,
						ovec[0]->o_name[0] != '-');
				}
				/* XXX freeobj ??? */
				continue;
			}
			if (streql(ovec[0]->o_name, "export")) {
				for (i = 1; i < objcnt; i++) {
					doexport(ovec[i]->o_name);
				}
				/* XXX freeobj ??? */
				continue;
			}
			if (streql(ovec[0]->o_name, "readonly")) {
				for (i = 1; i < objcnt; i++) {
					ovec[i]->o_flags |= F_READONLY;
				}
				/* XXX freeobj ??? */
				continue;
			}
			/*
			 * Any other word on the beginning of a line
			 * that is not followed by a ':' or a '=' is an error.
			 */
		} /* end new code for include */

		if (lastc != ':' && lastc != '=') {
			errmsgno(EX_BAD, "Objcount: %d Objects: ", objcnt);
			for (i = 0; i < objcnt; i++)
				error("'%s' ", ovec[i]->o_name);
			error("\n");
			exerror("Missing : or =, got '%c' <%o>", lastc, lastc);
		}
		getch();
		deplist = getlist(&type);
		if (type == SHVAR)
			deplist = getshvar(&type);

		if (lastc == ';') {
			lastc = '\t';
			firstc = '\t';
		}

		/*
		 * We should test for type == COLON only but unfortunately,
		 * the makefilesystem contained some simple pattern rule
		 * definitions that are written as: tgt_suff = list.
		 * We only allow what has been used in the makefilesystem.
		 * This changed in late 1999, we should keep support
		 * for it for at least 5 years.
		 */
/*		if (basetype(type) == COLON) {*/
		if (basetype(type) == COLON ||
		    (basetype(type) == EQUAL && deplist &&
					ovec[0]->o_name[0] == '.')) {
			cmdlist = getcmd();
		} else {
			cmdlist = NULL;
		}
		while (lastc == '\n')
			getch();
		for (i = 0; i < objcnt; i++)
			define_obj(ovec[i], i, objcnt, type, deplist, cmdlist);

		if (Dmake > 0)
			printobj(stderr, ovec, objcnt, type, deplist, cmdlist);
	}
	if (Dmake > 0)
		error(">>>>>>>>>>>>>>>> End of  makefile '%s'\n", mfname);
}

/*
 * Add dependency and command lists to a node in the object tree.
 * Allow definitions to be overridden if the new definition in in
 * a different file.
 */
LOCAL void
define_obj(obj, n, objcnt, type, dep, cmd)
	register obj_t	*obj;
		int	n;
		int	objcnt;
		int	type;
	register list_t	*dep;
		cmd_t	*cmd;
{
	/*
	 * If we have a list of targets with the same dependency list,
	 * we copy the list structure to be able to separately
	 * append new elements to each of the targets.
	 */
	if (n > 0)
		dep = cplist(dep);

	if (objcnt == 1) {
		if (basetype(type) == COLON &&
		    strchr(obj->o_name, '%') != NULL) {
			define_patrule(obj, dep, cmd, type);
			return;
		}
		/*
		 * We should test for type == COLON too but unfortunately,
		 * the makefilesystem contained some simple pattern rule
		 * definitions that are written as: tgt_suff = list.
		 * This changed in late 1999, we should keep support
		 * for it for at least 5 years.
		 */
/*		if (type == COLON && && dep != NULL && cmd != NULL) {*/
		if (dep != NULL && cmd != NULL) {
			obj = check_ssuffrule(obj, dep);
		}
	} else {
		obj->o_flags |= F_MULTITARGET;
	}
	if (obj->o_type == 0)
		obj->o_type = type;

	/*
	 * Save first target that does not start with
	 * a dot in case no arguments have been supplied
	 * to make.
	 */
	if (n == 0 && !default_tgt && basetype(type) == COLON &&
	    (obj->o_name[0] != '.' || obj->o_name[1] == SLASH))
		default_tgt = obj;

	if (type == DCOLON)
		obj = define_dcolon(obj);

	/*
	 * XXX Probleme gibt es mit der Gleichbehandlung von ':' und '=' typen.
	 * XXX So definiert 'smake' MAKE_NAME= smake und später kommt evt.
	 * XXX smake: $(OBJLIST)
	 */


	if (obj->o_flags & F_READONLY)		/* Check for "read only" */
		return;
	obj->o_flags |= Mflags;			/* Add current global flags */

	/*
	 * Definition in new Makefile overrides existing definitions
	 */
	if ((obj->o_fileindex < MF_IDX_MAKEFILE || obj->o_type == EQUAL) &&
/*	if (((obj->o_fileindex != Mfileindex) || (obj->o_type == EQUAL)) &&*/
	    (type != ADDMAC) &&
	    !streql(obj->o_name, ".SUFFIXES")) {

		if (DoWarn)
			warn("'%s' RE-defined", obj->o_name);
		if (obj->o_fileindex == MF_IDX_ENVIRON) {
			obj->o_fileindex = Mfileindex;
			obj->o_type = type;
		}
		obj->o_list = dep;
		obj->o_cmd = cmd;
		return;
	}

	/*
	 * Add new definitions to list.
	 * Ignore definitions already in list.
	 */
	listappend(obj, dep);

	if (obj->o_cmd != (cmd_t *) NULL) {
		if (cmd != (cmd_t *) NULL) {
			warn("Multiple commands defined for '%s'", obj->o_name);
			/* XXX freelist()/freecmd() ??? */
		}
	} else {
		obj->o_cmd = cmd;
	}
}

/*
 * Define an intermediate target for Double Colon :: Rules
 *
 * a:: b
 *	cmd_b
 * a:: c
 *	cmd_c
 *
 * results in:
 * 	a::	::1@a ::2@a
 * and
 *	::1@a: b
 *		cmd_b
 *	::2@a: c
 *		cmd_c
 */
LOCAL obj_t *
define_dcolon(obj)
	register obj_t	*obj;
{
static	int	serial = 0;
	obj_t	*o;
	list_t	*l;
	list_t	*lo;
	char	_name[TYPICAL_NAMEMAX];
	char	*name = _name;
	size_t	namelen = sizeof (_name);
	char	*np = NULL;

	if (obj->o_namelen + 16 >= namelen) {
		namelen = obj->o_namelen + 16;
		name = np = __realloc(np, namelen, "dcolon target");
	}
	snprintf(name, namelen, "::%d@%s", ++serial, obj->o_name);
	o = objlook(name, TRUE);
	if (np)
		free(np);
	o->o_type = ':';
	o->o_flags |= F_DCOLON;		/* Mark as intermediate :: object */
	l = obj->o_list;
	lo = (list_t *) fastalloc(sizeof (list_t));
	lo->l_next = NULL;
	lo->l_obj = o;

	listappend(obj, lo);		/* Append this intermediate to list */

	o->o_node = obj;		/* Make this inter. obj aux to orig */
	return (o);			/* Return intermediate object */
}

/*
 * Append new definitions to dependencies in o_list, but
 * ignore them if they are already in the list.
 */
LOCAL void
listappend(obj, dep)
	register obj_t	*obj;
	register list_t	*dep;
{
	if (obj->o_list != (list_t *) NULL) {
		register list_t *l = obj->o_list;

		if (dep == NULL) {
			/*
			 * Allow to clear special targets.
			 */
			if (streql(obj->o_name, ".SUFFIXES") ||
			    streql(obj->o_name, ".DEFAULT") ||
			    streql(obj->o_name, ".NO_WARN") ||
			    streql(obj->o_name, ".SCCS_GET") ||
			    streql(obj->o_name, ".SPACE_IN_NAMES"))
				obj->o_list = NULL;
			return;
		}

		if (DoWarn)
			warn("'%s' ADD-defined", obj->o_name);
		/*
		 * if not already head of list, try to append ...
		 */
		if (l != dep) {
			while (l->l_next && l->l_next != dep)
				l = l->l_next;
			if (l->l_next == (list_t *) NULL)
				l->l_next = dep;
		}
	} else {
		obj->o_list = dep;
	}
}

patr_t	*Patrules;
patr_t	**pattail = &Patrules;

/*
 * Parse a pattern rule definition. This is a special type of implicit rules.
 */
LOCAL void
define_patrule(obj, dep, cmd, type)
	obj_t	*obj;
	list_t	*dep;
	cmd_t	*cmd;
	int	type;
{
	patr_t	*p;
	char	*s;
	register list_t *l;

	p = (patr_t *) fastalloc(sizeof (*p));

	p->p_name = obj;
	p->p_list = dep;
	p->p_cmd = cmd;
	s = strchr(obj->o_name, '%');
	if (s != NULL) {
		*s = '\0';
		p->p_tgt_prefix = strsave(obj->o_name);
		p->p_tgt_suffix = strsave(&s[1]);
		*s = '%';
	} else {		/* This can never happen! */
		p->p_tgt_prefix = strsave(Nullstr);
		p->p_tgt_suffix = strsave(obj->o_name);
	}
	p->p_tgt_pfxlen = strlen(p->p_tgt_prefix);
	p->p_tgt_suflen = strlen(p->p_tgt_suffix);

	for (l = dep; l; l = l->l_next) {
		if (strchr(l->l_obj->o_name, '%'))
			l->l_obj->o_flags |= F_PERCENT;
	}
	if (dep == NULL) {
		p->p_src_prefix = 0;
		p->p_src_suffix = 0;
		p->p_src_pfxlen = 0;
		p->p_src_suflen = 0;
	} else {
		s = strchr(dep->l_obj->o_name, '%');
		if (s != NULL) {
			*s = '\0';
			p->p_src_prefix = strsave(dep->l_obj->o_name);
			p->p_src_suffix = strsave(&s[1]);
			*s = '%';
		} else {
			p->p_src_prefix = strsave(Nullstr);
			p->p_src_suffix = strsave(dep->l_obj->o_name);
		}
		p->p_src_pfxlen = strlen(p->p_src_prefix);
		p->p_src_suflen = strlen(p->p_src_suffix);
	}
	p->p_flags = 0;
	if (type == DCOLON)
		p->p_flags |= PF_TERM;	/* Make it a termiator rule */
	*pattail = p;
	pattail = &p->p_next;
	p->p_next = 0;
}

/*
 * Print the complete list of pattern rules.
 */
LOCAL void
print_patrules(f)
	FILE	*f;
{
	patr_t	*p = Patrules;
	cmd_t	*c;

	while (p) {
		register list_t *l;

		fprintf(f, "%s%%%s:%s",
		p->p_tgt_prefix, p->p_tgt_suffix,
		(p->p_flags & PF_TERM) ? ":":"");
		
		for (l = p->p_list; l; l = l->l_next) {
			fprintf(f, " %s", l->l_obj->o_name);
		}
		printf("\n");

		for (c = p->p_cmd; c; c = c->c_next)
			fprintf(f, "\t%s\n", c->c_line);
		p = p->p_next;
	}
}

/*
 * Check for a simple suffix rule definition.
 * In case we found a simple suffix rule definition, return a new obj_t *.
 */
LOCAL obj_t *
check_ssuffrule(obj, dep)
	obj_t	*obj;
	list_t	*dep;
{
	register char	*name = obj->o_name;
	register list_t	*l;
	extern	 int	xssrules;

	/*
	 * Check if the first charater of the target is a '.' or
	 * if the target name equals "", but make sure this is
	 * not a vanilla target that just starts with "./".
	 */
	if ((name[0] == '.' && strchr(name, SLASH) == NULL) ||
	    (name[0] == '"' && name[1] == '"' && name[2] == '\0')) {

		/*
		 * All dependency names must start with a '.'
		 * and may not contain a SLASH.
		 */
		for (l = dep; l; l = l->l_next) {
			if (l->l_obj->o_name[0] != '.')
				return (obj);
			if (strchr(l->l_obj->o_name, SLASH) != NULL)
				return (obj);
		}
		obj = ssufflook(obj->o_name, TRUE);
		xssrules++;
	}
	return (obj);
}

/*
 * Get the macro value as in the form macro=value.
 */
EXPORT char *
get_var(name)
	char	*name;
{
	obj_t	*o = objlook(name, FALSE);

	if (o == (obj_t *)NULL)
		return ((char *)NULL);

	if (basetype(o->o_type) == EQUAL && o->o_list != NULL)
		return (o->o_list->l_obj->o_name);
	return ((char *)NULL);
}

/*
 * Define a macro as in the form macro=value.
 * Value may only be one word.
 */
EXPORT void
define_var(name, val)
	char	*name;
	char	*val;
{
	obj_t	*o;
	obj_t	*ov;
	list_t	*list;
	list_t	**tail = &list;

	o = objlook(name, TRUE);
	if (o->o_flags & F_READONLY)		/* Check for "read only" */
		return;
	o->o_flags |= Mflags;			/* Add current global flags */
	if (*val) {				/* Only if not empty */
		ov = objlook(val, TRUE);
		tail = listcat(ov, tail);
	}
	*tail = (list_t *) NULL;
	o->o_list = list;
	o->o_type = EQUAL;
}

#ifdef	NEEDED
/*
 * Define a macro as in the form macro=vallist.
 * Vallist may be a list of words that is white space separated in one string.
 */
EXPORT void
define_lvar(name, vallist)
	char	*name;
	char	*vallist;
{
	obj_t	*o;
	obj_t	*ov;
	list_t	*list;
	list_t	**tail = &list;

	o = objlook(name, TRUE);
	if (o->o_flags & F_READONLY)		/* Check for "read only" */
		return;
	o->o_flags |= Mflags;			/* Add current global flags */
	tail = mklist(vallist, tail, FALSE);
	*tail = (list_t *) NULL;
	o->o_list = list;
	o->o_type = EQUAL;
}

/*
 * Return a list_t pointer to the nth word in a macro value list.
 */
list_t *
varvaln(name, n)
	char	*name;
	int	n;
{
	obj_t	*o = objlook(name, FALSE);

	if (o)
		return (list_nth(o->o_list, n));
	return ((list_t *)0);
}
#endif	/* NEEDED */

#define	white(c)	(c == ' ' || c == '\t')

/*
 * Read a space separated word from current Makefile.
 * Used for target list. Stop at space, ':' '=' and ','.
 * Returns the length of the word.
 */
LOCAL int
getobjname()
{
	register int	n = 0;
	register char	*p = gbuf;
	register int	beg = 0;
	register int	end = 0;
	register int	nestlevel = 0;

	while (white(lastc))
		getch();			/* Skip white space. */
	if (lastc == '$') {
		switch (beg = peekch()) {

		case '(': end = ')'; break;
		case '{': end = '}'; break;

		default:
			beg = end = -1;
		}
	}
	while (lastc != EOF && (' ' < lastc || nestlevel > 0)) {
		if (lastc == beg)
			nestlevel++;
		else if (lastc == end)
			nestlevel--;
		if (nestlevel <= 0) {
			if (lastc == ':' || lastc == '=' || lastc == ',')
				break;
		}
#ifdef	__CHECK_NAMEMAX_IN_GETOBJNAME__
		if (n >= NAMEMAX - 2)
			exerror("Name more than %d chars long", NAMEMAX - 2);
#endif	/*  __CHECK_NAMEMAX_IN_GETOBJNAME__ */
		if (p >= gbufend)
			p = growgbuf(p);
		*p++ = lastc;
		if (nestlevel <= 0 && lastc == '\\') {
			if (white(peekch()) && objlist(".SPACE_IN_NAMES")) {
				getch();
				p--;
				*p++ = lastc;
			}
		}
		n++;
		getch();
	}
	*p = '\0';				/* Terminate with null char */
	return (n);
}

/*
 * Read a target file name.
 */
LOCAL obj_t
*getobj()
{
	return (getobjname() ? objlook(gbuf, TRUE) : (obj_t *) NULL);
}

/*
 * Read a space separated word from current Makefile.
 * General purpose version for dependency list.
 * Returns the length of the word.
 */
LOCAL int
getname(type)
	int	type;
{
	register int	n = 0;
	register char	*p = gbuf;
	register int	beg = 0;
	register int	end = 0;
	register int	nestlevel = 0;

	if (type == ':')
		type = ';';
	else
		type = '\0';

	while (white(lastc))
		getch();			/* Skip white space. */
	if (lastc == '$') {
		switch (beg = peekch()) {

		case '(': end = ')'; break;
		case '{': end = '}'; break;

		default:
			beg = end = -1;
		}
	}
	while (lastc != EOF && (' ' < lastc || nestlevel > 0)) {
		if (lastc == beg)
			nestlevel++;
		else if (lastc == end)
			nestlevel--;
		if (nestlevel <= 0) {
			if (lastc == type)
				break;
		}
#ifdef	__CHECK_NAMEMAX_IN_GETNAME__
		if (n >= NAMEMAX - 2)
			exerror("Name more than %d chars long", NAMEMAX - 2);
#endif	/* __CHECK_NAMEMAX_IN_GETNAME__ */
		if (p >= gbufend)
			p = growgbuf(p);
		*p++ = lastc;
		if (nestlevel <= 0 && lastc == '\\') {
			if (white(peekch()) && objlist(".SPACE_IN_NAMES")) {
				getch();
				p--;
				*p++ = lastc;
			}
		}
		n++;
		getch();
	}
	*p = '\0';				/* Terminate with null char */
	return (n);
}

/*
 * Read a dependency file name.
 */
LOCAL obj_t *
getnam(type)
	int	type;
{
	return (getname(type) ? objlook(gbuf, TRUE) : (obj_t *) NULL);
}

/*
 * Reads a line from current Makefile.
 * Returns the length of the string.
 */
LOCAL int
getln()
{
	register int	n = 0;
	register char	*p = gbuf;

	while (white(lastc))
		getch();			/* Skip white space. */
	while (lastc != EOF && lastc != '\n') {
#ifdef	__CHECK_NAMEMAX_IN_GETLN__
		if (n >= NAMEMAX - 2) {
			exerror("Line more than %d chars long", NAMEMAX - 2);
		}
#endif /* __CHECK_NAMEMAX_IN_GETLN__ */
		if (p >= gbufend)
			p = growgbuf(p);
		*p++ = lastc;
		n++;
		getch();
	}
	*p = '\0';				/* Terminate with null char */
	return (n);
}

/*
 * Add one new object to the end of a list of objects.
 */
LOCAL list_t **
listcat(obj, tail)
	obj_t	*obj;
	list_t	**tail;
{
	list_t	*item;

	*tail = item = (list_t *) fastalloc(sizeof (list_t));
	item->l_obj = obj;
	tail = &item->l_next;
	return (tail);
}

/*
 * Make a list of objects by parsing a line
 * and cutting it at whitespaces.
 */
LOCAL list_t **
mklist(line, tail, doexpand)
	register char	*line;
	register list_t	**tail;
		BOOL	doexpand;
{
	register obj_t	*o;
	register char	*p;

/*printf("Line: '%s'\n", line);*/

	for (p = line; *p; ) {
		while (*p && white(*p))
			p++;
		line = p;
		while (*p) {
			if (white(*p)) {
				*p++ = '\0';
				if (*line)
					break;
			}
			p++;
		}
/*printf("line: '%s'\n", line);*/
		/*
		 * If the list ends with white space, we will see
		 * an empty string at the end of the list unless
		 * we apply this test.
		 */
		if (*line == '\0')
			break;
		o = objlook(line, TRUE);
		if (doexpand)
			tail = exp_list(o, tail);
		else
			tail = listcat(o, tail);
	}
	return (tail);
}

/*
 * Copy a list.
 * Generate new list pointers and re-use the old obj pointers.
 */
LOCAL list_t *
cplist(l)
	list_t	*l;
{
		list_t	*list;
	register list_t	**tail = &list;

	while (l) {
		tail = listcat(l->l_obj, tail);
		l = l->l_next;
	}
	*tail = (list_t *) NULL;
	return (list);
}

/*
 * Expand one object name using make macro definitions.
 * Add the new objects to the end of a list of objects.
 */
LOCAL list_t **
exp_list(o, tail)
	obj_t	*o;
	list_t	**tail;
{
	char	*name;
	char	*xname;

	name = o->o_name;
	if (strchr(name, '$')) {
		xname = substitute(name, NullObj, 0, 0);
		if (streql(name, xname)) {
			printf("eql: %s\n", name);
			tail = listcat(o, tail);
		} else {
/*printf("Sxname: %s <- %s\n", xname, name);*/
			tail = mklist(xname, tail, FALSE);
		}
	} else {
		tail = listcat(o, tail);
	}
	*tail = (list_t *) NULL;
	return (tail);
}

#ifdef	NEEDED
/*
 * Expand a list of object names using make macro definitions.
 * Add the new objects to the end of a list of objects.
 */
LOCAL list_t *
exp_olist(l)
	list_t	*l;
{
	list_t	*list;
	list_t	**tail = &list;

	while (l) {
		tail = exp_list(l->l_obj, tail);
		l = l->l_next;
	}
	*tail = (list_t *) NULL;
	return (list);
}
#endif	/* NEEDED */

/*
 * Read a list of dependency file names.
 */
LOCAL list_t *
getlist(typep)
	int	*typep;
{
	list_t	*list;
	list_t	**tail = &list;
	obj_t	*o;
	int	type = basetype(*typep);
	BOOL	first = TRUE;

	if (type == '=') {
		int n;
#ifdef	nono
		char *p;
#endif

		n = getln();

#ifdef	nono	/* Do not kill trailing whitespace !!! */

		p = &gbuf[n-1];
		while (white(*p))
			p--;
		*++p = '\0';
#endif
		/*
		 * Only add to list if right hand side is not completely white.
		 */
		if (n) {
			o = objlook(gbuf, TRUE);
			if (*typep == ASSIGN)
				tail = exp_list(o, tail);
			else
				tail = listcat(o, tail);
		}
	} else while ((o = getnam(type)) != (obj_t *) NULL) {
		if (type == '=') {
			tail = listcat(o, tail);
		} else {
			if (first) {
				first = FALSE;
				if (is_shvar(&o, typep, &tail))
					break;
			}
			tail = exp_list(o, tail);
		}
	}
	*tail = (list_t *) NULL;
	return (list);
}

/*
 * Check if a definition for immediate shell expansion follows.
 */
LOCAL BOOL
is_shvar(op, typep, tailp)
	obj_t	**op;
	int	*typep;
	list_t	***tailp;
{
	obj_t	*o = *op;

	if (streql(o->o_name, "sh=")) {
		*typep = SHVAR;
		return (TRUE);
	}
	if (streql(o->o_name, "sh")) {
		if ((o = getnam(*typep)) == (obj_t *)NULL) {
			return (FALSE);
		}
		if (streql(o->o_name, "=")) {
			*typep = SHVAR;
			return (TRUE);
		} else {
			*tailp = exp_list(*op, *tailp);
			*op = o;
		}
	}
	return (FALSE);
}

/*
 * read a line and expand by shell
 */
LOCAL list_t *
getshvar(typep)
	int	*typep;
{
		list_t	*list;
	register list_t	**tail = &list;
		char	*p;

	*typep = '=';
	getln();
	p = shout(gbuf);

	tail = mklist(p, tail, FALSE);
	*tail = (list_t *) NULL;
	return (list);
}

/*
 * Read a list of command lines that follow a dependency list.
 */
LOCAL cmd_t *
getcmd()
{
		cmd_t	*list;
	register cmd_t	*item, **tail = &list;
	register char	*p;

	setincmd(TRUE);			/* Switch reader to command mode */
	if (lastc == '\n')		/* Not a ';' command type	 */
		getch();

	while (lastc != EOF && (firstc == '\t' || firstc == '#')) {
		/*
		 * We handle comment in command lines as in old UNIX 'make' and
		 * not as required by POSIX. A command line that starts with
		 * a '#' (the following code) is handled as usual.
		 * A '#' inside a command line and escaped newlines are
		 * forwarded to the shell. This is needed to allow something
		 * like cc -# with SunPRO C.
		 */
		if (firstc == '#') {
			skipline();	/* Skip commented out line	*/
			getch();	/* Skip newline character.	*/
			continue;
		}
		while (white(lastc))
			getch();	/* Skip white space. */

		for (p = gbuf; lastc != EOF; getch()) {
			if (lastc == '\n' && p[-1] != '\\')
				break;
			if (p >= gbufend)
				p = growgbuf(p);
			*p++ = lastc;
		}
		*p = '\0';
		*tail = item = (cmd_t *) fastalloc(sizeof (cmd_t));
		item->c_line = strsave(gbuf);
		tail = &item->c_next;

		if (lastc == '\n')	/* Skip newline character. */
			getch();
	}
/*printf("getcmd: lastc: %c %CX '%.20s'\n", lastc, lastc, readbufp);*/
	setincmd(FALSE);
	*tail = (cmd_t *) NULL;
	return (list);
}

/*
 * Expand a list of target names using make macro definitions.
 */
LOCAL int
exp_ovec(ovec, objcnt)
	obj_t	*ovec[];
	int	objcnt;
{
		list_t	*list;
		list_t	*l;
	register list_t	**tail = &list;
	int	i;

	if (objcnt == 0)
		return (0);
	/*
	 * Catch easy case.
	 */
	if (objcnt == 1 && (strchr(ovec[0]->o_name, '$') == NULL))
		return (objcnt);

	for (i = 0; i < objcnt; i++) {
		tail = exp_list(ovec[i], tail);
		*tail = (list_t *) NULL;
	}

	for (i = 0; list; i++) {
		if (i >= MAXOBJS) {
			warn("Too many expanded target items");
			return (-i);
		}
		ovec[i] = list->l_obj;
		l = list;
		list = list->l_next;
		fastfree((char *)l, sizeof (*l));
	}
	return (i);
}

/*
 * Read a list of target names.
 */
LOCAL int
read_ovec(ovec, typep)
	obj_t	*ovec[];
	int	*typep;
{
	obj_t	*o;
	int	objcnt;
	char	*p;
	int	c;

/*printf("read_ovec\n");*/
	for (objcnt = 0; lastc != EOF; ) {
		o = getobj();
		c = lastc;
		while (white(lastc))
			getch();
/*printf("lastc: '%c'", lastc); if (o) printf("nameL %s\n", o->o_name);*/
		if (o != (obj_t *) NULL) {
			p = o->o_name;
			if (p[0] == '+' && p[1] == '\0' && c == '=') {
				*typep = ADDMAC;
				break;
			}
			if (objcnt >= MAXOBJS) {
				warn("Too many target items");
				return (-objcnt);
			}
			ovec[objcnt++] = o;
		} else {
			if (lastc == '\n') {
				getch();
				continue;
			}
			if (lastc == EOF && objcnt == 0)
				break;
			exerror("Missing object name");
		}
		/*
		 * end of definition:
		 * colon, equal or end of line
		 */
		if (lastc == ':' && peekch() == ':') {
			getch();
			*typep = DCOLON;
			break;
		}
		if (lastc == ':' && peekch() == '=') {
			getch();
			*typep = ASSIGN;
			if (!nowarn(":=")) {
				warn(
				"Nonportable ':=' assignement found for macro '%s'.\n",
				o->o_name);
			}
			break;
		}
		if (lastc == ':' || lastc == '=' || lastc == '\n') {
			*typep = lastc;
			break;
		}
		if (lastc == ',')
			getch();
	}
	/*
	 * XXX Achtung: UNIX make expandiert alles was links und rechts von ':'
	 * steht, sowie Variablennamen (links von '=').
	 */
	objcnt = exp_ovec(ovec, objcnt);

	return (objcnt);
}

/*
 *
 */
EXPORT list_t *
cvtvpath(l)
	list_t	*l;
{
	list_t	*lsave;
	list_t	*list = (list_t *)0;
	list_t	**tail = &list;
	char	vpath[NAMEMAX];
	char	*p1, *p2;

	if (l != NULL) {
		for (p1 = l->l_obj->o_name, p2 = vpath; *p1; p2++) {
			if ((*p2 = *p1++) == ':')
				*p2 = ' ';
		}
		*p2 = '\0';
		tail = mklist(vpath, tail, TRUE);
		*tail = (list_t *) NULL;
		lsave = l = list;

		tail = &list;

		for (; l; l = l->l_next) {
			tail = listcat(l->l_obj, tail);
			tail = listcat(l->l_obj, tail);
		}
		*tail = (list_t *) NULL;
		freelist(lsave);
	}
	if (DoWarn)
		error("VPATH but no .SEARCHLIST\n");
	return (list);
}


EXPORT BOOL
nowarn(name)
	char	*name;
{
	list_t	*l;

	for (l = objlist(".NO_WARN"); l != NULL; l = l->l_next) {
		if (streql(l->l_obj->o_name, name))
			return (TRUE);
	}
	return (FALSE);
}

/*
 * NOTE: as long as warn() and exerror() use the global vars
 * lineno, col and Mfileindex,
 * we cannot use them at any other time than parse time.
 */

/*
 * Print a warning with text, line number, column and filename.
 */

/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL void
warn(char *msg, ...)
#else
LOCAL void
warn(msg, va_alist)
	char	*msg;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	if (!NoWarn)
		errmsgno(EX_BAD,
			"WARNING: %r in line %d col %d of '%s'\n", msg, args,
				lineno, col, mfname);
	va_end(args);
}

/*
 * Print an error message with text, line number, column and filename, then exit.
 */

/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL void
exerror(char *msg, ...)
#else
LOCAL void
exerror(msg, va_alist)
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	len;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	errmsgno(EX_BAD, "%r in line %d col %d of '%s'\n", msg, args,
			lineno, col, mfname);
	va_end(args);

	len = getrdbufsize();
	if (len > 80)
		len = 80;
	if (Debug > 0)
		errmsgno(EX_BAD, "Current read buffer is: '%.*s'\n", len, getrdbuf());
	comerrno(EX_BAD, "Bad syntax in '%s'.\n", mfname);
}


#define	MyObjTabSize	128	/* # of Hash table entries (power of two) .*/

#define	ObjHash(name) (name[0] & (MyObjTabSize - 1))

LOCAL	obj_t	*ObjTab[MyObjTabSize];
LOCAL	obj_t	*SuffTab[MyObjTabSize];
EXPORT	obj_t	*NullObj;

/*
 * Look up name in 'table'.
 * First look up in hash table then do binary search.
 */
LOCAL obj_t *
_objlook(table, name, create)
	obj_t	*table[];
	char	*name;
	BOOL	create;
{
	register obj_t	**pp;
	register obj_t	*p;
	register char	*new;
	register char	*old;

	for (pp = &table[ObjHash(name)]; (p = *pp) != NULL; ) {
		for (new = name, old = p->o_name; *new++ == *old; ) {
			if (*old++ == '\0')	/* Found 'name' */
				return (p);
		}
		if (*--new < *old)
			pp = &p->o_left;
		else
			pp = &p->o_right;
	}
	if (!create)
		return ((obj_t *) NULL);

	/*
	 * Add new entry to ObjTab.
	 */
	*pp = p = (obj_t *) fastalloc(sizeof (obj_t));	/* insert into list */
	p->o_left = p->o_right = (obj_t *) NULL;	/* old 'p' was NULL */
	p->o_cmd = (cmd_t *) NULL;
	p->o_list = (list_t *) NULL;
	p->o_date = NOTIME;
	p->o_level = MAXLEVEL;
	p->o_type = 0;
	p->o_flags = 0;
	p->o_fileindex = Mfileindex;
	p->o_name = strsave(name);
	p->o_namelen = strlen(name);
	p->o_node = NULL;
	return (p);
}

/*
 * Look up name in ObjTab.
 */
EXPORT obj_t *
objlook(name, create)
	char	*name;
	BOOL	create;
{
	return (_objlook(ObjTab, name, create));
}

EXPORT list_t *
objlist(name)
	char	*name;
{
	obj_t	*o;

	if ((o = objlook(name, FALSE)) == NULL)
		return ((list_t *)NULL);
	return (o->o_list);
}

/*
 * Look up name in SuffTab.
 */
EXPORT obj_t *
ssufflook(name, create)
	char	*name;
	BOOL	create;
{
	return (_objlook(SuffTab, name, create));
}

/*
 * Check is SuffTab contains any entry.
 */
EXPORT BOOL
check_ssufftab()
{
	int	i;

	for (i = 0; i < MyObjTabSize; i++)
		if (SuffTab[i])
			return (TRUE);
	return (FALSE);
}

/*
 * Used by ptree() to print one single object.
 */
LOCAL void
prvar(p)
	register obj_t	*p;
{
	register list_t	*q;
	register cmd_t	*c;

	if (!p)
		return;

	error("%s:", p->o_name);
	for (q = p->o_list; q; q = q->l_next)
		error(" %s", q->l_obj->o_name);
	putc('\n', stderr);
	for (c = p->o_cmd; c; c = c->c_next)
		error("\t...%s\n", c->c_line);
}

/*
 * Currently only used by printtree() to implement the -probj option.
 */
LOCAL void
ptree(p, n)
	register obj_t	*p;
		int	n;
{
	register int	i;

	for (; p; p = p->o_right) {
		ptree(p->o_left, ++n);
		for (i = 1; i < n; i++)
			putc(' ', stderr);
		prvar(p);
	}
}

/*
 * Used by -probj Flag
 */
EXPORT void
printtree()
{
	int	i;

	print_patrules(stderr);
	for (i = 0; i < MyObjTabSize; i++)
		ptree(ObjTab[i], 0);
}

/*
 * Currently only used by prtree() to implement the -p option.
 */
EXPORT void
probj(f, o, type)
	FILE	*f;
	obj_t	*o;
	int	type;
{
	for (; o; o = o->o_right) {
		probj(f, o->o_left, type);
		if (type >= 0 && type != o->o_type)
			continue;
		if (type < 0 && (o->o_type == ':' || o->o_type == '='))
			continue;
		if (Debug <= 0 && o->o_type == 0)
			continue;	/* Ommit target only strings */

		printobj(f, &o, 1, o->o_type, o->o_list, o->o_cmd);
	}
}

/*
 * Used by -p Flag, called from main().
 */
EXPORT void
prtree()
{
	int	i;

	printf("# Implicit Pattern Rules:\n");
	print_patrules(stdout);
	printf("# Implicit Suffix Rules:\n");
	for (i = 0; i < MyObjTabSize; i++) {
		probj(stdout, ObjTab[i], ':');
	}
	printf("# Simple Suffix Rules:\n");
	for (i = 0; i < MyObjTabSize; i++) {
		probj(stdout, SuffTab[i], ':');
	}
	printf("# Macro definitions:\n");
	for (i = 0; i < MyObjTabSize; i++) {
		probj(stdout, ObjTab[i], '=');
	}
	printf("# Various other definitions:\n");
	for (i = 0; i < MyObjTabSize; i++) {
		probj(stdout, ObjTab[i], -1);
	}
}

/*
 * Convert the object type into a human readable string.
 */
LOCAL char *
typestr(type)
	int	type;
{
	if (type == 0)
		return ("(%)");
	if (type == EQUAL)
		return ("=");
	if (type == COLON)
		return (":");
	if (type == DCOLON)
		return ("::");
	if (type == SEMI)
		return (";");
	if (type == ADDMAC)
		return ("+=");
	if (type == ASSIGN)
		return (":=");
	if (type == SHVAR)
		return (":sh=");
	return ("UNKNOWN TYPE");
}

/*
 * This is the central routine to print an object.
 */
LOCAL void
printobj(f, ovec, objcnt, type, deplist, cmdlist)
	FILE	*f;
	obj_t	*ovec[];
	int	objcnt;
	int	type;
	list_t	*deplist;
	cmd_t	*cmdlist;
{
	register	list_t	*l;
	register	cmd_t	*c;
	register	int	i;

	for (i = 0; i < objcnt; i++)
		fprintf(f, "%s ", ovec[i]->o_name);
	fprintf(f, "%2s\t", typestr(type));

	for (l = deplist; l; l = l->l_next)
		fprintf(f, "%s ", l->l_obj->o_name);
	fprintf(f, "\n");

	for (c = cmdlist; c; c = c->c_next)
		fprintf(f, "\t%s\n", c->c_line);
	fflush(f);
}
