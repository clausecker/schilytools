/* @(#)header.c	1.94 19/10/13 Copyright 2001-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)header.c	1.94 19/10/13 Copyright 2001-2018 J. Schilling";
#endif
/*
 *	Handling routines for StreamArchive header metadata.
 *	Based on the implementation from "star" to support
 *	POSIX.1-2001 extended archive headers.
 *
 *	This implementation is an archive format based only on
 *
 *		"%d %s=%s\n", <length>, <keyword>, <value>
 *
 *	type headers. The "length" field is the decimal length of the
 *	header field including the trailing newline. The "keyword" may
 *	include any UTF-8 character, the "value" is using UTF-8 or
 *	binary data.
 *
 *	Copyright (c) 2001-2018 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* getpagesize() */
#include <schily/libport.h>	/* getpagesize() */
#include <schily/utypes.h>
#include <schily/standard.h>
#include <schily/device.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/idcache.h>
#include <schily/strar.h>

#include "header.h"
#include "table.h"
#include "xtab.h"

#define	xbreset		strar_xbreset

#define	gxbuf		strar_gxbuf
#define	gxbsize		strar_gxbsize
#define	gxblen		strar_gxblen
#define	xbgrow		strar_xbgrow

#define	gen_text	strar_gen_text
#define	gen_xtime	strar_gen_xtime
#define	gen_number	strar_gen_number
#define	gen_unumber	strar_gen_unumber

#define	xhparse		strar_xhparse

#define	setnowarn	strar_setnowarn

LOCAL	BOOL	nowarn;

#define	MAX_UNAME	64	/* The maximum length of a user/group name */

/*
 * Flags for gen_text()
 */
#ifndef	T_ADDSLASH
#define	T_ADDSLASH	1	/* Add slash to the argument	*/
#define	T_UTF8		2	/* Convert arg to UTF-8 coding	*/
#endif

typedef struct _unknown unkn_t;

struct _unknown {
	unkn_t	*u_next;
	char	u_name[1];
};

LOCAL	void	_xbinit		__PR((void));
EXPORT	void	xbreset		__PR((void));
EXPORT	char	*gxbuf		__PR((void));
EXPORT	int	gxbsize		__PR((void));
EXPORT	int	gxblen		__PR((void));
EXPORT	void	xbgrow		__PR((int newsize));
#ifdef	__USED__
LOCAL	void	check_xtime	__PR((char *keyword, FINFO *info));
#endif
EXPORT	void	gen_xtime	__PR((char *keyword, time_t sec, Ulong nsec));
EXPORT	void	gen_unumber	__PR((char *keyword, Ullong arg));
EXPORT	void	gen_number	__PR((char *keyword, Llong arg));
#ifdef	__USED__
LOCAL	void	gen_iarray	__PR((char *keyword, ino_t *arg, int ents, int len));
#endif
EXPORT	void	gen_text	__PR((char *keyword, char *arg, int alen,
								Uint flags));
LOCAL	int	len_len		__PR((int len));
LOCAL	xtab_t	*lookup		__PR((char *cmd, int clen, xtab_t *cp));
EXPORT	BOOL	xhparse		__PR((FINFO *info, char	*p, char *ep));
LOCAL	void	print_unknown	__PR((char *keyword));
EXPORT	int	setnowarn	__PR((int val));
LOCAL	void	xh_rangeerr	__PR((char *keyword, char *arg, int len));
LOCAL	void	print_toolong	__PR((char *keyword, char *arg, int len));
LOCAL	BOOL	get_xtime	__PR((char *keyword, char *arg, int len,
						time_t *secp, long *nsecp));
LOCAL	void	get_atime	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_ctime	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_mtime	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	BOOL	get_xnumber	__PR((char *keyword, char *arg, Ullong *llp, char *type));
LOCAL	BOOL	get_unumber	__PR((char *keyword, char *arg, Ullong *ullp, Ullong maxval));
LOCAL	BOOL	get_snumber	__PR((char *keyword, char *arg, Ullong *ullp, BOOL *negp, Ullong minval, Ullong maxval));
LOCAL	void	get_uid		__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_gid		__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_uname	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_gname	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_path	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_lpath	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_size	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_status	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_mode	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_major	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_minor	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_fsmajor	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_fsminor	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_dev		__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_ino		__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_nlink	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_filetype	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_archtype	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_hdrcharset	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_dummy	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	unsup_arg	__PR((char *keyword, char *arg));
LOCAL	void	bad_utf8	__PR((char *keyword, char *arg));

/*
 * Convert unsigned integer into a decimal string.
 * The parameter 'val' is of the needed size (basic type), so it is as fast
 * as possible.
 *
 * The string is written starting from the end backwards for best speed.
 */
LOCAL	Uchar	dtab[] = "0123456789";

#define	ui2decimal(val, np)	do {					      \
					*--(np) = dtab[(val) % (unsigned)10]; \
					(val) = (val) / (unsigned)10;	      \
				} while ((val) > 0)

#define	scopy(to, from)		while ((*(to)++ = *(from)++) != '\0')	\
					;

LOCAL	char	*xbuf;	/* Space used to prepare I/O from/to extended headers */
LOCAL	int	xblen;	/* the length of the buffer for the extended headers */
LOCAL	int	xbidx;	/* The index where we start to prepare next entry    */

LOCAL	unkn_t	*unkn;	/* A list of unknown keywords to print warnings once */

/*
 * Important for the correctness of gen_number(): As long as we stay <= 55
 * chars for the keyword, a 128 bit number entry will fit into 100 chars.
 *
 *			x_name,		x_namelen, x_func,	x_flag
 */
LOCAL xtab_t xtab[] = {
			{ "atime",		5, get_atime,	0	},
			{ "ctime",		5, get_ctime,	0	},
			{ "mtime",		5, get_mtime,	0	},

			{ "uid",		3, get_uid,	0	},
			{ "uname",		5, get_uname,	0	},
			{ "gid",		3, get_gid,	0	},
			{ "gname",		5, get_gname,	0	},

			{ "path",		4, get_path,	0	},
			{ "linkpath",		8, get_lpath,	0	},

			{ "size",		4, get_size,	1	},

			{ "mode",		4, get_mode,	0	},

			{ "status",		6, get_status,	1	},

			{ "charset",		7, get_dummy,	0	},
			{ "comment",		7, get_dummy,	0	},
			{ "hdrcharset",		10, get_hdrcharset,	0	},

			{ "devmajor",		8, get_major,	0	},
			{ "devminor",		8, get_minor,	0	},

			{ "dev",		3, get_dev,	0	},
			{ "fsdevmajor",		10, get_fsmajor, 0	},
			{ "fsdevminor",		10, get_fsminor, 0	},
			{ "ino",		3, get_ino,	0	},
			{ "nlink",		5, get_nlink,	0	},

			{ "filetype",		8, get_filetype, 0	},
			{ "arfiletype",		10, get_filetype, 0	},
			{ "archtype",		8, get_archtype, 0	},

			{ NULL,			0, NULL,	0	}};

/*
 * Initialize the growable buffer used for reading the extended headers
 */
LOCAL void
_xbinit()
{
	xbuf = ___malloc(1, "growable xheader");
	xblen = 1;
	xbidx = 0;
}

EXPORT void
xbreset()
{
	if (xbuf == NULL)
		_xbinit();
	xbidx = 0;
}

EXPORT char *
gxbuf()
{
	return (xbuf);
}

EXPORT int
gxbsize()
{
	return (xbidx);
}

EXPORT int
gxblen()
{
	return (xblen);
}

/*
 * Grow the growable buffer used for reading the extended headers
 */
EXPORT void
xbgrow(newsize)
	int	newsize;
{
	char	*newbuf;
	int	i;
	int	ps = getpagesize();

	/*
	 * grow in pagesize units
	 */
	for (i = 0; i < newsize; i += ps)
		/* LINTED */
		;
	newsize = i + xblen;
	newbuf = ___realloc(xbuf, newsize, "growable xheader");
	xbuf = newbuf;
	xblen = newsize;
}

#ifdef	__USED__
LOCAL void
check_xtime(keyword, info)
	register char	*keyword;
	register FINFO	*info;
{
	long	l = 0;	/* Make GCC happy */

	if (*keyword == 'a')
		l = info->f_ansec;
	else if (*keyword == 'c')
		l = info->f_cnsec;
	else if (*keyword == 'm')
		l = info->f_mnsec;

	if (l >= 0 && l < 1000000000)
		return;

	/*
	 * check_xtime() is used in write mode so info->f_name is valid.
	 */
	errmsgno(EX_BAD, "Bad '%s' nsec value %ld for '%s'.\n",
		keyword, l, info->f_name);
	l = 0;

	if (*keyword == 'a')
		info->f_ansec = l;
	else if (*keyword == 'c')
		info->f_cnsec = l;
	else if (*keyword == 'm')
		info->f_mnsec = l;
}
#endif

/*
 * Create a time string with sub-second granularity.
 *
 * <length> <time-name-spec>=<seconds>.<nano-seconds>\n
 *
 *	00 atime=<seconds>.123456789\n
 *	00 ctime=<seconds>.123456789\n
 *	00 mtime=<seconds>.123456789\n
 *
 *	00 mtime=.123456789\n
 *	.123456789.123456789
 *
 * As we always emmit 9 digits for the sub-second part, the
 * length of this entry is always more than 20 and less than 100.
 * The size of an entry is 20 + the size of the "seconds" part of the entry.
 * With 64 bit unsigned numbers, the "seconds" part will be 20 char at max.
 * We may safely fill in the two digit <length> later, when we know the value.
 *
 * The code is highly optimised because in PAX (extended TAR) mode we are
 * spending a significant amount of user CPU time in it.
 */
EXPORT void
gen_xtime(keyword, sec, nsec)
		char	*keyword;
	register time_t	sec;
	register Ulong	nsec;
{
		char	nb[32];
	register char	*p;
	register char	*np;
	register int	len;

	if (nsec >= 1000000000)	/* We would create an unusable string */
		nsec = 0;

	if ((xbidx + 100) > xblen)
		xbgrow(100);

	/*
	 * The following code is equivalent to:
	 *
	 * sprintf(p, "%d %s=%lld.%9.9ld\n", length, keyword, (Llong)sec, nsec)
	 *
	 * But is twice as fast.
	 */
	len = 20;		/* Base length, second field must be added  */
	p = &xbuf[xbidx+2];	/* point past length field		    */
	/*
	 * Fill in " ?time="
	 */
	*p++ = ' ';
	if (keyword[0] == 'a' || keyword[0] == 'c' || keyword[0] == 'm') {
		*p++ = keyword[0];
		*p++ = 't'; *p++ = 'i'; *p++ = 'm'; *p++ = 'e';
	} else {
		len = 15;	/* 2digitlen + ' ' + '=' + '.' + nsec + '\n' */
		np = keyword;
		while ((*p++ = *np++) != '\0')
			len++;
		--p;
	}
	*p++ = '=';
	if (sec < 0) {		/* Time is before 01.01.1970 ...	    */
		*p++ = '-';
		sec = -sec;
		len += 1;
	}

	np = &nb[sizeof (nb)-1]; /* Point to end of number string buffer    */
	*np = '\0';		/* Null terminate			    */
	ui2decimal(sec, np);	/* Convert to unsigned decimal string	    */
	len += &nb[sizeof (nb)-1] - np;

	scopy(p, np);		/* Copy number string buffer to xheader	    */
	--p,			/* Overshoot compensation		    */
	*p++ = '.';		/* Decimal point between secs and nsecs	    */

	np = &p[10];		/* Point to end of <nsec> string part	    */
	*np = '\0';		/* Null terminate			    */
	*--np = '\n';		/* Newline at end of line		    */
	ui2decimal(nsec, np);	/* Convert to unsigned decimal string	    */
	while (np > p)
		*--np = '0';	/* Left fill with '0' to 9 digits	    */

	np = &xbuf[xbidx+2];	/* Point to end of length string part	    */
	xbidx += len;		/* 'len' is trashed in ui2decimal()	    */
	ui2decimal(len, np);	/* Convert to unsigned decimal string	    */
}

/*
 * Create a generic unsigned number string.
 *
 * <length> <name-spec>=<value>\n
 *
 * The length of this entry is always less than 100 chars if the length of the
 * 'name-spec' is less than 75 chars (the maximum length of a 64 bit number in
 * decimal is 20 digits). Even in case of a 128 bit number length will be less
 * than 100 chars if the length of 'name-spec' is less than 55 chars.
 *
 * The code is highly optimised because in dump mode when using extended TAR
 * headers with additional fields, we are spending a significant amount of
 * user CPU time in it.
 */
EXPORT void
gen_unumber(keyword, arg)
	register char	*keyword;
	register Ullong	arg;
{
		char	nb[64];	/* 41 is enough for unsigned 128 bit ints    */
	register char	*p;
	register char	*np;
	register int	len;
	register int	i;

	if ((xbidx + 100) > xblen)
		xbgrow(100);

	/*
	 * The following code is equivalent to:
	 *
	 * sprintf(p, "%d %s=%llu\n", length, keyword, arg);
	 *
	 * But is twice as fast.
	 */
	np = &nb[sizeof (nb)-1]; /* Point to end of number string buffer    */
	*np = '\0';		/* Null terminate			    */
	ui2decimal(arg, np);	/* Convert to unsigned decimal string	    */

	len = strlen(keyword);	/* Compute the length, start with strlen(kw) */

	len += &nb[sizeof (nb)-1] - np;	/* Add strlen(number)		    */
	len += 2 + 3;		/* Add 2 for strlen(len) and 3 for " =\n"   */

	if (len < 10) {		/* If < 10, the len field is 1 digit only   */
		len -= 1;	/* This happens when strlen(keyword) is < 4 */
		i = 1;		/* and number is one digit only.	    */
	} else {
		i = 2;
	}
	p = &xbuf[xbidx+i];	/* Point past length field		    */
	*p++ = ' ';		/* Fill in space after length field	    */
	scopy(p, keyword);	/* Copy keyword into to xheader		    */
	--p,			/* Overshoot compensation		    */
	*p++ = '=';		/* Fill in '=' after keyword field	    */

	scopy(p, np);		/* Copy number string buffer to xheader	    */
	*--p = '\n';		/* Newline at end of line		    */

	np = &xbuf[xbidx+i];	/* Point to end of length string part	    */
	xbidx += len;		/* 'len' is trashed in ui2decimal()	    */
	ui2decimal(len, np);	/* Convert to unsigned decimal string	    */
}

/*
 * Create a generic signed number string.
 *
 * <length> <name-spec>=<value>\n
 *
 * The length of this entry is always less than 100 chars if the length of the
 * 'name-spec' is less than 75 chars (the maximum length of a 64 bit number in
 * decimal is 20 digits). Even in case of a 128 bit number length will be less
 * than 100 chars if the length of 'name-spec' is less than 55 chars.
 *
 * The code is highly optimised because in dump mode when using extended TAR
 * headers with additional fields, we are spending a significant amount of
 * user CPU time in it.
 */
EXPORT void
gen_number(keyword, arg)
	register char	*keyword;
	register Llong	arg;
{
		char	nb[64];	/* 41 is enough for unsigned 128 bit ints    */
	register char	*p;
	register char	*np;
	register int	len;
	register int	i;
		BOOL	neg = FALSE;

	if ((xbidx + 100) > xblen)
		xbgrow(100);

	/*
	 * The following code is equivalent to:
	 *
	 * sprintf(p, "%d %s=%lld\n", length, keyword, arg);
	 *
	 * But is twice as fast.
	 */
	np = &nb[sizeof (nb)-1]; /* Point to end of number string buffer    */
	*np = '\0';		/* Null terminate			    */
	if (arg < 0) {
		arg = -arg;
		neg = TRUE;
	}
	ui2decimal(arg, np);	/* Convert to unsigned decimal string	    */
	if (neg)
		*--np = '-';

	len = strlen(keyword);	/* Compute the length, start with strlen(kw) */

	len += &nb[sizeof (nb)-1] - np;	/* Add strlen(number)		    */
	len += 2 + 3;		/* Add 2 for strlen(len) and 3 for " =\n"   */

	if (len < 10) {		/* If < 10, the len field is 1 digit only   */
		len -= 1;	/* This happens when strlen(keyword) is < 4 */
		i = 1;		/* and number is one digit only.	    */
	} else {
		i = 2;
	}
	p = &xbuf[xbidx+i];	/* Point past length field		    */
	*p++ = ' ';		/* Fill in space after length field	    */
	scopy(p, keyword);	/* Copy keyword into to xheader		    */
	--p,			/* Overshoot compensation		    */
	*p++ = '=';		/* Fill in '=' after keyword field	    */

	scopy(p, np);		/* Copy number string buffer to xheader	    */
	*--p = '\n';		/* Newline at end of line		    */

	np = &xbuf[xbidx+i];	/* Point to end of length string part	    */
	xbidx += len;		/* 'len' is trashed in ui2decimal()	    */
	ui2decimal(len, np);	/* Convert to unsigned decimal string	    */
}

/*
 * Create a string from an array of unsigned inode numbers.
 *
 * <length> <keyword>=<values>\n
 *
 * <values> is a space separated list of unsigned integers in decimal ascii.
 *
 * The len parameter is the estimated length of the whole string. It is used
 * to pre-allocate a buffer that hopefully has the right size already in order
 * to avoid copying the content.
 *
 * The code is highly optimised because in dump mode when using extended TAR
 * headers with additional fields, we are spending a significant amount of
 * user CPU time in it.
 */
#ifdef	__USED__
LOCAL void
gen_iarray(keyword, arg, ents, len)
	register char	*keyword;
		ino_t	*arg;
		int	ents;
	register int	len;	/* Estimated length */
{
		char	nb[64];	/* 41 is enough for unsigned 128 bit ints    */
	register Ullong	ll;
	register char	*p;
	register char	*np;
	register int	i;
	register int	llen;
	register int	olen;

	/*
	 * The following code is equivalent to:
	 *
	 * sprintf(p, "%d %s=%llu %llu ...\n", length, keyword, arg[...]...);
	 *
	 * But avoids copying if possible.
	 */
	i = len;
	if (len <= 0)			/* No estimated length already	    */
		len = strlen(keyword) + 3; /* + ' ' + '=' + '\n'	    */
	olen = len;
	len += llen = len_len(len);	/* add length of length string	    */

	if (i <= 0)
		i = len + ents * 2;	/* Make a minimal guess on the len  */
	else
		i = len;		/* Use guessed value from parameter */

	if ((xbidx + i) > xblen)
		xbgrow(i);

	p = &xbuf[xbidx+llen];	/* Point past length field		    */
	*p++ = ' ';		/* Fill in space after length field	    */
	scopy(p, keyword);	/* Copy keyword into to xheader		    */
	--p,			/* Overshoot compensation		    */
	*p++ = '=';		/* Fill in '=' after keyword field	    */

	len = p - &xbuf[xbidx];	/* strlen(keyword) + ' ' + '='		    */
	for (i = 0; i < ents; i++) {
		if (((p - xbuf) + 100) > xblen) {
			register int	xb_idx;
			/*
			 * The address for xbuf may change in case that
			 * realloc() cannot grow the current memory chunk,
			 * recalculate 'p' in this case.
			 */
			xb_idx = p - xbuf;
			xbgrow(100);
			p = xbuf + xb_idx;
		}
		ll = (Llong)arg[i];
		np = &nb[sizeof (nb)-1]; /* Point to end of number str. buf */
		*np = '\0';		/* Null terminate		    */
		ui2decimal(ll, np);	/* Convert to unsigned decimal str. */
		len += &nb[sizeof (nb)-1] - np;	/* strlen(number)	    */
		len += 1;		/* Space (' ') or Newline ('\n')    */

		scopy(p, np);		/* Copy number str. buf to xheader  */
		p[-1] = ' ';		/* Space at end of number	    */
	}
	*--p = '\n';			/* Overwrite last space by newline  */
	i = p - &xbuf[xbidx] + 1; /* Total string length		    */
	i -= llen;		/* Subtract old length length value	    */

	if (olen != i) {	/* Length without length string changed	    */
		olen = llen;	/* Save old length string length	    */
		llen = len_len(i);	/* New length of length string	    */
		if (olen != llen) {	/* We need to move the whole text   */
			p = &xbuf[xbidx+olen];
			olen = llen - olen;
			movebytes(p, p+olen, i);
		}
		len = i + llen;
	}

	np = &xbuf[xbidx+llen];	/* Point to end of length string part	    */
	*np = ' ';		/* May have been overwritten by movebytes() */
	xbidx += len;		/* 'len' is trashed in ui2decimal()	    */
	ui2decimal(len, np);	/* Convert to unsigned decimal string	    */
}
#endif	/* __USED__ */

/*
 * Create a generic text string in UTF-8 coding.
 *
 * <length> <name-spec>=<value>\n
 *
 * This function will have to carefully check for the resultant length
 * and thus compute the total length in advance. If the rare case that the
 * UTF-8 conversion changes the length so much that the length of the length
 * string will be different from the estimated value, we need to move whole
 * text by one.
 *
 * The code is highly optimised because in dump mode when using extended TAR
 * headers with additional fields, we may need to copy directory listings
 * that are longer then 100000 bytes.
 */
EXPORT void
gen_text(keyword, arg, alen, flags)
	register char	*keyword;
		char	*arg;
		int	alen;
		Uint	flags;
{
	register char	*p;
	register char	*np;
	register int	len;
	register int	i;
	register int	llen;
	register int	olen;

	/*
	 * The following code is equivalent to:
	 *
	 * sprintf(p, "%d %s=%s\n", length, keyword, arg);
	 *
	 * But avoids copying if possible.
	 */
	if ((len = alen) == -1)
		len = strlen(arg);
	alen = len;
	if (flags & T_ADDSLASH)		/* only used if 'path' is a dir	    */
		len++;

	len += strlen(keyword) + 3;	/* + ' ' + '=' + '\n'		    */
	olen = len;
	len += llen = len_len(len);	/* add length of length string	    */

	i = len;
	if (flags & T_UTF8)
		i *= 6;			/* UTF-8 Factor may be up to 6!	    */
	if ((xbidx + i) > xblen)
		xbgrow(i);


	p = &xbuf[xbidx+llen];	/* Point past length field		    */
	*p++ = ' ';		/* Fill in space after length field	    */
	scopy(p, keyword);	/* Copy keyword into to xheader		    */
	--p,			/* Overshoot compensation		    */
	*p++ = '=';		/* Fill in '=' after keyword field	    */

	if (flags & T_UTF8) {
		i = to_utf8((Uchar *)p, i, (Uchar *)arg, alen);
		p += i + 1;
	} else {
		p = movebytes(arg, p, alen);
		p++;
	}

	if (flags & T_ADDSLASH) { /* only used if 'path' is a dir	    */
		i++;		/* String length increases by '/'	    */
		*--p = '/';	/* Slash at end of string		    */
		*++p = '\n';	/* Newline at end of line		    */
	} else {
		*--p = '\n';	/* Newline at end of line		    */
	}
	i = p - &xbuf[xbidx] + 1; /* Total string length		    */
	i -= llen;		/* Subtract old length length value	    */

	if (olen != i) {	/* Length without length string changed	    */
		olen = llen;	/* Save old length string length	    */
		llen = len_len(i);	/* New length of length string	    */
		if (olen != llen) {	/* We need to move the whole text   */
			p = &xbuf[xbidx+olen];
			olen = llen - olen;
			movebytes(p, p+olen, i);
		}
		len = i + llen;
	}

	np = &xbuf[xbidx+llen];	/* Point to end of length string part	    */
	*np = ' ';		/* May have been overwritten by movebytes() */
	xbidx += len;		/* 'len' is trashed in ui2decimal()	    */
	ui2decimal(len, np);	/* Convert to unsigned decimal string	    */
}

LOCAL int
len_len(len)
	register int	len;
{
	if (len <= 8)
		return (1);
	if (len <= 97)
		return (2);
	if (len <= 996)
		return (3);
	if (len <= 9995)
		return (4);
	if (len <= 99994)
		return (5);
	if (len <= 999993)
		return (6);
	if (len <= 9999992)
		return (7);
	if (len <= 99999991)
		return (8);
	if (len <= 999999990)
		return (9);

	return (10);
}

/*
 * Lookup command in command tab
 */
LOCAL xtab_t *
lookup(cmd, clen, cp)
	register char	*cmd;
	register int	clen;
	register xtab_t	*cp;
{
	for (; cp->x_name; cp++) {
		register int	len = cp->x_namelen-1;

		if (cp->x_name[len] == '*') {
			if (strncmp(cmd, cp->x_name, len) == 0)
				return (cp);
		}
		if (clen != cp->x_namelen)
			continue;

		if ((*cmd == *cp->x_name) &&
		    strcmp(cmd, cp->x_name) == 0) {
			return (cp);
		}
	}
	return ((xtab_t *)NULL);
}

EXPORT BOOL
xhparse(info, p, ep)
	register FINFO	*info;
	register char	*p;
	register char	*ep;
{
	register xtab_t	*cp;
	register char	*keyword;
	register char 	*arg;
		long	length;

	while (p < ep) {
		register int klen;

		if (*p < '0' || *p > '9') {
			errmsgno(EX_BAD,
			"Syntax error in extended header: non digit in len.\n");
			return (FALSE);
		}
		keyword = astolb(p, &length, 10);
		if (*keyword != ' ') {
			errmsgno(EX_BAD,
			"Syntax error in extended header: missing ' '.\n");
			return (FALSE);
		}
		keyword++;
		arg = strchr(keyword, '=');
		klen = arg - keyword;
		if ((arg == NULL) || (klen > length)) {
			errmsgno(EX_BAD,
			"Syntax error in extended header: missing '='.\n");
			return (FALSE);
		}
		*arg++ = '\0';			/* Kill equal sign */

		if (*(p + length -1) != '\n') {
			arg[-1] = '=';
			errmsgno(EX_BAD,
			"Syntax error in extended header: missing '\\n'.\n");
			return (FALSE);
		}
		*(p + length -1) = '\0';	/* Kill new-line character */

		if ((cp = lookup(keyword, klen, xtab)) != NULL) {
			(*cp->x_func)(info, keyword, klen,
						arg, length - (arg-p) - 1);
		} else {
			print_unknown(keyword);
		}
		arg[-1] = '=';
		p += length;
		p[-1] = '\n';
if (cp && cp->x_flag)
	return (TRUE+1);
	}
	return (TRUE);
}

LOCAL void
print_unknown(keyword)
	register char	*keyword;
{
	register unkn_t	*up = unkn;

	while (up) {
		if (streql(keyword, up->u_name))
			break;
		up = up->u_next;
	}
	if (up == NULL) {
		errmsgno(EX_BAD,
			"Unknown extended header keyword '%s' ignored.\n",
				keyword);

		up = ___malloc(sizeof (*up) + strlen(keyword), "unknown list");
		strcpy(up->u_name, keyword);
		up->u_next = unkn;
		unkn = up;
	}
}

EXPORT int
setnowarn(val)
	int	val;
{
	int	old	= nowarn;

	nowarn = val;

	return (old);
}

LOCAL void
xh_rangeerr(keyword, arg, len)
	char	*keyword;
	char	*arg;
	int	len;
{
	if (nowarn)
		return;
	errmsgno(EX_BAD,
		"WARNING: %s '%.*s' in extended header exceeds local range.\n",
		keyword, len, arg);
}

LOCAL void
print_toolong(keyword, arg, len)
	char	*keyword;
	char	*arg;
	int	len;
{
	if (nowarn)
		return;
	errmsgno(EX_BAD,
		"WARNING: %s '%.*s' in extended header too long, ignoring.\n",
		keyword, len, arg);
}

/*
 * generic function to read args that hold times
 *
 * The time may either be in second resolution or in sub-second resolution.
 * In the latter case the second fraction and the sub second fraction
 * is separated by a dot ('.').
 */
LOCAL BOOL
get_xtime(keyword, arg, len, secp, nsecp)
	char	*keyword;
	char	*arg;
	int	len;
	time_t	*secp;
	long	*nsecp;
{
	Llong	ll;
	long	l;
	int	flen;
	char	*p;

	p = astollb(arg, &ll, 10);
	if (*p == '\0' || *p == '.') {
		time_t	secs = ll;
		if (secs != ll) {
			xh_rangeerr(keyword, arg, len);
#ifdef	__use_default_time__
			*secp = ddate.tv_sec;
#endif
			*secp = 0;
			goto bad;
		}
		*secp = ll;		/* XXX Check for NULL ptr? */
	}
	if (*p == '\0') {		/* time has second resolution only */
		if (nsecp == NULL)
			return (TRUE);
		*nsecp = 0;
		return (TRUE);
	} else if (*p == '.') {		/* time has sub-second resolution */
		flen = strlen(++p);	/* remember resolution		    */
		if (flen > 9)		/* if resolution is better than	    */
			p[9] = '\0';	/* nanoseconds kill rest of line as */
		p = astolb(p, &l, 10);	/* we don't understand more.	    */
		if (*p == '\0') {	/* number read correctly	    */
			if (l >= 0) {
				while (flen < 9) {	/* convert to nsecs */
					l *= 10;
					flen++;
				}
				if (nsecp == NULL)
					return (TRUE);
				*nsecp = l;
				return (TRUE);
			}
		}
	}
bad:
	errmsgno(EX_BAD, "Bad timespec '%s' for '%s' in extended header.\n",
		arg, keyword);
	return (FALSE);
}

/*
 * get read access time from extended header
 */
/* ARGSUSED */
LOCAL void
get_atime(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_ATIME;
		return;
	}
	if (get_xtime(keyword, arg, len, &info->f_atime, &info->f_ansec))
		info->f_xflags |= XF_ATIME;
}

/*
 * get inode status change time from extended header
 */
/* ARGSUSED */
LOCAL void
get_ctime(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_CTIME;
		return;
	}
	if (get_xtime(keyword, arg, len, &info->f_ctime, &info->f_cnsec))
		info->f_xflags |= XF_CTIME;
}

/*
 * get modification time from extended header
 */
/* ARGSUSED */
LOCAL void
get_mtime(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_MTIME;
		return;
	}
	if (get_xtime(keyword, arg, len, &info->f_mtime, &info->f_mnsec))
		info->f_xflags |= XF_MTIME;
}

/*
 * generic function to read args that hold unsigned decimal numbers
 */
LOCAL BOOL
get_xnumber(keyword, arg, ullp, type)
	char	*keyword;
	char	*arg;
	Ullong	*ullp;
	char	*type;
{
	Ullong	ull;
	char	*p;

	seterrno(0);
	p = astoullb(arg, &ull, 10);
	if (*p == '\0') {		/* number read correctly */
		if (geterrno() != 0) {
			errmsgno(EX_BAD,
			"Number overflow with '%s' for '%s' in extended header.\n",
			arg, keyword);
			return (FALSE);
		}
		*ullp = ull;		/* XXX Check for NULL ptr? */
		return (TRUE);
	}
	errmsgno(EX_BAD, "Bad %s number '%s' for '%s' in extended header.\n",
		type,
		arg, keyword);
	return (FALSE);
}

LOCAL BOOL
get_unumber(keyword, arg, ullp, maxval)
	char	*keyword;
	char	*arg;
	Ullong	*ullp;
	Ullong	maxval;
{
	if (!get_xnumber(keyword, arg, ullp, "unsigned"))
		return (FALSE);

	if (*ullp > maxval) {
		errmsgno(EX_BAD,
		"Value '%s' is out of range 0..%llu for '%s' in extended header.\n",
		arg, maxval, keyword);
		return (FALSE);
	}
	return (TRUE);
}


/*
 * generic function to read args that hold signed decimal numbers
 */
LOCAL BOOL
get_snumber(keyword, arg, ullp, negp, minval, maxval)
	char	*keyword;
	char	*arg;
	Ullong	*ullp;
	BOOL	*negp;
	Ullong	minval;
	Ullong	maxval;
{
	char	*p = arg;
#define	is_space(c)	 ((c) == ' ' || (c) == '\t')

	while (is_space(*p))
		p++;

	*negp = FALSE;
	if (*p == '-') {
		p++;
		*negp = TRUE;
	}
	return (get_xnumber(keyword, p, ullp, "signed"));
}

/*
 * get user id (if > 2097151)
 * POSIX requires uid_t to be a signed int but the values for uid_t to be
 * non-negative.
 * We allow signed ints and carefully check the values.
 */
/* ARGSUSED */
LOCAL void
get_uid(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;

	if (len == 0) {
		info->f_xflags &= ~XF_UID;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)UID_T_MIN, UID_T_MAX)) {
		info->f_xflags |= XF_UID;
		if (neg)
			info->f_uid = -ull;
		else
			info->f_uid = ull;

		if ((neg && -info->f_uid != ull) ||
			(!neg && info->f_uid != ull)) {

			xh_rangeerr(keyword, arg, len);
			info->f_flags |= F_BAD_UID;
			info->f_uid = ic_uid_nobody();
		}
	}
}

/*
 * get group id (if > 2097151)
 * POSIX requires gid_t to be a signed int but the values for gid_t to be
 * non-negative.
 * We allow signed ints and carefully check the values.
 */
/* ARGSUSED */
LOCAL void
get_gid(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;

	if (len == 0) {
		info->f_xflags &= ~XF_GID;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)GID_T_MIN, GID_T_MAX)) {
		info->f_xflags |= XF_UID;
		if (neg)
			info->f_gid = -ull;
		else
			info->f_gid = ull;

		if ((neg && -info->f_gid != ull) ||
			(!neg && info->f_gid != ull)) {

			xh_rangeerr(keyword, arg, len);
			info->f_flags |= F_BAD_GID;
			info->f_gid = ic_gid_nobody();
		}
	}
}

/*
 * Space for returning user/group names.
 * XXX If we ever change to use allocated space, we need to change info_xcopy()
 */
LOCAL	Uchar	_uname[MAX_UNAME+2];
LOCAL	Uchar	_gname[MAX_UNAME+2];

/*
 * get user name (if name length is > 32 chars or if contains non ASCII chars)
 */
/* ARGSUSED */
LOCAL void
get_uname(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_UNAME;
		return;
	}
	if (len > MAX_UNAME) {
		print_toolong(keyword, arg, len);
		return;
	}
	if (info->f_xflags & XF_BINARY) {
		strcpy((char *)_uname, arg);
		info->f_xflags |= XF_UNAME;
		info->f_uname = (char *)_uname;
		info->f_umaxlen = len;
	} else if (from_utf8(_uname, sizeof (_uname), (Uchar *)arg, &len)) {
		info->f_xflags |= XF_UNAME;
		info->f_uname = (char *)_uname;
		info->f_umaxlen = len;
	} else {
		bad_utf8(keyword, arg);
	}
}

/*
 * get group name (if name length is > 32 chars or if contains non ASCII chars)
 */
/* ARGSUSED */
LOCAL void
get_gname(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_GNAME;
		return;
	}
	if (len > MAX_UNAME) {
		print_toolong(keyword, arg, len);
		return;
	}
	if (info->f_xflags & XF_BINARY) {
		strcpy((char *)_gname, arg);
		info->f_xflags |= XF_GNAME;
		info->f_gname = (char *)_gname;
		info->f_gmaxlen = len;
	} else if (from_utf8(_gname, sizeof (_gname), (Uchar *)arg, &len)) {
		info->f_xflags |= XF_GNAME;
		info->f_gname = (char *)_gname;
		info->f_gmaxlen = len;
	} else {
		bad_utf8(keyword, arg);
	}
}

/*
 * get path (if name length is > 100-255 chars or if contains non ASCII chars)
 */
/* ARGSUSED */
LOCAL void
get_path(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_PATH;
		return;
	}
	if (len > PATH_MAX) {
		print_toolong(keyword, arg, len);
		return;
	}
	/*
	 * Check whether we are called via get_xhtype() -> xhparse()
	 */
	if (info->f_name == NULL)
		return;
	if (info->f_xflags & XF_BINARY) {
		strcpy(info->f_name, arg);
		info->f_xflags |= XF_PATH;
	} else if (from_utf8((Uchar *)info->f_name, PATH_MAX+1, (Uchar *)arg, &len)) {
		info->f_xflags |= XF_PATH;
	} else {
		bad_utf8(keyword, arg);
	}
}

/*
 * get link path (if name length is > 100 chars or if contains non ASCII chars)
 */
/* ARGSUSED */
LOCAL void
get_lpath(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_LINKPATH;
		return;
	}
	if (len > PATH_MAX) {
		print_toolong(keyword, arg, len);
		return;
	}
	/*
	 * Check whether we are called via get_xhtype() -> xhparse()
	 */
	if (info->f_lname == NULL)
		return;
	if (info->f_xflags & XF_BINARY) {
		strcpy(info->f_lname, arg);
		info->f_xflags |= XF_LINKPATH;
	} else if (from_utf8((Uchar *)info->f_lname, PATH_MAX+1, (Uchar *)arg, &len)) {
		info->f_xflags |= XF_LINKPATH;
	} else {
		bad_utf8(keyword, arg);
	}
}

/*
 * get size, either real size or size on tape (usually when size is > 8 GB)
 * The file size is doubtlessly an ungined integer
 */
/* ARGSUSED */
LOCAL void
get_size(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		info->f_xflags &= ~XF_SIZE;
		return;
	}
	if (get_unumber(keyword, arg, &ull, OFF_T_MAX)) {
		info->f_xflags |= XF_SIZE;
		info->f_llsize = ull;
		info->f_rsize = (off_t)ull;
		if (info->f_rsize != ull) {
			xh_rangeerr(keyword, arg, len);
			ull = 0;
			info->f_flags |= (F_BAD_META | F_BAD_SIZE);
			info->f_rsize = (off_t)ull;
		}
		/*
		 * If real size is not yet known, copy over the tape size to
		 * avoid problems. If real size is found later, it will
		 * overwrite unconditional.
		 */
		if ((info->f_xflags & XF_REALSIZE) == 0) {
			info->f_xflags |= XF_REALSIZE;
			info->f_size = (off_t)ull;
		}
	}
}

/*
 * get file send status
 * The file size is doubtlessly an ungined integer
 */
/* ARGSUSED */
LOCAL void
get_status(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;

	if (len == 0) {
		info->f_xflags &= ~XF_STATUS;
		return;
	}
	if (*arg == 'E' && streql(arg, "EOF")) {
		info->f_xflags |= XF_EOF;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)UID_T_MIN, OFF_T_MAX)) {
		info->f_xflags |= XF_STATUS;
		if (neg)
			info->f_status = -ull;
		else
			info->f_status = ull;
		if ((neg && -info->f_status != ull) ||
			(!neg && info->f_status != ull)) {
			xh_rangeerr(keyword, arg, len);
			ull = -1L;
			info->f_flags |= (F_BAD_META | F_BAD_SIZE);
			info->f_status = ull;
		}
	}
}

/* ARGSUSED */
LOCAL void
get_mode(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		return;
	}
	if (len > 24) {
		print_toolong(keyword, arg, len);
		return;
	}
	if (getperm(NULL, arg, NULL, &info->f_mode, (mode_t)0, 0) >= 0) {
		info->f_xflags |= XF_MODE;
	} else {
	}
}

/*
 * get major device number (always vendor unique)
 * The major device number should be unsigned but POSIX does not say anything
 * about the content and defined dev_t to be a signed int.
 */
/* ARGSUSED */
LOCAL void
get_major(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;
	dev_t	d;

	if (len == 0) {
		info->f_xflags &= ~XF_DEVMAJOR;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)MAJOR_T_MIN, MAJOR_T_MAX)) {
		info->f_xflags |= XF_DEVMAJOR;
		if (neg)
			info->f_rdevmaj = -ull;
		else
			info->f_rdevmaj = ull;
		d = makedev(info->f_rdevmaj, 0);
		d = major(d);
		if ((neg && -d != ull) || (!neg && d != ull)) {
			xh_rangeerr(keyword, arg, len);
			info->f_flags |= F_BAD_META;
		}
	}
}

/*
 * get minor device number (always vendor unique)
 * The minor device number should be unsigned but POSIX does not say anything
 * about the content and defined dev_t to be a signed int.
 */
/* ARGSUSED */
LOCAL void
get_minor(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;
	dev_t	d;

	if (len == 0) {
		info->f_xflags &= ~XF_DEVMINOR;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)MINOR_T_MIN, MINOR_T_MAX)) {
		info->f_xflags |= XF_DEVMINOR;
		if (neg)
			info->f_rdevmin = -ull;
		else
			info->f_rdevmin = ull;
		d = makedev(0, info->f_rdevmin);
		d = minor(d);
		if ((neg && -d != ull) || (!neg && d != ull)) {
			xh_rangeerr(keyword, arg, len);
			info->f_flags |= F_BAD_META;
		}
	}
}

/*
 * get major device number for st_dev (always vendor unique)
 * The major device number should be unsigned but POSIX does not say anything
 * about the content and defined dev_t to be a signed int.
 */
/* ARGSUSED */
LOCAL void
get_fsmajor(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;
	dev_t	d;

	if (len == 0) {
		info->f_xflags &= ~XF_FSDEVMAJOR;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)MAJOR_T_MIN, MAJOR_T_MAX)) {
		info->f_xflags |= XF_FSDEVMAJOR;
		if (neg)
			info->f_devmaj = -ull;
		else
			info->f_devmaj = ull;
		d = makedev(info->f_devmaj, 0);
		d = major(d);
		if ((neg && -d != ull) || (!neg && d != ull)) {
			xh_rangeerr(keyword, arg, len);
			info->f_flags |= F_BAD_META;
		}
	}
	if (info->f_xflags & XF_FSDEVMINOR)
		info->f_dev = makedev(info->f_devmaj, info->f_devmin);
}

/*
 * get minor device number for st_dev (always vendor unique)
 * The minor device number should be unsigned but POSIX does not say anything
 * about the content and defined dev_t to be a signed int.
 */
/* ARGSUSED */
LOCAL void
get_fsminor(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;
	dev_t	d;

	if (len == 0) {
		info->f_xflags &= ~XF_FSDEVMINOR;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)MINOR_T_MIN, MINOR_T_MAX)) {
		info->f_xflags |= XF_FSDEVMINOR;
		if (neg)
			info->f_devmin = -ull;
		else
			info->f_devmin = ull;
		d = makedev(0, info->f_devmin);
		d = minor(d);
		if ((neg && -d != ull) || (!neg && d != ull)) {
			xh_rangeerr(keyword, arg, len);
			info->f_flags |= F_BAD_META;
		}
	}
	if (info->f_xflags & XF_FSDEVMAJOR)
		info->f_dev = makedev(info->f_devmaj, info->f_devmin);
}

/*
 * get device number of device containing FS (vendor unique)
 * The device number should be unsigned but POSIX does not say anything
 * about the content and defined dev_t to be a signed int.
 */
/* ARGSUSED */
LOCAL void
get_dev(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;

	if (len == 0) {
		info->f_dev = 0;
		return;
	}
	/*
	 * fsdevmajor and fsdevminor win
	 */
	if (info->f_xflags & (XF_FSDEVMAJOR|XF_FSDEVMINOR))
		return;

	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)DEV_T_MIN, DEV_T_MAX)) {
		if (neg)
			info->f_dev = -ull;
		else
			info->f_dev = ull;

		if ((neg && -info->f_dev != ull) ||
			(!neg && info->f_dev != ull)) {
			xh_rangeerr(keyword, arg, len);
			info->f_dev = 0;
		}
	}
}

/*
 * get inode number for this file (vendor unique)
 * POSIX defines ino_t to be unsigned.
 */
/* ARGSUSED */
LOCAL void
get_ino(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		info->f_ino = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, INO_T_MAX)) {
		info->f_ino = ull;
		if (info->f_ino != ull) {
			xh_rangeerr(keyword, arg, len);
			info->f_ino = 0;
		}
	}
}

/*
 * get link count for this file (vendor unique)
 * POSIX defines nlink_t to ne signed but all real link counts in archives
 * need to be positive numbers.
 */
/* ARGSUSED */
LOCAL void
get_nlink(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		info->f_nlink = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, NLINK_T_MAX)) {
		info->f_nlink = ull;
		if (info->f_nlink != ull) {
			xh_rangeerr(keyword, arg, len);
			info->f_nlink = 0;
		}
	}
}

/*
 * get tar file type or real file type for this file (vendor unique)
 */
/* ARGSUSED */
LOCAL void
get_filetype(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	int	i;

	if (len == 0) {
		if (keyword[0] == 'f')		/* "filetype" */
			info->f_rxftype = XT_BAD;
		else
			info->f_xftype = XT_BAD;
		return;
	}

	for (i = 0; i <= XT_BAD; i++) {
		if (xtnamelen_tab[i] != len)
			continue;
		if (streql(xttoname_tab[i], arg))
			break;
	}
	if (i >= XT_BAD)
		return;

	if (keyword[0] == 'f') {		/* "filetype" */
		info->f_xflags |= XF_FILETYPE;
		info->f_rxftype = i;
	} else {				/* "arfiletype" */
		info->f_xftype = i;
	}
}

/* ARGSUSED */
LOCAL void
get_archtype(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		return;
	}
	if (!streql(arg, "StreamArchive")) {
		errmsgno(EX_BAD, "Bad arg '%s' for '%s' in extended header.\n",
		arg, keyword);
	}
}

/*
 * Get the charset used for path, linkpath, uname and gname.
 */
/* ARGSUSED */
LOCAL void
get_hdrcharset(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 23 && streql("ISO-IR 10646 2000 UTF-8", arg)) {
		info->f_xflags &= ~XF_BINARY;
	} else if (len == 6 && streql("BINARY", arg)) {
		info->f_xflags |= XF_BINARY;
	} else {
		unsup_arg(keyword, arg);
	}
}

/*
 * Dummy 'get' function used for all fields that we don't yet understand or
 * fields that we indent to ignore.
 */
/* ARGSUSED */
LOCAL void
get_dummy(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
}

LOCAL void
unsup_arg(keyword, arg)
	char	*keyword;
	char	*arg;
{
	errmsgno(EX_BAD,
		"Unsupported arg '%s' for '%s' in extended header.\n",
		arg, keyword);
}

LOCAL void
bad_utf8(keyword, arg)
	char	*keyword;
	char	*arg;
{
	errmsgno(EX_BAD, "Bad UTF-8 arg '%s' for '%s' in extended header.\n",
		arg, keyword);
}
