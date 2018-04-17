/* @(#)gmatch.c	1.21 18/04/17 2008-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)gmatch.c	1.21 18/04/17 2008-2018 J. Schilling";
#endif

#include <schily/mconfig.h>

#ifdef	MY_GMATCH			/* #define to enforce this gmatch() */
#undef	HAVE_GMATCH			/* instead of gmatch() from -lgen   */
#endif
#ifdef	DO_POSIX_GMATCH			/* POSIX features like [:alpha:]    */
#undef	HAVE_GMATCH			/* need this gmatch()		    */
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
 * Rewritten for multi-byte support (c) 2012 by J. Schilling
 * Rewritten to avoid recursion using the concepts from
 * https://research.swtch.com/glob by J. Schilling
 * Added POSIX pattern support (c) 2017 by J. Schilling
 */

#ifndef	NULL
#define	NULL	0
#endif
#define	NOT	'!'	/* might use ^ */

static int cclass	__PR((const char *p, int sub, char **ret));
	int gmatch	__PR((const char *s, const char *p));

#define	nextwc(p, c) \
		n = mbtowc(&lc, p, MB_LEN_MAX); \
		c = lc; \
		if (n < 0) { \
			(void) mbtowc(NULL, NULL, 0); \
			return (0); \
		} \
		p += n

#define	CL_ERR		0	/* Error in pattern or multi byte char	*/
#define	CL_MATCH	1	/* Range OK, and match			*/
#define	CL_NOMATCH	2	/* Range OK, but no match		*/

#define	CL_SIZE		32	/* Max size for '[: :]'			*/

static int
cclass(p, sub, ret)
	register const char	*p;
	register int		sub;
		char		**ret;
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
			return (0);

		nextwc(p, c);
		if (c == '\\') {
			nextwc(p, c);
		}
		if ((n = mbtowc(&lc, p, MB_LEN_MAX)) < 0) {
			(void) mbtowc(NULL, NULL, 0);
			return (0);
		}
#ifdef	DO_POSIX_GMATCH
		if (c == '[') {
			if (lc == ':') {
				char	class[CL_SIZE+1];
				char	*pc = class;

				p += n;		/* Eat ':' */
				for (;;) {
					if (*p == '\0')
						return (0);
					if (*p == ':' && p[1] == ']')
						break;
					if (pc >= &class[CL_SIZE])
						return (0);
					*pc++ = *p++;
				}
				if (pc == class)
					return (0);
				*pc = '\0';
				p += 2;		/* Skip ":]" */
				if (iswctype(sub, wctype(class)))
					found = !not;
				if (*p == ']')	/* End of class */
					break;	/* parsing complete */
				continue;
			}
		}
#endif	/* DO_POSIX_GMATCH */
		if (lc == '-' && p[n] != ']') {
			p += n;
			nextwc(p, d);
			if (d == '\\') {
				nextwc(p, d);
			}
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
	*ret = (char *)p+1;
	return (found? CL_MATCH : CL_NOMATCH);
}

int
gmatch(s, p)
	register const char	*s;	/* The string to match	*/
	register const char	*p;	/* The pattern		*/
{
		const char *os;
		const char *bt_s;
		const char *bt_p;
	register wchar_t sc;
	register wchar_t pc;
		wchar_t	lc;
		int	n;

	if (s == NULL || p == NULL)
		return (0);

	bt_p = bt_s = NULL;
	while (*p != '\0') {
		os = s;
		nextwc(s, sc);
again:
		nextwc(p, pc);

		switch (pc) {
		case '[': {
			char *p2;

			if (sc == 0)
				return (0);

			switch (cclass(p, sc, &p2)) {
			case CL_ERR:
#ifdef	GMATCH_CLERR_NORM
				goto def;
#endif
			case CL_NOMATCH:
				goto backtrack;
			}

			p = p2;
			break;
		}
		case '?':
			if (sc == 0)
				return (0);
			break;

		case '*':
			while (*p == '*')
				p++;
			if (*p == '\0')
				return (1);

			bt_p = p;
			bt_s = os;
			goto again;

		case '\\':
			nextwc(p, pc);
			if (pc == 0)
				return (0);
			/* FALLTROUGH */

#ifdef	GMATCH_CLERR_NORM
		def:
#endif
		default:
			if (sc == pc) {
				;
			} else {
backtrack:
				if (bt_p == NULL)
					return (0);
				if (*bt_s == '\0')
					return (0);

				nextwc(bt_s, sc);
				if (sc == '\0')
					return (0);
				p = bt_p;
				s = bt_s;
			}
		}
	}
	if (*s != 0)
		goto backtrack;
	return (1);
}

#endif	/* HAVE_GMATCH */
