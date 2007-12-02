/* @(#)message.c	1.25 06/09/13 Copyright 1984-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)message.c	1.25 06/09/13 Copyright 1984-2004 J. Schilling";
#endif
/*
 *	Management routines for the system (status) line of the editor
 *	displayed at the top of the screen.
 *
 *	Copyright (c) 1984-2004 J. Schilling
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

/*
 * Layout of the systemline:
 *
 * |.123456789.123456789.123456789.123456789.123456789.123456789.123456789...
 * |info               |num      |takebuf  |filename           |errortext
 * |1/4 llen           |1/8 llen |1/8 llen |1/4 llen           |1/4 llen
 *
 * -	info	informational messages (search pattern, modflag ...)
 * -	num	actual multiplying factor for the next command
 * -	take	the name of the current take buffer
 * -	name	the name of the file currently beeing edited
 * -	error	error messages
 */

#include "ved.h"
#include "terminal.h"
#include <schily/varargs.h>

/*
 * INFOSIZE is 1/4th of the linelength of the terminal/window.
 * If the screen has 1280 pixel and the width of a character is 8 pixel,
 * a maximum of 160 charcters/line is sufficient.
 * If the screen has 1280 pixel and the width of a character is 6 pixel,
 * a maximum of 213 charcters/line is sufficient.
 * A maximum linelength of 256 should be sufficient in any case.
 *
 * INFOSIZE should be at least 20+2 (see cmdline.c)
 * We use ( 256 / 4 ) + 1. This should be enough for some time.
 *
 * The size of the info (left most) string should be big enough to print
 * out a string of NAMESIZE length. The maximum length is 4 * INFOSIZE.
 */
#define	INFOSIZE	65			/* Allow screen width of 259 */
#define	SYSLMIN		NAMESIZE
#define	SYSLSIZE	(SYSLMIN > (4*INFOSIZE) ? (4*INFOSIZE) : SYSLMIN)
#define	NUMSIZE		(INFOSIZE/2)
#define	TAKESIZE	(INFOSIZE/2)

LOCAL	Uchar	infostr[SYSLSIZE];		/* left most part of message */
LOCAL	Uchar	namestr[2*INFOSIZE];		/* center part, name of file */
LOCAL	Uchar	errorstr[INFOSIZE];		/* right part, error messages*/
LOCAL	Uchar	numstr[NUMSIZE];		/* display of multiple	    */
LOCAL	Uchar	takestr[TAKESIZE];		/* display of take buffer   */
LOCAL	Uchar	DefInfostr[INFOSIZE];		/* default info string	    */
LOCAL	Uchar	DefErrorstr[INFOSIZE-1];	/* default error string	    */
						/* may be prepended by a ' ' */
						/* to create error message  */

EXPORT	BOOL	updatemsg = 0;			/* infostr is temporary */
EXPORT	BOOL	updateerr = 0;			/* errorstr is temporary */
EXPORT	BOOL	updatesysline = 0;		/* whole systemline should be redrawn */
LOCAL	int	lastmsglen;			/* the len of the last message */

LOCAL	int	numplace = 0;		/* where the numstr starts */
LOCAL	int	takeplace = 0;		/* where the takestr starts */
LOCAL	int	nameplace = 0;		/* where the namestr starts */
LOCAL	int	errorplace = 0;		/* where the errorstr starts */
LOCAL	ecnt_t	maxmsgnum = (ecnt_t)1;	/* how big a displayed # can be */

#define	infosize (numplace - 0)		/* actual length of the infostring */
#define	numsize	 (takeplace-numplace)	/* actual length of the numstring */
#define	takesize (nameplace-takeplace)	/* actual length of the takestring */
#define	namesize (errorplace-nameplace)	/* actual length of the namestring */
#define	errorsize (wp->llen-errorplace)	/* actual length of the errorstring */
#define	nameerrsize (wp->llen-nameplace) /* actual length of the name+errorstring */

EXPORT	void	initmessage	__PR((ewin_t *wp));
EXPORT	void	initmsgsize	__PR((ewin_t *wp));
EXPORT	void	writemsg	__PR((ewin_t *wp, char *str, ...));
EXPORT	void	writenum	__PR((ewin_t *wp, ecnt_t num));
EXPORT	void	writetake	__PR((ewin_t *wp, Uchar *str));
EXPORT	void	namemsg		__PR((Uchar* name));
EXPORT	void	writeerr	__PR((ewin_t *wp, char *str, ...));
EXPORT	void	writeserr	__PR((ewin_t *wp, char *str, ...));
LOCAL	void	_writeerr	__PR((ewin_t *wp, BOOL dobeep, char *str, va_list args));
EXPORT	void	write_errno	__PR((ewin_t *wp, char *msg, ...));
EXPORT	void	defaultinfo	__PR((ewin_t *wp, Uchar *str));
EXPORT	void	defaulterr	__PR((ewin_t *wp, Uchar *str));
LOCAL	void	errdefault	__PR((void));
EXPORT	void	refreshmsg	__PR((ewin_t *wp));
EXPORT	void	refreshsysline	__PR((ewin_t *wp));
EXPORT	void	write_systemline __PR((ewin_t *wp));
EXPORT	void	abortmsg	__PR((ewin_t *wp));
EXPORT	void	nomarkmsg	__PR((ewin_t *wp));


/*---------------------------------------------------------------------------
|
| Init the system (status) message line.
|
| Do not print it yet, this will be done later.
|
+---------------------------------------------------------------------------*/

EXPORT void
initmessage(wp)
	ewin_t	*wp;
{
	infostr[0] = '\0';
	namestr[0] = '\0';
	errorstr[0] = '\0';
	numstr[0] = '\0';
	takestr[0] = '\0';
	DefInfostr[0] = '\0';
	DefErrorstr[0] = '\0';

	initmsgsize(wp);
}


/*---------------------------------------------------------------------------
|
| Set up the actual sizes for the various strings (will be called on resize)
|
+---------------------------------------------------------------------------*/

EXPORT void
initmsgsize(wp)
	ewin_t	*wp;
{
	register int	maxnumplaces;
	register int	i;
	register ecnt_t	n;		/* same type as curnum */
	register ecnt_t	on;		/* same type as curnum */

	numplace	= (2*wp->llen)/8;
	takeplace	= (3*wp->llen)/8;
	nameplace	= (4*wp->llen)/8;
	errorplace	= (6*wp->llen)/8;

	/*
	 * Overflow bei Berechnung von maxmsgnum verhindern
	 */
	for (on = (ecnt_t)0, n = (ecnt_t)1, maxnumplaces = 0;
			n > 0 && n > on && maxnumplaces < NUMSIZE-1;
							maxnumplaces++) {
		on = n;
		n  = 10*n;
	}
	maxnumplaces--;

	i = takeplace - numplace - 3;
	if (i < 1)
		i = 1;
	if (i < maxnumplaces)
		maxnumplaces = i;


	maxmsgnum = (ecnt_t)1;
	while (--maxnumplaces >= 0)
		maxmsgnum = 10*maxmsgnum;
}


/*---------------------------------------------------------------------------
|
| Write a message to the (left) info part of the system (status) line.
|
| If called: writemsg(wp, NULL),	display content of default info string
| If called: writemsg(wp, "text"),	display "text" up to the next command
|
+---------------------------------------------------------------------------*/

/* PRINTFLIKE2 */
#ifdef	PROTOTYPES
EXPORT void
writemsg(ewin_t *wp, char *str, ...)
#else
EXPORT void
writemsg(wp, str, va_alist)
	ewin_t	*wp;
	char	*str;
	va_dcl
#endif
{
	va_list	args;
	int	len = 0;

	if (str == 0) {
		strncpy(C infostr, C DefInfostr, sizeof (infostr));
		infostr[sizeof (infostr)-1] = '\0';
		updatemsg = 0;
	} else if (*str == '\0') {
		infostr[0] = '\0';
		updatemsg = 1;
	} else {
#ifdef	PROTOTYPES
		va_start(args, str);
#else
		va_start(args);
#endif
		len = snprintf(C infostr, sizeof (infostr), "%r", str, args);
		va_end(args);
		updatemsg = 1;
	}
	CURSOR_HOME(wp);
	if (len <= infosize) {
		len = infosize;
		if (updatesysline && len < lastmsglen)
			len = lastmsglen;
	} else {
		updatesysline = 1;
	}

	/*
	 * We don't like the extended message to end in the middle of some text
	 * from another field. If we touch the next field, clear it temporary.
	 */
	if (len > infosize && len <= (infosize+numsize))
		len = (infosize+numsize);
	if (len > (infosize+numsize) && len <= (infosize+numsize+takesize))
		len = (infosize+numsize+takesize);
	if (len > (infosize+numsize+takesize) &&
				len <= (infosize+numsize+takesize+namesize))
		len = (infosize+numsize+takesize+namesize);
	lastmsglen = len;

	printfield(infostr, len);
	setcursor(wp);
	if (str != 0)
		flush();
}


/*---------------------------------------------------------------------------
|
| Write a number to the (left center) info part of the system (status) line.
|
| If called: writenum(wp, -1),	erase content of the number string
| If called: writenum(wp, >=0),	display number
|
+---------------------------------------------------------------------------*/

EXPORT void
writenum(wp, num)
	ewin_t	*wp;
	ecnt_t	num;
{
	if (num < (ecnt_t)0)
		numstr[0] = '\0';
	else if (num < maxmsgnum)
		snprintf(C numstr, sizeof (numstr), " #:%lld", (Llong)num);
	else
		strcpy(C numstr, " #:BIG");
	MOVE_CURSOR_ABS(wp, 0, numplace);
	printfield(numstr, numsize);
	setcursor(wp);
}


/*---------------------------------------------------------------------------
|
| Write the take buffer name to the center part of the system (status) line.
|
+---------------------------------------------------------------------------*/

EXPORT void
writetake(wp, str)
	ewin_t	*wp;
	Uchar	*str;
{
	snprintf(C takestr, sizeof (takestr), " \\:%s", str);
	MOVE_CURSOR_ABS(wp, 0, takeplace);
	printfield(takestr, takesize);
	setcursor(wp);
}


/*---------------------------------------------------------------------------
|
| Set up the file name in the (right center) part of the system (status) line.
|
+---------------------------------------------------------------------------*/

EXPORT void
namemsg(name)
	Uchar	*name;
{
	snprintf(C namestr, sizeof (namestr), " %s", C name);
}

/*---------------------------------------------------------------------------
|
| Write a message to the (right) error part of the system (status) line.
|
| If called: writeerr(wp, NULL),	display content of default error string
| If called: writeerr(wp, "text"),	display error "text" up to the next
|					command
|
| Ring the bell
|
+---------------------------------------------------------------------------*/

/* PRINTFLIKE2 */
#ifdef	PROTOTYPES
EXPORT void
writeerr(ewin_t *wp, char *str, ...)
#else
EXPORT void
writeerr(wp, str, va_alist)
	ewin_t	*wp;
	char	*str;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, str);
#else
	va_start(args);
#endif
	_writeerr(wp, TRUE, str, args);
	va_end(args);
}

/*---------------------------------------------------------------------------
|
| Write a message to the (right) error part of the system (status) line.
|
| If called: writeserr(wp, NULL),	display content of default error string
| If called: writeserr(wp, "text"),	display error "text" up to the next
|					command
|
| Don't ring the bell (be silent)
|
+---------------------------------------------------------------------------*/

/* PRINTFLIKE2 */
#ifdef	PROTOTYPES
EXPORT void
writeserr(ewin_t *wp, char *str, ...)
#else
EXPORT void
writeserr(wp, str, va_alist)
	ewin_t	*wp;
	char	*str;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, str);
#else
	va_start(args);
#endif
	_writeerr(wp, FALSE, str, args);
	va_end(args);
}

/*---------------------------------------------------------------------------
|
| Write a message to the (right) error part of the system (status) line.
|
| If called: _writeerr(wp, NULL),	display content of default error string
| If called: _writeerr(wp, "text"),	display error "text" up to the next
|					command
|
| Whether to ring the bell depends on 'dobeep'.
|
+---------------------------------------------------------------------------*/

LOCAL void
_writeerr(wp, dobeep, str, args)
	ewin_t	*wp;
	BOOL	dobeep;
	char	*str;
	va_list	args;
{
	int	len = 0;

	if (str == 0) {
		errdefault();
		updateerr = 0;
	} else {
		if (*str == '\0') {
			errorstr[0] = '\0';
			updateerr = 1;
		} else {
			len = snprintf(C errorstr, sizeof (errorstr), " %r",
								str, args);
			if (dobeep)
				ringbell();
			updateerr = 1;
		}
	}
	if (*errorstr) {
		if (len < 0 || len > errorsize) {
			MOVE_CURSOR_ABS(wp, 0, nameplace);
			printfield(errorstr, nameerrsize);
		} else {
			MOVE_CURSOR_ABS(wp, 0, errorplace);
			printfield(errorstr, errorsize);
		}
	} else {		/* Let name field use rest of screen if no error */
		MOVE_CURSOR_ABS(wp, 0, nameplace);
		printfield(namestr, nameerrsize);
	}
	setcursor(wp);
	if (str != 0)
		flush();
}


/*---------------------------------------------------------------------------
|
| Write a message to the (right) error part of the system (status) line.
| The message contains the error text or the error number for the last
| failed syscall depending on the space.
| If the message including the error text will fit the error number
| is ommitted. If the text will overflow the space, the error number will be
| placed in front of the text because it will possibly be readeable.
|
+---------------------------------------------------------------------------*/

/* PRINTFLIKE2 */
#ifdef	PROTOTYPES
EXPORT void
write_errno(ewin_t *wp, char *msg, ...)
#else
EXPORT void
write_errno(wp, msg, va_alist)
	ewin_t	*wp;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	char	buf[100];
	int	len;
	int	err = geterrno();
	char	*errstr;

	errstr = errmsgstr(err);
	if (errstr == NULL)
		errstr = "";

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	len = snprintf(buf, sizeof (buf), "%r: %s", msg, args, errstr);
	va_end(args);

	if (len <= nameerrsize) {
		writeerr(wp, buf);
		return;
	}

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	snprintf(buf, sizeof (buf), "(%d) %r: %s", err, msg, args, errstr);
	va_end(args);
	writeerr(wp, buf);
}


/*---------------------------------------------------------------------------
|
| Set up the default info string (currently only used for modflag "*")
| It is displayed after the command that follows a command that sets
| the info string until a new info string is set.
|
+---------------------------------------------------------------------------*/

EXPORT void
defaultinfo(wp, str)
	ewin_t	*wp;
	Uchar	*str;
{
	if (str == NULL)
		DefInfostr[0] = '\0';
	else
		snprintf(C DefInfostr, sizeof (DefInfostr), "%s", str);
	writemsg(wp, C DefInfostr);

}


/*---------------------------------------------------------------------------
|
| Set up the default error string (currently only used for "NEW FILE")
| It is displayed after the command that follows a command that sets
| the error string until a new error string is set.
|
+---------------------------------------------------------------------------*/

EXPORT void
defaulterr(wp, str)
	ewin_t	*wp;
	Uchar	*str;
{
	if (str == 0) {
		DefErrorstr[0] = '\0';
		writeerr(wp, "");
	} else {
		snprintf(C DefErrorstr, sizeof (DefErrorstr), "%s", str);
		writeerr(wp, C DefErrorstr);
	}
}


/*---------------------------------------------------------------------------
|
| Set the error string to the default error string.
| If the default error string contains a message, prepend a blank as a
| separator to the file name.
|
+---------------------------------------------------------------------------*/

LOCAL void
errdefault()
{
	if (*DefErrorstr == '\0') {
		errorstr[0] = '\0';
	} else {
		/*
		 * Permanent error messages should be separated from
		 * the filename.
		 */
		errorstr[0] = ' ';
		strcpy(C &errorstr[1], C DefErrorstr); /* no overflow (same size)*/

	}
}


/*---------------------------------------------------------------------------
|
| Refresh the whole system (status) line e.g when parts get overwritten.
|
+---------------------------------------------------------------------------*/

EXPORT void
refreshmsg(wp)
	ewin_t	*wp;
{
	updatesysline = 0;
	CURSOR_HOME(wp);
	write_systemline(wp);
	setcursor(wp);
}

/*---------------------------------------------------------------------------
|
| Refresh the whole system (status) line or parts of it depending on status.
|
+---------------------------------------------------------------------------*/

EXPORT void
refreshsysline(wp)
	ewin_t	*wp;
{
	if (updatesysline) {
		refreshmsg(wp);
	} else {
		if (updateerr)
			writeerr(wp, C NULL);
		if (updatemsg)
			writemsg(wp, C NULL);
	}
}

/*---------------------------------------------------------------------------
|
| Rewrite the whole system (status) line e.g when parts get overwritten.
| Reset info string & error string to the default if necessary.
| Allow the file name to use up the space of the error string
| if the error string is blank.
|
+---------------------------------------------------------------------------*/

EXPORT void
write_systemline(wp)
	ewin_t	*wp;
{
	if (updatemsg) {
		strncpy(C infostr, C DefInfostr, sizeof (infostr));
		infostr[sizeof (infostr)-1] = '\0';
		updatemsg = 0;
	}
	if (updateerr) {
		errdefault();
		updateerr = 0;
	}
	printfield(infostr, infosize);
	printfield(numstr, numsize);
	printfield(takestr, takesize);

	if (*errorstr) {
		printfield(namestr, namesize);
		printfield(errorstr, errorsize);
	} else {
		printfield(namestr, nameerrsize);
	}
}

EXPORT void
abortmsg(wp)
	ewin_t	*wp;
{
	writeerr(wp, "ABORTED");
}

EXPORT void
nomarkmsg(wp)
	ewin_t	*wp;
{
	writeerr(wp, "No Mark !");
}
