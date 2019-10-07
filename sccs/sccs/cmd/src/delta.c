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
 * Copyright 2006-2019 J. Schilling
 *
 * @(#)delta.c	1.86 19/09/23 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)delta.c 1.86 19/09/23 J. Schilling"
#endif
/*
 * @(#)delta.c 1.40 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)delta.c"
#pragma ident	"@(#)sccs:cmd/delta.c"
#endif

# define	NEED_PRINTF_J		/* Need defines for js_snprintf()? */
# include	<schily/resource.h>
# include	<defines.h>
# include	<version.h>
# include	<had.h>
# include	<i18n.h>
# include	<schily/utsname.h>
# include	<schily/fcntl.h>
# include	<schily/wait.h>
# define	VMS_VFORK_OK
# include	<schily/vfork.h>
# include	<ccstypes.h>
# include	<schily/sysexits.h>

# define	LENMR	60

static FILE	*Diffin, *Gin;
static Nparms	N;			/* Keep -N parameters		*/
static Xparms	X;			/* Keep -X parameters		*/
static struct packet	gpkt;
static struct utsname	un;
static int	num_files;
static int	number_of_lines;
static off_t	size_of_file;
static off_t	Szqfile;
static off_t	Checksum_offset;
#if	defined(PROTOTYPES) && defined(INS_BASE)
static char	BDiffpgmp[]  =   NOGETTEXT(INS_BASE "/" SCCS_BIN_PRE "bin/" "bdiff");
#else
/*
 * XXX If you are using a K&R compiler and like to install to a path
 * XXX different from "/usr/ccs/bin/", you need to edit this string.
 */
static char	BDiffpgmp[]  =   NOGETTEXT("/usr/ccs/bin/bdiff");
#endif
static char	BDiffpgm[]  =   NOGETTEXT("/usr/bin/bdiff");
static char	BDiffpgm2[]  =   NOGETTEXT("/bin/bdiff");
#if	defined(PROTOTYPES) && defined(INS_BASE)
static char	Diffpgmp[]  =   NOGETTEXT(INS_BASE  "/" SCCS_BIN_PRE "bin/" "diff");
#else
static char	Diffpgmp[]  =   NOGETTEXT("/usr/ccs/bin/diff");
#endif
static char	Diffpgm[]   =   NOGETTEXT("/usr/bin/diff");
static char	Diffpgm2[]   =   NOGETTEXT("/bin/diff");
static char	*diffpgm = "";
static char	*ilist, *elist, *glist, Cmrs[300], *Nsid;
static char	Pfilename[FILESIZE];
static char	*uuname;
static char	*Cwd = "";
static char	*Dfilename;

static struct timespec	gfile_mtime;	/* Timestamp for -o		*/
static	time_t	cutoff = MAX_TIME;

static struct sid sid;

static	void    clean_up __PR((void));
static	void	enter	__PR((struct packet *pkt, int ch, int n, struct sid *sidp));

	int	main __PR((int argc, char **argv));
static void	delta __PR((char *file));
static int	mkdelt __PR((struct packet *pkt, struct sid *sp, struct sid *osp,
				int diffloop, int orig_nlines));
static void	mkixg __PR((struct packet *pkt, int reason, int ch));
static void	putmrs __PR((struct packet *pkt));
static void	putcmrs __PR((struct packet *pkt));
static struct pfile *rdpfile __PR((struct packet *pkt, struct sid *sp));
static FILE *	dodiff __PR((char *newf, char *oldf, int difflim));
static int	getdiff __PR((char *type, int *plinenum));
static void	insert __PR((struct packet *pkt, int linenum, int n, int ser, int off));
static void	delete __PR((struct packet *pkt, int linenum, int n, int ser));
static void	after __PR((struct packet *pkt, int n));
static void	before __PR((struct packet *pkt, int n));
static char *	linerange __PR((char *cp, int *low, int *high));
static void	skipline __PR((char *lp, int num));
static char *	rddiff __PR((char *s, int n));
static void	fgetchk __PR((char *file, struct packet *pkt));
static void	warnctl __PR((char *file, off_t nline));
static void 	fixghash __PR((struct packet *pkt, int ser));

extern int	org_ihash;
extern int	org_chash;
extern int	org_uchash;

int
main(argc,argv)
int argc;
register char *argv[];
{
	register int i;
	register char *p;
	int no_arg, c;
	extern int Fcnt;
	int current_optind;

	/*
	 * Set locale for all categories.
	 */
	setlocale(LC_ALL, "");
	
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

	set_clean_up(clean_up);
	Fflags = FTLEXIT | FTLMSG | FTLCLN;
#ifdef	SCCS_FATALHELP
	Fflags |= FTLFUNC;
	Ffunc = sccsfatalhelp;
#endif

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
		        c = getopt(argc, argv, "()-r:dpsnm:g:y:fhoqkzC:D:N:X:V(version)");

				/* this takes care of options given after
				** file names.
				*/
			if(c == EOF) {
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
			switch (c) {

			case 'r':
				if (*p == 0) continue;
				chksid(sid_ab(p,&sid),&sid);
				break;
			case 'g':
				glist = p;
				break;
			case 'y':
				if (optarg == argv[i+1]) {
				   Comments = "";
				   no_arg = 1;
				} else {  
				   Comments = p;
				}
				break;
			case 'm':
				Mrs = p;
				repl(Mrs,'\n',' ');
				break;
 			case 'd':
			case 'p':
			case 'n':
			case 's':
                        case 'f': /* force delta without p. file (NSE only) */
				if (p) {
				   if (*p) {
				      sprintf(SccsError,
					gettext("value after %c arg (cm7)"),
					c);
				      fatal(SccsError);
				   }
				}
				break;
			case 'h': /* allow diffh for large files (NSE only) */
			case 'k': /* get(1) without keyword expand */
			case 'o': /* use original file date */
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
			case 'z':
				break;
			case 'C':
				Cwd = p;
				break;
			case 'D':
				Dfilename = p;
				break;

			case 'N':	/* Bulk names */
				initN(&N);
				if (optarg == argv[i+1]) {
				   no_arg = 1;
				   break;
				}
				N.n_parm = p;
				break;

			case 'X':
				X.x_parm = optarg;
				X.x_flags = XO_PREPEND_FILE;
				if (!parseX(&X))
					goto err;
				break;

			case 'V':		/* version */
				printf(gettext(
				    "delta %s-SCCS version %s %s (%s-%s-%s)\n"),
					PROVIDER,
					VERSION,
					VDATE,
					HOST_CPU, HOST_VENDOR, HOST_OS);
				exit(EX_OK);

			default:
			err:
				fatal(gettext("Usage: delta [ -dknops ][ -g sid-list ][ -m mr-list ][ -r SID ]\n\t[ -y[comment] ][ -D diff-file ][ -N[bulk-spec]][ -Xxopts ] s.filename..."));
			}

			/*
			 * Make sure that we only collect option letters from
			 * the range 'a'..'z' and 'A'..'Z'.
			 */
			if (ALPHA(c) &&
			    (had[LOWER(c)? c-'a' : NLOWER+c-'A']++))
				fatal(gettext("key letter twice (cm2)"));
	}
#ifdef	NO_BDIFF
#if	!(defined(SVR4) && defined(sun))
	had['d' - 'a'] = 1;			/* Do not use "bdiff" */
#endif
#endif

	for(i=1; i<argc; i++){
		if(argv[i]) {
		       num_files++;
		}
	}
        if ((HADF || HADH) && !HADQ) {
                fatal(gettext("unknown key letter (cm1)"));
        }
 	if (num_files == 0) {
 	   if (HADD != 0) Fflags &= ~FTLEXIT;
 	   fatal(gettext("missing file arg (cm3)"));
 	   exit(2);
 	}

	setsig();
	xsethome(NULL);
	if (HADUCN) {					/* Parse -N args  */
		parseN(&N);

		if (N.n_sdot && (sethomestat & SETHOME_OFFTREE))
			fatal(gettext("-Ns. not supported in off-tree project mode"));
	}

	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if ((p=argv[i]) != NULL)
			do_file(p, delta, 1, N.n_sdot);

	return (Fcnt ? 1 : 0);
}

/*
 * Create the delta for one file
 */
static void
delta(file)
char *file;
{
	static int first = 1;
	int n, linenum;
	int	ghash;
	char type;
	register int ser;
	extern char had_dir, had_standinp;
	char nsid[SID_STRSIZE];
	char dfilename[FILESIZE];
	char gfilename[FILESIZE];
	char *ifile;
	char line[BUFSIZ];		/* Buffer for uuencoded IO, see below */
	struct stats stats;
	struct pfile *pp = NULL;
	struct stat sbuf;
	int inserted, deleted, orig;
	int newser;
	uid_t holduid;
	gid_t holdgid;
	FILE *efp;
	int status;
	int diffloop;
	int difflim;

	Gin = NULL;
	if (setjmp(Fjmp))
		return;

	if (HADUCN) {
		char	*ofile = file;

		file = bulkprepare(&N, file);
		if (file == NULL) {
			if (N.n_ifile)
				ofile = N.n_ifile;
			fatal(gettext("directory specified as s-file (cm14)"));
		}
		ifile = N.n_ifile;
	} else {
		ifile = NULL;
	}

	/*
	 * Init and check for validity of file name but do not open the file.
	 * This prevents us from potentially damaging files with lockit().
	 */
	sinit(&gpkt, file, SI_INIT);

	uname(&un);
	uuname = un.nodename;
	if (lockit(auxf(gpkt.p_file,'z'),SCCS_LOCK_ATTEMPTS,getpid(),uuname))
		efatal(gettext("cannot create lock file (cm4)"));

	sinit(&gpkt, file, SI_OPEN);
	gpkt.p_enter = enter;
	if ((gpkt.p_flags & PF_V6) == 0)
		gpkt.p_flags |= PF_GMT;
	if (first) {
		first = 0;
		dohist(file);
	}
	gpkt.p_reopen = 1;
	gpkt.p_stdout = stdout;
	gfilename[0] = '\0';
	strlcatl(gfilename, sizeof (gfilename),
		Cwd,
		ifile ? ifile :
			auxf(gpkt.p_file, 'g'),
		(char *)0);
	Gin = xfopen(gfilename, O_RDONLY|O_BINARY);
#ifdef	USE_SETVBUF
	setvbuf(Gin, NULL, _IOFBF, VBUF_SIZE);
#endif
	Pfilename[0] = '\0';
	if (!HADF || exists(auxf(gpkt.p_file,'p')))
		pp = rdpfile(&gpkt, &sid);

	Cmrs[0] = 0;
	if (pp != NULL && pp->pf_cmrlist != NULL)
		strlcpy(Cmrs, pp->pf_cmrlist, sizeof (Cmrs));

	if (dodelt(&gpkt,&stats,(struct sid *) 0,0) == 0)
		fmterr(&gpkt);

        if ((HADF) && !exists(auxf(gpkt.p_file,'p'))) {
                /* if no p. file exists delta can still happen if
                 * -f flag given (in NSE mode) - uses the same logic
                 * as get -e to assign a new SID */
                gpkt.p_reqsid.s_rel = 0;
                gpkt.p_reqsid.s_lev = 0;
                gpkt.p_reqsid.s_br = 0;
                gpkt.p_reqsid.s_seq = 0;
                gpkt.p_cutoff = cutoff;
                ilist = 0;
                elist = 0;
                ser = getser(&gpkt);
                newsid(&gpkt, 0);	/* sets gpkt.p_reqsid */
        } else {        
                gpkt.p_cutoff = pp->pf_date;
                ilist = pp->pf_ilist;
                elist = pp->pf_elist;
                if ((ser = sidtoser(&pp->pf_gsid,&gpkt)) == 0 ||
                        sidtoser(&pp->pf_nsid,&gpkt))
                                fatal(gettext("invalid sid in p-file (de3)"));
		gpkt.p_reqsid = pp->pf_nsid;
        }
	sid_ba(&gpkt.p_reqsid, nsid);
	Nsid=nsid;
	gfile_mtime.tv_sec = gfile_mtime.tv_nsec = 0;
        if ((HADO || HADQ) && stat(gfilename, &sbuf) == 0) {
                /*
		 * When specifying -o (original date) and when
		 * in NSE mode, the mtime of the clear file is remembered for
                 * use as delta time. Sccs is thus now vulnerable to clock
                 * skew between NFS server and host machine and to a mis-set
                 * clock when file is last changed.
		 * In non-original date mode, sccs is vulnerable to a mis-set of
		 * the local clock while calling 'delta'.
                 */
                gfile_mtime.tv_sec = sbuf.st_mtime;
                gfile_mtime.tv_nsec = stat_mnsecs(&sbuf);
	}

	doie(&gpkt,ilist,elist,glist);
	setup(&gpkt,ser);
	finduser(&gpkt);
	doflags(&gpkt);
	permiss(&gpkt);
	donamedflags(&gpkt);
	dometa(&gpkt);
	flushto(&gpkt, EUSERTXT, FLUSH_NOCOPY);
	gpkt.p_chkeof = 1;
	/* if encode flag is set, encode the g-file before diffing it
	 * with the s.file
	 */
	if (gpkt.p_encoding & EF_UUENCODE) {
		efp = xfcreat(auxf(gpkt.p_file,'e'),0644);
#ifdef	USE_SETVBUF
		setvbuf(efp, NULL, _IOFBF, VBUF_SIZE);
#endif
		encode(Gin,efp);
		fclose(efp);
		if (Gin)
			fclose(Gin);
		Gin = xfopen(auxf(gpkt.p_file,'e'), O_RDONLY|O_BINARY);
#ifdef	USE_SETVBUF
		setvbuf(Gin, NULL, _IOFBF, VBUF_SIZE);
#endif
	}

	dfilename[0] = '\0';
	if ((X.x_opts & XO_PREPEND_FILE) == 0) {
		copy(auxf(gpkt.p_file,'d'),dfilename);
		gpkt.p_gout = xfcreat(dfilename,(mode_t)0444);
#ifdef	USE_SETVBUF
		setvbuf(gpkt.p_gout, NULL, _IOFBF, VBUF_SIZE);
#endif
		while(readmod(&gpkt)) {
			if (gpkt.p_flags & PF_NONL)
				gpkt.p_line[gpkt.p_line_length-1] = '\0';
			if(fputs(gpkt.p_lineptr, gpkt.p_gout) == EOF)
				xmsg(dfilename, NOGETTEXT("delta"));
		}
		if (fflush(gpkt.p_gout) == EOF)
			xmsg(dfilename, NOGETTEXT("delta"));
#ifdef	HAVE_FSYNC
		if (fsync(fileno(gpkt.p_gout)) < 0)
			xmsg(dfilename, NOGETTEXT("delta"));
#endif
		if (fclose(gpkt.p_gout) == EOF)
			xmsg(dfilename, NOGETTEXT("delta"));
		gpkt.p_gout = NULL;
		orig = gpkt.p_glnno;
	} else {
		/*
		 * We don't count the previous version and we
		 * do not have real diffs, so we need to use
		 * an estimation.
		 */
		orig = stats.s_ins + stats.s_unc - stats.s_del;
	}

	gpkt.p_glnno = 0;
	gpkt.p_verbose = (HADS) ? 0 : 1;
	gpkt.p_did_id = 0;
 	number_of_lines = size_of_file = 0;

	gpkt.p_ghash = 0;	/* Reset ghash from previous readmod() calls */

	if (gpkt.p_sflags[EXPANDFLAG - 'a'] &&
	    *(gpkt.p_sflags[EXPANDFLAG - 'a']) == '\0') {
		gpkt.p_did_id = 1;	/* No need to scan for keywds */
	}
	if ((gpkt.p_encoding & EF_UUENCODE) == 0) {
		/*
		 * Compute the checksum and scan for unsupported characters.
		 * This method supports nul bytes.
		 */
		fgetchk(gfilename, &gpkt);
		number_of_lines = gpkt.p_glines;
	} else {
		/*
		 * This does not support nul bytes, but this is uuencoded text.
		 */
	 	while (fgets(line,sizeof(line),Gin) != NULL) {
			register int	len = strlen(line);
			register char	**sflags = gpkt.p_sflags;

			gpkt.p_ghash += usum(line, len);
 			if (line[len-1] == '\n') {
 	   			number_of_lines++;
			}
			if (gpkt.p_did_id == 0) {
				gpkt.p_did_id = chkid(line,
							sflags[IDFLAG - 'a'],
							sflags);
			}
		}
	}
	gpkt.p_ghash &= 0xFFFF;
	if (stat(gfilename, &Statbuf) == 0) {
		size_of_file = Statbuf.st_size;
	}
	if ((X.x_opts & XO_PREPEND_FILE) == 0) {
		if (Gin)
			fclose(Gin);
		Gin = NULL;
	}
	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
 	   fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
 	if (HADD != 0) {
 	   if (number_of_lines > 70000 && size_of_file > 3670000) {
 	      fprintf(stderr, 
 	         gettext("Warning: the file is greater than 70000 lines and 3.5Mb\n"));
 	   } else {
 	      if (size_of_file > 5872000) {
 	         fprintf(stderr, 
 	            gettext("Warning: the file is greater than 5.6Mb\n"));
 	      }
 	   }
 	}

	if (!gpkt.p_did_id && !HADQ &&
	    (!gpkt.p_sflags[EXPANDFLAG - 'a'] ||
	    *(gpkt.p_sflags[EXPANDFLAG - 'a']))) {
		if (gpkt.p_sflags[IDFLAG - 'a']) {
			if(!(*gpkt.p_sflags[IDFLAG - 'a']))
				fatal(gettext("no id keywords (cm6)"));
			else
				fatal(gettext("invalid id keywords (cm10)"));
		} else if (gpkt.p_verbose) {
			fprintf(stderr,gettext("No id keywords (cm7)\n"));
		}
	}

	/*
	The following while loop executes 'bdiff' on g-file and
	d-file. If 'bdiff' fails (usually because segmentation
	limit it is using is too large for 'diff'), it is
	invoked again, with a lower segmentation limit.
	*/
	difflim = 24000;
	diffloop = 0;
	ghash = gpkt.p_ghash;			/* Save ghash value */

	if (X.x_opts & XO_PREPEND_FILE) {
		int	oihash = gpkt.p_ihash;	/* Remember hash from sinit() */

		grewind(&gpkt);
		gpkt.do_chksum = 1;		/* No old g-file, do it now */
		gpkt.p_ihash = oihash;		/* Restore hash */
		if (gpkt.p_flags & PF_V6) {
			/*
			 * We do not read the "current" file as it does not yet
			 * exist, but we compute the sum of the hashes from the
			 * old file and the new beginning that is a file with
			 * the name of the g-file.
			 */
			if (gpkt.p_hash != NULL) {
				ghash += gpkt.p_hash[ser];
				ghash &= 0xFFFF;
			}
		}
	}

	/*CONSTCOND*/
	while (1) {
		inserted = deleted = 0;
		gpkt.p_glnno = 0;
		gpkt.p_upd = 1;
		gpkt.p_wrttn = 1;
		getline(&gpkt);		/* Read the magic line */
		gpkt.p_chash = 0; 	/* Reset signed hash */
		gpkt.p_uchash = 0; 	/* Reset unsigned hash */
		gpkt.p_wrttn = 1;
		gpkt.p_ghash = ghash;	/* ghash may be destroyed in loop */
		if (glist && gpkt.p_flags & PF_V6) {
			gpkt.p_ghash = 0; /* write 00000 before correcting */
		}
        	if (HADF) {
                	newser = mkdelt(&gpkt, &gpkt.p_reqsid, &gpkt.p_gotsid, 
				diffloop, orig);
       		 } else {
                	newser = mkdelt(&gpkt,&pp->pf_nsid,&pp->pf_gsid, 
				diffloop, orig);
        	}
		diffloop = 1;
		flushto(&gpkt, EUSERTXT, FLUSH_COPY);

		if (X.x_opts & XO_PREPEND_FILE) {
			/*
			 * Since we do not call "diff", we come here only once.
			 */
			Diffin = Gin;
			rewind(Diffin);
			Gin = NULL;
			inserted += number_of_lines;
			insert(&gpkt, 0, number_of_lines, newser, 0);
			status = 0;		/* Causes a break from while */
		} else {
			if (gpkt.p_encoding & EF_UUENCODE) {
				Diffin = dodiff(auxf(gpkt.p_file,'e'),
						dfilename, difflim);
			} else {
				Diffin = dodiff(gfilename, dfilename, difflim);
			}
			type = 0;			/* Make GCC quiet */
			while ((n = getdiff(&type,&linenum)) != 0) {
				if (type == INS) {
					inserted += n;
					insert(&gpkt, linenum , n, newser, 1);
				} else {
					deleted += n;
					delete(&gpkt,linenum,n,newser);
				}
			}
		}
		if (Diffin)
			fclose(Diffin);
		Diffin = NULL;
		if (gpkt.p_iop)
			while (readmod(&gpkt))
				;
		if ((X.x_opts & XO_PREPEND_FILE) == 0)
			wait(&status);
 		/*
 		 Check top byte (exit code of child).
 		*/

 		/*
		 * 32 is the exit code below after a failed execlp() call.
 		 */
 		if (WEXITSTATUS(status) == 32) { /* 'execl' failed */
 		   sprintf(SccsError,
 		      gettext("cannot execute '%s' (de12)"), diffpgm);
 		   fatal(SccsError);
 		}
 		if ((status != 0) && (HADD == 0)) { /* diff failed */
			/*
			Re-try.
			*/
			if (difflim -= 3000) {	/* reduce segmentation */
				fprintf(stderr,
					gettext("'%s' failed, re-trying, segmentation = %d (de13)\n"),
 					diffpgm,
					difflim);
				xrm(&gpkt);		/* Close x-file */
				/*
				 * Re-open s-file.
				 */
				sclose(&gpkt);
				gpkt.p_iop = xfopen(gpkt.p_file, O_RDONLY|O_BINARY);
#ifdef	USE_SETVBUF
				setvbuf(gpkt.p_iop, NULL, _IOFBF, VBUF_SIZE);
#else
				setbuf(gpkt.p_iop, gpkt.p_buf);
#endif
				/*
				Reset counters.
				*/
				org_ihash = gpkt.p_ihash;
				org_chash = gpkt.p_chash;
				org_uchash = gpkt.p_uchash;
				gpkt.p_slnno = 0;
				gpkt.p_ihash = 0;
				gpkt.p_chash = 0;
				gpkt.p_uchash = 0;
				gpkt.p_nhash = 0;
				gpkt.p_keep = 0;
			}
			else
				/* tried up to 500 lines, can't go on */
/*
TRANSLATION_NOTE
"diff" refers to the UNIX "diff" program, used by this SCCS "delta"
command, to check the differences found between two files.
*/
				fatal(gettext("diff failed (de4)"));
		} else {		/* no need to try again, worked */
			break;			/* exit while loop */
		}
	}
	if (gpkt.p_encoding & EF_UUENCODE) {
		unlink(auxf(gpkt.p_file,'e'));
	}
	if (dfilename[0]) {
		unlink(dfilename);
		dfilename[0] = '\0';
	}
	/*
	 * If we had a glist, the checked out file will differ from the checked
	 * in file. Check out the real new content and recompute + correct the
	 * ghash.
	 */
	if (glist && gpkt.p_flags & PF_V6)
		fixghash(&gpkt, newser);

	stats.s_ins = inserted;
	stats.s_del = deleted;
	stats.s_unc = orig - deleted;
	if (gpkt.p_verbose) {
		fprintf(gpkt.p_stdout, gettext("%d inserted\n"), stats.s_ins);
		fprintf(gpkt.p_stdout, gettext("%d deleted\n"), stats.s_del);
		fprintf(gpkt.p_stdout, gettext("%d unchanged\n"),stats.s_unc);
	}
	flushline(&gpkt,&stats);
	stat(gpkt.p_file,&sbuf);
	rename(auxf(gpkt.p_file,'x'),gpkt.p_file);
	chmod(gpkt.p_file, (unsigned int)sbuf.st_mode);

	chown(gpkt.p_file, (unsigned int)sbuf.st_uid,
			(unsigned int)sbuf.st_gid);
	if (HADO) {
		struct timespec	ts[2];
		extern dtime_t	Timenow;

		ts[0].tv_sec = Timenow.dt_sec;
		ts[0].tv_nsec = Timenow.dt_nsec;
		ts[1].tv_sec = gfile_mtime.tv_sec;
		ts[1].tv_nsec = gfile_mtime.tv_nsec;

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
	if (!HADF || Pfilename[0] != '\0') {
		char	*qfile;
		
		if (exists(qfile = auxf(gpkt.p_file, 'q'))) {
			Szqfile = Statbuf.st_size;
		}
		if (Szqfile) {
			rename(qfile, Pfilename);
		}
		else {
			xunlink(Pfilename);
		}
	}
	sclose(&gpkt);
	clean_up();
	if (!HADN) {
		fflush(gpkt.p_stdout);
		holduid=geteuid();
		holdgid=getegid();
		setuid(getuid());
		setgid(getgid());
		unlink(gfilename);
		if (N.n_get) {
			doget(gpkt.p_file, gfilename, newser);
			if (HADO)
				dogtime(&gpkt, gfilename, &gfile_mtime);
		}
		setuid(holduid);
		setgid(holdgid);
	}
}

/*
 * Make the delta table for the current file
 */
static int
mkdelt(pkt,sp,osp,diffloop,orig_nlines)
struct packet *pkt;
struct sid *sp, *osp;
int diffloop;
int orig_nlines;
{
	extern dtime_t Timenow;
	struct deltab dt;
	char str[max(BUFSIZ, SID_STRSIZE)];	/* Buffer for delta table IO */
	int newser;
	register char *p;
	int ser_inc, opred, nulldel;

	if (!diffloop && pkt->p_verbose) {
		sid_ba(sp,str);
		fprintf(pkt->p_stdout,"%s\n",str);
		fflush(pkt->p_stdout);
	}
	putmagic(pkt, "00000");
	newstats(pkt,str,"0");
	dt.d_sid = *sp;

	/*
	Check if 'null' deltas should be inserted
	(only if 'null' flag is in file and
	releases are being skipped) and set
	'nulldel' indicator appropriately.
	*/
	if (pkt->p_sflags[NULLFLAG - 'a'] && (sp->s_rel > osp->s_rel + 1) &&
			!sp->s_br && !sp->s_seq &&
			!osp->s_br && !osp->s_seq)
		nulldel = 1;
	else
		nulldel = 0;
	/*
	Calculate how many serial numbers are needed.
	*/
	if (nulldel)
		ser_inc = sp->s_rel - osp->s_rel;
	else
		ser_inc = 1;
	/*
	Find serial number of the new delta.
	*/
	newser = dt.d_serial = maxser(pkt) + ser_inc;
	/*
	Find old predecessor's serial number.
	*/
	opred = sidtoser(osp,pkt);
	if (nulldel)
		dt.d_pred = newser - 1;	/* set predecessor to 'null' delta */
	else
		dt.d_pred = opred;

#ifdef	NO_NANOSECS
	time2dt(&dt.d_dtime, Timenow, 0); /* Timenow was set by dodelt() */
#else
	dt.d_dtime = Timenow;		/* Timenow was set by dodelt() */
#endif

        /* Since the NSE always preserves the clear file after delta and
         * makes it read only (no get is required since keywords are not
         * supported), the delta time is set to be the mtime of the clear
         * file.
         */
        if ((HADO || HADQ) && (gfile_mtime.tv_sec != 0)) {
		time2dt(&dt.d_dtime, gfile_mtime.tv_sec, gfile_mtime.tv_nsec);
        }
	strncpy(dt.d_pgmr,logname(),LOGSIZE-1);
	dt.d_type = 'D';
	del_ba(&dt,str, pkt->p_flags & ~PF_GMT);
	putline(pkt,str);
	if (ilist)
		mkixg(pkt,INCLUSER,INCLUDE);
	if (elist)
		mkixg(pkt,EXCLUSER,EXCLUDE);
	if (glist)
		mkixg(pkt,IGNRUSER,IGNORE);
	if (Mrs) {
		if ((p = pkt->p_sflags[VALFLAG - 'a']) == NULL)
			fatal(gettext("MRs not allowed (de8)"));
		if (*p && !diffloop && valmrs(pkt,p))
			fatal(gettext("invalid MRs (de9)"));
		putmrs(pkt);
	} else if (pkt->p_sflags[VALFLAG - 'a'] && !HADQ) {
		fatal(gettext("MRs required (de10)"));
	}
/*
*
* CMF enhancement
*
*/
	if (pkt->p_sflags[CMFFLAG - 'a']) {
		if (Mrs) {
			 cmrerror(gettext("input CMR's ignored"));
			 Mrs = NOGETTEXT("");
		}
		if (!deltack(pkt->p_file,Cmrs,Nsid, pkt->p_sflags[CMFFLAG - 'a'], pkt->p_sflags)) {
			 fatal(gettext("Delta denied due to CMR difficulties"));
		}
		putcmrs(pkt); /* this routine puts cmrs on the out put file */
	}
	if (pkt->p_flags & PF_V6) {
		Checksum_offset = ftell(gpkt.p_xiop);
		sidext_ba(pkt, &dt);
	}

	sprintf(str, NOGETTEXT("%c%c "), CTLCHAR, COMMENTS);
	putline(pkt,str);
	{
	  char *comment = savecmt(Comments);
	  putline(pkt,comment);
	  putline(pkt,"\n");
	}
	sprintf(str,CTLSTR,CTLCHAR,EDELTAB);
	putline(pkt,str);
	if (nulldel)			/* insert 'null' deltas */
		while (--ser_inc) {
			sprintf(str, NOGETTEXT("%c%c %s/%s/%05d\n"),
			  CTLCHAR, STATS,
			  NOGETTEXT("00000"), NOGETTEXT("00000"),
			  orig_nlines);
			putline(pkt,str);
			dt.d_sid.s_rel -= 1;
			dt.d_serial -= 1;
			if (ser_inc != 1)
				dt.d_pred -= 1;
			else
				dt.d_pred = opred;	/* point to old pred */
			del_ba(&dt,str, pkt->p_flags);
			putline(pkt,str);
			sprintf(str, NOGETTEXT("%c%c "), CTLCHAR, COMMENTS);
			putline(pkt,str);
			putline(pkt,NOGETTEXT("AUTO NULL DELTA\n"));
			sprintf(str,CTLSTR,CTLCHAR,EDELTAB);
			putline(pkt,str);
		}
	return(newser);
}

/*
 * Write include/exclude/ignore table for delta table
 */
static void
mkixg(pkt,reason,ch)
struct packet *pkt;
int reason;
char ch;
{
	int n;
 	char str[BUFSIZ];	/* Only limits the size of a single entry */

	sprintf(str, NOGETTEXT("%c%c"), CTLCHAR, ch);
	putline(pkt,str);
	for (n = maxser(pkt); n; n--) {
		if (pkt->p_apply[n].a_reason == reason) {
			sprintf(str, NOGETTEXT(" %u"), n);
			putline(pkt,str);
		}
	}
	putline(pkt,"\n");
}

static void
putmrs(pkt)
struct packet *pkt;
{
	register char **argv;
	char str[LENMR+6];
	extern char **Varg;

	for (argv = &Varg[VSTART]; *argv; argv++) {
		sprintf(str, NOGETTEXT("%c%c %s\n"), CTLCHAR, MRNUM, *argv);
		if (strcmp(str,NOGETTEXT("\001m \012")))
			putline(pkt,str);
	}
}



/*
*
*	putcmrs takes the cmrs list on the Mrs line built by deltack
* 	and puts them in the packet
*	
*/
static void
putcmrs(pkt)    
struct packet *pkt;
	{
		char str[510];
		sprintf(str, NOGETTEXT("%c%c %s\n"), CTLCHAR, MRNUM, Cmrs);
		putline(pkt,str);
	}


static char ambig[] = NOGETTEXT("ambiguous `r' keyletter value (de15)");

/*
 * Read the p-file and write a new version as q-file.
 */
static struct pfile *
rdpfile(pkt,sp)
register struct packet *pkt;
struct sid *sp;
{
	char *user;
	struct pfile pf;
	static struct pfile goodpf;
	char line[BUFSIZ];		/* Limits the line length of a p-file */
	int cnt, uniq, fd;
	FILE *in, *out;
	char *outname;

	uniq = cnt = -1;
	if ((user=logname()) == NULL)
	   fatal(gettext("User ID not in password file (cm9)"));
	zero((char *)&goodpf,sizeof(goodpf));
	in = xfopen(auxf(pkt->p_file,'p'), O_RDONLY|O_BINARY);
	outname = auxf(pkt->p_file, 'q');
	if ((fd=open(outname, O_WRONLY|O_CREAT|O_EXCL|O_BINARY, 0444)) < 0) {
	   efatal(gettext("cannot create lock file (cm4)"));
	}
#ifdef	HAVE_FCHMOD
	fchmod(fd, (mode_t)0644);
#else
	chmod(outname, (mode_t)0644);
#endif
	out = fdfopen(fd, O_WRONLY|O_BINARY);
	while (fgets(line,sizeof(line),in) != NULL) {
		pf_ab(line,&pf,1);
		pf.pf_date = cutoff;
		if (equal(pf.pf_user,user)||getuid()==0) {
			if (sp->s_rel == 0) {
				if (++cnt) {
					if (fflush(out) == EOF)
						xmsg(outname, NOGETTEXT("rdpfile"));
#ifdef	HAVE_FSYNC
					if (fsync(fileno(out)) < 0)
						xmsg(outname, NOGETTEXT("rdpfile"));
#endif
					if (fclose(out) == EOF)
						xmsg(outname, NOGETTEXT("rdpfile"));
					fclose(in);
					fatal(gettext("missing -r argument (de1)"));
				}
				goodpf = pf;
				continue;
			}
			else if ((sp->s_rel == pf.pf_nsid.s_rel &&
				sp->s_lev == pf.pf_nsid.s_lev &&
				sp->s_br == pf.pf_nsid.s_br &&
				sp->s_seq == pf.pf_nsid.s_seq) ||
				(sp->s_rel == pf.pf_gsid.s_rel &&
				sp->s_lev == pf.pf_gsid.s_lev &&
				sp->s_br == pf.pf_gsid.s_br &&
				sp->s_seq == pf.pf_gsid.s_seq)) {
					if (++uniq) {
						if (fflush(out) == EOF)
							xmsg(outname, NOGETTEXT("rdpfile"));
#ifdef	HAVE_FSYNC
						if (fsync(fileno(out)) < 0)
							xmsg(outname, NOGETTEXT("rdpfile"));
#endif
						if (fclose(out) == EOF)
							xmsg(outname, NOGETTEXT("rdpfile"));
						fclose(in);
						fatal(ambig);
					}
					goodpf = pf;
					continue;
			}
		}
		if(fputs(line,out)==EOF)
			xmsg(outname, NOGETTEXT("rdpfile"));
	}
	fflush(stderr);
	if (fflush(out) == EOF)
		xmsg(outname, NOGETTEXT("rdpfile"));
#ifdef	HAVE_FSYNC
	if (fsync(fileno(out)) < 0)
		xmsg(outname, NOGETTEXT("rdpfile"));
#endif
	if (fclose(out) == EOF)
		xmsg(outname, NOGETTEXT("rdpfile"));
	copy(auxf(pkt->p_file,'p'),Pfilename);
	fclose(in);
	if (!goodpf.pf_user[0])
		fatal(gettext("login name or SID specified not in p-file (de2)"));
	return(&goodpf);
}

/*
 * Open a FILE * to the diff output.
 */
static FILE *
dodiff(newf,oldf,difflim)
char *newf, *oldf;
int difflim;
{
	register int i;
	register int n;
	int pfd[2];
	FILE *iop;
	char num[10];
	struct rlimit	rlim;

	if (HADUCD)
		return  (xfopen(Dfilename, O_RDONLY|O_BINARY));
	xpipe(pfd);
	if ((i = vfork()) < 0) {
		int	errsav = errno;

		close(pfd[0]);
		close(pfd[1]);
		errno = errsav;
		efatal(gettext("cannot fork, try again (de11)"));
	}
	else if (i == 0) {
#ifdef	set_child_standard_fds
		set_child_standard_fds(STDIN_FILENO,
					pfd[1],
					STDERR_FILENO);
#ifdef	F_SETFD
		fcntl(pfd[0], F_SETFD, 1);
		n = getdtablesize();	/* We are single threaded, so cache */
		for (i = 5; i < n; i++)
			fcntl(i, F_SETFD, 1);
#endif
#else
		close(pfd[0]);
		close(1);
		dup(pfd[1]);
		close(pfd[1]);

#if defined(HAVE_GETRLIMIT) && defined(HAVE_SETRLIMIT) && defined(RLIMIT_NOFILE)
		/*
		 * Set max # of file descriptors to allow bdiff to hold all
		 * files open and to reduce the number of files to close.
		 */
		getrlimit(RLIMIT_NOFILE, &rlim);
		if (rlim.rlim_cur > 20)
			rlim.rlim_cur = 20;	/* Suffifient for bdiff/diff */
		setrlimit(RLIMIT_NOFILE, &rlim);
#endif
		n = getdtablesize();	/* We are single threaded, so cache */
		for (i = 5; i < n; i++)
			close(i);
#endif
		sprintf(num, NOGETTEXT("%d"), difflim);
 		if (HADD) {
#if	defined(PROTOTYPES) && defined(INS_BASE)
		   diffpgm = Diffpgmp;
 		   execl(Diffpgmp,Diffpgmp,oldf,newf, (char *)0);
#endif
		   diffpgm = Diffpgm;
 		   execl(Diffpgm,Diffpgm,oldf,newf, (char *)0);
		   diffpgm = Diffpgm2;
 		   execl(Diffpgm2,Diffpgm2,oldf,newf, (char *)0);
 		} else {
#if	defined(PROTOTYPES) && defined(INS_BASE)
		   diffpgm = BDiffpgmp;
 		   execl(BDiffpgmp,BDiffpgmp,oldf,newf,num,"-s", (char *)0);
#endif
		   diffpgm = BDiffpgm;
 		   execl(BDiffpgm,BDiffpgm,oldf,newf,num,"-s", (char *)0);
		   diffpgm = BDiffpgm2;
 		   execl(BDiffpgm2,BDiffpgm2,oldf,newf,num,"-s", (char *)0);
 		}
		close(1);
		_exit(32);	/* tell parent that 'execl' failed */
	}
	else {
		close(pfd[1]);
		iop = fdfopen(pfd[0], O_RDONLY|O_BINARY);
		return(iop);
	}
	/*NOTREACHED*/
	return (0);	/* fake for gcc */
}

/*
 * Parse the part of the diff output that contains the line numbers and the
 * delete/append/change instructions.
 */
static int
getdiff(type,plinenum)
register char *type;
register int *plinenum;
{
	char line[BUFSIZ];
	register char *p;
	int num_lines = 0;
	static int chg_num, chg_ln;
	int lowline, highline;

	if ((p = rddiff(line,sizeof(line))) == NULL)
		return(0);

	if (*p == '-') {
		*type = INS;
		*plinenum = chg_ln;
		num_lines = chg_num;
	}
	else {
		p = linerange(p,&lowline,&highline);
		*plinenum = lowline;

		switch(*p++) {
		case 'd':
			num_lines = highline - lowline + 1;
			*type = DEL;
			skipline(line,num_lines);
			break;

		case 'a':
			linerange(p,&lowline,&highline);
			num_lines = highline - lowline + 1;
			*type = INS;
			break;

		case 'c':
			chg_ln = lowline;
			num_lines = highline - lowline + 1;
			linerange(p,&lowline,&highline);
			chg_num = highline - lowline + 1;
			*type = DEL;
			skipline(line,num_lines);
			break;
		}
	}

	return(num_lines);
}

/*
 * Skip the next chunk of kept lines from the old file and then
 * insert the new lines from the diff output.
 */
static void
insert(pkt, linenum, n, ser, off)
struct	packet	*pkt;
int	linenum, n, ser;
	int	off;
{
 	char	str[BUFSIZ];
 	int 	first;
	int	nonl = 0;

	/*
	 * We only count newlines and thus need to add one to number_of_lines
	 * to get the same view as bdiff/diff.
	 */
	if ((pkt->p_props & CK_NONL) && (linenum + n) >= (number_of_lines+1)) {
		if ((pkt->p_encoding & EF_UUENCODE) == 0) {
			nonl++;
		}
	}

	after(pkt, linenum);
	sprintf(str, NOGETTEXT("%c%c %d\n"), CTLCHAR, INS, ser);
	putline(pkt, str);
	if (off)
		off = 2;			/* strlen("> ") from diff */
	while (--n >= 0) {
 		first = 1;
 		for (;;) {			/* Loop over partial line */
			if (rddiff(str, BUFSIZ) == NULL) {
				fatal(gettext("Cannot read the diffs file (de19)"));
			}
			if (first) {
				first = 0;
				if (n == 0 && nonl) {	/* No newline at end */
					putctlnnl(pkt);	/* ^AN escape	    */
				} else if (str[off] == CTLCHAR) /* ^A escape? */
					putctl(pkt);
				putline(pkt, str+off);	/* Skip diff's "> " */
			} else {
				putline(pkt, str);
			}
			if (str[strlen(str)-1] == '\n') {
				break;
			}
		}
	}
	sprintf(str, NOGETTEXT("%c%c %d\n"), CTLCHAR, END, ser);
	putline(pkt, str);
}

/*
 * Add delete markers in the weave data.
 */
static void
delete(pkt, linenum, n, ser)
struct	packet	*pkt;
int	linenum, n, ser;
{
	char str[BUFSIZ];	/* Only used for ^A lines in the weave */

	before(pkt, linenum);
	sprintf(str, NOGETTEXT("%c%c %d\n"), CTLCHAR, DEL, ser);
	putline(pkt, str);
	after(pkt, linenum + n - 1);
	sprintf(str, NOGETTEXT("%c%c %d\n"), CTLCHAR, END, ser);
	putline(pkt, str);
}

static void
after(pkt, n)
struct	packet	*pkt;
int	n;
{
	before(pkt, n);
	if (pkt->p_glnno == n) {
		for (;;) {
			if (pkt->p_line[strlen(pkt->p_line)-1] != '\n') {
				getline(pkt);
			}
			else {
				putline(pkt, (char *)0);
				break;
			}
		}
	}
}

static void
before(pkt, n)
struct	packet	*pkt;
int	n;
{
	while (pkt->p_glnno < n) {
		if (!readmod(pkt))
			break;
	}
}

/*
 * Parse the line range from the diff output
 */
static char *
linerange(cp, low, high)
char	*cp;
int	*low, *high;
{
	cp = satoi(cp, low);
	if (*cp == ',')
		cp = satoi(++cp, high);
	else
		*high = *low;

	return(cp);
}

/*
 * Skip lines from the diff outout, e.g. lines that start with "< ".
 */
static void
skipline(lp, num)
char	*lp;
int	num;
{
 	for (++num; --num; ) {
 		do {
 		   (void)rddiff(lp, BUFSIZ);
 		} while (lp[strlen(lp)-1] != '\n');
 	}
}

/*
 * This is the central read routine.
 * As long as we use fgets() here, we cannot support nul bytes in the files.
 */
static char *
rddiff(s, n)
char	*s;
int	n;
{
	char	*r;
	
	strcpy(s, "");
	if ((r = fgets(s, n, Diffin)) != NULL) {
	   if (HADP) {
	      if (fputs(s, gpkt.p_stdout) == EOF)
		 FAILPUT;
	   }
	}
	return (r);
}

static void
enter(pkt,ch,n,sidp)
struct packet *pkt;
char ch;
int n;
struct sid *sidp;
{
	char str[SID_STRSIZE];
	register struct apply *ap;

	sid_ba(sidp,str);
	ap = &pkt->p_apply[n];
	if (pkt->p_cutoff > pkt->p_idel[n].i_datetime.tv_sec)
		switch(ap->a_code) {
	
		case SX_EMPTY:
			switch (ch) {
			case INCLUDE:
				condset(ap,APPLY,INCLUSER);
				break;
			case EXCLUDE:
				condset(ap,NOAPPLY,EXCLUSER);
				break;
			case IGNORE:
				condset(ap,SX_EMPTY,IGNRUSER);
				break;
			}
			break;
		case APPLY:
			fatal(gettext("internal error in delta/enter() (de5)"));
			break;
		case NOAPPLY:
			fatal(gettext("internal error in delta/enter() (de6)"));
			break;
		default:
			fatal(gettext("internal error in delta/enter() (de7)"));
			break;
		}
}

static void
clean_up()
{
	uname(&un);
	uuname = un.nodename;
	if (mylock(auxf(gpkt.p_file,'z'), getpid(),uuname)) {
		sclose(&gpkt);
		sfree(&gpkt);
		if (gpkt.p_xiop) {
			fclose(gpkt.p_xiop);
			gpkt.p_xiop = NULL;
			unlink(auxf(gpkt.p_file,'x'));
		}
		if(Gin) {
			fclose(Gin);
			Gin = NULL;
		}
		unlink(auxf(gpkt.p_file,'d'));
		unlink(auxf(gpkt.p_file,'q'));
		xrm(&gpkt);
		ffreeall();
		uname(&un);
		uuname = un.nodename;
		unlockit(auxf(gpkt.p_file,'z'), getpid(),uuname);
	}
}

/*
 * Compute the checksum for the new version and check for characters that
 * are not allowed in the file.
 *
 * SCCSv4 disallows ^A (0x01) at the start of a line, nul bytes and requires a
 *	newline at the end of the file.
 *
 * SCCSv6 currently disallows nul bytes in the file.
 *	Since our diff(1) implementation supports to treat even files with
 *	nul bytes as text files, we could support nul bytes in the future.
 *
 * Both versions allow an unlimited line length.
 *
 * Since fgets() does not report the amount of bytes read, we cannot support
 * nul bytes on platforms with record oriented IO (VMS).
 */
/*ARGSUSED*/
static void
fgetchk(file, pkt)
char	*file;
struct	packet	*pkt;
{
	FILE	*inptr;
#ifndef	RECORD_IO
	char	*p = NULL;	/* Intialize to make gcc quiet */
	char	*pn =  NULL;
	char	line[VBUF_SIZE+1];
#else
	char	line[BUFSIZ];
	int	search_on = 0;
#endif
	off_t	nline;
	int	idx = 0;
	int	warned = 0;
	char	chkflags = 0;
	char	lastchar;
	unsigned int sum = 0;

	inptr = xfopen(file, O_RDONLY|O_BINARY);
#ifdef	USE_SETVBUF
	setvbuf(inptr, NULL, _IOFBF, VBUF_SIZE);
#endif
	/*
	 * This gives the illusion that a zero-length file ends
	 * in a newline so that it won't be mistaken for a 
	 * binary file.
	 */
	lastchar = '\n';
	(void)memset(line, '\377', sizeof (line));
	nline = 0;
#ifndef	RECORD_IO
	/*
	 * In most cases (non record oriented I/O), we can optimize the way we
	 * scan files for '\0' bytes, line-ends '\n' and ^A '\1'. The optimized
	 * algorithm allows to avoid to do a reverse scan for '\0' from the end
	 * of the buffer.
	 */
	while ((idx = fread(line, 1, sizeof (line) - 1, inptr)) > 0) {
		sum += usum(line, idx);
		if (lastchar == '\n' && line[0] == CTLCHAR) {
			chkflags |= CK_CTLCHAR;
			if ((pkt->p_flags & PF_V6) == 0)
				goto err;
			if (!warned) {
				warnctl(file, nline+1);
				warned = 1;
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
			if (pn && p > pn)
				goto err;
			nline++;
			if ((p - line) >= (idx-1))
				break;

			if (p[1] == CTLCHAR) {
				chkflags |= CK_CTLCHAR;
	err:
				if ((pkt->p_flags & PF_V6) &&
				    (chkflags & CK_NULL) == 0) {
					if (!warned) {
						warnctl(file, nline+1);
						warned = 1;
					}
					continue;
				}
				fclose(inptr);
				sprintf(SccsError,
				gettext(
			  "file '%s' contains illegal data on line %jd (de14)"),
				file, (Intmax_t)++nline);
				fatal(SccsError);
			}
		}
		line[idx] = '\0';
		if (pkt->p_did_id == 0) {
			pkt->p_did_id =
				chkid(line, pkt->p_sflags[IDFLAG - 'a'], pkt->p_sflags);
		}
	}
#else	/* !RECORD_IO */
	while (fgets(line, sizeof (line), inptr) != NULL) {
	   if (lastchar == '\n' && line[0] == CTLCHAR) {
	      chkflags |= CK_CTLCHAR;
	      if ((pkt->p_flags & PF_V6) == 0) {
		nline++;
		goto err;
	      }
	      if (!warned) {
		warnctl(file, nline);
		warned = 1;
	      }
	   }
	   search_on = 0;
	   for (idx = sizeof (line)-1; idx >= 0; idx--) {
	      if (search_on > 0) {
		 if (line[idx] == '\0') {
		    chkflags |= CK_NULL;
	err:
		    fclose(inptr);
		    sprintf(SccsError,
		      gettext("file '%s' contains illegal data on line %jd (de14)"),
		      file, (Intmax_t)nline);
		    fatal(SccsError);
		 }
	      } else {
		 if (line[idx] == '\0') {
		    sum += usum(line, idx);
		    search_on = 1;
		    lastchar = line[idx-1];
		    if (lastchar == '\n') {
		       nline++;
		    }
		 }
	      }
	   }   
	   if (pkt->p_did_id == 0) {
		pkt->p_did_id =
			chkid(line, pkt->p_sflags[IDFLAG - 'a'], pkt->p_sflags);
	   }
	   (void)memset(line, '\377', sizeof (line));
	}
#endif	/* !RECORD_IO */
	fclose(inptr);

	pkt->p_ghash = sum & 0xFFFF;
	pkt->p_glines = nline;

	if (lastchar != '\n')
		chkflags |= CK_NONL;
	pkt->p_props |= chkflags;

	if (chkflags & CK_NONL) {
#ifndef	RECORD_IO
		if (pn && nline == 0)	/* Found null byte but no newline */
			goto err;
#endif
		if ((pkt->p_flags & PF_V6) == 0) {
			sprintf(SccsError,
			    gettext("No newline at end of file '%s' (de18)"),
			    file);
			fatal(SccsError);
		} else {
			fprintf(stderr,
			    gettext("WARNING [%s]: No newline at end of file (de18)"),
			    file);
		}
	}
}

static void
warnctl(file, nline)
	char	*file;
	off_t	nline;
{
	fprintf(stderr,
		gettext(
		"WARNING [%s]: line %jd begins with ^A\n"),
		file, (Intmax_t)nline);
}

static void 
fixghash(pkt, ser)
	struct	packet	*pkt;
	int		ser;
{
	char		ghbuf[10];
	signed char	*q;
	struct packet	pk2;
	struct stats	stats;

	putline(pkt,(char *) 0);	/* Flush last unwritten line	*/
	fflush(pkt->p_xiop);		/* Flush stdio buffers		*/

	sinit(&pk2, auxf(pkt->p_file,'x'), SI_OPEN|SI_FORCE);

	pk2.do_chksum = 0;		/* Checksums are still wrong	*/
	pk2.p_stdout = stderr;
	pk2.p_cutoff = MAX_TIME;

	if (dodelt(&pk2, &stats, (struct sid *) 0, 0) == 0)
		fmterr(&pk2);
	flushto(&pk2, EUSERTXT, FLUSH_NOCOPY);

	pk2.p_chkeof = 1;
	pk2.p_gotsid = pk2.p_idel[ser].i_sid;
	pk2.p_reqsid = pk2.p_gotsid;

	setup(&pk2, ser);

	pk2.p_ghash = 0;
	while (readmod(&pk2))		/* Compute ghash for gotten content */
		;
	pk2.p_ghash &= 0xFFFF;

	fseek(pkt->p_xiop, Checksum_offset, SEEK_SET);
	fprintf(pkt->p_xiop, "%c%c s %5.5d\n",
		CTLCHAR, SIDEXTENS, pk2.p_ghash);
	pkt->p_nhash -= 5 * '0';
	sprintf(ghbuf, "%5.5d", pk2.p_ghash);
	q = (signed char *) ghbuf;
	while (*q)
		pkt->p_nhash += *q++;
}

/* SVR4.0 does not support getdtablesize().				  */

#ifndef	HAVE_GETDTABLESIZE
int
getdtablesize()
{
#if defined(HAVE_GETRLIMIT) && defined(RLIMIT_NOFILE)
	struct rlimit	rlim;

	rlim.rlim_cur = 20;
	getrlimit(RLIMIT_NOFILE, &rlim);
	return (rlim.rlim_cur);
#endif
	return (20);
}
#endif
