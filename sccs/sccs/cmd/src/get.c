/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
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
 * Copyright 2006-2013 J. Schilling
 *
 * @(#)get.c	1.56 13/11/01 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)get.c 1.56 13/11/01 J. Schilling"
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
#define INCPATH		INS_BASE "/ccs/include"	/* place to find binaries */
#else
/*
 * XXX With a K&R compiler, you need to edit the following string in case
 * XXX you like to change the install path.
 */
#define INCPATH		"/usr/ccs/include"	/* place to find binaries */
#endif



/*
 * Get is now much more careful than before about checking
 * for write errors when creating the g- l- p- and z-files.
 * However, it remains deliberately unconcerned about failures
 * in writing statistical information (e.g., number of lines
 * retrieved).
 */

static struct sid sid;

static struct packet gpkt;
static unsigned	Ser;
static int	num_files;
static int	num_ID_lines;
static int	cnt_ID_lines;
static int	expand_IDs;
static char	*list_expand_IDs;
static char    *Whatstr = NULL;
static char	Pfilename[FILESIZE];
static char	*ilist, *elist, *lfile;
static time_t	cutoff = MAX_TIME;
static char	*cutoffstr;
static char	Gfile[PATH_MAX];
static char	gfile[PATH_MAX];
static char	*Type;
static struct utsname un;
static char *uuname;
static char	*Cwd = "";

/* Beginning of modifications made for CMF system. */
#define CMRLIMIT 128
static char	cmr[CMRLIMIT];
static int	cmri = 0;
/* End of insertion */

static	void    clean_up __PR((void));
static	void	enter	__PR((struct packet *pkt, int ch, int n, struct sid *sidp));

	int	main __PR((int argc, char **argv));
static void	get __PR((char *file));
static void	gen_lfile __PR((struct packet *pkt));
static void	idsetup __PR((struct packet *pkt));
static void	makgdate __PR((char *old, char *new));
static void	makgdatel __PR((char *old, char *new));
static char *	idsubst __PR((struct packet *pkt, char *line));
static char *	_trans __PR((char *tp, char *str, char *rest));
static int	readcopy __PR((char *name, char *tp));
static void	prfx __PR((struct packet *pkt));
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

	set_clean_up(clean_up);
	Fflags = FTLEXIT | FTLMSG | FTLCLN;
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
		        c = getopt(argc, argv, "-r:c:ebi:x:kl:Lpsmnogta:G:w:zqdC:V(version)");
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
    			case CMFFLAG:
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
				if (p) Whatstr = p;
				break;
			case 'c':
				if (*p == 0) continue;
				cutoffstr = p;
				if (parse_date(p,&cutoff, PF_GMT))
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

			case 'V':		/* version */
				printf("get %s-SCCS version %s %s (%s-%s-%s)\n",
					PROVIDER,
					VERSION,
					VDATE,
					HOST_CPU, HOST_VENDOR, HOST_OS);
				exit(EX_OK);

			default:
			   fatal(gettext("Usage: get [-begkmLopst] [-l[p]] [-asequence] [-cdate-time] [-Gg-file] [-isid-list] [-rsid] [-xsid-list] file ..."));
			
			}

			/*
			 * Make sure that we only collect option letters from
			 * the range 'a'..'z' and 'A'..'Z'.
			 */
			if (ALPHA(c) &&
			    (had[LOWER(c)? c-'a' : NLOWER+c-'A']++))
				fatal(gettext("key letter twice (cm2)"));
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
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if ((p=argv[i]) != NULL)
			do_file(p, get, 1, 1);

	return (Fcnt ? 1 : 0);
}

static void
get(file)
char *file;
{
	register char *p;
	register unsigned ser;
	extern char had_dir, had_standinp;
	struct stats stats;
	char	str[32];		/* Must fit a SID string */
#ifdef	PROTOTYPES
	char template[] = NOGETTEXT("/get.XXXXXX");
#else
	char *template = NOGETTEXT("/get.XXXXXX");
#endif
	char buf1[PATH_MAX];
	uid_t holduid;
	gid_t holdgid;
	static int first = 1;

	if (setjmp(Fjmp))
		return;
	if (HADE) {
		/*
		call `sinit' to check if file is an SCCS file
		but don't open the SCCS file.
		If it is, then create lock file.
		pass both the PID and machine name to lockit
		*/
		sinit(&gpkt,file,0);
		uname(&un);
		uuname = un.nodename;
		if (lockit(auxf(file,'z'),10, getpid(),uuname))
			efatal(gettext("cannot create lock file (cm4)"));
	}
	/*
	Open SCCS file and initialize packet
	*/
	sinit(&gpkt,file,1);
	gpkt.p_ixuser = (HADI | HADX);
	gpkt.p_reqsid.s_rel = sid.s_rel;
	gpkt.p_reqsid.s_lev = sid.s_lev;
	gpkt.p_reqsid.s_br = sid.s_br;
	gpkt.p_reqsid.s_seq = sid.s_seq;
	gpkt.p_verbose = (HADS) ? 0 : 1;
	gpkt.p_stdout  = (HADP||lfile) ? stderr : stdout;
	gpkt.p_cutoff = cutoff;
	gpkt.p_lfile = lfile;
	gpkt.p_enter = enter;
	if ((gpkt.p_flags & PF_V6) == 0) {
		gpkt.p_flags |= PF_GMT;
	} else if (cutoffstr != NULL) {
		if (parse_date(cutoffstr, &cutoff, 0))
			fatal(gettext("bad date/time (cm5)"));
		gpkt.p_cutoff = cutoff;
	}
	if (Gfile[0] == 0 || !first) {
		cat(gfile,Cwd,auxf(gpkt.p_file,'g'), (char *)0);
		cat(Gfile,Cwd,auxf(gpkt.p_file,'A'), (char *)0);
	}
	strlcpy(buf1, dname(Gfile), sizeof (buf1));
	strlcat(buf1, template, sizeof (buf1));
	Gfile[0] = '\0';		/* File not yet created */
	first = 0;

	if (gpkt.p_verbose && (num_files > 1 || had_dir || had_standinp))
		fprintf(gpkt.p_stdout,"\n%s:\n",gpkt.p_file);
	if (dodelt(&gpkt,&stats,(struct sid *) 0,0) == 0)
		fmterr(&gpkt);
	finduser(&gpkt);
	doflags(&gpkt);
	if ((p = gpkt.p_sflags[SCANFLAG - 'a']) == NULL) {
		num_ID_lines = 0;
	} else {
		num_ID_lines = atoi(p);
	}
	expand_IDs = 0;
	if ((list_expand_IDs = gpkt.p_sflags[EXPANDFLAG - 'a']) != NULL) {
		if (*list_expand_IDs) {
			/*
			 * The old Sun based code did break with get -k in case
			 * that someone did previously call admin -fy...
			 * Make sure that this now works correctly again.
			 */
			if (!HADK)		/* JS fix get -k */
				expand_IDs = 1;
		}
	}
	if (HADE) {
		expand_IDs = 0;
	}
	if (!HADK) {
		expand_IDs = 1;
	}
	if (!HADA) {
		ser = getser(&gpkt);
	} else {
		if ((ser = Ser) > maxser(&gpkt))
			fatal(gettext("serial number too large (ge19)"));
		gpkt.p_gotsid = gpkt.p_idel[ser].i_sid;
		if (HADR && sid.s_rel != gpkt.p_gotsid.s_rel) {
			zero((char *) &gpkt.p_reqsid, sizeof(gpkt.p_reqsid));
			gpkt.p_reqsid.s_rel = sid.s_rel;
		} else {
			gpkt.p_reqsid = gpkt.p_gotsid;
		}
	}
	doie(&gpkt,ilist,elist,(char *) 0);
	setup(&gpkt,(int) ser);
	if ((Type = gpkt.p_sflags[TYPEFLAG - 'a']) == NULL)
		Type = Null;
	if (!(HADP || HADG)) {
		if (exists(gfile) && (S_IWRITE & Statbuf.st_mode)) {
			sprintf(SccsError,gettext("writable `%s' exists (ge4)"),
				gfile);
			fatal(SccsError);
		}
	}
	if (gpkt.p_verbose) {
		sid_ba(&gpkt.p_gotsid,str);
		fprintf(gpkt.p_stdout,"%s\n",str);
	}
	if (HADE) {
		if (HADC || gpkt.p_reqsid.s_rel == 0)
			gpkt.p_reqsid = gpkt.p_gotsid;
		newsid(&gpkt, gpkt.p_sflags[BRCHFLAG - 'a'] && HADB);
		permiss(&gpkt);
		wrtpfile(&gpkt,ilist,elist);
	}
	if (!HADG || HADL) {
		if (gpkt.p_stdout) {
			fflush(gpkt.p_stdout);
			fflush (stderr);
		}
		holduid=geteuid();
		holdgid=getegid();
		setuid(getuid());
		setgid(getgid());
		if (HADL)
			gen_lfile(&gpkt);
		if (HADG)
			goto unlock;
		flushto(&gpkt,EUSERTXT,1);
		idsetup(&gpkt);
		gpkt.p_chkeof = 1;
		gpkt.p_did_id = 0;
		/*
		call `xcreate' which deletes the old `g-file' and
		creates a new one except if the `p' keyletter is set in
		which case any old `g-file' is not disturbed.
		The mod of the file depends on whether or not the `k'
		keyletter has been set.
		*/
		if (gpkt.p_gout == 0) {
			if (HADP) {
				gpkt.p_gout = stdout;
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
				if ((exists(gpkt.p_file) && (S_IEXEC & Statbuf.st_mode)) ||
				    (gpkt.p_flags & PF_SCOX)) {
					gpkt.p_gout = xfcreat(Gfile,HADK ? 
						((mode_t)0755) : ((mode_t)0555));
				} else {
					gpkt.p_gout = xfcreat(Gfile,HADK ? 
						((mode_t)0644) : ((mode_t)0444));
				}
#ifdef	USE_SETVBUF
				/*
				 * Do not call setvbuf() with stdout as this may result
				 * in a second illegal call in gen_lfile().
				 */
				setvbuf(gpkt.p_gout, NULL, _IOFBF, VBUF_SIZE);
#endif
			}
		}
		gpkt.p_ghash = 0;		/* Reset ghash */
		if (gpkt.p_encoding & EF_UUENCODE) {
			while (readmod(&gpkt)) {
				decode(gpkt.p_line,gpkt.p_gout);
			}
		} else {
			while (readmod(&gpkt)) {
				prfx(&gpkt);
				if (gpkt.p_flags & PF_NONL)
					gpkt.p_line[gpkt.p_line_length-1] = '\0';
				p = idsubst(&gpkt, gpkt.p_lineptr);
				if (fputs(p,gpkt.p_gout)==EOF)
					xmsg(gfile, NOGETTEXT("get"));
			}
		}
		if ((gpkt.p_flags & (PF_V6 | PF_V6TAGS)) && gpkt.p_hash)
			if (gpkt.p_hash[ser] != (gpkt.p_ghash & 0xFFFF))
				fatal(gettext("corrupted file version (co27)"));
			
		if (gpkt.p_gout) {
			if (fflush(gpkt.p_gout) == EOF)
				xmsg(gfile, NOGETTEXT("get"));
			fflush (stderr);
		}
		if (gpkt.p_gout && gpkt.p_gout != stdout) {
			/*
			 * Force g-file to disk and verify
			 * that it actually got there.
			 */
#ifdef	HAVE_FSYNC
			if (fsync(fileno(gpkt.p_gout)) < 0)
				xmsg(gfile, NOGETTEXT("get"));
#endif
			if (fclose(gpkt.p_gout) == EOF)
				xmsg(gfile, NOGETTEXT("get"));
			gpkt.p_gout = NULL;
		}
		if (gpkt.p_verbose) {
#ifdef XPG4		
		   fprintf(gpkt.p_stdout, NOGETTEXT("%d lines\n"), gpkt.p_glnno);
#else
		   if (HADD == 0)
		      fprintf(gpkt.p_stdout,gettext("%d lines\n"),gpkt.p_glnno);
#endif
		}		
		if (!gpkt.p_did_id && !HADK && !HADQ &&
		    (!gpkt.p_sflags[EXPANDFLAG - 'a'] ||
		    *(gpkt.p_sflags[EXPANDFLAG - 'a']))) {
		   if (gpkt.p_sflags[IDFLAG - 'a']) {
		      if (!(*gpkt.p_sflags[IDFLAG - 'a'])) {
		         fatal(gettext("no id keywords (cm6)"));
		      } else {
			 fatal(gettext("invalid id keywords (cm10)"));
		      }
		   } else {
		      if (gpkt.p_verbose) {
			 fprintf(stderr, gettext("No id keywords (cm7)\n"));
		      }
		   }
		}
		if (Gfile[0] != '\0' && exists(Gfile) ) {
			rename(Gfile, gfile);
			if (HADO) {
				struct tm	tm;
				struct timespec	ts[2];
				unsigned int	gser;
				extern dtime_t	Timenow;

				gser = sidtoser(&gpkt.p_gotsid, &gpkt);

				ts[0].tv_sec = Timenow.dt_sec;
				ts[0].tv_nsec = Timenow.dt_nsec;
				ts[1].tv_sec = gpkt.p_idel[gser].i_datetime.tv_sec;
				ts[1].tv_nsec = gpkt.p_idel[gser].i_datetime.tv_nsec;
				/*
				 * If we did cheat while scanning the delta
				 * table and converted the time stamps assuming
				 * GMT. Fix the resulting error here.
				 */
				if (gpkt.p_flags & PF_GMT) {
					tm = *gmtime(&ts[1].tv_sec);
					tm.tm_isdst = -1;
					ts[1].tv_sec = mktime(&tm);
				}
				utimensat(AT_FDCWD, gfile, ts, 0);
			}
		}
		setuid(holduid);
		setgid(holdgid);
	}
	if (gpkt.p_iop) {
		fclose(gpkt.p_iop);
		gpkt.p_iop = NULL;
	}
unlock:
	if (HADE) {
		copy(auxf(gpkt.p_file,'p'),Pfilename);
		rename(auxf(gpkt.p_file,'q'),Pfilename);
		uname(&un);
		uuname = un.nodename;
		unlockit(auxf(gpkt.p_file,'z'), getpid(),uuname);
	}
	ffreeall();
}

static void
enter(pkt,ch,n,sidp)
struct packet *pkt;
char ch;
int n;
struct sid *sidp;
{
	char	str[32];		/* Must fit a SID string */
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
	char str[DT_ZSTRSIZE];		/* SCCS v6 date or SID str */
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
		if (dt.d_type == 'D') {
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
					if (dt.d_type == 'D') {
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

static char	Curdatel[DT_ZSTRSIZE];
static char	Curdate[DT_ZSTRSIZE];
static char	*Curtime;
static char	Gdate[DATELEN];
static char	Gdatel[DATELEN];
static char	Chgdate[DT_ZSTRSIZE];
static char	Chgdatel[DT_ZSTRSIZE];
static char	*Chgtime;
static char	Gchgdate[DATELEN];
static char	Gchgdatel[DATELEN];
static char	Sid[32];
static char	Mod[FILESIZE];
static char	Olddir[BUFSIZ];
static char	Pname[BUFSIZ];
static char	Dir[BUFSIZ];
static char	*Qsect;

static void
idsetup(pkt)
register struct packet *pkt;
{
	extern dtime_t Timenow;
	register int n;
	register char *p;

	date_ba(&Timenow.dt_sec, Curdate, 0);
	date_bal(&Timenow.dt_sec, Curdatel, 0);
	Curdatel[10] = 0;
	Curtime = &Curdate[9];
	Curdate[8] = 0;
	makgdate(Curdate,Gdate);
	makgdatel(Curdatel,Gdatel);
	for (n = maxser(pkt); n; n--)
		if (pkt->p_apply[n].a_code == APPLY)
			break;
	if (n) {
		date_ba(&pkt->p_idel[n].i_datetime.tv_sec, Chgdate, pkt->p_flags);
		date_bal(&pkt->p_idel[n].i_datetime.tv_sec, Chgdatel, pkt->p_flags);
	} else {
#ifdef XPG4
		FILE	*xf;

		if (exists(gfile))
			unlink(gfile);
		xf = xfcreat(gfile, HADK ? ((mode_t)0644) : ((mode_t)0444));
		if (xf)
			fclose(xf);
#endif
		fatal(gettext("No sid prior to cutoff date-time (ge23)"));
	}
	Chgdatel[10] = 0;
	Chgtime = &Chgdate[9];
	Chgdate[8] = 0;
	makgdate(Chgdate,Gchgdate);
	makgdatel(Chgdatel,Gchgdatel);
	sid_ba(&pkt->p_gotsid,Sid);
	if ((p = pkt->p_sflags[MODFLAG - 'a']) != NULL)
		copy(p,Mod);
	else
		copy(auxf(gpkt.p_file,'g'), Mod);
	if ((Qsect = pkt->p_sflags[QSECTFLAG - 'a']) == NULL)
		Qsect = Null;
}

#ifdef	HAVE_STRFTIME
static void 
makgdate(old,new)
register char *old, *new;
{
	struct tm	tm;
	sscanf(old, "%d/%d/%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday));
	tm.tm_mon--;
	strftime (new, (size_t) DATELEN, NOGETTEXT("%D"), &tm);
}

static void 
makgdatel(old,new)
register char *old, *new;
{
	struct tm	tm;
	sscanf(old, "%d/%d/%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday));
	if (tm.tm_year > 1900)
		tm.tm_year -= 1900;
	tm.tm_mon--;
	strftime (new, (size_t) DATELEN, NOGETTEXT("%m/%d/%Y"), &tm);
}
	

#else	/* HAVE_STRFTIME not defined */

static void
makgdate(old,new)
register char *old, *new;
{
#define	NULL_PAD_DATE		/* Make it compatible to strftime()/man page */
#ifndef	NULL_PAD_DATE
	if ((*new = old[3]) != '0')
		new++;
#else
	*new++ = old[3];	
#endif
	*new++ = old[4];
	*new++ = '/';
#ifndef	NULL_PAD_DATE
	if ((*new = old[6]) != '0')
		new++;
#else
	*new++ = old[6];
#endif
	*new++ = old[7];
	*new++ = '/';
	*new++ = old[0];
	*new++ = old[1];
	*new = 0;
}

static void
makgdatel(old,new)
register char *old, *new;
{
	char	*p;
	int	off;

	for (p = old; *p >= '0' && *p <= '9'; p++)
		;
	off = p - old - 2;
	if (off < 0)
		off = 0;
	
#ifndef	NULL_PAD_DATE
	if ((*new = old[3+off]) != '0')
		new++;
#else
	*new++ = old[3+off];
#endif
	*new++ = old[4+off];
	*new++ = '/';
#ifndef	NULL_PAD_DATE
	if ((*new = old[6+off]) != '0')
		new++;
#else
	*new++ = old[6+off];
#endif
	*new++ = old[7+off];
	*new++ = '/';
	*new++ = old[0];
	*new++ = old[1];
	if (off > 0)
		*new++ = old[2];
	if (off > 1)
		*new++ = old[3];
	*new = 0;
}
#endif	/* HAVE_STRFTIME */


static char Zkeywd[5] = "@(#)";
static char *tline = NULL;
static size_t tline_size = 0;

#define trans(a, b)	_trans(a, b, lp)

static char *
idsubst(pkt,line)
register struct packet *pkt;
char line[];
{
	static char *hold = NULL;
	static size_t hold_size = 0;
	size_t new_hold_size;
	static char str[32];
	register char *lp, *tp;
	int recursive = 0;
	int expand_XIDs;
	char *expand_ID;
	register char **sflags = pkt->p_sflags;
		
	cnt_ID_lines++;
	if (HADK) {
		if (!expand_IDs)
			return(line);
	} else {
		if (!any('%', line))
			return(line);
	}
	if (cnt_ID_lines > num_ID_lines) {
		if (!expand_IDs) {
			return(line);
		}
		if (num_ID_lines) {
			expand_IDs = 0;
			return(line);
		}
	}
	expand_XIDs = sflags[EXTENSFLAG - 'a'] != NULL || list_expand_IDs != NULL;
	tp = tline;
	for (lp=line; *lp != 0; lp++) {
		if ((!pkt->p_did_id) && (sflags[IDFLAG - 'a']) &&
		    (!(strncmp(sflags[IDFLAG-'a'],lp,strlen(sflags[IDFLAG-'a'])))))
				pkt->p_did_id = 1;
		if (lp[0] == '%' && lp[1] != 0 && lp[2] == '%') {
			expand_ID = ++lp;
			if (!HADK && expand_IDs && (list_expand_IDs != NULL)) {
				if (!any(*expand_ID, list_expand_IDs)) {
					str[3] = '\0';
					strncpy(str, lp - 1, 3);
					tp = trans(tp, str);
					lp++;
					continue;
				}
			}
			switch (*expand_ID) {

			case 'M':
				tp = trans(tp,Mod);
				break;
			case 'Q':
				tp = trans(tp,Qsect);
				break;
			case 'R':
				sprintf(str,"%d",pkt->p_gotsid.s_rel);
				tp = trans(tp,str);
				break;
			case 'L':
				sprintf(str,"%d",pkt->p_gotsid.s_lev);
				tp = trans(tp,str);
				break;
			case 'B':
				sprintf(str,"%d",pkt->p_gotsid.s_br);
				tp = trans(tp,str);
				break;
			case 'S':
				sprintf(str,"%d",pkt->p_gotsid.s_seq);
				tp = trans(tp,str);
				break;
			case 'D':
				tp = trans(tp,Curdate);
				break;
			case 'd':
				if (!expand_XIDs)
					goto noexpand;
				tp = trans(tp,Curdatel);
				break;
			case 'H':
				tp = trans(tp,Gdate);
				break;
			case 'h':
				if (!expand_XIDs)
					goto noexpand;
				tp = trans(tp,Gdatel);
				break;
			case 'T':
				tp = trans(tp,Curtime);
				break;
			case 'E':
				tp = trans(tp,Chgdate);
				break;
			case 'e':
				if (!expand_XIDs)
					goto noexpand;
				tp = trans(tp,Chgdatel);
				break;
			case 'G':
				tp = trans(tp,Gchgdate);
				break;
			case 'g':
				if (!expand_XIDs)
					goto noexpand;
				tp = trans(tp,Gchgdatel);
				break;
			case 'U':
				tp = trans(tp,Chgtime);
				break;
			case 'Z':
				tp = trans(tp,Zkeywd);
				break;
			case 'Y':
				tp = trans(tp,Type);
				break;
			/*FALLTHRU*/
			case 'W':
				if ((Whatstr != NULL) && (recursive == 0)) {
					recursive = 1;
					lp += 2;
					new_hold_size = strlen(lp) + strlen(Whatstr) + 1;
					if (new_hold_size > hold_size) {
						if (hold)
							free(hold);
						hold_size = new_hold_size;
						hold = (char *)malloc(hold_size);
						if (hold == NULL) {
							efatal(gettext("OUT OF SPACE (ut9)"));
						}
					}
					strcpy(hold,Whatstr);
					strcat(hold,lp);
					lp = hold;
					lp--;
					continue;
				}
				tp = trans(tp,Zkeywd);
				tp = trans(tp,Mod);
				tp = trans(tp,"\t");
			case 'I':
				tp = trans(tp,Sid);
				break;
			case 'P':
				copy(pkt->p_file,Dir);
				dname(Dir);
				if (getcwd(Olddir,sizeof(Olddir)) == NULL)
				   efatal(gettext("getcwd() failed (ge20)"));
				if (chdir(Dir) != 0)
				   efatal(gettext("cannot change directory (ge21)"));
			  	if (getcwd(Pname,sizeof(Pname)) == NULL)
				   efatal(gettext("getcwd() failed (ge20)"));
				if (chdir(Olddir) != 0)
				   efatal(gettext("cannot change directory (ge21)"));
				tp = trans(tp,Pname);
				tp = trans(tp,"/");
				tp = trans(tp,(sname(pkt->p_file)));
				break;
			case 'F':
				tp = trans(tp,pkt->p_file);
				break;
			case 'C':
				sprintf(str,"%d",pkt->p_glnno);
				tp = trans(tp,str);
				break;
			case 'A':
				tp = trans(tp,Zkeywd);
				tp = trans(tp,Type);
				tp = trans(tp," ");
				tp = trans(tp,Mod);
				tp = trans(tp," ");
				tp = trans(tp,Sid);
				tp = trans(tp,Zkeywd);
				break;
			noexpand:
			default:
				str[0] = '%';
				str[1] = *lp;
				str[2] = '\0';
				tp = trans(tp,str);
				continue;
			}
			if (!(sflags[IDFLAG - 'a']))
				pkt->p_did_id = 1;
			lp++;
		} else if (strncmp(lp, "%sccs.include.", 14) == 0) {
			register char *p;

			for (p = lp + 14; *p && *p != '%'; ++p);
			if (*p == '%') {
				*p = '\0';
				if (readcopy(lp + 14, tline))
					return(tline);
				*p = '%';
			}
			str[0] = *lp;
			str[1] = '\0';
			tp = trans(tp,str);
		} else {
			str[0] = *lp;
			str[1] = '\0';
			tp = trans(tp,str);
		}
	}
	return(tline);
}

static char *
_trans(tp,str,rest)
register char *tp, *str, *rest;
{
	register size_t filled_size	= tp - tline;
	register size_t new_size	= filled_size + strlen(str) + 1;

	if (new_size > tline_size) {
		tline_size = new_size + strlen(rest) + DEF_LINE_SIZE;
		tline = (char*) realloc(tline, tline_size);
		if (tline == NULL) {
			efatal(gettext("OUT OF SPACE (ut9)"));
		}
		tp = tline + filled_size;
	}
	while((*tp++ = *str++) != '\0')
		;
	return(tp-1);
}

#ifndef	HAVE_STRERROR
#define	strerror(p)		errmsgstr(p)	/* Use libschily version */
#endif
static int
readcopy(name, tp)
	register char *name;
	register char *tp;
{
	register FILE *fp = NULL;
	register int ch;
	char path[MAXPATHLEN];
	struct stat sb;
	register size_t filled_size	= tp - tline;
	register size_t new_size	= filled_size;

	if (!HADK && expand_IDs && (list_expand_IDs != NULL)) {
		if (!any('*', list_expand_IDs))
			return (0);
	}
	if (fp == NULL) {
		char *np;
		char *p = getenv(NOGETTEXT("SCCS_INCLUDEPATH"));

		if (p == NULL)
			p = INCPATH;
		do {
			for (np = p; *np; np++)
				if (*np == ':')
					break;
			(void)snprintf(path, sizeof (path), "%.*s/%s",
						(int)(np-p), p, name);
			if (p == np || *p == '\0')
				strlcpy(path, name, sizeof (path));
			fp = fopen(path, "r");
			p = np;
			if (*p == '\0')
				break;
			if (*p == ':')
				p++;
		} while (fp == NULL);
	}
	if (fp == NULL) {
		fprintf(stderr, gettext("WARNING: %s: %s.\n"),
						path, strerror(errno));
		return (0);
	}
	if (fstat(fileno(fp), &sb) < 0)
		return (0);
	new_size += sb.st_size + 1;

	if (new_size > tline_size) {
		tline_size = new_size + DEF_LINE_SIZE;
		tline = (char*) realloc(tline, tline_size);
		if (tline == NULL) {
			efatal(gettext("OUT OF SPACE (ut9)"));
		}
		tp = tline + filled_size;
	}

	while ((ch = getc(fp)) != EOF)
		*tp++ = ch;
	*tp++ = '\0';
	(void)fclose(fp);
	return (1);
}

static void
prfx(pkt)
register struct packet *pkt;
{
	char str[32];		/* Must fit a SID string */

	if (HADN)
		if (fprintf(pkt->p_gout, "%s\t", Mod) == EOF)
			xmsg(gfile, NOGETTEXT("prfx"));
	if (HADM) {
		sid_ba(&pkt->p_inssid,str);
		if (fprintf(pkt->p_gout, "%s\t", str) == EOF)
			xmsg(gfile, NOGETTEXT("prfx"));
	}
}

static void
clean_up()
{
	/*
	clean_up is only called from fatal() upon bad termination.
	*/
	if (gpkt.p_iop) {
		fclose(gpkt.p_iop);
		gpkt.p_iop = NULL;
	}
	if (gpkt.p_gout) {
		fflush(gpkt.p_gout);
		gpkt.p_gout = NULL;
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
	      unlockit(auxf(gpkt.p_file,'z'), getpid(), uuname);
	   }
	}
	ffreeall();
}

static	char	warn[] = NOGETTEXT("WARNING: being edited: `%s' (ge18)\n");

static void
wrtpfile(pkt,inc,exc)
register struct packet *pkt;
char *inc, *exc;
{
	char line[64];
	char str1[32], str2[32];	/* Must fit a SID string */
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
