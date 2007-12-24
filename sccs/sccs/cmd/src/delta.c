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
 * @(#)delta.c	1.6 07/12/11 J. Schilling
 */
#if defined(sun) || defined(__GNUC__)

#ident "@(#)delta.c 1.6 07/12/11 J. Schilling"
#endif
/*
 * @(#)delta.c 1.40 06/12/12
 */

#ident	"@(#)delta.c"
#ident	"@(#)sccs:cmd/delta.c"

# include	<defines.h>
# include	<version.h>
# include	<had.h>
# include	<i18n.h>
# include	<sys/utsname.h>
# include	<schily/wait.h>
# include	<ccstypes.h>
# include	<sysexits.h>

struct stat Statbuf;
char Null[1];
char SccsError[MAXERRORLEN];

# define	LENMR	60

static FILE	*Diffin, *Gin;
static struct packet	gpkt;
static struct utsname	un;
static int	num_files;
static int	number_of_lines;
static off_t	size_of_file;
static off_t	Szqfile;
#ifdef	PROTOTYPES
static char	BDiffpgm[]  =   NOGETTEXT(INS_BASE "/ccs/bin/bdiff");
#else
/*
 * XXX If you are using a K&R compiler and like to install to a path
 * XXX different from "/usr/ccs/bin/", you need to edit this string.
 */
static char	BDiffpgm[]  =   NOGETTEXT("/usr/ccs/bin/bdiff");
#endif
static char	BDiffpgm2[]  =   NOGETTEXT("bdiff");
static char	Diffpgm[]   =   NOGETTEXT("/usr/bin/diff");
static char	Diffpgm2[]   =   NOGETTEXT("/bin/diff");
static char	*ilist, *elist, *glist, Cmrs[300], *Nsid;
static char	Pfilename[FILESIZE];
static char	*uuname;

static	time_t	gfile_mtime;
static	time_t	cutoff = (time_t)0x7FFFFFFFL;

struct sid sid;
char	*Comments,*Mrs;
int	Domrs;
int	Did_id;

extern FILE	*Xiop;
extern int	Xcreate;

	void    clean_up __PR((void));
	void	enter	__PR((struct packet *pkt, int ch, int n, struct sid *sidp));

	int	main __PR((int argc, char **argv));
static void	delta __PR((char *file));
static int	mkdelt __PR((struct packet *pkt, struct sid *sp, struct sid *osp, int diffloop, int orig_nlines));
static void	mkixg __PR((struct packet *pkt, int reason, int ch));
static void	putmrs __PR((struct packet *pkt));
static void	putcmrs __PR((struct packet *pkt));
static struct pfile *rdpfile __PR((struct packet *pkt, struct sid *sp));
static FILE *	dodiff __PR((char *newf, char *oldf, int difflim));
static int	getdiff __PR((char *type, int *plinenum));
static void	insert __PR((struct packet *pkt, int linenum, int n, int ser));
static void	delete __PR((struct packet *pkt, int linenum, int n, int ser));
static void	after __PR((struct packet *pkt, int n));
static void	before __PR((struct packet *pkt, int n));
static char *	linerange __PR((char *cp, int *low, int *high));
static void	skipline __PR((char *lp, int num));
static char *	rddiff __PR((char *s, int n));
static void	fgetchk __PR((char *file, struct packet *pkt));

extern int	org_ihash;
extern int	org_chash;
extern int	org_uchash;
extern char	saveid[];

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
		        c = getopt(argc, argv, "-r:dpsnm:g:y:fhqzV(version)");

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

			case 'V':		/* version */
				printf("delta %s-SCCS version %s (%s-%s-%s)\n",
					PROVIDER,
					VERSION,
					HOST_CPU, HOST_VENDOR, HOST_OS);
				exit(EX_OK);

			default:
				fatal(gettext("Usage: delta [ -dnps ][ -g sid-list ][ -m mr-list ]\n\t[ -r SID ][ -y[comment] ] s.filename... "));
			}

			/* The following is necessary in case the user types */
			/* some localized character,  which will exceed the */
			/* limit of the array "had", defined in ../hdr/had.h */
			if ((c - 'a') < 0 || (c - 'a') > 25) 
			       continue;
			if (had[c - 'a']++)
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
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;
	for (i=1; i<argc; i++)
		if ((p=argv[i]) != NULL)
			do_file(p, delta, 1);

	return (Fcnt ? 1 : 0);
}

static void
delta(file)
char *file;
{
	static int first = 1;
	int n, linenum;
	char type;
	register int ser;
	extern char had_dir, had_standinp;
	extern char *Sflags[];
	char nsid[50];
	char dfilename[FILESIZE];
	char gfilename[FILESIZE];
	char line[BUFSIZ];
	struct stats stats;
	struct pfile *pp;
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
	sinit(&gpkt,file,1);
	if (first) {
		first = 0;
		dohist(file);
	}
	uname(&un);
	uuname = un.nodename;
	if (lockit(auxf(gpkt.p_file,'z'),SCCS_LOCK_ATTEMPTS,getpid(),uuname))
		fatal(gettext("cannot create lock file (cm4)"));
	gpkt.p_reopen = 1;
	gpkt.p_stdout = stdout;
	copy(auxf(gpkt.p_file,'g'),gfilename);
	Gin = xfopen(gfilename, O_RDONLY|O_BINARY);
	pp = rdpfile(&gpkt,&sid);

	if((pp->pf_cmrlist)==0)
		Cmrs[0] = 0;
	else
		strcpy(Cmrs,pp->pf_cmrlist);

	if(!pp->pf_nsid.s_br) {
		sprintf(nsid, NOGETTEXT("%d.%d"),
		  pp->pf_nsid.s_rel, pp->pf_nsid.s_lev);
	}
	else {
		sprintf(nsid, NOGETTEXT("%d.%d.%d.%d"),
		  pp->pf_nsid.s_rel, pp->pf_nsid.s_lev,
		  pp->pf_nsid.s_br,  pp->pf_nsid.s_seq);
	}
	Nsid=nsid;

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
                gpkt.p_cutoff = time(0);
                ilist = 0;
                elist = 0;
                ser = getser(&gpkt);
                newsid(&gpkt, 0);
        } else {        
                gpkt.p_cutoff = pp->pf_date;
                ilist = pp->pf_ilist;
                elist = pp->pf_elist;
                if ((ser = sidtoser(&pp->pf_gsid,&gpkt)) == 0 ||
                        sidtoser(&pp->pf_nsid,&gpkt))
                                fatal(gettext("invalid sid in p-file (de3)"));
		gpkt.p_reqsid = pp->pf_nsid;
        }
        if (HADQ && stat(gfilename, &sbuf) == 0) {
                /* In NSE mode, the mtime of the clear file is remembered for
                 * use as delta time. Sccs is thus now vulnerable to clock
                 * skew between NFS server and host machine and to a mis-set
                 * clock when file is last changed.
                 */
                gfile_mtime = sbuf.st_mtime;
	}

	doie(&gpkt,ilist,elist,glist);
	setup(&gpkt,ser);
	finduser(&gpkt);
	doflags(&gpkt);
	gpkt.p_reqsid = pp->pf_nsid;
	permiss(&gpkt);
	flushto(&gpkt,EUSERTXT,1);
	gpkt.p_chkeof = 1;
	/* if encode flag is set, encode the g-file before diffing it
	 * with the s.file
	 */
	if (Sflags[ENCODEFLAG - 'a'] && (strcmp(Sflags[ENCODEFLAG - 'a'],"1") == 0)) {
		efp = xfcreat(auxf(gpkt.p_file,'e'),0644);
		encode(Gin,efp);
		fclose(efp);
		if (Gin)
			fclose(Gin);
		Gin = xfopen(auxf(gpkt.p_file,'e'), O_RDONLY|O_BINARY);
	}
	copy(auxf(gpkt.p_file,'d'),dfilename);
	gpkt.p_gout = xfcreat(dfilename,(mode_t)0444);
	while(readmod(&gpkt)) {
		chkid(gpkt.p_line,Sflags['i'-'a']);
		if(fputs(gpkt.p_line,gpkt.p_gout)==EOF)
			xmsg(dfilename, NOGETTEXT("delta"));
	}
	if (fflush(gpkt.p_gout) == EOF)
		xmsg(dfilename, NOGETTEXT("delta"));
	if (fsync(fileno(gpkt.p_gout)) < 0)
		xmsg(dfilename, NOGETTEXT("delta"));
	if (fclose(gpkt.p_gout) == EOF)
		xmsg(dfilename, NOGETTEXT("delta"));
	gpkt.p_gout = NULL;
	orig = gpkt.p_glnno;
	gpkt.p_glnno = 0;
	gpkt.p_verbose = (HADS) ? 0 : 1;
 	Did_id = number_of_lines = size_of_file = 0;
 	while (fgets(line,sizeof(line),Gin) != NULL) {
 	   if (line[strlen(line)-1] == '\n') {
 	   	number_of_lines++;
 	   }
 	   if (Did_id == 0) {
 	      chkid(line,Sflags['i'-'a']);
 	   }
 	}
 	if (stat(gfilename, &Statbuf) == 0) {
 	   size_of_file = Statbuf.st_size;
 	}
	if (Gin)
		fclose(Gin);
	Gin = NULL;
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

	if ((Sflags[ENCODEFLAG - 'a'] == NULL) ||
	    (strcmp(Sflags[ENCODEFLAG - 'a'],"0") == 0)) {
		fgetchk(gfilename, &gpkt);
	}

	if (!Did_id && !HADQ) {
		if (Sflags[IDFLAG - 'a'])
			if(!(*Sflags[IDFLAG - 'a']))
				fatal(gettext("no id keywords (cm6)"));
			else
				fatal(gettext("invalid id keywords (cm10)"));
		else if (gpkt.p_verbose)
			fprintf(stderr,gettext("No id keywords (cm7)\n"));
	}

	/*
	The following while loop executes 'bdiff' on g-file and
	d-file. If 'bdiff' fails (usually because segmentation
	limit it is using is too large for 'diff'), it is
	invoked again, with a lower segmentation limit.
	*/
	difflim = 24000;
	diffloop = 0;
	/*CONSTCOND*/
	while (1) {
		inserted = deleted = 0;
		gpkt.p_glnno = 0;
		gpkt.p_upd = 1;
		gpkt.p_wrttn = 1;
		getline(&gpkt);
		gpkt.p_wrttn = 1;
        	if (HADF) {
                	newser = mkdelt(&gpkt, &gpkt.p_reqsid, &gpkt.p_gotsid, 
				diffloop, orig);
       		 } else {
                	newser = mkdelt(&gpkt,&pp->pf_nsid,&pp->pf_gsid, 
				diffloop, orig);
        	}
		diffloop = 1;
		flushto(&gpkt,EUSERTXT,0);
		if (Sflags[ENCODEFLAG - 'a'] && (strcmp(Sflags[ENCODEFLAG - 'a'],"1") == 0))
			Diffin = dodiff(auxf(gpkt.p_file,'e'),dfilename,difflim);
		else
			Diffin = dodiff(gfilename, dfilename, difflim);

		while ((n = getdiff(&type,&linenum)) != 0) {
			if (type == INS) {
				inserted += n;
				insert(&gpkt,linenum,n,newser);
			}
			else {
				deleted += n;
				delete(&gpkt,linenum,n,newser);
			}
		}
		if (Diffin)
			fclose(Diffin);
		Diffin = NULL;
		if (gpkt.p_iop)
			while (readmod(&gpkt))
				;
		wait(&status);
 		/*
 		 Check top byte (exit code of child).
 		*/

 		/*
 		 * The logical shift operator construct ">>" below
 		 * does not violate i18n rules, since "status" is
 		 * an internally used variable and is not used
 		 * for i/o.
 		 */
 		if (((status >> 8) & 0377) == 32) { /* 'execl' failed */
 		   sprintf(SccsError,
 		      gettext("cannot execute '%s' (de12)"), BDiffpgm);
 		   fatal(SccsError);
 		}
 		if ((status != 0) && (HADD == 0)) { /* diff failed */
			/*
			Re-try.
			*/
			if (difflim -= 3000) {	/* reduce segmentation */
				fprintf(stderr,
					gettext("'%s' failed, re-trying, segmentation = %d (de13)\n"),
 					BDiffpgm,
					difflim);
				if (Xiop)
					fclose(Xiop);	/* set up */
				Xiop = 0;		/* for new x-file */
				Xcreate = 0;
				/*
				Re-open s-file.
				*/
				gpkt.p_iop = xfopen(gpkt.p_file, O_RDONLY|O_BINARY);
				setbuf(gpkt.p_iop,gpkt.p_buf);
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
		}
		else {		/* no need to try again, worked */
			break;			/* exit while loop */
		}
	}
	if (Sflags[ENCODEFLAG - 'a'] && 
	    (strcmp(Sflags[ENCODEFLAG - 'a'],"1") == 0)) {
		unlink(auxf(gpkt.p_file,'e'));
	}
	unlink(dfilename);
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
	if (!HADF) {
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
	clean_up();
	if (!HADN) {
		fflush(gpkt.p_stdout);
		holduid=geteuid();
		holdgid=getegid();
		setuid(getuid());
		setgid(getgid());
		unlink(gfilename);
		setuid(holduid);
		setgid(holdgid);
	}
}

static int
mkdelt(pkt,sp,osp,diffloop,orig_nlines)
struct packet *pkt;
struct sid *sp, *osp;
int diffloop;
int orig_nlines;
{
	extern time_t Timenow;
	struct deltab dt;
	char str[BUFSIZ];
	int newser;
	extern char *Sflags[];
	register char *p;
	int ser_inc, opred, nulldel;

	if (!diffloop && pkt->p_verbose) {
		sid_ba(sp,str);
		fprintf(pkt->p_stdout,"%s\n",str);
		fflush(pkt->p_stdout);
	}
	sprintf(str, NOGETTEXT("%c%c00000\n"), CTLCHAR, HEAD);
	putline(pkt,str);
	newstats(pkt,str,"0");
	dt.d_sid = *sp;

	/*
	Check if 'null' deltas should be inserted
	(only if 'null' flag is in file and
	releases are being skipped) and set
	'nulldel' indicator appropriately.
	*/
	if (Sflags[NULLFLAG - 'a'] && (sp->s_rel > osp->s_rel + 1) &&
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
	dt.d_datetime = Timenow;
        /* Since the NSE always preserves the clear file after delta and
         * makes it read only (no get is required since keywords are not
         * supported), the delta time is set to be the mtime of the clear
         * file.
         */
        if (HADQ && (gfile_mtime != 0)) {
                dt.d_datetime = gfile_mtime;
        }
	strncpy(dt.d_pgmr,logname(),LOGSIZE-1);
	dt.d_type = 'D';
	del_ba(&dt,str);
	putline(pkt,str);
	if (ilist)
		mkixg(pkt,INCLUSER,INCLUDE);
	if (elist)
		mkixg(pkt,EXCLUSER,EXCLUDE);
	if (glist)
		mkixg(pkt,IGNRUSER,IGNORE);
	if (Mrs) {
		if ((p = Sflags[VALFLAG - 'a']) == NULL)
			fatal(gettext("MRs not allowed (de8)"));
		if (*p && !diffloop && valmrs(pkt,p))
			fatal(gettext("invalid MRs (de9)"));
		putmrs(pkt);
	}
	else if (Sflags[VALFLAG - 'a'] && !HADQ)
		fatal(gettext("MRs required (de10)"));
/*
*
* CMF enhancement
*
*/
	if(Sflags[CMFFLAG - 'a']) {
		if (Mrs) {
			 error(gettext("input CMR's ignored"));
			 Mrs = NOGETTEXT("");
		}
		if(!deltack(pkt->p_file,Cmrs,Nsid,Sflags[CMFFLAG - 'a'])) {
			 fatal(gettext("Delta denied due to CMR difficulties"));
		}
		putcmrs(pkt); /* this routine puts cmrs on the out put file */
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
			del_ba(&dt,str);
			putline(pkt,str);
			sprintf(str, NOGETTEXT("%c%c "), CTLCHAR, COMMENTS);
			putline(pkt,str);
			putline(pkt,NOGETTEXT("AUTO NULL DELTA\n"));
			sprintf(str,CTLSTR,CTLCHAR,EDELTAB);
			putline(pkt,str);
		}
	return(newser);
}

static void
mkixg(pkt,reason,ch)
struct packet *pkt;
int reason;
char ch;
{
	int n;
 	char str[BUFSIZ];

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

static struct pfile *
rdpfile(pkt,sp)
register struct packet *pkt;
struct sid *sp;
{
	char *user;
	struct pfile pf;
	static struct pfile goodpf;
	char line[BUFSIZ];
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
	   fatal(gettext("cannot create lock file (cm4)"));
	}
	fchmod(fd, (mode_t)0644);
	out = fdfopen(fd, O_WRONLY|O_BINARY);
	while (fgets(line,sizeof(line),in) != NULL) {
		pf_ab(line,&pf,1);
		pf.pf_date = cutoff;
		if (equal(pf.pf_user,user)||getuid()==0) {
			if (sp->s_rel == 0) {
				if (++cnt) {
					if (fflush(out) == EOF)
						xmsg(outname, NOGETTEXT("rdpfile"));
					if (fsync(fileno(out)) < 0)
						xmsg(outname, NOGETTEXT("rdpfile"));
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
						if (fsync(fileno(out)) < 0)
							xmsg(outname, NOGETTEXT("rdpfile"));
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
	if (fsync(fileno(out)) < 0)
		xmsg(outname, NOGETTEXT("rdpfile"));
	if (fclose(out) == EOF)
		xmsg(outname, NOGETTEXT("rdpfile"));
	copy(auxf(pkt->p_file,'p'),Pfilename);
	fclose(in);
	if (!goodpf.pf_user[0])
		fatal(gettext("login name or SID specified not in p-file (de2)"));
	return(&goodpf);
}


static FILE *
dodiff(newf,oldf,difflim)
char *newf, *oldf;
int difflim;
{
	register int i;
	int pfd[2];
	FILE *iop;
	char num[10];

	xpipe(pfd);
	if ((i = fork()) < 0) {
		close(pfd[0]);
		close(pfd[1]);
		fatal(gettext("cannot fork, try again (de11)"));
	}
	else if (i == 0) {
		close(pfd[0]);
		close(1);
		dup(pfd[1]);
		close(pfd[1]);
		for (i = 5; i < getdtablesize(); i++)
			close(i);
		sprintf(num, NOGETTEXT("%d"), difflim);
 		if (HADD) {
 		   execl(Diffpgm,Diffpgm,oldf,newf,0);
 		   execl(Diffpgm2,Diffpgm2,oldf,newf,0);
 		} else {
 		   execl(BDiffpgm,BDiffpgm,oldf,newf,num,"-s",0);
 		   execl(BDiffpgm2,BDiffpgm,oldf,newf,num,"-s",0);
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

static void
insert(pkt, linenum, n, ser)
struct	packet	*pkt;
int	linenum, n, ser;
{
 	char	str[BUFSIZ];
 	int 	first;

	after(pkt, linenum);
	sprintf(str, NOGETTEXT("%c%c %d\n"), CTLCHAR, INS, ser);
	putline(pkt, str);
	for (; n; n--) {
 		first = 1;
 		for (;;) {
			if (rddiff(str, BUFSIZ) == NULL) {
				fatal(gettext("Cannot read the diffs file (de19)"));
			}
			if (first) {
				first = 0;
				putline(pkt, str+2);
			}
			else {
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

static void
delete(pkt, linenum, n, ser)
struct	packet	*pkt;
int	linenum, n, ser;
{
	char str[BUFSIZ];

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

void
enter(pkt,ch,n,sidp)
struct packet *pkt;
char ch;
int n;
struct sid *sidp;
{
	char str[32];
	register struct apply *ap;

	sid_ba(sidp,str);
	ap = &pkt->p_apply[n];
	if (pkt->p_cutoff > pkt->p_idel[n].i_datetime)
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

/*ARGSUSED*/
void
escdodelt(pkt)			/* dummy routine for dodelt() */
	struct packet *pkt;
{
}

/*ARGSUSED*/
void
fredck(pkt)			/*dummy routine for dodelt()*/
	struct packet *pkt;
{
}

void
clean_up()
{
	uname(&un);
	uuname = un.nodename;
	if (mylock(auxf(gpkt.p_file,'z'), getpid(),uuname)) {
		if (gpkt.p_iop) {
			fclose(gpkt.p_iop);
			gpkt.p_iop = NULL;
		}
		if (Xiop) {
			fclose(Xiop);
			Xiop = NULL;
			unlink(auxf(gpkt.p_file,'x'));
		}
		if(Gin) {
			fclose(Gin);
			Gin = NULL;
		}
		unlink(auxf(gpkt.p_file,'d'));
		unlink(auxf(gpkt.p_file,'q'));
		xrm();
		ffreeall();
		uname(&un);
		uuname = un.nodename;
		unlockit(auxf(gpkt.p_file,'z'), getpid(),uuname);
	}
}

/*ARGSUSED*/
static void
fgetchk(file, pkt)
char	*file;
struct	packet	*pkt;
{
	FILE	*inptr;
	char	line[BUFSIZ];
	int	nline, idx = 0, search_on = 0;
	char	lastchar;

	inptr = xfopen(file, O_RDONLY|O_BINARY);
	/*
	 * This gives the illusion that a zero-length file ends
	 * in a newline so that it won't be mistaken for a 
	 * binary file.
	 */
	lastchar = '\n';
	(void)memset(line, '\377', BUFSIZ);
	nline = 0;
	while (fgets(line, BUFSIZ, inptr) != NULL) {
	   if (line[0] == CTLCHAR) {
	      nline++;
	      goto err;
	   }
	   search_on = 0;
	   for (idx = BUFSIZ-1; idx >= 0; idx--) {
	      if (search_on == 1) {
		 if (line[idx] == '\0') {
	err:
		    fclose(inptr);
		    sprintf(SccsError, 
		      gettext("file '%s' contains illegal data on line %d (de14)"),
		      file, nline);
		    fatal(SccsError);
		 }
	      } else {
		 if (line[idx] == '\0') {
		    search_on = 1;
		    lastchar = line[idx-1];
		    if (lastchar == '\n') {
		       nline++;
		    }
		 }
	      }
	   }   
	   (void)memset(line, '\377', BUFSIZ);
	}
	fclose(inptr);
	if (lastchar != '\n'){
	   sprintf(SccsError,
	     gettext("No newline at end of file '%s' (de18)"),
	     file);
	   fatal(SccsError);
	}

}
 
/* SVR4.0 does not support getdtablesize().				  */
/* Code should be rewritten using getrlimit() when R_NFILES is available. */

#ifndef	HAVE_GETDTABLESIZE
int
getdtablesize()
{
	return (15);
}
#endif