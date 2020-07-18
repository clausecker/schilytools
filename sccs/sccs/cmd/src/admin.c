/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)admin.c	1.138 20/07/14 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)admin.c 1.138 20/07/14 J. Schilling"
#endif
/*
 * @(#)admin.c 1.39 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)admin.c"
#pragma ident	"@(#)sccs:cmd/admin.c"
#endif

# define	NEED_PRINTF_J		/* Need defines for js_snprintf()? */
# include	<defines.h>
# include	<version.h>
# include	<had.h>
# include	<i18n.h>
# include	<schily/dirent.h>
# include	<schily/setjmp.h>
# include	<schily/utsname.h>
# include	<schily/wait.h>
# include	<schily/sysexits.h>
# include	<schily/maxpath.h>
# define	VMS_VFORK_OK
# include	<schily/vfork.h>

/*
	Program to create new SCCS files and change parameters
	of existing ones. Arguments to the program may appear in
	any order and consist of keyletters, which begin with '-',
	and named files. Named files which do not exist are created
	and their parameters are initialized according to the given
	keyletter arguments, or are given default values if the
	corresponding keyletters were not supplied. Named files which
	do exist have those parameters corresponding to given key-letter
	arguments changed and other parameters are left as is.

	If a directory is given as an argument, each SCCS file within
	the directory is processed as if it had been specifically named.
	If a name of '-' is given, the standard input is read for a list
	of names of SCCS files to be processed.
	Non-SCCS files are ignored.

	Files created are given mode 444.
*/

/*
TRANSLATION_NOTE
For all message strings the double quotation marks at the beginning
and end of the string MUST be included in the translation.  Formatting
characters, e.g. "%s" "%c" "%d" "\n" "\t" must appear in the
translated string exactly as they do in the msgid string.  Spaces
and/or tabs around these formats must be maintained.

The following are examples of text that should not be translated, but
should appear exactly as they do in the msgid string:

- Any SCCS error code, which will be one or two letters followed by one
  or two numbers all in parenthesis, e.g. "(ad3)",

- All descriptions of SCCS option flags, e.g. "-r" or " 'e'" , or "f",
  or "-fz"

- ".FRED", "sid", "SID", "MRs", "CMR", "p-file", "CASSI",  "cassi",
  function names, e.g. "getcwd()",
*/

# define MAXNAMES 9

static FILE	*Cs;			/* The changeset file		*/
static char	stdin_file_buf [20];	/* For "/tmp/admin.XXXXXX"	*/
static char	*ifile;			/* -i argument			*/
static char	*tfile;			/* -t argument			*/
static char	*dir_name;		/* directory for -N		*/
static struct timespec	ifile_mtime;	/* Timestamp for -i -o		*/
static Nparms	N;			/* Keep -N parameters		*/
static Xparms	X;			/* Keep -X parameters		*/
static char	*CMFAPPL;		/* CMF MODS			*/
static char	*z;			/* for validation program name	*/
static char	had_flag[NFLAGS];	/* -f seen list			*/
static char	rm_flag[NFLAGS];	/* -d seen list			*/
#if	defined(PROTOTYPES) && defined(INS_BASE)
static char	Valpgmp[]	=	NOGETTEXT(INS_BASE "/" SCCS_BIN_PRE "bin/" "val");
#endif
static char	Valpgm[]	=	NOGETTEXT("val");
static int	fexists;		/* Current file exists		*/
static int	num_files;		/* Number of file args		*/
static int	VFLAG  =  0;		/* -v option seen		*/
static int	versflag = -1;		/* history vers for new files	*/
static struct sid	new_sid;	/* -r argument			*/
static char	*anames[MAXNAMES];	/* -a arguments			*/
static char	*enames[MAXNAMES];	/* -e arguments			*/
static char	*unlockarg;		/* -dl argument			*/
static char	*locks;			/* 'l' flag value in file	*/
static char	*flag_p[NFLAGS];	/* -f arguments			*/
static int	asub;			/* Index for anames[]		*/
static int	esub;			/* Index for enames[]		*/
static int	check_id;		/* To check for Keywds with -i	*/
static struct	utsname	un;		/* uname for lockit()		*/
static char	*uuname;		/* un.nodename			*/
static int 	Encoded = EF_TEXT;	/* Default encoding is '0'	*/
static off_t	Encodeflag_offset;	/* offset in file where encoded flag is stored */
static off_t	Checksum_offset;	/* offset in file where g-file hash is stored */

	int	main __PR((int argc, char **argv));
static	void	admin __PR((char *afile));
static	int	fgetchk __PR((FILE *inptr, char *file, struct packet *pkt, int fflag));
static	void	warnctl __PR((char *file, off_t nline));
static	void	clean_up __PR((void));
static	void	cmt_ba __PR((register struct deltab *dt, char *str, int flags));
static	void	putmrs __PR((struct packet *pkt));
static	char *	adjust __PR((char *line));
static	char *	getval __PR((register char *sourcep, register char *destp));
static	int	val_list __PR((register char *list));
static	int	pos_ser __PR((char *s1, char *s2));
static	int	range __PR((register char *line));
static	FILE *	code __PR((FILE *iptr, char *afile, off_t offset, int thash, struct packet *pktp));
static	void	direrror __PR((char *dir, int keylet));

extern int	org_ihash;
extern int	org_chash;
extern int	org_uchash;

int
main(argc,argv)
int argc;
char *argv[];
{
	register int j;
	register char *p;
	char  f;
	int i, testklt,c;
	extern int Fcnt;
	struct sid sid;
	int no_arg=0;
	int current_optind;

	/*
	 * Set locale for all categories.
	 */
	setlocale(LC_ALL, NOGETTEXT(""));

	sccs_setinsbase(INS_BASE);

	/*
	 * Set directory to search for general l10n SCCS messages.
	 */
#ifdef	PROTOTYPES
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	   NOGETTEXT(INS_BASE "/" SCCS_BIN_PRE "lib/locale/"));
#else
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	   NOGETTEXT("/usr/ccs/lib/locale/"));
#endif

	(void) textdomain(NOGETTEXT("SUNW_SPRO_SCCS"));

	tzset();	/* Set up timezome related vars */

#ifdef	SCHILY_BUILD
	save_args(argc, argv);
#endif
	/*
	Set flags for 'fatal' to issue message, call clean-up
	routine and terminate processing.
	*/
	set_clean_up(clean_up);
	Fflags = FTLMSG | FTLCLN | FTLEXIT;
#ifdef	SCCS_FATALHELP
	Fflags |= FTLFUNC;
	Ffunc = sccsfatalhelp;
#endif

	testklt = 1;

	/*
	The following loop processes keyletters and arguments.
	Note that these are processed only once for each
	invocation of 'main'.
	*/

	current_optind = 1;
	optind = 1;
	opterr = 0;
	j = 1;
	/*CONSTCOND*/
	while (1) {
			if(current_optind < optind) {
			   current_optind = optind;
			   argv[j] = 0;
			   if (optind > j+1 ) {
			      if((argv[j+1][0] != '-') && !no_arg){
			        argv[j+1] = NULL;
			      }
			      else {
				optind = j+1;
			        current_optind = optind;
			      }
			   }
			}
			no_arg = 0;
			j = current_optind;
		        c = getopt(argc, argv, "()-i:t:m:y:d:f:r:nN:hzboqkw:a:e:X:V:(version)");
				/*
				*  this takes care of options given after
				*  file names.
				*/
			if (c == EOF) {
			   if (optind < argc) {
				/* if it's due to -- then break; */
			       if(argv[j][0] == '-' &&
				      argv[j][1] == '-') {
			          argv[j] = 0;
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
			switch (c) {

			case 'i':	/* name of file of body */
				if (optarg == argv[j+1]) {
				   no_arg = 1;
				   ifile = "";
				   break;
				}
				/*
				 * Use -i. to tell admin to retrieve the ifile
				 * name from the g-file name with -N
				 * We cannot check for "." here as -N may have
				 * been specified later on the cmd line.
				 */
				ifile = p;
				if (*ifile &&
				    !(ifile[0] == '.' && ifile[1] == '\0') &&
				    exists(ifile)) {
				   if ((Statbuf.st_mode & S_IFMT) == S_IFDIR) {
					direrror(ifile, c);
				   } else {
					ifile_mtime.tv_sec = Statbuf.st_mtime;
					ifile_mtime.tv_nsec = stat_mnsecs(&Statbuf);
				   }
				}
				break;
			case 't':	/* name of file of descriptive text */
				if (optarg == argv[j+1]) {
				   no_arg = 1;
				   tfile = NULL;
				   break;
				}
				tfile = p;
				if (*tfile && exists(tfile))
				   if ((Statbuf.st_mode & S_IFMT) == S_IFDIR)
					direrror(tfile, c);
				break;
			case 'm':	/* mr flag */
				/*
				 * Former sccs versions did allow to call
				 * admin -fv -m -i... to specify "no mr".
				 * With getopt, this only works with '-m '
				 * or -m ''.
				 */
				if (*p == '-')
					fatal(gettext("bad m argument (ad34)"));
				Mrs = p;
				repl(Mrs,'\n',' ');
				break;
			case 'y':	/* comments flag for entry */
				if (optarg == argv[j+1]) {
				   no_arg = 1;
				   Comments = "";
				} else {
				   Comments = p;
				}
				break;
			case 'd':	/* flags to be deleted */
				testklt = 0;
				if ((f = *p) == '\0')
				     fatal(gettext("d has no argument (ad1)"));
				p = p+1;

				switch (f) {

				case IDFLAG:	/* see 'f' keyletter */
				case BRCHFLAG:	/* for meanings of flags */
				case VALFLAG:
				case TYPEFLAG:
				case MODFLAG:
				case QSECTFLAG:
				case NULLFLAG:
				case FLORFLAG:
				case CEILFLAG:
				case DEFTFLAG:
				case JOINTFLAG:
				case SCANFLAG:
				case EXTENSFLAG:
				case EXPANDFLAG:
				case CMFFLAG:	/* option installed by CMF */
					if (*p) {
						sprintf(SccsError,
						gettext("value after %c flag (ad12)"),
							f);
						fatal(SccsError);
					}
					break;
				case LOCKFLAG:
					if (*p == '\0') {
						fatal(gettext("bad list format (ad27)"));
					}
					if (*p) {
						/*
						set pointer to releases
						to be unlocked
						*/
						repl(p,',',' ');
						if (!val_list(p))
							fatal(gettext("bad list format (ad27)"));
						if (!range(p))
							fatal(gettext("element in list out of range (ad28)"));
						if (*p != 'a')
							unlockarg = p;
					}
					break;

				default:
					fatal(gettext("unknown flag (ad3)"));
				}

				if (rm_flag[f - 'a']++)
					fatal(gettext("flag twice (ad4)"));
				break;

			case 'f':	/* flags to be added */
				testklt = 0;
				if ((f = *p) == '\0')
					fatal(gettext("f has no argument (ad5)"));
				p = p+1;

				switch (f) {

				case BRCHFLAG:	/* branch */
				case NULLFLAG:	/* null deltas */
				case JOINTFLAG:	/* joint edit flag */
					if (*p) {
						sprintf(SccsError,
						gettext("value after %c flag (ad13)"),
							f);
						fatal(SccsError);
					}
					break;

				case IDFLAG:	/* id-kwd message (err/warn) */
					break;

				case VALFLAG:	/* mr validation */
					VFLAG++;
					if (*p)
						z = p;
					break;

				case CMFFLAG: /* CMFNET MODS */
					if (*p)	/* CMFNET MODS */
						CMFAPPL = p; /* CMFNET MODS */
					else /* CMFNET MODS */
						fatal (gettext("No application with application flag."));
					if (gf (CMFAPPL) == (char*) NULL)
						fatal (gettext("No .FRED file exists for this application."));
					break; /* END CMFNET MODS */

				case FLORFLAG:	/* floor */
					if ((i = patoi(p)) == -1)
						fatal(gettext("floor not numeric (ad22)"));
					if (((int) size(p) > 5) || (i < MINR) ||
							(i > MAXR))
						fatal(gettext("floor out of range (ad23)"));
					break;

				case CEILFLAG:	/* ceiling */
					if ((i = patoi(p)) == -1)
						fatal(gettext("ceiling not numeric (ad24)"));
					if (((int) size(p) > 5) || (i < MINR) ||
							(i > MAXR))
						fatal(gettext("ceiling out of range (ad25)"));
					break;

				case DEFTFLAG:	/* default sid */
					if (!(*p))
						fatal(gettext("no default sid (ad14)"));
					chksid(sid_ab(p,&sid),&sid);
					break;

				case TYPEFLAG:	/* type */
				case MODFLAG:	/* module name */
				case QSECTFLAG:	/* csect name */
					if (!(*p)) {
						sprintf(SccsError,
						gettext("flag %c has no value (ad2)"),
							f);
						fatal(SccsError);
					}
					break;
				case LOCKFLAG:	/* release lock */
					if (!(*p))
						/*
						lock all releases
						*/
						p = NOGETTEXT("a");
					/*
					replace all commas with
					blanks in SCCS file
					*/
					repl(p,',',' ');
					if (!val_list(p))
						fatal(gettext("bad list format (ad27)"));
					if (!range(p))
						fatal(gettext("element in list out of range (ad28)"));
					break;
				case SCANFLAG:	/* the number of lines that are scanned to expand of SCCS KeyWords. */
					if ((i = patoi(p)) == -1)
						fatal(gettext("line not numeric (ad33)"));
					break;
				case EXTENSFLAG:	/* exable SCCS extensions */
					p = "SCHILY";	/* we use SCHILY type ext */
					break;
				case EXPANDFLAG:	/* expand the SCCS KeyWords */
					/* replace all commas with blanks in SCCS file */
					repl(p,',',' ');
					break;

				default:
					fatal(gettext("unknown flag (ad3)"));
				}

				if (had_flag[f - 'a']++)
					fatal(gettext("flag twice (ad4)"));
				flag_p[f - 'a'] = p;
				break;

			case 'r':	/* initial release number supplied */
				 chksid(sid_ab(p,&new_sid),&new_sid);
				 if ((new_sid.s_rel < MINR) ||
				     (new_sid.s_rel > MAXR))
					fatal(gettext("r out of range (ad7)"));
				 break;

			case 'N':	/* creating new SCCS files */
				initN(&N);
				if (optarg == argv[j+1]) {
				   no_arg = 1;
				   break;
				}
				N.n_parm = p;
				break;
			case 'n':	/* creating new SCCS file */
			case 'h':	/* only check hash of file */
			case 'k':	/* get(1) without keyword expand */
			case 'z':	/* zero the input hash */
			case 'b':	/* force file to be encoded (binary) */
			case 'o':	/* use original file date */
				break;

                        case 'q':       /* activate NSE features */
				if(p) {
                                  if (*p) {
                                        nsedelim = p;
				  }
                                } else {
                                        nsedelim = (char *) 0;
                                }
                                break;

			case 'w':
				if (p)
					whatsetup(p);
				break;

			case 'a':	/* user-name allowed to make deltas */
				testklt = 0;
				if (!(*p) || *p == '-')
					fatal(gettext("bad a argument (ad8)"));
				if (asub > MAXNAMES) {
					fatal(gettext("too many 'a' keyletters (ad9)"));
					/* NOTREACHED */
				}
				anames[asub++] = sccs_user(p);
				break;

			case 'e':	/* user-name to be removed */
				testklt = 0;
				if (!(*p) || *p == '-')
					fatal(gettext("bad e argument (ad10)"));
				if (esub > MAXNAMES) {
					fatal(gettext("too many 'e' keyletters (ad11)"));
					/* NOTREACHED */
				}
				enames[esub++] = sccs_user(p);
				break;

			case 'X':
				X.x_parm = optarg;
				X.x_flags = XO_INIT_PATH|XO_URAND|\
					XO_UNLINK|XO_MAIL|XO_USER|XO_DATE|\
					XO_NULLPATH|XO_NOBULK;
				if (!parseX(&X))
					goto err;
				had[NLOWER+c-'A'] = 0;	/* Allow mult -X */
				break;

			case 'V':		/* version */
				if (optarg == argv[j+1]) {
				doversion:
					printf(gettext(
					    "admin %s-SCCS version %s %s (%s-%s-%s)\n"),
						PROVIDER,
						VERSION,
						VDATE,
						HOST_CPU, HOST_VENDOR, HOST_OS);
					exit(EX_OK);
				}
				if (p[1] == '\0') {
					if (p[0] == '4') {
						versflag = 4;
						break;
					} else if (p[0] == '6') {
						versflag = 6;
						break;
					}
				}

			default:
			err:
				/*
				 * Check whether "-V" was last arg...
				 */
				if (optind == argc &&
				    ((argv[argc-1][0] == '-' &&
				    argv[argc-1][1] == 'V' &&
				    argv[argc-1][2] == '\0') ||
				    strcmp(argv[argc-1], "-version") == 0 ||
				    strcmp(argv[argc-1], "--version") == 0))
					goto doversion;
				fatal(gettext("Usage: admin [ -bhknoz ][ -ausername|groupid ]\n\t[ -dflag ][ -eusername|groupid ]\n\t[ -fflag [value]][ -i [filename]]\n\t[ -m mr-list][ -r release ][ -t [description-file]]\n\t[ -N[bulk-spec]][ -Xxopts ] [ -y[comment]] s.filename ..."));
			}
			/*
			 * Make sure that we only collect option letters from
			 * the range 'a'..'z' and 'A'..'Z'.
			 */
			if (ALPHA(c) &&
			    (had[LOWER(c)? c-'a' : NLOWER+c-'A']++ && testklt++)) {
				if (c != 'X')
					fatal(gettext("key letter twice (cm2)"));
			}
	}

	for (i = 1; i < argc; i++){
		if (argv[i]) {
		       num_files++;
		}
	}

	if (num_files == 0 && !HADUCN)
		fatal(gettext("missing file arg (cm3)"));
	if (num_files > 1 && (X.x_opts & (XO_INIT_PATH|XO_URAND)))
		fatal(gettext("too many file args (cm18)"));

	setsig();
	xsethome(NULL);
	if (HADUCN) {					/* Parse -N args  */
		/*
		 * initN() was already called while parsing options.
		 */
		if (HADI)
			N.n_flags |= N_IFILE;
		if (HADI && ifile[0] == '.' && ifile[1] == '\0')
			N.n_flags |= N_IDOT;
		if (HADN)
			N.n_flags |= N_NFILE;
		parseN(&N);
		if (N.n_get)
			N.n_flags |= N_GETI;

		if (N.n_sdot && (sethomestat & SETHOME_OFFTREE))
			fatal(gettext("-Ns. not supported in off-tree project mode"));

		/*
		 * If someone previously successfully called "sccs init" and
		 * "admin" is called with the option -N, we default to -V6.
		 */
		if (versflag < 0 && SETHOME_INIT()) {
			versflag = 6;
		}

	} else if (HADI && ifile[0] == '.' && ifile[1] == '\0') {
		direrror(ifile, 'i');
	}

#ifdef	SCCS_V6_ENV
	if (versflag < 0) {		/* Not explicitly selected, check ENV */
		if (getenv("SCCS_V6"))
			versflag = 6;
	}
#endif
	if (versflag < 0)
		versflag = 4;		/* This is the historic default */

	if ((HADY || HADM) && ! (HADI || HADN))
		fatal(gettext("illegal use of 'y' or 'm' keyletter (ad30)"));
	if (HADI && !HADUCN && num_files > 1) /* only one file allowed with `i' */
		fatal(gettext("more than one file (ad15)"));
	if ((HADI || HADN) && ! logname())
		fatal(gettext("USER ID not in password file (cm9)"));

	/*
	 * Get the name of our machine to be used for the lockfile.
	 */
	uname(&un);
	uuname = un.nodename;

	/*
	 * Set up a project global lock on the changeset file.
	 * Since we set FTLJMP, we do not need to unlockchset() from clean_up().
	 */
	if (!HADH && SETHOME_CHSET()) {
		lockchset(getppid(), getpid(), uuname);
		if (HADUCN && exists(changesetgfile)) {
			/*
			 * Should we open/create the file even if it does not
			 * yet exist? This may change with the final concept.
			 */
			Cs = xfopen(changesetgfile, O_WRONLY|O_APPEND|O_BINARY);
		}
	}
	timerchsetlock();

	/*
	Change flags for 'fatal' so that it will return to this
	routine (main) instead of terminating processing.
	*/
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;

	/*
	Call 'admin' routine for each file argument.
	*/
	for (j=1; j<argc; j++)
		if ((p = argv[j]) != NULL)
			do_file(p, admin, HADN|HADI ? 0 : 1, N.n_sdot, &X);

	if (num_files == 0 && HADUCN)
		do_file("-", admin, 0, N.n_sdot, &X);

	/*
	 * Only remove the global lock it it was created by us and not by
	 * our parent.
	 */
	if (!HADH && SETHOME_CHSET()) {
		if (HADUCN) {
			bulkchdir(&N);
			if (Cs)
				fclose(Cs);
		}
		unlockchset(getpid(), uuname);
	}

	return (Fcnt ? 1 : 0);
}

/*
	Routine that actually does admin's work on SCCS files.
	Existing s-files are copied, with changes being made, to a
	temporary file (x-file). The name of the x-file is the same as the
	name of the s-file, with the 's.' replaced by 'x.'.
	s-files which are to be created are processed in a similar
	manner, except that a dummy s-file is first created with
	mode 444.
	At end of processing, the x-file is renamed with the name of s-file
	and the old s-file is removed.
*/

static struct packet gpkt;	/* see file defines.h */
static char	Zhold[MAXPATHLEN];	/* temporary z-file name */

static void
admin(afile)
char	*afile;
{
	struct deltab dt;	/* see file defines.h */
	struct stats stats;	/* see file defines.h */
	struct stat sbuf;
	FILE	*iptr;
	register int k;
	register char *cp;
	register signed char *q;
	char	*in_f;		/* ptr holder for lockflag vals in SCCS file */
	char	nline[MAXLINE];
	char	*p_lval, *tval;
	char	*lval;
	char	f;		/* character holder for flag character */
	char	line[MAXLINE];
	int	ck_it;		/* used for lockflag checking */
	off_t	offset;
	int     thash;
	int	status;		/* used for status return from fork */
	int	from_stdin;	/* used for ifile */
	extern	char had_dir;

	if (setjmp(Fjmp))	/* set up to return here from 'fatal' */
		return;		/* and return to caller of admin */

	/*
	 * Initialize here to avoid setjmp() clobbering warnings
	 */
	iptr = NULL;
	lval = NULL;
	ck_it = 0;
	from_stdin = 0;
	dir_name = "";
 	Encoded = EF_TEXT;	/* Default encoding is '0' */

	zero((char *) &stats,sizeof(stats));

	/*
	 * In order to make the global lock with a potentially long duration
	 * not look as if it was expired, we refresh it for every file in our
	 * task list. This is needed since another SCCS instance on a different
	 * NFS machine cannot use kill() to check for a still active process.
	 */
	if (!HADH && SETHOME_CHSET()) {
		if (HADUCN)
			bulkchdir(&N);	/* Done by bulkprepare() anyway */
		refreshchsetlock();
	}

	if (HADUCN && !(X.x_opts & XO_NOBULK)) {
		char	*oafile = afile;

		afile = bulkprepare(&N, afile);
		if (N.n_error != 0 && N.n_error != BULK_EISDIR) {
			fatal(gettext(bulkerror(&N)));
		} else if (afile == NULL) {
			if (N.n_ifile)
				direrror(N.n_ifile, 'i');
			else
				direrror(oafile, 'i');
		}
		if (N.n_mtime.tv_sec != 0) {
			ifile_mtime.tv_sec = N.n_mtime.tv_sec;
			ifile_mtime.tv_nsec = N.n_mtime.tv_nsec;
		}
		had_dir = 0;
		dir_name = N.n_dir_name;
		if (N.n_flags & N_IDOT)
			ifile = N.n_ifile;
	}

	if (HADI && had_dir) /* directory not allowed with `i' keyletter */
		fatal(gettext("directory named with `i' keyletter (ad26)"));

	fexists = exists(afile);

	if (HADI)
		HADN = 1;
	if (HADI || HADN) {
		if (VFLAG && had_flag[CMFFLAG - 'a'])
			fatal(gettext("Can't have two verification routines."));

		if (HADM && !VFLAG && !had_flag[CMFFLAG - 'a'])
			fatal(gettext("MRs not allowed (de8)"));

		if (VFLAG && !HADM)
			fatal(gettext("MRs required (de10)"));

	}

	if (!(HADI||HADN) && HADR)
		fatal(gettext("r only allowed with i or n (ad16)"));

	if (HADN && HADT && !tfile)
		fatal(gettext("t has no argument (ad17)"));

	if (HADN && HADD)
		fatal(gettext("d not allowed with n (ad18)"));

	if (HADN && fexists) {
		sprintf(SccsError, gettext("file %s exists (ad19)"),
			afile);
		fatal(SccsError);
	}

	if (!HADN && !fexists) {
		sprintf(SccsError, gettext("file %s does not exist (ad20)"),
			afile);
		fatal(SccsError);
	}
	if (HADH) {
		pid_t	pid;

		/*
		   fork here so 'admin' can execute 'val' to
		   check for a corrupted file.
		*/
		if ((pid = vfork()) < 0)
			efatal(gettext("cannot fork, try again"));
		if (pid == 0) {		/* child */
			/*
			   perform 'val' with appropriate keyletters
			*/
#if	defined(PROTOTYPES) && defined(INS_BASE)
			execlp(Valpgmp, Valpgm, "-s", afile, (char *)0);
#endif
			execlp(Valpgm, Valpgm, "-s", afile, (char *)0);
			sprintf(SccsError, gettext("cannot execute '%s'"),
				Valpgm);
#ifdef	HAVE_VFORK
			Fflags |= FTLVFORK;
#endif
			efatal(SccsError);
		}
		else {
			wait(&status);	   /* wait on status from 'execlp' */
			if (status)
				fatal(gettext("corrupted file (co6)"));
			return;		/* return to caller of 'admin' */
		}
	}

	/*
	 * Lock out any other user who may be trying to process
	 * the same file.
	 */
	if (!HADH && !islockchset(copy(auxf(afile, 'z'), Zhold))) {
		if (lockit(Zhold, SCCS_LOCK_ATTEMPTS, getpid(), uuname)) {
			lockfatal(Zhold, getpid(), uuname);
		}
		timersetlockfile(Zhold);
	}

	if (fexists) { /* modifying */
		int	cklen = 8;

		sinit(&gpkt, afile, SI_OPEN);	/* init pkt & open s-file */

		if (gpkt.p_flags & PF_V6)
			cklen = 15;

		/* Modify checksum if corrupted */

		if ((int) strlen(gpkt.p_line) > cklen &&
		    gpkt.p_line[0] == '\001' &&
		    gpkt.p_line[1] == '\150') {
			gpkt.p_line[cklen-1] = '\012';
			gpkt.p_line[cklen]   = '\000';
		}
	}
	else {
		if ((int) strlen(sname(afile)) > MAXNAMLEN) {
			sprintf(SccsError, gettext("file name is greater than %d characters"),
				MAXNAMLEN);
			fatal(SccsError);
		}
		if (sccsfile(afile)) {
			FILE	*xf;

			/*
			 * create dummy s-file
			 *
			 * Closing is needed on Cygwin to avoid an EPERM in
			 * rename()
			 */
			Statbuf.st_mode = 0;
			if (HADI && *ifile)
				(void) exists(ifile);
			else
				(void )exists(auxf(afile,'g'));
			if (S_IEXEC & Statbuf.st_mode) {
				xf = xfcreat(afile, 0555);
			} else {
				xf = xfcreat(afile, 0444);
			}
			if (xf)
				fclose(xf);
		}

		sinit(&gpkt, afile, SI_INIT);	/* and init pkt */

		/*
		 * Initialize global meta data
		 */
		if (versflag == 6) {
			if (X.x_opts & XO_INIT_PATH) {
				gpkt.p_init_path = X.x_init_path;
			} else if (HADUCN && !(X.x_opts & XO_NOBULK)) {
				/*
				 * Only if we have been called with -N..., we
				 * know the real g-file name. We cannot derive
				 * the g-file name from the s.file name
				 * otherwise, since we cannot know about sub
				 * dirs like "SCCS" in the other cases.
				 */
				set_init_path(&gpkt, N.n_ifile, dir_name);
			}

			if (X.x_opts & XO_URAND)
				gpkt.p_rand = X.x_rand;
			else
				urandom(&gpkt.p_rand);
		}
	}

	if (!HADH)
		/*
		   set the flag for 'putline' routine to open
		   the 'x-file' and allow writing on it.
		*/
		gpkt.p_upd = 1;

	if (HADZ) {
		gpkt.do_chksum = 0;	/* ignore checksum processing */
		org_ihash = gpkt.p_ihash;
		gpkt.p_ihash = 0;
	}

	/*
	Get statistics of latest delta in old file.
	*/
	if (!HADN) {
		stats_ab(&gpkt,&stats);
		gpkt.p_wrttn++;
		newstats(&gpkt,line,"0");
	}

	if (HADN) {		/*   N E W   F I L E   */

		if (versflag == 6)
			gpkt.p_flags |= PF_V6;

		/*
		Beginning of SCCS file.
		*/
		putmagic(&gpkt, "00000");

		/*
		Statistics.
		*/
		newstats(&gpkt,line,"0");

		dt.d_type = 'D';	/* type of delta */
		if (X.x_opts & XO_UNLINK)
			dt.d_type = 'U';

		/*
		Set initial release, level, branch and
		sequence values.
		*/
		if (HADR)
			{
			 dt.d_sid.s_rel = new_sid.s_rel;
			 dt.d_sid.s_lev = new_sid.s_lev;
			 dt.d_sid.s_br  = new_sid.s_br ;
			 dt.d_sid.s_seq = new_sid.s_seq;
			 if (dt.d_sid.s_lev == 0) dt.d_sid.s_lev = 1;
			 if ((dt.d_sid.s_br) && ( ! dt.d_sid.s_seq))
				dt.d_sid.s_seq = 1;
			}
		else
			{
			 dt.d_sid.s_rel = 1;
			 dt.d_sid.s_lev = 1;
			if (X.x_opts & XO_UNLINK)
				dt.d_sid.s_lev = 0;
			 dt.d_sid.s_br = dt.d_sid.s_seq = 0;
			}
		dtime(&dt.d_dtime);		/* get time and date */
                if (HADN && HADI && (HADO || HADQ) &&
		    (ifile_mtime.tv_sec != 0)) {
                        /*
			 * When specifying -o (original date) and
			 * for NSE when putting existing file under sccs the
                         * delta time is the mtime of the clear file.
                         */
			time2dt(&dt.d_dtime,
				ifile_mtime.tv_sec, ifile_mtime.tv_nsec);
                }
		if (HADN && (X.x_opts & XO_DATE)) {
			dt.d_dtime = X.x_dtime;
		}

		strlcpy(dt.d_pgmr, logname(), LOGSIZE);	/* get user's name */
		if (X.x_user)				/* from -Xuser=	*/
			strlcpy(dt.d_pgmr, X.x_user, LOGSIZE);

		dt.d_serial = 1;
		dt.d_pred = 0;

		gpkt.p_reqsid = dt.d_sid;	/* set sid for changelog */

		del_ba(&dt,line, gpkt.p_flags);	/* form and write */
		putline(&gpkt,line);	/* delta-table entry */

		/*
		If -m flag, enter MR numbers
		*/

		if (Mrs) {
			if (had_flag[CMFFLAG - 'a']) {	/* CMF check routine */
				if (cmrcheck (Mrs, CMFAPPL) != 0) {	/* check them */
					fatal (gettext("Bad CMR number(s)."));
					}
				}
			mrfixup();
			if (z && valmrs(&gpkt,z))
				fatal(gettext("invalid MRs (de9)"));
			putmrs(&gpkt);
		}
		if (gpkt.p_flags & PF_V6) {
			Checksum_offset = ftell(gpkt.p_xiop);
			gpkt.p_mail = X.x_mail;
			sidext_ba(&gpkt, &dt);	/* Will not write "dt" entry. */
			gpkt.p_mail = NULL;
		}

		/*
		Enter comment line for `chghist'
		*/

		if (HADY) {
		        char *comment = savecmt(Comments);
			sprintf(line,"%c%c ",CTLCHAR,COMMENTS);
			putline(&gpkt,line);
			putline(&gpkt,comment);
			putline(&gpkt,"\n");
		}
		else {
			/*
			insert date/time and pgmr into comment line
			*/
			cmt_ba(&dt, line, gpkt.p_flags);
			putline(&gpkt,line);
		}
		/*
		End of delta-table.
		*/
		sprintf(line,CTLSTR,CTLCHAR,EDELTAB);
		putline(&gpkt,line);

		/*
		Beginning of user-name section.
		*/
		sprintf(line,CTLSTR,CTLCHAR,BUSERNAM);
		putline(&gpkt,line);
	}
	else
		/*
		For old file, copy to x-file until user-name section
		is found.
		*/
		flushto(&gpkt, BUSERNAM, FLUSH_COPY);

	/*
	Write user-names to be added to list of those
	allowed to make deltas.
	*/
	if (HADA)
		for (k = 0; k < asub; k++) {
			sprintf(line,"%s\n",anames[k]);
			putline(&gpkt,line);
		}

	/*
	Do not copy those user-names which are to be erased.
	*/
	if (HADE && !HADN)
		while (((cp = getline(&gpkt)) != NULL) &&
				!(*cp++ == CTLCHAR && *cp == EUSERNAM)) {
			for (k = 0; k < esub; k++) {
				cp = gpkt.p_line;
				while (*cp)	/* find and */
					cp++;	/* zero newline */
				*--cp = '\0';	/* character */

				if (equal(enames[k],gpkt.p_line)) {
					/*
					Tell getline not to output
					previously read line.
					*/
					gpkt.p_wrttn = 1;
					break;
				}
				else
					*cp = '\n';	/* restore newline */
			}
		}

	if (HADN) {		/*   N E W  F I L E   */

		/*
		End of user-name section.
		*/
		sprintf(line,CTLSTR,CTLCHAR,EUSERNAM);
		putline(&gpkt,line);
	} else {
		/*
		For old file, copy to x-file until end of
		user-names section is found.
		*/
		if (!HADE)
			flushto(&gpkt, EUSERNAM, FLUSH_COPY);
	}

	/*
	For old file, read flags and their values (if any), and
	store them. Check to see if the flag read is one that
	should be deleted.
	*/
	if (!HADN) {
		while (((cp = getline(&gpkt)) != NULL) &&
				(*cp++ == CTLCHAR && *cp++ == FLAG)) {

			gpkt.p_wrttn = 1;	/* don't write previous line */

			NONBLANK(cp);	/* point to flag character */
			k = *cp - 'a';
			if (k < 0 || k >= NFLAGS) {
				fprintf(stderr,
				gettext(
				"WARNING [%s]: unsupported flag at line %d\n"),
					gpkt.p_file,
					gpkt.p_slnno);
				/*
				 * Better to abort then to silently remove flags
				 * as previous versions did.
				 */
				fatal("unsupported flag (ad35)");
				continue;
			}
			f = *cp++;
			NONBLANK(cp);
			if (f == LOCKFLAG) {
				p_lval = cp;
				tval = fmalloc(size(gpkt.p_line)- (unsigned)5);
				copy(++p_lval,tval);
				lval = tval;
				while(*tval)
					++tval;
				*--tval = '\0';
			}

			if (!had_flag[k] && !rm_flag[k]) {
				had_flag[k] = 2;	/* indicate flag is */
							/* from file, not */
							/* from arg list */

				if (*cp != '\n') {	/* get flag value */
					q = (signed char *) fmalloc(size(gpkt.p_line) - (unsigned)5);
					copy(cp, (char *)q);
					flag_p[k] = (char*) q;
					while (*q)	/* find and */
						q++;	/* zero newline */
					*--q = '\0';	/* character */
					if (k == ENCODEFLAG - 'a') {
						int	i;

						NONBLANK(cp);
						cp = satoi(cp, &i);
						if (*cp == '\n')
							gpkt.p_encoding = i;
					}
				}
			}
			if (rm_flag[k]) {
				if (f == LOCKFLAG) {
					if (unlockarg) {
						in_f = lval;
						if (((lval = adjust(in_f)) != NULL) &&
							!had_flag[k])
							ck_it = had_flag[k] = 1;
					}
					else had_flag[k] = 0;
				}
				else had_flag[k] = 0;
			}
		}
	}

	/*
	Write out flags.
	*/
	/* test to see if the CMFFLAG is safe */
	if (had_flag[CMFFLAG - 'a']) {
		if (had_flag[VALFLAG - 'a'] && !rm_flag[VALFLAG - 'a'])
			fatal (gettext("Can't use -fz with -fv."));
	}
	for (k = 0; k < NFLAGS; k++) {
		if (had_flag[k]) {
			int	i;		/* for flag string cleanup */

			if (flag_p[k] || lval ) {
				if (('a' + k) == LOCKFLAG && had_flag[k] == 1) {
					if ((flag_p[k] && *flag_p[k] == 'a') || (lval && *lval == 'a'))
						locks = NOGETTEXT("a");
					else if (lval && flag_p[k]) {
						nline[0] = '\0';
						i = strlcatl(nline, sizeof (nline),
							lval, " ", flag_p[k], (char *)0);
						if (i >= sizeof (nline))
							fatal(gettext("line too long (co31)"));
						locks = nline;
					} else if (lval) {
						locks = lval;
					} else {
						locks = flag_p[k];
					}
					sprintf(line,"%c%c %c %s\n",
						CTLCHAR,FLAG,'a' + k,locks);
					locks = 0;
					if (lval) {
						ffree(lval);
						tval = lval = 0;
					}
					if (ck_it)
						had_flag[k] = ck_it = 0;
				}
				else if (flag_p[k])
					sprintf(line,"%c%c %c %s\n",
					 CTLCHAR,FLAG,'a'+k,flag_p[k]);
				     else
					sprintf(line,"%c%c %c\n",
					 CTLCHAR,FLAG,'a'+k);
			}
			else
				sprintf(line,"%c%c %c\n",
					CTLCHAR,FLAG,'a'+k);

			/* flush imbeded newlines from flag value */
			i = 4;
			if (line[i] == ' ')
				for (i++; line[i+1]; i++)
					if (line[i] == '\n')
						line[i] = ' ';
			putline(&gpkt,line);

			if (had_flag[k] == 2) {	/* flag was taken from file */
				had_flag[k] = 0;
				if (flag_p[k]) {
					ffree(flag_p[k]);
					flag_p[k] = 0;
				}
			}
		}
	}

	if (HADN) {
		if (HADI || HADB) {
			/*
			If the "encoded" flag was not present, put it in
			with a value of 0; this acts as a place holder,
			so that if we later discover that the file contains
			non-ASCII characters we can flag it as encoded
			by setting the value to 1.
			*/
			Encodeflag_offset = ftell(gpkt.p_xiop);
			sprintf(line,"%c%c %c %d\n",
				CTLCHAR, FLAG, ENCODEFLAG, Encoded);
			putline(&gpkt,line);
		}
		/*
		 * Writing out SCCS v6 flags belongs here.
		 */

		/*
		 * Since we are creating a new history file, everything is from
		 * us and every meta data needs to be written.
		 */
		putmeta(&gpkt, M_ALL);

		/*
		Beginning of descriptive (user) text.
		*/
		sprintf(line,CTLSTR,CTLCHAR,BUSERTXT);
		putline(&gpkt,line);
	} else {
		/*
		 * Check where the above loop that processes flags stopped:
		 */
		gpkt.p_wrttn = 0;
		/*
		 * Copy over SCCS v6 flags.
		 */
		while (gpkt.p_line_length > 1 &&
			    gpkt.p_line[0] == CTLCHAR &&
			    gpkt.p_line[1] == NAMEDFLAG) {
			getline(&gpkt);
		}

		/*
		 * Copy over SCCS v6 global metadata.
		 */
		while (gpkt.p_line_length > 1 &&
			    gpkt.p_line[0] == CTLCHAR &&
			    gpkt.p_line[1] == GLOBALEXTENS) {
			getline(&gpkt);
		}

		/*
		 * Write out everything until the BUSERTXT record.
		 * This includes possible future extensions.
		 */
		if (gpkt.p_line[0] == CTLCHAR && gpkt.p_line[1] == BUSERTXT)
			putline(&gpkt,(char *) 0);
		else
			flushto(&gpkt, BUSERTXT, FLUSH_COPY);
	}

	/*
	Get user description, copy to x-file.
	*/
	if (HADT) {
		if (tfile) {
		   if (*tfile) {
			iptr = xfopen(tfile, O_RDONLY|O_BINARY);
#ifdef	USE_SETVBUF
			setvbuf(iptr, NULL, _IOFBF, VBUF_SIZE);
#endif
			(void)fgetchk(iptr, tfile, &gpkt, 0);
			fclose(iptr);
			iptr = NULL;
			/*
			 * fgetchk() did set p_ghash and in case that the
			 * file has zero size or -n is used, p_ghash may never
			 * be set up again. So we need to clear it here.
			 */
			gpkt.p_ghash = 0;
		   }
		}

		/*
		If old file, ignore any previously supplied
		commentary. (i.e., don't copy it to x-file.)
		*/
		if (!HADN)
			flushto(&gpkt, EUSERTXT, FLUSH_NOCOPY);
	}

	if (HADN) {		/*   N E W  F I L E   */

		/*
		End of user description.
		*/
		sprintf(line,CTLSTR,CTLCHAR,EUSERTXT);
		putline(&gpkt,line);

		/*
		Beginning of body (text) of first delta.
		*/
		sprintf(line,"%c%c %d\n",CTLCHAR,INS,1);
		putline(&gpkt,line);

		if (HADB)
			Encoded |= EF_UUENCODE;
		if (HADI) {		/* get body */

			/*
			Set indicator to check lines of body of file for
			keyword definitions.
			If no keywords are found, a warning
			will be produced.
			*/
			check_id = 1;
			/*
			Set indicator that tells whether there
			were any keywords to 'no'.
			*/
			gpkt.p_did_id = 0;
			if (ifile) {
			   if (*ifile) {
				/* from a file */
				from_stdin = 0;
			   } else {
				/* from standard input */
				int    err = 0, cnt;
				char   buf[MAXLINE];
				FILE * out;
				mode_t cur_umask;

				from_stdin = 1;
				ifile	   = stdin_file_buf;
				strlcpy(stdin_file_buf, "/tmp/admin.XXXXXX", sizeof (stdin_file_buf));
				cur_umask = umask((mode_t)((S_IRWXU|S_IRWXG|S_IRWXO)&~(S_IRUSR|S_IWUSR)));
#ifdef	HAVE_MKSTEMP
				if ((out = fdopen(mkstemp(ifile), "wb")) == NULL) {
					xmsg(ifile, NOGETTEXT("admin"));
				}
#else
				mktemp(stdin_file_buf);
				if ((out = fopen(ifile, "wb")) == NULL) {
					xmsg(ifile, NOGETTEXT("admin"));
				}
#endif
				setmode(fileno(out), O_BINARY);
				(void)umask(cur_umask);
				/*CONSTCOND*/
				while (1) {
					if ((cnt = fread(buf, 1, sizeof (buf), stdin))) {
						if (fwrite(buf, 1, cnt, out) == cnt) {
							continue;
						}
						err = 1;
						break;
					} else {
						if (!feof(stdin)) {
							err = 1;
						}
						break;
					}
				}
				if (err) {
					unlink(ifile);
					xmsg(ifile, NOGETTEXT("admin"));
				}
				fclose(out);
			   }
			   iptr = xfopen(ifile, O_RDONLY|O_BINARY);
#ifdef	USE_SETVBUF
			   setvbuf(iptr, NULL, _IOFBF, VBUF_SIZE);
#endif
			}

			/* save an offset to x-file in case need to encode
                           file.  Then won't have to start all over.  Also
                           save the hash value up to this point.
			 */
			offset = ftell(gpkt.p_xiop);
			thash = gpkt.p_nhash;

			/*
			If we haven't already been told that the file
			should be encoded, read and copy to x-file,
			while checking for control characters (octal 1),
			and also check if file ends in newline.  If
			control char or no newline, the file needs to
			be encoded.
			Also, count lines read, and set statistics'
			structure appropriately.
			The 'fgetchk' routine will check for keywords.
			*/
			if (!HADB) {
			   stats.s_ins = fgetchk(iptr, ifile, &gpkt, 1);
			   if (stats.s_ins == -1 ) {
				Encoded |= EF_UUENCODE;
			   } else {
				Encoded &= ~EF_UUENCODE;	/* Keep EF_GZIP */
			   }
			} else {
				Encoded |= EF_UUENCODE;
			}
			if (Encoded & EF_UUENCODE) {
			   /* non-ascii characters in file, encode them */
			   iptr = code(iptr, afile, offset, thash, &gpkt);
			   stats.s_ins = fgetchk(iptr, ifile, &gpkt, 0);
			}
			if (iptr) {
				fclose(iptr);
				iptr = NULL;
			}
			stats.s_del = stats.s_unc = 0;

			/*
			 * If no keywords were found, issue warning unless in
			 * NSE mode...or warnings have been disabled via an
			 * empty 'y' flag value.
			 */
			if (!gpkt.p_did_id && !HADQ &&
			    (!flag_p[EXPANDFLAG - 'a'] ||
			    *(flag_p[EXPANDFLAG - 'a']))) {
				if (had_flag[IDFLAG - 'a']) {
					if(!(flag_p[IDFLAG -'a']))
						fatal(gettext("no id keywords (cm6)"));
					else
						fatal(gettext("invalid id keywords (cm10)"));
				} else {
					fprintf(stderr, gettext("No id keywords (cm7)\n"));
					(void) sccsfatalhelp("(cm7)");
				}
			}

			check_id = 0;
			gpkt.p_did_id = 0;
		}

		/*
		End of body of first delta.
		*/
		sprintf(line,"%c%c %d\n",CTLCHAR,END,1);
		putline(&gpkt,line);
		if (gpkt.p_flags & PF_V6 && gpkt.p_ghash != 0) {
			fseek(gpkt.p_xiop, Checksum_offset, SEEK_SET);
			fprintf(gpkt.p_xiop, "%c%c s %5.5d\n",
				CTLCHAR, SIDEXTENS, gpkt.p_ghash);
			gpkt.p_nhash -= 5 * '0';
			sprintf(line, "%5.5d", gpkt.p_ghash);
			q = (signed char *) line;
			while (*q)
				gpkt.p_nhash += *q++;
		}
	} else {
		/*
		Indicate that EOF at this point is ok, and
		flush rest of (old) s-file to x-file.
		*/
		gpkt.p_chkeof = 1;
		while (getline(&gpkt)) ;
	}

	/* If encoded file, put change "fe" flag and recalculate
	   the hash value
	 */

	if (Encoded & EF_UUENCODE)
	{
		strcpy(line,"0");
		q = (signed char *) line;
		while (*q)
			gpkt.p_nhash -= *q++;
		strcpy(line,"1");
		q = (signed char *) line;
		while (*q)
			gpkt.p_nhash += *q++;
		fseek(gpkt.p_xiop, Encodeflag_offset, SEEK_SET);
		fprintf(gpkt.p_xiop,"%c%c %c %d\n",
			CTLCHAR, FLAG, ENCODEFLAG, Encoded);
	}

	/*
	Flush the buffer, take care of rewinding to insert
	checksum and statistics in file, and close.
	*/
	org_chash = gpkt.p_chash;
	org_uchash = gpkt.p_uchash;
	flushline(&gpkt,&stats);

	/*
	Change x-file name to s-file, and delete old file.
	Unlock file before returning.
	*/
	if (!HADH) {
		if (!HADN) stat(gpkt.p_file,&sbuf);
		rename(auxf(gpkt.p_file,'x'), gpkt.p_file);
		if (!HADN) {
			chmod(gpkt.p_file, sbuf.st_mode);
			chown(gpkt.p_file,sbuf.st_uid, sbuf.st_gid);
		}
		if (HADO) {
			struct timespec	ts[2];
			extern dtime_t	Timenow;

			ts[0].tv_sec = Timenow.dt_sec;
			ts[0].tv_nsec = Timenow.dt_nsec;
			ts[1].tv_sec = ifile_mtime.tv_sec;
			ts[1].tv_nsec = ifile_mtime.tv_nsec;

			/*
			 * As SunPro make and gmake call sccs get when the time
			 * if s.file equals the time stamp of the g-file, make
			 * sure the s.file is a bit older.
			 */
			if (!(gpkt.p_flags & PF_V6)) {
				struct timespec	tn;

				getnstimeofday(&tn);
				ts[1].tv_nsec = tn.tv_nsec;
			}
			if (ts[1].tv_nsec > 500000000)
				ts[1].tv_nsec -= 500000000;

			utimensat(AT_FDCWD, gpkt.p_file, ts, 0);
		}
		if (HADI && *ifile) {
			if (N.n_comma) {
				char cfile[FILESIZE];
				register char *snp;

				snp = sname(ifile);

				strlcpy(cfile, ifile, sizeof (cfile));
				cfile[snp-ifile] = '\0';
				strlcat(cfile, ",", sizeof (cfile));
				if (strlcat(cfile, snp, sizeof (cfile)) <
				    sizeof (cfile)) {
					rename(ifile, cfile);
				}
			} else if(N.n_unlink) {
				unlink(ifile);
			} else if (N.n_get) {
				unlink(ifile);
				doget(gpkt.p_file, ifile, 1);
				if (HADO)
					dogtime(&gpkt, ifile, &ifile_mtime);
			}
		}
		xrm(&gpkt);
		timersetlockfile(NULL);
		if (!islockchset(Zhold))
			unlockit(Zhold, getpid(), uuname);

		if ((gpkt.p_flags & PF_V6) && Cs && gpkt.p_init_path) {
			char	cbuf[2*MAXPATHLEN];

			change_ba(&gpkt, cbuf, sizeof (cbuf));
			fprintf(Cs, "%s\n", cbuf);
		}
	}

	if (HADI)
		unlink(auxf(gpkt.p_file,'e'));
	if (from_stdin) {
		unlink(stdin_file_buf);
		stdin_file_buf[0] = '\0';
	}
	if (gpkt.p_init_path) {
		if (gpkt.p_init_path != X.x_init_path)
			ffree(gpkt.p_init_path);
		gpkt.p_init_path = NULL;
	}
	sclose(&gpkt);
	sfree(&gpkt);
	ffreeall();
}

static int
fgetchk(inptr, file, pkt, fflag)
	FILE	*inptr;		/* File pointer to read from	*/
	char	*file;		/* File name to read from	*/
struct	packet	*pkt;		/* struct paket for output	*/
	int	fflag;		/* 0 = abort, 1 == flag		*/
{
	off_t	nline;
	int	idx = 0;
	int	warned = 0;
	char	chkflags = 0;
	char	lastchar;
#ifndef	RECORD_IO
	char	*p = NULL;	/* Intialize to make gcc quiet */
	char	*pn =  NULL;	/* Pointer to nul character */
	char	line[VBUF_SIZE+1];
	char	*lastline = line; /* Init to make GCC quiet */
#else
	int	search_on = 0;
	char	line[256];	/* Avoid a too long buffer for speed */
#endif
	off_t	ibase = 0;	/* Ifile off from last read operation	*/
	off_t	ioff = 0;	/* Ifile offset past last newline	*/
	off_t	soff = ftell(pkt->p_xiop); /* Ofile (s. file) base offset */
	unsigned int sum = 0;

	/*
	 * This gives the illusion that a zero-length file ends
	 * in a newline so that it won't be mistaken for a
	 * binary file.
	 */
	lastchar = '\n';

	nline = 0;
	(void)memset(line, '\377', sizeof (line));
#ifndef	RECORD_IO
	/*
	 * In most cases (non record oriented I/O), we can optimize the way we
	 * scan files for '\0' bytes, line-ends '\n' and ^A '\1'. The optimized
	 * algorithm allows to avoid to do a reverse scan for '\0' from the end
	 * of the buffer.
	 */
	while ((idx = fread(line, 1, sizeof (line) - 1, inptr)) > 0) {
		sum += usum(line, idx);
		lastline = line;
		if (lastchar == '\n' && line[0] == CTLCHAR) {
			chkflags |= CK_CTLCHAR;
			if (fflag && (pkt->p_flags & PF_V6)) {
				if (!warned) {
					warnctl(file, nline+1);
					warned = 1;
				}
				putctl(pkt);
			} else {
				goto err;
			}
		}
		lastchar = line[idx-1];
		p = findbytes(line, idx, '\0');
		if (p != NULL) {
			chkflags |= CK_NULL;
			pn = p;
		}
		for (p = line;
		    (p = findbytes(p, idx - (p-line), '\n')) != NULL; p++) {
			ioff = ibase + (p - line) + 1;
			if (pn && p > pn)
				goto err;
			nline++;
			if ((p - line) >= (idx-1))
				break;

			if (p[1] == CTLCHAR) {
				chkflags |= CK_CTLCHAR;
				if (fflag && (pkt->p_flags & PF_V6) &&
				    (chkflags & CK_NULL) == 0) {
					if (!warned) {
						warnctl(file, nline+1);
						warned = 1;
					}
					p[1] = '\0';
					putline(pkt, lastline);
					p[1] = CTLCHAR;
					lastline = &p[1];
					putctl(pkt);
					continue;
				}
	err:
				if (fflag) {
					return(-1);
				} else {
					sprintf(SccsError,
					gettext(
			  "file '%s' contains illegal data on line %jd (ad21)"),
					file, (Intmax_t)++nline);
					fatal(SccsError);
				}
			}
		}
		line[idx] = '\0';
		putline(pkt, lastline);

		if (check_id && pkt->p_did_id == 0) {
			pkt->p_did_id =
				chkid(line, flag_p[IDFLAG - 'a'], flag_p);
		}
		ibase += idx;
	}
#else	/* !RECORD_IO */
	while (fgets(line, sizeof (line), inptr) != NULL) {
	   if (lastchar == '\n' && line[0] == CTLCHAR) {
		chkflags |= CK_CTLCHAR;
		if (fflag && (pkt->p_flags & PF_V6)) {
			if (!warned) {
				warnctl(file, nline+1);
				warned = 1;
			}
			putctl(pkt);
		} else {
			nline++;
			goto err;
		}
	   }
	   search_on = 0;
	   for (idx = sizeof (line)-1; idx >= 0; idx--) {
	      if (search_on > 0) {
		 if (line[idx] == '\0') {
		    chkflags |= CK_NULL;
	err:
		    if (fflag) {
		       return(-1);
		    } else {
		       sprintf(SccsError,
			 gettext("file '%s' contains illegal data on line %jd (ad21)"),
			 file, (Intmax_t)nline);
		       fatal(SccsError);
		    }
		 }
	      } else {
		 if (line[idx] == '\0') {
		    sum += usum(line, idx);
		    search_on = 1;
		    lastchar = line[idx-1];
		    if (lastchar == '\n') {
		       nline++;
		       ioff = ibase + idx;

		    }
		    ibase += idx;
		 }
	      }
	   }
	   if (check_id && pkt->p_did_id == 0) {
		pkt->p_did_id =
			chkid(line, flag_p[IDFLAG - 'a'], flag_p);
	   }
	   putline(pkt, line);
	   (void)memset(line, '\377', sizeof (line));
	}
#endif	/* !RECORD_IO */
	pkt->p_ghash = sum & 0xFFFF;
	if (lastchar != '\n')
		chkflags |= CK_NONL;
	pkt->p_props |= chkflags;

	if (chkflags & CK_NONL) {
#ifndef	RECORD_IO
		if (pn)		/* Found null byte but no newline past null */
			goto err;
#endif
		if (fflag && (pkt->p_flags & PF_V6)) {
			fseek(inptr, ioff, SEEK_SET);
			fseek(pkt->p_xiop, ioff + soff, SEEK_SET);

			putctlnnl(pkt);
			/*
			 * Write again already written text without recomputing
			 * the checksum for this part of the text.
			 */
			while ((idx =
			    fread(line, 1, sizeof (line) - 1, inptr)) > 0) {
				if (fwrite(line, 1, idx, pkt->p_xiop) <= 0)
					FAILPUT;
			}
			putchr(pkt, '\n');
			fprintf(stderr, gettext(
			    "WARNING [%s%s%s]: No newline at end of file (ad31)\n"),
				dir_name,
				*dir_name?"/":"",
				file);
			(void) sccsfatalhelp("(ad31)");
			return (nline);
		}

	   if (fflag) {
	      return(-1);
	   } else {
	      sprintf(SccsError,
		gettext("No newline at end of file '%s' (ad31)"),
		file);
	      fatal(SccsError);
	   }
	}
	return(nline);
}

static void
warnctl(file, nline)
	char	*file;
	off_t	nline;
{
	fprintf(stderr,
		gettext(
		"WARNING [%s%s%s]: line %jd begins with ^A\n"),
		dir_name,
		*dir_name?"/":"",
		file, (Intmax_t)nline);
}

static void
clean_up()
{
	xrm(&gpkt);
	if(gpkt.p_file[0]) {
		unlink(auxf(gpkt.p_file,'x'));
		if (HADI)
			unlink(auxf(gpkt.p_file,'e'));
		if (HADN)
			unlink(gpkt.p_file);
	}
	if (gpkt.p_init_path) {
		if (gpkt.p_init_path != X.x_init_path)
			ffree(gpkt.p_init_path);
		gpkt.p_init_path = NULL;
	}
	if (!HADH) {
		uname(&un);
		uuname = un.nodename;
		timersetlockfile(NULL);
		if (!islockchset(Zhold))
			unlockit(Zhold, getpid(), uuname);
	}
	sclose(&gpkt);
	sfree(&gpkt);
	ffreeall();
}

static void
cmt_ba(dt,str, flags)
register struct deltab *dt;
char *str;
int flags;
{
	register char *p;

	p = str;
	*p++ = CTLCHAR;
	*p++ = COMMENTS;
	*p++ = ' ';
	copy(NOGETTEXT("date and time created"),p);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	/*
	 * The s-file is not part of the POSIX standard. For this reason, we
	 * are free to switch to a 4-digit year for the initial comment.
	 */
	if ((flags & PF_V6) ||
	    (dt->d_dtime.dt_sec < Y1969) ||
	    (dt->d_dtime.dt_sec >= Y2038))		/* comment only */
		date_bazl(&dt->d_dtime,p, flags);	/* 4 digit year */
	else
		date_ba(&dt->d_dtime.dt_sec,p, flags);	/* 2 digit year */
	while (*p++)
		;
	--p;
	*p++ = ' ';
	copy(NOGETTEXT("by"),p);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	copy(dt->d_pgmr,p);
	while (*p++)
		;
	--p;
	*p++ = '\n';
	*p = 0;
}

static void
putmrs(pkt)
struct packet *pkt;
{
	register char **argv;
	char str[64];
	extern char **Varg;

	for (argv = &Varg[VSTART]; *argv; argv++) {
		sprintf(str,"%c%c %s\n",CTLCHAR,MRNUM,*argv);
		putline(pkt,str);
	}
}


static char*
adjust(line)
char	*line;
{
	register int k;
	register int i;
	char	*t_unlock;
	char	t_line[MAXLINE];
	char	rel[5];

	t_unlock = unlockarg;
	while(*t_unlock) {
		NONBLANK(t_unlock);
		t_unlock = getval(t_unlock,rel);
		while ((k = pos_ser(line,rel)) != -1) {
			for(i = k; i < ((int) size(rel) + k); i++) {
				line[i] = '+';
				if (line[i++] == ' ')
					line[i] = '+';
				else if (line[i] == '\0')
					break;
				else --i;
			}
			k = 0;
			for(i = 0; i < (int) length(line); i++) {
				if (line[i] == '+')
					continue;
				else if (k == 0 && line[i] == ' ')
					continue;
				else t_line[k++] = line[i];
			}
			t_line[k] = '\0';
			if (k > 0 &&
			    t_line[(int) strlen(t_line) - 1] == ' ')
				t_line[(int) strlen(t_line) - 1] = '\0';
			line = t_line;
		}
	}
	if (length(line) == 0)
		return (0);
	if (line == t_line) {
		t_unlock = fmalloc(size(line));
		copy(line, t_unlock);
		return (t_unlock);
	}
	return (line);
}

static char*
getval(sourcep,destp)
register char	*sourcep;
register char	*destp;
{
	while (*sourcep != ' ' && *sourcep != '\t' && *sourcep != '\0')
		*destp++ = *sourcep++;
	*destp = 0;
	return(sourcep);
}

static int
val_list(list)
register char *list;
{
	register int i;

	if (list[0] == 'a')
		return(1);
	else for(i = 0; list[i] != '\0'; i++)
		if (list[i] == ' ' || numeric(list[i]))
			continue;
		else if (list[i] == 'a') {
			list[0] = 'a';
			list[1] = '\0';
			return(1);
		}
		else return(0);
	return(1);
}

static int
pos_ser(s1,s2)
char	*s1;
char	*s2;
{
	register int offset;
	register char *p;
	char	num[5];

	p = s1;
	offset = 0;

	while(*p) {
		NONBLANK(p);
		p = getval(p,num);
		if (equal(num,s2)) {
			return(offset);
		}
		offset = offset + (int) size(num);
	}
	return(-1);
}

static int
range(line)
register char *line;
{
	register char *p;
	char	rel[MAXLINE];

	p = line;
	while(*p) {
		NONBLANK(p);
		p = getval(p,rel);
		if ((int) size(rel) > 5)
			return(0);
	}
	return(1);
}

static FILE *
code(iptr,afile,offset,thash,pktp)
FILE *iptr;
char *afile;
off_t offset;
int thash;
struct packet *pktp;
{
	FILE *eptr;


	/* issue a warning that file is non-text */
	if (!HADB) {
		fprintf(stderr, gettext("WARNING [%s%s%s]: %s"),
			dir_name,
			*dir_name?"/":"",
			ifile,
			gettext("Not a text file (ad32)\n"));
			(void) sccsfatalhelp("(ad32)");
	}
	rewind(iptr);
	eptr = fopen(auxf(afile,'e'), "wb");

	encode(iptr,eptr);
	fclose(eptr);
	fclose(iptr);
	iptr = fopen(auxf(afile,'e'), "rb");
	/* close the stream to xfile and reopen it at offset.  Offset is
	 * the end of sccs header info and before gfile contents
	 */
	putline(pktp,0);
	fseek(pktp->p_xiop, offset, SEEK_SET);
	pktp->p_nhash = thash;

	return (iptr);
}

static void
direrror(dir, keylet)
	char	*dir;
	int	keylet;
{
	sprintf(SccsError,
	gettext("directory `%s' specified as `%c' keyletter value (ad29)"),
		dir, (char)keylet);
	fatal(SccsError);
}
