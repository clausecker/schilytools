/* @(#)filewrite.c	1.15 07/04/03 Copyright 1986, 1995-2007 J. Schilling */
/*
 *	Copyright (c) 1986, 1995-2007 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include "schilyio.h"

static	char	_writeerr[]	= "file_write_err";

#ifdef	HAVE_USG_STDIO

EXPORT int
filewrite(f, vbuf, len)
	register FILE	*f;
	void	*vbuf;
	int	len;
{
	register int	n;
	int	cnt;
	char		*buf = vbuf;

	down2(f, _IOWRT, _IORW);

	if (f->_flag & _IONBF) {
		cnt = write(fileno(f), buf, len);
		if (cnt < 0) {
			f->_flag |= _IOERR;
			if (!(my_flag(f) & _JS_IONORAISE))
				raisecond(_writeerr, 0L);
		}
		return (cnt);
	}
	cnt = 0;
	while (len > 0) {
		if (f->_cnt <= 0) {
			if (usg_flsbuf((unsigned char) *buf++, f) == EOF)
				break;
			cnt++;
			if (--len == 0)
				break;
		}
		if ((n = f->_cnt >= len ? len : f->_cnt) > 0) {
			f->_ptr = (unsigned char *)movebytes(buf, f->_ptr, n);
			buf += n;
			f->_cnt -= n;
			cnt += n;
			len -= n;
		}
	}
	if (!ferror(f))
		return (cnt);
	if (!(my_flag(f) & _JS_IONORAISE))
	raisecond(_writeerr, 0L);
	return (-1);
}

#else

EXPORT int
filewrite(f, vbuf, len)
	register FILE	*f;
	void	*vbuf;
	int	len;
{
	int	cnt;
	char		*buf = vbuf;

	down2(f, _IOWRT, _IORW);

	if (my_flag(f) & _JS_IOUNBUF)
		return (write(fileno(f), buf, len));
	cnt = fwrite(buf, 1, len, f);

	if (!ferror(f))
		return (cnt);
	if (!(my_flag(f) & _JS_IONORAISE))
	raisecond(_writeerr, 0L);
	return (-1);
}

#endif	/* HAVE_USG_STDIO */
