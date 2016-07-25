/* @(#)gmatch.c	1.10 16/07/15 2008-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)gmatch.c	1.10 16/07/15 2008-2016 J. Schilling";
#endif

#include <schily/mconfig.h>

#ifdef	MY_GMATCH			/* #define to enforce this gmatch() */
#undef	HAVE_GMATCH			/* instead of gmatch() from -lgen   */
#endif
#ifndef	HAVE_GMATCH

#include	<schily/limits.h>	/* MB_LEN_MAX */
#include	<schily/wchar.h>	/* includes stdio.h */
#include	<schily/wctype.h>	/* needed before we use wchar_t */

/* -------- gmatch.c -------- */
/*
 * int gmatch(string, pattern)
 * char *string, *pattern;
 *
 * Match a pattern as in sh(1).
 *
 * This version is under BSD license.
 * Originally written by Andrzej Bialecki <abial@FreeBSD.org>
 * Rewritten for multi-byte support by J. Schilling
 */

#ifndef	NULL
#define	NULL	0
#endif
#define	NOT	'!'	/* might use ^ */

static char *cclass	__PR((const char *p, int sub));
	int gmatch	__PR((const char *s, const char *p));

#define	nextwc(p, c) \
		n = mbtowc(&lc, p, MB_LEN_MAX); \
		c = lc; \
		if (n < 0) { \
			(void) mbtowc(NULL, NULL, 0); \
			return (0); \
		} \
		p += n

static char *
cclass(p, sub)
	register const char	*p;
	register int		sub;
{
	register int c, d, not, found;
		wchar_t	lc;
		int	n;

	if ((n = mbtowc(&lc, p, MB_LEN_MAX)) < 0) {
		(void) mbtowc(NULL, NULL, 0);
		return (0);
	} else if ((not = (lc == NOT)) != 0) {
		p += n;
	}
	found = not;
	do {
		if (*p == '\0')
			return ((char *)NULL);

		nextwc(p, c);
		if ((n = mbtowc(&lc, p, MB_LEN_MAX)) < 0) {
			(void) mbtowc(NULL, NULL, 0);
			return (0);
		}
		if (lc == '-' && p[n] != ']') {
			p += n;
			nextwc(p, d);
			if (mbtowc(&lc, p, MB_LEN_MAX) < 0) {
				(void) mbtowc(NULL, NULL, 0);
				return (0);
			}
		} else {
			d = c;
		}
		if (c == sub || (c <= sub && sub <= d))
			found = !not;
	} while (lc != ']');
	return (found? (char *)p+1: (char *)NULL);
}

int
gmatch(s, p)
	register const char	*s;	/* The string to match	*/
	register const char	*p;	/* The pattern		*/
{
		const char *os;
	register wchar_t sc;
	register wchar_t pc;
		wchar_t	lc;
		int	n;

	if (s == NULL || p == NULL)
		return (0);

	while (*p != '\0') {
		os = s;
		nextwc(p, pc);
		nextwc(s, sc);

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
			while (*p == '*')
				p++;
			do {
				if (*p == '\0' || gmatch(os, p))
					return (1);
				os = s;
				nextwc(s, sc);
			} while (sc != '\0');
			return (0);

		case '\\':
			nextwc(p, pc);
			/* FALLTROUGH */

		default:
			if (sc != pc)
				return (0);
		}
	}
	return (*s == 0);
}

#endif	/* HAVE_GMATCH */
