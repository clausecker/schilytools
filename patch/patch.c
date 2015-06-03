/* @(#)patch.c	1.40 15/06/03 2011-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)patch.c	1.40 15/06/03 2011-2015 J. Schilling";
#endif
/*
 *	Copyright (c) 1984-1988 Larry Wall
 *	Copyright (c) 2011-2015 J. Schilling
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition is met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this condition and the following disclaimer.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define	EXT
#include "common.h"
#undef	EXT
#define	EXT	extern
#include "util.h"
#include "pch.h"
#include "inp.h"

#define	PATCHVERSION	"3.1"

/* procedures */

	int	main __PR((int argc, char **argv));

static	void	reinitialize_almost_everything __PR((void));
static	void	init_defaults __PR((void));
static	char	*nextarg __PR((void));
static	void	get_some_switches __PR((void));
static	LINENUM	locate_hunk __PR((LINENUM fuzz));
static	void	abort_hunk __PR((void));
static	void	apply_hunk __PR((LINENUM where));
static	void	init_output __PR((char *name));
static	void	init_reject __PR((char *name));
static	void	copy_till __PR((LINENUM lastline));
static	void	spew_output __PR((void));
static	void	dump_line __PR((LINENUM line));
static	bool	patch_match __PR((LINENUM base, LINENUM offset, LINENUM fuzz));
static	bool	similar __PR((char *a, char *b, int len));
	void	my_exit __PR((int status));

/* Nonzero if -R was specified on command line.  */
static int reverse_flag_specified = FALSE;

static bool do_defines;			/* -D patch using ifdef, ifndef, etc. */
static bool remove_empty;		/* -E on command line */
static char if_defined[128];		/* #ifdef xyzzy */
static char not_defined[128];		/* #ifndef xyzzy */
static char else_defined[] = "#else\n";	/* #else */
static char end_defined[128];		/* #endif xyzzy */

static char serrbuf[BUFSIZ];		/* buffer for stderr */

static LINENUM maxfuzz = 2;		/* -F maxfuzz	*/


/* Apply a set of diffs as appropriate. */

int
main(argc, argv)
	int	argc;
	char	**argv;
{
	LINENUM where;
	LINENUM newwhere;
	LINENUM fuzz;
	LINENUM mymaxfuzz;
	int hunk = 0;
	int failed = 0;
	int failtotal = 0;
	int reject_written = 0;
	int i;
#if	defined(USE_NLS) && defined(INS_BASE)
	char *dir;
#endif

	setbuf(stderr, serrbuf);

	(void) setlocale(LC_ALL, "");

#if	defined(USE_NLS)
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "patch"	/* Use this only if it weren't */
#endif
#ifdef	INS_BASE
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#ifdef	PROTOTYPES
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
#endif	/* INS_BASE */
	(void) textdomain(TEXT_DOMAIN);
#endif
	time(&starttime);
	bufsize = BUFFERSIZE;
	___mexval(EXIT_FAIL);
	buf = ___malloc(BUFFERSIZE, _("general purpose buffer"));

	for (i = 0; i < MAXFILEC; i++)
		filearg[i] = Nullch;

	/* Cons up the names of the temporary files.  */
	{
	/* Directory for temporary files.  */
	char *tmpdir;
	int tmpname_len;

	tmpdir = getenv("TMPDIR");
	if (tmpdir == NULL) {
		tmpdir = "/tmp";
	}
	tmpname_len = strlen(tmpdir) + 20;

	TMPOUTNAME = (char *) malloc(tmpname_len);
	strcpy(TMPOUTNAME, tmpdir);
	strcat(TMPOUTNAME, "/patchoXXXXXX");
	Mktemp(TMPOUTNAME);

	TMPINNAME = (char *) malloc(tmpname_len);
	strcpy(TMPINNAME, tmpdir);
	strcat(TMPINNAME, "/patchiXXXXXX");
	Mktemp(TMPINNAME);

	TMPREJNAME = (char *) malloc(tmpname_len);
	strcpy(TMPREJNAME, tmpdir);
	strcat(TMPREJNAME, "/patchrXXXXXX");
	Mktemp(TMPREJNAME);

	TMPPATNAME = (char *) malloc(tmpname_len);
	strcpy(TMPPATNAME, tmpdir);
	strcat(TMPPATNAME, "/patchpXXXXXX");
	Mktemp(TMPPATNAME);
	}

	/* parse switches */
	Argc = argc;
	Argv = argv;
	init_defaults();	/* Set initial default values */
	get_some_switches();

	/* make sure we clean up /tmp in case of disaster */
	set_signals(0);

	using_plan_a = TRUE;	/* try to keep everything in memory */

	/* for each patch in patch file */
	for (open_patch_file(filearg[1]);
	    there_is_another_patch();
	    reinitialize_almost_everything()) {

		filemode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		if (outname == Nullch)
			outname = savestr(filearg[0]);

		/* initialize the patched file */
		if (!skip_rest_of_patch)
			init_output(TMPOUTNAME);

		/* for ed script just up and do it and exit */
		if (diff_type == ED_DIFF) {
			do_ed_script();
			continue;
		}

		/* initialize reject file */
		init_reject(TMPREJNAME);

		/* find out where all the lines are */
		if (!skip_rest_of_patch)
			scan_input(filearg[0]);

		/* from here on, open no standard i/o files, because malloc */
		/* might misfire and we can't catch it easily */

		/* apply each hunk of patch */
		hunk = 0;
		failed = 0;
		out_of_mem = FALSE;
		while (another_hunk()) {
			where = Nulline;
			hunk++;
			fuzz = Nulline;
			mymaxfuzz = pch_context();
			if (maxfuzz < mymaxfuzz)
				mymaxfuzz = maxfuzz;
			if (!skip_rest_of_patch) {
				do {
					where = locate_hunk(fuzz);
					if (hunk == 1 && where == Nulline &&
					    !force) {
						/*
						 * dwim for reversed patch?
						 *
						 * In case of a normal diff,
						 * swapping a simple delete will
						 * always work and this is not
						 * what we like to do...
						 */
						if (diff_type == NORMAL_DIFF &&
						    p_repl_lines == 0)
							continue;

						if (!pch_swap()) {
							if (fuzz == Nulline)
								say(
_("Not enough memory to try swapped hunk!  Assuming unswapped.\n"));
							continue;
						}
						reverse = !reverse;
						/*
						 * try again
						 */
						where = locate_hunk(fuzz);
						if (where == Nulline) {
							/*
							 * didn't find it
							 * swapped,
							 * put it back to normal
							 */
							if (!pch_swap())
								fatal(
_("Lost hunk on alloc error!\n"));
							reverse = !reverse;
						} else if (noreverse) {
							/*
							 * put it back to normal
							 */
							if (!pch_swap())
								fatal(
_("Lost hunk on alloc error!\n"));
							reverse = !reverse;
							say(
_("Ignoring previously applied (or reversed) patch.\n"));
							skip_rest_of_patch =
									TRUE;
						} else {
							if (reverse)
								ask(
_("Reversed (or previously applied) patch detected!  Assume -R? [y] "));
							else
								ask(
_("Unreversed (or previously applied) patch detected!  Ignore -R? [y] "));
							if (*buf == 'n') {
								ask(
_("Apply anyway? [n] "));
								if (*buf != 'y')
									skip_rest_of_patch = TRUE;
								where = Nulline;
								reverse =
								    !reverse;
								/*
								 * put it back
								 * to normal
								 */
								if (!pch_swap())
									fatal(
_("Lost hunk on alloc error!\n"));
							}
						}
					}
				} while (!skip_rest_of_patch &&
					    where == Nulline &&
					    ++fuzz <= mymaxfuzz);

				if (skip_rest_of_patch) { /* just got decided */
					Fclose(ofp);
					ofp = Nullfp;
				}
			}

			newwhere = pch_newfirst() + last_offset;
			if (skip_rest_of_patch) {
				abort_hunk();
				failed++;
				if (verbose)
					say(_("Hunk #%d ignored at %lld.\n"),
					    hunk, (Llong)newwhere);
			} else if (where == Nulline) {
				abort_hunk();
				failed++;
				if (verbose)
					say(_("Hunk #%d failed at %lld.\n"),
					    hunk, (Llong)newwhere);
			} else {
				apply_hunk(where);
				if (verbose > TRUE ||
				    (verbose && (fuzz || last_offset))) {
					say(_("Hunk #%d succeeded at %lld"),
					    hunk, (Llong)newwhere);
					if (fuzz)
						say(
_(" with fuzz %lld"), (Llong)fuzz);
					if (last_offset) {
						if (last_offset == 1)
						say(_(" (offset 1 line)"));
						else
						say(_(" (offset %lld lines)"),
						    (Llong)last_offset);
					}
					say(".\n");
				}
			}
		}

		if (out_of_mem && using_plan_a) {
			Argc = Argc_last;
			Argv = Argv_last;
			say(
_("\n\nRan out of memory using Plan A--trying again...\n\n"));
			continue;
		}

		assert(hunk);

		/* finish spewing out the new file */
		if (!skip_rest_of_patch)
			spew_output();

		/* and put the output where desired */
		ignore_signals();
		if (!skip_rest_of_patch) {
			if (!failed && wall_plus && !strEQ(outname, "-") &&
			    (file_stat.st_size == (size_t)0) &&
			    (remove_empty || is_null_time[reverse])) {
				say(_(
"Removing file %s and any empty ancestor directories.\n"), outname);
				move_file(NULL, outname);
				removedirs(outname);
			} else if (move_file(TMPOUTNAME, outname) < 0) {
				toutkeep = TRUE;
				chmod(TMPOUTNAME, filemode);
			} else {
				chmod(outname, filemode);
			}
		}
		Fclose(rejfp);
		rejfp = Nullfp;
		if (failed) {
			failtotal += failed;
			if (failtotal <= 0)
				failtotal = 1;

			if (!*rejname) {
				Strcpy(rejname, outname);
#ifndef FLEXFILENAMES
				{
				char *s = strrchr(rejname, '/');

				if (!s)
					s = rejname;
				/*
				 * try to preserve difference between
				 * .h, .c, .y, etc.
				 */
				if (strlen(s) > 13)
					if (s[12] == '.')
						s[12] = s[13];
				s[13] = '\0';
				}
#endif
				Strcat(rejname, REJEXT);
			}
			if (skip_rest_of_patch) {
				say(
_("%d out of %d hunks ignored--saving rejects to %s\n"),
				    failed, hunk, rejname);
			} else {
				say(
_("%d out of %d hunks failed--saving rejects to %s\n"),
				    failed, hunk, rejname);
			}
			if (move_file(TMPREJNAME, rejname) < 0) {
				trejkeep = TRUE;
				reject_written = EXIT_FAIL;
			} else {
				reject_written = EXIT_REJECT;
			}
		}
		set_signals(1);
	}

	if (reject_written)
		my_exit(reject_written);
	/*
	 * Use POSIX exit code for erors if any failure happened.
	 */
	if (failtotal > 0)
		failtotal = EXIT_FAIL;
	my_exit(failtotal);
	/* NOTREACHED */
	return (failtotal);
}

/* Prepare to find the next patch to do in the patch file. */

static void
reinitialize_almost_everything()
{
	re_patch();
	re_input();

	input_lines = 0;
	last_frozen_line = 0;

	filec = 0;
	if (filearg[0] != Nullch && !out_of_mem) {
		free(filearg[0]);
		filearg[0] = Nullch;
	}

	if (outname != Nullch) {
		free(outname);
		outname = Nullch;
	}

	last_offset = 0;

	diff_type = 0;

	if (revision != Nullch) {
		free(revision);
		revision = Nullch;
	}

	reverse = reverse_flag_specified;
	skip_rest_of_patch = FALSE;

	get_some_switches();

	if (filec >= 2)
		fatal(_("You may not change to a different patch file.\n"));
}

static void
init_defaults()
{
	if (Argv[0] == Nullch) {
		do_posix = TRUE; /* Default to POSIX behavior	*/
	} else {
		char *s = strrchr(Argv[0], '/');

		if (s == Nullch)
			s = Argv[0];
		else
			s++;
		if (strEQ(s, "sccspatch")) {
			do_posix = TRUE;
			wall_plus = TRUE; /* POSIX + old patch options	*/
		} else if (!strEQ(s, "opatch")) {
			do_posix = TRUE;
		}
	}
	if (getenv("POSIXLY_CORRECT") != Nullch) {
		do_posix = TRUE; /* Do it similar to GNU patch	*/
		wall_plus = TRUE; /* POSIX + old patch options	*/
	}
	if (!do_posix) {	/* If not POSIX			*/
		wall_plus = TRUE; /* allow old patch options 	*/
#ifdef	BACKUP_BY_DEFAULT
		do_backup = TRUE; /* and always create a backup	*/
#endif
	}

	verbose = TRUE;		/* Initial default */
	strippath = 957;	/* Initial default */
}

static char *
nextarg()
{
	if (!--Argc)
		fatal(_("patch: missing argument after `%s'\n"), *Argv);
	return (*++Argv);
}

/* Process switches and filenames up to next '+' or end of list. */

static void
get_some_switches()
{
	bool endopts = FALSE;
	char *s;

	rejname[0] = '\0';
	Argc_last = Argc;
	Argv_last = Argv;
	if (!Argc)
		return;
	for (Argc--, Argv++; Argc; Argc--, Argv++) {
		s = Argv[0];
		if (!do_posix && strEQ(s, "+")) {
			return;		/* + will be skipped by for loop */
		}
		if (endopts || *s != '-' || !s[1]) {
			if (filec == MAXFILEC || (do_posix && filec > 0))
				fatal(_("patch: Too many file arguments.\n"));
			filearg[filec++] = savestr(s);
		} else {
			switch (*++s) {

			case 'b':			/* POSIX: no arg */
				/*
				 * POSIX:	backup file only with -b
				 * Larry Wall:	always create a backup file
				 *		set backup extension with -b
				 */
#ifdef	BACKUP_BY_DEFAULT
				if (do_posix)
					do_backup = TRUE;
				else
					origext = savestr(nextarg());
#else
				/*
				 * We by default follow POSIX and GNU and do
				 * not create a backup unless -b was used.
				 */
				do_backup = TRUE;
#endif
				break;
			case 'B':			/* Wall: PL 10	*/
				if (!wall_plus)
					goto unknown;
				origprae = savestr(nextarg());
				break;
			case 'c':			/* POSIX: OK	*/
				diff_type = CONTEXT_DIFF;
				break;
			case 'd':			/* POSIX: OK	*/
				if (!*++s)
					s = nextarg();
				if (chdir(s) < 0)
					pfatal(_("Can't cd to %s.\n"), s);
				break;
			case 'D':			/* POSIX: ~..OK	*/
				do_defines = TRUE;
				if (!*++s)
					s = nextarg();
				if (!isalpha(UCH *s) && '_' != *s)
					fatal(
_("Argument to -D not an identifier.\n"));
				Sprintf(if_defined, "#ifdef %s\n", s);
				Sprintf(not_defined, "#ifndef %s\n", s);
				if (do_posix) {
					Sprintf(end_defined, "#endif\n");
				} else {
					Sprintf(end_defined,
						"#endif /* %s */\n", s);
				}
				break;
			case 'E':			/* GNU		*/
				if (!wall_plus)
					goto unknown;
				remove_empty = TRUE;
				break;
			case 'e':			/* POSIX: OK	*/
				diff_type = ED_DIFF;
				break;
			case 'f':			/* Wall: all vers */
				if (!wall_plus)
					goto unknown;
				force = TRUE;
				break;
			case 'F':			/* Wall: all vers */
				if (!wall_plus)
					goto unknown;
				if (*++s == '=')
					s++;
				maxfuzz = atolnum(s);
				break;
			case 'i':			/* POSIX: only	*/
				if (!*++s)
					s = nextarg();
				else if (*s == '=')
					s++;
				filearg[1] = savestr(s);
				break;
			case 'l':			/* POSIX: OK	*/
				canonicalize = TRUE;
				break;
			case 'n':			/* POSIX: OK	*/
				diff_type = NORMAL_DIFF;
				break;
			case 'N':			/* POSIX: OK	*/
				noreverse = TRUE;
				break;
			case 'o':			/* POSIX: OK	*/
				do_backup = TRUE;	/* Always backup */
				outname = savestr(nextarg());
				break;
			case 'p':			/* POSIX: ~..OK	*/
				/*
				 * Larry Wall:	-p may have no arg and any
				 *		arg to -p must directly follow.
				 * POSIX:	no -p: only match basename
				 *		if -p: must have an argument
				 */
				s++;
				if (do_posix && !*s)
					s = nextarg();
				else if (*s == '=')
					s++;
				strippath = atoinum(s);
				break;
			case 'r':			/* POSIX: OK	*/
				Strcpy(rejname, nextarg());
				break;
			case 'R':			/* POSIX: OK	*/
				reverse = TRUE;
				reverse_flag_specified = TRUE;
				break;
			case 's':			/* Wall: all vers */
				if (!wall_plus)
					goto unknown;
				verbose = FALSE;
				break;
			case 'S':			/* Wall: all vers */
				if (!wall_plus)
					goto unknown;
				skip_rest_of_patch = TRUE;
				break;
			case 'u':			/* POSIX: OK	*/
				/*
				 * Added with patch-2.0.12u5
				 * Not compatible with historical patch.
				 */
				diff_type = UNI_DIFF;
				break;
			case 'v':			/* Wall: all vers */
				if (!wall_plus)
					goto unknown;
				if (s[1] == 'v') {
					s++;
					verbose = TRUE + 1;
					break;
				}

				printf(
				_("patch %s (%s-%s-%s)\n\n"),
					PATCHVERSION,
					HOST_CPU, HOST_VENDOR, HOST_OS);
				printf(
				_("Copyright (C) 1984 - 1988 Larry Wall\n"));
				printf(_("Copyright (C) 2011 - 2015 %s\n"),
					_("Joerg Schilling"));
				printf(_(
"This is free software; see the source for copying conditions.  There is NO\n"
));
				printf(_(
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
));
				exit(EXIT_OK);
				break;
			case 'W':			/* POSIX: vendor */
				if (strEQ(s, "W+")) {	/* POSIX +old */
					wall_plus = TRUE;
				} else if (strEQ(s, "Wall")) {
					do_wall = TRUE;	/* Larry 2.0 compat */
				} else if (strEQ(s, "Wposix")) {
					do_posix = TRUE;
					wall_plus = FALSE;
					do_wall = FALSE;
				} else if (strEQ(s, "W-posix")) {
					do_posix = FALSE;
					do_wall = FALSE;
				}
				break;
#ifdef DEBUGGING
			case 'x':			/* Wall: all vers */
				if (!wall_plus)
					goto unknown;
				debug = atoinum(s+1);
				break;
#endif
			case 'z':			/* GNU		*/
				if (!wall_plus)
					goto unknown;
				origext = savestr(nextarg());
				break;
			case '-':			/* "--"		*/
				if (*++s == '\0') {
					endopts = TRUE;
					break;
				}
				/* FALLTHROUGH */
			default:
			unknown:
				fprintf(stderr,
				_("patch: unrecognized option `%s'\n"),
					Argv[0]);
				if (do_posix && !wall_plus) {
					fprintf(stderr, _("\
Usage: patch [-blNR] [-c|-e|-n|-u] [-d dir] [-D define] [-i patchfile]\n\
\t[-o outfile] [-p num] [-r rejectfile] [file]\n\
"));
				} else {
					fprintf(stderr, _("\
Usage: patch [-bEflNRsSv] [-c|-e|-n|-u]\n\
\t[-z backup-ext] [-B backup-prefix] [-d directory]\n\
\t[-D symbol] [-Fmax-fuzz] [-i patchfile] [-o out-file] [-p[strip-count]]\n\
\t[-r rej-name] [origfile] [patchfile] [[+] [options] [origfile]...]\n\
\t[-W+] [-Wall] [-Wposix] [-W-posix]\n\
"));
				}
				my_exit(EXIT_FAIL);
			}
		}
	}
}

/* Attempt to find the right place to apply this hunk of patch. */

static LINENUM
locate_hunk(fuzz)
	LINENUM fuzz;
{
	LINENUM first_guess = pch_first() + last_offset;
	LINENUM offset;
	LINENUM pat_lines = pch_ptrn_lines();
	LINENUM max_pos_offset = input_lines - first_guess
				- pat_lines + 1;
	LINENUM max_neg_offset = first_guess - last_frozen_line - 1
				+ pch_context();

	if (!pat_lines)			/* null range matches always */
		return (first_guess);
	if (max_neg_offset >= first_guess) /* do not try lines < 0 */
		max_neg_offset = first_guess - 1;
	if (first_guess <= input_lines &&
	    patch_match(first_guess, Nulline, fuzz))
		return (first_guess);
	for (offset = 1; ; offset++) {
		bool check_after = (offset <= max_pos_offset);
		bool check_before = (offset <= max_neg_offset);

		if (check_after && patch_match(first_guess, offset, fuzz)) {
#ifdef DEBUGGING
			if (debug & 1)
				say(_("Offset changing from %lld to %lld\n"),
					(Llong)last_offset, (Llong)offset);
#endif
			last_offset = offset;
			return (first_guess+offset);
		} else if (check_before &&
			    patch_match(first_guess, -offset, fuzz)) {
#ifdef DEBUGGING
			if (debug & 1)
				say(_("Offset changing from %lld to %lld\n"),
					(Llong)last_offset, (Llong)-offset);
#endif
			last_offset = -offset;
			return (first_guess-offset);
		} else if (!check_before && !check_after) {
			return (Nulline);
		}
	}
}

/* We did not find the pattern, dump out the hunk so they can handle it. */

static void
abort_hunk()
{
	LINENUM i;
	LINENUM pat_end = pch_end();
	/* add last_offset to guess the same as the previous successful hunk */
	LINENUM oldfirst = pch_first() + last_offset;
	LINENUM newfirst = pch_newfirst() + last_offset;
	LINENUM oldlast = oldfirst + pch_ptrn_lines() - 1;
	LINENUM newlast = newfirst + pch_repl_lines() - 1;
	char *stars = (diff_type >= NEW_CONTEXT_DIFF ? " ****" : "");
	char *minuses = (diff_type >= NEW_CONTEXT_DIFF ? " ----" : " -----");

	fprintf(rejfp, "***************\n");
	for (i = 0; i <= pat_end; i++) {
		switch (pch_char(i)) {

		case '*':
			if (oldlast < oldfirst)
				fprintf(rejfp, "*** 0%s\n", stars);
			else if (oldlast == oldfirst)
				fprintf(rejfp, "*** %lld%s\n",
					(Llong)oldfirst, stars);
			else
				fprintf(rejfp, "*** %lld,%lld%s\n",
					(Llong)oldfirst, (Llong)oldlast,
					stars);
			break;
		case '=':
			if (newlast < newfirst)
				fprintf(rejfp, "--- 0%s\n", minuses);
			else if (newlast == newfirst)
				fprintf(rejfp, "--- %lld%s\n",
					(Llong)newfirst, minuses);
			else
				fprintf(rejfp, "--- %lld,%lld%s\n",
					(Llong)newfirst,
					(Llong)newlast, minuses);
			break;
		case '\n':
			fprintf(rejfp, "%s", pfetch(i));
			break;
		case ' ':
		case '-':
		case '+':
		case '!':
			fprintf(rejfp, "%c %s", pch_char(i), pfetch(i));
			break;
		default:
			say(_("Fatal internal error in abort_hunk().\n"));
			abort();
		}
	}
}

/* We found where to apply it (we hope), so do it. */

static void
apply_hunk(where)
	LINENUM where;
{
	LINENUM old = 1;
	LINENUM lastline = pch_ptrn_lines();
	LINENUM new = lastline+1;
#define	OUTSIDE		0
#define	IN_IFNDEF	1
#define	IN_IFDEF	2
#define	IN_ELSE		3
	int def_state = OUTSIDE;
	bool R_do_defines = do_defines;
	LINENUM pat_end = pch_end();

	where--;
	while (pch_char(new) == '=' || pch_char(new) == '\n')
		new++;

	while (old <= lastline) {
		if (pch_char(old) == '-') {
			copy_till(where + old - 1);
			if (R_do_defines) {
				if (def_state == OUTSIDE) {
					fputs(not_defined, ofp);
					def_state = IN_IFNDEF;
				} else if (def_state == IN_IFDEF) {
					fputs(else_defined, ofp);
					def_state = IN_ELSE;
				}
				fputs(pfetch(old), ofp);
			}
			last_frozen_line++;
			old++;
		} else if (new > pat_end) {
			break;
		} else if (pch_char(new) == '+') {
			copy_till(where + old - 1);
			if (R_do_defines) {
				if (def_state == IN_IFNDEF) {
					fputs(else_defined, ofp);
					def_state = IN_ELSE;
				} else if (def_state == OUTSIDE) {
					fputs(if_defined, ofp);
					def_state = IN_IFDEF;
				}
			}
			fputs(pfetch(new), ofp);
			new++;
		} else if (pch_char(new) != pch_char(old)) {
			say(
_("Out-of-sync patch, lines %lld,%lld--mangled text or line numbers, maybe?\n"),
				(Llong)(pch_hunk_beg() + old),
				(Llong)(pch_hunk_beg() + new));
#ifdef DEBUGGING
			say("oldchar = '%c', newchar = '%c'\n",
			    pch_char(old), pch_char(new));
#endif
			my_exit(EXIT_FAIL);
		} else if (pch_char(new) == '!') {
			copy_till(where + old - 1);
			if (R_do_defines) {
				fputs(not_defined, ofp);
				def_state = IN_IFNDEF;
			}
			while (pch_char(old) == '!') {
				if (R_do_defines) {
					fputs(pfetch(old), ofp);
				}
				last_frozen_line++;
				old++;
			}
			if (R_do_defines) {
				fputs(else_defined, ofp);
				def_state = IN_ELSE;
			}
			while (pch_char(new) == '!') {
				fputs(pfetch(new), ofp);
				new++;
			}
		} else {
			assert(pch_char(new) == ' ');
			old++;
			new++;
			if (R_do_defines && def_state != OUTSIDE) {
				fputs(end_defined, ofp);
				def_state = OUTSIDE;
			}
		}
	}
	if (new <= pat_end && pch_char(new) == '+') {
		copy_till(where + old - 1);
		if (R_do_defines) {
			if (def_state == OUTSIDE) {
				fputs(if_defined, ofp);
				def_state = IN_IFDEF;
			} else if (def_state == IN_IFNDEF) {
				fputs(else_defined, ofp);
				def_state = IN_ELSE;
			}
		}
		while (new <= pat_end && pch_char(new) == '+') {
			fputs(pfetch(new), ofp);
			new++;
		}
	}
	if (R_do_defines && def_state != OUTSIDE) {
		fputs(end_defined, ofp);
	}
}

/* Open the new file. */

static void
init_output(name)
	char *name;
{
	ofp = fopen(name, "w");
	if (ofp == Nullfp)
		pfatal(_("can't create %s.\n"), name);
}

/* Open a file to put hunks we can't locate. */

static void
init_reject(name)
	char *name;
{
	rejfp = fopen(name, "w");
	if (rejfp == Nullfp)
		pfatal(_("can't create %s.\n"), name);
}

/* Copy input file to output, up to wherever hunk is to be applied. */

static void
copy_till(lastline)
	LINENUM lastline;
{
	LINENUM R_last_frozen_line = last_frozen_line;

	if (R_last_frozen_line > lastline)
		say(_("patch: misordered hunks! output will be garbled.\n"));
	while (R_last_frozen_line < lastline) {
		dump_line(++R_last_frozen_line);
	}
	last_frozen_line = R_last_frozen_line;
}

/* Finish copying the input file to the output file. */

static void
spew_output()
{
#ifdef DEBUGGING
	if (debug & 256)
		say("il=%lld lfl=%lld\n",
			(Llong)input_lines, (Llong)last_frozen_line);
#endif
	if (input_lines)
		copy_till(input_lines);		/* dump remainder of file */
	fflush(ofp);
	file_stat.st_size = -1;
	fstat(fileno(ofp), &file_stat);
	Fclose(ofp);
	ofp = Nullfp;
}

/* Copy one line from input to output. */

static void
dump_line(line)
	LINENUM line;
{
	char *s;
	char R_newline = '\n';

	/* Note: string is not null terminated. */
	for (s = ifetch(line, 0); putc(*s, ofp) != R_newline; s++) {
		;
		/* LINTED */
	}
}

/* Does the patch pattern match at line base+offset? */

static bool
patch_match(base, offset, fuzz)
	LINENUM base;
	LINENUM offset;
	LINENUM fuzz;
{
	LINENUM pline = 1 + fuzz;
	LINENUM iline;
	LINENUM pat_lines = pch_ptrn_lines() - fuzz;

	/*
	 * If we never enter the for() loop, we cannot do any match attepmpt
	 * and thus must assume a non-match.
	 */
	if (pline > pat_lines)
		return (FALSE);

	for (iline = base+offset+fuzz; pline <= pat_lines; pline++, iline++) {
		if (canonicalize) {
			if (!similar(ifetch(iline, (offset >= 0)),
			    pfetch(pline),
			    pch_line_len(pline)))
				return (FALSE);
		} else if (strnNE(ifetch(iline, (offset >= 0)),
		    pfetch(pline),
		    pch_line_len(pline))) {
			return (FALSE);
		}
	}
	return (TRUE);
}

/* Do two lines match with canonicalized white space? */

static bool
similar(a, b, len)
	char *a;
	char *b;
	int len;
{
	while (len) {
		if (isspace(UCH *b)) {		/* whitespace or \n to match? */
			if (!isspace(UCH *a))	/* no correspond. whitespace? */
				return (FALSE);
			while (len && isspace(UCH *b) && *b != '\n')
				b++, len--;	/* skip pattern whitespace */
			while (isspace(UCH *a) && *a != '\n')
				a++;		/* skip target whitespace */
			if (*a == '\n' || *b == '\n')
				return (*a == *b); /* should end in sync */
		} else if (*a++ != *b++) {	/* match non-whitespace chars */
			return (FALSE);
		} else {
			len--;			/* probably not necessary */
		}
	}
	return (TRUE);				/* this is not reached */
						/* since there is always a \n */
}

/* Exit with cleanup. */

void
my_exit(status)
	int status;
{
	Unlink(TMPINNAME);
	if (!toutkeep) {
		Unlink(TMPOUTNAME);
	}
	if (!trejkeep) {
		Unlink(TMPREJNAME);
	}
	Unlink(TMPPATNAME);
	exit(status);
}
