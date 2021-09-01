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
 * Copyright 1995 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)depvar.cc 1.14 06/12/12
 */

#pragma	ident	"@(#)depvar.cc	1.14	06/12/12"

/*
 * Copyright 2017-2018 J. Schilling
 *
 * @(#)depvar.cc	1.4 21/08/15 2017-2018 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)depvar.cc	1.4 21/08/15 2017-2018 J. Schilling";
#endif

/*
 * Included files
 */
#include <mk/defs.h>
#include <mksh/misc.h>		/* getmem() */

/*
 * This file deals with "Dependency Variables".
 * The "-V var" command line option is used to indicate
 * that var is a dependency variable.  Used in conjunction with
 * the -P option the user is asking if the named variables affect
 * the dependencies of the given target.
 */

struct _Depvar {
	Name		name;		/* Name of variable */
	struct _Depvar	*next;		/* Linked list */
	Boolean		cmdline;	/* Macro defined on the cmdline? */
};

typedef	struct _Depvar	*Depvar;

static	Depvar		depvar_list;
static	Depvar		*bpatch = &depvar_list;
static	Boolean		variant_deps;

/*
 * Add a name to the list.
 */

void
depvar_add_to_list(Name name, Boolean cmdline)
{
	Depvar		dv;

#ifdef SUNOS4_AND_AFTER
	dv = ALLOC(Depvar);
#else
	dv = (Depvar) Malloc(sizeof(struct _Depvar));
#endif
	dv->name = name;
	dv->next = NULL;
	dv->cmdline = cmdline;
	*bpatch = dv;
	bpatch = &dv->next;
}

/*
 * The macro `name' has been used in either the left-hand or
 * right-hand side of a dependency.  See if it is in the
 * list.  Two things are looked for.  Names given as args
 * to the -V list are checked so as to set the same/differ
 * output for the -P option.  Names given as macro=value
 * command-line args are checked and, if found, an NSE
 * warning is produced.
 */
void
depvar_dep_macro_used(Name name)
{
	Depvar		dv;

	for (dv = depvar_list; dv != NULL; dv = dv->next) {
		if (name == dv->name) {
#ifdef NSE
#ifdef SUNOS4_AND_AFTER
			if (dv->cmdline) {
#else
			if (is_true(dv->cmdline)) {
#endif
				nse_dep_cmdmacro(dv->name->string);
			}
#endif
			variant_deps = true;
			break;
		}
	}
}

#ifdef NSE
/*
 * The macro `name' has been used in either the argument
 * to a cd before a recursive make.  See if it was
 * defined on the command-line and, if so, complain.
 */
void
depvar_rule_macro_used(Name name)
{
	Depvar		dv;

	for (dv = depvar_list; dv != NULL; dv = dv->next) {
		if (name == dv->name) {
#ifdef SUNOS4_AND_AFTER
			if (dv->cmdline) {
#else
			if (is_true(dv->cmdline)) {
#endif
				nse_rule_cmdmacro(dv->name->string);
			}
			break;
		}
	}
}
#endif

/*
 * Print the results.  If any of the Dependency Variables
 * affected the dependencies then the dependencies potentially
 * differ because of these variables.
 */
void
depvar_print_results(void)
{
	if (variant_deps) {
		printf(gettext("differ\n"));
	} else {
		printf(gettext("same\n"));
	}
}

