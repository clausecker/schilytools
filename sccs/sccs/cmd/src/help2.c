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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2007 J. Schilling
 *
 * @(#)help2.c	1.3 07/01/10 J. Schilling
 */
#if defined(sun) || defined(__GNUC__)

#ident "@(#)help2.c 1.3 07/01/10 J. Schilling"
#endif
/*
 * @(#)help2.c 1.10 06/12/12
 */

#ident	"@(#)help2.c"
#ident	"@(#)sccs:cmd/help2.c"
#include	<defines.h>
#include	<i18n.h>
#include	<ccstypes.h>

/*
	Program to locate helpful info in an ascii file.
	The program accepts a variable number of arguments.

	I18N changes are as follows:

	First determine the appropriate directory to search for help text 
	files corresponding to the user's locale.  Consistent with the
	setlocale() routine algorithm, environment variables are here 
	searched in the following order of descending priority: LC_ALL, 
	LC_MESSAGES, LANG.  The first one found to be defined is used
	as the locale for help messages.  If this directory does in fact
	not exist, an error is produced and the command exits.

	The file to be searched is determined from the argument. If the
	argument does not contain numerics, the search 
	will be attempted on '/usr/ccs/lib/help/cmds', with the search key
	being the whole argument.
	If the argument begins with non-numerics but contains
	numerics (e.g, zz32) the file /usr/ccs/lib/help/helploc 
	will be checked for a file corresponding to the non numeric prefix,
	That file will then be seached for the mesage. If 
	/usr/ccs/lib/help/helploc
	does not exist or the prefix is not found there the search will
	be attempted on '/usr/ccs/lib/help/<non-numeric prefix>', 
	(e.g,/usr/ccs/lib/help/zz), with the search key being <remainder of arg>, 
	(e.g., 32).
	If the argument is all numeric, or if the file as
	determined above does not exist, the search will be attempted on
	'/usr/ccs/lib/help/default' with the search key being
	the entire argument.
	In no case will more than one search per argument be performed.

	File is formatted as follows:

		* comment
		* comment
		-str1
		text
		-str2
		text
		* comment
		text
		-str3
		text

	The "str?" that matches the key is found and
	the following text lines are printed.
	Comments are ignored.

	If the argument is omitted, the program requests it.
*/

#ifdef	PROTOTYPES
#define HELPLOC INS_BASE "/ccs/lib/help/helploc"
#else
#define HELPLOC "/usr/ccs/lib/help/helploc"
#endif

/* char	dftfile[]   =   "/usr/ccs/lib/help/default"; */
/* char	helpdir[]   =   "/usr/ccs/lib/help/"; */

static char dftfile[] = NOGETTEXT("/default");
#ifdef	PROTOTYPES
static char helpdir[] = NOGETTEXT(INS_BASE "/ccs/lib/help/locale/");
#else
static char helpdir[] = NOGETTEXT("/usr/ccs/lib/help/locale/");
#endif

char	SccsError[MAXERRORLEN];
static char	hfile[256];
struct	stat	Statbuf;
static FILE	*iop;
static char	line [MAXLINE+1];
static char   *locale = NULL; /* User's locale. */

	int	main __PR((int argc, char **argv));
static int	findprt __PR((char *p));
static char *	ask __PR((void));
static int	lochelp __PR((char *ky, char *fi));


int
main(argc,argv)
int argc;
char *argv[];
{
	register int i;
	int numerrs=0;
#ifdef	PROTOTYPES
	char default_locale[] = NOGETTEXT("C"); /* Default English. */
#else
	char *default_locale = NOGETTEXT("C"); /* Default English. */
#endif
        char help_dir[200]; /* Directory to search for help text. */

	/*
	 * Set locale for all categories.
	 */
	setlocale(LC_ALL, NOGETTEXT(""));

	/*
	 * Returns the locale value for the LC_MESSAGES category.  This
	 * will be used to set the path to retrieve the appropriate
	 * help files in "/.../help.d/locale/<locale>".
	 */
	locale = setlocale(LC_MESSAGES, NOGETTEXT(""));
 	if (locale == NULL) {
 	   locale = default_locale;
 	}

	/* 
	 * Set directory to search for general l10n SCCS messages.
	 */
#ifdef	PROTOTYPES
	bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	   NOGETTEXT(INS_BASE "/ccs/lib/locale/"));
#else
	bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	   NOGETTEXT("/usr/ccs/lib/locale/"));
#endif
	textdomain(NOGETTEXT("SUNW_SPRO_SCCS"));

	Fflags = FTLMSG;
	
	/* 
	 * Set directory to search for SCCS help text, not general
	 * message text wrapped with "gettext()".
	 */
	strcpy(help_dir, helpdir);
	strcat(help_dir, locale);

       /*
	* The text of the printf statement below should not be wrapped
	* with gettext().  Since we don't know what the locale is, we
	* don't know how to get the proper translation text.
	*/
	if (stat(help_dir, &Statbuf) != 0) { /* Does help directory exist? */
	        printf(NOGETTEXT("Unrecognized locale... setting to English\n"));
	        strcpy(help_dir, helpdir); /* If not, set default locale */
		strcat(help_dir, default_locale); /* to English. */
		locale = default_locale;
	}

	if (argc == 1)
		numerrs += findprt(ask());
	else
		for (i = 1; i < argc; i++)
			numerrs += findprt(argv[i]);

	return ((numerrs == (argc-1)) ? 1 : 0);
}


static int
findprt(p)
char *p;		/* "p" is user specified error code. */
{
	register char *q;
	char key[150];

	if ((int) size(p) > 50) 
		return(1);

	q = p;

	while (*q && !numeric(*q))
		q++;

	if (*q == '\0') {		/* all alphabetics */
		strcpy(key,p);
		sprintf(hfile,"%s%s%s",helpdir,locale,NOGETTEXT("/cmds"));
		if (!exists(hfile))     
			sprintf(hfile,"%s%s%s",helpdir,locale,dftfile);
	}
	else
		if (q == p) {		/* first char numeric */
			strcpy(key,p);
			sprintf(hfile,"%s%s%s",helpdir,locale,dftfile);
		}
	else {				/* first char alpha, then numeric */
		strcpy(key,p);		/* key used as temporary */
		*(key + (q - p)) = '\0';
		if(!lochelp(key,hfile)) 
		        sprintf(hfile,"%s%s%s%s",helpdir,locale,
		           NOGETTEXT("/"),key);
		else
			cat(hfile,hfile,NOGETTEXT("/"),locale,
			   NOGETTEXT("/"),key,0);
		strcpy(key,q);
		if (!exists(hfile)) {
			strcpy(key,p);
			sprintf(hfile,"%s%s%s",helpdir,locale,dftfile);
		}
	}

	if((iop = fopen(hfile,NOGETTEXT("r"))) == NULL)
		return(1);

	/*
	Now read file, looking for key.
	*/
	while ((q = fgets(line,sizeof(line)-1,iop)) != NULL) {
		repl(line,'\n','\0');		/* replace newline char */
		if (line[0] == '-' && equal(&line[1],key))
			break;
	}

	if (q == NULL) {	/* endfile? */
		fclose(iop);
		sprintf(SccsError, gettext("Key '%s' not found (he1)"),p);
		fatal(SccsError);
		return(1);
	}

	printf("\n%s:\n",p);

	while (fgets(line,sizeof(line)-1,iop) != NULL && line[0] == '-')
		;

	do {
		if (line[0] != '*')
			printf("%s",line);
	} while (fgets(line,sizeof(line)-1,iop) != NULL && line[0] != '-');

	fclose(iop);
	return(0);
}

static char *
ask()
{
	static char resp[51];

	iop = stdin;

	printf(gettext("Enter the message number or SCCS command name: "));
	fgets(resp,51,iop);
	return(repl(resp,'\n','\0'));
}


/* lochelp finds the file which contains the help messages.
 * if none found returns 0.  If found, as a side effect, lochelp
 * modifies the actual second parameter passed to lochelp to contain
 * the file name of the found file (pointed to by the automatic
 * variable fi).
 */
static int
lochelp(ky,fi)
        char *ky,*fi; /*ky is key  fi is found file name */
{
	FILE *fp;
	char locfile[MAXLINE + 1];
	char *hold;
	if(!(fp = fopen(HELPLOC,"r")))
	{
		/*no lochelp file*/
		return(0); 
	}
	while(fgets(locfile,sizeof(locfile)-1,fp)!=NULL)
	{
		hold=(char *)strtok(locfile,"\t ");
		if(!(strcmp(ky,hold)))
		{
			hold=(char *)strtok(0,"\n");
			strcpy(fi,hold); /* copy file name to fi */
			fclose(fp);
			return(1); /* entry found */
		}
	}
	fclose(fp);
	return(0); /* no entry found */
}

/* for fatal() */
void
clean_up()
{
}