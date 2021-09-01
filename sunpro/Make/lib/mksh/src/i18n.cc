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
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)i18n.cc 1.3 06/12/12
 */

#pragma	ident	"@(#)i18n.cc	1.3	06/12/12"

/*
 * Copyright 2017 J. Schilling
 *
 * @(#)i18n.cc	1.4 21/08/16 2017 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)i18n.cc	1.4 21/08/16 2017 J. Schilling";
#endif

/*
 *      i18n.cc
 *
 *      Deal with internationalization conversions
 */

/*
 * Included files
 */
#include <mksh/defs.h>
#include <mksh/i18n.h>
#include <mksh/misc.h>		/* setup_char_semantics() */

/*
 *	get_char_semantics_value(ch)
 *
 *	Return value:
 *		The character semantics of ch.
 *
 *	Parameters:
 *		ch		character we want semantics for.
 *
 */
char
get_char_semantics_value(wchar_t ch)
{
	static Boolean	char_semantics_setup;

	if (!char_semantics_setup) {
		setup_char_semantics();
		char_semantics_setup = true;
	}
	return char_semantics[get_char_semantics_entry(ch)];
}

/*
 *	get_char_semantics_entry(ch)
 *
 *	Return value:
 *		The slot number in the array for special make chars,
 *		else the slot number of the last array entry.
 *
 *	Parameters:
 *		ch		The wide character
 *
 *	Global variables used:
 *		char_semantics_char[]	array of special wchar_t chars
 *					"&*@`\\|[]:$=!>-\n#()%?;^<'\""
 */
int
get_char_semantics_entry(wchar_t ch)
{
	wchar_t		*char_sem_char;

	char_sem_char = (wchar_t *) wcschr(char_semantics_char, ch);
	if (char_sem_char == NULL) {
		/*
		 * Return the integer entry for the last slot,
		 * whose content is empty.
		 */
		return (CHAR_SEMANTICS_ENTRIES - 1);
	} else {
		return (char_sem_char - char_semantics_char);
	}
}

