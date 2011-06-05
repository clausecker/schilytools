/* @(#)pch.c	1.16 11/05/22 2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)pch.c	1.16 11/05/22 2011 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-1988 Larry Wall
 *	Copyright (c) 1990 Wayne Davison
 *	Copyright (c) 2011 J. Schilling
 *
 *	This program may be copied as long as you don't try to make any
 *	money off of it, or pretend that you wrote it.
 */

#define	EXT	extern
#include "common.h"
#include "util.h"
#undef	EXT
#define	EXT
#include "pch.h"

/* Patch (diff listing) abstract type. */

static FILE *pfp;			/* patch file pointer */
static off_t	p_filesize;		/* size of the patch file */
static LINENUM p_first;			/* 1st line number */
static LINENUM p_newfirst;		/* 1st line number of replacement */
static LINENUM p_ptrn_lines;		/* # lines in pattern */
static LINENUM p_repl_lines;		/* # lines in replacement text */
static LINENUM p_end = -1;		/* last line in hunk */
static LINENUM p_max;			/* max allowed value of p_end */
static LINENUM p_context = 3;		/* # of context lines */
static LINENUM p_input_line = 0;	/* current line # from patch file */
static char **p_line = Null(char **);	/* the text of the hunk */
static short *p_len = Null(short *);	/* length of each line */
static char *p_char = Nullch;		/* +, -, and ! */
static LINENUM hunkmax = INITHUNKMAX;	/* size of above arrays to begin with */
static size_t p_indent;			/* indent to patch */
static LINENUM p_base;			/* where to intuit this time */
static LINENUM p_bline;			/* line # of p_base */
static LINENUM p_start;			/* where intuit found a patch */
static LINENUM p_sline;			/* and the line number for it */
static LINENUM p_hunk_beg;		/* line number of current hunk */
static LINENUM p_efake = -1;		/* end of faked up lines--don't free */
static LINENUM p_bfake = -1;		/* beg of faked up lines */

static	void	grow_hunkmax __PR((void));
static	int	intuit_diff_type __PR((void));
static	void	next_intuit_at __PR((LINENUM file_pos, LINENUM file_line));
static	void	skip_to __PR((off_t file_pos, off_t file_line));
static	void	malformed __PR((void));
static	char	*pgets __PR((char *bf, int sz, FILE *fp));

/* Prepare to look for the next patch in the patch file. */

void
re_patch()
{
	p_first = Nulline;
	p_newfirst = Nulline;
	p_ptrn_lines = Nulline;
	p_repl_lines = Nulline;
	p_end = (LINENUM)-1;
	p_max = Nulline;
	p_indent = 0;
}

/* Open the patch file at the beginning of time. */

void
open_patch_file(filename)
	char *filename;
{
	if (filename == Nullch || !*filename || strEQ(filename, "-")) {
		pfp = fopen(TMPPATNAME, "w");
		if (pfp == Nullfp)
			fatal(_("patch: can't create %s.\n"), TMPPATNAME);
		while (fgets(buf, sizeof (buf), stdin) != Nullch)
			fputs(buf, pfp);
		Fclose(pfp);
		filename = TMPPATNAME;
	}
	pfp = fopen(filename, "r");
	if (pfp == Nullfp)
		fatal(_("patch file %s not found\n"), filename);
	Fstat(fileno(pfp), &file_stat);
	p_filesize = file_stat.st_size;
	next_intuit_at((LINENUM)0, (LINENUM)1);	/* start at the beginning */
	set_hunkmax();
}

/* Make sure our dynamically realloced tables are malloced to begin with. */

void
set_hunkmax()
{
	if (p_line == Null(char **))
		p_line = (char **) malloc(hunkmax * sizeof (char *));
	if (p_len == Null(short *))
		p_len  = (short *) malloc(hunkmax * sizeof (short));
	if (p_char == Nullch)
		p_char = (char *)  malloc(hunkmax * sizeof (char));
}

/* Enlarge the arrays containing the current hunk of patch. */

static void
grow_hunkmax()
{
	hunkmax *= 2;
	/*
	 * Note that on most systems, only the p_line array ever gets fresh
	 * memory since p_len can move into p_line's old space, and p_char can
	 * move into p_len's old space.  Not on PDP-11's however.  But it
	 * doesn't matter.
	 */
	assert(p_line != Null(char **) &&
	    p_len != Null(short *) &&
	    p_char != Nullch);
	p_line = (char **) realloc((char *)p_line, hunkmax * sizeof (char *));
	p_len  = (short *) realloc((char *)p_len,  hunkmax * sizeof (short));
	p_char = (char *)  realloc((char *)p_char, hunkmax * sizeof (char));
	if (p_line != Null(char **) &&
	    p_len != Null(short *) &&
	    p_char != Nullch) {
		return;
	}
	if (!using_plan_a)
		fatal(_("patch: out of memory (grow_hunkmax)\n"));
	out_of_mem = TRUE;	/* whatever is null will be allocated again */
				/* from within plan_a(), of all places */
}

/* True if the remainder of the patch file contains a diff of some sort. */

bool
there_is_another_patch()
{
	if (p_base != 0L && p_base >= p_filesize) {
		if (verbose)
			say(_("done\n"));
		return (FALSE);
	}
	if (verbose)
		say(_("Hmm..."));
	diff_type = intuit_diff_type();
	if (!diff_type) {
		if (p_base != 0L) {
			if (verbose)
				say(_("  Ignoring the trailing garbage.\ndone\n"));
		} else {
		    say(_("  I can't seem to find a patch in there anywhere.\n"));
		}
		return (FALSE);
	}
	if (verbose)
		say(p_base == 0L ?
		    _("  Looks like %s to me...\n") :
		    _("  The next patch looks like %s to me...\n"),
		    diff_type == UNI_DIFF ? "a unified diff" :
		    diff_type == CONTEXT_DIFF ? "a context diff" :
		    diff_type == NEW_CONTEXT_DIFF ? "a new-style context diff" :
		    diff_type == NORMAL_DIFF ? "a normal diff" :
		    "an ed script");
	if (p_indent && verbose) {
		if (p_indent == 1)
			say("(Patch is indented 1 space.)\n");
		else
			say("(Patch is indented %lld spaces.)\n",
			(Llong)p_indent);
	}
	skip_to(p_start, p_sline);
	while (filearg[0] == Nullch) {
		if (force) {
			say(_("No file to patch.  Skipping...\n"));
			filearg[0] = savestr(bestguess);
			return (TRUE);
		}
		ask(_("File to patch: "));
		if (*buf != '\n') {
			if (bestguess)
				free(bestguess);
			bestguess = savestr(buf);
			filearg[0] = fetchname(buf, 0, FALSE, NULL);
		}
		if (filearg[0] == Nullch) {
			ask(_("No file found--skip this patch? [n] "));
			if (*buf != 'y') {
				continue;
			}
			if (verbose)
				say(_("Skipping patch...\n"));
			filearg[0] = fetchname(bestguess, 0, TRUE, NULL);
			skip_rest_of_patch = TRUE;
			return (TRUE);
		}
	}
	return (TRUE);
}

/* Determine what kind of diff is in the remaining part of the patch file. */

static int
intuit_diff_type()
{
	off_t this_line = 0;
	off_t previous_line;
	off_t first_command_line = (off_t)-1;
	LINENUM fcl_line = 0;
	bool last_line_was_command = FALSE;
	bool this_is_a_command = FALSE;
	bool stars_last_line = FALSE;
	bool stars_this_line = FALSE;
	size_t indent;
	char *s;
	char *t;
	char *indtmp = Nullch;
	char *oldtmp = Nullch;
	char *newtmp = Nullch;
	char *indname = Nullch;
	char *oldname = Nullch;
	char *newname = Nullch;
	int retval;
	bool no_filearg = (filearg[0] == Nullch);

	is_null_time[0] = is_null_time[1] = FALSE;
	ok_to_create_file = FALSE;
	Fseek(pfp, p_base, 0);
	p_input_line = p_bline - 1;
	for (;;) {
		previous_line = this_line;
		last_line_was_command = this_is_a_command;
		stars_last_line = stars_this_line;
		this_line = ftell(pfp);
		indent = 0;
		p_input_line++;
		if (fgets(buf, sizeof (buf), pfp) == Nullch) {
			if (first_command_line >= 0L) {
				/* nothing but deletes!? */
				p_start = first_command_line;
				p_sline = fcl_line;
				retval = ED_DIFF;
				goto scan_exit;
			} else {
				p_start = this_line;
				p_sline = p_input_line;
				retval = 0;
				goto scan_exit;
			}
		}
		for (s = buf; *s == ' ' || *s == '\t' || *s == 'X'; s++) {
			if (*s == '\t')
				indent += 8 - (indent % 8);
			else
				indent++;
		}
		for (t = s; isdigit(*t) || *t == ','; t++) {
			;
			/* LINTED */
		}
		this_is_a_command = (isdigit(*s) &&
		    (*t == 'd' || *t == 'c' || *t == 'a'));
		if (first_command_line < 0L && this_is_a_command) {
		first_command_line = this_line;
		fcl_line = p_input_line;
		p_indent = indent;		/* assume this for now */
		}
		if (!stars_last_line && strnEQ(s, "*** ", 4)) {
			oldtmp = savestr(s+4);
		} else if (strnEQ(s, "--- ", 4)) {
			newtmp = savestr(s+4);
		} else if (strnEQ(s, "+++ ", 4)) {
			oldtmp = savestr(s+4);	/* pretend it is the old name */
		} else if (strnEQ(s, "Index:", 6)) {
			indtmp = savestr(s+6);
		} else if (wall_plus &&
			    strnEQ(s, "Prereq:", 7)) {
			for (t = s+7; isspace(*t); t++) {
				;
				/* LINTED */
			}
			revision = savestr(t);
			for (t = revision; *t && !isspace(*t); t++) {
				;
				/* LINTED */
			}
			*t = '\0';
			if (!*revision) {
				free(revision);
				revision = Nullch;
			}
		}
		if ((!diff_type || diff_type == ED_DIFF) &&
		    first_command_line >= 0L &&
		    strEQ(s, ".\n")) {
			p_indent = indent;
			p_start = first_command_line;
			p_sline = fcl_line;
			retval = ED_DIFF;
			goto scan_exit;
		}
		if ((!diff_type || diff_type == UNI_DIFF) &&
		    strnEQ(s, "@@ -", 4)) {
			if (!atol(s+3))
				ok_to_create_file = TRUE;
			p_indent = indent;
			p_start = this_line;
			p_sline = p_input_line;
			retval = UNI_DIFF;
			goto scan_exit;
		}
		stars_this_line = strnEQ(s, "********", 8);
		if ((!diff_type || diff_type == CONTEXT_DIFF) &&
		    stars_last_line &&
		    strnEQ(s, "*** ", 4)) {
			if (!atol(s+4))
				ok_to_create_file = TRUE;
			/* if this is a new context diff the character just */
			/* before the newline is a '*'. */
			while (*s != '\n')
				s++;
			p_indent = indent;
			p_start = previous_line;
			p_sline = p_input_line - 1;
			retval = (*(s-1) == '*' ?
					NEW_CONTEXT_DIFF : CONTEXT_DIFF);
			goto scan_exit;
		}
		if ((!diff_type || diff_type == NORMAL_DIFF) &&
		    last_line_was_command &&
		    (strnEQ(s, "< ", 2) || strnEQ(s, "> ", 2))) {
			p_start = previous_line;
			p_sline = p_input_line - 1;
			p_indent = indent;
			retval = NORMAL_DIFF;
			goto scan_exit;
		}
	}

	scan_exit:

	if (no_filearg) {
		if (indtmp != Nullch)
			indname = fetchname(indtmp, strippath,
						ok_to_create_file, NULL);
		if (oldtmp != Nullch)
			oldname = fetchname(oldtmp, strippath,
						ok_to_create_file,
						&is_null_time[0]);
		if (newtmp != Nullch)
			newname = fetchname(newtmp, strippath,
						ok_to_create_file,
						&is_null_time[1]);
		if (oldname && newname) {
			if (strlen(oldname) < strlen(newname))
				filearg[0] = savestr(oldname);
			else
				filearg[0] = savestr(newname);
		} else if (oldname) {
			filearg[0] = savestr(oldname);
		} else if (newname) {
			filearg[0] = savestr(newname);
		} else if (indname) {
			filearg[0] = savestr(indname);
		}
	}
	if (bestguess) {
		free(bestguess);
		bestguess = Nullch;
	}
	if (filearg[0] != Nullch)
		bestguess = savestr(filearg[0]);
	else if (indtmp != Nullch)
		bestguess = fetchname(indtmp, strippath, TRUE, NULL);
	else {
		if (oldtmp != Nullch)
			oldname = fetchname(oldtmp, strippath, TRUE, NULL);
		if (newtmp != Nullch)
			newname = fetchname(newtmp, strippath, TRUE, NULL);
		if (oldname && newname) {
			if (strlen(oldname) < strlen(newname))
				bestguess = savestr(oldname);
			else
				bestguess = savestr(newname);
		} else if (oldname) {
			bestguess = savestr(oldname);
		} else if (newname) {
			bestguess = savestr(newname);
		}
	}
	if (indtmp != Nullch)
		free(indtmp);
	if (oldtmp != Nullch)
		free(oldtmp);
	if (newtmp != Nullch)
		free(newtmp);
	if (indname != Nullch)
		free(indname);
	if (oldname != Nullch)
		free(oldname);
	if (newname != Nullch)
		free(newname);
	return (retval);
}

/* Remember where this patch ends so we know where to start up again. */

static void
next_intuit_at(file_pos, file_line)
	LINENUM	file_pos;
	LINENUM	file_line;
{
	p_base = file_pos;
	p_bline = file_line;
}

/* Basically a verbose fseek() to the actual diff listing. */

static void
skip_to(file_pos, file_line)
	off_t file_pos;
	off_t file_line;
{
	char *ret;

	assert(p_base <= file_pos);
	if (verbose && p_base < file_pos) {
		Fseek(pfp, p_base, 0);
		say(
_("The text leading up to this was:\n--------------------------\n"));
		while (ftell(pfp) < file_pos) {
			ret = fgets(buf, sizeof (buf), pfp);
			assert(ret != Nullch);
			say("|%s", buf);
		}
		say("--------------------------\n");
	} else {
		Fseek(pfp, file_pos, 0);
	}
	p_input_line = file_line - 1;
}

/* Make this a function for better debugging.  */
static void
malformed()
{
	fatal(_("Malformed patch at line %lld: %s"), (Llong)p_input_line, buf);
		/* about as informative as "Syntax error" in C */
}

/* True if there is more of the current diff listing to process. */

bool
another_hunk()
{
	char	*s;
	char	*ret;
	LINENUM	context = 0;

	while (p_end >= 0) {
		if (p_end == p_efake)
			p_end = p_bfake;	/* don't free twice */
		else
			free(p_line[p_end]);
		p_end--;
	}
	assert(p_end == -1);
	p_efake = -1;

	p_max = hunkmax;		/* gets reduced when --- found */
	if (diff_type == CONTEXT_DIFF || diff_type == NEW_CONTEXT_DIFF) {
		off_t	line_beginning = ftell(pfp);
					/* file pos of the current line */
		LINENUM repl_beginning = 0; /* index of --- line */
		LINENUM fillcnt = 0;	/* #lines of missing ptrn or repl */
		LINENUM fillsrc;	/* index of first line to copy */
		LINENUM filldst;	/* index of first missing line */
		bool ptrn_spaces_eaten = FALSE;	/* ptrn w. slightly misformed */
		bool repl_could_be_missing = TRUE;
					/* no + or ! lines in this hunk */
		bool repl_missing = FALSE; /* we are now backtracking */
		off_t repl_backtrack_position = 0;
					/* file pos of first repl line */
		LINENUM repl_patch_line; /* input line number for same */
		LINENUM ptrn_copiable = 0;
					/* # of copiable lines in ptrn */

		fillsrc = filldst = repl_patch_line = 0; /* Make gcc quiet */

		ret = pgets(buf, sizeof (buf), pfp);
		p_input_line++;
		if (ret == Nullch || strnNE(buf, "********", 8)) {
			next_intuit_at(line_beginning, p_input_line);
			return (FALSE);
		}
		p_context = 100;
		p_hunk_beg = p_input_line + 1;
		while (p_end < p_max) {
			line_beginning = ftell(pfp);
			ret = pgets(buf, sizeof (buf), pfp);
			p_input_line++;
			if (ret == Nullch) {
				if (p_max - p_end < 4)
					Strcpy(buf, "  \n");  /* assume blank lines got chopped */
				else {
					if (repl_beginning &&
					    repl_could_be_missing) {
						repl_missing = TRUE;
						goto hunk_done;
					}
					fatal(
_("Unexpected end of file in patch.\n"));
				}
			}
			p_end++;
			assert(p_end < hunkmax);
			p_char[p_end] = *buf;
#ifdef zilog
			p_line[(short)p_end] = Nullch;
#else
			p_line[p_end] = Nullch;
#endif
			switch (*buf) {

			case '*':
				if (strnEQ(buf, "********", 8)) {
					if (repl_beginning &&
					    repl_could_be_missing) {
						repl_missing = TRUE;
						goto hunk_done;
					} else {
						fatal(
_("Unexpected end of hunk at line %lld.\n"),
						    (Llong)p_input_line);
					}
				}
				if (p_end != 0) {
					if (repl_beginning &&
					    repl_could_be_missing) {
						repl_missing = TRUE;
						goto hunk_done;
					}
					fatal(_("Unexpected *** at line %lld: %s"),
						(Llong)p_input_line, buf);
				}
				context = 0;
				p_line[p_end] = savestr(buf);
				if (out_of_mem) {
					p_end--;
					return (FALSE);
				}
				for (s = buf; *s && !isdigit(*s); s++) {
					;
					/* LINTED */
				}
				if (!*s)
					malformed();
				if (strnEQ(s, "0,0", 3))
					strcpy(s, s+2);
				p_first = (LINENUM) atol(s);
				while (isdigit(*s))
					s++;
				if (*s == ',') {
					for (; *s && !isdigit(*s); s++) {
						;
						/* LINTED */
					}
					if (!*s)
						malformed();
					p_ptrn_lines = ((LINENUM)atol(s)) -
								p_first + 1;
				} else if (p_first) {
					p_ptrn_lines = 1;
				} else {
					p_ptrn_lines = 0;
					p_first = 1;
				}
				p_max = p_ptrn_lines + 6;	/* we need this much at least */
				while (p_max >= hunkmax)
					grow_hunkmax();
				p_max = hunkmax;
				break;
			case '-':
				if (buf[1] == '-') {
					if (repl_beginning ||
					    (p_end != p_ptrn_lines + 1 +
					    (p_char[p_end-1] == '\n'))) {
						if (p_end == 1) {
							/* `old' lines were omitted - set up to fill */
							/* them in from 'new' context lines. */
							p_end = p_ptrn_lines + 1;
							fillsrc = p_end + 1;
							filldst = 1;
							fillcnt = p_ptrn_lines;
						} else {
							if (repl_beginning) {
								if (repl_could_be_missing) {
									repl_missing = TRUE;
									goto hunk_done;
								}
								fatal(
_("Duplicate \"---\" at line %lld--check line numbers at line %lld.\n"),
								    (Llong)p_input_line,
								    (Llong)(p_hunk_beg + repl_beginning));
							} else {
								fatal(
_("%s \"---\" at line %lld--check line numbers at line %lld.\n"),
								    (p_end <= p_ptrn_lines
								    ? _("Premature")
								    : _("Overdue")),
								    (Llong)p_input_line,
								    (Llong)p_hunk_beg);
							}
						}
					}
					repl_beginning = p_end;
					repl_backtrack_position = ftell(pfp);
					repl_patch_line = p_input_line;
					p_line[p_end] = savestr(buf);
					if (out_of_mem) {
						p_end--;
						return (FALSE);
					}
					p_char[p_end] = '=';
					for (s = buf; *s && !isdigit(*s); s++) {
						;
						/* LINTED */
					}
					if (!*s)
						malformed();
					p_newfirst = (LINENUM) atol(s);
					while (isdigit(*s))
						s++;
					if (*s == ',') {
						for (; *s && !isdigit(*s); s++) {
							;
							/* LINTED */
						}
						if (!*s)
							malformed();
						p_repl_lines = ((LINENUM)atol(s)) - p_newfirst + 1;
					} else if (p_newfirst) {
						p_repl_lines = 1;
					} else {
						p_repl_lines = 0;
						p_newfirst = 1;
					}
					p_max = p_repl_lines + p_end;
					if (p_max > MAXHUNKSIZE)
						fatal(
_("Hunk too large (%lld lines) at line %lld: %s"),
						    (Llong)p_max,
						    (Llong)p_input_line, buf);
					while (p_max >= hunkmax)
						grow_hunkmax();
					if (p_repl_lines != ptrn_copiable &&
					    (p_context != 0 || p_repl_lines != 1))
						repl_could_be_missing = FALSE;
					break;
				}
				goto change_line;
			case '+':
			case '!':
				repl_could_be_missing = FALSE;
			change_line:
				if (buf[1] == '\n' && canonicalize)
					strcpy(buf+1, " \n");
				if (!isspace(buf[1]) && buf[1] != '>' && buf[1] != '<' &&
				    repl_beginning && repl_could_be_missing) {
					repl_missing = TRUE;
					goto hunk_done;
				}
				if (context >= 0) {
					if (context < p_context)
						p_context = context;
					context = -1000;
				}
				p_line[p_end] = savestr(buf+2);
				if (out_of_mem) {
					p_end--;
					return (FALSE);
				}
				break;
			case '\t':
			case '\n':	/* assume the 2 spaces got eaten */
				if (repl_beginning && repl_could_be_missing &&
				    (!ptrn_spaces_eaten || diff_type == NEW_CONTEXT_DIFF)) {
					repl_missing = TRUE;
					goto hunk_done;
				}
				p_line[p_end] = savestr(buf);
				if (out_of_mem) {
					p_end--;
					return (FALSE);
				}
				if (p_end != p_ptrn_lines + 1) {
					ptrn_spaces_eaten |= (repl_beginning != 0);
					context++;
					if (!repl_beginning)
						ptrn_copiable++;
					p_char[p_end] = ' ';
				}
				break;
			case ' ':
				if (!isspace(buf[1]) &&
				    repl_beginning && repl_could_be_missing) {
					repl_missing = TRUE;
					goto hunk_done;
				}
				context++;
				if (!repl_beginning)
					ptrn_copiable++;
				p_line[p_end] = savestr(buf+2);
				if (out_of_mem) {
					p_end--;
					return (FALSE);
				}
				break;
			default:
				if (repl_beginning && repl_could_be_missing) {
					repl_missing = TRUE;
					goto hunk_done;
				}
				malformed();
			}
			/* set up p_len for strncmp() so we don't have to */
			/* assume null termination */
			if (p_line[p_end])
				p_len[p_end] = strlen(p_line[p_end]);
			else
				p_len[p_end] = 0;
		}

	hunk_done:
		if (p_end >= 0 && !repl_beginning)
			fatal(_("No --- found in patch at line %lld\n"),
				(Llong)pch_hunk_beg());

		if (repl_missing) {
			/* reset state back to just after --- */
			p_input_line = repl_patch_line;
			for (p_end--; p_end > repl_beginning; p_end--)
				free(p_line[p_end]);
			Fseek(pfp, repl_backtrack_position, 0);

			/* redundant 'new' context lines were omitted - set */
			/* up to fill them in from the old file context */
			if (!p_context && p_repl_lines == 1) {
				p_repl_lines = 0;
				p_max--;
			}
			fillsrc = 1;
			filldst = repl_beginning+1;
			fillcnt = p_repl_lines;
			p_end = p_max;
		} else if (!p_context && fillcnt == 1) {
			/* the first hunk was a null hunk with no context */
			/* and we were expecting one line -- fix it up. */
			while (filldst < p_end) {
				p_line[filldst] = p_line[filldst+1];
				p_char[filldst] = p_char[filldst+1];
				p_len[filldst] = p_len[filldst+1];
				filldst++;
			}
#if 0
			repl_beginning--; /* this doesn't need to be fixed */
#endif
			p_end--;
			p_first++;	/* do append rather than insert */
			fillcnt = 0;
			p_ptrn_lines = 0;
		}

		if (diff_type == CONTEXT_DIFF &&
		    (fillcnt || (p_first > 1 && ptrn_copiable > 2*p_context))) {
			if (verbose)
				say("%s\n%s\n%s\n",
_("(Fascinating--this is really a new-style context diff but without"),
_("the telltale extra asterisks on the *** line that usually indicate"),
_("the new style...)"));
			diff_type = NEW_CONTEXT_DIFF;
		}

		/* if there were omitted context lines, fill them in now */
		if (fillcnt) {
			p_bfake = filldst;		/* remember where not to free() */
			p_efake = filldst + fillcnt - 1;
			while (fillcnt-- > 0) {
				while (fillsrc <= p_end && p_char[fillsrc] != ' ')
					fillsrc++;
				if (fillsrc > p_end)
					fatal(
_("Replacement text or line numbers mangled in hunk at line %lld\n"),
						(Llong)p_hunk_beg);
				p_line[filldst] = p_line[fillsrc];
				p_char[filldst] = p_char[fillsrc];
				p_len[filldst] = p_len[fillsrc];
				fillsrc++; filldst++;
			}
			while (fillsrc <= p_end && fillsrc != repl_beginning &&
			    p_char[fillsrc] != ' ')
				fillsrc++;
#ifdef DEBUGGING
			if (debug & 64)
				printf("fillsrc %lld, filldst %lld, rb %lld, e+1 %lld\n",
				    (Llong)fillsrc, (Llong)filldst,
				    (Llong)repl_beginning, (Llong)p_end+1);
#endif
			assert(fillsrc == p_end+1 || fillsrc == repl_beginning);
			assert(filldst == p_end+1 || filldst == repl_beginning);
		}
	} else if (diff_type == UNI_DIFF) {
		off_t	line_beginning = ftell(pfp);
						/* file pos of the current line */
		LINENUM fillsrc;		/* index of old lines */
		LINENUM filldst;		/* index of new lines */
		char ch;

		ret = pgets(buf, sizeof (buf), pfp);
		p_input_line++;
		if (ret == Nullch || strnNE(buf, "@@ -", 4)) {
			next_intuit_at(line_beginning, p_input_line);
			return (FALSE);
		}
		s = buf+4;
		if (!*s)
			malformed();
		p_first = (LINENUM) atol(s);
		while (isdigit(*s))
			s++;
		if (*s == ',') {
			p_ptrn_lines = (LINENUM) atol(++s);
			while (isdigit(*s))
				s++;
		} else {
			p_ptrn_lines = 1;
		}
		if (*s == ' ')
			s++;
		if (*s != '+' || !*++s)
			malformed();
		p_newfirst = (LINENUM) atol(s);
		while (isdigit(*s))
			s++;
		if (*s == ',') {
			p_repl_lines = (LINENUM) atol(++s);
			while (isdigit(*s)) s++;
		} else {
			p_repl_lines = 1;
		}
		if (*s == ' ')
			s++;
		if (*s != '@')
			malformed();
		if (!p_ptrn_lines)
			p_first++;	/* do append rather than insert */
		p_max = p_ptrn_lines + p_repl_lines + 1;
		while (p_max >= hunkmax)
			grow_hunkmax();
		fillsrc = 1;
		filldst = fillsrc + p_ptrn_lines;
		p_end = filldst + p_repl_lines;
		Sprintf(buf, "*** %lld,%lld ****\n",
			(Llong)p_first, (Llong)(p_first + p_ptrn_lines - 1));
		p_line[0] = savestr(buf);
		if (out_of_mem) {
			p_end = -1;
			return (FALSE);
		}
		p_char[0] = '*';
		Sprintf(buf, "--- %lld,%lld ----\n",
			(Llong)p_newfirst, (Llong)(p_newfirst+p_repl_lines-1));
		p_line[filldst] = savestr(buf);
		if (out_of_mem) {
			p_end = 0;
			return (FALSE);
		}
		p_char[filldst++] = '=';
		p_context = 100;
		context = 0;
		p_hunk_beg = p_input_line + 1;
		while (fillsrc <= p_ptrn_lines || filldst <= p_end) {
			line_beginning = ftell(pfp);
			ret = pgets(buf, sizeof (buf), pfp);
			p_input_line++;
			if (ret == Nullch) {
				if (p_max - filldst < 3) {
					Strcpy(buf, " \n");  /* assume blank lines got chopped */
				} else {
					fatal(
_("Unexpected end of file in patch.\n"));
				}
			}
			if (*buf == '\t' || *buf == '\n') {
				ch = ' ';	/* assume the space got eaten */
				s = savestr(buf);
			} else {
				ch = *buf;
				s = savestr(buf+1);
			}
			if (out_of_mem) {
				while (--filldst > p_ptrn_lines)
					free(p_line[filldst]);
				p_end = fillsrc-1;
				return (FALSE);
			}
			switch (ch) {
			case '-':
				if (fillsrc > p_ptrn_lines) {
					free(s);
					p_end = filldst-1;
					malformed();
				}
				p_char[fillsrc] = ch;
				p_line[fillsrc] = s;
				p_len[fillsrc++] = strlen(s);
				break;
			case '=':
				ch = ' ';
				/* FALLTHROUGH */
			case ' ':
				if (fillsrc > p_ptrn_lines) {
					free(s);
					while (--filldst > p_ptrn_lines)
						free(p_line[filldst]);
					p_end = fillsrc-1;
					malformed();
				}
				context++;
				p_char[fillsrc] = ch;
				p_line[fillsrc] = s;
				p_len[fillsrc++] = strlen(s);
				s = savestr(s);
				if (out_of_mem) {
					while (--filldst > p_ptrn_lines)
						free(p_line[filldst]);
					p_end = fillsrc-1;
					return (FALSE);
				}
				/* FALLTHROUGH */
			case '+':
				if (filldst > p_end) {
					free(s);
					while (--filldst > p_ptrn_lines)
						free(p_line[filldst]);
					p_end = fillsrc-1;
					malformed();
				}
				p_char[filldst] = ch;
				p_line[filldst] = s;
				p_len[filldst++] = strlen(s);
				break;
			default:
				p_end = filldst;
				malformed();
			}
			if (ch != ' ' && context > 0) {
				if (context < p_context)
					p_context = context;
				context = -1000;
			}
		} /* while */
	} else {			/* normal diff--fake it up */
		char hunk_type;
		int i;
#undef	min
#undef	max
		LINENUM	min, max;
		off_t	line_beginning = ftell(pfp);

		p_context = 0;
		ret = pgets(buf, sizeof (buf), pfp);
		p_input_line++;
		if (ret == Nullch || !isdigit(*buf)) {
			next_intuit_at(line_beginning, p_input_line);
			return (FALSE);
		}
		p_first = (LINENUM)atol(buf);
		for (s = buf; isdigit(*s); s++) {
			;
			/* LINTED */
		}
		if (*s == ',') {
			p_ptrn_lines = (LINENUM)atol(++s) - p_first + 1;
			while (isdigit(*s))
				s++;
		} else {
			p_ptrn_lines = (*s != 'a');
		}
		hunk_type = *s;
		if (hunk_type == 'a')
			p_first++;	/* do append rather than insert */
		min = (LINENUM)atol(++s);
		for (; isdigit(*s); s++) {
			;
			/* LINTED */
		}
		if (*s == ',')
			max = (LINENUM)atol(++s);
		else
			max = min;
		if (hunk_type == 'd')
			min++;
		p_end = p_ptrn_lines + 1 + max - min + 1;
		if (p_end > MAXHUNKSIZE)
			fatal(_("Hunk too large (%lld lines) at line %lld: %s"),
			    (Llong)p_end, (Llong)p_input_line, buf);
		while (p_end >= hunkmax)
			grow_hunkmax();
		p_newfirst = min;
		p_repl_lines = max - min + 1;
		Sprintf(buf, "*** %lld,%lld\n",
			(Llong)p_first, (Llong)(p_first + p_ptrn_lines - 1));
		p_line[0] = savestr(buf);
		if (out_of_mem) {
			p_end = -1;
			return (FALSE);
		}
		p_char[0] = '*';
		for (i = 1; i <= p_ptrn_lines; i++) {
			ret = pgets(buf, sizeof (buf), pfp);
			p_input_line++;
			if (ret == Nullch)
				fatal(
_("Unexpected end of file in patch at line %lld.\n"),
				    (Llong)p_input_line);
			if (*buf != '<')
				fatal(_("< expected at line %lld of patch.\n"),
					(Llong)p_input_line);
			p_line[i] = savestr(buf+2);
			if (out_of_mem) {
				p_end = i-1;
				return (FALSE);
			}
			p_len[i] = strlen(p_line[i]);
			p_char[i] = '-';
		}
		if (hunk_type == 'c') {
			ret = pgets(buf, sizeof (buf), pfp);
			p_input_line++;
			if (ret == Nullch)
				fatal(
_("Unexpected end of file in patch at line %lld.\n"),
				    (Llong)p_input_line);
			if (*buf != '-')
				fatal(_("--- expected at line %lld of patch.\n"),
				(Llong)p_input_line);
		}
		Sprintf(buf, "--- %lld,%lld\n", (Llong)min, (Llong)max);
		p_line[i] = savestr(buf);
		if (out_of_mem) {
			p_end = i-1;
			return (FALSE);
		}
		p_char[i] = '=';
		for (i++; i <= p_end; i++) {
			ret = pgets(buf, sizeof (buf), pfp);
			p_input_line++;
			if (ret == Nullch)
				fatal(
_("Unexpected end of file in patch at line %lld.\n"),
				    (Llong)p_input_line);
			if (*buf != '>')
				fatal(_("> expected at line %lld of patch.\n"),
					(Llong)p_input_line);
			p_line[i] = savestr(buf+2);
			if (out_of_mem) {
				p_end = i-1;
				return (FALSE);
			}
			p_len[i] = strlen(p_line[i]);
			p_char[i] = '+';
		}
	}
	if (reverse)			/* backwards patch? */
		if (!pch_swap())
		    say(_("Not enough memory to swap next hunk!\n"));
#ifdef DEBUGGING
	if (debug & 2) {
		int i;
		char special;

		for (i = 0; i <= p_end; i++) {
			if (i == p_ptrn_lines)
				special = '^';
			else
				special = ' ';
			fprintf(stderr, "%3d %c %c %s",
					i, p_char[i], special, p_line[i]);
			Fflush(stderr);
		}
	}
#endif
	if (p_end+1 < hunkmax)		/* paranoia reigns supreme... */
		p_char[p_end+1] = '^';	/* add a stopper for apply_hunk */
	return (TRUE);
}

/* Input a line from the patch file, worrying about indentation. */

static char *
pgets(bf, sz, fp)
	char	*bf;
	int	sz;
	FILE	*fp;
{
	char *ret = fgets(bf, sz, fp);
	char *s;
	size_t indent = 0;

	if (p_indent && ret != Nullch) {
		for (s = buf;
		    indent < p_indent && (*s == ' ' || *s == '\t' || *s == 'X');
		    s++) {
			if (*s == '\t')
				indent += 8 - (indent % 7);
			else
				indent++;
		}
		if (buf != s)
			Strcpy(buf, s);
	}
	return (ret);
}

/* Reverse the old and new portions of the current hunk. */

bool
pch_swap()
{
	char **tp_line;		/* the text of the hunk */
	short *tp_len;		/* length of each line */
	char *tp_char;		/* +, -, and ! */
	LINENUM i;
	LINENUM n;
	bool blankline = FALSE;
	char *s;

	i = p_first;
	p_first = p_newfirst;
	p_newfirst = i;

	/* make a scratch copy */

	tp_line = p_line;
	tp_len = p_len;
	tp_char = p_char;
	p_line = Null(char **);	/* force set_hunkmax to allocate again */
	p_len = Null(short *);
	p_char = Nullch;
	set_hunkmax();
	if (p_line == Null(char **) ||
	    p_len == Null(short *) ||
	    p_char == Nullch) {
		if (p_line == Null(char **))
			free((char *)p_line);
		p_line = tp_line;
		if (p_len == Null(short *))
			free((char *)p_len);
		p_len = tp_len;
		if (p_char == Nullch)
			free((char *)p_char);
		p_char = tp_char;
		return (FALSE);		/* not enough memory to swap hunk! */
	}

	/* now turn the new into the old */

	i = p_ptrn_lines + 1;
	if (tp_char[i] == '\n') {	/* account for possible blank line */
		blankline = TRUE;
		i++;
	}
	if (p_efake >= 0) {		/* fix non-freeable ptr range */
		if (p_efake <= i)
			n = p_end - i + 1;
		else
			n = -i;
		p_efake += n;
		p_bfake += n;
	}
	for (n = 0; i <= p_end; i++, n++) {
		p_line[n] = tp_line[i];
		p_char[n] = tp_char[i];
		if (p_char[n] == '+')
			p_char[n] = '-';
		p_len[n] = tp_len[i];
	}
	if (blankline) {
		i = p_ptrn_lines + 1;
		p_line[n] = tp_line[i];
		p_char[n] = tp_char[i];
		p_len[n] = tp_len[i];
		n++;
	}
	assert(p_char[0] == '=');
	p_char[0] = '*';
	for (s = p_line[0]; *s; s++)
		if (*s == '-')
			*s = '*';

	/* now turn the old into the new */

	assert(tp_char[0] == '*');
	tp_char[0] = '=';
	for (s = tp_line[0]; *s; s++)
		if (*s == '*')
			*s = '-';
	for (i = 0; n <= p_end; i++, n++) {
		p_line[n] = tp_line[i];
		p_char[n] = tp_char[i];
		if (p_char[n] == '-')
			p_char[n] = '+';
		p_len[n] = tp_len[i];
	}
	assert(i == p_ptrn_lines + 1);
	i = p_ptrn_lines;
	p_ptrn_lines = p_repl_lines;
	p_repl_lines = i;
	if (tp_line == Null(char **))
		free((char *)tp_line);
	if (tp_len == Null(short *))
		free((char *)tp_len);
	if (tp_char == Nullch)
		free((char *)tp_char);
	return (TRUE);
}

/* Return the specified line position in the old file of the old context. */

LINENUM
pch_first()
{
	return (p_first);
}

/* Return the number of lines of old context. */

LINENUM
pch_ptrn_lines()
{
	return (p_ptrn_lines);
}

/* Return the probable line position in the new file of the first line. */

LINENUM
pch_newfirst()
{
	return (p_newfirst);
}

/* Return the number of lines in the replacement text including context. */

LINENUM
pch_repl_lines()
{
	return (p_repl_lines);
}

/* Return the number of lines in the whole hunk. */

LINENUM
pch_end()
{
	return (p_end);
}

/* Return the number of context lines before the first changed line. */

LINENUM
pch_context()
{
	return (p_context);
}

/* Return the length of a particular patch line. */

short
pch_line_len(line)
LINENUM line;
{
	return (p_len[line]);
}

/* Return the control character (+, -, *, !, etc) for a patch line. */

char
pch_char(line)
LINENUM line;
{
	return (p_char[line]);
}

/* Return a pointer to a particular patch line. */

char *
pfetch(line)
LINENUM line;
{
	return (p_line[line]);
}

/* Return where in the patch file this hunk began, for error messages. */

LINENUM
pch_hunk_beg()
{
	return (p_hunk_beg);
}

/* Apply an ed script by feeding ed itself. */

void
do_ed_script()
{
	char	*t;
	off_t	beginning_of_this_line;
	bool	this_line_is_command = FALSE;
	FILE	*pipefp = 0;

	if (!skip_rest_of_patch) {
		Unlink(TMPOUTNAME);
		copy_file(filearg[0], TMPOUTNAME);
		if (verbose)
			Sprintf(buf, "/bin/ed %s", TMPOUTNAME);
		else
			Sprintf(buf, "/bin/ed - %s", TMPOUTNAME);
		pipefp = popen(buf, "w");
	}
	for (;;) {
		beginning_of_this_line = ftell(pfp);
		if (pgets(buf, sizeof (buf), pfp) == Nullch) {
			next_intuit_at(beginning_of_this_line, p_input_line);
			break;
		}
		p_input_line++;
		for (t = buf; isdigit(*t) || *t == ','; t++) {
			;
			/* LINTED */
		}
		this_line_is_command = (isdigit(*buf) &&
		    (*t == 'd' || *t == 'c' || *t == 'a'));
		if (this_line_is_command) {
			if (!skip_rest_of_patch)
				fputs(buf, pipefp);
			if (*t != 'd') {
				while (pgets(buf, sizeof (buf), pfp) !=
				    Nullch) {
					p_input_line++;
					if (!skip_rest_of_patch)
						fputs(buf, pipefp);
					if (strEQ(buf, ".\n"))
						break;
				}
			}
		} else {
			next_intuit_at(beginning_of_this_line, p_input_line);
			break;
		}
	}
	if (skip_rest_of_patch)
		return;
	fprintf(pipefp, "w\n");
	fprintf(pipefp, "q\n");
	Fflush(pipefp);
	Pclose(pipefp);
	ignore_signals();
	if (move_file(TMPOUTNAME, outname) < 0) {
		toutkeep = TRUE;
		chmod(TMPOUTNAME, filemode);
	} else {
		chmod(outname, filemode);
	}
	set_signals(1);
}
