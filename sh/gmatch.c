/* @(#)gmatch.c	1.4 09/07/11 2008-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)gmatch.c	1.4 09/07/11 2008-2009 J. Schilling";
#endif

#include <schily/mconfig.h>

#ifndef	HAVE_GMATCH

/* -------- gmatch.c -------- */
/*
 * int gmatch(string, pattern)
 * char *string, *pattern;
 *
 * Match a pattern as in sh(1).
 *
 * This version is under BSD license. 
 * Andrzej Bialecki 
 * <abial@FreeBSD.org> 
 */

#define	NULL	0

#define	CMASK	0377
#define	QUOTE	0200
#define	QMASK	(CMASK&~QUOTE)
#define	NOT	'!'	/* might use ^ */

static char *cclass	__PR((const char *p, int sub));
	int gmatch	__PR((const char *s, const char *p));

static char *
cclass(p, sub)
register const char *p;
register int sub;
{
	register int c, d, not, found;

	if ((not = *p == NOT) != 0)
		p++;
	found = not;
	do {
		if (*p == '\0')
			return ((char *)NULL);
		c = *p & CMASK;
		if (p[1] == '-' && p[2] != ']') {
			d = p[2] & CMASK;
			p++;
		} else
			d = c;
		if (c == sub || (c <= sub && sub <= d))
			found = !not;
	} while (*++p != ']');
	return (found? (char *)p+1: (char *)NULL);
}

int
gmatch(s, p)
register const char *s, *p;
{
	register int sc, pc;

	if (s == NULL || p == NULL)
		return (0);
	while ((pc = *p++ & CMASK) != '\0') {
		sc = *s++ & QMASK;
		switch (pc) {
		case '[':
			if ((p = cclass(p, sc)) == NULL)
				return (0);
			break;

		case '?':
			if (sc == 0)
				return (0);
			break;

		case '*':
			s--;
			do {
				if (*p == '\0' || gmatch(s, p))
					return (1);
			} while (*s++ != '\0');
			return (0);

		default:
			if (sc != (pc&~QUOTE))
				return (0);
		}
	}
	return (*s == 0);
}

#endif	/* HAVE_GMATCH */
