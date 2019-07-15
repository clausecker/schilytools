/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
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
/*
 * Copyright 2006 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)read.cc 1.64 06/12/12
 */

#pragma	ident	"@(#)read.cc	1.64	06/12/12"

/*
 * This file contains modifications Copyright 2017-2019 J. Schilling
 *
 * @(#)read.cc	1.22 19/07/01 2017-2019 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)read.cc	1.22 19/07/01 2017-2019 J. Schilling";
#endif

/*
 *	read.c
 *
 *	This file contains the makefile reader.
 */

/*
 * Included files
 */
#include <avo/avo_alloca.h>		/* alloca() */
#include <mk/defs.h>
#include <mksh/macro.h>		/* expand_value(), expand_macro() */
#include <mksh/misc.h>		/* getmem() */
#include <mksh/read.h>		/* get_next_block_fn() */
#include <sys/uio.h>		/* read() */

#if defined(TEAMWARE_MAKE_CMN)
#if defined(HP_UX) || defined(linux)
#include <avo/types.h>
extern "C" Avo_err *avo_find_run_dir(char **dirp);
#endif
#endif

#include <schily/stdio.h>
#include <schily/wchar.h>
#include <schily/schily.h>

/*
 * We cannot use "using std::wcsdup" as wcsdup() is not always
 * in the std namespace.
 */
using namespace std;		/* needed for wcsdup() */

/*
 * typedefs & structs
 */

enum directive {
	D_NONE = 0,		/* No valid directive found	*/
	D_INCLUDE,		/* "include" directive found	*/
	D_IINCLUDE,		/* "-include" directive found	*/
	D_EXPORT,		/* "export" directive found	*/
	D_UNEXPORT,		/* "unexport" directive found	*/
	D_READONLY		/* "readonly" directive found	*/
};

/*
 * Static variables
 */

static int line_started_with_space=0; // Used to diagnose spaces instead of tabs

static wchar_t		include_d[8];
static wchar_t		iinclude_d[9];
static wchar_t		export_d[7];
static wchar_t		unexport_d[9];
static wchar_t		readonly_d[9];

static struct dent {
	wchar_t		*directive;
	int		dlen;
	enum directive	dir;
} directives[] = {
	{ include_d,	7, D_INCLUDE },
	{ iinclude_d,	8, D_IINCLUDE },
	{ export_d,	6, D_EXPORT },
	{ unexport_d,	8, D_UNEXPORT },
	{ readonly_d,	8, D_READONLY },
	{ NULL,		0, D_NONE }
};

/*
 * File table of contents
 */
static	void		parse_makefile(register Name true_makefile_name, register Source source);
static	Source		push_macro_value(register Source bp, register wchar_t *buffer, int size, register Source source);
extern  void 		enter_target_groups_and_dependencies(Name_vector target, Name_vector depes, Cmd_line command, Separator separator, Boolean target_group_seen);
extern	Name		normalize_name(register wchar_t *name_string, register int length);
extern	void		doexport(Name name);
extern	void		dounexport(Name name);
extern	void		doreadonly(Name name);

static Boolean		skip_comment(wchar_t * &source_p, wchar_t * &source_end, Source &source);
static void		init_directives(void);

/*
 *	read_simple_file(makefile_name, chase_path, doname_it,
 *		 complain, must_exist, report_file, lock_makefile)
 *
 *	Make the makefile and setup to read it. Actually read it if it is stdio
 *
 *	Return value:
 *				false if the read failed
 *
 *	Parameters:
 *		makefile_name	Name of the file to read
 *		chase_path	Use the makefile path when opening file
 *		doname_it	Call doname() to build the file first
 *		complain	Print message if doname/open fails
 *		must_exist	Generate fatal if file is missing
 *		report_file	Report file when running -P
 *		lock_makefile	Lock the makefile when reading
 *
 *	Static variables used:
 *
 *	Global variables used:
 *		do_not_exec_rule Is -n on?
 *		file_being_read	Set to the name of the new file
 *		line_number	The number of the current makefile line
 *		makefiles_used	A list of all makefiles used, appended to
 */


Boolean
read_simple_file(register Name makefile_name, register Boolean chase_path, register Boolean doname_it, Boolean complain, Boolean must_exist, Boolean report_file, Boolean lock_makefile)
{
	static short		max_include_depth;
	register Property	makefile = maybe_append_prop(makefile_name,
							     makefile_prop);
	Boolean			forget_after_parse = false;
	static pathpt		makefile_path;
	register int		n;
	char			*path;
	register Source		source = ALLOC(Source);
	Property		orig_makefile = makefile;
	Dependency		*dpp;
	Dependency		dp;
	register int		length;
	wchar_t			*previous_file_being_read = file_being_read;
	int			previous_line_number = line_number;
	wchar_t			previous_current_makefile[MAXPATHLEN];
	Makefile_type		save_makefile_type;
	Name 			normalized_makefile_name;
	register wchar_t        *string_start;
	register wchar_t        *string_end;
	char                    *run_dir, makerules_dir[BUFSIZ];


#if defined(TEAMWARE_MAKE_CMN)
#if defined(HP_UX) || defined(linux)
	Avo_err                 *findrundir_err;
#endif
#endif

	wchar_t * wcb = get_wstring(makefile_name->string_mb);

#ifdef NSE
	if (report_file){
		wcscpy(previous_current_makefile, current_makefile);
		wcscpy(current_makefile, wcb);
	}
#endif 
	if (max_include_depth++ >= 40) {
		fatal(gettext("Too many nested include statements"));
	}
	if (makefile->body.makefile.contents != NULL) {
		retmem(makefile->body.makefile.contents);
	}
	source->inp_buf =
	  source->inp_buf_ptr =
	    source->inp_buf_end = NULL;
	source->error_converting = false;
	makefile->body.makefile.contents = NULL;
	makefile->body.makefile.size = 0;
	if ((makefile_name->hash.length != 1) ||
	    (wcb[0] != (int) hyphen_char)) {
		if ((makefile->body.makefile.contents == NULL) &&
		    (doname_it)) {
			if (makefile_path == NULL) {
				add_dir_to_path(".",
						&makefile_path,
						-1);
#if !defined(TEAMWARE_MAKE_CMN)
                		run_dir = make_run_dir;
				if (run_dir) {
					if (strstr(run_dir, "xpg4/bin")) {
	                        		(void) sprintf(makerules_dir,
						NOCATGETS("%s/../../share/lib/make"), run_dir);
					} else {
	                        		(void) sprintf(makerules_dir,
						NOCATGETS("%s/../share/lib/make"), run_dir);
					}
					add_dir_to_path(makerules_dir,
							&makefile_path,
							-1);
				}
#ifdef INS_BASE
				add_dir_to_path(NOCATGETS(INS_BASE "/share/lib/make"),
						&makefile_path,
						-1);
#endif
				add_dir_to_path(NOCATGETS("/usr/share/lib/make"),
						&makefile_path,
						-1);
				add_dir_to_path(NOCATGETS("/etc/default"),
						&makefile_path,
						-1);
#else
#ifdef SUN5_0
				add_dir_to_path(NOCATGETS("/usr/share/lib/make"),
						&makefile_path,
						-1);
				add_dir_to_path(NOCATGETS("/etc/default"),
						&makefile_path,
						-1);
#elif defined(HP_UX)
                		findrundir_err = avo_find_run_dir(&run_dir);
                		if (! findrundir_err) {
                        		(void) sprintf(makerules_dir, NOCATGETS("%s/../share/lib/make"), run_dir);
					add_dir_to_path(makerules_dir,
							&makefile_path,
							-1);
                		}

				add_dir_to_path(NOCATGETS("/opt/SUNWspro/share/lib/make"),
						&makefile_path,
						-1);
				add_dir_to_path(NOCATGETS("/usr/share/lib/make"),
						&makefile_path,
						-1);
#elif defined(linux)
                		findrundir_err = avo_find_run_dir(&run_dir);
                		if (! findrundir_err) {
                        		(void) sprintf(makerules_dir, NOCATGETS("%s/../lib"), run_dir);
					add_dir_to_path(makerules_dir,
							&makefile_path,
							-1);
                		}

				add_dir_to_path(NOCATGETS("/usr/SUNWspro/lib"),
						&makefile_path,
						-1);
				add_dir_to_path(NOCATGETS("/opt/SUNWspro/share/lib/make"),
						&makefile_path,
						-1);
				add_dir_to_path(NOCATGETS("/usr/share/lib/make"),
						&makefile_path,
						-1);
#else
				add_dir_to_path(NOCATGETS("/usr/include/make"),
						&makefile_path,
						-1);
#endif
#endif
			}
			save_makefile_type = makefile_type;
			makefile_type = reading_nothing;
			if (doname(makefile_name, true, false) == build_dont_know) {
				/* Try normalized filename */
				string_start=get_wstring(makefile_name->string_mb);
				for (string_end=string_start+1; *string_end != '\0'; string_end++);
				normalized_makefile_name=normalize_name(string_start, string_end - string_start);
				if ((strcmp(makefile_name->string_mb, normalized_makefile_name->string_mb) == 0) || 
					(doname(normalized_makefile_name, true, false) == build_dont_know)) {
					n = access_vroot(makefile_name->string_mb,
						 4,
						 chase_path ?
						 makefile_path : NULL,
						 VROOT_DEFAULT);
					if (n == 0) {
						get_vroot_path((char **) NULL,
						       &path,
						       (char **) NULL);
						if ((path[0] == (int) period_char) &&
						    (path[1] == (int) slash_char)) {
							path += 2;
						}
						MBSTOWCS(wcs_buffer, path);
						makefile_name = GETNAME(wcs_buffer,
								FIND_LENGTH);
					}
				}
				retmem(string_start);
				/* 
				 * Commented out: retmem_mb(normalized_makefile_name->string_mb);
				 * We have to return this memory, but it seems to trigger a bug
				 * in dmake or in Sun C++ 5.7 compiler (it works ok if this code
				 * is compiled using Sun C++ 5.6).
				 */
				// retmem_mb(normalized_makefile_name->string_mb); 
			}
			makefile_type = save_makefile_type;
		}
		source->string.free_after_use = false;
		source->previous = NULL;
		source->already_expanded = false;
		/* Lock the file for read, but not when -n. */
		if (lock_makefile && 
		    !do_not_exec_rule) {

			 make_state_lockfile = getmem(strlen(make_state->string_mb) + strlen(NOCATGETS(".lock")) + 1);
			 (void) sprintf(make_state_lockfile,
						NOCATGETS("%s.lock"),
						make_state->string_mb);
			(void) file_lock(make_state->string_mb,
					 make_state_lockfile,
					 (int *) &make_state_locked,
					 0);
			if(!make_state_locked) {
				printf(NOCATGETS("-- NO LOCKING for read\n"));
				retmem_mb(make_state_lockfile);
				make_state_lockfile = 0;
				return failed;
			}
		}
		if (makefile->body.makefile.contents == NULL) {
			save_makefile_type = makefile_type;
			makefile_type = reading_nothing;
			if ((doname_it) &&
			    (doname(makefile_name, true, false) == build_failed)) {
				if (complain) {
					(void) fprintf(stderr,
#ifdef DISTRIBUTED
						       gettext("dmake: Couldn't dmake `%s'\n"),
#else
						       gettext("make: Couldn't make `%s'\n"),
#endif
						       makefile_name->string_mb);
				}
				max_include_depth--;
				makefile_type = save_makefile_type;
				return failed;
			}
			makefile_type = save_makefile_type;
			//
			// Before calling exists() make sure that we have the right timestamp
			//
			makefile_name->stat.time = file_no_time;

			if (exists(makefile_name) == file_doesnt_exist) {
				if (complain ||
				    (makefile_name->stat.stat_errno != ENOENT)) {
					if (must_exist) {
						fatal(gettext("Can't find `%s': %s"),
						      makefile_name->string_mb,
						      errmsg(makefile_name->
							     stat.stat_errno));
					} else {
						warning(gettext("Can't find `%s': %s"),
							makefile_name->string_mb,
							errmsg(makefile_name->
							       stat.stat_errno));
					}
				}
				max_include_depth--;
				if(make_state_locked && (make_state_lockfile != NULL)) {
					(void) unlink(make_state_lockfile);
					retmem_mb(make_state_lockfile);
					make_state_lockfile = NULL;
					make_state_locked = false;
				}
				retmem(wcb);
				retmem_mb((char *)source);
				return failed;
			}
			/*
			 * These values are the size and bytes of
			 * the MULTI-BYTE makefile.
			 */
			orig_makefile->body.makefile.size =
			  makefile->body.makefile.size =
			    source->bytes_left_in_file =
			      makefile_name->stat.size;
			if (report_file) {
				for (dpp = &makefiles_used;
				     *dpp != NULL;
				     dpp = &(*dpp)->next);
				dp = ALLOC(Dependency);
				dp->next = NULL;
				dp->name = makefile_name;
				dp->automatic = false;
				dp->stale = false;
				dp->built = false;
				*dpp = dp;
			}
			source->fd = open_vroot(makefile_name->string_mb,
						O_RDONLY,
						0,
						NULL,
						VROOT_DEFAULT);
			if (source->fd < 0) {
				if (complain || (errno != ENOENT)) {
					if (must_exist) {
						fatal(gettext("Can't open `%s': %s"),
						      makefile_name->string_mb,
						      errmsg(errno));
					} else {
						warning(gettext("Can't open `%s': %s"),
							makefile_name->string_mb,
							errmsg(errno));
					}
				}
				max_include_depth--;
				return failed;
			}
			(void) fcntl(source->fd, F_SETFD, 1);
			orig_makefile->body.makefile.contents =
			  makefile->body.makefile.contents =
			    source->string.text.p =
			      source->string.buffer.start =
				ALLOC_WC((int) (makefile_name->stat.size + 2));
			if (makefile_type == reading_cpp_file) {
				forget_after_parse = true;
			}
			source->string.text.end = source->string.text.p;
			source->string.buffer.end =
			  source->string.text.p + makefile_name->stat.size;
		} else {
			/* Do we ever reach here? */
			source->fd = -1;
			source->string.text.p =
			  source->string.buffer.start =
			    makefile->body.makefile.contents;
			source->string.text.end =
			  source->string.buffer.end =
			    source->string.text.p + makefile->body.makefile.size;
			source->bytes_left_in_file =
			  makefile->body.makefile.size;
		}
		file_being_read = wcb;
	} else {
		char		*stdin_text_p;
		char		*stdin_text_end;
		char		*stdin_buffer_start;
		char		*stdin_buffer_end;
		char		*p_mb;
		int		num_mb_chars;
		size_t		num_wc_chars;

		MBSTOWCS(wcs_buffer, NOCATGETS("Standard in"));
		makefile_name = GETNAME(wcs_buffer, FIND_LENGTH);
		/*
		 * Memory to read standard in, then convert it
		 * to wide char strings.
		 */
		stdin_buffer_start =
		  stdin_text_p = getmem(length = 1024);
		stdin_buffer_end = stdin_text_p + length;
		MBSTOWCS(wcs_buffer, NOCATGETS("standard input"));
		file_being_read = (wchar_t *) wcsdup(wcs_buffer);
		line_number = 0;
		while ((n = read(fileno(stdin),
				 stdin_text_p,
				 length)) > 0) {
			length -= n;
			stdin_text_p += n;
			if (length == 0) {
				p_mb = getmem(length = 1024 +
					      (stdin_buffer_end -
					       stdin_buffer_start));
				(void) strncpy(p_mb,
					       stdin_buffer_start,
					       (stdin_buffer_end -
					        stdin_buffer_start));
				retmem_mb(stdin_buffer_start);
				stdin_text_p = p_mb +
				  (stdin_buffer_end - stdin_buffer_start);
				stdin_buffer_start = p_mb;
				stdin_buffer_end =
				  stdin_buffer_start + length;
				length = 1024;
			}
		}
		if (n < 0) {
			fatal(gettext("Error reading standard input: %s"),
			      errmsg(errno));
		}
		stdin_text_p = stdin_buffer_start;
		stdin_text_end = stdin_buffer_end - length;
		num_mb_chars = stdin_text_end - stdin_text_p;

		/*
		 * Now, convert the sequence of multibyte chars into
		 * a sequence of corresponding wide character codes.
		 */
		source->string.free_after_use = false;
		source->previous = NULL;
		source->bytes_left_in_file = 0;
		source->fd = -1;
		source->already_expanded = false;
		source->string.buffer.start =
		  source->string.text.p = ALLOC_WC(num_mb_chars + 1);
		source->string.buffer.end =
		    source->string.text.p + num_mb_chars;
		num_wc_chars = mbstowcs(source->string.text.p,
					stdin_text_p,
					num_mb_chars);
		if ((int) num_wc_chars >= 0) {
			source->string.text.end =
			  source->string.text.p + num_wc_chars;
		}
		(void) retmem_mb(stdin_text_p);
	}
	line_number = 1;
	if (trace_reader) {
		(void) printf(gettext(">>>>>>>>>>>>>>>> Reading makefile %s\n"),
			      makefile_name->string_mb);
	}
	parse_makefile(makefile_name, source);
	if (trace_reader) {
		(void) printf(gettext(">>>>>>>>>>>>>>>> End of makefile %s\n"),
			      makefile_name->string_mb);
	}
#ifdef NSE
	if (report_file && (previous_current_makefile[0] != NULL)) {
		wcscpy(current_makefile, previous_current_makefile);
	}
#endif
	if(file_being_read) {
		retmem(file_being_read);
	}
	file_being_read = previous_file_being_read;
	line_number = previous_line_number;
	makefile_type = reading_nothing;
	max_include_depth--;
	if (make_state_locked) {
		/* Unlock .make.state. */
		unlink(make_state_lockfile);
		make_state_locked = false;
		retmem_mb(make_state_lockfile);
	}
	if (forget_after_parse) {
		retmem(makefile->body.makefile.contents);
		makefile->body.makefile.contents = NULL;
	}
	retmem_mb((char *)source);
	return succeeded;
}

/*
 *	parse_makefile(true_makefile_name, source)
 *
 *	Strings are read from Sources.
 *	When macros are found, their values are represented by a
 *	Source that is pushed on a stack. At end of string
 *	(that is returned from GET_CHAR() as 0), the block is popped.
 *
 *	Parameters:
 *		true_makefile_name	The name of makefile we are parsing
 *		source			The source block to read from
 *
 *	Global variables used:
 *		do_not_exec_rule Is -n on?
 *		line_number	The number of the current makefile line
 *		makefile_type	What kind of makefile are we reading?
 *		empty_name	The Name ""
 */
static void
parse_makefile(register Name true_makefile_name, register Source source)
{
/*
	char			mb_buffer[MB_LEN_MAX];
 */
	register wchar_t	*source_p;
	register wchar_t	*source_end;
	register wchar_t	*string_start;
	wchar_t			*string_end;
	register Boolean	macro_seen_in_string;
	Boolean			append;
	String_rec		name_string;
	wchar_t			name_buffer[STRING_BUFFER_LENGTH];
	register int		distance;
	register int		paren_count;
	int			brace_count;
	int			char_number;
	Cmd_line		command;
	Cmd_line		command_tail;
	Name			macro_value;

	Name_vector_rec		target;
	Name_vector_rec		depes;
	Name_vector_rec		extra_name_vector;
	Name_vector		current_names;
	Name_vector		extra_names = &extra_name_vector;
	Name_vector		nvp;
	Boolean			target_group_seen;

	register Reader_state   state;
	register Reader_state   on_eoln_state;
	register Separator	separator;

	wchar_t                 buffer[4 * STRING_BUFFER_LENGTH];
	Source			extrap;

	Boolean                 save_do_not_exec_rule = do_not_exec_rule;
	Name                    makefile_name;
	Name                    makefile_name_raw;

	static Name		sh_name;
	static Name		shell_name;
	int			i;

	int			tmp_bytes_left_in_string;
	Boolean			tmp_maybe_directive = false;
	int    			emptycount = 0;
	Boolean			first_target;

	enum directive		directive_type; 
	int			directive_len;
	struct dent		*dp;

	String_rec		include_name;
	wchar_t			include_buffer[STRING_BUFFER_LENGTH];

	target.next = depes.next = NULL;
	/* Move some values from their struct to register declared locals */
	CACHE_SOURCE(0);

 start_new_line:
	/*
	 * Read whitespace on old line. Leave pointer on first char on
	 * next line.
	 */
	first_target = true;
	on_eoln_state = exit_state;
/*
	for (WCTOMB(mb_buffer, GET_CHAR());
	     1;
	     source_p++, WCTOMB(mb_buffer, GET_CHAR()))
		switch (mb_buffer[0]) {
 */
	for (char_number=0; 1; source_p++,char_number++) switch (GET_CHAR()) {
	case nul_char:
		/* End of this string. Pop it and return to the previous one */
		GET_NEXT_BLOCK(source);
		source_p--;
		if (source == NULL) {
			GOTO_STATE(on_eoln_state);
		}
		break;
	case newline_char:
	end_of_line:
		source_p++;
		if (source->fd >= 0) {
			line_number++;
		}
		switch (GET_CHAR()) {
		case nul_char:
			GET_NEXT_BLOCK(source);
			if (source == NULL) {
				GOTO_STATE(on_eoln_state);
			}
			/* Go back to the top of this loop */
			goto start_new_line;
		case newline_char:
		case numbersign_char:
		case dollar_char:
		case space_char:
		case tab_char:
			/*
			 * Go back to the top of this loop since the
			 * new line does not start with a regular char.
			 */
			goto start_new_line;
		default:
			/* We found the first proper char on the new line */
			goto start_new_line_no_skip;
		}
	case space_char:
		if (char_number == 0)
			line_started_with_space=line_number;
	case tab_char:
		/* Whitespace. Just keep going in this loop */
		break;
	case numbersign_char:
		/* Comment. Skip over it */
		if (skip_comment(source_p, source_end, source) == false) {
			GOTO_STATE(on_eoln_state);
		}
		/*
		 * After we skip the comment we go to
		 * the end of line handler since end of
		 * line terminates comments.
		 */
		goto end_of_line;
	case dollar_char:
		/* Macro reference */
		if (source->already_expanded) {
			/*
			 * If we are reading from the expansion of a
			 * macro we already expanded everything enough.
			 */
			goto start_new_line_no_skip;
		}
		/*
		 * Expand the value and push the Source on the stack of
		 * things being read.
		 */
		source_p++;
		UNCACHE_SOURCE();
		{
			Source t = (Source) alloca((int) sizeof (Source_rec));
			source = push_macro_value(t,
						  buffer,
						  sizeof buffer,
						  source);
		}
		CACHE_SOURCE(1);
		break;
	default:
		/* We found the first proper char on the new line */
		goto start_new_line_no_skip;
	}

	/*
	 * We found the first normal char (one that starts an identifier)
	 * on the newline.
	 */
start_new_line_no_skip:
	/* Inspect that first char to see if it maybe is special anyway */
	switch (GET_CHAR()) {
	case nul_char:
		GET_NEXT_BLOCK(source);
		if (source == NULL) {
			GOTO_STATE(on_eoln_state);
		}
		goto start_new_line_no_skip;
	case newline_char:
		/* Just in case */
		goto start_new_line;
	case exclam_char:
		/* Evaluate the line before it is read */
		string_start = source_p + 1;
		macro_seen_in_string = false;
		/* Stuff the line in a string so we can eval it. */
		for (; 1; source_p++) {
			switch (GET_CHAR()) {
			case newline_char:
				goto eoln_1;
			case nul_char:
				if (source->fd > 0) {
					if (!macro_seen_in_string) {
						macro_seen_in_string = true;
						INIT_STRING_FROM_STACK(
						      name_string, name_buffer);
					}
					append_string(string_start,
						      &name_string,
						      source_p - string_start);
					GET_NEXT_BLOCK(source);
					string_start = source_p;
					source_p--;
					break;
				}
			eoln_1:
				if (!macro_seen_in_string) {
					INIT_STRING_FROM_STACK(name_string,
							       name_buffer);
				}
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				extrap = (Source)
				  alloca((int) sizeof (Source_rec));
				extrap->string.buffer.start = NULL;
				extrap->inp_buf =
				  extrap->inp_buf_ptr =
				    extrap->inp_buf_end = NULL;
				extrap->error_converting = false;
				if (*source_p == (int) nul_char) {
					source_p++;
				}
				/* Eval the macro */
				expand_value(GETNAME(name_string.buffer.start,
						     FIND_LENGTH),
					     &extrap->string,
					     false);
				if (name_string.free_after_use) {
					retmem(name_string.buffer.start);
				}
				UNCACHE_SOURCE();
				extrap->string.text.p =
				  extrap->string.buffer.start;
				extrap->fd = -1;
				/* And push the value */
				extrap->previous = source;
				source = extrap;
				CACHE_SOURCE(0);
				goto line_evald;
			}
		}
	default:
		goto line_evald;
	}

	/* We now have a line we can start reading */
 line_evald:
	if (source == NULL) {
		GOTO_STATE(exit_state);
	}
	/* Check if this is a directive command */
	if ((makefile_type == reading_makefile) &&
	    !source->already_expanded) {

	    if (include_d[0] == (int) nul_char)
		init_directives();

	    directive_len = 0;
	    directive_type = D_NONE;
	    for (dp = directives; dp->directive; dp++) {
		if (IS_WEQUALN(source_p, dp->directive, dp->dlen)) {
			if (source_p[dp->dlen] == (int)space_char ||
			    source_p[dp->dlen] == (int)tab_char) {
				directive_len = dp->dlen;
				directive_type = dp->dir;
				break;
			}
		}
		if (sunpro_compat)
			break;
	    }

	    if (directive_type) {
		source_p += directive_len;
	try_next_include:	/* More than one include name on the line? */

		if (!sunpro_compat && *source_p == numbersign_char)
			goto start_new_line;
		if (iswspace(*source_p)) {
			Makefile_type save_makefile_type;
			wchar_t		*name_start;
			int		name_length;
			String_rec	destination;

			/*
			 * Yes, this is an include.
			 * Skip spaces to get to the filename.
			 */
			while (iswspace(*source_p) ||
			       (*source_p == (int) nul_char)) {
				switch (GET_CHAR()) {
				case newline_char:
					goto start_new_line;

				case nul_char:
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
					break;

				default:
					source_p++;
					break;
				}
			}

			string_start = source_p;
			/* Find the end of the filename */
			macro_seen_in_string = false;
			while (!iswspace(*source_p) ||
			       (*source_p == (int) nul_char)) {
				switch (GET_CHAR()) {
				case nul_char:
					if (!macro_seen_in_string) {
						INIT_STRING_FROM_STACK(name_string,
								       name_buffer);
					}
					append_string(string_start,
						      &name_string,
						      source_p - string_start);
					macro_seen_in_string = true;
					GET_NEXT_BLOCK(source);
					string_start = source_p;
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
					break;

			    	case numbersign_char:
					if (!sunpro_compat) {
						if (source_p == string_start)
							goto start_new_line;
						goto string_end;
					}

				default:
					source_p++;
					break;
				}
			}

	string_end:
			source->string.text.p = source_p;
			if (macro_seen_in_string) {
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				name_start = name_string.buffer.start;
				name_length = name_string.text.p - name_start;
			} else {
				name_start = string_start;
				name_length = source_p - string_start;
			}

			/* Strip "./" from the head of the name */
			if ((name_start[0] == (int) period_char) &&
	    		   (name_start[1] == (int) slash_char)) {
				name_start += 2;
				name_length -= 2;
			}
			/* if include file name is surrounded by double quotes */
			if ((name_start[0] == (int) doublequote_char) &&
			    (name_start[name_length - 1] == (int) doublequote_char)) {
			    	name_start += 1;
			    	name_length -= 2;

			    	/* if name does not begin with a slash char */
			    	if (name_start[0] != (int) slash_char) {
					if ((name_start[0] == (int) period_char) &&
					    (name_start[1] == (int) slash_char)) {
						name_start += 2;
						name_length -= 2;
					}

					INIT_STRING_FROM_STACK(include_name, include_buffer);
					APPEND_NAME(true_makefile_name,
						      &include_name,
						      true_makefile_name->hash.length);

					wchar_t *slash = wcsrchr(include_name.buffer.start, (int) slash_char);
					if (slash != NULL) {
						include_name.text.p = slash + 1;
						append_string(name_start,
							      &include_name,
							      name_length);

						name_start = include_name.buffer.start;
						name_length = include_name.text.p - name_start; 
					}
				}
			}

			/* Even when we run -n we want to create makefiles */
			do_not_exec_rule = false;
			makefile_name_raw = GETNAME(name_start, name_length);
			if (makefile_name_raw->dollar) {
				wchar_t		buffer[STRING_BUFFER_LENGTH];
				wchar_t		*p;
				wchar_t		*q;

				INIT_STRING_FROM_STACK(destination, buffer);
				expand_value(makefile_name_raw,
					     &destination,
					     false);

				destination.text.p = destination.buffer.start;
			next_in_var:
				for (p = destination.text.p;
				     (*p != (int) nul_char) && iswspace(*p);
				     p++);
				for (q = p;
				     (*q != (int) nul_char) && !iswspace(*q);
				     q++);
				destination.text.p = q;

				makefile_name = GETNAME(p, q-p);
				if (destination.text.p >= destination.text.end &&
				    destination.free_after_use) {
					/*
					 * Free it after we did a complete
					 * parsing of the macro value.
					 */
					retmem(destination.buffer.start);
				}
			} else {
				makefile_name = makefile_name_raw;
			}
			switch (directive_type) {

			case D_INCLUDE:
			case D_IINCLUDE:
				UNCACHE_SOURCE();
				/* Read the file */
				save_makefile_type = makefile_type;
				/*
				 * The original make program from Sun did complain with:
				 * FOO=
				 * include $(FOO)
				 * but this is in conflict with the behavior of smake
				 * and gmake and it would cause prolems if we allow
				 * $(FOO) to expand to more than one incude file name.
				 * So let us be quiet with empty includes.
				 */
				if (*makefile_name->string_mb != nul_char &&
				    read_simple_file(makefile_name,
					     true,
					     true,
					     directive_type == D_IINCLUDE ? false:true,
					     false,
					     true,
					     false) == failed && directive_type != D_IINCLUDE) {
					fatal_reader(gettext("Read of include file `%s' failed"),
					     makefile_name->string_mb);
				}
				makefile_type = save_makefile_type;
				do_not_exec_rule = save_do_not_exec_rule;
				CACHE_SOURCE(0);
				break;

			case D_EXPORT:
				doexport(makefile_name);
				break;
			case D_UNEXPORT:
				dounexport(makefile_name);
				break;
			case D_READONLY:
				doreadonly(makefile_name);
				break;
			case D_NONE:
				/*
				 * Since we checked for directive_type != 0
				 * before, this cannot happen, but it makes
				 * clang quiet.
				 */
				break;
			}
			if (sunpro_compat) {
				source_p++;
				goto start_new_line;
			}
			if (makefile_name != makefile_name_raw) {
				/*
				 * The "makefile_name" is not from a line in
				 * Makefile, but from a macro expansion. Check
				 * whether there may be more names in the value
				 * of that macro.
				 */
				if (destination.text.p < destination.text.end)
					goto next_in_var;
			}
			if (*source_p != newline_char) {
				/*
				 * The next character after the filename in the
				 * include directive was not a newline, there
				 * may be more names in that directive line.
				 */
				goto try_next_include;
			}
			source_p++;
			goto start_new_line;
		} else {
			source_p -= directive_len;
		}
	    } else {
		/* Check if the directive text was split across 8K boundary. */
		
		tmp_bytes_left_in_string = source->string.text.end - source_p;
		if (tmp_bytes_left_in_string < 9) {
			struct dent	*dp;

			tmp_maybe_directive = false;
			for (dp = directives; dp->directive; dp++) {
				if (dp != directives && sunpro_compat)
					break;
				if (dp->dlen < tmp_bytes_left_in_string)
					continue;
				if (IS_WEQUALN(source_p, dp->directive,
				    dp->dlen)) {
					tmp_maybe_directive = true;
					break;
				}
			}
			if (tmp_maybe_directive) {
				GET_NEXT_BLOCK(source);
				tmp_maybe_directive = false;
				goto line_evald;
			}
		}
	    }
	}

	/* Reset the status in preparation for the new line */
	for (nvp = &target; nvp != NULL; nvp = nvp->next) {
		nvp->used = 0;
	}
	for (nvp = &depes; nvp != NULL; nvp = nvp->next) {
		nvp->used = 0;
	}
	target_group_seen = false;
	command = command_tail = NULL;
	macro_value = NULL;
	append = false;
	current_names = &target;
	SET_STATE(scan_name_state);
	on_eoln_state = illegal_eoln_state;
	separator = none_seen;

	/* The state machine starts here */
 enter_state:
	while (1) switch (state) {

/****************************************************************
 *	Scan name state
 */
case scan_name_state:
	/* Scan an identifier. We skip over chars until we find a break char */
	/* First skip white space. */
	for (; 1; source_p++) switch (GET_CHAR()) {
	case nul_char:
		GET_NEXT_BLOCK(source);
		source_p--;
		if (source == NULL) {
			GOTO_STATE(on_eoln_state);
		}
		break;
	case newline_char:
		/* We found the end of the line. */
		/* Do postprocessing or return error */
		source_p++;
		if (source->fd >= 0) {
			line_number++;
		}
		GOTO_STATE(on_eoln_state);
	case backslash_char:
		/* Continuation */
		if (*++source_p == (int) nul_char) {
			GET_NEXT_BLOCK(source);
			if (source == NULL) {
				GOTO_STATE(on_eoln_state);
			}
		}
		if (*source_p == (int) newline_char) {
			if (source->fd >= 0) {
				line_number++;
			}
		} else {
			source_p--;
		}
		break;
	case tab_char:
	case space_char:
		/* Whitespace is skipped */
		break;
	case numbersign_char:
		/* Comment. Skip over it */
		if (skip_comment(source_p, source_end, source) == true) {
			source_p++;
			if (source->fd >= 0) {
				line_number++;
			}
		}
		GOTO_STATE(on_eoln_state);
	case dollar_char:
		/* Macro reference. Expand and push value */
		if (source->already_expanded) {
			goto scan_name;
		}
		source_p++;
		UNCACHE_SOURCE();
		{
			Source t = (Source) alloca((int) sizeof (Source_rec));
			source = push_macro_value(t,
						  buffer,
						  sizeof buffer,
						  source);
		}
		CACHE_SOURCE(1);
		break;
	default:
		/* End of white space */
		goto scan_name;
	}

	/* First proper identifier character */
 scan_name:

	string_start = source_p;
	paren_count = brace_count = 0;
	macro_seen_in_string = false;
	resume_name_scan:
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case nul_char:
			/* Save what we have seen so far of the identifier */
			if (source_p != string_start) {
				if (!macro_seen_in_string) {
					INIT_STRING_FROM_STACK(name_string,
							       name_buffer);
				}
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				macro_seen_in_string = true;
			}
			/* Get more text to read */
			GET_NEXT_BLOCK(source);
			string_start = source_p;
			source_p--;
			if (source == NULL) {
				GOTO_STATE(on_eoln_state);
			}
			break;
		case newline_char:
			if (paren_count > 0) {
				fatal_reader(gettext("Unmatched `(' on line"));
			}
			if (brace_count > 0) {
				fatal_reader(gettext("Unmatched `{' on line"));
			}
			source_p++;
			/* Enter name */
			current_names = enter_name(&name_string,
						   macro_seen_in_string,
						   string_start,
						   source_p - 1,
						   current_names,
						   &extra_names,
						   &target_group_seen);
			first_target = false;
			if (extra_names == NULL) {
				extra_names = (Name_vector)
				  alloca((int) sizeof (Name_vector_rec));
			}
			/* Do postprocessing or return error */
			if (source->fd >= 0) {
				line_number++;
			}
			GOTO_STATE(on_eoln_state);
		case backslash_char:
			/* Check if this is a quoting backslash */
			if (!macro_seen_in_string) {
				INIT_STRING_FROM_STACK(name_string,
						       name_buffer);
				macro_seen_in_string = true;
			}
			append_string(string_start,
				      &name_string,
				      source_p - string_start);
			if (*++source_p == (int) nul_char) {
				GET_NEXT_BLOCK(source);
				if (source == NULL) {
					GOTO_STATE(on_eoln_state);
				}
			}
			if (*source_p == (int) newline_char) {
				if (source->fd >= 0) {
					line_number++;
				}
				*source_p = (int) space_char;
				string_start = source_p;
				goto resume_name_scan;
			} else {
				string_start = source_p;
				break;
			}
			break;
		case numbersign_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			fatal_reader(gettext("Unexpected comment seen"));
		case dollar_char:
			if (source->already_expanded) {
				break;
			}
			/* Save the identifier so far */
			if (source_p != string_start) {
				if (!macro_seen_in_string) {
					INIT_STRING_FROM_STACK(name_string,
							       name_buffer);
				}
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				macro_seen_in_string = true;
			}
			/* Eval and push the macro */
			source_p++;
			UNCACHE_SOURCE();
			{
				Source t =
				  (Source) alloca((int) sizeof (Source_rec));
				source = push_macro_value(t,
							  buffer,
							  sizeof buffer,
							  source);
			}
			CACHE_SOURCE(1);
			string_start = source_p + 1;
			break;
		case parenleft_char:
			paren_count++;
			break;
		case parenright_char:
			if (--paren_count < 0) {
				fatal_reader(gettext("Unmatched `)' on line"));
			}
			break;
		case braceleft_char:
			brace_count++;
			break;
		case braceright_char:
			if (--brace_count < 0) {
				fatal_reader(gettext("Unmatched `}' on line"));
			}
			break;
		case ampersand_char:
		case greater_char:
		case bar_char:
			if (paren_count + brace_count == 0) {
				source_p++;
			}
			/* Fall into */
		case tab_char:
		case space_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			current_names = enter_name(&name_string,
						   macro_seen_in_string,
						   string_start,
						   source_p,
						   current_names,
						   &extra_names,
						   &target_group_seen);
			first_target = false;
			if (extra_names == NULL) {
				extra_names = (Name_vector)
				  alloca((int) sizeof (Name_vector_rec));
			}
			goto enter_state;
		case colon_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			if (separator == conditional_seen) {
				break;
			}
/** POSIX **/
#if 0
			if(posix) {
			  emptycount = 0;
			}
#endif
/** END POSIX **/
			/* End of the target list. We now start reading */
			/* dependencies or a conditional assignment */
			if (separator != none_seen) {
				fatal_reader(gettext("Extra `:', `::', or `:=' on dependency line"));
			}
			/* Enter the last target */
			if ((string_start != source_p) ||
			    macro_seen_in_string) {
				current_names =
				  enter_name(&name_string,
					     macro_seen_in_string,
					     string_start,
					     source_p,
					     current_names,
					     &extra_names,
					     &target_group_seen);
				first_target = false;
				if (extra_names == NULL) {
					extra_names = (Name_vector)
					  alloca((int)
						 sizeof (Name_vector_rec));
				}
			}
			/* Check if it is ":" "::" or ":=" */
		scan_colon_label:
			switch (*++source_p) {
			case nul_char:
				GET_NEXT_BLOCK(source);
				source_p--;
				if (source == NULL) {
					GOTO_STATE(enter_dependencies_state);
				}
				goto scan_colon_label;
			case equal_char:
				if(svr4) {
				  fatal_reader(gettext("syntax error"));
				}
				separator = conditional_seen;
				source_p++;
				current_names = &depes;
				GOTO_STATE(scan_name_state);
			case colon_char:
				separator = two_colon;
				source_p++;
				break;
			default:
				separator = one_colon;
			}
			current_names = &depes;
			on_eoln_state = enter_dependencies_state;
			GOTO_STATE(scan_name_state);
		case semicolon_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			/* End of reading names. Start reading the rule */
			if ((separator != one_colon) &&
			    (separator != two_colon)) {
				fatal_reader(gettext("Unexpected command seen"));
			}
			/* Enter the last dependency */
			if ((string_start != source_p) ||
			    macro_seen_in_string) {
				current_names =
				  enter_name(&name_string,
					     macro_seen_in_string,
					     string_start,
					     source_p,
					     current_names,
					     &extra_names,
					     &target_group_seen);
				first_target = false;
				if (extra_names == NULL) {
					extra_names = (Name_vector)
					  alloca((int)
						 sizeof (Name_vector_rec));
				}
			}
			source_p++;
			/* Make sure to enter a rule even if the is */
			/* no text here */
			command = command_tail = ALLOC(Cmd_line);
			command->next = NULL;
			command->command_line = empty_name;
			command->make_refd = false;
			command->ignore_command_dependency = false;
			command->assign = false;
			command->ignore_error = false;
			command->silent = false;

			GOTO_STATE(scan_command_state);
		case plus_char:
			/*
			** following code drops the target separator plus char if it starts
			** a line.
			*/ 
			if(first_target && !macro_seen_in_string &&
					source_p == string_start) {
				for (; 1; source_p++)
				switch (GET_CHAR()) {
				case nul_char:
					if (source_p != string_start) {
						if (!macro_seen_in_string) {
							INIT_STRING_FROM_STACK(name_string,
									       name_buffer);
						}
						append_string(string_start,
							      &name_string,
							      source_p - string_start);
						macro_seen_in_string = true;
					}
					GET_NEXT_BLOCK(source);
					string_start = source_p;
					source_p--;
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
					break;
				case plus_char:
					source_p++;
					while (*source_p == (int) nul_char) {
						if (source_p != string_start) {
							if (!macro_seen_in_string) {
								INIT_STRING_FROM_STACK(name_string,
									       name_buffer);
							}
							append_string(string_start,
								      &name_string,
								      source_p - string_start);
							macro_seen_in_string = true;
						}
						GET_NEXT_BLOCK(source);
						string_start = source_p;
						if (source == NULL) {
							GOTO_STATE(on_eoln_state);
						}
					}
					if (*source_p == (int) tab_char ||
							*source_p == (int) space_char) {
						macro_seen_in_string = false;
						string_start = source_p + 1;
					} else {
						goto resume_name_scan;
					}
					break;
				case tab_char:
				case space_char:
					string_start = source_p + 1;
					break;
				default:
					goto resume_name_scan;
				}
			}
			if (paren_count + brace_count > 0) {
				break;
			}
			/* We found "+=" construct */
			if (source_p != string_start) {
				/* "+" is not a break char. */
				/* Ignore it if it is part of an identifier */
				source_p++;
				goto resume_name_scan;
			}
			/* Make sure the "+" is followed by a "=" */
		scan_append:
			switch (*++source_p) {
			case nul_char:
				if (!macro_seen_in_string) {
					INIT_STRING_FROM_STACK(name_string,
							       name_buffer);
				}
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				GET_NEXT_BLOCK(source);
				source_p--;
				string_start = source_p;
				if (source == NULL) {
					GOTO_STATE(illegal_eoln_state);
				}
				goto scan_append;
			case equal_char:
				if(!svr4) {
				  append = true;
				} else {
				  fatal_reader(gettext("Must be a separator on rules"));
				}
				break;
			default:
				/* The "+" just starts a regular name. */
				/* Start reading that name */
				goto resume_name_scan;
			}
			/* Fall into */
		case equal_char:
			if (paren_count + brace_count > 0) {
				break;
			}
			/* We found macro assignment. */
			/* Check if it is legal and if it is appending */
			switch (separator) {
			case none_seen:
				separator = equal_seen;
				on_eoln_state = enter_equal_state;
				break;
			case conditional_seen:
				on_eoln_state = enter_conditional_state;
				break;
			default:
				/* Reader must special check for "MACRO:sh=" */
				/* notation */
				if (sh_name == NULL) {
					MBSTOWCS(wcs_buffer, NOCATGETS("sh"));
					sh_name = GETNAME(wcs_buffer, FIND_LENGTH);
					MBSTOWCS(wcs_buffer, NOCATGETS("shell"));
					shell_name = GETNAME(wcs_buffer, FIND_LENGTH);
				}

				if (!macro_seen_in_string) {
					INIT_STRING_FROM_STACK(name_string,
						       name_buffer);
				}
				append_string(string_start,
					      &name_string,
					      source_p - string_start
				);

				if ( (((target.used == 1) &&
				     (depes.used == 1) &&
				     (depes.names[0] == sh_name)) ||
				    ((target.used == 1) &&
				     (depes.used == 0) &&
				     (separator == one_colon) &&
				     (GETNAME(name_string.buffer.start,FIND_LENGTH) == sh_name))) &&
				    (!svr4)) {
					String_rec	macro_name;
					wchar_t		buffer[100];

					INIT_STRING_FROM_STACK(macro_name,
							       buffer);
					APPEND_NAME(target.names[0],
						      &macro_name,
						      FIND_LENGTH);
					append_char((int) colon_char,
						    &macro_name);
					APPEND_NAME(sh_name,
						      &macro_name,
						      FIND_LENGTH);
					target.names[0] =
					  GETNAME(macro_name.buffer.start,
						  FIND_LENGTH);
					separator = equal_seen;
					on_eoln_state = enter_equal_state;
					break;
				} else if ( (((target.used == 1) &&
					    (depes.used == 1) &&
					    (depes.names[0] == shell_name)) ||
					   ((target.used == 1) &&
					    (depes.used == 0) &&
					    (separator == one_colon) &&
					    (GETNAME(name_string.buffer.start,FIND_LENGTH) == shell_name))) &&
					   (!svr4)) {
					String_rec	macro_name;
					wchar_t		buffer[100];

					INIT_STRING_FROM_STACK(macro_name,
							       buffer);
					APPEND_NAME(target.names[0],
						      &macro_name,
						      FIND_LENGTH);
					append_char((int) colon_char,
						    &macro_name);
					APPEND_NAME(shell_name,
						      &macro_name,
						      FIND_LENGTH);
					target.names[0] =
					  GETNAME(macro_name.buffer.start,
						  FIND_LENGTH);
					separator = equal_seen;
					on_eoln_state = enter_equal_state;
					break;
				} 
				if(svr4) {
				  fatal_reader(gettext("syntax error"));
				}
				else {
				  fatal_reader(gettext("Macro assignment on dependency line"));
				}
			}
			if (append) {
				source_p--;
			}
			/* Enter the macro name */
			if ((string_start != source_p) ||
			    macro_seen_in_string) {
				current_names =
				  enter_name(&name_string,
					     macro_seen_in_string,
					     string_start,
					     source_p,
					     current_names,
					     &extra_names,
					     &target_group_seen);
				first_target = false;
				if (extra_names == NULL) {
					extra_names = (Name_vector)
					  alloca((int)
						 sizeof (Name_vector_rec));
				}
			}
			if (append) {
				source_p++;
			}
			macro_value = NULL;
			source_p++;
			distance = 0;
			/* Skip whitespace to the start of the value */
			macro_seen_in_string = false;
			for (; 1; source_p++) {
				switch (GET_CHAR()) {
				case nul_char:
					GET_NEXT_BLOCK(source);
					source_p--;
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
					break;
				case backslash_char:
					if (*++source_p == (int) nul_char) {
						GET_NEXT_BLOCK(source);
						if (source == NULL) {
							GOTO_STATE(on_eoln_state);
						}
					}
					if (*source_p != (int) newline_char) {
						if (!macro_seen_in_string) {
							macro_seen_in_string =
							  true;
							INIT_STRING_FROM_STACK(name_string,
									       name_buffer);
						}
						append_char((int)
							    backslash_char,
							    &name_string);
						append_char(*source_p,
							    &name_string);
						string_start = source_p+1;
						goto macro_value_start;
					} else {
                                            if (source->fd >= 0) {
                                            	line_number++;
                                            }
                                        }
					break;
				case newline_char:
				case numbersign_char:
					string_start = source_p;
					goto macro_value_end;
				case tab_char:
				case space_char:
					break;
				default:
					string_start = source_p;
					goto macro_value_start;
				}
			}
		macro_value_start:
			/* Find the end of the value */
			for (; 1; source_p++) {
				if (distance != 0) {
					*source_p = *(source_p + distance);
				}
				switch (GET_CHAR()) {
				case nul_char:
					if (!macro_seen_in_string) {
						macro_seen_in_string = true;
						INIT_STRING_FROM_STACK(name_string,
								       name_buffer);
					}
					append_string(string_start,
						      &name_string,
						      source_p - string_start);
					GET_NEXT_BLOCK(source);
					string_start = source_p;
					source_p--;
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
					break;
				case backslash_char:
					source_p++;
					if (distance != 0) {
						*source_p =
						  *(source_p + distance);
					}
					if (*source_p == (int) nul_char) {
						if (!macro_seen_in_string) {
							macro_seen_in_string =
							  true;
							INIT_STRING_FROM_STACK(name_string,
									       name_buffer);
						}

/*  BID_1225561 */
						*(source_p - 1) = (int) space_char;
						append_string(string_start,
							      &name_string,
							      source_p -
							      string_start - 1);
						GET_NEXT_BLOCK(source);
						string_start = source_p;
						if (source == NULL) {
							GOTO_STATE(on_eoln_state);
						}
						if (distance != 0) {
							*source_p =
							  *(source_p +
							    distance);
						}
						if (*source_p == (int) newline_char) {
							append_char((int) space_char, &name_string);
						} else {
							append_char((int) backslash_char, &name_string);
						}
/****************/
					}
					if (*source_p == (int) newline_char) {
						source_p--;
						line_number++;
						distance++;
						*source_p = (int) space_char;
						while ((*(source_p +
							  distance + 1) ==
							(int) tab_char) ||
						       (*(source_p +
							  distance + 1) ==
							(int) space_char)) {
							distance++;
						}
					}
					break;
				case newline_char:
				case numbersign_char:
					goto macro_value_end;
				}
			}
		macro_value_end:
			/* Complete the value in the string */
			if (!macro_seen_in_string) {
				macro_seen_in_string = true;
				INIT_STRING_FROM_STACK(name_string,
						       name_buffer);
			}
			append_string(string_start,
				      &name_string,
				      source_p - string_start);
			if (name_string.buffer.start != name_string.text.p) {
					macro_value =
					  GETNAME(name_string.buffer.start,
						  FIND_LENGTH);
				}
			if (name_string.free_after_use) {
				retmem(name_string.buffer.start);
			}
			for (; distance > 0; distance--) {
				*source_p++ = (int) space_char;
			}
			GOTO_STATE(on_eoln_state);
		}
	}

/****************************************************************
 *	enter dependencies state
 */
 case enter_dependencies_state:
 enter_dependencies_label:
/* Expects pointer on first non whitespace char after last dependency. (On */
/* next line.) We end up here after having read a "targets : dependencies" */
/* line. The state checks if there is a rule to read and if so dispatches */
/* to scan_command_state scan_command_state reads one rule line and the */
/* returns here */

	/* First check if the first char on the next line is special */
	switch (GET_CHAR()) {
	case nul_char:
		GET_NEXT_BLOCK(source);
		if (source == NULL) {
			break;
		}
		goto enter_dependencies_label;
	case exclam_char:
		/* The line should be evaluate before it is read */
		macro_seen_in_string = false;
		string_start = source_p + 1;
		for (; 1; source_p++) {
			switch (GET_CHAR()) {
			case newline_char:
				goto eoln_2;
			case nul_char:
				if (source->fd > 0) {
					if (!macro_seen_in_string) {
						macro_seen_in_string = true;
						INIT_STRING_FROM_STACK(name_string,
								       name_buffer);
					}
					append_string(string_start,
						      &name_string,
						      source_p - string_start);
					GET_NEXT_BLOCK(source);
					string_start = source_p;
					source_p--;
					break;
				}
			eoln_2:
				if (!macro_seen_in_string) {
					INIT_STRING_FROM_STACK(name_string,
							       name_buffer);
				}
				append_string(string_start,
					      &name_string,
					      source_p - string_start);
				extrap = (Source)
				  alloca((int) sizeof (Source_rec));
				extrap->string.buffer.start = NULL;
				extrap->inp_buf =
				  extrap->inp_buf_ptr =
				    extrap->inp_buf_end = NULL;
				extrap->error_converting = false;
				expand_value(GETNAME(name_string.buffer.start,
						     FIND_LENGTH),
					     &extrap->string,
					     false);
				if (name_string.free_after_use) {
					retmem(name_string.buffer.start);
				}
				UNCACHE_SOURCE();
				extrap->string.text.p =
				  extrap->string.buffer.start;
				extrap->fd = -1;
				extrap->previous = source;
				source = extrap;
				CACHE_SOURCE(0);
				goto enter_dependencies_label;
			}
		}
	case dollar_char:
		if (source->already_expanded) {
			break;
		}
		source_p++;
		UNCACHE_SOURCE();
		{
			Source t = (Source) alloca((int) sizeof (Source_rec));
			source = push_macro_value(t,
						  buffer,
						  sizeof buffer,
						  source);
		}
		CACHE_SOURCE(0);
		goto enter_dependencies_label;
	case numbersign_char:
		if (makefile_type != reading_makefile) {
			source_p++;
			GOTO_STATE(scan_command_state);
		}
		for (; 1; source_p++) {
			switch (GET_CHAR()) {
			case nul_char:
				GET_NEXT_BLOCK_NOCHK(source);
				if (source == NULL) {
					GOTO_STATE(on_eoln_state);
				}
				if (source->error_converting) {
				// Illegal byte sequence - skip its first byte
					source->inp_buf_ptr++;
				}
				source_p--;
				break;
			case backslash_char:
				if (*++source_p == (int) nul_char) {
					GET_NEXT_BLOCK_NOCHK(source);
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
					if (source->error_converting) {
					// Illegal byte sequence - skip its first byte
						source->inp_buf_ptr++;
						source_p--;
						break;
					}
				}
				if(*source_p == (int) newline_char) {
					if (source->fd >= 0) {
						line_number++;
					}
				}
				break;
			case newline_char:
				source_p++;
				if (source->fd >= 0) {
					line_number++;
				}
				goto enter_dependencies_label;
			}
		}

	case tab_char:
		GOTO_STATE(scan_command_state);
	}

	/* We read all the command lines for the target/dependency line. */
	/* Enter the stuff */
	enter_target_groups_and_dependencies( &target, &depes, command, 
					     separator, target_group_seen);

	goto start_new_line;

/****************************************************************
 *	scan command state
 */
case scan_command_state:
	/* We need to read one rule line. Do that and return to */
	/* the enter dependencies state */
	string_start = source_p;
	macro_seen_in_string = false;
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case backslash_char:
			if (!macro_seen_in_string) {
				INIT_STRING_FROM_STACK(name_string,
						       name_buffer);
			}
			append_string(string_start,
				      &name_string,
				      source_p - string_start);
			macro_seen_in_string = true;
			if (*++source_p == (int) nul_char) {
				GET_NEXT_BLOCK(source);
				if (source == NULL) {
					string_start = source_p;
					goto command_newline;
				}
			}
			append_char((int) backslash_char, &name_string);
			append_char(*source_p, &name_string);
			if (*source_p == (int) newline_char) {
				if (source->fd >= 0) {
					line_number++;
				}
				if (*++source_p == (int) nul_char) {
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						string_start = source_p;
						goto command_newline;
					}
				}
				if (*source_p == (int) tab_char) {
					source_p++;
				}
			} else {
				if (*++source_p == (int) nul_char) {
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						string_start = source_p;
						goto command_newline;
					}
				}
			}
			string_start = source_p;
			if ((*source_p == (int) newline_char) ||
			    (*source_p == (int) backslash_char) ||
			    (*source_p == (int) nul_char)) {
				source_p--;
			}
			break;
		case newline_char:
		command_newline:
			if ((string_start != source_p) ||
			    macro_seen_in_string) {
				if (macro_seen_in_string) {
					append_string(string_start,
						      &name_string,
						      source_p - string_start);
					string_start =
					  name_string.buffer.start;
					string_end = name_string.text.p;
				} else {
					string_end = source_p;
				}
				while ((*string_start != (int) newline_char) &&
				       iswspace(*string_start)){
					string_start++;
				}
				if ((string_end > string_start) ||
				    (makefile_type == reading_statefile)) {
					if (command_tail == NULL) {
						command =
						  command_tail =
						    ALLOC(Cmd_line);
					} else {
						command_tail->next =
						  ALLOC(Cmd_line);
						command_tail =
						  command_tail->next;
					}
					command_tail->next = NULL;
					command_tail->make_refd = false;
					command_tail->ignore_command_dependency = false;
					command_tail->assign = false;
					command_tail->ignore_error = false;
					command_tail->silent = false;
					command_tail->command_line =
					  GETNAME(string_start,
						  string_end - string_start);
					if (macro_seen_in_string &&
					    name_string.free_after_use) {
						retmem(name_string.
						       buffer.start);
					}
				}
			}
			do {
				if ((source != NULL) && (source->fd >= 0)) {
					line_number++;
				}
				if ((source != NULL) &&
				    (*++source_p == (int) nul_char)) {
					GET_NEXT_BLOCK(source);
					if (source == NULL) {
						GOTO_STATE(on_eoln_state);
					}
				}
			} while (*source_p == (int) newline_char);

			GOTO_STATE(enter_dependencies_state);
		case nul_char:
			if (!macro_seen_in_string) {
				INIT_STRING_FROM_STACK(name_string,
						       name_buffer);
			}
			append_string(string_start,
				      &name_string,
				      source_p - string_start);
			macro_seen_in_string = true;
			GET_NEXT_BLOCK(source);
			string_start = source_p;
			source_p--;
			if (source == NULL) {
				GOTO_STATE(enter_dependencies_state);
			}
			break;
		}
	}

/****************************************************************
 *	enter equal state
 */
case enter_equal_state:
	if (target.used != 1) {
		GOTO_STATE(poorly_formed_macro_state);
	}
	enter_equal(target.names[0], macro_value, append);
	goto start_new_line;

/****************************************************************
 *	enter conditional state
 */
case enter_conditional_state:
	if (depes.used != 1) {
		GOTO_STATE(poorly_formed_macro_state);
	}
	for (nvp = &target; nvp != NULL; nvp = nvp->next) {
		for (i = 0; i < nvp->used; i++) {
			enter_conditional(nvp->names[i],
					  depes.names[0],
					  macro_value,
					  append);
		}
	}
	goto start_new_line;

/****************************************************************
 *	Error states
 */
case illegal_bytes_state:
	fatal_reader(gettext("Invalid byte sequence"));
case illegal_eoln_state:
	if (line_number > 1) {
		if (line_started_with_space == (line_number - 1)) {
			line_number--;
			fatal_reader(gettext("Unexpected end of line seen\n\t*** missing separator (did you mean TAB instead of 8 spaces?)"));
		}
	}
	fatal_reader(gettext("Unexpected end of line seen"));
case poorly_formed_macro_state:
	fatal_reader(gettext("Badly formed macro assignment"));
case exit_state:
	return;
default:
	fatal_reader(gettext("Internal error. Unknown reader state"));
}
}

/*
 *	push_macro_value(bp, buffer, size, source)
 *
 *	Macro and function that evaluates one macro
 *	and makes the reader read from the value of it
 *
 *	Return value:
 *				The source block to read the macro from
 *
 *	Parameters:
 *		bp		The new source block to fill in
 *		buffer		Buffer to read from
 *		size		size of the buffer
 *		source		The old source block
 *
 *	Global variables used:
 */
static Source
push_macro_value(register Source bp, register wchar_t *buffer, int size, register Source source)
{
	bp->string.buffer.start = bp->string.text.p = buffer;
	bp->string.text.end = NULL;
	bp->string.buffer.end = buffer + (size/SIZEOFWCHAR_T);
	bp->string.free_after_use = false;
	bp->inp_buf =
	  bp->inp_buf_ptr =
	    bp->inp_buf_end = NULL;
	bp->error_converting = false;
	expand_macro(source, &bp->string, (wchar_t *) NULL, false);
	bp->string.text.p = bp->string.buffer.start;

	/* 4209588: 'make' doesn't understand a macro with whitespaces in the head as target.
	 * strip whitespace from the begining of the macro value
	 */
	while (iswspace(*bp->string.text.p)) {
		bp->string.text.p++;
	}

	bp->fd = -1;
	bp->already_expanded = true;
	bp->previous = source;
	return bp;
}

/*
 *	enter_target_groups_and_dependencies(target, depes, command, separator,
 *					     target_group_seen)
 *
 *	Parameters:
 *		target 		Structure that shows the target(s) on the line
 *				we are currently parsing. This can looks like
 *				target1 .. targetN : dependencies
 *						  	commands
 *				or
 *				target1 + .. + targetN : dependencies
 *							 commands
 *		depes		Dependencies
 *		command		Points to the command(s) to be executed for 
 *				this target.
 *		separator	: or :: or :=
 *		target_group_seen	Set if we have target1 + .. + targetN
 *		
 *		
 * 	After reading the command lines for a target, this routine 
 *	is called to setup the dependencies and the commands for it.
 * 	If the target is a % pattern or part of a target group, then 
 *  	the appropriate routines are called.
 */
	
void
enter_target_groups_and_dependencies(Name_vector target, Name_vector depes, Cmd_line command, Separator separator, Boolean target_group_seen)
{
	int			i;
	Boolean			reset= true;
	Chain			target_group_member;
	Percent			percent_ptr;

	for (; target != NULL; target = target->next) {
		for (i = 0; i < target->used; i++) {
			if (target->names[i] != NULL) {
				if (target_group_seen) {
					target_group_member =
					  find_target_groups(target, i, reset);
					if(target_group_member == NULL) {
						fatal_reader(gettext("Unexpected '+' on dependency line"));
					}
				}
				reset = false;

				/* If we saw it in the makefile it must be
				 * a file */
				target->names[i]->stat.is_file = true;
				/* Make sure that we use dependencies 
				 * entered for makefiles */
				target->names[i]->state = build_dont_know;

				/* If the target is special we delegate 
				 * the processing */
				if (target->names[i]->special_reader 
				    != no_special) {
					special_reader(target->names[i], 
						       depes, 
						       command);
				}	
				/* Check if this is a "a%b : x%y" type rule */
				else if (target->names[i]->percent) {
					percent_ptr = 
					  enter_percent(target->names[i], 
							target->target_group[i], 
							depes, command);
					if (target_group_seen) {
						target_group_member->percent_member =
						  percent_ptr;
					}
				} else if (target->names[i]->dollar) {
					enter_dyntarget(target->names[i]);
					enter_dependencies
					  (target->names[i],
					   target->target_group[i],
					   depes,	
					   command,
					   separator);
				} else {
					if (target_group_seen) {
						target_group_member->percent_member =
						  NULL;
					}
				
					enter_dependencies
					  (target->names[i],
					   target->target_group[i],
					   depes,	
					   command,
					   separator);
				}
			}
		}
	}
}

void
doexport(Name name)
{
	Name		val;
	char		*eval;
	size_t		len;

	if (strcmp(name->string_mb, NOCATGETS("SHELL")) == 0)
		return;

	val = getvar(name);
	len = strlen(name->string_mb) + 1 + strlen(val->string_mb) + 1;
	eval = (char *)malloc(len);
	strcpy(eval, name->string_mb);
	strcat(eval, "=");
	strcat(eval, val->string_mb);

	putenv(eval);
}

void
dounexport(Name name)
{
	Name		val;
	char		*eval;
	size_t		len;

	if (strcmp(name->string_mb, NOCATGETS("SHELL")) == 0)
		return;

	unsetenv(name->string_mb);
}

void
doreadonly(Name name)
{
	Property	macro;

	if ((macro = get_prop(name->prop, macro_prop)) != NULL)
		macro->body.macro.read_only = true;
}

static Boolean
skip_comment(wchar_t * &source_p, wchar_t * &source_end, Source &source)
{
	/* Comment. Skip over it */
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case nul_char:
			GET_NEXT_BLOCK_NOCHK(source);
			if (source == NULL) {
				return (false);
			}
			if (source->error_converting) {
			// Illegal byte sequence - skip its first byte
				source->inp_buf_ptr++;
			}
			source_p--;
			break;
		case backslash_char:
			/* Comments can be continued */
			if (*++source_p == (int) nul_char) {
				GET_NEXT_BLOCK_NOCHK(source);
				if (source == NULL) {
					return (false);
				}
				if (source->error_converting) {
				// Illegal byte sequence - skip its first byte
					source->inp_buf_ptr++;
					source_p--;
					break;
				}
			}
			if(*source_p == (int) newline_char) {
				if (source->fd >= 0) {
					line_number++;
				}
			}
			break;
		case newline_char:
			return (true);
		}
	}
}

static void
init_directives(void)
{
	MBSTOWCS(include_d,  NOCATGETS("include "));
	MBSTOWCS(iinclude_d, NOCATGETS("-include "));
	MBSTOWCS(export_d,   NOCATGETS("export "));
	MBSTOWCS(unexport_d, NOCATGETS("unexport "));
	MBSTOWCS(readonly_d, NOCATGETS("readonly "));
}
