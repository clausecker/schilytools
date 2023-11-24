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
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)dosys.cc 1.45 06/12/12
 */

#pragma	ident	"@(#)dosys.cc	1.45	06/12/12"

/*
 * Copyright 2017-2018 J. Schilling
 * Copyright 2022 the schilytools team
 *
 * @(#)dosys.cc	1.5 21/08/15 2017-2018 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)dosys.cc	1.5 21/08/15 2017-2018 J. Schilling";
#endif

/*
 *	dosys.cc
 *
 *	Execute one commandline
 */

/*
 * Included files
 */
#include <mk/defs.h>
#include <mksh/dosys.h>		/* doshell(), doexec() */
#include <mksh/misc.h>		/* getmem() */

/*
 * Defined macros
 */

/*
 * typedefs & structs
 */

/*
 * Static variables
 */

/*
 * File table of contents
 */

/*
 *	dosys(command, ignore_error, call_make, silent_error, target)
 *
 *	Check if command string contains meta chars and dispatch to
 *	the proper routine for executing one command line.
 *
 *	Return value:
 *				Indicates if the command execution failed
 *
 *	Parameters:
 *		command		The command to run
 *		ignore_error	Should make abort when an error is seen?
 *		call_make	Did command reference $(MAKE) ?
 *		silent_error	Should error messages be suppressed for pmake?
 *		target		Target we are building
 *
 *	Global variables used:
 *		do_not_exec_rule Is -n on?
 *		working_on_targets We started processing real targets
 */
Doname
dosys(Name command, Boolean ignore_error, Boolean call_make, Boolean silent_error, Boolean always_exec, Name target, Boolean redirect_out_err)
{
	timestruc_t		before;
	int		length = command->hash.length;
	Wstring			wcb(command);
	wchar_t	*p = wcb.get_string();
	wchar_t	*q;
	Doname			result;

	/* Strip spaces from head of command string */
	while (iswspace(*p)) {
		p++, length--;
	}
	if (*p == (int) nul_char) {
		return build_failed;
	}
	/* If we are faking it we just return */
	if (do_not_exec_rule &&
	    working_on_targets &&
	    !call_make &&
	    !always_exec) {
		return build_ok;
	}
	/* no_action_was_taken is used to print special message */
	no_action_was_taken = false;

	/* Copy string to make it OK to write it. */
	q = ALLOC_WC(length + 1);
	(void) wcscpy(q, p);
	/* Write the state file iff this command uses make. */
	if (call_make && command_changed) {
		write_state_file(0, false);
	}
	make_state->stat.time = file_no_time;
	(void)exists(make_state);
	before = make_state->stat.time;
	/*
	 * Run command directly if it contains no shell meta chars,
	 * else run it using the shell.
	 */
	if (await(ignore_error,
		  silent_error,
		  target,
		  wcb.get_string(),
                  command->meta ?
	            doshell(q, ignore_error, redirect_out_err,
			    stdout_file, stderr_file, 0) :
	            doexec(q, ignore_error, redirect_out_err,
		 	   stdout_file, stderr_file,
	                   vroot_path, 0),
	          send_mtool_msgs,
#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
                  get_xdrs_ptr(),
                  get_job_msg_id()
#else
                  NULL,
                  -1
#endif
	          )) {
		result = build_ok;
	} else {
		result = build_failed;
	}
	retmem(q);

	if ((report_dependencies_level == 0) &&
	    call_make) {
		make_state->stat.time = file_no_time;
		(void)exists(make_state);
		if (before == make_state->stat.time) {
			return result;
		}
		makefile_type = reading_statefile;
		if (read_trace_level > 1) {
			trace_reader = true;
		}
		temp_file_number++;
		(void) read_simple_file(make_state,
					false,
					false,
					false,
					false,
					false,
					true);
		trace_reader = false;
	}
	return result;
}
