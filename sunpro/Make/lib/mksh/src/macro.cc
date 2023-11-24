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
 * @(#)macro.cc 1.22 06/12/12
 */

#pragma	ident	"@(#)macro.cc	1.22	06/12/12"

/*
 * Copyright 2017-2021 J. Schilling
 * Copyright 2022, 2023 the schilytools team
 *
 * @(#)macro.cc	1.14 21/08/15 2017-2021 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)macro.cc	1.14 21/08/15 2017-2021 J. Schilling";
#endif

/*
 *	macro.cc
 *
 *	Handle expansion of make macros
 */

/*
 * Included files
 */
#include <mk/defs.h>		/* For "dollar" ($ = $) */
#include <mksh/dosys.h>		/* sh_command2string() */
#include <mksh/i18n.h>		/* get_char_semantics_value() */
#include <mksh/macro.h>
#include <mksh/misc.h>		/* retmem() */
#include <mksh/read.h>		/* get_next_block_fn() */

#include <schily/stdio.h>
#include <schily/wchar.h>
#include <schily/schily.h>

/*
 * We cannot use "using std::wcsdup" as wcsdup() is not always
 * in the std namespace.
 * The Sun CC compiler in version 4 does not suport using namespace std;
 * so be careful.
 */
#if !defined(__SUNPRO_CC_COMPAT) || __SUNPRO_CC_COMPAT >= 5
using namespace std;		/* needed for wcsdup() */
#endif

/*
 * File table of contents
 */
static void	add_macro_to_global_list(Name macro_to_add);
static void	expand_none(Name value, String destination);
#ifdef NSE
static void	expand_value_with_daemon(Name name, Property macro, String destination, Boolean cmd, Expand_Type exp_type = deflt_expand);
#else
static void	expand_value_with_daemon(Name, Property macro, String destination, Boolean cmd, Expand_Type exp_type = deflt_expand);
#endif

static void	init_arch_macros(void);
static void	init_mach_macros(void);
static Boolean	init_arch_done = false;
static Boolean	init_mach_done = false;


long env_alloc_num = 0;
long env_alloc_bytes = 0;

/*
 *	getvar(name)
 *
 *	Return expanded value of macro.
 *
 *	Return value:
 *				The expanded value of the macro
 *
 *	Parameters:
 *		name		The name of the macro we want the value for
 *
 *	Global variables used:
 */
Name
getvar(Name name)
{
	String_rec		destination;
	wchar_t			buffer[STRING_BUFFER_LENGTH];
	Name			result;

	if ((name == host_arch) || (name == target_arch)) {
		if (!init_arch_done) {
			init_arch_done = true;
			init_arch_macros();
		}
	}
	if ((name == host_mach) || (name == target_mach)) {
		if (!init_mach_done) {
			init_mach_done = true;
			init_mach_macros();
		}
	}

	INIT_STRING_FROM_STACK(destination, buffer);
	expand_value(maybe_append_prop(name, macro_prop)->body.macro.value,
		     &destination,
		     false);
	result = GETNAME(destination.buffer.start, FIND_LENGTH);
	if (destination.free_after_use) {
		retmem(destination.buffer.start);
	}
	return result;
}

/*
 *	expand_none(value, destination)
 *
 *	Copies the unexpanded string value todestination.
 *	destination is where the expanded value should be appended.
 *
 *	Parameters:
 *		value		The value we are expanding
 *		destination	Where to deposit the expansion
 *
 *	Global variables used:
 */
static void
expand_none(Name value, String destination)
{
	APPEND_NAME(value,
		destination,
		(int) value->hash.length
	);
	destination->text.end = destination->text.p;
}

/*
 *	expand_value(value, destination, cmd, exp_type)
 *
 *	Recursively expands all macros in the string value.
 *	destination is where the expanded value should be appended.
 *
 *	Parameters:
 *		value		The value we are expanding
 *		destination	Where to deposit the expansion
 *		cmd		If we are evaluating a command line we
 *				turn \ quoting off
 *		exp_type	The expansion type by default is deflt_expand
 *				but it may be:
 *
 *				no_expand	If macros of type gnu_assign
 *						should not be expanded.
 *				keep_ddollar	If we are in a :::= or +:=
 *						assignment and $$ should be
 *						left untouched.
 *
 *	Global variables used:
 */
void
expand_value(Name value, String destination, Boolean cmd, Expand_Type exp_type)
{
	Source_rec		sourceb;
	Source			source = &sourceb;
	wchar_t			*source_p = NULL;
	wchar_t			*source_end = NULL;
	wchar_t			*block_start = NULL;
	int			quote_seen = 0;

	if (value == NULL) {
		/*
		 * Make sure to get a string allocated even if it
		 * will be empty.
		 */
		MBSTOWCS(wcs_buffer, "");
		append_string(wcs_buffer, destination, FIND_LENGTH);
		destination->text.end = destination->text.p;
		return;
	}
	if (!value->dollar) {
		/*
		 * If the value we are expanding does not contain
		 * any $, we don't have to parse it.
		 */
		APPEND_NAME(value,
			destination,
			(int) value->hash.length
		);
		destination->text.end = destination->text.p;
		return;
	}

	if (value->being_expanded) {
		fatal_reader_mksh(gettext("Loop detected when expanding macro value `%s'"),
			     value->string_mb);
	}
	value->being_expanded = true;
	/* Setup the structure we read from */
	Wstring vals(value);
	sourceb.string.text.p = sourceb.string.buffer.start = wcsdup(vals.get_string());
	sourceb.string.free_after_use = true;
	sourceb.string.text.end =
	  sourceb.string.buffer.end =
	    sourceb.string.text.p + value->hash.length;
	sourceb.previous = NULL;
	sourceb.fd = -1;
	sourceb.inp_buf =
	  sourceb.inp_buf_ptr =
	    sourceb.inp_buf_end = NULL;
	sourceb.error_converting = false;
	/* Lift some pointers from the struct to local register variables */
	CACHE_SOURCE(0);
/* We parse the string in segments */
/* We read chars until we find a $, then we append what we have read so far */
/* (since last $ processing) to the destination. When we find a $ we call */
/* expand_macro() and let it expand that particular $ reference into dest */
	block_start = source_p;
	quote_seen = 0;
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case backslash_char:
			/* Quote $ in macro value */
			if (!cmd) {
				quote_seen = ~quote_seen;
			}
			continue;
		case dollar_char:
			/* Save the plain string we found since */
			/* start of string or previous $ */
			if (quote_seen) {
				append_string(block_start,
					      destination,
					      source_p - block_start - 1);
				block_start = source_p;
				break;
			}
			append_string(block_start,
				      destination,
				      source_p - block_start);
			source->string.text.p = ++source_p;
			if (exp_type == keep_ddollar && *source_p == dollar_char) {
				/*
				 * Found two dollars with keep_ddollar:
				 * output $$ and continue scanning after $$ in
				 * the source string.
				 */
				expand_none(dollar, destination);
				expand_none(dollar, destination);
				source->string.text.p = ++source_p;
				block_start = source_p;
				break;
			}
			UNCACHE_SOURCE();
			/* Go expand the macro reference */
			expand_macro(source, destination, sourceb.string.buffer.start, cmd, exp_type);
			CACHE_SOURCE(1);
			block_start = source_p + 1;
			break;
		case nul_char:
			/* The string ran out. Get some more */
			append_string(block_start,
				      destination,
				      source_p - block_start);
			GET_NEXT_BLOCK_NOCHK(source);
			if (source == NULL) {
				destination->text.end = destination->text.p;
				value->being_expanded = false;
				return;
			}
			if (source->error_converting) {
				fatal_reader_mksh(NOCATGETS("Internal error: Invalid byte sequence in expand_value()"));
			}
			block_start = source_p;
			source_p--;
			continue;
		}
		quote_seen = 0;
	}
	retmem(sourceb.string.buffer.start);
}

/*
 *	expand_macro(source, destination, current_string, cmd, exp_type)
 *
 *	Should be called with source->string.text.p pointing to
 *	the first char after the $ that starts a macro reference.
 *	source->string.text.p is returned pointing to the first char after
 *	the macro name.
 *	It will read the macro name, expanding any macros in it,
 *	and get the value. The value is then expanded.
 *	destination is a String that is filled in with the expanded macro.
 *	It may be passed in referencing a buffer to expand the macro into.
 * 	Note that most expansions are done on demand, e.g. right 
 *	before the command is executed and not while the file is
 * 	being parsed.
 *
 *	Parameters:
 *		source		The source block that references the string
 *				to expand
 *		destination	Where to put the result
 *		current_string	The string we are expanding, for error msg
 *		cmd		If we are evaluating a command line we
 *				turn \ quoting off
 *		exp_type	The expansion type by default is deflt_expand
 *				but it may be:
 *
 *				no_expand	If macros of type gnu_assign
 *						should not be expanded.
 *				keep_ddollar	If we are in a :::= or +:=
 *						assignment and $$ should be
 *						left untouched.
 *
 *	Global variables used:
 *		funny		Vector of semantic tags for characters
 *		is_conditional	Set if a conditional macro is refd
 *		make_word_mentioned Set if the word "MAKE" is mentioned
 *		makefile_type	We deliver extra msg when reading makefiles
 *		query		The Name "?", compared against
 *		query_mentioned	Set if the word "?" is mentioned
 */
void
expand_macro(Source source, String destination, wchar_t *current_string, Boolean cmd, Expand_Type exp_type)
{
	static Name		make = (Name)NULL;
	static wchar_t		colon_sh[4];
	static wchar_t		colon_shell[7];
	String_rec		string;
	wchar_t			buffer[STRING_BUFFER_LENGTH];
	wchar_t			*source_p = source->string.text.p;
	wchar_t			*source_end = source->string.text.end;
	int			closer = 0;
	wchar_t			*block_start = (wchar_t *)NULL;
	int			quote_seen = 0;
	int			closer_level = 1;
	Name			name = (Name)NULL;
	wchar_t			*colon = (wchar_t *)NULL;
	wchar_t			*percent = (wchar_t *)NULL;
	wchar_t			*eq = (wchar_t *) NULL;
	Property		macro = NULL;
	wchar_t			*p = (wchar_t*)NULL;
	String_rec		extracted;
	wchar_t			extracted_string[MAXPATHLEN];
	wchar_t			*left_head = NULL;
	wchar_t			*left_tail = NULL;
	wchar_t			*right_tail = NULL;
	int			left_head_len = 0;
	int			left_tail_len = 0;
	int			tmp_len = 0;
	wchar_t			*right_hand[128];
	int			i = 0;
	enum {
		no_extract,
		dir_extract,
		file_extract
	}                       extraction = no_extract;
	enum {
		no_replace,
		suffix_replace,
		pattern_replace,
		sh_replace
	}			replacement = no_replace;

	if (make == NULL) {
		MBSTOWCS(wcs_buffer, NOCATGETS("MAKE"));
		make = GETNAME(wcs_buffer, FIND_LENGTH);

		MBSTOWCS(colon_sh, NOCATGETS(":sh"));
		MBSTOWCS(colon_shell, NOCATGETS(":shell"));
	}

	right_hand[0] = NULL;

	/* First copy the (macro-expanded) macro name into string. */
	INIT_STRING_FROM_STACK(string, buffer);
recheck_first_char:
	/* Check the first char of the macro name to figure out what to do. */
	switch (GET_CHAR()) {
	case nul_char:
		GET_NEXT_BLOCK_NOCHK(source);
		if (source == NULL) {
			WCSTOMBS(mbs_buffer, current_string);
			fatal_reader_mksh(gettext("'$' at end of string `%s'"),
				     mbs_buffer);
		}
		if (source->error_converting) {
			fatal_reader_mksh(NOCATGETS("Internal error: Invalid byte sequence in expand_macro()"));
		}
		goto recheck_first_char;
	case parenleft_char:
		/* Multi char name. */
		closer = (int) parenright_char;
		break;
	case braceleft_char:
		/* Multi char name. */
		closer = (int) braceright_char;
		break;
	case newline_char:
		fatal_reader_mksh(gettext("'$' at end of line"));
	default:
		/* Single char macro name. Just suck it up */
		append_char(*source_p, &string);
		source->string.text.p = source_p + 1;
		goto get_macro_value;
	}

	/* Handle multi-char macro names */
	block_start = ++source_p;
	quote_seen = 0;
	for (; 1; source_p++) {
		switch (GET_CHAR()) {
		case nul_char:
			append_string(block_start,
				      &string,
				      source_p - block_start);
			GET_NEXT_BLOCK_NOCHK(source);
			if (source == NULL) {
				if (current_string != NULL) {
					WCSTOMBS(mbs_buffer, current_string);
					fatal_reader_mksh(gettext("Unmatched `%c' in string `%s'"),
						     closer ==
						     (int) braceright_char ?
						     (int) braceleft_char :
						     (int) parenleft_char,
						     mbs_buffer);
				} else {
					fatal_reader_mksh(gettext("Premature EOF"));
				}
			}
			if (source->error_converting) {
				fatal_reader_mksh(NOCATGETS("Internal error: Invalid byte sequence in expand_macro()"));
			}
			block_start = source_p;
			source_p--;
			continue;
		case newline_char:
			fatal_reader_mksh(gettext("Unmatched `%c' on line"),
				     closer == (int) braceright_char ?
				     (int) braceleft_char :
				     (int) parenleft_char);
		case backslash_char:
			/* Quote dollar in macro value. */
			if (!cmd) {
				quote_seen = ~quote_seen;
			}
			continue;
		case dollar_char:
			/*
			 * Macro names may reference macros.
			 * This expands the value of such macros into the
			 * macro name string.
			 */
			if (quote_seen) {
				append_string(block_start,
					      &string,
					      source_p - block_start - 1);
				block_start = source_p;
				break;
			}
			append_string(block_start,
				      &string,
				      source_p - block_start);
			source->string.text.p = ++source_p;
			UNCACHE_SOURCE();
			expand_macro(source, &string, current_string, cmd, exp_type);
			CACHE_SOURCE(0);
			block_start = source_p;
			source_p--;
			break;
		case parenleft_char:
			/* Allow nested pairs of () in the macro name. */
			if (closer == (int) parenright_char) {
				closer_level++;
			}
			break;
		case braceleft_char:
			/* Allow nested pairs of {} in the macro name. */
			if (closer == (int) braceright_char) {
				closer_level++;
			}
			break;
		case parenright_char:
		case braceright_char:
			/*
			 * End of the name. Save the string in the macro
			 * name string.
			 */
			if ((*source_p == closer) && (--closer_level <= 0)) {
				source->string.text.p = source_p + 1;
				append_string(block_start,
					      &string,
					      source_p - block_start);
				goto get_macro_value;
			}
			break;
		}
		quote_seen = 0;
	}
	/*
	 * We got the macro name. We now inspect it to see if it
	 * specifies any translations of the value.
	 */
get_macro_value:
	name = NULL;
	/* First check if we have a $(@D) type translation. */
	if ((get_char_semantics_value(string.buffer.start[0]) &
	     (int) special_macro_sem) &&
	    (string.text.p - string.buffer.start >= 2) &&
	    ((string.buffer.start[1] == 'D') ||
	     (string.buffer.start[1] == 'F'))) {
		switch (string.buffer.start[1]) {
		case 'D':
			extraction = dir_extract;
			break;
		case 'F':
			extraction = file_extract;
			break;
		default:
			WCSTOMBS(mbs_buffer, string.buffer.start);
			fatal_reader_mksh(gettext("Illegal macro reference `%s'"),
				     mbs_buffer);
		}
		/* Internalize the macro name using the first char only. */
		name = GETNAME(string.buffer.start, 1);
		(void) wcscpy(string.buffer.start, string.buffer.start + 2);
	}
	/* Check for other kinds of translations. */
	if ((colon = (wchar_t *) wcschr(string.buffer.start,
				       (int) colon_char)) != NULL) {
		/*
		 * We have a $(FOO:.c=.o) type translation.
		 * Get the name of the macro proper.
		 */
		if (name == NULL) {
			name = GETNAME(string.buffer.start,
				       colon - string.buffer.start);
		}
		/* Pickup all the translations. */
		if (IS_WEQUAL(colon, colon_sh) || IS_WEQUAL(colon, colon_shell)) {
			replacement = sh_replace;
		} else if ((svr4) ||
		           ((percent = (wchar_t *) wcschr(colon + 1,
							 (int) percent_char)) == NULL)) {
			while (colon != NULL) {
				if ((eq = (wchar_t *) wcschr(colon + 1,
							    (int) equal_char)) == NULL) {
					fatal_reader_mksh(gettext("= missing from replacement macro reference"));
				}
				left_tail_len = eq - colon - 1;
				if(left_tail) {
					retmem(left_tail);
				}
				left_tail = ALLOC_WC(left_tail_len + 1);
				(void) wcsncpy(left_tail,
					      colon + 1,
					      eq - colon - 1);
				left_tail[eq - colon - 1] = (int) nul_char;
				replacement = suffix_replace;
				if ((colon = (wchar_t *) wcschr(eq + 1,
							       (int) colon_char)) != NULL) {
					tmp_len = colon - eq;
					if(right_tail) {
						retmem(right_tail);
					}
					right_tail = ALLOC_WC(tmp_len);
					(void) wcsncpy(right_tail,
						      eq + 1,
						      colon - eq - 1);
					right_tail[colon - eq - 1] =
					  (int) nul_char;
				} else {
					if(right_tail) {
						retmem(right_tail);
					}
					right_tail = ALLOC_WC(wcslen(eq) + 1);
					(void) wcscpy(right_tail, eq + 1);
				}
			}
		} else {
			if ((eq = (wchar_t *) wcschr(colon + 1,
						    (int) equal_char)) == NULL) {
				fatal_reader_mksh(gettext("= missing from replacement macro reference"));
			}
			if ((percent = (wchar_t *) wcschr(colon + 1,
							 (int) percent_char)) == NULL) {
				fatal_reader_mksh(gettext("%% missing from replacement macro reference"));
			}
			if (eq < percent) {
				fatal_reader_mksh(gettext("%% missing from replacement macro reference"));
			}

			if (percent > (colon + 1)) {
				tmp_len = percent - colon;
				if(left_head) {
					retmem(left_head);
				}
				left_head = ALLOC_WC(tmp_len);
				(void) wcsncpy(left_head,
					      colon + 1,
					      percent - colon - 1);
				left_head[percent-colon-1] = (int) nul_char;
				left_head_len = percent-colon-1;
			} else {
				left_head = NULL;
				left_head_len = 0;
			}

			if (eq > percent+1) {
				tmp_len = eq - percent;
				if(left_tail) {
					retmem(left_tail);
				}
				left_tail = ALLOC_WC(tmp_len);
				(void) wcsncpy(left_tail,
					      percent + 1,
					      eq - percent - 1);
				left_tail[eq-percent-1] = (int) nul_char;
				left_tail_len = eq-percent-1;
			} else {
				left_tail = NULL;
				left_tail_len = 0;
			}

			if ((percent = (wchar_t *) wcschr(++eq,
							 (int) percent_char)) == NULL) {

				right_hand[0] = ALLOC_WC(wcslen(eq) + 1);
				right_hand[1] = NULL;
				(void) wcscpy(right_hand[0], eq);
			} else {
				i = 0;
				do {
					right_hand[i] = ALLOC_WC(percent-eq+1);
					(void) wcsncpy(right_hand[i],
						      eq,
						      percent - eq);
					right_hand[i][percent-eq] =
					  (int) nul_char;
					if (i++ >= (int) VSIZEOF(right_hand)) {
						fatal_mksh(gettext("Too many %% in pattern"));
					}
					eq = percent + 1;
					if (eq[0] == (int) nul_char) {
						MBSTOWCS(wcs_buffer, "");
						right_hand[i] = (wchar_t *) wcsdup(wcs_buffer);
						i++;
						break;
					}
				} while ((percent = (wchar_t *) wcschr(eq, (int) percent_char)) != NULL);
				if (eq[0] != (int) nul_char) {
					right_hand[i] = ALLOC_WC(wcslen(eq) + 1);
					(void) wcscpy(right_hand[i], eq);
					i++;
				}
				right_hand[i] = NULL;
			}
			replacement = pattern_replace;
		}
	}
	if (name == NULL) {
		/*
		 * No translations found.
		 * Use the whole string as the macro name.
		 */
		name = GETNAME(string.buffer.start,
			       string.text.p - string.buffer.start);
	}
	if (string.free_after_use) {
		retmem(string.buffer.start);
	}
	if (name == make) {
		make_word_mentioned = true;
	}
	if (name == query) {
		query_mentioned = true;
	}
	if ((name == host_arch) || (name == target_arch)) {
		if (!init_arch_done) {
			init_arch_done = true;
			init_arch_macros();
		}
	}
	if ((name == host_mach) || (name == target_mach)) {
		if (!init_mach_done) {
			init_mach_done = true;
			init_mach_macros();
		}
	}
	/* Get the macro value. */
	macro = get_prop(name->prop, macro_prop);
#ifdef NSE
        if (nse_watch_vars && nse && macro != NULL) {
                if (macro->body.macro.imported) {
                        nse_shell_var_used= name;
		}
                if (macro->body.macro.value != NULL){
	              if (nse_backquotes(macro->body.macro.value->string)) {
	                       nse_backquote_seen= name;
		      }
	       }
	}
#endif
	if ((macro != NULL) && macro->body.macro.is_conditional) {
		conditional_macro_used = true;
		/*
		 * Add this conditional macro to the beginning of the
		 * global list.
		 */
		add_macro_to_global_list(name);
		if (makefile_type == reading_makefile) {
			warning_mksh(gettext("Conditional macro `%s' referenced in file `%ws', line %d"),
					name->string_mb, file_being_read, line_number);
		}
	}
	/* Macro name read and parsed. Expand the value. */
	if ((macro == NULL) || (macro->body.macro.value == NULL)) {
		/* If the value is empty, we just get out of here. */
		goto exit;
	}
	if (replacement == sh_replace) {
		/* If we should do a :sh transform, we expand the command
		 * and process it.
		 */
		INIT_STRING_FROM_STACK(string, buffer);
		/* Expand the value into a local string buffer and run cmd. */
		if (exp_type == no_expand && name->stat.macro_type == gnu_assign)
			expand_none(macro->body.macro.value, destination);
		else
			expand_value_with_daemon(name, macro, &string, cmd, exp_type);
		sh_command2string(&string, destination);
	} else if ((replacement != no_replace) || (extraction != no_extract)) {
		/*
		 * If there were any transforms specified in the macro
		 * name, we deal with them here.
		 */
		INIT_STRING_FROM_STACK(string, buffer);
		/* Expand the value into a local string buffer. */
		if (exp_type == no_expand && name->stat.macro_type == gnu_assign)
			expand_none(macro->body.macro.value, destination);
		else
			expand_value_with_daemon(name, macro, &string, cmd, exp_type);
		/* Scan the expanded string. */
		p = string.buffer.start;
		while (*p != (int) nul_char) {
			wchar_t		chr;

			/*
			 * First skip over any white space and append
			 * that to the destination string.
			 */
			block_start = p;
			while ((*p != (int) nul_char) && iswspace(*p)) {
				p++;
			}
			append_string(block_start,
				      destination,
				      p - block_start);
			/* Then find the end of the next word. */
			block_start = p;
			while ((*p != (int) nul_char) && !iswspace(*p)) {
				p++;
			}
			/* If we cant find another word we are done */
			if (block_start == p) {
				break;
			}
			/* Then apply the transforms to the word */
			INIT_STRING_FROM_STACK(extracted, extracted_string);
			switch (extraction) {
			case dir_extract:
				/*
				 * $(@D) type transform. Extract the
				 * path from the word. Deliver "." if
				 * none is found.
				 */
				if (p != NULL) {
					chr = *p;
					*p = (int) nul_char;
				}
				eq = (wchar_t *) wcsrchr(block_start, (int) slash_char);
				if (p != NULL) {
					*p = chr;
				}
				if ((eq == NULL) || (eq > p)) {
					MBSTOWCS(wcs_buffer, ".");
					append_string(wcs_buffer, &extracted, 1);
				} else {
					append_string(block_start,
						      &extracted,
						      eq - block_start);
				}
				break;
			case file_extract:
				/*
				 * $(@F) type transform. Remove the path
				 * from the word if any.
				 */
				if (p != NULL) {
					chr = *p;
					*p = (int) nul_char;
				}
				eq = (wchar_t *) wcsrchr(block_start, (int) slash_char);
				if (p != NULL) {
					*p = chr;
				}
				if ((eq == NULL) || (eq > p)) {
					append_string(block_start,
						      &extracted,
						      p - block_start);
				} else {
					append_string(eq + 1,
						      &extracted,
						      p - eq - 1);
				}
				break;
			case no_extract:
				append_string(block_start,
					      &extracted,
					      p - block_start);
				break;
			}
			switch (replacement) {
			case suffix_replace:
				/*
				 * $(FOO:.o=.c) type transform.
				 * Maybe replace the tail of the word.
				 */
				if (((extracted.text.p -
				      extracted.buffer.start) >=
				     left_tail_len) &&
				    IS_WEQUALN(extracted.text.p - left_tail_len,
					      left_tail,
					      left_tail_len)) {
					append_string(extracted.buffer.start,
						      destination,
						      (extracted.text.p -
						       extracted.buffer.start)
						      - left_tail_len);
					append_string(right_tail,
						      destination,
						      FIND_LENGTH);
				} else {
					append_string(extracted.buffer.start,
						      destination,
						      FIND_LENGTH);
				}
				break;
			case pattern_replace:
				/* $(X:a%b=c%d) type transform. */
				if (((extracted.text.p -
				      extracted.buffer.start) >=
				     left_head_len+left_tail_len) &&
				    IS_WEQUALN(left_head,
					      extracted.buffer.start,
					      left_head_len) &&
				    IS_WEQUALN(left_tail,
					      extracted.text.p - left_tail_len,
					      left_tail_len)) {
					i = 0;
					while (right_hand[i] != NULL) {
						append_string(right_hand[i],
							      destination,
							      FIND_LENGTH);
						i++;
						if (right_hand[i] != NULL) {
							append_string(extracted.buffer.
								      start +
								      left_head_len,
								      destination,
								      (extracted.text.p - extracted.buffer.start)-left_head_len-left_tail_len);
						}
					}
				} else {
					append_string(extracted.buffer.start,
						      destination,
						      FIND_LENGTH);
				}
				break;
			case no_replace:
				append_string(extracted.buffer.start,
					      destination,
					      FIND_LENGTH);
				break;
			case sh_replace:
				break;
			    }
		}
		if (string.free_after_use) {
			retmem(string.buffer.start);
		}
	} else {
		/*
		 * This is for the case when the macro name did not
		 * specify transforms.
		 */
		if (!strncmp(name->string_mb, NOCATGETS("GET"), 3)) {
			dollarget_seen = true;
		}
		dollarless_flag = false;
		if (!strncmp(name->string_mb, "<", 1) &&
		    dollarget_seen) {
			dollarless_flag = true;
			dollarget_seen = false;
		}
			if (exp_type == no_expand && name->stat.macro_type == gnu_assign)
				expand_none(macro->body.macro.value, destination);
			else
				expand_value_with_daemon(name, macro, destination, cmd, exp_type);
	}
exit:
	if(left_tail) {
		retmem(left_tail);
	}
	if(right_tail) {
		retmem(right_tail);
	}
	if(left_head) {
		retmem(left_head);
	}
	i = 0;
	while (right_hand[i] != NULL) {
		retmem(right_hand[i]);
		i++;
	}
	*destination->text.p = (int) nul_char;
	destination->text.end = destination->text.p;
}

static void
add_macro_to_global_list(Name macro_to_add)
{
	Macro_list	new_macro;
	Macro_list	macro_on_list;
	char		*name_on_list = (char*)NULL;
	char		*name_to_add = macro_to_add->string_mb;
	char		*value_on_list = (char*)NULL;
	char		*value_to_add = (char*)NULL;

	if (macro_to_add->prop->body.macro.value != NULL) {
		value_to_add = macro_to_add->prop->body.macro.value->string_mb;
	} else {
		value_to_add = (char *)"";
	}

	/* 
	 * Check if this macro is already on list, if so, do nothing
	 */
	for (macro_on_list = cond_macro_list;
	     macro_on_list != NULL;
	     macro_on_list = macro_on_list->next) {

		name_on_list = macro_on_list->macro_name;
		value_on_list = macro_on_list->value;

		if (IS_EQUAL(name_on_list, name_to_add)) {
			if (IS_EQUAL(value_on_list, value_to_add)) {
				return;
			}
		}
	}
	new_macro = (Macro_list) malloc(sizeof(Macro_list_rec));
	new_macro->macro_name = strdup(name_to_add);
	new_macro->value = strdup(value_to_add);
	new_macro->next = cond_macro_list;
	cond_macro_list = new_macro;
}

/*
 *	init_arch_macros(void)
 *
 *	Set the magic macros TARGET_ARCH, HOST_ARCH,
 *
 *	Parameters: 
 *
 *	Global variables used:
 * 	                        host_arch   Property for magic macro HOST_ARCH
 * 	                        target_arch Property for magic macro TARGET_ARCH
 *
 *	Return value:
 *				The function does not return a value, but can
 *				call fatal() in case of error.
 */
static void
init_arch_macros(void)
{
	String_rec	result_string;
	wchar_t		wc_buf[STRING_BUFFER_LENGTH];
	char		mb_buf[STRING_BUFFER_LENGTH];
	FILE		*pipe;
	Name		value;
	int		set_host, set_target;
#ifdef NSE
	Property	macro;
#endif
#if !defined(__sun)
	const char	*mach_command = NOCATGETS("/bin/uname -p");
#else
	const char	*mach_command = NOCATGETS("/bin/mach");
#endif

	set_host = (get_prop(host_arch->prop, macro_prop) == NULL);
	set_target = (get_prop(target_arch->prop, macro_prop) == NULL);

	if (set_host || set_target) {
		INIT_STRING_FROM_STACK(result_string, wc_buf);
		append_char((int) hyphen_char, &result_string);

		if ((pipe = popen(mach_command, "r")) == NULL) {
			fatal_mksh(gettext("Execute of %s failed"), mach_command);
		}
		while (fgets(mb_buf, sizeof(mb_buf), pipe) != NULL) {
			MBSTOWCS(wcs_buffer, mb_buf);
			append_string(wcs_buffer, &result_string, wcslen(wcs_buffer));
		}
		if (pclose(pipe) != 0) {
			fatal_mksh(gettext("Execute of %s failed"), mach_command);
		}

		value = GETNAME(result_string.buffer.start, wcslen(result_string.buffer.start));

#ifdef NSE
	        macro = setvar_daemon(host_arch, value, false, no_daemon, true, 0);
	        macro->body.macro.imported= true;
	        macro = setvar_daemon(target_arch, value, false, no_daemon, true, 0);
	        macro->body.macro.imported= true;
#else
		if (set_host) {
			(void) setvar_daemon(host_arch, value, false, no_daemon, true, 0);
		}
		if (set_target) {
			(void) setvar_daemon(target_arch, value, false, no_daemon, true, 0);
		}
#endif
	}
}

/*
 *	init_mach_macros(void)
 *
 *	Set the magic macros TARGET_MACH, HOST_MACH,
 *
 *	Parameters: 
 *
 *	Global variables used:
 * 	                        host_mach   Property for magic macro HOST_MACH
 * 	                        target_mach Property for magic macro TARGET_MACH
 *
 *	Return value:
 *				The function does not return a value, but can
 *				call fatal() in case of error.
 */
static void
init_mach_macros(void)
{
	String_rec	result_string;
	wchar_t		wc_buf[STRING_BUFFER_LENGTH];
	char		mb_buf[STRING_BUFFER_LENGTH];
	FILE		*pipe;
	Name		value;
	int		set_host, set_target;
#if !defined(__sun)
	const char	*arch_command = NOCATGETS("/bin/uname -m");
#else
	const char	*arch_command = NOCATGETS("/bin/arch");
#endif

	set_host = (get_prop(host_mach->prop, macro_prop) == NULL);
	set_target = (get_prop(target_mach->prop, macro_prop) == NULL);

	if (set_host || set_target) {
		INIT_STRING_FROM_STACK(result_string, wc_buf);
		append_char((int) hyphen_char, &result_string);

		if ((pipe = popen(arch_command, "r")) == NULL) {
			fatal_mksh(gettext("Execute of %s failed"), arch_command);
		}
		while (fgets(mb_buf, sizeof(mb_buf), pipe) != NULL) {
			MBSTOWCS(wcs_buffer, mb_buf);
			append_string(wcs_buffer, &result_string, wcslen(wcs_buffer));
		}
		if (pclose(pipe) != 0) {
			fatal_mksh(gettext("Execute of %s failed"), arch_command);
		}

		value = GETNAME(result_string.buffer.start, wcslen(result_string.buffer.start));

		if (set_host) {
			(void) setvar_daemon(host_mach, value, false, no_daemon, true, 0);
		}
		if (set_target) {
			(void) setvar_daemon(target_mach, value, false, no_daemon, true, 0);
		}
	}
}

/*
 *	expand_value_with_daemon(name, macro, destination, cmd, exp_type)
 *
 *	Checks for daemons and then maybe calls expand_value().
 *
 *	Parameters:
 *              name            Name of the macro  (Added by the NSE)
 *		macro		The property block with the value to expand
 *		destination	Where the result should be deposited
 *		cmd		If we are evaluating a command line we
 *				turn \ quoting off
 *		exp_type	The expansion type by default is deflt_expand
 *				but it may be:
 *
 *				no_expand	If macros of type gnu_assign
 *						should not be expanded.
 *				keep_ddollar	If we are in a :::= or +:=
 *						assignment and $$ should be
 *						left untouched.
 *
 *	Global variables used:
 */
static void
#ifdef NSE
expand_value_with_daemon(Name name, Property macro, String destination, Boolean cmd, Expand_Type exp_type)
#else
expand_value_with_daemon(Name, Property macro, String destination, Boolean cmd, Expand_Type exp_type)
#endif
{
	Chain			chain;

#ifdef NSE
        if (reading_dependencies) {
                /*
                 * Processing the dependencies themselves
                 */
                depvar_dep_macro_used(name);
	} else {
                /*
	         * Processing the rules for the targets
	         * the nse_watch_vars flags chokes off most
	         * checks.  it is true only when processing
	         * the output from a recursive make run
	         * which is all we are interested in here.
	         */
	         if (nse_watch_vars) {
	                depvar_rule_macro_used(name);
		}
	 }
#endif

	switch (macro->body.macro.daemon) {
	case no_daemon:
		if (!svr4 && !posix) {
			expand_value(macro->body.macro.value, destination, cmd, exp_type);
		} else {
			if (dollarless_flag && tilde_rule) {
				expand_value(dollarless_value, destination, cmd, exp_type);
				dollarless_flag = false;
				tilde_rule = false;
			} else {
				expand_value(macro->body.macro.value, destination, cmd, exp_type);
			}
		}
		return;
	case chain_daemon:
		/* If this is a $? value we call the daemon to translate the */
		/* list of names to a string */
		for (chain = (Chain) macro->body.macro.value;
		     chain != NULL;
		     chain = chain->next) {
			APPEND_NAME(chain->name,
				      destination,
				      (int) chain->name->hash.length);
			if (chain->next != NULL) {
				append_char((int) space_char, destination);
			}
		}
		return;
	}
}

/*
 * We use a permanent buffer to reset SUNPRO_DEPENDENCIES value.
 */
char	*sunpro_dependencies_buf = NULL;
char	*sunpro_dependencies_oldbuf = NULL;
int	sunpro_dependencies_buf_size = 0;

/*
 *	setvar_daemon(name, value, append, daemon, strip_trailing_spaces)
 *
 *	Set a macro value, possibly supplying a daemon to be used
 *	when referencing the value.
 *
 *	Return value:
 *				The property block with the new value
 *
 *	Parameters:
 *		name		Name of the macro to set
 *		value		The value to set
 *		append		Should we reset or append to the current value?
 *		daemon		Special treatment when reading the value
 *		strip_trailing_spaces from the end of value->string
 *		debug_lvl	Indicates how much tracing we should do
 *
 *	Global variables used:
 *		makefile_type	Used to check if we should enforce read only
 *		path_name	The Name "PATH", compared against
 *		virtual_root	The Name "VIRTUAL_ROOT", compared against
 *		vpath_defined	Set if the macro VPATH is set
 *		vpath_name	The Name "VPATH", compared against
 *		envvar		A list of environment vars with $ in value
 */
Property
setvar_daemon(Name name, Name value, Boolean append, Daemon daemon, Boolean strip_trailing_spaces, short debug_lvl)
{
	Property		macro = maybe_append_prop(name, macro_prop);
	Property		macro_apx = get_prop(name->prop, macro_append_prop);
	int			length = 0;
	String_rec		destination;
	wchar_t			buffer[STRING_BUFFER_LENGTH];
	Chain			chain;
	Name			val;
	wchar_t			*val_string = (wchar_t*)NULL;
	Wstring			wcb;

#ifdef NSE
        macro->body.macro.imported = false;
#endif

	if ((makefile_type != reading_nothing) &&
	    macro->body.macro.read_only) {
		return macro;
	}
	/* Strip spaces from the end of the value */
	if (daemon == no_daemon) {
		if(value != NULL) {
			wcb.init(value);
			length = wcb.length();
			val_string = wcb.get_string();
		}
		if ((length > 0) && iswspace(val_string[length-1])) {
			INIT_STRING_FROM_STACK(destination, buffer);
			buffer[0] = 0;
			append_string(val_string, &destination, length);
			if (strip_trailing_spaces) {
				while ((length > 0) &&
				       iswspace(destination.buffer.start[length-1])) {
					destination.buffer.start[--length] = 0;
				}
			}
			value = GETNAME(destination.buffer.start, FIND_LENGTH);
		}
	}
		
	if(macro_apx != NULL) {
		val = macro_apx->body.macro_appendix.value;
	} else {
		val = macro->body.macro.value;
	}

	if (append) {
		/*
		 * If we are appending, we just tack the new value after
		 * the old one with a space in between.
		 */
		INIT_STRING_FROM_STACK(destination, buffer);
		buffer[0] = 0;
		if ((macro != NULL) && (val != NULL)) {
			APPEND_NAME(val,
				      &destination,
				      (int) val->hash.length);
			if (value != NULL) {
				wcb.init(value);
				if(wcb.length() > 0) {
					MBTOWC(wcs_buffer, " ");
					append_char(wcs_buffer[0], &destination);
				}
			}
		}
		if (value != NULL) {
			APPEND_NAME(value,
				      &destination,
				      (int) value->hash.length);
		}
		value = GETNAME(destination.buffer.start, FIND_LENGTH);
		wcb.init(value);
		if (destination.free_after_use) {
			retmem(destination.buffer.start);
		}
	}

	/* Debugging trace */
	if (debug_lvl > 1) {
		if (value != NULL) {
			switch (daemon) {
			case chain_daemon:
				(void) printf("%s =", name->string_mb);
				for (chain = (Chain) value;
				     chain != NULL;
				     chain = chain->next) {
					(void) printf(" %s", chain->name->string_mb);
				}
				(void) printf("\n");
				break;
			case no_daemon:
				(void) printf("%s= %s\n",
					      name->string_mb,
					      value->string_mb);
				break;
			}
		} else {
			(void) printf("%s =\n", name->string_mb);
		}
	}
	/* Set the new values in the macro property block */
/**/
	if(macro_apx != NULL) {
		macro_apx->body.macro_appendix.value = value;
		INIT_STRING_FROM_STACK(destination, buffer);
		buffer[0] = 0;
		if (value != NULL) {
			APPEND_NAME(value,
				      &destination,
				      (int) value->hash.length);
			if (macro_apx->body.macro_appendix.value_to_append != NULL) {
				MBTOWC(wcs_buffer, " ");
				append_char(wcs_buffer[0], &destination);
			}
		}
		if (macro_apx->body.macro_appendix.value_to_append != NULL) {
			APPEND_NAME(macro_apx->body.macro_appendix.value_to_append,
				      &destination,
				      (int) macro_apx->body.macro_appendix.value_to_append->hash.length);
		}
		value = GETNAME(destination.buffer.start, FIND_LENGTH);
		if (destination.free_after_use) {
			retmem(destination.buffer.start);
		}
	}
/**/
	macro->body.macro.value = value;
	macro->body.macro.daemon = daemon;
	/*
	 * If the user changes the VIRTUAL_ROOT, we need to flush
	 * the vroot package cache.
	 */
	if (name == path_name) {
		flush_path_cache();
	}
	if (name == virtual_root) {
		flush_vroot_cache();
	}
	/* If this sets the VPATH we remember that */
	if ((name == vpath_name) &&
	    (value != NULL) &&
	    (value->hash.length > 0)) {
		vpath_defined = true;
	}
	/*
	 * For environment variables we also set the
	 * environment value each time.
	 */
	if (macro->body.macro.exported) {
		static char	*env;

#ifdef DISTRIBUTED
		if (!reading_environment && (value != NULL)) {
#else
		if (!reading_environment && (value != NULL) && value->dollar) {
#endif
			Envvar	p;

			for (p = envvar; p != NULL; p = p->next) {
				if (p->name == name) {
					p->value = value;
					p->already_put = false;
					goto found_it;
				}
			}
			p = ALLOC(Envvar);
			p->name = name;
			p->value = value;
			p->next = envvar;
			p->env_string = NULL;
			p->already_put = false;
			envvar = p;
found_it:;
#ifdef DISTRIBUTED
		}
		if (reading_environment || (value == NULL) || !value->dollar) {
#else
		} else {
#endif
			length = 2 + strlen(name->string_mb);
			if (value != NULL) {
				length += strlen(value->string_mb);
			}
			Property env_prop = maybe_append_prop(name, env_mem_prop);
			/*
			 * We use a permanent buffer to reset SUNPRO_DEPENDENCIES value.
			 */
			if (!strncmp(name->string_mb, NOCATGETS("SUNPRO_DEPENDENCIES"), 19)) {
				if (length >= sunpro_dependencies_buf_size) {
					sunpro_dependencies_buf_size=length*2;
					if (sunpro_dependencies_buf_size < 4096)
						sunpro_dependencies_buf_size = 4096; // Default minimum size
					if (sunpro_dependencies_buf)
						sunpro_dependencies_oldbuf = sunpro_dependencies_buf;
					sunpro_dependencies_buf=getmem(sunpro_dependencies_buf_size);
				}
				env = sunpro_dependencies_buf;
			} else {
				env = getmem(length);
			}
			env_alloc_num++;
			env_alloc_bytes += length;
			(void) sprintf(env,
				       "%s=%s",
				       name->string_mb,
				       value == NULL ?
			                 "" : value->string_mb);
			(void) putenv(env);
			env_prop->body.env_mem.value = env;
			if (sunpro_dependencies_oldbuf) {
				/* Return old buffer */
				retmem_mb(sunpro_dependencies_oldbuf);
				sunpro_dependencies_oldbuf = NULL;
			}
		}
	}
	if (name == target_arch) {
		Name		ha = getvar(host_arch);
		Name		ta = getvar(target_arch);
		Name		vr = getvar(virtual_root);
		size_t		len;
		wchar_t		*new_value;
		wchar_t		*old_vr;
		Boolean		new_value_allocated = false;

		Wstring		ha_str(ha);
		Wstring		ta_str(ta);
		Wstring		vr_str(vr);

		wchar_t * wcb_ha = ha_str.get_string();
		wchar_t * wcb_ta = ta_str.get_string();
		wchar_t * wcb_vr = vr_str.get_string();

		len = 32 +
		  wcslen(wcb_ha) +
		    wcslen(wcb_ta) +
		      wcslen(wcb_vr);
		old_vr = wcb_vr;
		MBSTOWCS(wcs_buffer, NOCATGETS("/usr/arch/"));
		if (IS_WEQUALN(old_vr,
			       wcs_buffer,
			       wcslen(wcs_buffer))) {
			old_vr = (wchar_t *) wcschr(old_vr, (int) colon_char) + 1;
		}
		if ( (ha == ta) || (wcslen(wcb_ta) == 0) ) {
			new_value = old_vr;
		} else {
			new_value = ALLOC_WC(len);
			new_value_allocated = true;
			WCSTOMBS(mbs_buffer, old_vr);
#ifdef	__use_sun_wsprintf__
			(void) wsprintf(new_value,
				        NOCATGETS("/usr/arch/%s/%s:%s"),
				        ha->string_mb + 1,
				        ta->string_mb + 1,
				        mbs_buffer);
#else
			char * mbs_new_value = (char *)getmem(len);
			(void) sprintf(mbs_new_value,
				        NOCATGETS("/usr/arch/%s/%s:%s"),
				        ha->string_mb + 1,
				        ta->string_mb + 1,
				        mbs_buffer);
			MBSTOWCS(new_value, mbs_new_value);
			retmem_mb(mbs_new_value);
#endif
		}
		if (new_value[0] != 0) {
			(void) setvar_daemon(virtual_root,
					     GETNAME(new_value, FIND_LENGTH),
					     false,
					     no_daemon,
					     true,
					     debug_lvl);
		}
		if (new_value_allocated) {
			retmem(new_value);
		}
	}
	return macro;
}
