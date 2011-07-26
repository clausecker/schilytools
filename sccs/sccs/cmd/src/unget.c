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
 * This file contains modifications Copyright 2006-2011 J. Schilling
 *
 * @(#)unget.c	1.17 11/07/04 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)unget.c 1.17 11/07/04 J. Schilling"
#endif
/*
 * @(#)unget.c 1.24 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)unget.c"
#pragma ident	"@(#)sccs:cmd/unget.c"
#endif
# include	<defines.h>
# include	<version.h>
# include	<had.h>
# include       <i18n.h>
# include       <schily/utsname.h>
# include       <schily/wait.h>
# include       <schily/ctype.h>
# include       <ccstypes.h>
# include	<schily/sysexits.h>

#ifdef	HAVE_SETRESUID
/*
 * HP-UX does not have seteuid(). use setresuid() instead.
 */
#define	seteuid(u)	setresuid((uid_t)-1, (uid_t)(u), (uid_t)-1)
#endif

/*
		Program can be invoked as either "unget" or
		"sact".  Sact simply displays the p-file on the
		standard output.  Unget removes a specified entry
		from the p-file.
*/

extern	char had_dir, had_standinp;

static int	num_files;
static int	cmd;
static long	Szqfile;
static char	Pfilename[FILESIZE];
static struct packet	gpkt;
static struct sid	sid;
static struct utsname 	un;
static char *uuname;

struct stat	Statbuf;
char	SccsError[MAXERRORLEN];

	int	main	__PR((int argc, char **argv));
static void	unget	__PR((char *file));
static struct	pfile *edpfile __PR((struct packet *pkt, struct sid *sp));
	void    clean_up __PR((void));
static void	catpfile __PR((struct packet *pkt));

int
main(argc,argv)
int argc;
char *argv[];
{
	int	c,i, testmore;
	char	*p;
	extern	int Fcnt;
	int current_optind;
	int no_arg;

	/*
	 * get the user name unfront.
	 */
	logname();
	/*
	 * Set locale for all categories.
	 */
	setlocale(LC_ALL, NOGETTEXT(""));
	
	/* 
	 * Set directory to search for general l10n SCCS messages.
	 */
#ifdef	PROTOTYPES
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	   NOGETTEXT(INS_BASE "/ccs/lib/locale/"));
#else
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	   NOGETTEXT("/usr/ccs/lib/locale/"));
#endif
	
	(void) textdomain(NOGETTEXT("SUNW_SPRO_SCCS"));

	tzset();	/* Set up timezome related vars */

	Fflags = FTLEXIT | FTLMSG | FTLCLN;

	current_optind = 1;
	optind = 1;
	opterr = 0;
	no_arg = 0;
	i = 1;
	/*CONSTCOND*/
	while (1) {
			if(current_optind < optind) {
			   current_optind = optind;
			   argv[i] = 0;
			   if (optind > i+1 ) {
			      if( (argv[i+1][0]!='-')&&(no_arg==0) ) {
				 argv[i+1] = NULL;
			      } else {
			         optind = i+1;
			         current_optind = optind;
			      }   	 
			   }
			}
			no_arg = 0;
			i = current_optind;
		        c = getopt(argc, argv, "-r:snV(version)");

				/* this takes care of options given after
				** file names.
				*/
			if((c == EOF)) {
			   if (optind < argc) {
				/* if it's due to -- then break; */
			       if(argv[i][0] == '-' &&
				      argv[i][1] == '-') {
			          argv[i] = 0;
			          break;
			       }
			       optind++;
			       current_optind = optind;
			       continue;
			   } else {
			       break;
			   }
			}
			p = optarg;
			testmore = 0;
			switch (c) {

			case 'r':
				if ( (p[0]==0)||(isdigit(((unsigned char *)p)[0])==0) ) {
					no_arg = 1;
					continue;
				}				
				chksid(sid_ab(p,&sid),&sid);
				break;
			case 'n':
			case 's':
				testmore++;
				break;
                        case 'q': /* enable NSE mode */
				if(p) {
                                  if (*p) {
                                        nsedelim = p;
				  }
                                } else {
                                        nsedelim = (char *) 0;
                                }
                                break;

			case 'V':		/* version */
				printf("unget %s-SCCS version %s %s (%s-%s-%s)\n",
					PROVIDER,
					VERSION,
					VDATE,
					HOST_CPU, HOST_VENDOR, HOST_OS);
				exit(EX_OK);

			default:
				fatal(gettext("Usage: unget [-ns][-r SID] file.."));
			}

			if (testmore) {
				testmore = 0;
				if (p) {
				   if (*p) {
					sprintf(SccsError,
						gettext("value after %c arg (cm7)"),
						c);
					fatal(SccsError);
				   }
				}
			}

			/*
			 * Make sure that we only collect option letters from
			 * the range 'a'..'z' and 'A'..'Z'.
			 */
			if (ALPHA(c) &&
			    (had[LOWER(c)? c-'a' : NLOWER+c-'A']++))
				fatal(gettext("key letter twice (cm2)"));
	}

	for(i=1; i<argc; i++){
		if(argv[i]) {
		       num_files++;
		}
	}

	if(num_files == 0)
		fatal(gettext("missing file arg (cm3)"));

	/*	If envoked as "sact", set flag
		otherwise executed as "unget".
	*/
	if (equal(sname(argv[0]),NOGETTEXT("sact"))) {
		cmd = 1;
		HADS = 0;
	}

	setsig();
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if ((p=argv[i]) != NULL)
			do_file(p, unget, 1, 1);

	return (Fcnt ? 1 : 0);
}

static void
unget(file)
char *file;
{
	char	gfilename[FILESIZE];
	char	str[BUFSIZ];
	struct	pfile *pp;
	uid_t	holduid;

	if (setjmp(Fjmp))
		return;

	/*	Initialize packet, but do not open SCCS file.
	*/
	sinit(&gpkt,file,0);
	gpkt.p_stdout = stdout;
	gpkt.p_verbose = (HADS) ? 0 : 1;

	copy(auxf(gpkt.p_file,'g'),gfilename);
	if (cmd == 0) {
	   if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	} else {
	   /*  envoked as "sact", call catpfile() and return. */
	   catpfile(&gpkt);
	   return;
	}
	uname(&un);
	uuname = un.nodename;
	if (lockit(auxf(gpkt.p_file,'z'),SCCS_LOCK_ATTEMPTS, getpid(),uuname))
		fatal(gettext("cannot create lock file (cm4)"));
	pp = edpfile(&gpkt,&sid);
	if (gpkt.p_verbose) {
		sid_ba(&pp->pf_nsid,str);
		fprintf(gpkt.p_stdout,"%s\n",str);
	}

	/*	If the size of the q-file is greater than zero,
		rename the q-file the p-file and remove the
		old p-file; else remove both the q-file and
		the p-file.
	*/
	if (Szqfile)
		rename(auxf(gpkt.p_file,'q'),Pfilename);
	else {
		xunlink(Pfilename);
		xunlink(auxf(gpkt.p_file,'q'));
	}
	ffreeall();
	uname(&un);
	uuname = un.nodename;
	unlockit(auxf(gpkt.p_file,'z'), getpid(),uuname);

	if (!HADN) {
		fflush(gpkt.p_stdout);

		holduid=geteuid();
		seteuid(getuid());
		unlink(gfilename);
		seteuid(holduid);
	}
}


static struct pfile *
edpfile(pkt,sp)
struct packet *pkt;
struct sid *sp;
{
	static	struct pfile goodpf;
	char	*user, *cp;
	char	line[BUFSIZ];
	struct	pfile pf;
	int	cnt, name;
	FILE	*in, *out;

	cnt = -1;
	name = 0;
	user = logname();
	zero((char *)&goodpf,sizeof(goodpf));
	in = xfopen(auxf(pkt->p_file,'p'), O_RDONLY|O_BINARY);
	cp  = auxf(pkt->p_file,'q');
	out = xfcreat(cp, (mode_t)0644);
	while (fgets(line,sizeof(line),in) != NULL) {
		pf_ab(line,&pf,1);
		if (equal(pf.pf_user,user)) {
			name++;
			if (sp->s_rel == 0) {
				if (++cnt) {
					fclose(out);
					fclose(in);
					fatal(gettext("SID must be specified (un1)"));
				}
				goodpf = pf;
				continue;
			}
			else if (sp->s_rel == pf.pf_nsid.s_rel &&
				sp->s_lev == pf.pf_nsid.s_lev &&
				sp->s_br == pf.pf_nsid.s_br &&
				sp->s_seq == pf.pf_nsid.s_seq) {
					goodpf = pf;
					continue;
			}
		}
		if (fputs(line,out) == EOF) {
			xmsg(cp, NOGETTEXT("edpfile"));
		}
	}
	fflush(out);
	fflush(stderr);
	fstat((int) fileno(out),&Statbuf);
	Szqfile = Statbuf.st_size;
	copy(auxf(pkt->p_file,'p'),Pfilename);
	fclose(out);
	fclose(in);
	if (!goodpf.pf_user[0]) {
		if (!name)
			fatal(gettext("login name not in p-file (un2)"));
		else fatal(gettext("specified SID not in p-file (un3)"));
	}
	return(&goodpf);
}


/* clean_up() only called from fatal().
*/

void
clean_up()
{
	/*	Lockfile and q-file only removed if lockfile
		was created by this process.
	*/
	uname(&un);
	uuname = un.nodename;
	if (mylock(auxf(gpkt.p_file,'z'), getpid(),uuname)) {
		unlink(auxf(gpkt.p_file,'q'));
		ffreeall();
		unlockit(auxf(gpkt.p_file,'z'), getpid(),uuname);
	}
}

static void
catpfile(pkt)
struct packet *pkt;
{
	int c;
	FILE *in;

	if(!(in = fopen(auxf(pkt->p_file,'p'), NOGETTEXT("rb")))) {
	   if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
	      fprintf(stderr,"\n%s:\n",gpkt.p_file);
	   fatal(gettext("No outstanding deltas"));
	} else {
	   if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	   while ((c = getc(in)) != EOF)
	      putc(c,pkt->p_stdout);
	   fclose(in);
	}
}
