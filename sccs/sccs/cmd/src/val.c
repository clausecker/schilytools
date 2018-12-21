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
 * Copyright 2006-2018 J. Schilling
 *
 * @(#)val.c	1.59 18/12/17 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)val.c 1.59 18/12/17 J. Schilling"
#endif
/*
 * @(#)val.c 1.22 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)val.c"
#pragma ident	"@(#)sccs:cmd/val.c"
#endif
/************************************************************************/
/*									*/
/*  val -								*/
/*  val [-mname] [-rSID] [-s] [-ytype] file ...				*/
/*									*/
/************************************************************************/

#include	<defines.h>
#include	<version.h>
#include	<had.h>
#include	<i18n.h>
#include	<ccstypes.h>
#include	<schily/sysexits.h>

#define	FILARG_ERR	0200	/* no file name given */
#define	UNKDUP_ERR	0100	/* unknown or duplicate keyletter */
#define	CORRUPT_ERR	040	/* corrupt file error code */
#define	FILENAM_ERR	020	/* file name error code */
#define	INVALSID_ERR	010	/* invalid or ambiguous SID error  */
#define	NONEXSID_ERR	04	/* non-existent SID error code */
#define	TYPE_ERR	02	/* type arg value error code */
#define	NAME_ERR	01	/* name arg value error code */
#define	TRUE		1
#define	FALSE		0
#define	BLANK(l)	while (!(*l == '\0' || *l == ' ' || *l == '\t')) l++;

static int	silent;		/* be silent, report only in exit code */
static int	debug;		/* print debug messages */
static int	ret_code;	/* prime return code from 'main' program */
static int	inline_err = 0;	/* input line error code (from 'process') */
static int	infile_err = 0;	/* file error code (from 'validate') */
static int	inpstd;		/* TRUE = args from standard input */

static struct packet gpkt;
static Nparms	N;			/* Keep -N parameters		*/

static struct deltab dt;
static struct deltab odt;

static char	path[FILESIZE];	/* storage for file name value */
static char	sid[50];	/* storage for sid  (-r) value */
static char	type[50];	/* storage for type (-y) value */
static char	name[50];	/* storage for name (-m) value */
static char	*Argv[BUFSIZ];
static char	line[BUFSIZ];
static char	save_line[BUFSIZ];

static struct delent {		/* structure for delta table entry */
	char type;
	char *osid;
	char *datetime;
	char *pgmr;
	char *serial;
	char *pred;
} del;

	int	main __PR((int argc, char **argv));
static void	process __PR((char *p_line, int argc, char **argv));
static void	do_validate __PR((char *c_path));
static void	validate __PR((char *c_path, char *c_sid, char *c_type, char *c_name));
static void	getdel __PR((struct delent *delp, char *lp, struct packet *pkt));
static void	read_to __PR((int ch, struct packet *pkt));
static void	report __PR((int code, char *inp_line, char *file));
static int	invalid __PR((char *i_sid));
static char *	get_line __PR((struct packet *pkt));	/* function returning ptr to line read */
static void	s_init __PR((struct packet *pkt, char *file));
static int	read_mod __PR((struct packet *pkt));
static void	add_q __PR((struct packet *pkt, int ser, int keep, int iord, int user));
static void	rem_q __PR((struct packet *pkt, int ser));
static void	set_keep __PR((struct packet *pkt));
static int	chk_ix __PR((struct queue *new, struct queue *head));
static int	do_delt __PR((struct packet *pkt, int goods, char *d_sid));
static int	getstats __PR((struct packet *pkt));
static char *	getastat __PR((char *p, int  *ip));
static void	get_setup	__PR((char *file));
static void	get_close	__PR((void));
static int	get_hashtest	__PR((int ser));


/* This is the main program that determines whether the command line
 * comes from the standard input or read off the original command
 * line.  See VAL(I) for more information.
*/

int
main(argc, argv)
int argc;
char	*argv[];
{
	FILE	*iop;
	char *lp;

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

	/*
	Set flags for 'fatal' to issue message, call clean-up
	routine and terminate processing.
	*/
	Fflags = FTLMSG | FTLCLN | FTLEXIT;
#ifdef	SCCS_FATALHELP
	Fflags |= FTLFUNC;
	Ffunc = sccsfatalhelp;
#endif

	ret_code = 0;
	if (argc == 2 && argv[1][0] == '-' && !(argv[1][1])) {
		inpstd = TRUE;
		iop = stdin;		/* read from standard input */
		while (fgets(line, BUFSIZ, iop) != NULL) {
		    if (line[0] != '\n') {
			repl(line, '\n', '\0');
			strlcpy(save_line, line, sizeof (save_line));
			argv = Argv;
			*argv++ = "val";
			lp = save_line;
			argc = 1;
			while (*lp != '\0') {
			    while ((*lp == ' ') || (*lp == '\t'))
				*lp++ = '\0';
			    *argv++ = lp++;
			    argc++;
			    while ((*lp != ' ') && (*lp != '\t') && (*lp != '\0'))
				lp++;
			}
			*argv = NULL;
			argv = Argv;
			process(line, argc, argv);
			ret_code |= inline_err;
		    }
		}
	} else {
		inpstd = FALSE;
		line[0] = '\0';
		process(line, argc, argv);
		ret_code = inline_err;
	}

	return (ret_code);
}


/* This function processes the line sent by the main routine.  It
 * determines which keyletter values are present on the command
 * line and assigns the values to the correct storage place.  It
 * then calls validate for each file name on the command line
 * It will return to main if the input line contains an error,
 * otherwise it returns any error code found by validate.
*/

static void
process(p_line, argc, argv)
char	*p_line;
int	argc;
char	*argv[];
{
	register int	j;
	register int	line_sw;
	register char   *p;
	register int	i;

	int	num_files;

	char	**filelist = NULL;
	char	*savelinep;
	int 	c;

	int current_optind;
	int no_arg;

	silent = FALSE;
	path[0] = sid[0] = type[0] = name[0] = 0;
	num_files = inline_err = 0;

	/*
	make copy of 'line' for use later
	*/
	savelinep = p_line;
	/*
	clear out had flags for each 'line' processed
	*/
	for (j = 0; j < HAD_SIZE; j++)
		had[j] = 0;
	/*
	execute loop until all characters in 'line' are checked.
	*/

	current_optind = 1;
	optind = 1;
	opterr = 0;
	no_arg = 0;
	j = 1;
	/*CONSTCOND*/
	while (1) {
			if (current_optind < optind) {
				current_optind = optind;
				argv[j] = 0;
				if (optind > j+1) {
					if ((argv[j+1][0] != '-') &&
					    (no_arg == 0)) {
						argv[j+1] = NULL;
					} else {
						optind = j+1;
						current_optind = optind;
			 		}
				}
			}
			no_arg = 0;
			j = current_optind;
			c = getopt(argc, argv, "()-r:sm:y:hvTN:V(version)");

				/* this takes care of options given after
				** file names.
				*/
			if (c == EOF) {
			    if (optind < argc) {
				/* if it's due to -- then break; */
				if (argv[j][0] == '-' &&
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
				case 'h':
					break;
				case 's':
					silent = TRUE;
					break;
				case 'r':
					strlcpy(sid, p, sizeof (sid));
					break;
				case 'y':
					strlcpy(type, p, sizeof (type));
					break;
				case 'm':
					strlcpy(name, p, sizeof (name));
					break;
				case 'v':
					break;

				case 'T':
					debug = TRUE;
					silent = FALSE;
					break;

				case 'N':	/* Bulk names */
					initN(&N);
					if (optarg == argv[j+1]) {
					   no_arg = 1;
					   break;
					}
					N.n_parm = p;
					break;

				case 'V':		/* version */
					printf(gettext(
					    "val %s-SCCS version %s %s (%s-%s-%s)\n"),
						PROVIDER,
						VERSION,
						VDATE,
						HOST_CPU, HOST_VENDOR, HOST_OS);
					exit(EX_OK);

				default:
					Fflags &= ~FTLEXIT;
					fatal(gettext("Usage: val [ -h ] [ -s ] [ -m name ] [ -r SID ] [ -T ] [ -v ]\n\t[ -y type ] [ -N[bulk-spec]] s.filename..."));
					if (debug) {
						printf(gettext("Uknown option '%c'.\n"),
							optopt?optopt:c);
					}
					inline_err |= UNKDUP_ERR;
					if (inpstd)
					   report(inline_err, savelinep, "");
					else
					   report(inline_err, "", "");
					return;
			}
			/*
			 * Make sure that we only collect option letters from
			 * the range 'a'..'z' and 'A'..'Z'.
			 *
			 * use 'had' array and determine if the keyletter
			 * was given twice.
			 */
			if (ALPHA(c) &&
			    (had[LOWER(c)? c-'a' : NLOWER+c-'A']++)) {
				if (debug)
					printf(gettext("Duplicate option '%c'.\n"), c);
				inline_err |= UNKDUP_ERR;
			}
	}

	for (j = 1; j < argc; j++) {
		if (argv[j]) {
			if (filelist == NULL)
				filelist = &argv[j];
			num_files++;
		}
	}
	/*
	check if any files were named as arguments
	*/
	if (num_files == 0) {
		if (debug)
			printf(gettext("Missing file argument.\n"));
		inline_err |= FILARG_ERR;
	}
	/*
	check for error in command line.
	*/
	if (inline_err) {
	    if (!silent) {
		if (inpstd)
			report(inline_err, savelinep, "");
		else
			report(inline_err, "", "");
	    }
	    return;		/* return to 'main' routine */
	}
	line_sw = 1;		/* print command line flag */

	xsethome(NULL);
	if (HADUCN) {					/* Parse -N args  */
		parseN(&N);

		if (N.n_sdot && (sethomestat & SETHOME_OFFTREE))
			fatal(gettext("-Ns. not supported in off-tree project mode"));
	}

	/*
	loop through 'validate' for each file on command line.
	*/
	for (i = 0, j = 0; j < num_files; j++) {
		while (filelist[i+j] == NULL) {
			i++;
			if ((i+j+1) >= argc)
				break;
		}
		if (filelist[i+j] == NULL)
			break;

		/*
		read a file from 'filelist' and place into 'path'.
		*/
		if (size(filelist[i+j]) > FILESIZE) {
			extern char *Ffile;
			Ffile = filelist[i+j];
			fatal(gettext("too long (co7)"));
		}
		strlcpy(path, filelist[i+j], sizeof (path));
		if (!inpstd) {
			do_file(path, do_validate, 1, N.n_sdot);
			continue;
		}
		validate(path, sid, type, name);
		inline_err |= infile_err;
		/*
		check for error from 'validate' and call 'report'
		depending on 'silent' flag.
		*/
		if (infile_err && !silent) {
			if (line_sw && inpstd) {
				report(infile_err, savelinep, path);
				line_sw = 0;
			} else
				report(infile_err, "", path);
		}
	}
	/* return to 'main' routine */
}

static void
do_validate(c_path)
	char	*c_path;
{
	if (HADUCN) {
		char	*ofile = c_path;

		c_path = bulkprepare(&N, c_path);
		if (c_path == NULL) {
			if (N.n_ifile)
				ofile = N.n_ifile;
			fatal(gettext("directory specified as s-file (cm14)"));
		}
	}

	validate(c_path, sid, type, name);
	inline_err |= infile_err;

	/*
	 * check for error from 'validate' and call 'report'
	 * depending on 'silent' flag.
	 */
	if (infile_err && !silent) {
		report(infile_err, "", c_path);
	}
}

/* This function actually does the validation on the named file.
 * It determines whether the file is an SCCS-file or if the file
 * exists.  It also determines if the values given for type, SID,
 * and name match those in the named file.  An error code is returned
 * if any mismatch occurs.  See VAL(I) for more information.
*/

static void
validate(c_path, c_sid, c_type, c_name)
char	*c_path;
char	*c_sid;
char	*c_type;
char	*c_name;
{
	register char	*l;
	int	goods, goodt, goodn, hadmflag;

	infile_err = goods = goodt = goodn = hadmflag = 0;
	dt.d_dtime.dt_sec = odt.d_dtime.dt_sec = 0;
	dt.d_dtime.dt_nsec = odt.d_dtime.dt_nsec = 0;
	dt.d_dtime.dt_zone = odt.d_dtime.dt_zone = DT_NO_ZONE;

	s_init(&gpkt, c_path);

	if (!sccsfile(c_path) || (gpkt.p_iop = fopen(c_path, "rb")) == NULL) {
		if (debug) {
			if (!sccsfile(c_path)) {
				printf(gettext("%s%s: not a sccs file\n"),
					"    ", c_path);
			} else {
				printf(gettext("%s%s cannot open\n"),
					"    ", c_path);
			}
		}
		infile_err |= FILENAM_ERR;
	} else {
		l = get_line(&gpkt);		/* read first line in file */
		/*
		check that it is header line.
		*/
		if (l == NULL || (l = checkmagic(&gpkt, l)) == NULL) {
			if (debug)
				printf(
				gettext("%s%s: corrupted first line in file\n"),
					"    ", c_path);
			infile_err |= CORRUPT_ERR;
		}
		else {
			if (HADV) {
				if (gpkt.p_flags & PF_V6)
					printf("SCCS V6 %s\n", c_path);
				else
					printf("SCCS V4 %s\n", c_path);
				sclose(&gpkt);
				return;
			}

			/*
			 * Read delta table for get_hashtest()
			 */
			if (gpkt.p_flags & PF_V6 && HADH)
				get_setup(c_path);

			/*
			 * get old file checksum count
			 */
			satoi(l, &gpkt.p_ihash);
			gpkt.p_chash = 0;
			gpkt.p_uchash = 0;
			if (HADR)
				/*
				check for invalid or ambiguous SID.
				*/
				if (invalid(c_sid)) {
					infile_err |= INVALSID_ERR;
				}
			/*
			read delta table checking for errors and/or
			SID.
			*/
			if (do_delt(&gpkt, goods, c_sid)) {
				sclose(&gpkt);
				get_close();		/* for SID checksums */
				if (debug)
					printf(gettext(
					"%s%s: invalid delta table at line %d\n"),
						"    ", c_path, gpkt.p_slnno);
				infile_err |= CORRUPT_ERR;
				return;
			}

			read_to(EUSERNAM, &gpkt);

			if (HADY || HADM) {
				/*
				read flag section of delta table.
				*/
				while (((l = get_line(&gpkt)) != NULL) &&
					*l++ == CTLCHAR &&
					*l++ == FLAG) {
					NONBLANK(l);
					repl(l, '\n', '\0');
					if (*l == TYPEFLAG) {
						l += 2;
						if (equal(c_type, l))
							goodt++;
					}
					else if (*l == MODFLAG) {
						hadmflag++;
						l += 2;
						if (equal(c_name, l))
							goodn++;
					}
				}

				/*
				 * If there are SCCS v6 flags or SCCS v6 global
				 * meta data, we need to skip this data here.
				 */
				if (gpkt.p_line != NULL &&
				    gpkt.p_line[0] == CTLCHAR &&
				    gpkt.p_line[1] != BUSERTXT)
					read_to(BUSERTXT, &gpkt);

				/*
				 * If we did not find the mandatory begin of
				 * the user comment area, the file is corrupt.
				 */
				if (gpkt.p_line != NULL &&
				    gpkt.p_line[0] == CTLCHAR &&
				    gpkt.p_line[1] != BUSERTXT) {
					sclose(&gpkt);
					get_close();	/* for SID checksums */
					if (debug)
						printf(gettext(
						"%s%s: flag section error at line %d\n"),
						"    ", c_path, gpkt.p_slnno);
					infile_err |= CORRUPT_ERR;
					return;
				}
				/*
				check if 'y' flag matched '-y' arg value.
				*/
				if (!goodt && HADY) {
					if (debug)
						printf(gettext(
						"%s%s: mismatch between %cY%c and '%s'\n"),
							"    ", c_path,
							'%', '%', c_type);
					infile_err |= TYPE_ERR;
				}
				/*
				check if 'm' flag matched '-m' arg value.
				*/
				if (HADM && !hadmflag) {
					if (!equal(auxf(sname(c_path), 'g'), c_name)) {
						if (debug)
							printf(gettext(
							"%s%s: no 'm' flag\n"),
							"    ", c_path);
						infile_err |= NAME_ERR;
					}
				}
				else if (HADM && hadmflag && !goodn) {
						if (debug)
							printf(gettext(
							"%s%s: mismatch between %cM%c and '%s'\n"),
							"    ", c_path,
							'%', '%', c_name);
						infile_err |= NAME_ERR;
				}
			}
			else read_to(BUSERTXT, &gpkt);
			read_to(EUSERTXT, &gpkt);
			gpkt.p_chkeof = 1;
			/*
			read remainder of file so 'read_mod'
			can check for corruptness.
			*/
			while (read_mod(&gpkt))
				;
		}
	sclose(&gpkt);		/* close file pointer */
	sfree(&gpkt);		/* free line pointer */
	get_close();		/* for SID checksums */
	}
	/* return to 'process' function */
}


/* This function reads the 'delta' line from the named file and stores
 * the information into the structure 'del'.
*/

static void
getdel(delp, lp, pkt)
register struct delent *delp;
register char *lp;
struct packet *pkt;
{
	char	*p;
	int	dflags = 0;
	int	missfld = 0;

	if (debug) {
		char	str[SID_STRSIZE];
		char	ostr[SID_STRSIZE];

		del_ab(&lp[-2], &dt, pkt);	/* We are called with &lp[2] */

		if (dt.d_dtime.dt_sec != 0 && odt.d_dtime.dt_sec != 0 &&
		    (dt.d_dtime.dt_sec > odt.d_dtime.dt_sec) ||
		    ((dt.d_dtime.dt_sec == odt.d_dtime.dt_sec) &&
		    (dt.d_dtime.dt_nsec > odt.d_dtime.dt_nsec))) {
			sid_ba(&dt.d_sid, str);
			sid_ba(&odt.d_sid, ostr);
			printf(gettext(
"%s%s: warning: date for sid %s is later than the date for sid %s in line %d\n"),
				"    ", pkt->p_file,
				str, ostr, pkt->p_slnno);
		}
		odt = dt;			/* Remember last date */
	}

	NONBLANK(lp);
	delp->type = *lp++;			/* Type	'D'		*/
	NONBLANK(lp);
	delp->osid = lp;			/* SID	"1.2"		*/
	BLANK(lp);
	if (*lp)
		*lp++ = '\0';
	else
		missfld++;
	NONBLANK(lp);
	delp->datetime = lp;			/* Date	"06/12/20 23:46:27" */
	BLANK(lp);				/* Skip past date	*/
	NONBLANK(lp);				/* Find time		*/
	BLANK(lp);				/* Skip past time	*/
	if (*lp)
		*lp++ = '\0';
	else
		missfld++;
	p = lp;
	NONBLANK(lp);				/* Find programmer	*/
	/*
	 * Old sccs implementations did frequently write something like
	 * "^Ad D 4.42 82/10/19 10:30:32  64 63"
	 * So we check for extra blanks and later verify whether there
	 * are too few fields.
	 */
	if (lp != p && *p == ' ')
		dflags |= 1;
	delp->pgmr = lp;			/* Programmer	"bill"	*/
	BLANK(lp);				/* Skip past programmer	*/
	if (*lp)
		*lp++ = '\0';
	else
		missfld++;
	NONBLANK(lp);				/* Find serial		*/
	delp->serial = lp;			/* Serial	"42"	*/
	BLANK(lp);				/* Skip past serial	*/
	if (*lp)
		*lp++ = '\0';
	else
		missfld++;
	NONBLANK(lp);				/* Find pred		*/
	delp->pred = lp;			/* Pred serial	"41"	*/
	if (missfld == 0 && (*lp == '\0' || *lp == '\n'))
		missfld++;
	repl(lp, '\n', '\0');
	if (dflags && (*lp == '\0' || *lp == '\n')) {
		if (debug)
			printf(gettext("%s%s: username missing in line %d\n"),
			"    ", pkt->p_file, pkt->p_slnno);
		infile_err |= CORRUPT_ERR;
	}
	if (missfld > 1 || (missfld && dflags == 0)) {
		if (debug)
			printf(gettext("%s%s: missing field in delta in line %d\n"),
			"    ", pkt->p_file, pkt->p_slnno);
		infile_err |= CORRUPT_ERR;
	}
	if ((pkt->p_flags & PF_V6) == 0) {
		int	dterr = 0;

		p = strchr(delp->datetime, '/');
		if ((p - delp->datetime) > 2) {
			if (dt.d_dtime.dt_sec < Y1969 ||
			    dt.d_dtime.dt_sec >= Y2038) {
				/*
				 * In this case, 4-digit year numbers are OK.
				 */
				if ((p - delp->datetime) != 4)
					dterr++;
			} else {
				dterr++;
			}
		}
		/*
		 * Nanoseconds not allowed in SCCSv4 mode.
		 */
		if (strchr(delp->datetime, '.'))
			dterr++;
		/*
		 * With 2-digit year, the length is 17.
		 */
		if (strlen(delp->datetime) > 19)
			dterr++;
		if (dterr) {
			if (debug)
				printf(gettext("%s%s: invalid v4 datetime field '%s' in line %d\n"),
				"    ", pkt->p_file, delp->datetime, pkt->p_slnno);
			infile_err |= CORRUPT_ERR;
		}
	}
}


/* This function does a read through the named file until it finds
 * the character sent over as an argument.
*/

static void
read_to(ch, pkt)
register char ch;
register struct packet *pkt;
{
	register char *n;
	while (((n = get_line(pkt)) != NULL) &&
			!(*n++ == CTLCHAR && *n == ch))
		;
}


/* This function will report the error that occured on the command
 * line.  It will print one diagnostic message for each error that
 * was found in the named file.
*/

static void
report(code, inp_line, file)
register int	code;
register char	*inp_line;
register char	*file;
{
	char	percent;
	percent = '%';		/* '%' for -m and/or -y messages */
/* xpg4 behaviour
	if (*inp_line)
		printf("%s:\n", inp_line);
	if (code & NAME_ERR)
	   printf(gettext(" %s: %cM%c -m mismatch\n"), file, percent, percent);
	if (code & TYPE_ERR)
	   printf(gettext(" %s: %cY%c -y mismatch\n"), file, percent, percent);
	if (code & NONEXSID_ERR)
	   printf(gettext(" %s: SID nonexistent\n"), file);
	if (code & INVALSID_ERR)
	   printf(gettext(" %s: SID invalid or ambiguous\n"), file);
	if (code & FILENAM_ERR)
	   printf(gettext(" %s: can't open file or file not SCCS\n"), file);
	if (code & CORRUPT_ERR)
	   printf(gettext(" %s: corrupted SCCS file\n"), file);
	if (code & UNKDUP_ERR)
	   printf(gettext(" %s: Unknown or duplicate keyletter argument\n"), file);
	if (code & FILARG_ERR)
		printf(gettext(" %s: missing file argument\n"), file);
*/
	if (*inp_line)
		printf("%s\n\n", inp_line);
	if (code & NAME_ERR)
	   printf(gettext("    %s: %cM%c -m mismatch\n"), file, percent, percent);
	if (code & TYPE_ERR)
	   printf(gettext("    %s: %cY%c -y mismatch\n"), file, percent, percent);
	if (code & NONEXSID_ERR)
	   printf(gettext("    %s: SID nonexistent\n"), file);
	if (code & INVALSID_ERR)
	   printf(gettext("    %s: SID invalid or ambiguous\n"), file);
	if (code & FILENAM_ERR)
	   printf(gettext("    %s: can't open file or file not SCCS\n"), file);
	if (code & CORRUPT_ERR)
	   printf(gettext("    %s: corrupted SCCS file\n"), file);
	if (code & UNKDUP_ERR)
	   printf(gettext("    %s: Unknown or duplicate keyletter argument\n"), file);
	if (code & FILARG_ERR)
		printf(gettext("    %s: missing file argument\n"), file);
}


/*
 * This function takes as its argument the SID inputed and determines
 * whether or not it is valid (e. g. not ambiguous or illegal).
 */

static int
invalid(i_sid)
register char	*i_sid;
{
	register int count;
	register int digits;
	count = digits = 0;
	if (*i_sid == '0' || *i_sid == '.')
		return (1);
	i_sid++;
	digits++;
	while (*i_sid != '\0') {
		if (*i_sid++ == '.') {
			digits = 0;
			count++;
			if (*i_sid == '0' || *i_sid == '.')
				return (1);
		}
		digits++;
		if (digits > 5)
			return (1);
	}
	if (*(--i_sid) == '.')
		return (1);
	if (count == 1 || count == 3)
		return (0);
	return (1);
}


/*
	Routine to read a line into the packet.  The main reason for
	it is to make sure that pkt->p_wrttn gets turned off,
	and to increment pkt->p_slnno.
*/

static char	*
get_line(pkt)
register struct packet *pkt;
{
#ifdef	NO_GETDELIM
	char	buf[DEF_LINE_SIZE];
	register size_t nread = 0;
#endif
	int	eof = 0;
	register size_t used = 0;
	register signed char *p;
	register unsigned char *u_p;

#ifndef	NO_GETDELIM
	/*
	 * getdelim() allows to read lines that have embedded nul bytes
	 * and we don't need to call strlen().
	 */
	errno = 0;
	used = getdelim(&pkt->p_line, &pkt->p_line_size, '\n', pkt->p_iop);
	if (used == -1) {
		if (errno == ENOMEM)
			fatal(gettext("OUT OF SPACE (ut9)"));
		if (ferror(pkt->p_iop)) {
			xmsg(pkt->p_file, NOGETTEXT("getline"));
		} else if (feof(pkt->p_iop)) {
			used = 0;
			eof = 1;
		}
	}
#else
	/* read until EOF or newline encountered */
	do {
		if (!(eof = (fgets(buf, sizeof (buf), pkt->p_iop) == NULL))) {
			nread = strlen(buf);

			if ((used + nread) >=  pkt->p_line_size) {
				pkt->p_line_size += sizeof (buf);
				pkt->p_line = (char *) realloc(pkt->p_line, pkt->p_line_size);
				if (pkt->p_line == NULL) {
					efatal(gettext("OUT OF SPACE (ut9)"));
				}
			}

			strcpy(pkt->p_line + used, buf);
			used += nread;
		}
	} while (!eof && (pkt->p_line[used-1] != '\n'));
#endif
	pkt->p_linebase = pkt->p_line;
	pkt->p_line_length = used;

	/* check end of file condition */
	if (eof && (used == 0)) {
		if (!pkt->p_chkeof) {
			if (debug)
				printf(gettext(
					"%s%s: incomplete delta table\n"),
							"    ", pkt->p_file);
			infile_err |= CORRUPT_ERR;
		}
		if (pkt->do_chksum && (pkt->p_chash ^ pkt->p_ihash)&0xFFFF) {
		   if (pkt->do_chksum && (pkt->p_uchash ^ pkt->p_ihash)&0xFFFF) {
			if (debug)
				printf(gettext(
				"%s%s: invalid checksum %d, expected %d\n"),
						"    ", pkt->p_file,
						pkt->p_ihash,
						pkt->p_chash & 0xFFFF);
			infile_err |= CORRUPT_ERR;
		   }
		}
		return (NULL);
	}

	/* increment line number */
	if (pkt->p_line[used-1] == '\n') {
		pkt->p_slnno++;
	}

	/* update check sum */
	for (p = (signed char *)pkt->p_line, u_p = (unsigned char *)pkt->p_line; *p; ) {
		pkt->p_chash += *p++;
		pkt->p_uchash += *u_p++;
	}

	return (pkt->p_line);
}


/*
	Does initialization for sccs files and packet.
*/

static void
s_init(pkt, file)
register struct packet *pkt;
register char *file;
{

	zero((char *) pkt, sizeof (*pkt));
	copy(file, pkt->p_file);
	pkt->p_wrttn = 1;
	pkt->do_chksum = 1;	/* turn on checksum check for getline */
}

static int
read_mod(pkt)
register struct packet *pkt;
{
	register char *p;
	int ser;
	int iord;
	register struct apply *ap;

	while (get_line(pkt) != NULL) {
		p = pkt->p_line;
		if (*p++ != CTLCHAR)
			continue;
		else {
			if (!((iord = *p++) == INS || iord == DEL || iord == END)) {
				if (iord == CTLCHAR && pkt->p_flags & PF_V6)
					continue;
				if (iord == NONL && pkt->p_flags & PF_V6)
					continue;
				if (debug)
					printf(gettext(
					"%s%s: invalid control in weave data near line %d\n"),
						"    ", pkt->p_file,
						pkt->p_slnno);
				infile_err |= CORRUPT_ERR;
				return (0);
			}
			NONBLANK(p);
			satoi(p, &ser);
			if (iord == END)
				rem_q(pkt, ser);
			else if (pkt->p_apply != 0) {
				ap = &pkt->p_apply[ser];
				if (ap->a_code == APPLY)
					add_q(pkt, ser, iord == INS ? YES : NO,
					    iord, ap->a_reason & USER);
				else
					add_q(pkt, ser, iord == INS ? NO : 0,
					    iord, ap->a_reason & USER);
			} else
				add_q(pkt, ser, iord == INS ? NO : 0, iord, 0);
		}
	}
	if (pkt->p_q) {
		if (debug)
			printf(gettext(
			"%s%s: incomplete weave data near line %d\n"),
				"    ", pkt->p_file,
				pkt->p_slnno);
		infile_err |= CORRUPT_ERR;
	}
	return (0);
}

static void
add_q(pkt, ser, keep, iord, user)
struct packet *pkt;
int ser;
int keep;
int iord;
int user;
{
	register struct queue *cur, *prev, *q;

	for (cur = (struct queue *) (&pkt->p_q); (cur = (prev = cur)->q_next) != NULL; )
		if (cur->q_sernum <= ser)
			break;
	if (cur && cur != (struct queue *)&pkt->p_q && cur->q_sernum == ser) {
		if (debug)
			printf(gettext(
			"%s%s: duplicate delta block in weave data with serial %d near line %d\n"),
				"    ", pkt->p_file,
				cur->q_sernum, pkt->p_slnno);
		infile_err |= CORRUPT_ERR;
	}
	prev->q_next = q = (struct queue *) fmalloc(sizeof (*q));
	q->q_next = cur;
	q->q_sernum = ser;
	q->q_keep = keep;
	q->q_iord = iord;
	q->q_user = user;
	if (pkt->p_ixuser && (q->q_ixmsg = chk_ix(q, pkt->p_q)))
		++(pkt->p_ixmsg);
	else
		q->q_ixmsg = 0;

	set_keep(pkt);
}

static void
rem_q(pkt, ser)
register struct packet *pkt;
int ser;
{
	register struct queue *cur, *prev;

	for (cur = (struct queue *) (&pkt->p_q); (cur = (prev = cur)->q_next) != NULL; )
		if (cur->q_sernum == ser)
			break;
	if (cur && cur != (struct queue *)&pkt->p_q) {
		if (cur->q_ixmsg)
			--(pkt->p_ixmsg);
		prev->q_next = cur->q_next;
		ffree((char *) cur);
		set_keep(pkt);
	}
	else {
		if (debug)
			printf(gettext(
			"%s%s: incomplete delta block in weave data near line %d\n"),
				"    ", pkt->p_file,
				pkt->p_slnno);
		infile_err |= CORRUPT_ERR;
	}
}

static void
set_keep(pkt)
register struct packet *pkt;
{
	register struct queue *q;
	register struct sid *sp;

	for (q = pkt->p_q; q; q = q->q_next)
		if (q->q_keep != '\0') {
			if ((pkt->p_keep = q->q_keep) == YES) {
				sp = &pkt->p_idel[q->q_sernum].i_sid;
				pkt->p_inssid.s_rel = sp->s_rel;
				pkt->p_inssid.s_lev = sp->s_lev;
				pkt->p_inssid.s_br = sp->s_br;
				pkt->p_inssid.s_seq = sp->s_seq;
			}
			return;
		}
	pkt->p_keep = NO;
}


#define	apply(qp)	((qp->q_iord == INS && qp->q_keep == YES) || \
			 (qp->q_iord == DEL && qp->q_keep == NO))
static int
chk_ix(new, head)
register struct queue *new;
struct queue *head;
{
	register int retval;
	register struct queue *cur;
	int firstins, lastdel;

	if (!apply(new))
		return (0);
	for (cur = head; cur; cur = cur->q_next)
		if (cur->q_user)
			break;
	if (!cur)
		return (0);
	retval = 0;
	firstins = 0;
	lastdel = 0;
	for (cur = head; cur; cur = cur->q_next) {
		if (apply(cur)) {
			if (cur->q_iord == DEL)
				lastdel = cur->q_sernum;
			else if (firstins == 0)
				firstins = cur->q_sernum;
		}
		else if (cur->q_iord == INS)
			retval++;
	}
	if (retval == 0) {
		if (lastdel && (new->q_sernum > lastdel))
			retval++;
		if (firstins && (new->q_sernum < firstins))
			retval++;
	}
	return (retval);
}


/*
 * This function reads the delta table entries and checks for the format
 * as specifed in sccsfile(V).  If the format is incorrect, a corrupt
 * error will be issued by 'val'.  This function also checks
 * if the sid requested is in the file (depending if '-r' was specified).
 */

static int
do_delt(pkt, goods, d_sid)
register struct packet *pkt;
register int goods;
register char *d_sid;
{
	char *l;

	while (getstats(pkt)) {
		if ((((l = get_line(pkt)) != NULL) && *l++ != CTLCHAR) ||
		    *l++ != BDELTAB)
			return (1);
		getdel(&del, l, pkt);
		if (HADR && !(infile_err & INVALSID_ERR)) {
			if (equal(d_sid, del.osid) && del.type == 'D')
				goods++;
		}
		if (gpkt.p_flags & PF_V6 &&
		    HADH && del.type == 'D') {
			int	ser;

			satoi(del.serial, &ser);
			get_hashtest(ser);
		}
		while ((l = get_line(pkt)) != NULL)
			if (pkt->p_line[0] != CTLCHAR)
				break;
			else {
				switch (pkt->p_line[1]) {
				case EDELTAB:
					break;
				case COMMENTS:
				case MRNUM:
				case INCLUDE:
				case EXCLUDE:
				case IGNORE:
					continue;
				case SIDEXTENS:
					if (pkt->p_flags & PF_V6)
						continue;
				default:
					return (1);
				}
				break;
			}
		if (l == NULL || pkt->p_line[0] != CTLCHAR)
			return (1);
	}
	if (pkt->p_line[1] != BUSERNAM)
		return (1);
	if (HADR && !goods && !(infile_err & INVALSID_ERR)) {
		infile_err |= NONEXSID_ERR;
	}
	return (0);
}


/* This function reads the stats line from the sccsfile */

static int
getstats(pkt)
register struct packet *pkt;
{
	register char *p = get_line(pkt);
	register char *op;
	register int i = 0;

	if (p == NULL || *p++ != CTLCHAR || *p != STATS)
		return (0);

	/*
	 * We do not check the format of the statistics line in non-debug mode
	 * as the related values are otherwise ignored by SCCS.
	 */
	if (!debug)
		return (1);

	p++;
	NONBLANK(p);
	if (*p < '0' || *p > '9') {
		printf(gettext(
		"%s%s: illegal character in statistics in line %d\n"),
		"    ", pkt->p_file, pkt->p_slnno);
	}
	while (*p) {
		if (*p == '\n')
			break;
		op = p;
		p = getastat(p, NULL);
		if ((i == 2 && *p != '\n') || (i < 2 && *p != '/')) {
			printf(gettext(
			"%s%s: illegal character in statistics in line %d\n"),
				"    ", pkt->p_file, pkt->p_slnno);
			infile_err |= CORRUPT_ERR;
			return (1);
		} else if ((p - op) != 5) {
			if ((p - op) > 5)
				printf(gettext(
				"%s%s: number exceeds '99999' in statistics in line %d\n"),
				"    ", pkt->p_file, pkt->p_slnno);
			else
				printf(gettext(
				"%s%s: illegal number length in statistics in line %d\n"),
				"    ", pkt->p_file, pkt->p_slnno);
			infile_err |= CORRUPT_ERR;
			return (1);
		}
		p++;
		i++;
	}
	return (1);
}

static char *
getastat(p, ip)
	char	*p;
	int	*ip;
{
	int	i = 0;
	int	c;

	while ((c = *p) != '\0') {
		if (c < '0' || c > '9') {
			if (c != '/' && c != '\n')
				i = -1;
			break;
		}
		i *= 10;
		i += c - '0';
		p++;
	}
	if (ip)
		*ip = i;
	return (p);
}

LOCAL struct packet pk2;
LOCAL off_t	get_off;
LOCAL int	slnno;

LOCAL void
get_setup(file)
	char	*file;
{
	struct stats stats;

	sinit(&pk2, file, SI_OPEN);

	pk2.do_chksum = 0;
	pk2.p_stdout = stderr;
	pk2.p_reopen = 1;
	pk2.p_cutoff = MAX_TIME;

	if ((pk2.p_flags & PF_V6) == 0)
		pk2.p_flags |= PF_GMT;

	if (dodelt(&pk2, &stats, (struct sid *) 0, 0) == 0)
		fmterr(&pk2);
	flushto(&pk2, EUSERTXT, FLUSH_NOCOPY);
	get_off = ftell(pk2.p_iop);
	slnno = pk2.p_slnno;

	if (pk2.p_hash == NULL) {
		if (debug)
			printf(gettext(
			"%s%s: SID checksums missing\n"),
					"    ", pk2.p_file);
		infile_err |= CORRUPT_ERR;
	}
}

LOCAL void
get_close()
{
	sclose(&pk2);
	sfree(&pk2);
	xrm(&gpkt);
	ffreeall();
}

LOCAL int
get_hashtest(ser)
	int	ser;
{
	int	max_ser = maxser(&pk2);

	if (ser > max_ser)
		return (-1);

	if (pk2.p_hash == NULL)
		return (-1);

	fseek(pk2.p_iop, get_off, SEEK_SET);
	pk2.p_slnno = slnno;

	pk2.p_reopen = 1;
	pk2.p_chkeof = 1;
	pk2.p_gotsid = pk2.p_idel[ser].i_sid;
	pk2.p_reqsid = pk2.p_gotsid;

	zero((char *) pk2.p_apply, (max_ser+1)*sizeof (*pk2.p_apply));
	setup(&pk2, ser);

	pk2.p_ghash = 0;
	while (readmod(&pk2))
		;

	if (pk2.p_hash == NULL)
		return (0);

	if (pk2.p_hash[ser] != (pk2.p_ghash & 0xFFFF)) {
		if (debug) {
			char	str[SID_STRSIZE];

			sid_ba(&pk2.p_gotsid, str);
			printf(gettext(
			"%s%s: SID %s: invalid checksum %d, expected %d\n"),
					"    ", pk2.p_file,
					str,
					pk2.p_ghash & 0xFFFF,
					pk2.p_hash[ser]);
		}
		infile_err |= CORRUPT_ERR;
		return (0);
	}
	return (1);
}
