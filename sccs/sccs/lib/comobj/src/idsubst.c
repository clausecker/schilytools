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
 * Copyright 2006-2017 J. Schilling
 *
 * @(#)idsubst.c	1.71 17/02/04 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)idsubst.c 1.71 17/02/04 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)idsubst.c"
#pragma ident	"@(#)sccs:lib/comobj/idsubst.c"
#endif

#include	<defines.h>
#include	<had.h>
#include	<i18n.h>

/*
 * ID (keyword) substitution module for get(1) and get(1)-like tasks.
 *
 * Uses HADE and HADK.
 */

#define	DATELEN	12

static int	is_xpg4;
static char	*Whatstr = NULL;	/* To hold -w parameter		*/

	void	sccs_xpg4 __PR((int val));
	void	whatsetup __PR((char *w));
	void	idsetup __PR((struct packet *pkt, char *gname));
static void	makgdate __PR((char *old, char *new));
static void	makgdatel __PR((char *old, char *new));
	char *	idsubst __PR((struct packet *pkt, char *line));
static char *	_trans __PR((char *tp, char *str, char *rest));
static int	readcopy __PR((char *name, char *tp));
	char *	getmodname __PR((void));

void
sccs_xpg4(val)
	int	val;
{
	is_xpg4 = val;
}

void
whatsetup(w)
	char	*w;
{
	Whatstr = w;
}

static char	Curdatel[DT_ZSTRSIZE];	/* d Current date: yyyy/mm/dd	*/
static char	Curdate[DT_ZSTRSIZE];	/* D Current date: yy/mm/dd	*/
static char	*Curtime;		/* T Current time: hh:mm:ss	*/
static char	Gdate[DATELEN];		/* H Current date: mm/dd/yy	*/
static char	Gdatel[DATELEN];	/* h Current date: mm/dd/yyyy	*/
static char	Chgdate[DT_ZSTRSIZE];	/* E Delta date: yy/mm/dd	*/
static char	Chgdatel[DT_ZSTRSIZE];	/* e Delta date: yyyy/mm/dd	*/
static char	*Chgtime;		/* U Delta time: hh:mm:ss	*/
static char	Gchgdate[DATELEN];	/* G Delta date: mm/dd/yy	*/
static char	Gchgdatel[DATELEN];	/* g Delta date: mm/dd/yyyy	*/
static char	Sid[SID_STRSIZE];	/* I SID: r.l.b.s		*/
static char	Mod[FILESIZE];		/* M Module name		*/
static char	Olddir[BUFSIZ];		/* P Used for current dir	*/
static char	Pname[BUFSIZ];		/* P Used for out dir prefix	*/
static char	Dir[BUFSIZ];		/* P Used for file dir prefix	*/
static char	*Qsect;			/* 'q' flag for current file	*/
static char	*Type;			/* 't' flag for current file	*/
static int	num_ID_lines;		/* 's' flag for current file	*/
static int	expand_IDs;		/* whether to expand ISs	*/
static char	*list_expand_IDs;	/* 'y' flag for current file	*/

void
idsetup(pkt, gname)
register struct packet	*pkt;
	char		*gname;
{
	extern dtime_t Timenow;
	register int n;
	register char *p;

	date_ba(&Timenow.dt_sec, Curdate, 0);
	date_bal(&Timenow.dt_sec, Curdatel, 0);
	Curdatel[10] = 0;
	Curtime = &Curdate[9];
	Curdate[8] = 0;
	makgdate(Curdate, Gdate);
	makgdatel(Curdatel, Gdatel);
	for (n = maxser(pkt); n; n--)
		if (pkt->p_apply[n].a_code == APPLY)
			break;
	if (n) {
		date_ba(&pkt->p_idel[n].i_datetime.tv_sec,
						Chgdate, pkt->p_flags);
		date_bal(&pkt->p_idel[n].i_datetime.tv_sec,
						Chgdatel, pkt->p_flags);
	} else {
		if (is_xpg4 && gname != NULL) {
			FILE	*xf;

			if (exists(gname))
				unlink(gname);
			xf = xfcreat(gname,
					HADK ? ((mode_t)0644) : ((mode_t)0444));
			if (xf)
				fclose(xf);
		}
		fatal(gettext("No sid prior to cutoff date-time (ge23)"));
	}
	Chgdatel[10] = 0;
	Chgtime = &Chgdate[9];
	Chgdate[8] = 0;
	makgdate(Chgdate, Gchgdate);
	makgdatel(Chgdatel, Gchgdatel);
	sid_ba(&pkt->p_gotsid, Sid);
	if ((p = pkt->p_sflags[MODFLAG - 'a']) != NULL)		/* 'm' flag */
		copy(p, Mod);
	else
		copy(auxf(pkt->p_file, 'g'), Mod);
	if ((Qsect = pkt->p_sflags[QSECTFLAG - 'a']) == NULL)	/* 'q' flag */
		Qsect = Null;
	if ((Type = pkt->p_sflags[TYPEFLAG - 'a']) == NULL)	/* 't' flag */
		Type = Null;
	if ((p = pkt->p_sflags[SCANFLAG - 'a']) == NULL) {	/* 's' flag */
		num_ID_lines = 0;
	} else {
		num_ID_lines = atoi(p);
	}

	expand_IDs = 0;						/* 'y' flag */
	if ((list_expand_IDs = pkt->p_sflags[EXPANDFLAG - 'a']) != NULL) {
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
}

#ifdef	HAVE_STRFTIME
static void
makgdate(old, new)
register char *old, *new;
{
	struct tm	tm;
	sscanf(old, "%d/%d/%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday));
	tm.tm_mon--;
	strftime(new, (size_t) DATELEN, NOGETTEXT("%D"), &tm);
}

static void
makgdatel(old, new)
register char *old, *new;
{
	struct tm	tm;
	sscanf(old, "%d/%d/%d", &(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday));
	if (tm.tm_year > 1900)
		tm.tm_year -= 1900;
	tm.tm_mon--;
	strftime(new, (size_t) DATELEN, NOGETTEXT("%m/%d/%Y"), &tm);
}


#else	/* HAVE_STRFTIME not defined */

static void
makgdate(old, new)
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
makgdatel(old, new)
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


static char Zkeywd[5] = "@(#)";		/* Constant string for Zkeyword	    */
static char *tline = NULL;		/* Growable buffer for trans/readcopy */
static size_t tline_size = 0;		/* Current size of growable buffer    */

#define	trans(a, b)	_trans(a, b, lp)

char *
idsubst(pkt, line)
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

	if (HADK) {
		if (!expand_IDs)
			return (line);
	} else {
		if (!any('%', line))
			return (line);
	}
	if (pkt->p_glnno > num_ID_lines) {
		if (!expand_IDs) {
			return (line);
		}
		if (num_ID_lines) {
			expand_IDs = 0;
			return (line);
		}
	}
	expand_XIDs = sflags[EXTENSFLAG - 'a'] != NULL ||
			list_expand_IDs != NULL;
	tp = tline;
	for (lp = line; *lp != 0; lp++) {
		if ((!pkt->p_did_id) && (sflags[IDFLAG - 'a']) &&
		    (!(strncmp(sflags[IDFLAG-'a'], lp, strlen(sflags[IDFLAG-'a'])))))
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
				tp = trans(tp, Mod);
				break;
			case 'Q':
				tp = trans(tp, Qsect);
				break;
			case 'R':
				sprintf(str, "%d", pkt->p_gotsid.s_rel);
				tp = trans(tp, str);
				break;
			case 'L':
				sprintf(str, "%d", pkt->p_gotsid.s_lev);
				tp = trans(tp, str);
				break;
			case 'B':
				sprintf(str, "%d", pkt->p_gotsid.s_br);
				tp = trans(tp, str);
				break;
			case 'S':
				sprintf(str, "%d", pkt->p_gotsid.s_seq);
				tp = trans(tp, str);
				break;
			case 'D':
				tp = trans(tp, Curdate);
				break;
			case 'd':
				if (!expand_XIDs)
					goto noexpand;
				tp = trans(tp, Curdatel);
				break;
			case 'H':
				tp = trans(tp, Gdate);
				break;
			case 'h':
				if (!expand_XIDs)
					goto noexpand;
				tp = trans(tp, Gdatel);
				break;
			case 'T':
				tp = trans(tp, Curtime);
				break;
			case 'E':
				tp = trans(tp, Chgdate);
				break;
			case 'e':
				if (!expand_XIDs)
					goto noexpand;
				tp = trans(tp, Chgdatel);
				break;
			case 'G':
				tp = trans(tp, Gchgdate);
				break;
			case 'g':
				if (!expand_XIDs)
					goto noexpand;
				tp = trans(tp, Gchgdatel);
				break;
			case 'U':
				tp = trans(tp, Chgtime);
				break;
			case 'Z':
				tp = trans(tp, Zkeywd);
				break;
			case 'Y':
				tp = trans(tp, Type);
				break;
			/*FALLTHRU*/
			case 'W':
				if ((Whatstr != NULL) && (recursive == 0)) {
					recursive = 1;
					lp += 2;
					new_hold_size = strlen(lp) +
							strlen(Whatstr) + 1;
					if (new_hold_size > hold_size) {
						if (hold)
							free(hold);
						hold_size = new_hold_size;
						hold = (char *)malloc(hold_size);
						if (hold == NULL) {
							efatal(gettext("OUT OF SPACE (ut9)"));
						}
					}
					strcpy(hold, Whatstr);
					strcat(hold, lp);
					lp = hold;
					lp--;
					continue;
				}
				tp = trans(tp, Zkeywd);
				tp = trans(tp, Mod);
				tp = trans(tp, "\t");
			case 'I':
				tp = trans(tp, Sid);
				break;
			case 'P':
				copy(pkt->p_file, Dir);
				dname(Dir);
				if (getcwd(Olddir, sizeof (Olddir)) == NULL)
				    efatal(gettext("getcwd() failed (ge20)"));
				if (chdir(Dir) != 0)
				    efatal(gettext("cannot change directory (ge21)"));
				if (getcwd(Pname, sizeof (Pname)) == NULL)
				    efatal(gettext("getcwd() failed (ge20)"));
				if (chdir(Olddir) != 0)
				    efatal(gettext("cannot change directory (ge21)"));
				tp = trans(tp, Pname);
				tp = trans(tp, "/");
				tp = trans(tp, (sname(pkt->p_file)));
				break;
			case 'F':
				tp = trans(tp, pkt->p_file);
				break;
			case 'C':
				sprintf(str, "%d", pkt->p_glnno);
				tp = trans(tp, str);
				break;
			case 'A':
				tp = trans(tp, Zkeywd);
				tp = trans(tp, Type);
				tp = trans(tp, " ");
				tp = trans(tp, Mod);
				tp = trans(tp, " ");
				tp = trans(tp, Sid);
				tp = trans(tp, Zkeywd);
				break;
			noexpand:
			default:
				str[0] = '%';
				str[1] = *lp;
				str[2] = '\0';
				tp = trans(tp, str);
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
					return (tline);
				*p = '%';
			}
			str[0] = *lp;
			str[1] = '\0';
			tp = trans(tp, str);
		} else {
			str[0] = *lp;
			str[1] = '\0';
			tp = trans(tp, str);
		}
	}
	return (tline);
}

static char *
_trans(tp, str, rest)
register char *tp, *str, *rest;
{
	register size_t filled_size	= tp - tline;
	register size_t new_size	= filled_size + strlen(str) + 1;

	if (new_size > tline_size) {
		tline_size = new_size + strlen(rest) + DEF_LINE_SIZE;
		tline = (char *) realloc(tline, tline_size);
		if (tline == NULL) {
			efatal(gettext("OUT OF SPACE (ut9)"));
		}
		tp = tline + filled_size;
	}
	while ((*tp++ = *str++) != '\0')
		;
	return (tp-1);
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
	char ipath[MAXPATHLEN];
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

		if (p == NULL) {
			strlcpy(ipath, sccs_insbase, sizeof (ipath));
			strlcat(ipath, "/ccs/include", sizeof (ipath));
			p = ipath;
		}
		do {
			for (np = p; *np; np++)
				if (*np == ':')
					break;
			(void) snprintf(path, sizeof (path), "%.*s/%s",
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
		tline = (char *) realloc(tline, tline_size);
		if (tline == NULL) {
			efatal(gettext("OUT OF SPACE (ut9)"));
		}
		tp = tline + filled_size;
	}

	while ((ch = getc(fp)) != EOF)
		*tp++ = ch;
	*tp++ = '\0';
	(void) fclose(fp);
	return (1);
}

char *
getmodname()
{
	return (Mod);
}
