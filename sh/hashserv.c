/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
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

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#pragma ident	"@(#)hashserv.c	1.14	06/06/16 SMI"

/*
 * This file contains modifications Copyright 2008 J. Schilling
 *
 * @(#)hashserv.c	1.5 08/03/28 2008 J. Schilling
 */
#ifndef lint
static	char sccsid[] =
	"@(#)hashserv.c	1.5 08/03/28 2008 J. Schilling";
#endif

/*
 *	UNIX shell
 */

#include	"defs.h"
#include	"hash.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>

#define		EXECUTE		01

static unsigned char	cost;
static int	dotpath;
static int	multrel;
static struct entry	relcmd;

	short	pathlook	__PR((unsigned char *com, int flg, struct argnod *arg));
static void	zapentry	__PR((ENTRY *h));
	void	zaphash		__PR((void));
	void	zapcd		__PR((void));
static void	hashout		__PR((ENTRY *h));
	void	hashpr		__PR((void));
	void	set_dotpath	__PR((void));
	void	hash_func	__PR((unsigned char *name));
	void	func_unhash	__PR((unsigned char *name));
	short	hash_cmd	__PR((unsigned char *name));
	int	what_is_path	__PR((unsigned char *name));
static	int	findpath	__PR((unsigned char *name, int oldpath));
	int	chk_access	__PR((unsigned char *name, mode_t mode, int regflag));
static void	pr_path		__PR((unsigned char *name, int count));
static int	argpath		__PR((struct argnod *arg));

short
pathlook(com, flg, arg)
	unsigned char	*com;
	int		flg;
	struct argnod	*arg;
{
	unsigned char	*name = com;
	ENTRY		*h;

	ENTRY		hentry;
	int		count = 0;
	int		i;
	int		pathset = 0;
	int		oldpath = 0;
	struct namnod	*n;



	hentry.data = 0;

	if (any('/', name))
		return(COMMAND);

	h = hfind(name);


	if (h)
	{
		if (h->data & (BUILTIN | FUNCTION))
		{
			if (flg)
				h->hits++;
			return(h->data);
		}

		if (arg && (pathset = argpath(arg)))
			return(PATH_COMMAND);

		if ((h->data & DOT_COMMAND) == DOT_COMMAND)
		{
			if (multrel == 0 && hashdata(h->data) > dotpath)
				oldpath = hashdata(h->data);
			else
				oldpath = dotpath;

			h->data = 0;
			goto pathsrch;
		}

		if (h->data & (COMMAND | REL_COMMAND))
		{
			if (flg)
				h->hits++;
			return(h->data);
		}

		h->data = 0;
		h->cost = 0;
	}

	if (i = syslook(name, commands, no_commands))
	{
		hentry.data = (BUILTIN | i);
		count = 1;
	}
	else
	{
		if (arg && (pathset = argpath(arg)))
			return(PATH_COMMAND);
pathsrch:
			count = findpath(name, oldpath);
	}

	if (count > 0)
	{
		if (h == 0)
		{
			hentry.cost = 0;
			hentry.key = make(name);
			h = henter(hentry);
		}

		if (h->data == 0)
		{
			if (count < dotpath)
				h->data = COMMAND | count;
			else
			{
				h->data = REL_COMMAND | count;
				h->next = relcmd.next;
				relcmd.next = h;
			}
		}


		h->hits = flg;
		h->cost += cost;
		return(h->data);
	}
	else 
	{
		return(-count);
	}
}


static void
zapentry(h)
	ENTRY *h;
{
	h->data &= HASHZAP;
}

void
zaphash()
{
	hscan(zapentry);
	relcmd.next = 0;
}

void 
zapcd()
{
	ENTRY *ptr = relcmd.next;

	while (ptr)
	{
		ptr->data |= CDMARK;
		ptr = ptr->next;
	}
	relcmd.next = 0;
}


static void
hashout(h)
	ENTRY *h;
{
	sigchk();

	if (hashtype(h->data) == NOTFOUND)
		return;

	if (h->data & (BUILTIN | FUNCTION))
		return;

	prn_buff(h->hits);

	if (h->data & REL_COMMAND)
		prc_buff('*');


	prc_buff(TAB);
	prn_buff(h->cost);
	prc_buff(TAB);

	pr_path(h->key, hashdata(h->data));
	prc_buff(NL);
}

void
hashpr()
{
	prs_buff(_gettext("hits	cost	command\n"));
	hscan(hashout);
}

void
set_dotpath()
{
	unsigned char	*path;
	int		cnt = 1;

	dotpath = 10000;
	path = getpath((unsigned char *)"");

	while (path && *path)
	{
		if (*path == '/')
			cnt++;
		else
		{
			if (dotpath == 10000)
				dotpath = cnt;
			else
			{
				multrel = 1;
				return;
			}
		}

		path = nextpath(path);
	}

	multrel = 0;
}

void
hash_func(name)
	unsigned char	*name;
{
	ENTRY	*h;
	ENTRY	hentry;

	h = hfind(name);

	if (h)
		h->data = FUNCTION;
	else
	{
		hentry.data = FUNCTION;
		hentry.key = make(name);
		hentry.cost = 0;
		hentry.hits = 0;
		henter(hentry);
	}
}

void
func_unhash(name)
	unsigned char	*name;
{
	ENTRY 	*h;
	int i;

	h = hfind(name);

	if (h && (h->data & FUNCTION)) {
		if(i = syslook(name, commands, no_commands))
			h->data = (BUILTIN|i);
		else
			h->data = NOTFOUND;
	}
}


short
hash_cmd(name)
	unsigned char *name;
{
	ENTRY	*h;

	if (any('/', name))
		return(COMMAND);

	h = hfind(name);

	if (h)
	{
		if (h->data & (BUILTIN | FUNCTION))
			return(h->data);
		else if ((h->data & REL_COMMAND) == REL_COMMAND)
		{ /* unlink h from relative command list */
			ENTRY *ptr = &relcmd;
			while(ptr-> next != h)
				ptr = ptr->next;
			ptr->next = h->next;
		}
		zapentry(h);
	}

	return (pathlook(name, 0, (struct argnod *)0));
}


/*
 * Return 0 if found, 1 if not.
 */
int
what_is_path(name)
	unsigned char	*name;
{
	ENTRY	*h;
	int	cnt;
	short	hashval;

	h = hfind(name);

	prs_buff(name);
	if (h)
	{
		hashval = hashdata(h->data);

		switch (hashtype(h->data))
		{
			case BUILTIN:
				prs_buff(_gettext(" is a shell builtin\n"));
				return (0);

			case FUNCTION:
			{
				struct namnod *n = lookup(name);

				prs_buff(_gettext(" is a function\n"));
				prs_buff(name);
				prs_buff((unsigned char *)"(){\n");
				prf((struct trenod *)n->namenv);
				prs_buff((unsigned char *)"\n}\n");
				return (0);
			}

			case REL_COMMAND:
			{
				short hash;

				if ((h->data & DOT_COMMAND) == DOT_COMMAND)
				{
					hash = pathlook(name, 0, (struct argnod *)0);
					if (hashtype(hash) == NOTFOUND)
					{
						prs_buff(_gettext(" not found\n"));
						return (1);
					}
					else
						hashval = hashdata(hash);
				}
			}

			case COMMAND:					
				prs_buff(_gettext(" is hashed ("));
				pr_path(name, hashval);
				prs_buff((unsigned char *)")\n");
				return (0);
		}
	}

	if (syslook(name, commands, no_commands))
	{
		prs_buff(_gettext(" is a shell builtin\n"));
		return (0);
	}

	if ((cnt = findpath(name, 0)) > 0)
	{
		prs_buff(_gettext(" is "));
		pr_path(name, cnt);
		prc_buff(NL);
		return (0);
	}
	else
	{
		prs_buff(_gettext(" not found\n"));
		return (1);
	}
}

static int
findpath(name, oldpath)
	unsigned char	*name;
	int		oldpath;
{
	unsigned char 	*path;
	int	count = 1;

	unsigned char	*p;
	int	ok = 1;
	int 	e_code = 1;

	cost = 0;
	path = getpath(name);

	if (oldpath)
	{
		count = dotpath;
		while (--count)
			path = nextpath(path);

		if (oldpath > dotpath)
		{
			catpath(path, name);
			p = curstak();
			cost = 1;

			if ((ok = chk_access(p, S_IEXEC, 1)) == 0)
				return(dotpath);
			else
				return(oldpath);
		}
		else 
			count = dotpath;
	}

	while (path)
	{
		path = catpath(path, name);
		cost++;
		p = curstak();

		if ((ok = chk_access(p, S_IEXEC, 1)) == 0)
			break;
		else
			e_code = max(e_code, ok);

		count++;
	}

	return(ok ? -e_code : count);
}

/*
 * Determine if file given by name is accessible with permissions
 * given by mode.
 * Regflag argument non-zero means not to consider 
 * a non-regular file as executable. 
 */
#ifdef	PROTOTYPES
int
chk_access(unsigned char *name, mode_t mode, int regflag)
#else
int
chk_access(name, mode, regflag)
	unsigned char	*name;
	mode_t		mode;
	int		regflag;
#endif
{	
	static int flag;
	static uid_t euid; 
	struct stat statb;
	mode_t ftype;
	
	if(flag == 0) {
		euid = geteuid();
		flag = 1;
	}
	ftype = statb.st_mode & S_IFMT;
	if (stat((char *)name, &statb) == 0) {
		ftype = statb.st_mode & S_IFMT;
		if(mode == S_IEXEC && regflag && ftype != S_IFREG)
			return(2);
#ifdef	HAVE_ACCESS_E_OK
		if(access((char *)name, 010|(mode>>6)) == 0) {
#else
		if(access((char *)name, (mode>>6)) == 0) {
#endif
			if(euid == 0) {
				if (ftype != S_IFREG || mode != S_IEXEC)
					return(0);
		    		/* root can execute file as long as it has execute 
			   	permission for someone */
				if (statb.st_mode & (S_IEXEC|(S_IEXEC>>3)|(S_IEXEC>>6)))
					return(0);
				return(3);
			}
			return(0);
		}
	}
	return(errno == EACCES ? 3 : 1);
}

static void
pr_path(name, count)
	unsigned char	*name;
	int		count;
{
	unsigned char	*path;

	path = getpath(name);

	while (--count && path)
		path = nextpath(path);

	catpath(path, name);
	prs_buff(curstak());
}


static int
argpath(arg)
	struct argnod	*arg;
{
	unsigned char 	*s;
	unsigned char	*start;

	while (arg)
	{
		s = arg->argval;
		start = s;

		if (letter(*s))		
		{
			while (alphanum(*s))
				s++;

			if (*s == '=')
			{
				*s = 0;

				if (eq(start, pathname))
				{
					*s = '=';
					return(1);
				}
				else
					*s = '=';
			}
		}
		arg = arg->argnxt;
	}

	return(0);
}