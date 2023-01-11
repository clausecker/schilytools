/* @(#)starsubs.h	1.144 20/07/19 Copyright 1996-2020 J. Schilling */
/*
 *	Prototypes for star subroutines
 *
 *	Copyright (c) 1996-2020 J. Schilling
 *	Copyright (c) 2022-2023 the schilytools team
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

#include <schily/ccomdefs.h>

#ifndef	_SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

/*
 * star.c
 */
extern	int	main		__PR((int ac, char **av));
extern	int	getnum		__PR((char *arg, long *valp));
extern	int	getbnum		__PR((char *arg, Llong *valp));
extern	int	getknum		__PR((char *arg, Llong *valp));
extern	void	set_signal	__PR((int sig, RETSIGTYPE (*handler)(int)));
extern	void	copy_create	__PR((int ac, char *const *av));
extern	void	star_verifyopts	__PR((void));
#ifdef	EOF
extern	BOOL	ttyerr		__PR((FILE *f));
#endif

/*
 * chdir.c
 */
extern	char	*dogetwdir	__PR((BOOL doexit));
extern	BOOL	dochdir		__PR((const char *dir, BOOL doexit));

/*
 * match.c
 */
extern	const char *filename	__PR((const char *name));
extern	BOOL	match		__PR((const char *name));
extern	int	addpattern	__PR((const char *pattern));
extern	int	addarg		__PR((const char *pattern));
extern	void	closepattern	__PR((void));
extern	void	prpatstats	__PR((void));

/*
 * buffer.c
 */
extern	void	opt_remote	__PR((void));
extern	BOOL	openremote	__PR((void));
extern	void	opentape	__PR((void));
extern	void	closetape	__PR((void));
extern	void	changetape	__PR((BOOL donext));
extern	void	runnewvolscript	__PR((int volno, int nindex));
extern	void	nextitape	__PR((void));
extern	void	nextotape	__PR((void));
extern	long	startvol	__PR((char *buf, long amount));
extern	void	newvolhdr	__PR((char *buf, long amount, BOOL do_fifo));
extern	void	initbuf		__PR((int nblocks));
extern	void	markeof		__PR((void));
extern	void	marktcb		__PR((char *addr));
extern	void	syncbuf		__PR((void));
extern	long	peekblock	__PR((char *buf, long amount));
extern	long	readblock	__PR((char *buf, long amount));
extern	ssize_t	readtape	__PR((char *buf, size_t amount));
#ifdef _STAR_H
extern	void	filltcb		__PR((TCB *ptb));
extern	void	movetcb		__PR((TCB *from_ptb, TCB *to_ptb));
#endif
extern	void	*get_block	__PR((long amount));
extern	void	put_block	__PR((long amount));
extern	char	*writeblock	__PR((char *buf));
extern	ssize_t	writetape	__PR((char *buf, size_t amount));
extern	void	writeempty	__PR((void));
extern	void	weof		__PR((void));
extern	void	buf_sync	__PR((long size));
extern	void	buf_drain	__PR((void));
extern	long	buf_wait	__PR((long amount));
extern	void	buf_wake	__PR((long amount));
extern	long	buf_rwait	__PR((long amount));
extern	void	buf_rwake	__PR((long amount));
extern	void	buf_resume	__PR((void));
extern	void	backtape	__PR((void));
extern	int	mtioctl		__PR((int cmd, int count));
extern	off_t	mtseek		__PR((off_t offset, int whence));
extern	Llong	tblocks		__PR((void));
extern	void	prstats		__PR((void));
extern	BOOL	checkerrs	__PR((void));
extern	void	exprstats	__PR((int ret));
extern	void	excomerrno	__PR((int err,
					char *fmt, ...)) __printflike__(2, 3);
extern	void	excomerr	__PR((char *fmt, ...)) __printflike__(1, 2);
extern	void	die		__PR((int err));

/*
 * append.c
 */
#ifdef _STAR_H
extern	void	skipall		__PR((void));
extern	BOOL	update_newer	__PR((FINFO *info));
#endif

/*
 * create.c
 */
extern	void	checklinks	__PR((void));
extern	ssize_t	_fileread	__PR((int *fp, void *buf, size_t len));
extern	void	create		__PR((char *name, BOOL Hflag, BOOL forceadd));
#if	defined(_SCHILY_WALK_H)
extern	void	createlist	__PR((struct WALK *state));
#endif
#ifdef _STAR_H
extern	BOOL	read_symlink	__PR((char *sname, char *name,
					FINFO *info, TCB *ptb));
extern	BOOL	last_cpio_link	__PR((FINFO *info));
extern	BOOL	xcpio_link	__PR((FINFO *info));
extern	void	flushlinks	__PR((void));
extern	BOOL	read_link	__PR((char *name, int namlen, FINFO *info,
								TCB *ptb));
#ifdef	EOF
extern	void	put_file	__PR((int *fp, FINFO *info));
#endif
extern	void	cr_file		__PR((FINFO *info,
					ssize_t (*)(void *, char *, size_t),
					void *arg, long amt, char *text));
#endif
#if defined(_SCHILY_STAT_H) && defined(_SCHILY_WALK_H)
extern	int	walkfunc	__PR((char *nm, struct stat *fs,
					int type, struct WALK *state));
#endif

/*
 * diff.c
 */
extern	void	diff		__PR((void));
#ifdef	EOF
extern	void	prdiffopts	__PR((FILE *f, char *label, int flags));
#endif

/*
 * restore.c
 */
#ifdef _STAR_H
#ifdef _RESTORE_H
extern	imap_t	*padd_node	__PR((char *name, ino_t oino, ino_t nino,
					Int32_t flags));
extern	imap_t	*sym_addrec	__PR((FINFO *info));
extern	void	sym_addstat	__PR((FINFO *info, imap_t *imp));
extern	imap_t	*sym_dirprepare	__PR((FINFO *info, imap_t *idir));
extern	imap_t	*sym_typecheck	__PR((FINFO *info, FINFO *cinfo, imap_t *imp));
#endif
#endif
extern	void	sym_initmaps	__PR((void));
extern	void	sym_open	__PR((char *name));
#ifdef _STAR_H
extern	void	sym_init	__PR((GINFO *gp));
#endif
extern	void	sym_close	__PR((void));
#ifdef	EOF
extern	void	printLsym	__PR((FILE *f));
#endif

/*
 * dirtime.c
 */
/* The prototype definitions have been moved into dirtime.h */

/*
 * extract.c
 */
#ifdef _STAR_H
extern	void	extract		__PR((char *vhname));
#ifdef	_RESTORE_H
extern	BOOL	extracti	__PR((FINFO *info, imap_t *imp));
#endif
extern	BOOL	newer		__PR((FINFO *info, FINFO *cinfo));
extern	BOOL	same_symlink	__PR((FINFO *info));
extern	BOOL	same_special	__PR((FINFO *info));
extern	BOOL	create_dirs	__PR((char *name));
extern	BOOL	make_adir	__PR((FINFO *info));
extern	BOOL	void_file	__PR((FINFO *info));
extern	int	xt_file		__PR((FINFO *info,
					ssize_t (*)(void *, char *, size_t),
					void *arg, long amt, char *text));
extern	void	skip_slash	__PR((FINFO *info));
#endif

/*
 * fifo.c
 */
#ifdef	FIFO
extern	void	initfifo	__PR((void));
extern	void	fifo_ibs_shrink	__PR((long newsize));
extern	void	runfifo		__PR((int ac, char *const *av));
extern	void	fifo_prmp	__PR((int sig));
extern	void	fifo_stats	__PR((void));
extern	size_t	fifo_amount	__PR((void));
extern	long	fifo_iwait	__PR((long amount));
extern	void	fifo_owake	__PR((long amount));
extern	void	fifo_oflush	__PR((void));
extern	void	fifo_oclose	__PR((void));
extern	long	fifo_owait	__PR((long amount));
extern	void	fifo_iwake	__PR((long amt));
extern	void	fifo_reelwake	__PR((void));
extern	void	fifo_resume	__PR((void));
extern	void	fifo_sync	__PR((long size));
extern	int	fifo_errno	__PR((void));
extern	void	fifo_onexit	__PR((int err, void *ignore));
extern	void	fifo_exit	__PR((int err));
extern	void	fifo_chitape	__PR((void));
extern	void	fifo_chotape	__PR((void));
#endif

/*
 * header.c
 */
#ifdef _STAR_H
extern	BOOL	tarsum_ok	__PR((TCB *ptb));
#ifdef	EOF
extern	void	print_hdrtype	__PR((FILE *f, int type));
#endif
extern	char	*hdr_name	__PR((int type));
extern	char	*hdr_text	__PR((int type));
extern	int	hdr_type	__PR((char *name));
extern	void	hdr_usage	__PR((void));
extern	int	get_hdrtype	__PR((TCB *ptb, BOOL isrecurse));
extern	int	get_compression	__PR((TCB *ptb));
extern	char	*get_cmpname	__PR((int type));
extern	int	get_tcb		__PR((TCB *ptb));
extern	void	put_tcb		__PR((TCB *ptb, FINFO *info));
extern	void	write_tcb	__PR((TCB *ptb, FINFO *info));
extern	void	info_to_tcb	__PR((register FINFO *info, register TCB *ptb));
extern	int	tcb_to_info	__PR((register TCB *ptb, register FINFO *info));
extern	void	stolli		__PR((register char *s, Ullong *ull));
extern	void	llitos		__PR((char *s, Ullong ull, int fieldw));
extern	void	dump_info	__PR((FINFO *info));
#endif

/*
 * volhdr.c
 */
#ifdef _STAR_H
extern	void	ginit		__PR((void));
extern	void	grinit		__PR((void));
extern	void	gipsetup	__PR((GINFO *gp));
extern	void	griprint	__PR((GINFO *gp));
extern	BOOL	verifyvol	__PR((char *buf, int amt, int volno,
					int *skipp));
extern	char	*dt_name	__PR((int type));
extern	int	dt_type		__PR((char *name));
extern	void	put_release	__PR((void));
extern	void	put_archtype	__PR((void));
extern	void	put_volhdr	__PR((char *name, BOOL putv));
extern	void	put_svolhdr	__PR((char *name));
extern	void	put_multhdr	__PR((off_t size, off_t off));
extern	BOOL	get_volhdr	__PR((FINFO *info, char *vhname));
#endif

/*
 * cpiohdr.c
 */
#ifdef _STAR_H
extern	void	put_cpioh	__PR((TCB *ptb, FINFO *info));
extern	void	cpioinfo_to_tcb	__PR((FINFO *info, TCB *ptb));
extern	int	cpiotcb_to_info	__PR((TCB *ptb, FINFO *info));
extern	int	cpio_checkswab	__PR((TCB *ptb));
#endif
extern	void	cpio_weof	__PR((void));
extern	void	cpio_resync	__PR((void));

/*
 * xheader.c
 */
#ifdef _STAR_H
extern	void	xbinit		__PR((void));
extern	void	xbbackup	__PR((void));
extern	void	xbrestore	__PR((void));
extern	size_t	xhsize		__PR((void));
extern	void	info_to_xhdr	__PR((FINFO *info, TCB *ptb));
extern	BOOL	xhparse		__PR((FINFO *info, char	*p, char *ep));
extern	void	xh_rangeerr	__PR((char *keyword, char *arg, size_t len));
extern	void	gen_xtime	__PR((char *keyword, time_t sec, Ulong nsec));
extern	void	gen_unumber	__PR((char *keyword, Ullong arg));
extern	void	gen_number	__PR((char *keyword, Llong arg));
extern	void	gen_text	__PR((char *keyword, char *arg, size_t alen,
								Uint flags));

extern	void	tcb_to_xhdr_reset __PR((void));
extern	int	tcb_to_xhdr	__PR((TCB *ptb, FINFO *info));

extern	BOOL	get_xtime	__PR((char *keyword, char *arg, size_t len,
						time_t *secp, long *nsecp));
#ifdef	__needed_
extern	BOOL	get_number	__PR((char *keyword, char *arg, Llong *llp));
#endif
extern	BOOL	get_unumber	__PR((char *keyword, char *arg, Ullong *ullp,
					Ullong maxval));
extern	BOOL	get_snumber	__PR((char *keyword, char *arg, Ullong *ullp,
					BOOL *negp,
					Ullong minval, Ullong maxval));
#endif

/*
 * xattr.c
 */
#ifdef _STAR_H
extern	void	opt_xattr	__PR((void));
extern	void	opt_selinux	__PR((void));
extern	BOOL	get_xattr	__PR((register FINFO *info));
extern	BOOL	set_xattr	__PR((register FINFO *info));
#ifdef	USE_SELINUX
extern	BOOL	setselinux	__PR((register FINFO *info));
#endif
extern	void	free_xattr	__PR((star_xattr_t **xattr));
#endif

/*
 * hole.c
 */
#ifdef _STAR_H
#ifdef	EOF
extern	int	get_forced_hole	__PR((FILE *f, FINFO *info));
extern	int	get_sparse	__PR((FILE *f, FINFO *info));
extern	int	get_as_hole	__PR((FILE *f, FINFO *info));
extern	BOOL	cmp_sparse	__PR((FILE *f, FINFO *info));
extern	void	put_sparse	__PR((int *fp, FINFO *info));
extern	BOOL	sparse_file	__PR((int *fp, FINFO *info));
#endif
extern	int	gnu_skip_extended	__PR((FINFO *info));
#endif

/*
 * lhash.c
 */
extern	size_t	hash_size	__PR((size_t size));
#ifdef	EOF
extern	void	hash_build	__PR((FILE *fp));
extern	void	hash_xbuild	__PR((FILE *fp));
#endif
extern	BOOL	hash_lookup	__PR((char *str));
extern	BOOL	hash_xlookup	__PR((char *str));

/*
 * list.c
 */
#ifdef _STAR_H
extern	void	list	__PR((void));
extern	void	list_file __PR((register FINFO *info));
extern	void	vprint	__PR((FINFO *info));
#endif

/*
 * longnames.c
 */
#ifdef _STAR_H
extern	BOOL	name_to_tcb	__PR((FINFO *info, TCB *ptb));
extern	void	tcb_to_name	__PR((TCB *ptb, FINFO *info));
extern	void	tcb_undo_split	__PR((TCB *ptb, FINFO *info));
extern	int	tcb_to_longname	__PR((register TCB *ptb, register FINFO *info));
extern	void	write_longnames	__PR((register FINFO *info));
#endif

/*
 * props.c
 */
extern	void	setprops	__PR((long htype));
extern	void	printprops	__PR((void));

/*
 * remove.c
 */
extern	BOOL	remove_file	__PR((char *name, BOOL isfirst));

/*
 * star_unix.c
 */
#ifdef _STAR_H
extern	BOOL	_getinfo	__PR((char *name, FINFO *info));
extern	BOOL	_lgetinfo	__PR((char *name, FINFO *info));
extern	BOOL	getinfo		__PR((char *name, FINFO *info));
#ifdef	HAVE_FSTATAT
extern	BOOL	getinfoat	__PR((int fd, char *name, FINFO *info));
#endif
#ifdef	_SCHILY_STAT_H
extern	BOOL	getstat		__PR((char *name, struct stat *sp));
extern	BOOL	stat_to_info	__PR((int fd, struct stat *sp, FINFO *info));
#endif
#ifdef	EOF
extern	void	checkarch	__PR((FILE *f));
extern	BOOL	samefile	__PR((FILE *fp1, FILE *fp2));
#endif
extern	BOOL	archisnull	__PR((const char *name));
extern	void	setmodes	__PR((FINFO *info));
extern	int	snulltimes	__PR((char *name, FINFO *info));
extern	int	sxsymlink	__PR((char *name, FINFO *info));
extern	int	rs_acctime	__PR((int fd, FINFO *info));
extern	void	setdirmodes	__PR((char *name, mode_t mode));
extern	mode_t	osmode		__PR((mode_t tarmode));

/*
 * lpath_unix.c
 */
extern	int	lchdir		__PR((char *name));
extern	char	*lgetcwd	__PR((void));
extern	int	lmkdir		__PR((char *name, mode_t mode));
extern	int	laccess		__PR((char *name, int amode));
#ifdef	_SCHILY_STAT_H
extern	int	lstatat		__PR((char *name, struct stat *buf, int flag));
#endif
extern	int	lchmodat	__PR((char *name, mode_t mode, int flag));
#ifdef	_SCHILY_TIME_H
extern	int	lutimensat	__PR((char *name, struct timespec *ts,
					int flag));
#endif
extern	int	lreadlink	__PR((char *name, char *buf, size_t _bufsize));
extern	int	lsymlink	__PR((char *name, char *name2));
extern	int	llink		__PR((char *name, char *name2));
extern	int	lrename		__PR((char *name, char *name2));
extern	int	lmknod		__PR((char *name, mode_t mode, dev_t dev));
extern	int	lmkfifo		__PR((char *name, mode_t mode));
extern	int	lchownat	__PR((char *name, uid_t uid, gid_t gid,
					int flag));
extern	int	lunlinkat	__PR((char *name, int flag));
extern	char	*lmktemp	__PR((char *name));
#ifdef	EOF
extern	FILE	*lfilemopen	__PR((char *name, char *mode, mode_t cmode));
#endif
extern	int	_lfileopen	__PR((char *name, char *mode));
#ifdef	_SCHILY_DIRENT_H
EXPORT	DIR	*lopendir	__PR((char *name));
#endif
#endif	/* _STAR_H */
EXPORT	int	hop_dirs	__PR((char *name, char **np));

/*
 * acl_unix.c
 */
extern	void	opt_acl		__PR((void));
#ifdef _STAR_H
extern	BOOL	get_acls	__PR((FINFO *info));
extern	void	set_acls	__PR((FINFO *info));
#endif

/*
 * unicode.c
 */
extern	void	utf8_init	__PR((int type));
extern	void	utf8_fini	__PR((void));
extern	size_t	to_utf8		__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t len));
extern	BOOL	from_utf8	__PR((Uchar *to, size_t tolen,
					Uchar *from, size_t *len));

/*
 * fflags.c
 */
extern	void	opt_fflags	__PR((void));
#ifdef _STAR_H
extern	void	get_fflags	__PR((FINFO *info));
extern	void	set_fflags	__PR((FINFO *info));
extern	char	*textfromflags	__PR((FINFO *info, char *buf));
extern	int	texttoflags	__PR((FINFO *info, char *buf));
#endif

/*
 * defaults.c
 */
extern	char	*get_stardefaults __PR((char *name));
extern	void	star_defaults	__PR((long *fsp, BOOL *no_fsyncp,
						BOOL *secure_linkp,
						char *dfltname));
extern	BOOL	star_darchive	__PR((char *arname, char *dfltname));
#if defined(HAVE_FORK) && !defined(__DJGPP__)
extern	char	**get_args_for_helper	__PR((char *alg, char *section,
						char *dfltflg, char *xtraflg));
#endif

/*
 * subst.c
 */
extern	int	paxpsubst	__PR((char *cmd, BOOL *arg));
extern	int	parsesubst	__PR((char *cmd, BOOL *arg));
#ifdef _STAR_H
extern	BOOL	subst		__PR((FINFO *info));
extern	BOOL	ia_change	__PR((TCB *ptb, FINFO *info));
#endif

/*
 * findinfo.c
 */
#ifdef _STAR_H
EXPORT	BOOL	findinfo	__PR((FINFO *info));
#endif

/*
 * paxopts.c
 */
extern	int	ppaxopts	__PR((const char *opts));
