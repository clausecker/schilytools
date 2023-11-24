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
 * @(#)nse_printdep.cc 1.18 06/12/12
 */

#pragma	ident	"@(#)nse_printdep.cc	1.18	06/12/12"

/*
 * Copyright 2017-2019 J. Schilling
 * Copyright 2022, 2023 the schilytools team
 *
 * @(#)nse_printdep.cc	1.7 19/10/19 2017-2019 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)nse_printdep.cc	1.7 19/10/19 2017-2019 J. Schilling";
#endif

/*
 * Included files
 */
#include <mk/defs.h>
#include <mksh/misc.h>		/* get_prop() */

/*
 * File table of contents
 */
void   print_dependencies(Name target, Property line);
static void	print_deps(Name target, Property line);
static void	print_more_deps(Name target, Name name);
static void	print_filename(Name name);
static Boolean	should_print_dep(Property line);
static void	print_forest(Name target);
static void	print_deplist(Dependency head);
void		print_value(Name value, Daemon daemon);
static	void	print_rec_info(Name target);
static Boolean	is_out_of_date(Property line);
extern void depvar_print_results (void);

/*
 *	print_dependencies(target, line)
 *
 *	Print all the dependencies of a target. First print all the Makefiles.
 *	Then print all the dependencies. Finally, print all the .INIT
 *	dependencies.
 *
 *	Parameters:
 *		target		The target we print dependencies for
 *		line		We get the dependency list from here
 *
 *	Global variables used:
 *		done		The Name ".DONE"
 *		init		The Name ".INIT"
 *		makefiles_used	List of all makefiles read
 */
void
print_dependencies(Name target, Property line)
{
	Dependency	dp;
	static Boolean	makefiles_printed = false;

#ifdef SUNOS4_AND_AFTER
	if (target_variants) {
#else
	if (is_true(flag.target_variants)) {
#endif
		depvar_print_results();
	}
	
	if (!makefiles_printed) {
		/*
		 * Search the makefile list for the primary makefile,
		 * then print it and its inclusions.  After that go back
		 * and print the default.mk file and its inclusions.
		 */
		for (dp = makefiles_used; dp != NULL; dp = dp->next) {
			if (dp->name == primary_makefile) {
				break;
			}
		}
		if (dp) {
			print_deplist(dp);
			for (dp = makefiles_used; dp != NULL; dp = dp->next) {
				if (dp->name == primary_makefile) {
					break;
				}
				(void)printf(" %s", dp->name->string_mb);
			}
		}
		(void) printf("\n");
		makefiles_printed = true;
	}
	print_deps(target, line);
#ifdef SUNOS4_AND_AFTER
/*
	print_more_deps(target, init);
	print_more_deps(target, done);
 */
	(void) print_more_deps;
	if (target_variants) {
#else
	print_more_deps(target, cached_names.init);
	print_more_deps(target, cached_names.done);
	if (is_true(flag.target_variants)) {
#endif
		print_forest(target);
	}
}

/*
 *	print_more_deps(target, name)
 *
 *	Print some special dependencies.
 *	These are the dependencies for the .INIT and .DONE targets.
 *
 *	Parameters:
 *		target		Target built during make run
 *		name		Special target to print dependencies for
 *
 *	Global variables used:
 */
static void
print_more_deps(Name target, Name name)
{
	Property		line;
	Dependency		dependencies;

	line = get_prop(name->prop, line_prop);
	if (line != NULL && line->body.line.dependencies != NULL) {
		(void) printf("%s:\t", target->string_mb);
		print_deplist(line->body.line.dependencies);
		(void) printf("\n");
		for (dependencies= line->body.line.dependencies;
		     dependencies != NULL;
		     dependencies= dependencies->next) {
	                 print_deps(dependencies->name,
				 get_prop(dependencies->name->prop, line_prop));
		}
	}
}

/*
 *	print_deps(target, line, go_recursive)
 *
 *	Print a regular dependency list.  Append to this information which
 *	indicates whether or not the target is recursive.
 *
 *	Parameters:
 *		target		target to print dependencies for
 *		line		We get the dependency list from here
 *		go_recursive	Should we show all dependencies recursively?
 *
 *	Global variables used:
 *		recursive_name	The Name ".RECURSIVE", printed
 */
static void
print_deps(Name target, Property line)
{
	Dependency		dep;

#ifdef SUNOS4_AND_AFTER
	if ((target->dependency_printed) ||
	    (target == force)) {
#else
	if (is_true(target->dependency_printed)) {
#endif
		return;
	}
	target->dependency_printed = true;

	/* only print entries that are actually derived and are not leaf
	 * files and are not the result of sccs get.
	 */
	if (should_print_dep(line)) {
#ifdef NSE
		nse_check_no_deps_no_rule(target, line, line);
#endif
		if ((report_dependencies_level == 2) ||
		    (report_dependencies_level == 4)) {
			if (is_out_of_date(line)) {
			        (void) printf("1 ");
			} else {
			        (void) printf("0 ");
			}
		}
		print_filename(target);
	    	(void) printf(":\t");
	    	print_deplist(line->body.line.dependencies);
		print_rec_info(target);
	   	(void) printf("\n");
	 	for (dep = line->body.line.dependencies;
		     dep != NULL;
		     dep = dep->next) {
			print_deps(dep->name,
			           get_prop(dep->name->prop, line_prop));
		}
	}
}

static Boolean
is_out_of_date(Property line)
{
	Dependency	dep;
	Property	line2;

	if (line == NULL) {
		return false;
	}
	if (line->body.line.is_out_of_date) {
		return true;
	}
	for (dep = line->body.line.dependencies;
	     dep != NULL;
	     dep = dep->next) {
		line2 = get_prop(dep->name->prop, line_prop);
		if (is_out_of_date(line2)) {
			line->body.line.is_out_of_date = true;
			return true;
		}
	}
	return false;
}

/*
 * Given a dependency print it and all its siblings.
 */
static void
print_deplist(Dependency head)
{
	Dependency	dp;

	for (dp = head; dp != NULL; dp = dp->next) {
		if ((report_dependencies_level != 2) ||
		    ((!dp->automatic) ||
		     (dp->name->is_double_colon))) {
			if (dp->name != force) {
				putchar(' ');
				print_filename(dp->name);
			}
		}
	}
}

/*
 * Print the name of a file for the -P option.
 * If the file is a directory put on a trailing slash.
 */
static void
print_filename(Name name)
{
	(void) printf("%s", name->string_mb);
/*
	if (name->stat.is_dir) {
		putchar('/');
	}
 */
}

/*
 *	should_print_dep(line)
 *
 *	Test if we should print the dependencies of this target.
 *	The line must exist and either have children dependencies
 *	or have a command that is not an SCCS command.
 *
 *	Return value:
 *				true if the dependencies should be printed
 *
 *	Parameters:
 *		line		We get the dependency list from here
 *
 *	Global variables used:
 */
static Boolean
should_print_dep(Property line)
{
	if (line == NULL) {
		return false;
	}
	if (line->body.line.dependencies != NULL) {
		return true;
	}
#ifdef SUNOS4_AND_AFTER
	if (line->body.line.sccs_command) {
#else
	if (is_true(line->body.line.sccs_command)) {
#endif
		return false;
	}
	return true;
}

/*
 * Print out the root nodes of all the dependency trees
 * in this makefile.
 */
static void
print_forest(Name target)
{
	Name_set::iterator np, e;
	Property	line;

 	for (np = hashtab.begin(), e = hashtab.end(); np != e; np++) {
#ifdef SUNOS4_AND_AFTER
			if (np->is_target && !np->has_parent && np != target) {
#else
			if (is_true(np->is_target) && 
			    is_false(np->has_parent) &&
			    np != target) {
#endif
				(void) doname_check(np, true, false, false);
				line = get_prop(np->prop, line_prop);
				printf("-\n");
				print_deps(np, line);
			}
	}
}

/*
 *	This is a set  of routines for dumping the internal make state
 *	Used for the -p option
 */
void
print_value(Name value, Daemon daemon)
	             		      
#ifdef SUNOS4_AND_AFTER
              			       
#else
	           		       
#endif
{
	Chain			cp;

	if (value == NULL)
		(void)printf("=\n");
	else
		switch (daemon) {
		    case no_daemon:
			(void)printf("= %s\n", value->string_mb);
			break;
		    case chain_daemon:
			for (cp= (Chain) value; cp != NULL; cp= cp->next)
				(void)printf(cp->next == NULL ? "%s" : "%s ",
					cp->name->string_mb);
			(void)printf("\n");
			break;
		};
}

/* 
 *  If target is recursive,  print the following to standard out:
 *	.RECURSIVE subdir targ Makefile
 */
static void
print_rec_info(Name target)
{
	Recursive_make	rp;
	wchar_t		*colon;

	report_recursive_init();

	rp = find_recursive_target(target);

	if (rp) {
		/* 
		 * if found,  print starting with the space after the ':'
		 */
		colon = (wchar_t *) wcschr(rp->oldline, (int) colon_char);
		(void) printf("%ls", colon + 1);
	}
}
		
