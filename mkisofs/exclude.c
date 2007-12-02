/* @(#)exclude.c	1.11 06/10/03 joerg */
#ifndef lint
static	char sccsid[] =
	"@(#)exclude.c	1.11 06/10/03 joerg";
#endif
/*
 * 9-Dec-93 R.-D. Marzusch, marzusch@odiehh.hanse.de:
 * added 'exclude' option (-x) to specify pathnames NOT to be included in
 * CD image.
 */

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>


/* this allows for 1000 entries to be excluded ... */
#define	MAXEXCL		1000

static char		*excl[MAXEXCL];

void	exclude		__PR((char *fn));
int	is_excluded	__PR((char *fn));


void
exclude(fn)
	char		*fn;
{
	register int	i;

	for (i = 0; excl[i] && i < MAXEXCL; i++)
		;

	if (i == MAXEXCL) {
		fprintf(stderr,
			"Can't exclude '%s' - too many entries in table\n",
								fn);
		return;
	}
	excl[i] = (char *) malloc(strlen(fn) + 1);
	if (excl[i] == NULL) {
		errmsg("Can't allocate memory for excluded filename\n");
		return;
	}
	strcpy(excl[i], fn);
}

int
is_excluded(fn)
	char		*fn;
{
	register int	i;

	/*
	 * very dumb search method ...
	 */
	for (i = 0; excl[i] && i < MAXEXCL; i++) {
		if (strcmp(excl[i], fn) == 0) {
			return (1);	/* found -> excluded filenmae */
		}
	}
	return (0);	/* not found -> not excluded */
}
