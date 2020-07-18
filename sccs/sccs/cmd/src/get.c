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
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)get.c	1.94 20/07/14 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)get.c 1.94 20/07/14 J. Schilling"
#endif
/*
 * @(#)get.c 1.59 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)get.c"
#pragma ident	"@(#)sccs:cmd/get.c"
#endif

#include	<defines.h>
#include	<version.h>
#include	<had.h>
#include	<i18n.h>
#include	<schily/utsname.h>
#include	<ccstypes.h>
#include	<schily/limits.h>
#include	<schily/sysexits.h>

#define	DATELEN	12

#if defined(__STDC__) || defined(PROTOTYPES)
#define	INCPATH		INS_BASE "/ccs/include"	/* place to find binaries */
#else
/*
 * XXX With a K&R compiler, you need to edit the following string in case
 * XXX you like to change the install path.
 */
#define	INCPATH		"/usr/ccs/include"	/* place to find binaries */
#endif



/*
 * Get is now much more careful than before about checking
 * for write errors when creating the g- l- p- and z-files.
 * However, it remains deliberately unconcerned about failures
 * in writing statistical information (e.g., number of lines
 * retrieved).
 */

static struct packet gpkt;		/* libcomobj data for get()	*/
static char	Zhold[MAXPATHLEN];	/* temporary z-file name */

static Nparms	N;			/* Keep -N parameters		*/
static Xparms	X;			/* Keep -X parameters		*/
static struct sid sid;			/* To hold -r parameter		*/
static unsigned	Ser;			/* To hold -a parameter		*/
static char	*ilist;			/* To hold -i parameter		*/
static char	*elist;			/* To hold -x parameter		*/
static char	*lfile;			/* To hold -l parameter		*/
static char	*cutoffstr;		/* To hold -c parameter		*/
static char	Gfile[PATH_MAX];	/* To hold -G (g. + parameter)	*/
static char	gfile[PATH_MAX];	/* To hold -G parameter		*/
static char	*Cwd = "";		/* To hold -C parameter		*/

/* Beginning of modifications made for CMF system. */
#define	CMRLIMIT 128
static char	cmr[CMRLIMIT];		/* To hold -z parameter		*/
/* End of insertion */

static int	num_files;		/* arg counter for main()/get()	*/
static char	Pfilename[FILESIZE];	/* p.filename used in get()	*/
static time_t	cutoff = MAX_TIME;	/* cutoff time for main()/get()	*/

static struct	utsname	un;		/* uname for lockit()		*/
static char	*uuname;		/* un.nodename			*/


static	void    clean_up __PR((void));
static	void	enter	__PR((struct packet *pkt, int ch, int n, struct sid *sidp));

	int	main __PR((int argc, char **argv));
static void	do_get __PR((char *file));
static void	get __PR((struct packet *pkt, char *file));
static void	gen_lfile __PR((struct packet *pkt));
static void	prfx __PR((struct packet *pkt));
static void	annotate __PR((struct packet *pkt));
static void	wrtpfile __PR((struct packet *pkt, char *inc, char *exc));
static int	cmrinsert __PR((struct packet *pkt));

int
main(argc,argv)
int argc;
register char *argv[];
{
	register int i;
	register char *p;
	int  c;
	extern int Fcnt;
	int current_optind;
	int no_arg;
	int	cmri = 0;	/* CMF index for option parsing */

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
	set_clean_up(clean_up);
	Fflags = FTLEXIT | FTLMSG | FTLCLN;
#ifdef	SCCS_FATALHELP
	Fflags |= FTLFUNC;
	Ffunc = sccsfatalhelp;
#endif
#ifdef	XPG4
	sccs_xpg4(TRUE);
#endif
	current_optind = 1;
	optind = 1;
	opterr = 0;
	no_arg = 0;
	i = 1;
	/*CONSTCOND*/
	while (1) {
			if (current_optind < optind) {
			   current_optind = optind;
			   argv[i] = 0;
			   if (optind > i+1) {
			      if ((argv[i+1][0] != '-') && (no_arg == 0)) {
			         argv[i+1] = NULL;
			      } else {
				 optind = i+1;
			         current_optind = optind;
			      }
			   }
			}
			no_arg = 0;
			i = current_optind;
		        c = getopt(argc, argv, "()-r:c:ebi:x:kl:Lpsmnogta:G:w:zqdC:AFN:X:V(version)");
				/* this takes care of options given after
				** file names.
				*/
			if (c == EOF) {
			   if (optind < argc) {
			      /* if it's due to -- then break; */
			      if (argv[i][0] == '-' &&
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
    			case CMFFLAG:		/* 'z' */
				/* Concatenate the rest of this argument with
				 * the existing CMR list. */
				if (p) {
				   while (*p) {
				      if (cmri == CMRLIMIT)
					 fatal(gettext("CMR list is too long."));
					 cmr[cmri++] = *p++;
				   }
				}
				cmr[cmri] = '\0';
				break;
			case 'a':
				if (*p == 0) continue;
				Ser = patoi(p);
				break;
			case 'r':
				if (argv[i][2] == '\0') {
				   if (*(omit_sid(p)) != '\0') {
				      no_arg = 1;
				      continue;
				   }
				}
				chksid(sid_ab(p, &sid), &sid);
				if ((sid.s_rel < MINR) || (sid.s_rel > MAXR)) {
				   fatal(gettext("r out of range (ge22)"));
				}
				break;
			case 'w':
				if (p) whatsetup(p);
				break;
			case 'c':
				if (*p == 0) continue;
				cutoffstr = p;
#if defined(BUG_1205145) || defined(GMT_TIME)
				if (parse_date(p,&cutoff, 0))
#else
				if (parse_date(p,&cutoff, PF_GMT))
#endif
				   fatal(gettext("bad date/time (cm5)"));
				break;
			case 'L':
				/* treat this as if -lp was given */
				lfile = NOGETTEXT("stdout");
				c = 'l';
				break;
			case 'l':
				if (argv[i][2] != '\0') {
				   if ((*p == 'p') && (strlen(p) == 1)) {
				      p = NOGETTEXT("stdout");
				   }
				   lfile = p;
				} else {
				   no_arg = 1;
				   lfile = NULL;
				}
				break;
			case 'i':
				if (*p == 0) continue;
				ilist = p;
				break;
			case 'x':
				if (*p == 0) continue;
				elist = p;
				break;
			case 'G':
				{
				char *cp;

				if (*p == 0) continue;
				copy(p, gfile);
 				cp = auxf(p, 'G');
				copy(cp, Gfile);
				break;
				}
			case 'b':
			case 'g':
			case 'e':
			case 'p':
			case 'k':
			case 'm':
			case 'n':
			case 'o':
			case 's':
			case 't':
			case 'd':
			case 'A':
			case 'F':
				if (p) {
				   sprintf(SccsError,
				     gettext("value after %c arg (cm8)"),
				     c);
				   fatal(SccsError);
				}
				break;
                        case 'q': /* enable NSE mode */
				if (p) {
                                   if (*p) {
                                      nsedelim = p;
				   }
                                } else {
                                   nsedelim = (char *) 0;
                                }
                                break;
			case 'C':
				Cwd = p;
				break;

			case 'N':	/* Bulk names */
				initN(&N);
				if (optarg == argv[i+1]) {
				   no_arg = 1;
				   break;
				}
				N.n_parm = p;
				break;

			case 'X':	/* -Xtended options */
				X.x_parm = optarg;
				X.x_flags = XO_NULLPATH;
				if (!parseX(&X))
					goto err;
				had[NLOWER+c-'A'] = 0;	/* Allow mult -X */
				break;

			case 'V':		/* version */
				printf(gettext(
				    "get %s-SCCS version %s %s (%s-%s-%s)\n"),
					PROVIDER,
					VERSION,
					VDATE,
					HOST_CPU, HOST_VENDOR, HOST_OS);
				exit(EX_OK);

			default:
			err:
			   fatal(gettext("Usage: get [-AbeFgkmLopst] [-l[p]] [-asequence] [-cdate-time] [-Gg-file]\n\t[-isid-list] [-rsid] [-xsid-list][ -N[bulk-spec]][ -Xxopts ] s.filename ..."));
			}

			/*
			 * Make sure that we only collect option letters from
			 * the range 'a'..'z' and 'A'..'Z'.
			 */
			if (ALPHA(c) &&
			    (had[LOWER(c)? c-'a' : NLOWER+c-'A']++)) {
				if (c != 'X')
					fatal(gettext("key letter twice (cm2)"));
			}
	}
	for (i=1; i<argc; i++) {
		if (argv[i]) {
		       num_files++;
		}
	}
	if(num_files == 0)
		fatal(gettext("missing file arg (cm3)"));
	if (HADE && HADM)
		fatal(gettext("e not allowed with m (ge3)"));
	if (HADE)
		HADK = 1;
	if (HADE && !logname())
		fatal(gettext("User ID not in password file (cm9)"));

	setsig();
	xsethome(NULL);
	if (HADUCN) {					/* Parse -N args  */
		/*
		 * initN() was already called while parsing options.
		 */
		if (!HADP && !HADG && !HADUCG)		/* Create g-file, so */
			N.n_flags |= N_GETI;		/* create subdirs    */
			
		parseN(&N);

		if (N.n_sdot && (sethomestat & SETHOME_OFFTREE))
			fatal(gettext("-Ns. not supported in off-tree project mode"));
	}

	/*
	 * Get the name of our machine to be used for the lockfile.
	 */
	uname(&un);
	uuname = un.nodename;

	/*
	 * Set up a project global lock on the changeset file.
	 * Since we set FTLJMP, we do not need to unlockchset() from clean_up().
	 */
	if (HADE && SETHOME_CHSET())
		lockchset(getppid(), getpid(), uuname);
	timerchsetlock();

	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if ((p=argv[i]) != NULL)
			do_file(p, do_get, 1, N.n_sdot, &X);

	/*
	 * Only remove the global lock it it was created by us and not by
	 * our parent.
	 */
	if (HADE && SETHOME_CHSET()) {
		if (HADUCN)
			bulkchdir(&N);
		unlockchset(getpid(), uuname);
	}

	return (Fcnt ? 1 : 0);
}

static void
do_get(file)
	char		*file;
{
	get(&gpkt, file);
}

static void
get(pkt, file)
	struct packet	*pkt;
	char		*file;
{
	register char *p;
	register unsigned ser;
	extern char had_dir, had_standinp;
	struct stats stats;
	char	str[SID_STRSIZE];		/* Must fit a SID string */
#ifdef	PROTOTYPES
	char template[] = NOGETTEXT("/get.XXXXXX");
#else
	char *template = NOGETTEXT("/get.XXXXXX");
#endif
	char *ifile;
	char buf1[PATH_MAX];
	uid_t holduid;
	gid_t holdgid;
	static int first = 1;

	if (setjmp(Fjmp))
		return;

	/*
	 * In order to make the global lock with a potentially long duration
	 * not look as if it was expired, we refresh it for every file in our
	 * task list. This is needed since another SCCS instance on a different
	 * NFS machine cannot use kill() to check for a still active process.
	 */
	if (HADE && SETHOME_CHSET()) {
		if (HADUCN)
			bulkchdir(&N);	/* Done by bulkprepare() anyway */
		refreshchsetlock();
	}

	if (HADUCN) {
#ifdef	__needed__
		char	*ofile = file;
#endif

		file = bulkprepare(&N, file);
		if (file == NULL) {
#ifdef	__needed__
			if (N.n_ifile)
				ofile = N.n_ifile;
#endif
			/*
			 * The error is typically
			 * "directory specified as s-file (cm14)"
			 */
			fatal(gettext(bulkerror(&N)));
		}
		ifile = N.n_ifile;
		if (sid.s_rel == 0 && N.n_sid.s_rel != 0) {
			sid.s_rel = N.n_sid.s_rel;
			sid.s_lev = N.n_sid.s_lev;
			sid.s_br  = N.n_sid.s_br;
			sid.s_seq = N.n_sid.s_seq;
		}
	} else {
		ifile = NULL;
	}

	if (HADE) {

		/*
		call `sinit' to check if file is an SCCS file
		but don't open the SCCS file.
		If it is, then create lock file.
		pass both the PID and machine name to lockit
		*/
		sinit(pkt, file, SI_INIT);

		/*
		 * Lock out any other user who may be trying to process
		 * the same file.
		 */
		if (!islockchset(copy(auxf(file, 'z'), Zhold)) &&
		    lockit(Zhold, SCCS_LOCK_ATTEMPTS, getpid(), uuname)) {
			lockfatal(Zhold, getpid(), uuname);
		} else {
			timersetlockfile(Zhold);
		}
	}
	/*
	Open SCCS file and initialize packet
	*/
	sinit(pkt, file, SI_OPEN);
	pkt->p_ixuser = (HADI | HADX);
	pkt->p_reqsid.s_rel = sid.s_rel;
	pkt->p_reqsid.s_lev = sid.s_lev;
	pkt->p_reqsid.s_br = sid.s_br;
	pkt->p_reqsid.s_seq = sid.s_seq;
	pkt->p_verbose = (HADS) ? 0 : 1;
	pkt->p_stdout  = (HADP||lfile) ? stderr : stdout;
	pkt->p_cutoff = cutoff;
	pkt->p_lfile = lfile;
	pkt->p_enter = enter;
#if defined(BUG_1205145) || defined(GMT_TIME)
#else
	if ((pkt->p_flags & PF_V6) == 0) {
		pkt->p_flags |= PF_GMT;
	} else if (cutoffstr != NULL) {
		if (parse_date(cutoffstr, &cutoff, 0))
			fatal(gettext("bad date/time (cm5)"));
		pkt->p_cutoff = cutoff;
	}
#endif
	if (HADUCA)
		pkt->p_pgmrs = (char **)Null;
	if (Gfile[0] == 0 || !first) {
		gfile[0] = '\0';
		strlcatl(gfile, sizeof (gfile),
			Cwd,
			ifile ? ifile :
				auxf(pkt->p_file, 'g'),
				(char *)0);
		Gfile[0] = '\0';
		strlcatl(Gfile, sizeof (Gfile),
			Cwd,
			ifile ? auxf(ifile, 'G') :
				auxf(pkt->p_file, 'A'),
			(char *)0);
	}
	strlcpy(buf1, dname(Gfile), sizeof (buf1));
	strlcat(buf1, template, sizeof (buf1));
	Gfile[0] = '\0';		/* File not yet created */
	first = 0;

	if (pkt->p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(pkt->p_stdout, "\n%s:\n", pkt->p_file);
	if (dodelt(pkt, &stats, (struct sid *) 0, 0) == 0)
		fmterr(pkt);
	finduser(pkt);
	doflags(pkt);
	donamedflags(pkt);
	dometa(pkt);

	if (!HADA) {
		ser = getser(pkt);
	} else {
		if ((ser = Ser) > maxser(pkt))
			fatal(gettext("serial number too large (ge19)"));
		pkt->p_gotsid = pkt->p_idel[ser].i_sid;
		if (HADR && sid.s_rel != pkt->p_gotsid.s_rel) {
			zero((char *) &pkt->p_reqsid, sizeof (pkt->p_reqsid));
			pkt->p_reqsid.s_rel = sid.s_rel;
		} else {
			pkt->p_reqsid = pkt->p_gotsid;
		}
	}
	doie(pkt, ilist, elist, (char *) 0);
	setup(pkt, (int) ser);
	if (!(HADP || HADG)) {
		if (exists(gfile) && (S_IWRITE & Statbuf.st_mode)) {
			sprintf(SccsError, gettext("writable `%s' exists (ge4)"),
				gfile);
			fatal(SccsError);
		}
	}
	if (pkt->p_verbose) {
		sid_ba(&pkt->p_gotsid, str);
		fprintf(pkt->p_stdout, "%s\n", str);
	}
	if (HADE) {
		if (HADC || pkt->p_reqsid.s_rel == 0)
			pkt->p_reqsid = pkt->p_gotsid;
		newsid(pkt, pkt->p_sflags[BRCHFLAG - 'a'] && HADB);
		permiss(pkt);
		wrtpfile(pkt, ilist, elist);
	}
	if (!HADG || HADL) {
		if (pkt->p_stdout) {
			fflush(pkt->p_stdout);
			fflush(stderr);
		}
		holduid=geteuid();
		holdgid=getegid();
		setuid(getuid());
		setgid(getgid());
		if (HADL)
			gen_lfile(pkt);
		if (HADG)
			goto unlock;

		if (pkt->p_idel[ser].i_dtype == 'U') {	/* unlink delta */
			unlink(gfile);
			goto unlock;
		}

		flushto(pkt, EUSERTXT, FLUSH_NOCOPY);
		idsetup(pkt, gfile);
		pkt->p_chkeof = 1;
		pkt->p_did_id = 0;
		/*
		call `xcreate' which deletes the old `g-file' and
		creates a new one except if the `p' keyletter is set in
		which case any old `g-file' is not disturbed.
		The mod of the file depends on whether or not the `k'
		keyletter has been set.
		*/
		if (pkt->p_gout == 0) {
			if (HADP) {
				pkt->p_gout = stdout;
			} else {
#ifdef	HAVE_MKSTEMP
				close(mkstemp(buf1));	/* safer than mktemp */
#else
				mktemp(buf1);
#endif
				copy(buf1, Gfile);
				/*
				 * It may be better to use fdopen() and chmod()
				 * instead of xfcreat() in order to avoid an
				 * unlink()/create() chain.
				 */
				if ((exists(pkt->p_file) && (S_IEXEC & Statbuf.st_mode)) ||
				    (pkt->p_flags & PF_SCOX)) {
					pkt->p_gout = xfcreat(Gfile, HADK ?
						((mode_t)0755) : ((mode_t)0555));
				} else {
					pkt->p_gout = xfcreat(Gfile, HADK ?
						((mode_t)0644) : ((mode_t)0444));
				}
#ifdef	USE_SETVBUF
				/*
				 * Do not call setvbuf() with stdout as this may result
				 * in a second illegal call in gen_lfile().
				 */
				setvbuf(pkt->p_gout, NULL, _IOFBF, VBUF_SIZE);
#endif
			}
		}
		pkt->p_ghash = 0;		/* Reset ghash */
		if (pkt->p_encoding & EF_UUENCODE) {
			while (readmod(pkt)) {
				decode(pkt->p_line, pkt->p_gout);
			}
		} else {
			while (readmod(pkt)) {
				prfx(pkt);
				if (pkt->p_flags & PF_NONL)
					pkt->p_line[pkt->p_line_length-1] = '\0';
				p = idsubst(pkt, pkt->p_lineptr);
				if (fputs(p, pkt->p_gout) == EOF)
					xmsg(gfile, NOGETTEXT("get"));
			}
		}
		if ((pkt->p_flags & (PF_V6 | PF_V6TAGS)) && pkt->p_hash) {
			/*
			 * SCCS v6 is able to check against SID specific
			 * checksums, but this will not work in case that
			 * we use include or exclude lists.
			 */
			if (pkt->p_hash[ser] != (pkt->p_ghash & 0xFFFF) &&
			    elist == NULL && ilist == NULL && !HADUCF)
				fatal(gettext("corrupted file version (co27)"));
		}

		if (pkt->p_gout) {
			if (fflush(pkt->p_gout) == EOF)
				xmsg(gfile, NOGETTEXT("get"));
			fflush (stderr);
		}
		if (pkt->p_gout && pkt->p_gout != stdout) {
			/*
			 * Force g-file to disk and verify
			 * that it actually got there.
			 */
#ifdef	HAVE_FSYNC
			if (fsync(fileno(pkt->p_gout)) < 0)
				xmsg(gfile, NOGETTEXT("get"));
#endif
			if (fclose(pkt->p_gout) == EOF)
				xmsg(gfile, NOGETTEXT("get"));
			pkt->p_gout = NULL;
		}
		if (pkt->p_verbose) {
#ifdef XPG4
		   fprintf(pkt->p_stdout, NOGETTEXT("%d lines\n"), pkt->p_glnno);
#else
		   if (HADD == 0)
		      fprintf(pkt->p_stdout, gettext("%d lines\n"), pkt->p_glnno);
#endif
		}
		if (!pkt->p_did_id && !HADK && !HADQ &&
		    (!pkt->p_sflags[EXPANDFLAG - 'a'] ||
		    *(pkt->p_sflags[EXPANDFLAG - 'a']))) {
		   if (pkt->p_sflags[IDFLAG - 'a']) {
		      if (!(*pkt->p_sflags[IDFLAG - 'a'])) {
		         fatal(gettext("no id keywords (cm6)"));
		      } else {
			 fatal(gettext("invalid id keywords (cm10)"));
		      }
		   } else {
		      if (pkt->p_verbose) {
			 fprintf(stderr, gettext("No id keywords (cm7)\n"));
		      }
		   }
		}
		if (Gfile[0] != '\0' && exists(Gfile)) {
			rename(Gfile, gfile);
			if (HADO) {
#if defined(BUG_1205145) || defined(GMT_TIME)
#else
				struct tm	tm;
#endif
				struct timespec	ts[2];
				unsigned int	gser;
				extern dtime_t	Timenow;

				gser = sidtoser(&pkt->p_gotsid, pkt);

				ts[0].tv_sec = Timenow.dt_sec;
				ts[0].tv_nsec = Timenow.dt_nsec;
				ts[1].tv_sec = pkt->p_idel[gser].i_datetime.tv_sec;
				ts[1].tv_nsec = pkt->p_idel[gser].i_datetime.tv_nsec;
				/*
				 * If we did cheat while scanning the delta
				 * table and converted the time stamps assuming
				 * GMT. Fix the resulting error here.
				 */
#if defined(BUG_1205145) || defined(GMT_TIME)
#else
				if (pkt->p_flags & PF_GMT) {
					tm = *gmtime(&ts[1].tv_sec);
					tm.tm_isdst = -1;
					ts[1].tv_sec = mktime(&tm);
				}
#endif
				/*
				 * As SunPro make and gmake call sccs
				 * get when the time if s.file equals
				 * the time stamp of the g-file, make
				 * sure the g-file is a bit younger.
				 */
				if (!(gpkt.p_flags & PF_V6)) {
					struct timespec	tn;

					getnstimeofday(&tn);
					ts[1].tv_nsec = tn.tv_nsec;
				}
				if (ts[1].tv_nsec <= 500000000)
					ts[1].tv_nsec += 499999999;

				utimensat(AT_FDCWD, gfile, ts, 0);
			}
		}
		setuid(holduid);
		setgid(holdgid);
	}
unlock:
	if (HADE) {
		copy(auxf(pkt->p_file, 'p'), Pfilename);
		rename(auxf(pkt->p_file, 'q'), Pfilename);
		timersetlockfile(NULL);
		if (!islockchset(Zhold))
			unlockit(Zhold, getpid(), uuname);
	}
	sclose(pkt);
	sfree(pkt);
	ffreeall();
	if (HADUCA)				/* ffreeall() killed it	*/
		lhash_destroy();		/* need to reset it	*/
}

static void
enter(pkt,ch,n,sidp)
struct packet *pkt;
char ch;
int n;
struct sid *sidp;
{
	char	str[SID_STRSIZE];		/* Must fit a SID string */
	register struct apply *ap;

	sid_ba(sidp,str);
	if (pkt->p_verbose)
		fprintf(pkt->p_stdout,"%s\n",str);
	ap = &pkt->p_apply[n];
	switch (ap->a_code) {

	case SX_EMPTY:
		if (ch == INCLUDE)
			condset(ap,APPLY,INCLUSER);
		else
			condset(ap,NOAPPLY,EXCLUSER);
		break;
	case APPLY:
		sid_ba(sidp,str);
		sprintf(SccsError, gettext("%s already included (ge9)"), str);
		fatal(SccsError);
		break;
	case NOAPPLY:
		sid_ba(sidp,str);
		sprintf(SccsError, gettext("%s already excluded (ge10)"), str);
		fatal(SccsError);
		break;
	default:
		fatal(gettext("internal error in get/enter() (ge11)"));
		break;
	}
}

static void
gen_lfile(pkt)
register struct packet *pkt;
{
	char *n;
	int reason;
	char str[max(DT_ZSTRSIZE, SID_STRSIZE)]; /* SCCS v6 date or SID str */
	char line[BUFSIZ];
	struct deltab dt;
	FILE *in;
	FILE *out;
	char *outname = NOGETTEXT("stdout");

#define	OUTPUTC(c) \
	if (putc((c), out) == EOF) \
		xmsg(outname, NOGETTEXT("gen_lfile"));

	in = xfopen(pkt->p_file, O_RDONLY|O_BINARY);
	if (pkt->p_lfile) {
		/*
		 * Do not call setvbuf() with stdout as this may result
		 * in a second illegal call in get().
		 */
		out = stdout;
	} else {
		outname = auxf(pkt->p_file, 'l');
		out = xfcreat(outname,(mode_t)0444);
	}
	fgets(line,sizeof(line),in);
	while (fgets(line,sizeof(line),in) != NULL &&
	       line[0] == CTLCHAR && line[1] == STATS) {
		fgets(line,sizeof(line),in);
		del_ab(line,&dt,pkt);
		if (dt.d_type == 'D' || dt.d_type == 'U') {
			reason = pkt->p_apply[dt.d_serial].a_reason;
			if (pkt->p_apply[dt.d_serial].a_code == APPLY) {
				OUTPUTC(' ');
				OUTPUTC(' ');
			} else {
				OUTPUTC('*');
				if (reason & IGNR) {
					OUTPUTC(' ');
				} else {
					OUTPUTC('*');
				}
			}
			switch (reason & (INCL | EXCL | CUTOFF)) {

			case INCL:
				OUTPUTC('I');
				break;
			case EXCL:
				OUTPUTC('X');
				break;
			case CUTOFF:
				OUTPUTC('C');
				break;
			default:
				OUTPUTC(' ');
				break;
			}
			OUTPUTC(' ');
			sid_ba(&dt.d_sid,str);
			if (fprintf(out, "%s\t", str) == EOF)
				xmsg(outname, NOGETTEXT("gen_lfile"));
			/*
			 * POSIX requires a 2-digit year for the l-file.
			 * We use 4-digits before 1969 and past 2068
			 * which is outside the year range specified by POSIX.
			 */
#if SIZEOF_TIME_T == 4
			if (dt.d_dtime.dt_sec < Y1969)
#else
			if ((dt.d_dtime.dt_sec < Y1969) ||
			    (dt.d_dtime.dt_sec >= Y2069))
#endif
				date_bal(&dt.d_dtime.dt_sec,str, pkt->p_flags);	/* 4 digit year */
			else
				date_ba(&dt.d_dtime.dt_sec,str, pkt->p_flags);	/* 2 digit year */
			if (fprintf(out, "%s %s\n", str, dt.d_pgmr) == EOF)
				xmsg(outname, NOGETTEXT("gen_lfile"));
		}
		while ((n = fgets(line,sizeof(line),in)) != NULL) {
			if (line[0] != CTLCHAR) {
				break;
			} else {
				switch (line[1]) {

				case EDELTAB:
					break;
				default:
					continue;
				case MRNUM:
				case COMMENTS:
					if (dt.d_type == 'D' || dt.d_type == 'U') {
					   if (fprintf(out,"\t%s",&line[3]) == EOF)
					      xmsg(outname,
					         NOGETTEXT("gen_lfile"));
					}
					continue;
				}
				break;
			}
		}
		if (n == NULL || line[0] != CTLCHAR)
			break;
		OUTPUTC('\n');
	}
	fclose(in);
	if (out != stdout) {
#ifdef	HAVE_FSYNC
		if (fsync(fileno(out)) < 0)
			xmsg(outname, NOGETTEXT("gen_lfile"));
#endif
		if (fclose(out) == EOF)
			xmsg(outname, NOGETTEXT("gen_lfile"));
	}

#undef	OUTPUTC
}

static void
prfx(pkt)
register struct packet *pkt;
{
	char str[SID_STRSIZE];		/* Must fit a SID string */

	if (HADN)
		if (fprintf(pkt->p_gout, "%s\t", getmodname()) == EOF)
			xmsg(gfile, NOGETTEXT("prfx"));
	if (HADM) {
		sid_ba(&pkt->p_inssid,str);
		if (fprintf(pkt->p_gout, "%s\t", str) == EOF)
			xmsg(gfile, NOGETTEXT("prfx"));
	}
	if (pkt->p_pgmrs)
		annotate(pkt);
}

static void
annotate(pkt)
	register struct packet *pkt;
{
	char	str[SID_STRSIZE];	/* Must fit a SID string */
	char	*p;

	date_ba(&pkt->p_idel[pkt->p_insser].i_datetime.tv_sec,
				str, pkt->p_flags);
	p = strchr(str, ' ');
	if (p)
		*p = '\0';
	if (fprintf(pkt->p_gout, "%-8s\t%s\t",
				pkt->p_pgmrs[pkt->p_insser], str) == EOF)
		xmsg(gfile, NOGETTEXT("prfx"));
}

static void
clean_up()
{
	/*
	clean_up is only called from fatal() upon bad termination.
	*/
	if (gpkt.p_gout) {
		fflush(gpkt.p_gout);
	}
	if (gpkt.p_gout && gpkt.p_gout != stdout) {
		fclose(gpkt.p_gout);
		gpkt.p_gout = NULL;
		unlink(Gfile);
	}
	if (HADE) {
		uname(&un);
		uuname = un.nodename;
		if (mylock(auxf(gpkt.p_file,'z'), getpid(), uuname)) {
			unlink(auxf(gpkt.p_file,'q'));
			timersetlockfile(NULL);
			if (!islockchset(Zhold))
				unlockit(Zhold, getpid(), uuname);
		}
	}
	sclose(&gpkt);
	sfree(&gpkt);
	ffreeall();
	if (HADUCA)				/* ffreeall() killed it	*/
		lhash_destroy();		/* need to reset it	*/
}

static	char	warn[] = NOGETTEXT("WARNING: being edited: `%s' (ge18)\n");

static void
wrtpfile(pkt,inc,exc)
register struct packet *pkt;
char *inc, *exc;
{
	char line[64];
	char str1[SID_STRSIZE], str2[SID_STRSIZE]; /* Must fit a SID string */
	char *user, *pfile;
	FILE *in, *out;
	struct pfile pf;
	extern dtime_t Timenow;
	int fd;
	char **sflags = pkt->p_sflags;

	if ((user=logname()) == NULL)
	   fatal(gettext("User ID not in password file (cm9)"));
	if ((fd=open(auxf(pkt->p_file,'q'),O_WRONLY|O_CREAT|O_EXCL|O_BINARY,0444)) < 0) {
	   efatal(gettext("cannot create lock file (cm4)"));
	}
#ifdef	HAVE_FCHMOD
	fchmod(fd, (mode_t)0644);
#else
	chmod(auxf(pkt->p_file,'q'), (mode_t)0644);
#endif
	out = fdfopen(fd, O_WRONLY|O_BINARY);
	if (exists(pfile = auxf(pkt->p_file,'p'))) {
		in = fdfopen(xopen(pfile, O_RDONLY|O_BINARY), O_RDONLY|O_BINARY);
		while (fgets(line,sizeof(line),in) != NULL) {
			if (fputs(line, out) == EOF)
			   xmsg(pfile, NOGETTEXT("wrtpfile"));
			pf_ab(line,&pf,0);
			if (!(sflags[JOINTFLAG - 'a'])) {
				if ((pf.pf_gsid.s_rel == pkt->p_gotsid.s_rel &&
     				   pf.pf_gsid.s_lev == pkt->p_gotsid.s_lev &&
				   pf.pf_gsid.s_br == pkt->p_gotsid.s_br &&
				   pf.pf_gsid.s_seq == pkt->p_gotsid.s_seq) ||
				   (pf.pf_nsid.s_rel == pkt->p_reqsid.s_rel &&
				   pf.pf_nsid.s_lev == pkt->p_reqsid.s_lev &&
				   pf.pf_nsid.s_br == pkt->p_reqsid.s_br &&
				   pf.pf_nsid.s_seq == pkt->p_reqsid.s_seq)) {
					fclose(in);
					sprintf(SccsError,
					   gettext("being edited: `%s' (ge17)"),
					   line);
					fatal(SccsError);
				}
				if (!equal(pf.pf_user,user))
					fprintf(stderr,warn,line);
			} else {
			   fprintf(stderr,warn,line);
			}
		}
		fclose(in);
	}
	if (fseek(out, (off_t)0, SEEK_END) == EOF)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
	sid_ba(&pkt->p_gotsid,str1);
	sid_ba(&pkt->p_reqsid,str2);
	/*
	 * POSIX does not explicitly mention a 2-digit year for the p-file but
	 * refers to "date-time" which is most likely expected to have 2-digits.
	 * We use 4-digits before 1969 and past 2068
	 * which is outside the year range specified by POSIX.
	 */
#if SIZEOF_TIME_T == 4
	if (Timenow.dt_sec < Y1969)
#else
	if ((Timenow.dt_sec < Y1969) ||
	    (Timenow.dt_sec >= Y2069))
#endif
		date_bal(&Timenow.dt_sec, line, 0);	/* 4 digit year */
	else
		date_ba(&Timenow.dt_sec, line, 0);	/* 2 digit year */
	if (fprintf(out,"%s %s %s %s",str1,str2,user,line) == EOF)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (inc)
		if (fprintf(out," -i%s",inc) == EOF)
			xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (exc)
		if (fprintf(out," -x%s",exc) == EOF)
			xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (cmrinsert(pkt) > 0)	/* if there are CMRS and they are okay */
		if (fprintf (out, " -z%s", cmr) == EOF)
			xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (fprintf(out, "\n") == EOF)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (fflush(out) == EOF)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
#ifdef	HAVE_FSYNC
	if (fsync(fileno(out)) < 0)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
#endif
	if (fclose(out) == EOF)
		xmsg(pfile, NOGETTEXT("wrtpfile"));
	if (pkt->p_verbose) {
                if (HADQ)
                   (void)fprintf(pkt->p_stdout, gettext("new version %s\n"),
				str2);
                else
                   (void)fprintf(pkt->p_stdout, gettext("new delta %s\n"),
				str2);
	}
}

/* cmrinsert -- insert CMR numbers in the p.file. */

static int
cmrinsert(pkt)
register struct packet *pkt;
{
	char holdcmr[CMRLIMIT];
	char tcmr[CMRLIMIT];
	char *p;
	int bad;
	int isvalid;

	if (pkt->p_sflags[CMFFLAG - 'a'] == 0)	{ /* CMFFLAG was not set. */
		return (0);
	}
	if (HADP && !HADZ) { /* no CMFFLAG and no place to prompt. */
		fatal(gettext("Background CASSI get with no CMRs\n"));
	}
retry:
	if (cmr[0] == '\0') {	/* No CMR list.  Make one. */
		if (HADZ && ((!isatty(0)) || (!isatty(1)))) {
		   fatal(gettext("Background CASSI get with invalid CMR\n"));
		}
		fprintf (stdout,
		   gettext("Input Comma Separated List of CMRs: "));
		fgets(cmr, CMRLIMIT, stdin);
		p=strend(cmr);
		*(--p) = '\0';
		if ((int)(p - cmr) == CMRLIMIT) {
		   fprintf(stdout, gettext("?Too many CMRs.\n"));
		   cmr[0] = '\0';
		   goto retry; /* Entry was too long. */
		}
	}
	/* Now, check the comma separated list of CMRs for accuracy. */
	bad = 0;
	isvalid = 0;
	strlcpy(tcmr, cmr, sizeof (tcmr));
	while ((p=strrchr(tcmr,',')) != NULL) {
		++p;
		if (cmrcheck(p, pkt->p_sflags[CMFFLAG - 'a'])) {
			++bad;
		} else {
			++isvalid;
			strlcat(holdcmr, ",", sizeof (holdcmr));
			strlcat(holdcmr, p, sizeof (holdcmr));
		}
		*(--p) = '\0';
	}
	if (*tcmr) {
		if (cmrcheck(tcmr, pkt->p_sflags[CMFFLAG - 'a'])) {
			++bad;
		} else {
			++isvalid;
			strlcat(holdcmr, ",", sizeof (holdcmr));
			strlcat(holdcmr, tcmr, sizeof (holdcmr));
		}
	}
	if (!bad && holdcmr[1]) {
	   strlcpy(cmr, holdcmr+1, sizeof (cmr));
	   return(1);
	} else {
	   if ((isatty(0)) && (isatty(1))) {
	      if (!isvalid)
		 fprintf(stdout,
		    gettext("Must enter at least one valid CMR.\n"));
	      else
		 fprintf(stdout,
		    gettext("Re-enter invalid CMRs, or press return.\n"));
	   }
	   cmr[0] = '\0';
	   goto retry;
	}
}
