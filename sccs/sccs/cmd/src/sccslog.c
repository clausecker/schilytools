/* @(#)sccslog.c	1.68 20/08/10 Copyright 1997-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)sccslog.c	1.68 20/08/10 Copyright 1997-2020 J. Schilling";
#endif
/*
 *	Copyright (c) 1997-2020 J. Schilling
 */
/*@@C@@*/

#include <defines.h>
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/time.h>
#include <schily/utypes.h>
#include <schily/stat.h>
#include <schily/dirent.h>
#include <schily/maxpath.h>
#include <schily/getargs.h>
#include <schily/schily.h>
#include <schily/wait.h>
#include <version.h>
#include <i18n.h>

#ifdef	INS_BASE
#if defined(__STDC__) || defined(PROTOTYPES)
#define	PROGPATH(name)	INS_BASE "/" SCCS_BIN_PRE "bin/" #name
#else
/*
 * XXX With a K&R compiler, you need to edit the following string in case
 * XXX you like to change the install path.
 */
#define	PROGPATH(name) "/usr/ccs/bin/name"	/* place to find binaries */
#endif
#endif

#define	streql(s1, s2)	(strcmp((s1), (s2)) == 0)
#undef	fgetline			/* May be #defined by schily.h */
#define	fgetline	log_fgetline

#define	DAY	(24*60*60)

struct filedata {
	urand_t	urand;			/* Unified random number for file */
	char	*init_path;		/* Initial path name from SCCSv6 */
};

struct xtime {
	time_t	xt;			/* time_t value from mk{gm}time() */
	Llong	xlt;			/* Time with larger time range	  */
	long	xns;			/* Nanoseconds for xt / struct tm */
	struct tm xtm;			/* Struct tm from textual parsing */
	int	xgmtoff;		/* GMT offset for struct tm	  */
};

struct delt {
	time_t	time;			/* The time the way UNIX counts */
	long	nsec;			/* Nanoseconds if available	*/
	Llong	ltime;			/* Time with larger time range	*/
	int	gmtoff;			/* GMT offset when in SCCSv6 mode */
	struct tm tm;			/* Struct tm as read from delta	*/
	char	*user;			/* User name from delta		*/
	size_t	userlen;		/* strlen(user)			*/
	char	*file;			/* Filename for this delta	*/
	char	*vers;			/* Version string for this delta */
	char	*comment;		/* Comment for this delta	*/
	size_t	commentlen;		/* strlen(comment)		*/
	int	flags;			/* Flags like PRINTED		*/
	int	ghash;			/* Delta specific checksum	*/
	char	type;
	struct filedata *fdata;
};

#define	PRINTED	0x01

struct author {
	char	*user;			/* The user's login name	*/
	char	*mail;			/* The user's realname and mail */
};

LOCAL	struct	delt	*list;
LOCAL	int		listmax;
LOCAL	int		listsize;

LOCAL	char		*Cwd;
LOCAL	char		*SccsPath = "";
LOCAL	char		*usermapfile;
LOCAL	BOOL		reverse = FALSE;
LOCAL	BOOL		multfile = FALSE;
LOCAL	BOOL		extended = FALSE;
LOCAL	BOOL		changeset = FALSE;
LOCAL	int		nopooling = 0;
LOCAL	time_t		maxdelta = DAY;
LOCAL	Nparms		N;			/* Keep -N parameters	*/
LOCAL	Xparms		X;			/* Keep -X parameters	*/
LOCAL	FILE		*Cs;
LOCAL	char		csname[30];

LOCAL	int	deltcmp		__PR((const void *vp1, const void *vp2));
LOCAL	int	rrcmp		__PR((const void *vp1, const void *vp2));
LOCAL	char *	mapuser		__PR((char *name));
LOCAL	void	usage		__PR((int exitcode));
EXPORT	int	main		__PR((int ac, char *av[]));
LOCAL	void	dodir		__PR((char *name));
LOCAL	void	dofile		__PR((char *name));
LOCAL	int	fgetline	__PR((FILE *, char *, int));
LOCAL	void	handle_created_msg __PR((char *));
LOCAL	int	getN		__PR((const char *, void *));
LOCAL	int	getX		__PR((const char *, void *));
LOCAL	void	print_changeset	__PR((FILE *, struct delt *));
LOCAL	Llong	find_changeset	__PR((int i, struct xtime *xtp, BOOL printit));
LOCAL	void	commit_changeset __PR((int i, struct xtime *xtp));

/*
 * XXX With SCCS v6 local time + GMT off, we should not compare struct tm
 * XXX but time_t or better Llong ltime.
 */
LOCAL int
deltcmp(vp1, vp2)
	const void	*vp1;
	const void	*vp2;
{
	const struct delt *p1 = vp1;
	const struct delt *p2 = vp2;
	const struct tm	*tm1;
	const struct tm	*tm2;

	tm1 = &(p1)->tm;
	tm2 = &(p2)->tm;

	if (tm1->tm_year < tm2->tm_year)
		return (1);
	else if (tm1->tm_year > tm2->tm_year)
		return (-1);
	else if (tm1->tm_mon < tm2->tm_mon)
		return (1);
	else if (tm1->tm_mon > tm2->tm_mon)
		return (-1);
	else if (tm1->tm_mday < tm2->tm_mday)
		return (1);
	else if (tm1->tm_mday > tm2->tm_mday)
		return (-1);
	else if (tm1->tm_hour < tm2->tm_hour)
		return (1);
	else if (tm1->tm_hour > tm2->tm_hour)
		return (-1);
	else if (tm1->tm_min < tm2->tm_min)
		return (1);
	else if (tm1->tm_min > tm2->tm_min)
		return (-1);
	else if (tm1->tm_sec < tm2->tm_sec)
		return (1);
	else if (tm1->tm_sec > tm2->tm_sec)
		return (-1);
	return (0);

#ifdef	OLD
	if ((p1)->time < (p2)->time)
		return (1);
	if ((p1)->time > (p2)->time)
		return (-1);
	return (0);
#endif
}

LOCAL int
rrcmp(vp1, vp2)
	const void	*vp1;
	const void	*vp2;
{
	return (deltcmp(vp1, vp2) * -1);
}

LOCAL char *
mapuser(name)
	char	*name;
{
static	char	nbuf[1024];
static	FILE	*f = NULL;
static	int	cannot = 0;
static	char	*lastname = NULL;
static	char	*lastuser = NULL;
static struct author	*auth = NULL;
static size_t		authsize = 0;
static size_t		authused = 0;
	int	len;

	if (cannot)
		return (name);

	if (lastname && streql(lastname, name))
		return (lastuser);

	if (auth) {
		int	i;

		for (i = 0; i < authused; i++) {
			if (streql(auth[i].user, name)) {
				lastname = name;
				lastuser = auth[i].mail;
				return (lastuser);
			}
		}
	}

	if (f == NULL) {
		char	*home = getenv("HOME");

		if (home == NULL)
			home = ".";
		js_snprintf(nbuf, sizeof (nbuf), "%s/.sccs/usermap", home);
		if (usermapfile)
			f = fopen(usermapfile, "r");
		else
			f = fopen(nbuf, "r");
		if (f == NULL) {
			cannot = 1;
			return (name);
		}
		lastname = lastuser = NULL;
	}
	rewind(f);
	while ((len = fgetline(f, nbuf, sizeof (nbuf))) >= 0) {
		char	*p;

		if (len == 0)
			continue;
		p = strchr(nbuf, '\t');
		if (p == NULL)
			p = strchr(nbuf, ' ');
		if (p == NULL || p == nbuf)
			continue;
		*p++ = '\0';
		if (!streql(nbuf, name))
			continue;
		while (*p == ' ' || *p == '\t')
			p++;
		lastname = name;
		lastuser = p;
		if (authsize <= authused) {
			authsize += 128;
			if (auth == NULL) {
				auth = malloc(authsize * sizeof (*auth));
			} else {
				auth = realloc(auth, authsize * sizeof (*auth));
			}
			if (auth == NULL)
				comerr("No memory.\n");
		}
		auth[authused].user = strdup(name);
		if (auth[authused].user == NULL)
			comerr("No memory.\n");
		auth[authused].mail = strdup(p);
		if (auth[authused].mail == NULL)
			comerr("No memory.\n");
		authused++;
		return (p);
	}
	lastname = lastuser = NULL;
	return (name);
}

LOCAL void
usage(exitcode)
	int	exitcode;
{
	fprintf(stderr, _("Usage: sccslog [options] s.file1 .. s.filen\n"));
	fprintf(stderr, _("	-help		Print this help.\n"));
	fprintf(stderr, _("	-version	Print version number.\n"));
	fprintf(stderr, _("	-a		Print all deltas with times differing > 60s separately.\n"));
	fprintf(stderr, _("	-aa		Print all deltas with different times separately.\n"));
	fprintf(stderr, _("	-Cdir		Base dir for printed filenames.\n"));
	fprintf(stderr, _("	maxdelta=#	Set maximum time delta for a commit (default one day).\n"));
	fprintf(stderr, _("	-multfile	Allow multiple versions of the same file in a commit.\n"));
	fprintf(stderr, _("	-p subdir	Define SCCS subdir.\n"));
	fprintf(stderr, _("	-R		Reverse sorting: oldest entries first.\n"));
	fprintf(stderr, _("	usermap=file	Specify user map file.\n"));
	fprintf(stderr, _("	-x		Include all comment, even SCCSv6 metadata.\n"));
	fprintf(stderr, _("	-Nbulk-spec	Processes a bulk of SCCS history files.\n"));
	fprintf(stderr, _("	-Xxopts		Processes SCCS extended files.\n"));
	exit(exitcode);
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	* const *cav;
	char	*opts = "help,V,version,a+,R,reverse,changeset,multfile,x,C*,p*,maxdelta*,usermap*,N&_,X&_";
	BOOL	help = FALSE;
	BOOL	pversion = FALSE;
	char	*maxdelt = NULL;
	int	i;
	int	j;

	save_args(ac, av);

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

	Fflags = FTLEXIT | FTLMSG | FTLCLN;
#ifdef	SCCS_FATALHELP
	Fflags |= FTLFUNC;
	Ffunc = sccsfatalhelp;
#endif
	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, opts,
			&help, &pversion, &pversion,
			&nopooling,
			&reverse, &reverse,
			&changeset,
			&multfile,
			&extended,
			&Cwd, &SccsPath,
			&maxdelt,
			&usermapfile,
			getN, &N,
			getX, &X) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (pversion) {
		printf(_(
		"sccslog %s-SCCS version %s %s (%s-%s-%s) Copyright (C) 1997-2020 Jörg Schilling\n"),
			PROVIDER,
			VERSION,
			VDATE,
			HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}
	if (maxdelt) {
		i = gettnum(maxdelt, &maxdelta);
		if (i < 0 || maxdelta == 0)
			comerrno(EX_BAD,
				_("Bad time delta specification '%s'.\n"),
				maxdelt);
		nopooling = 0;
	}
	if (changeset) {
		reverse = TRUE;
		snprintf(csname, sizeof (csname), "/tmp/cs.%d", (int)getpid());
	}

	if (N.n_parm) {					/* Parse -N args  */
		parseN(&N);
	}

	xsethome(NULL);
	if (N.n_parm && N.n_sdot && (sethomestat & SETHOME_OFFTREE))
		fatal(gettext("-Ns. not supported in off-tree project mode"));
	Fflags &= ~FTLEXIT;
	Fflags |= FTLJMP;

	cac = ac;
	cav = av;

	i = 0;
	while (getfiles(&cac, &cav, opts) > 0) {
		struct stat	sb;

		if (cav[0][0] == '-' && cav[0][1] == '\0')
			do_file("-", dofile, 0, N.n_sdot, &X);
		else if (stat(cav[0], &sb) >= 0 && S_ISDIR(sb.st_mode))
			dodir(cav[0]);
		else
			dofile(cav[0]);
		i++;
		cac--;
		cav++;
	}
	/*
	 * Make sure that "sccs -R log" results in useful output.
	 */
	if (i == 0 && *SccsPath)
		dodir(SccsPath);

	qsort(list, listsize, sizeof (struct delt), reverse?rrcmp:deltcmp);

#ifdef	SCCSLOG_DEBUG
	printf("%d Einträge\n", listsize);
#endif
	for (i = 0; i < listsize; i++) {
		int	k;
		int	l;
		Llong	xlt;
		Llong	nlt;
		struct xtime xtime;
		struct xtime xntime;

		if (list[i].flags & PRINTED)
			continue;

		/*
		 * First, retrieve latest time stamp for this changeset.
		 */
		nlt = find_changeset(i, &xtime, FALSE);

		/*
		 * Now look for overlapping changesets that need to be first.
		 */
		do {
			xlt = nlt;
			for (j = k = i+1; j < listsize; j++) {
				Llong	lt;

				if (list[j].flags & PRINTED)
					continue;

				if (list[j].comment[0]) {
					/*
					 * First skip all entries with the same
					 * commit message as previous ones.
					 * They are not new ones.
					 */
					for (l = i; l < j; l++) {
						if (list[l].flags & PRINTED)
							continue;
						if (list[l].commentlen ==
						    list[j].commentlen &&
						    streql(list[l].comment,
							    list[j].comment) &&
						    list[l].userlen ==
						    list[j].userlen &&
						    streql(list[l].user,
							    list[j].user)) {
							goto next;
						}
					}
				}

				if (reverse && (list[j].ltime > nlt))
					break;
				if (!reverse && (nlt > list[j].ltime))
					break;

				lt = find_changeset(j, &xntime, FALSE);

				if (lt == LLONG_MAX)
					goto next;

				if (reverse && lt < xlt) {
					xlt = lt;
					k = j;
				}
				if (!reverse && lt > xlt) {
					xlt = lt;
					k = j;
				}
			next:
				;
			}
			if (nlt != xlt) {
				(void) find_changeset(k, &xntime, TRUE);
			}
		} while (nlt != xlt);
		(void) find_changeset(i, &xtime, TRUE);
	}
	return (0);
}

LOCAL void
dodir(name)
	char	*name;
{
	DIR		*dp = opendir(name);
	struct dirent	*d;
	char		*np;
	char		fname[MAXPATHNAME+1];
	char		*base;
	int		len;

	if (dp == NULL) {
		errmsg("Cannot open directory '%s'\n", name);
		return;
	}
	strlcpy(fname, name, sizeof (fname));
	base = &fname[strlen(fname)-1];
	if (*base != '/')
		*++base = '/';
	base++;
	len = sizeof (fname) - strlen(fname);
	while ((d = readdir(dp)) != NULL) {
		char * oparm = N.n_parm;

		np = d->d_name;

		if (np[0] != 's' || np[1] != '.' || np[2] == '\0')
			continue;

		strlcpy(base, np, len);
		N.n_parm = NULL;
		dofile(fname);
		N.n_parm = oparm;
	}
	closedir(dp);
}

LOCAL void
dofile(name)
	char	*name;
{
	FILE	*f;
	char	*buf = NULL;
	size_t	bufsize = 0;
	int	len;
	BOOL	firstline = TRUE;
	BOOL	globalsection = FALSE;
	struct tm tm;
	char	*bname;
	char	*pname;
	struct filedata *fdata;
	char	type = 0;

	if (setjmp(Fjmp))
		return;
	if (N.n_parm) {
#ifdef	__needed__
		char	*ofile = name;
#endif

		name = bulkprepare(&N, name);
		if (name == NULL) {
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
	}

	f = fopen(name, "rb");
	if (f == NULL) {
		errmsg("Cannot open '%s'.\n", name);
		return;
	}
#ifdef	USE_SETVBUF
	setvbuf(f, NULL, _IOFBF, VBUF_SIZE);
#endif
	if (list == NULL) {
		listmax += 128;
		list = malloc(listmax*sizeof (*list));
		if (list == NULL)
			comerr("No memory.\n");
	}

	bname = pname = name;
	if ((pname = strrchr(pname, '/')) == 0)
		pname = name;
	else
		bname = ++pname;
	if (pname[0] == 's' && pname[1] == '.')
		pname += 2;
	if (*SccsPath && (pname != &name[2])) {
		char	*p = malloc(strlen(name) + 2);

		if (p) {
			char	*sp;

			sp = strstr(name, SccsPath);
			if (sp == NULL)
				len = bname - name;
			else
				len = sp - name;
			sprintf(p, "%.*s%s", len, name, pname);
		}
		pname = p;
	} else if (Cwd) {
		char	*p = malloc(strlen(Cwd) + strlen(pname) + 1);

		if (p)
			sprintf(p, "%s%s", Cwd, pname);
		pname = p;
	} else {
		pname = strdup(pname);
	}
	if (pname == NULL)
		comerr("No memory.\n");

	fdata = malloc(sizeof (struct filedata));
	if (fdata == NULL)
		comerr("No memory.\n");
	fdata->init_path = pname;	/* Cheat at the beginning */

	while ((len = getdelim(&buf, &bufsize, '\n', f)) > 0) {
		if (buf[len-1] == '\n')
			buf[--len] = '\0';
		if (firstline) {
			firstline = FALSE;
			if (buf[0] != 1 || buf[1] != 'h') {
				fclose(f);
				return;
			}
		}
		if (len == 0)
			continue;
		if (buf[0] != 1)	/* Not a SCCS control line */
			continue;
		if (buf[1] == 't')	/* End of meta data reached */
			break;
		if (changeset && type == 'R') {
			if (buf[1] == 'd')
				type = 0;
			else
				continue;
		}
		if (buf[1] == 'd') {	/* Delta entry star line */
			char	vers[256];
			char	user[256];
			time_t	t;
			Llong	lt;
			int	nsecs;
			int	gmtoffs;
			char	*p = &buf[4];

			type = buf[3];
			len = sscanf(p, "%s %d/%d/%d %d:%d:%d.%d%d %s",
				vers,
				&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
				&tm.tm_hour, &tm.tm_min, &tm.tm_sec,
				&nsecs,
				&gmtoffs,
				user);
			if (len == 10) {
				int hours = gmtoffs / 100;
				int mins  = gmtoffs % 100;

				gmtoffs = hours * 3600 + mins * 60;
			} else {
				gmtoffs = 1;
				nsecs = 0;
			}
			if (len < 10)
				len = sscanf(p, "%s %d/%d/%d %d:%d:%d%d %s",
				vers,
				&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
				&tm.tm_hour, &tm.tm_min, &tm.tm_sec,
				&gmtoffs,
				user);
			if (len == 9) {
				int hours = gmtoffs / 100;
				int mins  = gmtoffs % 100;

				gmtoffs = hours * 3600 + mins * 60;
			} else if (len < 9) {
				/*
				 * XXX GMT offset aus localtime bestimmen?
				 * XXX Nein, wir nehmen mktime() bei len >= 9.
				 */
				gmtoffs = 1;
			}
			if (len < 9)
				len = sscanf(p, "%s %d/%d/%d %d:%d:%d %s",
				vers,
				&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
				&tm.tm_hour, &tm.tm_min, &tm.tm_sec,
				user);
			if (len < 8) {
				errmsgno(EX_BAD,
					"Cannot scan date '%s' from '%s'.\n",
				p, name);
			}

			if (tm.tm_year >= 100)
				tm.tm_year -= 1900;
			else if (tm.tm_year >= 0 && tm.tm_year < 69)
				tm.tm_year += 100;
			tm.tm_isdst = -1;		/* let mktime() do it */
			tm.tm_mon -= 1;
			seterrno(0);
			if (tm.tm_year >= 138 &&	/* >= year 2038 && */
			    sizeof (t) < sizeof (lt)) {	/* 32 bit time_t */

				/*
				 * Make mk{gm}time() work until 2094 w. 32 Bit
				 * 56 years is 2x the # of years when the
				 * calendar repeats the same weekday...
				 */
				tm.tm_year -= 56;	/* 2 * 4 * 7 */
				if (len >= 9) {		/* w. GMT offset */
							/* never fails */
					t = lt = mklgmtime(&tm);
					lt -= gmtoffs;
					t -= gmtoffs;
				} else {
#undef	mktime						/* Don't use xmktime */
					lt = t = mktime(&tm);
				}
				tm.tm_year += 56;
				lt += 1767225600;	/* 56 years */
			} else {
				if (len >= 9) {		/* w. GMT offset */
							/* never fails */
					t = lt = mklgmtime(&tm);
					lt -= gmtoffs;
					t -= gmtoffs;
				} else {
					lt = t = mktime(&tm);
				}
			}
			/*
			 * Be careful, on IRIX mktime() sets errno but
			 * returns a time_t != -1.
			 */
			if (t == (time_t)-1 && geterrno() != 0) {
				comerr("Cannot convert date '%s' from '%s'.\n",
				p, name);
			}

/*#define	XXX*/
#ifdef	XXX
			error("len: %d '%s' %d/%d/%d%n",
				len, vers,
				tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
				&len);
			error("%*s %2.2d:%2.2d:%2.2d %s %.20s%d %lld %d\n",
				23 - len, "",
				tm.tm_hour, tm.tm_min, tm.tm_sec,
				user, ctime(&t), tm.tm_year+1900,
				lt, geterrno());
#endif

			if (listsize >= listmax) {
				listmax += 128;
				list = realloc(list, listmax*sizeof (*list));
			}
			if (list == NULL)
				comerr("No memory.\n");
			list[listsize].time = t;
			list[listsize].nsec = 0;
			list[listsize].ltime = lt;
			list[listsize].gmtoff = gmtoffs;
			list[listsize].tm   = tm;
			list[listsize].user = strdup(user);
			list[listsize].userlen = strlen(user);
			list[listsize].vers = strdup(vers);
			list[listsize].comment = NULL;
			list[listsize].commentlen = 0;
			list[listsize].flags = 0;
			list[listsize].ghash = -1;
			list[listsize].type = type;
			list[listsize].file = pname;
			list[listsize].fdata = fdata;

			if (nsecs && changeset) {
				dtime_t	dt;

				p = &buf[4];
				NONBLANK(p);
				while (*p != '\0' && *p != ' ' && *p != '\t')
					p++;
				NONBLANK(p);
				date_abz(p, &dt, 0);
				list[listsize].nsec = dt.dt_nsec;
			}

		} else if (buf[1] == 'S') {	/* SID specific metadata */
			if (buf[2] == ' ' && buf[3] == 's') {
				long	l = -1;

				astolb(&buf[4], &l, 10);
				if (l >= 0 && l <= 0xFFFF)
					list[listsize].ghash = l;
			}

		} else if (buf[1] == 'c') {		/* Comment */
			if (buf[2] == '_' && !extended)
				continue;
			if (list[listsize].comment == NULL) {
				list[listsize].comment = strdup(&buf[3]);
				handle_created_msg(list[listsize].comment);
				list[listsize].commentlen =
					strlen(list[listsize].comment);
			} else {
				/*
				 * multi line comments
				 */
				int lastlen = list[listsize].commentlen;

				list[listsize].comment =
				    realloc(list[listsize].comment,
						lastlen + (4-3) + len + 1);
				if (list[listsize].comment == NULL)
					comerr("No memory.\n");
							    /* 4 bytes */
				if (changeset) {
					strcat(list[listsize].comment, "\n");
					list[listsize].commentlen += 1;
				} else {
					strcat(list[listsize].comment, "\n\t  ");
					list[listsize].commentlen += 4;
				}
				strcat(list[listsize].comment, &buf[3]);
				list[listsize].commentlen += strlen(&buf[3]);
			}

		} else if (buf[1] == 'e') {
			if (list[listsize].user == NULL) {
				errmsgno(EX_BAD, "Corrupt file '%s'.\n", name);
				continue;
			}
			/*
			 * Check for very old SCCS history files that may have
			 * no comment at all in special for Release 1.1.
			 */
			if (list[listsize].comment == NULL)
				list[listsize].comment = strdup("");
			listsize++;
		} else if (buf[1] == 'u') {		/* End of delta table */
			globalsection = TRUE;

		} else if (globalsection && buf[1] == 'G') {
			char	*p;

			if (buf[2] != ' ')
				continue;
			if (buf[3] == 'r') {		/* urand number */
				p = &buf[4];
				NONBLANK(p);
				urand_ab(p, &fdata->urand);
				if (urand_valid(&fdata->urand)) {
					char	ubuf[100];

					urand_ba(&fdata->urand,
						ubuf, sizeof (ubuf));
				}

			} else if (buf[3] == 'p') {	/* inital path */
				p = &buf[4];
				NONBLANK(p);
				fdata->init_path = strdup(p);
				if (fdata->init_path == NULL)
					comerr("No memory.\n");
			}
		}
	}
	fclose(f);
}

LOCAL int
fgetline(f, buf, len)
	FILE	*f;
	char	*buf;
	int	len;
{
	if (fgets(buf, len, f) == NULL) {
		if (feof(f) || ferror(f))
			return (EOF);
	}
	len = strlen(buf);
	if (len > 0 && buf[len-1] == '\n')
		buf[--len] = '\0';
	return (len);
}

/*
 * Handle the initial "date and time created ..." message.
 */
LOCAL void
handle_created_msg(s)
	char	*s;
{
	if (strncmp(s, "date and time created ",
		    strlen("date and time created ")) == 0) {
		char	*p1;
		char	*p2;

		/*
		 * If it includes nanoseconds, remove the nanoseconds.
		 */
		if ((p1 = strchr(s, '.'))) {
			if ((p2 = strstr(p1, " by "))) {
				/*
				 * But keep the timezone offset.
				 */
				if ((p2 > (p1+5)) &&
				    (p2[-5] == '+' || p2[-5] == '-'))
					p2 -= 5;
				strcpy(p1, p2);
			}
		}
	}
}

LOCAL int
getN(argp, valp)
	const char	*argp;
	void		*valp;
{
	initN(&N);
	N.n_parm = (char *)argp;
	return (TRUE);
}

LOCAL int
getX(argp, valp)
	const char	*argp;
	void		*valp;
{
	X.x_parm = (char *)argp;
	X.x_flags = XO_NULLPATH;
	if (!parseX(&X))
		return (BADFLAG);
	return (TRUE);
}

LOCAL void
print_changeset(fp, lp)
	FILE		*fp;
	struct	delt	*lp;
{
	char	ubuf[20];
	char	*p  = lp->fdata->init_path;

	urand_ba(&lp->fdata->urand, ubuf, sizeof (ubuf));
	fprintf(fp, "%s|%s|%5.5d|%zd|%s\n",
		ubuf, lp->vers, lp->ghash, strlen(p), p);
}

LOCAL Llong
find_changeset(i, xtp, printit)
	int	i;
	struct xtime *xtp;
	BOOL	printit;
{
	int	j;
	int	k;
	struct xtime xtime;
#define	NFDATA	4096
	struct filedata *fdata[NFDATA];
	struct filedata **fdp = fdata;
	int	nfdata = 0;
	int	fdatamax = NFDATA;

	if (xtp == NULL)
		xtp = &xtime;

	xtp->xt = list[i].time;
	xtp->xlt = list[i].ltime;
	xtp->xns = list[i].nsec;
	xtp->xgmtoff = list[i].gmtoff;
	xtp->xtm = list[i].tm;

	if (printit) {
		struct xtime xptime;

		if (list[i].flags & PRINTED)
			return (LLONG_MAX);

		if (changeset && Cs == NULL) {
			/*
			 * Open file to collect changeset entries for the next
			 * commit.
			 */
			if ((Cs = fopen(csname, "wb")) == NULL)
				comerr("Cannot open '%s'.\n", csname);
		}
		/*
		 * XXX Should we implement a variant with local time +GMT off?
		 */
		find_changeset(i, &xptime, FALSE);
		printf("%.20s%d %s\n",
			ctime(&xptime.xt), xptime.xtm.tm_year + 1900,
			mapuser(list[i].user));
		if (changeset)
			print_changeset(Cs, &list[i]);
		else
			printf("	* %s%s %s\n",
				list[i].type == 'R'? "R ":"",
				list[i].file,
				list[i].vers);
		list[i].flags |= PRINTED;
	}
	if (!multfile)
		fdp[nfdata++] = list[i].fdata;
	for (j = i+1; j < listsize; j++) {
		if (list[j].flags & PRINTED)
			continue;
		if (nopooling) {
			if (list[i].time - list[j].time > 60)
				break;
			if (list[j].time - list[i].time > 60)
				break;
		}
		if (nopooling > 1 &&
		    list[i].time != list[j].time)
			break;
		if (reverse && list[j].time - list[i].time > maxdelta)
			break;
		if (!reverse && list[i].time - list[j].time > maxdelta)
			break;
		if (list[i].comment == NULL || list[j].comment == NULL)
			continue;
		if (list[i].comment[0] == '\0') {
			if (list[i].time - list[j].time > 60)
				break;
			if (list[j].time - list[i].time > 60)
				break;
		}
		if (list[i].commentlen == list[j].commentlen &&
		    streql(list[i].comment, list[j].comment)) {
			if (list[i].userlen != list[j].userlen ||
			    !streql(list[i].user, list[j].user)) {
				/*
				 * When creating a changeset, we cannot allow
				 * a commit with a different user name
				 * inside our timeline.
				 */
				continue;
			}
			for (k = 0; k < nfdata; k++) {
				/*
				 * Check whether the same filename
				 * already appears in our list.
				 */
				if (fdp[k] == list[j].fdata) {
					goto out;
				}
			}
			if (nfdata >= fdatamax) {
				fdatamax += 1024;

				if (fdp != fdata) {
					fdp = realloc(fdp,
						fdatamax * sizeof (*fdp));
				} else {
					fdp = malloc(fdatamax * sizeof (*fdp));
					if (fdp)
						movebytes(fdata, fdp,
							sizeof (fdata));
				}
				if (fdp == NULL)
					comerr("No memory.\n");
			}
			if (!multfile)
				fdp[nfdata++] = list[j].fdata;

			if (xtp->xlt < list[j].ltime) {
				xtp->xt = list[j].time;
				xtp->xlt = list[j].ltime;
				xtp->xns = list[j].nsec;
				xtp->xgmtoff = list[j].gmtoff;
				xtp->xtm = list[j].tm;
			} else if ((xtp->xlt == list[j].ltime) &&
				    (xtp->xns < list[j].nsec)) {
				xtp->xns = list[j].nsec;
			}
			if (printit) {
				if (changeset)
					print_changeset(Cs, &list[j]);
				else
					printf("	* %s%s %s\n",
						list[j].type == 'R'? "R ":"",
						list[j].file,
						list[j].vers);
				list[j].flags |= PRINTED;
			}
		}
	}
out:
	if (printit) {
		printf("	  %s\n\n",
			list[i].comment);
	}

	/*
	 * Make a delta for the next collection of changeset entries.
	 * This simulates an "sccs commit" for that bundle.
	 */
	if (printit && changeset) {
		commit_changeset(i, xtp);
	}
	if (fdp != fdata)
		free(fdp);
	return (xtp->xlt);
}

LOCAL void
commit_changeset(i, xtp)
	int	i;		/* Index in global array */
	struct xtime *xtp;
{
	char	cm[10240];	/* Comment */
	char	dy[100];	/* Datetime */
	char	cs[100];	/* gpath, mapped user, user */
	char	us[100];	/* mapped user */
	char	nm[100];	/* user name */
	char	*mu;
	pid_t	pid;
	int	gmtoffs = xtp->xgmtoff;

	if (gmtoffs < 0)
		gmtoffs = -gmtoffs;

	fflush(Cs);
	fclose(Cs);
	Cs = NULL;

	snprintf(cm, sizeof (cm), "-y%s", list[i].comment);

	us[0] = '\0';
	if ((mu = mapuser(list[i].user)) != list[i].user) {
		snprintf(us, sizeof (us), ",mail=%s", mu);
	}

	snprintf(nm, sizeof (nm), ",user=%s", list[i].user);

	snprintf(cs, sizeof (cs), "-Xgpath=%s%s%s", csname, us, nm);

	snprintf(dy, sizeof (dy),
		"%s=%d/%2.2d%2.2d%2.2d%2.2d%2.2d.%9.9ld%c%2.2d%2.2d",
		"-Xdate",
		xtp->xtm.tm_year + 1900,
		xtp->xtm.tm_mon + 1,
		xtp->xtm.tm_mday,
		xtp->xtm.tm_hour,
		xtp->xtm.tm_min,
		xtp->xtm.tm_sec,
		xtp->xns,
		xtp->xgmtoff < 0 ? '-':'+',
		gmtoffs / 3600,
		(gmtoffs % 3600) / 60);

	/*
	 * /opt/schily/ccs/bin/delta -q -f \
	 * 	-Xprepend,nobulk,gpath=/tmp/cs.$$ \
	 *	-Xmail=mmm -Xdate=xxx -ycomment
	 */
	if ((pid = vfork()) == 0) {
		execl(PROGPATH(delta), "delta",
			"-q", "-f",
			"-Xprepend,nobulk",
			cs,	/* -Xgpath=%s,mail=%s,user=%s */
			dy,	/* -Xdate=%s		    */
			cm,	/* -ycomment		    */
			".sccs/SCCS/s.changeset",
			(char *)NULL);
		_exit(1);
	} else if (pid < 0) {
		comerr("Cannot fork().\n");
	} else {
		WAIT_T	w;

		wait(&w);
		if (*((int *)(&w)) != 0) {
			comerrno(EX_BAD, "Cannot run %s.\n",
				PROGPATH(delta));
		}
	}
}
