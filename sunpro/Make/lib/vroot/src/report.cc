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
 * @(#)report.cc 1.17 06/12/12
 */

#pragma	ident	"@(#)report.cc	1.17	06/12/12"

/*
 * Copyright 2017-2021 J. Schilling
 * Copyright 2022 the schilytools team
 *
 * @(#)report.cc	1.13 21/08/15 2017-2021 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)report.cc	1.13 21/08/15 2017-2021 J. Schilling";
#endif

#if defined(SCHILY_BUILD) || defined(SCHILY_INCLUDES)
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/param.h>
#include <schily/wait.h>
#include <schily/unistd.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <unistd.h>
#define	WAIT_T	int
#endif

#include <vroot/report.h>
#include <vroot/vroot.h>
#include <avo/intl.h>	/* for NOCATGETS */
#include <mk/defs.h>	/* for tmpdir */

static	FILE	*report_file;
static	char	*target_being_reported_for;

FILE *
get_report_file(void)
{
	return(report_file);
}

char *
get_target_being_reported_for(void)
{
	return(target_being_reported_for);
}

#ifdef	HAVE_ATEXIT
extern "C" {
static void
close_report_file(void)
{
	(void)fputs("\n", report_file);
	(void)fclose(report_file);
}
} // extern "C"
#else
static void
close_report_file(int, ...)
{
	(void)fputs("\n", report_file);
	(void)fclose(report_file);
}
#endif

void
report_dependency(const char *name)
{
	char			*filename;
	char			buffer[MAXPATHLEN+1];
	char			*p;
	char			*p2;

	if (report_file == NULL) {
		if ((filename= getenv(SUNPRO_DEPENDENCIES)) == NULL) {
			report_file = (FILE *)-1;
			return;
		}
		if (strlen(filename) == 0) {
			report_file = (FILE *)-1;
			return;
		}
		(void)strcpy(buffer, name);
		name = buffer;
		p = strchr(filename, ' ');
		if(p) {
			*p= 0;
		} else {
			report_file = (FILE *)-1;
			return;
		}
		if ((report_file= fopen(filename, "a")) == NULL) {
			if ((report_file= fopen(filename, "w")) == NULL) {
				report_file= (FILE *)-1;
				return;
			}
		}
#ifdef	HAVE_ATEXIT
		atexit(close_report_file);
#else
		(void)on_exit(close_report_file, (char *)report_file);
#endif
		if ((p2= strchr(p+1, ' ')) != NULL)
			*p2= 0;
		target_being_reported_for= (char *)malloc((unsigned)(strlen(p+1)+1));
		(void)strcpy(target_being_reported_for, p+1);
		(void)fputs(p+1, report_file);
		(void)fputs(":", report_file);
		*p= ' ';
		if (p2 != NULL)
			*p2= ' ';
	}
	if (report_file == (FILE *)-1)
		return;
	(void)fputs(name, report_file);
	(void)fputs(" ", report_file);
}

#ifdef MAKE_IT
void
make_it(char *filename)
{
	char			*command;
	char			*argv[6];
	int			pid;
	WAIT_T			foo;

	if (getenv(SUNPRO_DEPENDENCIES) == NULL) return;
	command= alloca(strlen(filename)+32);
	(void)sprintf(command, NOCATGETS("make %s\n"), filename);
	switch (pid= fork()) {
		case 0: /* child */
			argv[0]= NOCATGETS("csh");
			argv[1]= NOCATGETS("-c");
			argv[2]= command;
			argv[3]= 0;			
			(void)dup2(2, 1);
			execve(NOCATGETS("/bin/sh"), argv, environ);
			perror(NOCATGETS("execve error"));
			exit(1);
		case -1: /* error */
			perror(NOCATGETS("fork error"));
		default: /* parent */
			while (wait(&foo) != pid);};
}
#endif

