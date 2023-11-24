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
 * @(#)rep.cc 1.25 06/12/12
 */

#pragma	ident	"@(#)rep.cc	1.25	06/12/12"

/*
 * Copyright 2017-2020 J. Schilling
 * Copyright 2022 the schilytools team
 *
 * @(#)rep.cc	1.11 20/11/19 2017-2020 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)rep.cc	1.11 20/11/19 2017-2020 J. Schilling";
#endif

/*
 *	rep.c
 *
 *	This file handles the .nse_depinfo file
 */

/*
 * Included files
 */ 
#include <mk/defs.h>
#include <mksh/misc.h>		/* retmem() */
#include <vroot/report.h>	/* NSE_DEPINFO */

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
 * Static variables
 */
static	Recursive_make	recursive_list;
static	Recursive_make	*bpatch = &recursive_list;
static	Boolean		changed;

#ifndef	HAVE_FGETWS
wchar_t *
fgetws(wchar_t *ws, int n, FILE *f)
{
	char	buf[MAXPATHLEN];
	char	*bp = buf;
	char	*p;

	if (n > sizeof (n))
		bp = getmem(n);

	p = fgets(bp, n, f);
	if (p == NULL) {
		ws = (wchar_t *)0;
		goto out;
	}

	if (mbstowcs(ws, buf, n) < 0) {
		ws = (wchar_t *)0;
		goto out;
	}

out:
	if (bp != buf)
		free(bp);
	return (ws);
}
#endif

/*
 * File table of contents
 */


/*
 *	report_recursive_init()
 *
 *	Read the .nse_depinfo file and make a list of all the
 *	.RECURSIVE entries.
 *
 *	Parameters:
 *
 *	Static variables used:
 *		bpatch		Points to slot where next cell should be added
 *
 *	Global variables used:
 *		recursive_name	The Name ".RECURSIVE", compared against
 */

void
report_recursive_init(void)
{
	char		*search_dir;
	char		nse_depinfo[MAXPATHLEN];
	FILE		*fp;
	size_t		line_size, line_index;
	wchar_t		*line;
	wchar_t		*bigger_line;
	wchar_t		*colon_loc;
	wchar_t		*dollar_loc;
	Recursive_make	rp;

	/*
	 * This routine can be called more than once,  don't do
	 * anything after the first time.
	 */
	if (depinfo_already_read) {
		return;
	} else {
		depinfo_already_read = true;
	}
					
	search_dir = getenv(NOCATGETS("NSE_DEP"));
	if (search_dir == NULL) {
		return;
	}
	(void) sprintf(nse_depinfo, "%s/%s", search_dir, NSE_DEPINFO);
	fp = fopen(nse_depinfo, "r");
	if (fp == NULL) {
		return;
	}
	line_size = MAXPATHLEN;
	line_index = line_size - 1;
	line = ALLOC_WC(line_size);
	Wstring rns(recursive_name);
	wchar_t * wcb = rns.get_string();
	while (fgetws(line, line_size, fp) != NULL) {
		while (wcslen(line) == line_index) {
			if (line[wcslen(line) - 1] == '\n') {
				continue;
			}
			bigger_line = ALLOC_WC(2 * line_size);
			wcscpy(bigger_line, line);
			retmem(line);
			line = bigger_line;
			if (fgetws(&line[line_index], line_size, fp) == NULL)
				continue;
			line_index = 2 * line_index;
			line_size = 2 * line_size;
		}

		colon_loc = (wchar_t *) wcschr(line, (int) colon_char);
		if (colon_loc == NULL) {
			continue;
		}
		dollar_loc = (wchar_t *) wcschr(line, (int) dollar_char);
		line[wcslen(line) - 1] = (int) nul_char;
		if (IS_WEQUALN(&colon_loc[2], wcb,
	            (int) recursive_name->hash.length)) {
			/*
			 * If this entry is an old entry, ignore it
			 */
			MBSTOWCS(wcs_buffer, DEPINFO_FMT_VERSION);
			if (dollar_loc == NULL ||
			    !IS_WEQUALN(wcs_buffer, (dollar_loc+1) - VER_LEN, VER_LEN)){
				continue;
			    }
			rp = ALLOC(Recursive_make);
			(void) memset((char *) rp, 0, sizeof (Recursive_make_rec));
			/*
			 * set conditional_macro_string if string is present
			 */
			rp->oldline = (wchar_t *) wcsdup(line);
			if ( dollar_loc != NULL ){
				rp->cond_macrostring = 
				    (wchar_t *) wcsdup(dollar_loc - VER_LEN + 1);
			}
			/* 
			 * get target name into recursive struct
			 */
			*colon_loc = (int) nul_char;
			rp->target = (wchar_t *) wcsdup(line);
			*bpatch = rp;
			bpatch = &rp->next;
		}
	}
	(void) fclose(fp);
}

/*
 *	report_recursive_dep(target, line)
 *
 *	Report a target as recursive.
 *
 *	Parameters:
 *		line		Dependency line reported
 *
 *	Static variables used:
 *		bpatch		Points to slot where next cell should be added
 *		changed		Written if report set changed
 */
void
report_recursive_dep(Name target, wchar_t *line)
{
	Recursive_make	rp;
	wchar_t		rec_buf[STRING_BUFFER_LENGTH];
	String_rec	string;

	INIT_STRING_FROM_STACK(string, rec_buf);
	cond_macros_into_string(target, &string);
	/* 
	 * find an applicable recursive entry, if there isn't one, create it
	 */
	rp = find_recursive_target(target);
	if (rp == NULL) {
		rp = ALLOC(Recursive_make);
		(void) memset((char *) rp, 0, sizeof (Recursive_make_rec));
		wchar_t * wcb = get_wstring(target->string_mb); // XXX Tolik: needs retmem
                rp->target = wcb;
		rp->newline = (wchar_t *) wcsdup(line);
		rp->cond_macrostring = (wchar_t *) wcsdup(rec_buf);
		*bpatch = rp;
		bpatch = &rp->next;
		changed = true;
	} else {
		if ((rp->oldline != NULL) && !IS_WEQUAL(rp->oldline, line)) {
			rp->newline = (wchar_t *) wcsdup(line);
			changed = true;
		}
		rp->removed = false;
	}
}

/*
 *	find_recursive_target(target)
 *
 *	Search the list for a given target.
 *
 *	Return value:
 *				The target cell
 *
 *	Parameters:
 *		target		The target we need
 *		top_level_target more info used to determinde the 
 *				 target we need
 *
 *	Static variables used:
 *		recursive_list	The list of targets
 */
Recursive_make
find_recursive_target(Name target)
{
	Recursive_make	rp;
	String_rec	string;
	wchar_t		rec_buf[STRING_BUFFER_LENGTH]; 

	INIT_STRING_FROM_STACK(string, rec_buf);
	cond_macros_into_string(target, &string);

	Wstring tstr(target);
	wchar_t * wcb = tstr.get_string();
	for (rp = recursive_list; rp != NULL; rp = rp->next) {
		/* 
		 * If this entry has already been removed, ignore it.
		 */
		if (rp->removed)
			continue;
		/* 
		 * If this target, and the target on the list are the same
		 * and if one of them contains conditional macro info, while
		 * the other doesn't,  remove this entry from the list of
		 * recursive entries.  This can only happen if the Makefile
		 * has changed to no longer contain conditional macros.
		 */
		if (IS_WEQUAL(rp->target, wcb)) {
			if (rp->cond_macrostring[VER_LEN] == '\0' &&
			    string.buffer.start[VER_LEN] != '\0'){
				rp->removed = true;
				continue;
			} else if (rp->cond_macrostring[VER_LEN] != '\0' &&
			    string.buffer.start[VER_LEN] == '\0'){
				rp->removed = true;
				continue;
			} 
		}
		/*
		 * If this is not a VERS2 entry,  only need to match
		 * the target name.  toptarg information from VERS1 entries
		 * are ignored.
		 */
		MBSTOWCS(wcs_buffer, DEPINFO_FMT_VERSION);
		if (IS_WEQUALN(wcs_buffer, string.buffer.start, VER_LEN)) { 
			if (IS_WEQUAL(rp->cond_macrostring, 
			    string.buffer.start) &&
			    IS_WEQUAL(rp->target, wcb)) {
				return rp;
			}
		} else {
			if (IS_WEQUAL(rp->target, wcb)) {
				return rp;
			}
		}
	}
	return NULL;
}

/*
 *	remove_recursive_dep(target, top_level_target)
 *
 *	Mark a target as no longer recursive.
 *
 *	Parameters:
 *		target		The target we want to remove
 *		top_level_target target we want to remove must be built from 
 *				 the same top level target
 *
 *	Static variables used:
 *		changed		Written if report set changed
 */
void
remove_recursive_dep(Name target)
{
	Recursive_make	rp;

	rp = find_recursive_target(target);

	if ( rp != NULL ) {
		rp->removed = true;	
		changed = true;
		if(rp->target) {
			retmem(rp->target);
			rp->target = NULL;
		}
		if(rp->newline) {
			retmem(rp->newline);
			rp->newline = NULL;
		}
		if(rp->oldline) {
			retmem(rp->oldline);
			rp->oldline = NULL;
		}
		if(rp->cond_macrostring) {
			retmem(rp->cond_macrostring);
			rp->cond_macrostring = NULL;
		}
	}
}

#ifdef NSE
/*
 *	report_recursive_done()
 *
 *	Write the .nse_depinfo file.
 *
 *	Parameters:
 *
 *	Static variables used:
 *		recursive_list	The list of targets
 *		changed		Written if report set changed
 *
 *	Global variables used:
 *		recursive_name	The Name ".RECURSIVE", compared against
 */
void
report_recursive_done(void)
{
	char		*search_dir;
	char		nse_depinfo[MAXPATHLEN];
	char		tmpfile[MAXPATHLEN];
	FILE		*ofp;
	FILE		*ifp;
	wchar_t		*space;
	wchar_t		*data;
	wchar_t		*line;
	wchar_t		*bigger_line;
	size_t		line_size, line_index;
	int		lock_err;
	Recursive_make	rp;

	if (changed == false) {
		return;
	}

	search_dir = getenv(NOCATGETS("NSE_DEP"));
	if (search_dir == NULL) {
		return;
	}
	(void) sprintf(nse_depinfo, "%s/%s", search_dir, NSE_DEPINFO);
	(void) sprintf(tmpfile, "%s.%d", nse_depinfo, getpid());
	ofp = fopen(tmpfile, "w");
	if (ofp == NULL) {
		(void) fprintf(stderr,
			       gettext("Cannot open `%s' for writing\n"),
			       tmpfile);
		return;
	}
	(void) sprintf(nse_depinfo_lockfile, 
		       "%s/%s", search_dir, NSE_DEPINFO_LOCK);
	if (lock_err = file_lock(nse_depinfo, 
				 nse_depinfo_lockfile, 
				 (int *) &nse_depinfo_locked, 0)) {
		(void) fprintf(stderr,
			       gettext("writing .RECURSIVE lines to %s\n"),
			       tmpfile);
		(void) fprintf(stderr,
			       gettext("To recover, merge .nse_depinfo.%d with .nse_depinfo\n"),
			       getpid(),
			       gettext("with .nse_depinfo"));
	}

	if (nse_depinfo_locked) {
		ifp = fopen(nse_depinfo, "r");
		if (ifp != NULL) {
			/*
			 * Copy all the non-.RECURSIVE lines from 
			 * the old file to the new one.
			 */
			line_size = MAXPATHLEN;
			line_index = line_size - 1;
			line = ALLOC_WC(line_size);
			while (fgetws(line, line_size, ifp) != NULL) {
				while (wcslen(line) == line_index) {
					if (line[wcslen(line) - 1] == '\n') {
						continue;
					}
					bigger_line = ALLOC_WC(2 * line_size);
					wcscpy(bigger_line, line);
					retmem(line);
					line = bigger_line;
					if (fgetws(&line[line_index], 
						  line_size, ifp) == NULL)
					  continue;
					line_index = 2 * line_index;
					line_size = 2 * line_size;
				}

				space = wcschr(line, (int) space_char);
				if (space != NULL && 
				    IS_WEQUALN(&space[1],
					      recursive_name->string,
					      (int) recursive_name->hash.length)) {
					continue;
				}
				WCSTOMBS(mbs_buffer, line);
				(void) fprintf(ofp, "%s", mbs_buffer);
			}
			(void) fclose(ifp);
		}
	}

	/*
	 * Write out the .RECURSIVE lines.
	 */
	for (rp = recursive_list; rp != NULL; rp = rp->next) {
		if (rp->removed) {
			continue;
		}
		if (rp->newline != NULL) {
			data = rp->newline;
		} else {
			data = rp->oldline;
		}
		if (data != NULL) {
			WCSTOMBS(mbs_buffer, data);
			(void) fprintf(ofp, "%s\n", mbs_buffer);
		}
	}
	(void) fclose(ofp);

	if (nse_depinfo_locked) {
		(void) rename(tmpfile, nse_depinfo);
		(void) unlink(nse_depinfo_lockfile);
		nse_depinfo_locked = false;
		nse_depinfo_lockfile[0] = '\0';
		(void) chmod(nse_depinfo, 0666);
	}
}
#endif // NSE

/* gather_recursive_deps()
 *
 *	Create or update list of recursive targets.  
 */
void
gather_recursive_deps(void)
{
	Name_set::iterator	np, e;
	String_rec		rec;
	wchar_t			rec_buf[STRING_BUFFER_LENGTH];
	Property		lines;
	Boolean			has_recursive;
	Dependency		dp;

	report_recursive_init();

	/* Go thru all targets and dump recursive dependencies */
	for (np = hashtab.begin(), e = hashtab.end(); np != e; np++) {
		if (np->has_recursive_dependency){
			has_recursive = false;
			/* 
			 * start .RECURSIVE line with target:
			 */
			INIT_STRING_FROM_STACK(rec, rec_buf);
			APPEND_NAME(np, &rec, FIND_LENGTH);
			append_char((int) colon_char, &rec);
			append_char((int) space_char, &rec);
			
			for (lines = get_prop(np->prop,recursive_prop); 
			    lines != NULL;
			    lines = get_prop(lines->next, recursive_prop)) {
				/* 
				 * if entry is already in depinfo
				 * file or entry was not built, ignore it
				 */
				if (lines->body.recursive.in_depinfo)
					continue;
				if (!lines->body.recursive.has_built)
					continue;
				has_recursive = true;
				lines->body.recursive.in_depinfo=true;
				
				/* 
				* Write the remainder of the
				* .RECURSIVE line 
				*/
				APPEND_NAME(recursive_name, &rec, 
				    FIND_LENGTH);
				append_char((int) space_char, &rec);
				APPEND_NAME(lines->body.recursive.directory,
					&rec, FIND_LENGTH);
				append_char((int) space_char, &rec);
				APPEND_NAME(lines->body.recursive.target,
					&rec, FIND_LENGTH);
				append_char((int) space_char, &rec);
				
				/* Complete list of makefiles used */
				for (dp = lines->body.recursive.makefiles; 
				    dp != NULL; 
				    dp = dp->next) {
					APPEND_NAME(dp->name, &rec,  FIND_LENGTH);
					append_char((int) space_char, &rec);
				}
			}  
			/* 
			 * dump list of conditional targets,
			 * and report recursive entry, if needed
			 */
			cond_macros_into_string(np, &rec);
			if (has_recursive){
				report_recursive_dep(np, rec.buffer.start);
			}

		} else if ( np->has_built ) {
			remove_recursive_dep(np);
		}
	}
}

