/* @(#)func.h	1.72 18/10/14 Copyright 1984-2018 J. Schilling */
/*
 *	Definitions for global functions in VED
 *
 *	Copyright (c) 1984-2018 J. Schilling
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

/*
 * ved.c
 */
extern	int	main		__PR((int argc, char **argv));
extern	void	settmodes	__PR((ewin_t *wp));
extern	void	rsttmodes	__PR((ewin_t *wp));

/*
 * edit.c
 */
extern	void	edit		__PR((ewin_t *wp));

/*
 * binding.c
 */
extern	void	init_binding	__PR((void));
extern	void	bindcmd		__PR((ewin_t *wp, Uchar *cmd, int cmdlen));

/*
 * vedtmpops.c
 */
extern	void	put_vedtmp	__PR((ewin_t *wp, BOOL inedit));
extern	BOOL	get_vedtmp	__PR((ewin_t *wp, epos_t *posp, int *colp));

/*
 * cmds.c
 */
extern 	void	vnorm		__PR((ewin_t *wp));
extern	void	vsnorm		__PR((ewin_t *wp));
extern	void	vnl		__PR((ewin_t *wp));
extern	void	vsnl		__PR((ewin_t *wp));
extern	void	verror		__PR((ewin_t *wp));
extern	void	modified	__PR((ewin_t *wp));
extern	void	vmode		__PR((ewin_t *wp));
extern	void	vwhere		__PR((ewin_t *wp));
extern	void	vewhere		__PR((ewin_t *wp));
extern	void	vswhere		__PR((ewin_t *wp));
extern	void	vsewhere	__PR((ewin_t *wp));
extern	void	vquote		__PR((ewin_t *wp));
extern	void	v8cntlq		__PR((ewin_t *wp));
extern	void	v8quote		__PR((ewin_t *wp));
extern	void	vhex		__PR((ewin_t *wp));
extern	void	vopen		__PR((ewin_t *wp));
extern	void	vsopen		__PR((ewin_t *wp));
extern	void	vhelp		__PR((ewin_t *wp));

/*
 * quitcmds.c
 */
extern	BOOL	bakbuf		__PR((ewin_t *wp, BOOL force));
extern	BOOL	writebuf	__PR((ewin_t *wp, BOOL force));
extern	void	vbackup		__PR((ewin_t *wp));
extern	void	vquit		__PR((ewin_t *wp));
extern	void	eexit		__PR((ewin_t *wp));
extern	void	vsuspend	__PR((ewin_t *wp));

/*
 * execcmds.c
 */
extern	void	vexec		__PR((ewin_t *wp));
extern	void	vtexec		__PR((ewin_t *wp));
extern	void	vsexec		__PR((ewin_t *wp));
extern	int	spawncmd	__PR((ewin_t *wp, char *name, char *arg));
extern	BOOL	white		__PR((int ch));

/*
 * numbercmds.c
 */
extern	void	initnum		__PR((void));
extern	void	vmult		__PR((ewin_t *wp));
extern	void	vsmult		__PR((ewin_t *wp));
extern	void	vnum		__PR((ewin_t *wp));

/*
 * cursorcmds.c
 */
extern	BOOL	dosnl		__PR((ewin_t *wp, epos_t pos));
extern	void	vforw		__PR((ewin_t *wp));
extern	void	vsforw		__PR((ewin_t *wp));
extern	void	vwforw		__PR((ewin_t *wp));
extern	void	vswforw		__PR((ewin_t *wp));
extern	void	vrev		__PR((ewin_t *wp));
extern	void	vsrev		__PR((ewin_t *wp));
extern	void	vwrev		__PR((ewin_t *wp));
extern	void	vswrev		__PR((ewin_t *wp));
extern	void	vup		__PR((ewin_t *wp));
extern	void	vsup		__PR((ewin_t *wp));
extern	void	vpup		__PR((ewin_t *wp));
extern	void	vspup		__PR((ewin_t *wp));
extern	void	vdown		__PR((ewin_t *wp));
extern	void	vsdown		__PR((ewin_t *wp));
extern	void	vpdown		__PR((ewin_t *wp));
extern	void	vspdwn		__PR((ewin_t *wp));
extern	void	vpageup		__PR((ewin_t *wp));
extern	void	vspageup	__PR((ewin_t *wp));
extern	void	vpagedwn	__PR((ewin_t *wp));
extern	void	vspagedwn	__PR((ewin_t *wp));
extern	void	vend		__PR((ewin_t *wp));
extern	void	vsend		__PR((ewin_t *wp));
extern	void	vpend		__PR((ewin_t *wp));
extern	void	vspend		__PR((ewin_t *wp));
extern	void	vbegin		__PR((ewin_t *wp));
extern	void	vsbegin		__PR((ewin_t *wp));
extern	void	vpbegin		__PR((ewin_t *wp));
extern	void	vspbegin	__PR((ewin_t *wp));
extern	void	vtop		__PR((ewin_t *wp));
extern	void	vstop		__PR((ewin_t *wp));
extern	void	vbottom		__PR((ewin_t *wp));
extern	void	vsbottom	__PR((ewin_t *wp));
extern	void	vadjwin		__PR((ewin_t *wp));
extern	void	vredisp		__PR((ewin_t *wp));
extern	void	vltopwin	__PR((ewin_t *wp));
extern	void	vbrack		__PR((ewin_t *wp));

/*
 * delcmds.c
 */
extern	void	delchars	__PR((ewin_t *wp, epos_t numchars));
extern	void	rubchars	__PR((ewin_t *wp, epos_t numchars));
extern	void	vdel		__PR((ewin_t *wp));
extern	void	vrub		__PR((ewin_t *wp));
extern	void	vwdel		__PR((ewin_t *wp));
extern	void	vwrub		__PR((ewin_t *wp));
extern	void	vkill		__PR((ewin_t *wp));
extern	void	vpkill		__PR((ewin_t *wp));
extern	void	vskill		__PR((ewin_t *wp));
extern	void	vundel		__PR((ewin_t *wp));

/*
 * searchcmds.c
 */
extern	void	vsearch		__PR((ewin_t *wp));
extern	void	vssearch	__PR((ewin_t *wp));
extern	void	vrsearch	__PR((ewin_t *wp));
extern	void	vsrsearch	__PR((ewin_t *wp));
extern	void	vagainsrch	__PR((ewin_t *wp));
extern	void	vsagainsrch	__PR((ewin_t *wp));
extern	void	vrevsrch	__PR((ewin_t *wp));
extern	void	vsrevsrch	__PR((ewin_t *wp));
extern	void	not_found	__PR((ewin_t *wp));

/*
 * filecmds.c
 */
extern	void	vread		__PR((ewin_t *wp));
extern	void	vwrite		__PR((ewin_t *wp));
extern	void	vchange		__PR((ewin_t *wp));
extern	BOOL	change_file	__PR((ewin_t *wp, Uchar	*file));
extern	void	fchange		__PR((ewin_t *wp, Uchar	*file));
extern	void	vswrite		__PR((ewin_t *wp));

/*
 * tmpfiles.c
 */
extern	FILE	*tmpfopen	__PR((ewin_t *wp, Uchar *name, char *mode));
extern	void	tmpsetup	__PR((void));
extern	void	takepath	__PR((Uchar *pathname, int pathsize,
					Uchar *name));
extern	void	tmpopen		__PR((ewin_t *wp));
extern	void	tmpcleanup	__PR((ewin_t *wp, BOOL force));

/*
 * takecmds.c
 */
extern	void	vcleartake	__PR((ewin_t *wp));
extern	void	vlsave		__PR((ewin_t *wp));
extern	void	vpsave		__PR((ewin_t *wp));
extern	void	vcsave		__PR((ewin_t *wp));
extern	void	vwsave		__PR((ewin_t *wp));
extern	void	vssave		__PR((ewin_t *wp));
extern	void	vgetclr		__PR((ewin_t *wp));
extern	void	vget		__PR((ewin_t *wp));
extern	void	vsgetclr	__PR((ewin_t *wp));
extern	void	vsget		__PR((ewin_t *wp));
extern	void	vtname		__PR((ewin_t *wp));
extern	void	vwrtake		__PR((ewin_t *wp));

/*
 * markcmds.c
 */
extern	void	setmark		__PR((ewin_t *wp, epos_t newmark));
extern	void	resetmark	__PR((ewin_t *wp));
extern	void	vjumpmark	__PR((ewin_t *wp));
extern	void	vexchmarkdot	__PR((ewin_t *wp));
extern	void	vclearmark	__PR((ewin_t *wp));
extern	void	vsetmark	__PR((ewin_t *wp));

/*
 * screen.c
 */
extern	void	update		__PR((ewin_t *wp));
extern	void	setwindow	__PR((ewin_t *wp));
extern	void	newwindow	__PR((ewin_t *wp));
extern	int	getindent	__PR((ewin_t *wp));
extern	void	setpos		__PR((ewin_t *wp));
extern	BOOL	setcursor	__PR((ewin_t *wp));
extern	epos_t	countpos	__PR((ewin_t *wp, epos_t x, epos_t y,
					cpos_t * pos));
extern	epos_t	findcol		__PR((ewin_t *wp, int h, epos_t x));
extern	void	dispup		__PR((ewin_t *wp, epos_t old, epos_t new));
extern	int	realvp		__PR((ewin_t *wp, cpos_t * pos));
extern	int	realhp		__PR((ewin_t *wp, cpos_t * pos));
extern	void	typescreen	__PR((ewin_t *wp, epos_t x, int col, epos_t y));

/*
 * ctab.c
 */
extern	void	init_charset	__PR((ewin_t *wp));

/*
 * movedot.c
 */
extern	epos_t	forwword	__PR((ewin_t *wp, epos_t start, ecnt_t n));
extern	epos_t	revword		__PR((ewin_t *wp, epos_t start, ecnt_t n));
extern	epos_t	forwline	__PR((ewin_t *wp, epos_t start, ecnt_t n));
extern	epos_t	revline		__PR((ewin_t *wp, epos_t start, ecnt_t n));
extern	epos_t	forwpara	__PR((ewin_t *wp, epos_t start, ecnt_t n));
extern	epos_t	revpara		__PR((ewin_t *wp, epos_t start, ecnt_t n));

/*
 * storage.c
 */
extern	void	insert		__PR((ewin_t *wp, Uchar* str, long size));
extern	void	delete		__PR((ewin_t *wp, epos_t size));
extern	void	rubout		__PR((ewin_t *wp, epos_t size));
extern	BOOL	loadfile	__PR((ewin_t *wp, Uchar* filename,
						BOOL newdefault));
extern	BOOL	savefile	__PR((ewin_t *wp, epos_t begin, epos_t end,
						FILE *  f, char *name));
extern	BOOL	backsavefile	__PR((ewin_t *wp, epos_t begin, epos_t end,
						FILE *  f, char *name));
extern	void	getfile		__PR((ewin_t *wp, FILE *  f, epos_t size,
						char *name));
extern	void	backgetfile	__PR((ewin_t *wp, FILE *  f, epos_t size,
						char *name));
extern	BOOL	isdos		__PR((ewin_t *wp));
extern	int	extract		__PR((ewin_t *wp, epos_t begin, Uchar *str,
						int size));
extern	int	extr_line	__PR((ewin_t *wp, epos_t begin, char *str,
						int size));
extern	int	retractline	__PR((ewin_t *wp, epos_t begin, char *str,
						int size));
#ifdef	_BUFFER_H
extern	void	clearifwpos	__PR((ewin_t *wp, headr_t *this));
#endif
extern	void	clearwpos	__PR((ewin_t *wp));
extern	void	findwpos	__PR((ewin_t *wp, epos_t new));
#ifdef	_BUFFER_H
extern	void	findpos		__PR((ewin_t *wp, epos_t pos,
						headr_t ** returnlink,
						int *returnpos));
#endif

/*
 * terminal.c
 */
extern	Uchar	*t_start	__PR((ewin_t *wp));
extern	void	t_begin		__PR((void));
extern	void	t_done		__PR((void));
extern	void	t_error		__PR((ewin_t *wp, char *s));
extern	void	t_home		__PR((ewin_t *wp));
extern	void	t_left		__PR((ewin_t *wp, int n));
extern	void	t_right		__PR((ewin_t *wp, int n));
extern	void	t_up		__PR((ewin_t *wp, int n));
extern	void	t_down		__PR((ewin_t *wp, int n));
extern	void	t_move		__PR((ewin_t *wp, int row, int col));
extern	void	t_move_abs	__PR((ewin_t *wp, int row, int col));
extern	void	t_clear		__PR((ewin_t *wp));
extern	void	t_cleos		__PR((ewin_t *wp));
extern	void	t_cleol		__PR((ewin_t *wp));
extern	void	t__cleol	__PR((ewin_t *wp, int do_move));
extern	void	t_insch		__PR((ewin_t *wp, char *str));
extern	void	t_insln		__PR((ewin_t *wp, int n));
extern	void	t_delch		__PR((ewin_t *wp, int n));
extern	void	t_delln		__PR((ewin_t *wp, int n));
extern	void	t_alt		__PR((char *str));
extern	void	t_setscroll	__PR((ewin_t *wp, int beg, int end));
extern	void	t_scrup		__PR((ewin_t *wp, int n));
extern	void	t_scrdown	__PR((ewin_t *wp, int n));

/*
 * cmdline.c
 */
extern	void	wait_for_confirm __PR((ewin_t *wp));
extern	void	wait_continue	__PR((ewin_t *wp));
/* PRINTFLIKE3 */
extern	int	getcmdchar	__PR((ewin_t *wp, char *ans,
					char *msg, ...)) __printflike__(3, 4);
/* PRINTFLIKE4 */
extern	int	getcmdline	__PR((ewin_t *wp, Uchar *result, int len,
					char *msg, ...)) __printflike__(4, 5);
/* PRINTFLIKE5 */
extern	int	getccmdline	__PR((ewin_t *wp, int c, Uchar* result, int len,
					char *msg, ...)) __printflike__(5, 6);

/*
 * io.c
 */
extern	echar_t	gchar		__PR((ewin_t *wp));
extern	echar_t	nigchar		__PR((ewin_t *wp));
extern	int	getnextc	__PR((ewin_t *wp));
extern	int	nigetnextc	__PR((ewin_t *wp));
extern	void	flushprot	__PR((ewin_t *wp));
extern	void	deleteprot	__PR((void));
extern	void	newprot		__PR((ewin_t *wp));
extern	void	openrecoverfile	__PR((ewin_t *wp, char *name));
extern	Uchar	*getrecoverfile	__PR((epos_t *posp, int *colp));
extern	int	putoutchar	__PR((int c));
extern	void	addchar		__PR((int c));
extern	void	onmark		__PR((void));
extern	void	offmark		__PR((void));
extern	void	addstr		__PR((Uchar *s));
extern	void	output		__PR((Uchar *s));
extern	void	printfield	__PR((Uchar *str, int len));
extern	void	printstring	__PR((Uchar *str, int nchars));
extern	void	ringbell	__PR((void));
extern	int	_bflush		__PR((int c));
extern	int	_bufflush	__PR((void));

/*
 * fileio.c
 */
extern	int	stmpfmodes	__PR((Uchar *name));
extern	FILE	*openerrmsg	__PR((Uchar *name, char *mode));
extern	FILE	*opencomerr	__PR((ewin_t *wp, Uchar *name, char *mode));
extern	FILE	*opensyserr	__PR((ewin_t *wp, Uchar *name, char *mode));
extern	int	readsyserr	__PR((ewin_t *wp, FILE *  f,
					void *buf, int len, Uchar *name));
extern	int	writesyserr	__PR((ewin_t *wp, FILE *  f,
					void *buf, int len, Uchar *name));
extern	int	writebsyserr	__PR((ewin_t *wp, FILE *  f,
					void *buf, int len, Uchar *name));
extern	int	writeerrmsg	__PR((ewin_t *wp, FILE *  f,
					void *buf, int len, Uchar *name));
/* PRINTFLIKE2 */
extern	void	exitcomerr	__PR((ewin_t *wp,
					char *fmt, ...)) __printflike__(2, 3);
/* PRINTFLIKE3 */
extern	void	excomerrno	__PR((ewin_t *wp, int err,
					char *fmt, ...)) __printflike__(3, 4);

/*
 * filesubs.c
 */
extern	void	lockfile	__PR((ewin_t *wp, char *filename));
extern	void	writelockmsg	__PR((ewin_t *wp));
extern	int	lockfd		__PR((int f));
extern	int	getfmodes	__PR((Uchar *name));
extern	long	gftime		__PR((char *file));
extern	BOOL	readable	__PR((Uchar *name));
extern	BOOL	writable	__PR((Uchar *name));
extern	BOOL	wrtcheck	__PR((ewin_t *wp, BOOL  err));
extern	BOOL	modcheck	__PR((ewin_t *wp));

/*
 * take.c
 */
extern	void	settakename	__PR((ewin_t *wp, Uchar *name));
extern	Uchar	*findtake	__PR((ewin_t *wp, Uchar *name));
extern	void	backuptake	__PR((ewin_t *wp));
extern	void	loadtake	__PR((ewin_t *wp));
extern	void	deletetake	__PR((void));
extern	int	fcopy		__PR((ewin_t *wp, FILE * from, FILE * to,
					epos_t size, char *fromname,
					char *toname));

/*
 * message.c
 */
extern	void	initmessage	__PR((ewin_t *wp));
extern	void	initmsgsize	__PR((ewin_t *wp));
/* PRINTFLIKE2 */
extern	void	writemsg	__PR((ewin_t *wp,
					char *str, ...)) __printf0like__(2, 3);
extern	void	writenum	__PR((ewin_t *wp, ecnt_t num));
extern	void	writetake	__PR((ewin_t *wp, Uchar *str));
extern	void	namemsg		__PR((Uchar* name));
/* PRINTFLIKE2 */
extern	void	writeerr	__PR((ewin_t *wp,
					char *str, ...)) __printflike__(2, 3);
/* PRINTFLIKE2 */
extern	void	writeserr	__PR((ewin_t *wp,
					char *str, ...)) __printflike__(2, 3);
/* PRINTFLIKE2 */
extern	void	write_errno	__PR((ewin_t *wp,
					char *msg, ...)) __printflike__(2, 3);
extern	void	defaultinfo	__PR((ewin_t *wp, Uchar *str));
extern	void	defaulterr	__PR((ewin_t *wp, Uchar *str));
extern	void	refreshmsg	__PR((ewin_t *wp));
extern	void	refreshsysline	__PR((ewin_t *wp));
extern	void	write_systemline __PR((ewin_t *wp));
extern	void	abortmsg	__PR((ewin_t *wp));
extern	void	nomarkmsg	__PR((ewin_t *wp));

/*
 * search.c
 */
extern	BOOL	issimple	__PR((Uchar* pattern, int length));
extern	epos_t	srevchar	__PR((ewin_t *wp, epos_t begin, int ch));
extern	epos_t	search		__PR((ewin_t *wp, epos_t begin,
					Uchar *pattern, int size, int domark));
extern	epos_t	reverse		__PR((ewin_t *wp, epos_t begin,
					Uchar *pattern, int size, int domark));

/*
 * ttymodes.c
 */
extern	void	get_modes	__PR((ewin_t *wp));
extern	void	set_modes	__PR((void));
extern	void	set_oldmodes	__PR((void));

/*
 * macro.c
 */
extern	char *	myhome		__PR((void));
extern	void	macro_init	__PR((ewin_t *wp));
extern	void	vxmac		__PR((ewin_t *wp));
extern	void	vmac		__PR((ewin_t *wp));
extern	int	gmacro		__PR((void));
extern	void	vemac		__PR((ewin_t *wp));
extern	void	macro_reinit	__PR((ewin_t *wp));

/*
 * coloncmds.c
 */
extern	void	vcolon		__PR((ewin_t *wp));
/* PRINTFLIKE2 */
extern	void	printscreen	__PR((ewin_t *wp,
					char *form, ...)) __printflike__(2, 3);

/*
 * substcmds.c
 */
extern	void	subst		__PR((ewin_t *wp, Uchar *cmd, int cmdlen));

/*
 * tags.c
 */
extern	int	gettag		__PR((Uchar** name));
extern	epos_t	searchtag	__PR((ewin_t *wp, epos_t opos));
extern	void	vtag		__PR((ewin_t *wp));
extern	void	gototag		__PR((ewin_t *wp, Uchar *tag));
extern	void	vrtag		__PR((ewin_t *wp));

/*
 * map.c
 */
extern	int	mapgetc		__PR((ewin_t *wp));
extern	void	map_init	__PR((void));
extern	int	rxmap		__PR((ewin_t *wp, int c));
extern	int	gmap		__PR((void));
extern	void	remap		__PR((void));
extern	BOOL	add_map		__PR((char *from, char *to, char *comment));
extern	BOOL	del_map		__PR((char *from));
extern	void	list_map	__PR((ewin_t *wp));
extern	void	init_fk_maps	__PR((void));

/*
 * vedstats.c
 */
extern	void	vedstartstats	__PR((void));
extern	void	vedstopstats	__PR((void));
extern	void	vedstatistics	__PR((void));

/*
 * consdebug.c
 */
/* PRINTFLIKE1 */
extern	void	cdbg		__PR((char *fmt, ...)) __printflike__(1, 2);
extern	void	writecons	__PR((char *s));
extern	long	getcaller	__PR((void));
