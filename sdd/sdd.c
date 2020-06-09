/* @(#)sdd.c	1.69 20/05/30 Copyright 1984-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)sdd.c	1.69 20/05/30 Copyright 1984-2020 J. Schilling";
#endif
/*
 *	sdd	Disk and Tape copy
 *
 *	Large File remarks:
 *
 *	Even platforms without large file support don't limit the
 *	amount of data that may be read/written to io-streams
 *	(e.g. stdin/stdout) and tapes. It makes no sense to use
 *	off_t for several data types. As more platforms support long long
 *	than large files, we use long long for several important
 *	variables that should not overflow.
 *
 *	Copyright (c) 1984-2020 J. Schilling
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

#include <schily/mconfig.h>

/*
 * XXX Until we find a better way, the next definitions must be in sync
 * XXX with the definitions in librmt/remote.c
 */
#if !defined(HAVE_FORK) || !defined(HAVE_SOCKETPAIR) || !defined(HAVE_DUP2)
#undef	USE_RCMD_RSH
#endif
#if !defined(HAVE_GETSERVBYNAME)
#undef	USE_REMOTE				/* Cannot get rcmd() port # */
#endif
#if (!defined(HAVE_NETDB_H) || !defined(HAVE_RCMD)) && !defined(USE_RCMD_RSH)
#undef	USE_REMOTE				/* There is no rcmd() */
#endif

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/signal.h>
#include <schily/utypes.h>
#include <schily/time.h>
#include <schily/standard.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/librmt.h>
#include <schily/errno.h>
#include <schily/md5.h>

#ifdef	SIGRELSE
#	define	signal	sigset	/* reliable signal */
#endif

EXPORT	int	main		__PR((int ac, char **av));
LOCAL	void	set_signal	__PR((int sig, RETSIGTYPE (*handler)(int)));
LOCAL	void	intr		__PR((int sig));
LOCAL	FILE 	*openfile	__PR((char *name, char *mode));
LOCAL	char	*memalloc	__PR((long size));
LOCAL	void	simple_copy	__PR((void));
LOCAL	void	copy_reblocked	__PR((void));
LOCAL	long	readvol		__PR((char *buf, long len));
LOCAL	long	writevol	__PR((char *buf, long len));
LOCAL	BOOL	next_in		__PR((void));
LOCAL	BOOL	next_out	__PR((void));
LOCAL	BOOL	next		__PR((char *fname, char *inout, int fd, long pos, int volnum));
LOCAL	BOOL	cont		__PR((char *inout, int num));
LOCAL	void	makelower	__PR((char *s));
LOCAL	long	readbuf		__PR((char *buf, long len));
LOCAL	long	writebuf	__PR((char *buf, long len));
LOCAL	long	readblocks	__PR((char *buf, long len));
LOCAL	long	writeblocks	__PR((char *buf, long len));
LOCAL	void	fill		__PR((char *bp, long start, long end));
LOCAL	void	swabb		__PR((char *bp, long cnt));
LOCAL	void	lcase		__PR((char *bp, long cnt));
LOCAL	void	ucase		__PR((char *bp, long cnt));
LOCAL	long	block		__PR((char *bp, long cnt));
LOCAL	long	unblock		__PR((char *bp, long cnt));
LOCAL	void	conv		__PR((char *bp, long cnt, unsigned char *tab));
LOCAL	void	term		__PR((int ret));
LOCAL	void	getstarttime	__PR((void));
LOCAL	void	prstats		__PR((void));
LOCAL	void	getopts		__PR((int ac, char **av));
LOCAL	void	usage		__PR((int ex));
LOCAL	int	openremote	__PR((char *filename, long iosize));
LOCAL	int	ropenfile	__PR((int rfd, char *name, int mode));
LOCAL	ssize_t	rread		__PR((void *buf, size_t cnt));
LOCAL	ssize_t	rwrite		__PR((void *buf, size_t cnt));
LOCAL	off_t	riseek		__PR((off_t pos));
LOCAL	off_t	roseek		__PR((off_t pos));
LOCAL	off_t	ifsize		__PR((void));

LOCAL	void	mdinit		__PR((void));
LOCAL	void	mdupdate	__PR((void *a, size_t s));
LOCAL	void	mdfinal		__PR((void));

#undef	min
#define	min(a, b)	((a) < (b) ? (a) : (b))

#define	SDD_BSIZE	512L

#define	FILL		000001
#define	SWAB		000002
#define	LCASE		000004
#define	UCASE		000010
#define	BLOCK		000020
#define	UNBLOCK		000040
#define	ASCII		000100
#define	EBCDIC		000200
#define	IBM		000400
#define	NULLIN		010000
#define	NULLOUT		020000
#define	MD5SUM		040000

LOCAL	char	*infile;
LOCAL	char	*outfile;
LOCAL	char	*rmtin;
LOCAL	char	*rmtout;

LOCAL	FILE	*tty;
LOCAL	FILE	*fin;
LOCAL	FILE	*fout;
LOCAL	int	ifd;
LOCAL	int	ofd;
LOCAL	int	rmtifd = -1;
LOCAL	int	rmtofd = -1;

LOCAL	long	ibs;
LOCAL	long	obs;
LOCAL	long	bs;
LOCAL	long	cbs;
LOCAL	long	sdd_bsize = SDD_BSIZE;	/* Sector size */
LOCAL	Llong	count;
LOCAL	off_t	iseek;
LOCAL	off_t	oseek;
LOCAL	off_t	seek;
LOCAL	Llong	iskip;	/* May skip on stdin so it may be more than off_t */
LOCAL	Llong	oskip;	/* May skip on stdin so it may be more than off_t */
LOCAL	Llong	skip;	/* May skip on stdin so it may be more than off_t */
LOCAL	off_t	ivseek;
LOCAL	off_t	ovseek;
LOCAL	Llong	ivsize;	/* If volume is a tape, it may be more than off_t */
LOCAL	Llong	ovsize;	/* If volume is a tape, it may be more than off_t */
LOCAL	int	try	= 2;

LOCAL	Llong	irec;
LOCAL	Llong	orec;
LOCAL	Llong	iparts;
LOCAL	Llong	oparts;
LOCAL	off_t	ivpos;
LOCAL	off_t	ovpos;
LOCAL	int	ivolnum = 1;
LOCAL	int	ovolnum = 1;
LOCAL	Llong	readerrs;
LOCAL	Llong	writeerrs;
LOCAL	Llong	lastreaderrs;
#ifdef	timerclear
LOCAL	struct	timeval	starttime;
LOCAL	struct	timeval	stoptime;
#endif

LOCAL	int	flags;
LOCAL	BOOL	notrunc;
LOCAL	BOOL	progress;
LOCAL	BOOL	noerror;
LOCAL	BOOL	noerrwrite;
LOCAL	BOOL	noseek;
LOCAL	BOOL	debug;
LOCAL	BOOL	showtime;

/* ebcdic to ascii */
LOCAL	unsigned char	asctab[] = {
/*  0 */	0x00,	0x01,	0x02,	0x03,	0x9C,	0x09,	0x86,	0x7F,
/*  8 */	0x97,	0x8D,	0x8E,	0x0B,	0x0C,	0x0D,	0x0E,	0x0F,
/* 10 */	0x10,	0x11,	0x12,	0x13,	0x9D,	0x85,	0x08,	0x87,
/* 18 */	0x18,	0x19,	0x92,	0x8F,	0x1C,	0x1D,	0x1E,	0x1F,
/* 20 */	0x80,	0x81,	0x82,	0x83,	0x84,	0x0A,	0x17,	0x1B,
/* 28 */	0x88,	0x89,	0x8A,	0x8B,	0x8C,	0x05,	0x06,	0x07,
/* 30 */	0x90,	0x91,	0x16,	0x93,	0x94,	0x95,	0x96,	0x04,
/* 38 */	0x98,	0x99,	0x9A,	0x9B,	0x14,	0x15,	0x9E,	0x1A,
/* 40 */	0x20,	0xA0,	0xA1,	0xA2,	0xA3,	0xA4,	0xA5,	0xA6,
/* 48 */	0xA7,	0xA8,	0xD5,	0x2E,	0x3C,	0x28,	0x2B,	0x7C,
/* 50 */	0x26,	0xA9,	0xAA,	0xAB,	0xAC,	0xAD,	0xAE,	0xAF,
/* 58 */	0xB0,	0xB1,	0x21,	0x24,	0x2A,	0x29,	0x3B,	0x7E,
/* 60 */	0x2D,	0x2F,	0xB2,	0xB3,	0xB4,	0xB5,	0xB6,	0xB7,
/* 68 */	0xB8,	0xB9,	0xCB,	0x2C,	0x25,	0x5F,	0x3E,	0x3F,
/* 70 */	0xBA,	0xBB,	0xBC,	0xBD,	0xBE,	0xBF,	0xC0,	0xC1,
/* 78 */	0xC2,	0x60,	0x3A,	0x23,	0x40,	0x27,	0x3D,	0x22,
/* 80 */	0xC3,	0x61,	0x62,	0x63,	0x64,	0x65,	0x66,	0x67,
/* 88 */	0x68,	0x69,	0xC4,	0xC5,	0xC6,	0xC7,	0xC8,	0xC9,
/* 90 */	0xCA,	0x6A,	0x6B,	0x6C,	0x6D,	0x6E,	0x6F,	0x70,
/* 98 */	0x71,	0x72,	0x5E,	0xCC,	0xCD,	0xCE,	0xCF,	0xD0,
/* A0 */	0xD1,	0xE5,	0x73,	0x74,	0x75,	0x76,	0x77,	0x78,
/* A8 */	0x79,	0x7A,	0xD2,	0xD3,	0xD4,	0x5B,	0xD6,	0xD7,
/* B0 */	0xD8,	0xD9,	0xDA,	0xDB,	0xDC,	0xDD,	0xDE,	0xDF,
/* B8 */	0xE0,	0xE1,	0xE2,	0xE3,	0xE4,	0x5D,	0xE6,	0xE7,
/* C0 */	0x7B,	0x41,	0x42,	0x43,	0x44,	0x45,	0x46,	0x47,
/* C8 */	0x48,	0x49,	0xE8,	0xE9,	0xEA,	0xEB,	0xEC,	0xED,
/* D0 */	0x7D,	0x4A,	0x4B,	0x4C,	0x4D,	0x4E,	0x4F,	0x50,
/* D8 */	0x51,	0x52,	0xEE,	0xEF,	0xF0,	0xF1,	0xF2,	0xF3,
/* E0 */	0x5C,	0x9F,	0x53,	0x54,	0x55,	0x56,	0x57,	0x58,
/* E8 */	0x59,	0x5A,	0xF4,	0xF5,	0xF6,	0xF7,	0xF8,	0xF9,
/* F0 */	0x30,	0x31,	0x32,	0x33,	0x34,	0x35,	0x36,	0x37,
/* F8 */	0x38,	0x39,	0xFA,	0xFB,	0xFC,	0xFD,	0xFE,	0xFF,
};

/* ascii to ebcdic */

LOCAL	unsigned char	ebctab[] = {

/*	0	1	2	3	4	5	6	7	*/
/*  0	NUL(@)	SOH(A)	STX(B)	ETX(C)	EOT(D)	ENQ(E)	ACK(F)	BEL(G)	*/
	0x00,	0x01,	0x02,	0x03,	0x37,	0x2D,	0x2E,	0x2F,

/*  8	BS(H)	HT(I)	LF(J)	VT(K)	FF(L)	CR(M)	SO(N)	SI(O)	*/
	0x16,	0x05,	0x25,	0x0B,	0x0C,	0x0D,	0x0E,	0x0F,

/* 10	DLE(P)	DC1(Q)	DC2(R)	DC3(S)	DC4(T)	NAK(U)	SYN(V)	ETB(W)	*/
	0x10,	0x11,	0x12,	0x13,	0x3C,	0x3D,	0x32,	0x26,

/* 18	CAN(X)	EM(Y)	SUB(Z)	ESC([)	FS(\)	GS(])	RS(^)	US(_)	*/
	0x18,	0x19,	0x3F,	0x27,	0x1C,	0x1D,	0x1E,	0x1F,

/* 20	SP	!	"	#	$	%	&	'	*/
	0x40,	0x5A,	0x7F,	0x7B,	0x5B,	0x6C,	0x50,	0x7D,

/* 28	(	)	*	+	,	-	.	/	*/
	0x4D,	0x5D,	0x5C,	0x4E,	0x6B,	0x60,	0x4B,	0x61,

/* 30	0	1	2	3	4	5	6	7	*/
	0xF0,	0xF1,	0xF2,	0xF3,	0xF4,	0xF5,	0xF6,	0xF7,

/* 38	8	9	:	;	<	=	>	?	*/
	0xF8,	0xF9,	0x7A,	0x5E,	0x4C,	0x7E,	0x6E,	0x6F,

/* 40	@	A	B	C	D	E	F	G	*/
	0x7C,	0xC1,	0xC2,	0xC3,	0xC4,	0xC5,	0xC6,	0xC7,

/* 48	H	I	J	K	L	M	N	O	*/
	0xC8,	0xC9,	0xD1,	0xD2,	0xD3,	0xD4,	0xD5,	0xD6,

/* 50	P	Q	R	S	T	U	V	W	*/
	0xD7,	0xD8,	0xD9,	0xE2,	0xE3,	0xE4,	0xE5,	0xE6,

/* 58	X	Y	Z	[	\	]	^ ??	_	*/
	0xE7,	0xE8,	0xE9,	0xAD,	0xE0,	0xBD,	0x9A,	0x6D,

/* 60	`	a	b	c	d	e	f	g	*/
	0x79,	0x81,	0x82,	0x83,	0x84,	0x85,	0x86,	0x87,

/* 68	h	i	j	k	l	m	n	o	*/
	0x88,	0x89,	0x91,	0x92,	0x93,	0x94,	0x95,	0x96,

/* 70	p	q	r	s	t	u	v	w	*/
	0x97,	0x98,	0x99,	0xA2,	0xA3,	0xA4,	0xA5,	0xA6,

/* 78	x	y	z	{	|	}	~ ??	DEL	*/
	0xA7,	0xA8,	0xA9,	0xC0,	0x4F,	0xD0,	0x5F,	0x07,

/* 80	NUL(@)	SOH(A)	STX(B)	ETX(C)	EOT(D)	ENQ(E)	ACK(F)	BEL(G)	*/
	0xC3,	0x61,	0x62,	0x63,	0x64,	0x65,	0x66,	0x67,

/* 88	BS(H)	HT(I)	LF(J)	VT(K)	FF(L)	CR(M)	SO(N)	SI(O)	*/
	0x68,	0x69,	0xC4,	0xC5,	0xC6,	0xC7,	0xC8,	0xC9,

/* 90	DLE(P)	DC1(Q)	DC2(R)	DC3(S)	DC4(T)	NAK(U)	SYN(V)	ETB(W)	*/
	0xCA,	0x6A,	0x6B,	0x6C,	0x6D,	0x6E,	0x6F,	0x70,

/* 98	CAN(X)	EM(Y)	SUB(Z)	ESC([)	FS(\)	GS(])	RS(^)	US(_)	*/
	0x71,	0x72,	0x5E,	0xCC,	0xCD,	0xCE,	0xCF,	0xD0,

/* A0	SP	!	"	#	$	%	&	'	*/
	0xD1,	0xE5,	0x73,	0x74,	0x75,	0x76,	0x77,	0x78,

/* A8	(	)	*	+	,	-	.	/	*/
	0x79,	0x7A,	0xD2,	0xD3,	0xD4,	0x5B,	0xD6,	0xD7,

/* B0	0	1	2	3	4	5	6	7	*/
	0xD8,	0xD9,	0xDA,	0xDB,	0xDC,	0xDD,	0xDE,	0xDF,

/* B8	8	9	:	;	<	=	>	?	*/
	0xE0,	0xE1,	0xE2,	0xE3,	0xE4,	0x5D,	0xE6,	0xE7,

/* C0	@	A	B	C	D	E	F	G	*/
	0x7B,	0x41,	0x42,	0x43,	0x44,	0x45,	0x46,	0x47,

/* C8	H	I	J	K	L	M	N	O	*/
	0x48,	0x49,	0xE8,	0xE9,	0xEA,	0xEB,	0xEC,	0xED,

/* D0	P	Q	R	S	T	U	V	W	*/
	0x7D,	0x4A,	0x4B,	0x4C,	0x4D,	0x4E,	0x4F,	0x50,

/* D8	X	Y	Z	[	\	]	^	_	*/
	0x51,	0x52,	0xEE,	0xEF,	0xF0,	0xF1,	0xF2,	0xF3,

/* E0	`	a	b	c	d	e	f	g	*/
	0x5C,	0x9F,	0x53,	0x54,	0x55,	0x56,	0x57,	0x58,

/* E8	h	i	j	k	l	m	n	o	*/
	0x59,	0x5A,	0xF4,	0xF5,	0xF6,	0xF7,	0xF8,	0xF9,

/* F0	p	q	r	s	t	u	v	w	*/
	0x30,	0x31,	0x32,	0x33,	0x34,	0x35,	0x36,	0x37,

/* F8	x	y	z	{	|	}	~	DEL	*/
	0x38,	0x39,	0xFA,	0xFB,	0xFC,	0xFD,	0xFE,	0xFF,
};

/* ascii to ibm */
LOCAL	unsigned char	ibmtab[] = {

/*	0	1	2	3	4	5	6	7	*/
/*  0	NUL(@)	SOH(A)	STX(B)	ETX(C)	EOT(D)	ENQ(E)	ACK(F)	BEL(G)	*/
	0x00,	0x01,	0x02,	0x03,	0x37,	0x2D,	0x2E,	0x2F,

/*  8	BS(H)	HT(I)	LF(J)	VT(K)	FF(L)	CR(M)	SO(N)	SI(O)	*/
	0x16,	0x05,	0x25,	0x0B,	0x0C,	0x0D,	0x0E,	0x0F,

/* 10	DLE(P)	DC1(Q)	DC2(R)	DC3(S)	DC4(T)	NAK(U)	SYN(V)	ETB(W)	*/
	0x10,	0x11,	0x12,	0x13,	0x3C,	0x3D,	0x32,	0x26,

/* 18	CAN(X)	EM(Y)	SUB(Z)	ESC([)	FS(\)	GS(])	RS(^)	US(_)	*/
	0x18,	0x19,	0x3F,	0x27,	0x1C,	0x1D,	0x1E,	0x1F,

/* 20	SP	!	"	#	$	%	&	'	*/
	0x40,	0x5A,	0x7F,	0x7B,	0x5B,	0x6C,	0x50,	0x7D,

/* 28	(	)	*	+	,	-	.	/	*/
	0x4D,	0x5D,	0x5C,	0x4E,	0x6B,	0x60,	0x4B,	0x61,

/* 30	0	1	2	3	4	5	6	7	*/
	0xF0,	0xF1,	0xF2,	0xF3,	0xF4,	0xF5,	0xF6,	0xF7,

/* 38	8	9	:	;	<	=	>	?	*/
	0xF8,	0xF9,	0x7A,	0x5E,	0x4C,	0x7E,	0x6E,	0x6F,

/* 40	@	A	B	C	D	E	F	G	*/
	0x7C,	0xC1,	0xC2,	0xC3,	0xC4,	0xC5,	0xC6,	0xC7,

/* 48	H	I	J	K	L	M	N	O	*/
	0xC8,	0xC9,	0xD1,	0xD2,	0xD3,	0xD4,	0xD5,	0xD6,

/* 50	P	Q	R	S	T	U	V	W	*/
	0xD7,	0xD8,	0xD9,	0xE2,	0xE3,	0xE4,	0xE5,	0xE6,

/* 58	X	Y	Z	[	\	]	^	_	*/
	0xE7,	0xE8,	0xE9,	0xAD,	0xE0,	0xBD,	0x5F,	0x6D,

/* 60	`	a	b	c	d	e	f	g	*/
	0x79,	0x81,	0x82,	0x83,	0x84,	0x85,	0x86,	0x87,

/* 68	h	i	j	k	l	m	n	o	*/
	0x88,	0x89,	0x91,	0x92,	0x93,	0x94,	0x95,	0x96,

/* 70	p	q	r	s	t	u	v	w	*/
	0x97,	0x98,	0x99,	0xA2,	0xA3,	0xA4,	0xA5,	0xA6,

/* 78	x	y	z	{	|	}	~	DEL	*/
	0xA7,	0xA8,	0xA9,	0xC0,	0x4F,	0xD0,	0xA1,	0x07,

/* 80	NUL(@)	SOH(A)	STX(B)	ETX(C)	EOT(D)	ENQ(E)	ACK(F)	BEL(G)	*/
	0x20,	0x21,	0x22,	0x23,	0x24,	0x15,	0x06,	0x17,

/* 88	BS(H)	HT(I)	LF(J)	VT(K)	FF(L)	CR(M)	SO(N)	SI(O)	*/
	0x28,	0x29,	0x2A,	0x2B,	0x2C,	0x09,	0x0A,	0x1B,

/* 90	DLE(P)	DC1(Q)	DC2(R)	DC3(S)	DC4(T)	NAK(U)	SYN(V)	ETB(W)	*/
	0x30,	0x31,	0x1A,	0x33,	0x34,	0x35,	0x36,	0x0B,

/* 98	CAN(X)	EM(Y)	SUB(Z)	ESC([)	FS(\)	GS(])	RS(^)	US(_)	*/
	0x38,	0x39,	0x3A,	0x3B,	0x04,	0x14,	0x3E,	0xE1,

/* A0	SP	!	"	#	$	%	&	'	*/
	0x41,	0x42,	0x43,	0x44,	0x45,	0x46,	0x47,	0x48,

/* A8	(	)	*	+	,	-	.	/	*/
	0x49,	0x51,	0x52,	0x53,	0x54,	0x55,	0x56,	0x57,

/* B0	0	1	2	3	4	5	6	7	*/
	0x58,	0x59,	0x62,	0x63,	0x64,	0x65,	0x66,	0x67,

/* B8	8	9	:	;	<	=	>	?	*/
	0x68,	0x69,	0x70,	0x71,	0x72,	0x73,	0x74,	0x75,

/* C0	@	A	B	C	D	E	F	G	*/
	0x76,	0x77,	0x78,	0x80,	0x8A,	0x8B,	0x8C,	0x8D,

/* C8	H	I	J	K	L	M	N	O	*/
	0x8E,	0x8F,	0x90,	0x6A,	0x9B,	0x9C,	0x9D,	0x9E,

/* D0	P	Q	R	S	T	U	V	W	*/
	0x9F,	0xA0,	0xAA,	0xAB,	0xAC,	0x4A,	0xAE,	0xAF,

/* D8	X	Y	Z	[	\	]	^	_	*/
	0xB0,	0xB1,	0xB2,	0xB3,	0xB4,	0xB5,	0xB6,	0xB7,

/* E0	`	a	b	c	d	e	f	g	*/
	0xB8,	0xB9,	0xBA,	0xBB,	0xBC,	0xA1,	0xBE,	0xBF,

/* E8	h	i	j	k	l	m	n	o	*/
	0xCA,	0xCB,	0xCC,	0xCD,	0xCE,	0xCF,	0xDA,	0xDB,

/* F0	p	q	r	s	t	u	v	w	*/
	0xDC,	0xDD,	0xDE,	0xDF,	0xEA,	0xEB,	0xEC,	0xED,

/* F8	x	y	z	{	|	}	~	DEL	*/
	0xEE,	0xEF,	0xFA,	0xFB,	0xFC,	0xFD,	0xFE,	0xFF,
};


EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	ret = 0;

	save_args(ac, av);
	getopts(ac, av);
	tty = stdin;

	getstarttime();
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) set_signal(SIGINT, intr);
#ifdef	SIGQUIT
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		(void) set_signal(SIGQUIT, intr);
#endif
#ifdef	SIGINFO
	/*
	 * Be polite to *BSD users.
	 * They copied our idea and implemented intermediate status
	 * printing in 'dd' in 1990.
	 */
	if (signal(SIGINFO, SIG_IGN) != SIG_IGN)
		(void) set_signal(SIGINFO, intr);
#endif

#ifdef	USE_REMOTE
	rmtdebug(debug);
	if (infile)
		rmtin = rmtfilename(infile);
	if (outfile)
		rmtout = rmtfilename(outfile);
	if (rmtin)
		rmtifd = openremote(infile, ibs); /* Needs root privilleges */

	if (rmtout)
		rmtofd = openremote(outfile, obs); /* Needs root privilleges */
#endif


	if (geteuid() != getuid()) {	/* AIX does not like to do this */
					/* If we are not root		*/
#ifdef	HAVE_SETREUID
		if (setreuid(-1, getuid()) < 0)
#else
#ifdef	HAVE_SETEUID
		if (seteuid(getuid()) < 0)
#else
		if (setuid(getuid()) < 0)
#endif
#endif
			comerr("Panic cannot set back effective uid.\n");
	}

	if (infile) {
		if (rmtin) {
			ropenfile(rmtifd, rmtin, O_RDONLY);
		} else {
			fin = openfile(infile, "ru");
		}
	} else {
		fin = stdin;
		setbuf(fin, NULL);
		file_raise(fin, FALSE);
		infile = "stdin";
		if (ivsize != 0 || ovsize != 0)
#ifdef	HAVE__DEV_TTY
			tty = openfile("/dev/tty", "r");
#else
			tty = stderr;
#endif
	}
	if (outfile) {
		if (rmtout) {
			ropenfile(rmtofd, rmtout,
					notrunc ?
					(O_WRONLY|O_CREAT):
					(O_WRONLY|O_CREAT|O_TRUNC));
		} else {
			fout = openfile(outfile,
					notrunc ? "wcu" : "wctu");
		}
	} else {
		fout = stdout;
		setbuf(fout, NULL);
		file_raise(fout, FALSE);
		outfile = "stdout";
	}
	if (rmtin)
		ifd = rmtifd;
	else
		ifd = fdown(fin);
	if (rmtout)
		ofd = rmtofd;
	else
		ofd = fdown(fout);

	ivpos = iseek + ivseek;
	ovpos = oseek + ovseek;
	(void) riseek(ivpos);
	(void) roseek(ovpos);

	if (flags & MD5SUM)
		mdinit();

	getstarttime();

	if ((obs != ibs) ||
	    (flags & (BLOCK|UNBLOCK)) ||	/* Reblock forced uncond.   */
	    (ivsize && !(flags & NULLOUT)) ||	/* Reblock at end of vol    */
	    (ovsize &&
	    (((ovsize - ovpos) % obs) ||	/* Reblock at end of 1st vol */
	    ((ovsize - ovseek) % obs))))	/* Reblock at end of 2nd vol */
		copy_reblocked();
	else
		simple_copy();

	term(ret);
	return (0);	/* Keep lint happy */
}

LOCAL void
set_signal(sig, handler)
	int		sig;
	RETSIGTYPE	(*handler)	__PR((int));
{
#if	defined(HAVE_SIGPROCMASK) && defined(SA_RESTART)
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = handler;
	sa.sa_flags = SA_RESTART;
	(void) sigaction(sig, &sa, (struct sigaction *)0);
#else
#ifdef	HAVE_SIGSETMASK
	struct sigvec	sv;

	sv.sv_mask = 0;
	sv.sv_handler = handler;
	sv.sv_flags = 0;
	(void) sigvec(sig, &sv, (struct sigvec *)0);
#else
	(void) signal(sig, handler);
#endif
#endif
}

LOCAL void
intr(sig)
	int	sig;
{
	(void) signal(sig, intr);
	prstats();
	if (sig == SIGINT) {
		errmsgno(EX_BAD, "KILLED by SIGINT.\n");
		exit(SIGINT);
	}
}

LOCAL FILE *
openfile(name, mode)
	char	*name;
	char	*mode;
{
	FILE	*f;

	if ((f = fileopen(name, mode)) == (FILE *) NULL)
		comerr("Can't open '%s'.\n", name);
	file_raise(f, FALSE);
	return (f);
}

#undef	roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

LOCAL char *
memalloc(size)
	long	size;
{
	char	*ret;
	unsigned int pagesize = 512;
	UIntptr_t l;

#ifdef	HAVE_GETPAGESIZE
	pagesize = getpagesize();
#else
#ifdef	_SC_PAGESIZE
	pagesize = sysconf(_SC_PAGESIZE);
#endif
#endif
	if ((ret = malloc((size_t)(size+pagesize))) == NULL)
		comerr("No memory.\n");

	l = (UIntptr_t)ret;
	l = roundup(l, pagesize);
	ret = (char *)l;

	return (ret);
}

/*
 * Simple copy
 *
 * Input buffer size == output buffer size.
 */
LOCAL void
simple_copy()
{
	register long	obcnt;
	register long	cnt;
	register char	*obp;
	register int	rflags;

	if (debug)
		error("Simple copy ...\n");

	obp = memalloc(bs);	/* obs == ibs == bs */
	rflags = flags;

	if (rflags & NULLIN)
		fill(obp, 0L, bs);

	while ((cnt = readvol(obp, bs)) > 0) {
		if (rflags & NULLOUT) {
			if (progress && !debug) {
				(void) putc('.', stderr);
				(void) fflush(stderr);
			}
			continue;
		}
		if (cnt < bs && (rflags & FILL)) {
			fill(obp, cnt, bs);
			cnt = bs;
		}
		for (obcnt = 0; obcnt < cnt; )
			obcnt += writevol(obp + obcnt, (long) (cnt - obcnt));
	}
}

/*
 * Copy reblocked
 *
 * Input buffer size != output buffer size or any other condition
 * that forces us to use not the simple method.
 */
LOCAL void
copy_reblocked()
{
	register long	obcnt = 0;
	register long	cnt;
	register char	*obp;

	if (debug)
		error("Copy reblocked ...\n");

	obp = memalloc((long) (obs + ibs));

	while ((cnt = readvol(obp + obcnt, ibs)) > 0) {
		obcnt += cnt;
		cnt = 0;
		for (cnt = 0; obcnt - cnt >= obs; )
			cnt += writevol(obp + cnt, obs);
		if (cnt != 0) {
			if (obcnt > cnt) {
				if (debug)
					error("Moving down %ld bytes.\n",
								obcnt - cnt);
				(void) movebytes(obp + cnt, obp, obcnt - cnt);
			}
			obcnt -= cnt;
		}
	}
	if (obcnt > 0) {
		if (debug)
			error("Writing tail of %ld bytes\n", obcnt);
		if (flags & FILL) {
			fill(obp, obcnt, obs);
			obcnt = obs;
		}
		cnt = 0;
		while (obcnt - cnt > 0)
			cnt += writevol(obp + cnt, (long) (obcnt - cnt));
	}
}

/*
 * Read one input block from the input volume.
 * Switch to the next input volume if needed.
 * Do conversions.
 */
LOCAL long
readvol(buf, len)
	char	*buf;
	long	len;
{
	Llong	left;
	long	cnt;
	register int	rflags;

	rflags = flags;

	if (count != 0 && irec >= count)
		return (0);

	if (rflags & NULLIN) {
		irec++;
		return (len);
	}

	if (ivsize == 0) {
		cnt = readbuf(buf, len);
	} else for (;;) {
		if ((left = ivsize - ivpos) > 0) {
			if ((cnt = readbuf(buf, (long) min(len, left))) > 0)
				break;
		}
		if (!next_in())
			return (0);
	}
	ivpos += cnt;
	if (cnt == ibs)
		irec++;
	else
		iparts += cnt;
	if (rflags & NULLOUT)
		return (cnt);
	if (rflags & SWAB)
		swabb(buf, cnt);
	if (rflags & ASCII)
		conv(buf, cnt, asctab);
	if (rflags & LCASE)
		lcase(buf, cnt);
	if (rflags & UCASE)
		ucase(buf, cnt);
	if (rflags & BLOCK)
		cnt = block(buf, cnt);
	if (rflags & UNBLOCK)
		cnt = unblock(buf, cnt);
	if (rflags & EBCDIC)
		conv(buf, cnt, ebctab);
	if (rflags & IBM)
		conv(buf, cnt, ibmtab);
	return (cnt);
}

/*
 * Write one block to the output volume.
 * Switch to the next output volume if needed.
 */
LOCAL long
writevol(buf, len)
	char	*buf;
	long	len;
{
	long cnt;

	if (ovsize != 0) {
		if (ovpos >= ovsize)
			if (!next_out())
				term(EX_BAD);
		len = min(len, ovsize - ovpos);
	}
	cnt = writebuf(buf, len);
	ovpos += cnt;
	if (cnt == obs)
		orec++;
	else
		oparts += cnt;
	return (cnt);
}

/*
 * Switch to next input volume.
 */
LOCAL BOOL
next_in()
{
	return (next(infile, "input", ifd, ivpos = ivseek, ivolnum++));
}

/*
 * Switch to next ouput volume.
 */
LOCAL BOOL
next_out()
{
	return (next(outfile, "output", ofd, ovpos = ovseek, ovolnum++));
}

/*
 * Switch to next I/O volume.
 */
LOCAL BOOL
next(fname, inout, fd, pos, volnum)
	char	*fname;
	char	*inout;
	int	fd;
	long	pos;
	int	volnum;
{
	if (progress || debug) {
		(void) putc('\n', stderr);
		(void) fflush(stderr);
	}
	errmsgno(EX_BAD, "Done with %s volume # %d.\n", inout, volnum);
	if (!cont(inout, ++volnum))
		return (FALSE);
	error("Insert %s volume # %d in '%s' and then hit <cr>: ",
						inout, volnum, fname);
	(void) fflush(stderr);
	while (getc(tty) != '\n')
		if (feof(tty))
			return (FALSE);
	error("Working on %s volume # %d of '%s'.\n", inout, volnum, fname);
	if (fd == ifd)
		(void) riseek(pos);
	else
		(void) roseek(pos);
	return (TRUE);
}

LOCAL BOOL
cont(inout, num)
	char	*inout;
	int	num;
{
		char	answer [16];
	register char	*ap;

	for (;;) {
		error("Do you want to continue with %s volume # %d (y/n): ",
						inout, num);
		(void) fflush(stderr);
		ap = answer;
		if (fgetline(tty, ap, 16) == EOF)
			return (FALSE);
		while (*ap == ' ' || *ap == '\t')
			ap++;
		makelower(ap);
		if (streql(ap, "y") || streql(ap, "yes"))
			return (TRUE);
		if (streql(ap, "n") || streql(ap, "no"))
			return (FALSE);
	}
}

LOCAL void
makelower(s)
	register char	*s;
{
	while (*s) {
		if (*s >= 'A' && *s <= 'Z')
			*s += 'a' - 'A';
		else if (*s == ' ' || *s == '\t') {
			*s = '\0';
			return;
		}
		s++;
	}
}

/*
 * Read one input block.
 * Call readblocks if an error occured and -noerror has been specified.
 */
LOCAL long
readbuf(buf, len)
	char	*buf;
	long	len;
{
	long	cnt;
	int	err = 0;

	lastreaderrs = (Llong)0;
	if (debug) {
		error("readbuf  (%d, %p, %ld) ", ifd, (void *)buf, len);
		(void) fflush(stderr);
	}
	if (noerror && noseek)
		fill(buf, 0L, len);
	cnt = rread(buf, len);
	if (debug) {
		if (cnt < 0)
			err = geterrno();
		error("= %ld\n", cnt);
	}
	if (cnt < 0) {
		if (!debug)
			err = geterrno();
		if (progress && !debug)
			(void) putc('\n', stderr);
		errmsgno(err, "Error reading '%s'.\n", infile);
#ifdef	ECONNRESET
		if (noerror && err != EPIPE && err != ECONNRESET) {
#else
		if (noerror && err != EPIPE) {
#endif
			seterrno(err);
			cnt = readblocks(buf, len);
		} else {
			term(err);
		}
	}
	if (cnt == 0)
		if (count != 0 && ivsize == 0) {
			if (progress || debug)
				(void) putc('\n', stderr);
			errmsgno(EX_BAD, "END OF FILE\n");
		}

	if ((flags & (NULLOUT|MD5SUM)) == (NULLOUT|MD5SUM))
		mdupdate(buf, cnt);
	return (cnt);
}

/*
 * Write one output block.
 * Call writeblocks if an error occured and -noerror has been specified.
 */
LOCAL long
writebuf(buf, len)
	char	*buf;
	long	len;
{
	long	cnt;
	int	err = 0;

	if (debug)
		error("writebuf (%d, %p, %ld)\n", ofd, (void *)buf, len);
	if ((lastreaderrs > (Llong)0) && noerrwrite) {
		if (debug)
			error("seek(%d, %lld)\n", ofd, (Llong)(ovpos + len));
		if (roseek(ovpos + len) == (off_t)-1) {
			err = geterrno();
			if (progress && !debug)
				(void) putc('\n', stderr);
			errmsgno((int) err, "Error seeking '%s'.\n", outfile);
		}
		cnt = len;
	} else if ((cnt = rwrite(buf, len)) <= 0) {
		if (debug)
			error("rwrite() -> cnt %ld\n", cnt);
		if (cnt == 0)		/* EOF */
			err = ENDOFFILE;
		else if (cnt < 0)
			err = geterrno();
		if (progress && !debug)
			(void) putc('\n', stderr);
		errmsgno((int) err, "Error writing '%s'.\n", outfile);
		if (noerror &&
#ifdef	ECONNRESET
		    err != ENDOFFILE && err != EPIPE && err != ECONNRESET) {
#else
		    err != ENDOFFILE && err != EPIPE) {
#endif
			seterrno(err);
			cnt = writeblocks(buf, len);
		} else {
			term((int) err);
		}
	}
	if (progress && !debug) {
		(void) putc('.', stderr);
		(void) fflush(stderr);
	}
	if ((flags & (NULLOUT|MD5SUM)) == MD5SUM)
		mdupdate(buf, cnt);
	return (cnt);
}

/*
 * Input error recovery
 */
LOCAL long
readblocks(buf, len)
	register char	*buf;
	register long	len;
{
	register long	cnt;
	register long	aktlen;
	register int	trys;
	register off_t	pos = ivpos;
	register long	total = 0;
		int	err = 0;

	if (noseek) {
		errmsgno(EX_BAD, "Can't read %ld Bytes at %lld\n",
							len, (Llong)pos);
		readerrs++;
		return (len);
	} else {
		errmsgno(EX_BAD,
			"Retrying to read %ld Bytes at %lld (Block %lld)\n",
			len, (Llong)pos, (Llong)pos/(Llong)sdd_bsize);
	}
	while (len > 0) {
		aktlen = min(sdd_bsize, len);
		trys = 0;
		do {
			if (trys && !(trys & 15)) {
				if (debug) {
					(void) putc('+', stderr);
					(void) fflush(stderr);
				}
				(void) riseek((off_t)(ifsize() - sdd_bsize));
				(void) rread(buf, aktlen);

			} else if (trys > 0 && (trys == 2 || ! (trys & 7))) {
				if (debug) {
					(void) putc('-', stderr);
					(void) fflush(stderr);
				}
				(void) riseek((off_t)0);
				(void) rread(buf, aktlen);
			}
			if (debug) {
				(void) putc(',', stderr);
				(void) fflush(stderr);
			}
			fill(buf, 0L, aktlen);
			(void) riseek(pos);
			cnt = rread(buf, aktlen);
			if (cnt < 0) {
				err = geterrno();
#ifdef	ECONNRESET
				if (err == EPIPE || err == ECONNRESET)
#else
				if (err == EPIPE)
#endif
					break;
				err = 0;
			}
		} while (cnt < 0 && trys++ < try);

		if (cnt < 0) {
			if (progress || debug) {
				(void) putc('\n', stderr);
				(void) fflush(stderr);
			}
			errmsgno(EX_BAD, "Block %lld not read correctly.\n",
						(Llong)pos/(Llong)sdd_bsize);
			if (err != 0)
				term((int) err);
			cnt = aktlen;
			readerrs++;
			lastreaderrs++;

		} else if (cnt == 0)	/* EOF */
			break;
		buf += cnt;
		len -= cnt;
		pos += cnt;
		total += cnt;
	}
	(void) riseek(pos);
	return (total);
}

/*
 * Output error recovery
 */
LOCAL long
writeblocks(buf, len)
	register char	*buf;
	register long	len;
{
	register long	cnt;
	register long	aktlen;
	register int	trys;
	register off_t	pos = ovpos;
	register long	total = 0;
/*		char	rdbuf[sdd_bsize];*/
		int	err = 0;

	if (noseek) {
		errmsgno(EX_BAD, "Can't write %ld Bytes at %lld\n",
							len, (Llong)pos);
		writeerrs++;
		return (len);
	} else {
		errmsgno(EX_BAD,
			"Retrying to write %ld Bytes at %lld (Block %lld)\n",
			len, (Llong)pos, (Llong)pos/(Llong)sdd_bsize);
	}
	while (len > 0) {
		aktlen = min(sdd_bsize, len);
		trys = 0;
		do {
			if (trys && !(trys & 15)) {
				if (debug) {
					(void) putc('>', stderr);
					(void) fflush(stderr);
				}
				(void) roseek((off_t)(ifsize() - sdd_bsize));
/* XXX we would need to read the output file here - open with "r" ??? */
/*				(void) rread(rdbuf, aktlen);*/

			} else if (trys > 0 && (trys == 2 || ! (trys & 7))) {
				if (debug) {
					(void) putc('<', stderr);
					(void) fflush(stderr);
				}
				(void) roseek((off_t)0);
/* XXX we would need to read the output file here - open with "r" ??? */
/*				(void) rread(rdbuf, aktlen);*/
			}
			if (debug) {
				(void) putc(';', stderr);
				(void) fflush(stderr);
			}
			(void) roseek(pos);
			cnt = rwrite(buf, aktlen);
			if (cnt < 0) {
				err = geterrno();
#ifdef	ECONNRESET
				if (err == EPIPE || err == ECONNRESET)
#else
				if (err == EPIPE)
#endif
					break;
				err = 0;
			}
		} while (cnt < 0 && trys++ < try);
		if (cnt < 0) {
			if (progress || debug) {
				(void) putc('\n', stderr);
				(void) fflush(stderr);
			}
			errmsgno(EX_BAD, "Block %lld not written correctly.\n",
						(Llong)pos/(Llong)sdd_bsize);
			if (err != 0)
				term((int) err);
			cnt = aktlen;
			writeerrs++;

		} else if (cnt == 0)	/* EOF */
			break;
		buf += cnt;
		len -= cnt;
		pos += cnt;
		total += cnt;
	}
	(void) roseek(pos);
	return (total);
}

LOCAL void
fill(bp, start, end)
	char	*bp;
	long	start;
	long	end;
{
#ifdef	OLD
	register char *p = &bp[start];
	register char *ep = &bp[end];

	while (p < ep)
		*p++ = '\0';
#else
	fillbytes(&bp[start], end-start, '\0');
#endif
}

#define	DO8(a)  a; a; a; a; a; a; a; a;

LOCAL void
swabb(bp, cnt)
	register char	*bp;
	register long	cnt;
{
	register char	c;

	cnt /= 2;	/* even count only */
	while ((cnt -= 8) >= 0) {
		DO8(
			c = *bp++;
			bp[-1] = *bp;
			*bp++ = c;
		);
	}
	cnt += 8;
	while (--cnt >= 0) {
		c = *bp++;
		bp[-1] = *bp;
		*bp++ = c;
	}
}

LOCAL void
lcase(bp, cnt)
	register char	*bp;
	register long	cnt;
{
	while (--cnt >= 0) {
		if (*bp >= 'A' && *bp <= 'Z')
			*bp += 'a' - 'A';
		bp++;
	}
}

LOCAL void
ucase(bp, cnt)
	register char	*bp;
	register long	cnt;
{
	while (--cnt >= 0) {
		if (*bp >= 'a' && *bp <= 'z')
			*bp -= 'a' - 'A';
		bp++;
	}
}

LOCAL long
block(bp, cnt)
	register char	*bp;
	register long	cnt;
{
	register long	ocnt;

	ocnt = cnt;
	while (--cnt >= 0) {
		bp++;
	}
	return (ocnt);
}

LOCAL long
unblock(bp, cnt)
	register char	*bp;
	register long	cnt;
{
	register long	ocnt;

	ocnt = cnt;
	while (--cnt >= 0) {
		bp++;
	}
	return (ocnt);
}

LOCAL void
conv(bp, cnt, tab)
	register char	*bp;
	register long	cnt;
	register unsigned char	*tab;
{
	register char	c;

	while ((cnt -= 8) >= 0) {
		DO8(
			c = (char)tab[(unsigned char) *bp];
			*bp++ = c;
		);
	}
	cnt += 8;
	while (--cnt >= 0) {
		c = (char)tab[(unsigned char) *bp];
		*bp++ = c;
	}

	/* Reihenfolge der Auswertung ist nicht sichergestellt !!! bei: */
	/* XXX		*bp++ = tab[(unsigned char) *bp];*/
}

LOCAL void
term(ret)
	int	ret;
{
	if (rmtout) {
#ifdef	USE_REMOTE
		/*
		 * Cannot happen in non remote versions.
		 */
		if (rmtclose(rmtofd) < 0)
			ret = geterrno();
#endif
	} else {
#ifdef	HAVE_FSYNC
		int	cnt = 0;

		do {
			if (fsync(ofd) != 0)
				ret = geterrno();

			if (ret == EINVAL)
				ret = 0;
		} while (ret == EINTR && ++cnt < 10);
#endif
		if (close(ofd) != 0 && ret == 0)
			ret = geterrno();
	}
	prstats();
	exit(ret);
}

LOCAL void
getstarttime()
{
#ifdef	timerclear
	if (showtime && gettimeofday(&starttime, 0L) < 0)
		comerr("Cannot get starttime\n");
#endif
}

LOCAL void
prstats()
{
	Llong	savirec = (Llong)0;
	Llong	ibytes;
	Llong	obytes;
	Llong	ikbytes;
	Llong	okbytes;
	int	iper;
	int	oper;
#ifdef	timerclear
	long	sec;
	long	usec;
	long	tmsec;
#endif

#ifdef	timerclear
	if (showtime && gettimeofday(&stoptime, 0L) < 0)
			comerr("Cannot get stoptime\n");
#endif
	if (flags & NULLIN) {
		savirec = irec;
		irec = (Llong)0;
	}
	ibytes = irec * (Llong)ibs + iparts;
	obytes = orec * (Llong)obs + oparts;
	ikbytes = ibytes >> 10;
	okbytes = obytes >> 10;
	iper = ((ibytes&1023)<<10)/10485;
	oper = ((obytes&1023)<<10)/10485;

	if (progress || debug) (void) putc('\n', stderr);
	if (readerrs)
		errmsgno(EX_BAD, "%lld %s(s) not read correctly.\n",
					readerrs, noseek?"Record":"Block");
	if (writeerrs)
		errmsgno(EX_BAD, "%lld %s(s) not written correctly.\n",
					writeerrs, noseek?"Record":"Block");

	errmsgno(EX_BAD,
	"Read  %lld records + %lld bytes (total of %lld bytes = %lld.%02dk).\n",
	irec, iparts, ibytes, ikbytes, iper);
	errmsgno(EX_BAD,
	"Wrote %lld records + %lld bytes (total of %lld bytes = %lld.%02dk).\n",
	orec, oparts, obytes, okbytes, oper);

	if (flags & NULLIN) {
		irec = savirec;
		ibytes = obytes;
		ikbytes = okbytes;
	}
#ifdef	timerclear
	if (showtime) {
		long	kbs;

		sec = stoptime.tv_sec - starttime.tv_sec;
		usec = stoptime.tv_usec - starttime.tv_usec;
		tmsec = sec*1000 + usec/1000;
		if (usec < 0) {
			sec--;
			usec += 1000000;
		}
		if (tmsec == 0)
			tmsec++;

		kbs = ikbytes*(Llong)1000/tmsec;
		errmsgno(EX_BAD, "Total time %ld.%03ldsec (%ld kBytes/sec)\n",
				sec, usec/1000, kbs);
	}
#endif
	if (flags & MD5SUM)
		mdfinal();
}

LOCAL	char	opts[]	= "\
if*,of*,ibs&,obs&,bs&,cbs&,secsize&,\
count&,ivsize&,ovsize&,iseek&,oseek&,seek&,\
iskip&,oskip&,skip&,ivseek&,ovseek&,\
notrunc,pg,noerror,noerrwrite,noseek,try#,\
fill,swab,block,unblock,lcase,ucase,ascii,ebcdic,ibm,\
md5,\
inull,onull,help,version,debug,time,t";

LOCAL void
getopts(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	* const *cav;
	BOOL	help	= FALSE;
	BOOL	prvers	= FALSE;
	BOOL	fillflg	= FALSE;
	BOOL	swabflg	= FALSE;
	BOOL	blkflg	= FALSE;
	BOOL	ublkflg	= FALSE;
	BOOL	lcflg	= FALSE;
	BOOL	ucflg	= FALSE;
	BOOL	ascflg	= FALSE;
	BOOL	ebcflg	= FALSE;
	BOOL	ibmflg	= FALSE;
	BOOL	md5flg	= FALSE;
	BOOL	nullin	= FALSE;
	BOOL	nullout	= FALSE;
	int	trys	= -1;
	Llong	lliseek	 = (Llong)0;
	Llong	lloseek	 = (Llong)0;
	Llong	llaseek	 = (Llong)0;
	Llong	llivseek = (Llong)0;
	Llong	llovseek = (Llong)0;

	cac = ac - 1;
	cav = av + 1;
	if (getallargs(&cac, &cav, opts,
				&infile, &outfile,
				getnum, &ibs, getnum, &obs, getnum, &bs,
				getnum,	&cbs,
				getnum,	&sdd_bsize,
				getllnum, &count,
				getllnum, &ivsize, getllnum, &ovsize,
				getllnum, &lliseek,  getllnum, &lloseek,
				getllnum, &llaseek,
				getllnum, &iskip,  getllnum, &oskip,
				getllnum, &skip,
				getllnum, &llivseek, getllnum, &llovseek,
#ifndef	lint			/* lint kann leider nur 52 args !!! */
				&notrunc,
				&progress,
				&noerror,
				&noerrwrite,
				&noseek,
				&trys,
				&fillflg,
				&swabflg,
				&blkflg, &ublkflg,
				&lcflg, &ucflg,
				&ascflg, &ebcflg, &ibmflg,
				&md5flg,
				&nullin,
				&nullout,
#endif
				&help, &prvers,
				&debug, &showtime, &showtime) < 0) {
		errmsgno(EX_BAD, "Bad Option: '%s'\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prvers) {
		printf("sdd %s %s (%s-%s-%s)\n\n", "1.69", "2020/05/30",
					HOST_CPU, HOST_VENDOR, HOST_OS);
		printf("Copyright (C) 1984-2020 Jörg Schilling\n");
		printf("This is free software; see the source for copying ");
		printf("conditions.  There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or ");
		printf("FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}
	cac = ac - 1;
	cav = av + 1;
	if (getfiles(&cac, &cav, opts) != 0) {
		errmsgno(EX_BAD, "Bad Argument: '%s'\n", cav[0]);
		usage(EX_BAD);
	}

	iseek	= (off_t)lliseek;
	oseek	= (off_t)lloseek;
	seek	= (off_t)llaseek;
	ivseek	= (off_t)llivseek;
	ovseek	= (off_t)llovseek;

	if (iseek != lliseek || oseek != lloseek || seek != llaseek ||
		ivseek != llivseek || ovseek != llovseek) {

		errmsgno(EX_BAD,
		"Value of *seek= is too large for data type 'off_t'.\n");
		usage(EX_BAD);
	}

	if (trys >= 0) {
		if (!noerror) {
			errmsgno(EX_BAD, "'try' only with '-noerror'.\n");
			usage(EX_BAD);
		}
		if (noseek) {
			errmsgno(EX_BAD, "Can't try with -noseek.\n");
			usage(EX_BAD);
		}
		try = trys;
	}
	if ((iseek || oseek || seek) && (iskip || oskip || skip)) {
		errmsgno(EX_BAD, "Can't seek and skip.\n");
		usage(EX_BAD);
	}
	if ((iseek || oseek || seek) && noseek) {
		errmsgno(EX_BAD, "Can't seek and noseek.\n");
		usage(EX_BAD);
	}
	if (noseek && noerrwrite) {
		errmsgno(EX_BAD, "Can't noseek and noerrwrite.\n");
		usage(EX_BAD);
	}
	if (bs == 0)
		bs = sdd_bsize;
	if (ibs == 0)
		ibs = bs;
	if (obs == 0)
		obs = bs;
	/*
	 * It makes no sense to check for EISPIPE with lseek() and to
	 * disable seeking, since we currently do not distinct between
	 * noiseek and nooseek.
	 */
	if (noerror && !noseek) {
		if ((ibs == obs) &&
		    (bs % sdd_bsize)) {
			errmsgno(EX_BAD,
			    "Buffer size must be a multiple of %ld.\n",
			    sdd_bsize);
			usage(EX_BAD);
		}
		if (ibs % sdd_bsize) {
			errmsgno(EX_BAD,
			    "Input buffer size must be a multiple of %ld.\n",
			    sdd_bsize);
			usage(EX_BAD);
		}
		if (obs % sdd_bsize) {
			errmsgno(EX_BAD,
			    "Output buffer size must be a multiple of %ld.\n",
			    sdd_bsize);
			usage(EX_BAD);
		}
	}
	if (iskip == 0)
		iskip = skip;
	if (oskip == 0)
		oskip = skip;
	if (iseek == 0)
		iseek = seek;
	if (oseek == 0)
		oseek = seek;
	if (iskip || oskip) {
		errmsgno(EX_BAD, "skip not implemented.\n");
		usage(EX_BAD);
	}
	if (fillflg)
		flags |= FILL;
	if (swabflg)
		flags |= SWAB;
	if (blkflg)
		flags |= BLOCK;
	if (ublkflg)
		flags |= UNBLOCK;
	if ((flags & (BLOCK|UNBLOCK)) && cbs == 0) {
		errmsgno(EX_BAD, "Must specify cbs if block or unblock.\n");
		usage(EX_BAD);
	}
	if (blkflg || ublkflg) {
		errmsgno(EX_BAD, "block/unblock not implemented.\n");
		usage(EX_BAD);
	}
	if (lcflg && ucflg) {
		errmsgno(EX_BAD, "Can't lcase and ucase.\n");
		usage(EX_BAD);
	}
	if (lcflg)
		flags |= LCASE;
	if (ucflg)
		flags |= UCASE;
	if (ascflg)
		flags |= ASCII;
	if (ebcflg && ibmflg) {
		errmsgno(EX_BAD, "Can't ebcdic and ibm.\n");
		usage(EX_BAD);
	}
	if (ebcflg)
		flags |= EBCDIC;
	if (ibmflg)
		flags |= IBM;
	if (md5flg)
		flags |= MD5SUM;
	if (nullin && nullout) {
		errmsgno(EX_BAD, "Can't inull and onull.\n");
		usage(EX_BAD);
	}
	if (nullin) {
		flags &= ~(BLOCK|UNBLOCK);
		flags |= NULLIN;
		ibs = bs = obs;
	}
	if (nullout) {
		flags &= ~(BLOCK|UNBLOCK);
		flags |= NULLOUT;
		obs = bs = ibs;
	}
}

LOCAL void
usage(ex)
	int ex;
{
	error("\
Usage:	sdd [option=value] [-flag]\n\
Options:\n\
");
	error("\
	if=name		  Read  input from name instead of stdin\n\
	of=name		  Write output to name instead of stdout\n\
	-inull		  Do not read input from file (use null char's)\n\
	-onull		  Do not write output to any file\n\
	ibs=#,obs=#,bs=#  Set input/outbut buffersize or both to #\n\
	cbs=#		  Set conversion buffersize to #\n\
	secsize=#	  Set basic buffersize for -noerror to # (default %ld)\n\
	ivsize=#,ovsize=# Set input/output volume size to #\n\
	count=#		  Transfer at most # input records\n\
	iseek=#,iskip=#	  Seek/skip # bytes on input before starting\n\
	oseek=#,oskip=#	  Seek/skip # bytes on output before starting\n\
	seek=#,skip=#	  Seek/skip # bytes on input/output before starting\n\
	ivseek=#,ovseek=# Seek # bytes on input/output volumes before starting\n\
",
	sdd_bsize);
	error("\
	-notrunc	  Do not trunctate existing output file\n\
	-pg		  Print a dot on each write to indicate progress\n\
	-noerror	  Do not stop on error\n\
	-noerrwrite	  Do not write blocks not read correctly\n\
	-noseek		  Don't seek\n\
	try=#		  Set error retrycount to # if -noerror (default 2)\n\
	-debug		  Print debugging messages\n\
	-fill		  Fill each record with zeros up to obs\n\
	-swab,-block,-unblock,-lcase,-ucase,-ascii,-ebcdic,-ibm\n\
	-md5		  Compute the md5 sum for the data\n\
");
	error("\t-help\t\t  print this online help\n");
	error("\t-version\t  print version number\n");
	exit(ex);
}

LOCAL int
openremote(filename, iosize)
	char	*filename;
	long	iosize;
{
	int	remfd	= -1;
	char	*remfn;
	char	host[128];

#ifdef	USE_REMOTE
	if ((remfn = rmtfilename(filename)) != NULL) {
		rmthostname(host, sizeof (host), filename);

		if (debug)
			errmsgno(EX_BAD, "Remote: %s Host: %s file: %s\n",
							filename, host, remfn);

		remfd = iosize;
		if (remfd != iosize) {
			comerrno(EX_BAD,
				"Buffer size %ld too large for remote operation.\n",
				iosize);
		}
		if ((remfd = rmtgetconn(host, (int)iosize, 0)) < 0)
			comerrno(EX_BAD, "Cannot get connection to '%s'.\n",
				/* errno not valid !! */		host);
	}
#else
	comerrno(EX_BAD, "Remote tape support not present.\n");
#endif
	return (remfd);
}

LOCAL int
ropenfile(rfd, name, mode)
	int	rfd;
	char	*name;
	int	mode;
{
#ifdef	USE_REMOTE
	int	fd;

	if ((fd = rmtopen(rfd, name, mode)) < 0)
		comerr("Can't open '%s'.\n", name);
	return (fd);
#else
	comerrno(EX_BAD, "Remote tape support not present.\n");
	/* NOTREACHED */
	return (-1);
#endif
}

LOCAL ssize_t
rread(buf, cnt)
	void	*buf;
	size_t	cnt;
{
#ifdef	USE_REMOTE
	if (rmtifd >= 0) {
		int	icnt = cnt;

		/*
		 * This check is needed as long as librmt uses int in rmtread()
		 */
		if (icnt != cnt) {
			seterrno(EINVAL);
			return (-1);
		}
		return (rmtread(rmtifd, buf, cnt));
	}
#endif
	return (_niread(ifd, buf, cnt));
}

LOCAL ssize_t
rwrite(buf, cnt)
	void	*buf;
	size_t	cnt;
{
#ifdef	USE_REMOTE
	if (rmtofd >= 0) {
		int	icnt = cnt;

		/*
		 * This check is needed as long as librmt uses int in rmtwrite()
		 */
		if (icnt != cnt) {
			seterrno(EINVAL);
			return (-1);
		}
		return (rmtwrite(rmtofd, buf, cnt));
	}
#endif
	return (_niwrite(ofd, buf, cnt));
}

LOCAL off_t
riseek(pos)
	off_t	pos;
{
#ifdef	USE_REMOTE
	if (rmtifd >= 0)
		return (rmtseek(rmtifd, pos, SEEK_SET));
#endif
	return (lseek(ifd, pos, SEEK_SET));
}

LOCAL off_t
roseek(pos)
	off_t	pos;
{
#ifdef	USE_REMOTE
	if (rmtofd >= 0)
		return (rmtseek(rmtofd, pos, SEEK_SET));
#endif
	return (lseek(ofd, pos, SEEK_SET));
}

LOCAL off_t
ifsize()
{
#ifdef	USE_REMOTE
	off_t	opos;
	off_t	pos;

	if (rmtifd >= 0) {
		opos = rmtseek(rmtifd, (off_t)0, SEEK_CUR);
		pos = rmtseek(rmtifd, (off_t)0, SEEK_END);
		(void) rmtseek(rmtifd, opos, SEEK_SET);
		return (pos);
	}
#endif
	return (filesize(fin));
}


MD5_CTX	MD5_context;

LOCAL void
mdinit()
{
	MD5Init(&MD5_context);
}

LOCAL void
mdupdate(a, s)
	void	*a;
	size_t	s;
{
	MD5Update(&MD5_context, a, s);
}

LOCAL void
mdfinal()
{
	MD5_CTX	ctx;
	UInt8_t result[MD5_DIGEST_LENGTH];

	ctx = MD5_context;

	MD5Final(result, &ctx);

	errmsgno(EX_BAD,
	"md5 %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			result[0],
			result[1],
			result[2],
			result[3],
			result[4],
			result[5],
			result[6],
			result[7],
			result[8],
			result[9],
			result[10],
			result[11],
			result[12],
			result[13],
			result[14],
			result[15]);
}
