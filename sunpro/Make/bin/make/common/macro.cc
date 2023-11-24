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
 * @(#)macro.cc 1.28 06/12/12
 */

#pragma	ident	"@(#)macro.cc	1.28	06/12/12"

/*
 * Copyright 2017-2018 J. Schilling
 * Copyright 2022, 2023 the schilytools team
 *
 * @(#)macro.cc	1.4 21/08/15 2017-2018 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)macro.cc	1.4 21/08/15 2017-2018 J. Schilling";
#endif

/*
 *	macro.cc
 *
 *	Handle expansion of make macros
 */

/*
 * Included files
 */
#ifdef DISTRIBUTED
#include <avo/strings.h>	/* AVO_STRDUP() */
#include <dm/Avo_DoJobMsg.h>
#endif
#include <mk/defs.h>
#include <mksh/macro.h>		/* getvar(), expand_value() */
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

void
setvar_append(Name name, Name value)
{
	Property		macro_apx = get_prop(name->prop, macro_append_prop);
	Property		macro = get_prop(name->prop, macro_prop);
	String_rec		destination;
	wchar_t			buffer[STRING_BUFFER_LENGTH];
	Name			val = NULL;

	if(macro_apx == NULL) {
		macro_apx = append_prop(name, macro_append_prop);
		if(macro != NULL) {
			macro_apx->body.macro_appendix.value = macro->body.macro.value;
		}
	}

	val = macro_apx->body.macro_appendix.value_to_append;

	INIT_STRING_FROM_STACK(destination, buffer);
	buffer[0] = 0;
	if (val != NULL) {
		APPEND_NAME(val,
			      &destination,
			      (int) val->hash.length);
		if (value != NULL) {
			MBTOWC(wcs_buffer, " ");
			append_char(wcs_buffer[0], &destination);
		}
	}
	if (value != NULL) {
		APPEND_NAME(value,
			      &destination,
			      (int) value->hash.length);
	}
	value = GETNAME(destination.buffer.start, FIND_LENGTH);
	if (destination.free_after_use) {
		retmem(destination.buffer.start);
	}
	macro_apx->body.macro_appendix.value_to_append = value;

	SETVAR(name, empty_name, true);
}

/*
 *	setvar_envvar()
 *
 *	This function scans the list of environment variables that have
 *	dynamic values and sets them.
 *
 *	Parameters:
 *
 *	Global variables used:
 *		envvar		A list of environment vars with $ in value
 */
void
#ifdef DISTRIBUTED
setvar_envvar(Avo_DoJobMsg *dmake_job_msg)
#else
setvar_envvar(void)
#endif
{
	wchar_t			buffer[STRING_BUFFER_LENGTH];
	int			length;
#ifdef DISTRIBUTED
	Property		macro;
#endif
	char			*mbs, *tmp_mbs_buffer = NULL;
	char			*env, *tmp_mbs_buffer2 = NULL;
	Envvar			p;
	String_rec		value;

	for (p = envvar; p != NULL; p = p->next) {
		if (p->already_put
#ifdef DISTRIBUTED
		    && !dmake_job_msg
#endif
		    ) {
			continue;
		}
		INIT_STRING_FROM_STACK(value, buffer);		
		expand_value(p->value, &value, false);
		if ((length = wcslen(value.buffer.start)) >= MAXPATHLEN) {
			mbs = tmp_mbs_buffer = getmem((length + 1) * MB_LEN_MAX);
			(void) wcstombs(mbs,
			                value.buffer.start,
			                (length + 1) * MB_LEN_MAX);
		} else {
			mbs = mbs_buffer;
			WCSTOMBS(mbs, value.buffer.start);
		}
		length = 2 + strlen(p->name->string_mb) + strlen(mbs);
		if (!p->already_put || length > (MAXPATHLEN * MB_LEN_MAX)) {
			env = tmp_mbs_buffer2 = getmem(length);
		} else {
			env = mbs_buffer2;
		}
		(void) sprintf(env,
			       "%s=%s",
			       p->name->string_mb,
			       mbs);
		if (!p->already_put) {
			(void) putenv(env);
			p->already_put = true;
			if (p->env_string) {
				retmem_mb(p->env_string);
			}
			p->env_string = env;
			tmp_mbs_buffer2 = NULL; // We should not return this memory now
		}
#ifdef DISTRIBUTED
		if (dmake_job_msg) {
			dmake_job_msg->appendVar(env);
		}
#endif
		if (tmp_mbs_buffer2) {
			retmem_mb(tmp_mbs_buffer2);
			tmp_mbs_buffer2 = NULL;
		}
		if (tmp_mbs_buffer) {
			retmem_mb(tmp_mbs_buffer);
			tmp_mbs_buffer = NULL;
		}
	}
#ifdef DISTRIBUTED
	/* Append SUNPRO_DEPENDENCIES to the dmake_job_msg. */
	if (keep_state && dmake_job_msg) {
		macro = get_prop(sunpro_dependencies->prop, macro_prop);
		length = 2 +
		         strlen(sunpro_dependencies->string_mb) +
		         strlen(macro->body.macro.value->string_mb);
		if (length > (MAXPATHLEN * MB_LEN_MAX)) {
			env = tmp_mbs_buffer2 = getmem(length);
		} else {
			env = mbs_buffer2;
		}
		(void) sprintf(env,
			       "%s=%s",
			       sunpro_dependencies->string_mb,
			       macro->body.macro.value->string_mb);
		dmake_job_msg->appendVar(env);
		if (tmp_mbs_buffer2) {
			retmem_mb(tmp_mbs_buffer2);
			tmp_mbs_buffer2 = NULL;
		}
	}
#endif
}


