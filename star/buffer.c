/* @(#)buffer.c	1.169 17/01/26 Copyright 1985, 1995, 2001-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)buffer.c	1.169 17/01/26 Copyright 1985, 1995, 2001-2017 J. Schilling";
#endif
/*
 *	Buffer handling routines
 *
 *	Copyright (c) 1985, 1995, 2001-2017 J. Schilling
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
#include <schily/fcntl.h>
#include <schily/ioctl.h>
#include <schily/varargs.h>
#include "star.h"
#include "props.h"
#include <schily/errno.h>
#include <schily/standard.h>
#include "fifo.h"
#include <schily/string.h>
#include <schily/wait.h>
#include <schily/mtio.h>
#include <schily/librmt.h>
#include <schily/schily.h>
#include "starsubs.h"

#include <schily/io.h>		/* for setmode() prototype */
#include <schily/libport.h>	/* getpagesize() */

long	bigcnt	= 0;
int	bigsize	= 0;		/* Tape block size (may shrink < bigbsize) */
int	bigbsize = 0;		/* Big buffer size */
int	bufsize	= 0;		/* Available buffer size */
char	*bigbase = NULL;
char	*bigbuf	= NULL;
char	*bigptr	= NULL;
char	*eofptr	= NULL;
Llong	curblockno;

m_stats	bstat;
m_stats	*stats	= &bstat;
int	pid;

#ifdef	timerclear
LOCAL	struct	timespec	starttime;
LOCAL	struct	timespec	stoptime;
#endif

LOCAL	BOOL	isremote = FALSE;
LOCAL	int	remfd	= -1;
LOCAL	char	*remfn;

#ifdef __DJGPP__
LOCAL	FILE	*compress_tarf_save;	/* Old value of tarf	 */
LOCAL	FILE	*compress_tmpf = NULL;	/* FILE * from tmpfile() */
#endif

extern	FILE	*tarf;
extern	FILE	*tty;
extern	FILE	*vpr;
extern	char	*tarfiles[];
extern	int	ntarfiles;
extern	int	tarfindex;
extern	BOOL	force_noremote;
LOCAL	int	lastremote = -1;
extern	BOOL	multivol;
extern	char	*newvol_script;
extern	BOOL	use_fifo;
extern	int	swapflg;
extern	int	cmptype;
extern	BOOL	debug;
extern	BOOL	print_artype;
extern	BOOL	silent;
extern	BOOL	showtime;
extern	BOOL	no_stats;
extern	BOOL	cpio_stats;
extern	BOOL	do_fifostats;
extern	BOOL	cflag;
extern	BOOL	uflag;
extern	BOOL	rflag;
extern	BOOL	copyflag;
extern	BOOL	Zflag;
extern	BOOL	zflag;
extern	BOOL	bzflag;
extern	BOOL	lzoflag;
extern	BOOL	p7zflag;
extern	BOOL	xzflag;
extern	BOOL	lzipflag;
extern	char	*compress_prg;
extern	BOOL	multblk;
extern	BOOL	partial;
extern	BOOL	wready;
extern	BOOL	nullout;
extern	Ullong	tsize;
extern	BOOL	nowarn;

extern	int	intr;

extern	GINFO	*gip;


EXPORT	void	opt_remote	__PR((void));
EXPORT	BOOL	openremote	__PR((void));
EXPORT	void	opentape	__PR((void));
EXPORT	void	closetape	__PR((void));
EXPORT	void	changetape	__PR((BOOL donext));
EXPORT	void	nextitape	__PR((void));
EXPORT	void	nextotape	__PR((void));
EXPORT	int	startvol	__PR((char *buf, int amount));
EXPORT	void	newvolhdr	__PR((char *buf, int amount, BOOL do_fifo));
#ifdef	FIFO
LOCAL	void	fbit_ffss	__PR((bitstr_t *name, int startb, int stopb, int *value));
LOCAL	BOOL	fifo_hpos	__PR((char *buf, off_t *posp));
#endif
EXPORT	void	initbuf		__PR((int nblocks));
EXPORT	void	markeof		__PR((void));
EXPORT	void	syncbuf		__PR((void));
EXPORT	int	peekblock	__PR((char *buf, int amount));
EXPORT	int	readblock	__PR((char *buf, int amount));
LOCAL	int	readtblock	__PR((char *buf, int amount));
LOCAL	void	readbuf		__PR((void));
EXPORT	int	readtape	__PR((char *buf, int amount));
EXPORT	void	filltcb		__PR((TCB *ptb));
EXPORT	void	movetcb		__PR((TCB *from_ptb, TCB *to_ptb));
EXPORT	void	*get_block	__PR((int amount));
EXPORT	void	put_block	__PR((int amount));
EXPORT	char	*writeblock	__PR((char *buf));
EXPORT	int	writetape	__PR((char *buf, int amount));
LOCAL	void	writebuf	__PR((int amount));
LOCAL	void	flushbuf	__PR((void));
EXPORT	void	writeempty	__PR((void));
EXPORT	void	weof		__PR((void));
EXPORT	void	buf_sync	__PR((int size));
EXPORT	void	buf_drain	__PR((void));
EXPORT	int	buf_wait	__PR((int amount));
EXPORT	void	buf_wake	__PR((int amount));
EXPORT	int	buf_rwait	__PR((int amount));
EXPORT	void	buf_rwake	__PR((int amount));
EXPORT	void	buf_resume	__PR((void));
EXPORT	void	backtape	__PR((void));
EXPORT	int	mtioctl		__PR((int cmd, int count));
EXPORT	off_t	mtseek		__PR((off_t offset, int whence));
EXPORT	void	marktcb		__PR((char *addr));
EXPORT	Llong	tblocks		__PR((void));
EXPORT	void	prstats		__PR((void));
EXPORT	BOOL	checkerrs	__PR((void));
EXPORT	void	exprstats	__PR((int ret));
EXPORT	void	excomerrno	__PR((int err, char *fmt, ...)) __printflike__(2, 3);
EXPORT	void	excomerr	__PR((char *fmt, ...)) __printflike__(1, 2);
EXPORT	void	die		__PR((int err));
LOCAL	void	compressopen	__PR((void));
LOCAL	void	compressclose	__PR((void));

EXPORT void
opt_remote()
{
#ifdef	USE_REMOTE
	printf(" remote");
#endif
}

/*
 * Check whether the current tarfiles[tarfindex] refers to a remote archive
 * location and open a remote connection if needed.
 * Called from star.c main() and from changetape().
 */
EXPORT BOOL
openremote()
{
	char	host[128];
	char	lasthost[128];

	if ((!nullout || (uflag || rflag)) && !force_noremote &&
			(remfn = rmtfilename(tarfiles[tarfindex])) != NULL) {

#ifdef	USE_REMOTE
		isremote = TRUE;
		rmtdebug(debug);
		rmthostname(host, sizeof (host), tarfiles[tarfindex]);
		if (debug)
			errmsgno(EX_BAD, "Remote: %s Host: %s file: %s\n",
					tarfiles[tarfindex], host, remfn);

		if (lastremote >= 0) {
			rmthostname(lasthost, sizeof (lasthost),
							tarfiles[lastremote]);
			if (!streql(host, lasthost)) {
				close(remfd);
				remfd = -1;
				lastremote = -1;
			}
		}
		if (remfd < 0 && (remfd = rmtgetconn(host, bigsize, 0)) < 0)
			comerrno(EX_BAD, "Cannot get connection to '%s'.\n",
				/* errno not valid !! */		host);
		lastremote = tarfindex;
#else
		comerrno(EX_BAD, "Remote tape support not present.\n");
#endif
	} else {
		isremote = FALSE;
	}
	return (isremote);
}

/*
 * Open the current tarfiles[tarfindex] (a remote connection must be open)
 * Called from star.c main() and from changetape().
 */
EXPORT void
opentape()
{
	int	n = 0;
	extern	dev_t	tape_dev;
	extern	ino_t	tape_ino;
	extern	BOOL	tape_isreg;

	if (copyflag || (nullout && !(uflag || rflag))) {
		tarfiles[tarfindex] = "null";
		tarf = (FILE *)NULL;
	} else if (streql(tarfiles[tarfindex], "-")) {
		if (cflag) {
			tarf = stdout;
		} else {
			tarf = stdin;
			multblk = TRUE;
		}
		setbuf(tarf, (char *)NULL);
		setmode(fileno(tarf), O_BINARY);
	} else if (isremote) {
#ifdef	USE_REMOTE
		/*
		 * isremote will always be FALSE if USE_REMOTE is not defined.
		 * NOTE any open flag bejond O_RDWR is not portable across
		 * different platforms. The remote tape library will check
		 * whether the current /etc/rmt server supports symbolic
		 * open flags. If there is no symbolic support in the
		 * remote server, our rmt client code will mask off all
		 * non portable bits. The remote rmt server defaults to
		 * O_BINARY as the client (we) may not know about O_BINARY.
		 * XXX Should we add an option that allows to specify O_TRUNC?
		 */
		while (rmtopen(remfd, remfn, (cflag ? O_RDWR|O_CREAT:O_RDONLY)|O_BINARY) < 0) {
			if (!wready || n++ > 12 ||
			    (geterrno() != EIO && geterrno() != EBUSY)) {
				comerr("Cannot open remote '%s'.\n",
						tarfiles[tarfindex]);
			} else {
				sleep(10);
			}
		}
#endif
	} else {
		FINFO	finfo;
	extern	BOOL	follow;
	extern	BOOL	fcompat;
	extern	int	ptype;

		if (fcompat && (cflag && !(uflag || rflag))) {
			/*
			 * The old syntax has a high risk of corrupting
			 * files if the user disorders the args.
			 * For this reason, we do not allow to overwrite
			 * a plain file in compat mode.
			 * XXX What if we implement 'r' & 'u' ???
			 */
			follow++;
			n = _getinfo(tarfiles[tarfindex], &finfo);
			follow--;
			if (n >= 0 && is_file(&finfo) && finfo.f_size > (off_t)0) {
				if (ptype != P_SUNTAR) {
					comerrno(EX_BAD,
					"Will not overwrite non empty plain files in compat mode.\n");
				} else {
					errmsgno(EX_BAD,
					"WARNING: Overwriting archive file '%s'.\n",
					tarfiles[tarfindex]);
				}
			}
		}

		n = 0;
		/*
		 * XXX Should we add an option that allows to specify O_TRUNC?
		 */
		while ((tarf = fileopen(tarfiles[tarfindex],
						cflag?"rwcub":"rub")) ==
								(FILE *)NULL) {
			if (!wready || n++ > 12 ||
			    (geterrno() != EIO && geterrno() != EBUSY)) {
				comerr("Cannot open '%s'.\n",
						tarfiles[tarfindex]);
			} else {
				sleep(10);
			}
		}
	}
	if (tarf != (FILE *)NULL && isatty(fdown(tarf)))
		comerrno(EX_BAD, "Archive cannot be a tty.\n");
	if (!isremote && (!nullout || (uflag || rflag)) &&
	    tarf != (FILE *)NULL) {
		file_raise(tarf, FALSE);
		checkarch(tarf);
	}
	vpr = tarf == stdout ? stderr : stdout;	/* f=stdout redirect listing */
	if (samefile(tarf, vpr)) {		/* Catch -f /dev/stdout case */
		if (tarf != stdin)		/* Don't redirect for -tv <  */
			vpr = stderr;
	}

	/*
	 * If the archive is a plain file and thus seekable
	 * do automatic compression detection.
	 */
	if (stats->volno == 1 &&
	    tape_isreg && !cflag && (!Zflag && !zflag && !bzflag && !lzoflag &&
	    !p7zflag && !xzflag && !compress_prg)) {
		long	htype;
		TCB	*ptb;

		readtblock(bigbuf, TBLOCK);
		ptb = (TCB *)bigbuf;
		htype = get_hdrtype(ptb, FALSE);

		if (htype == H_UNDEF) {
			switch (cmptype = get_compression(ptb)) {

			case C_NONE:
				break;
			case C_PACK:
			case C_GZIP:
			case C_LZW:
			case C_FREEZE:
			case C_LZH:
			case C_PKZIP:
				if (!silent && !print_artype) errmsgno(EX_BAD,
					"WARNING: Archive is '%s' compressed, trying to use the -z option.\n",
						get_cmpname(cmptype));
				zflag = TRUE;
				break;
			case C_BZIP2:
				if (!silent && !print_artype) errmsgno(EX_BAD,
					"WARNING: Archive is 'bzip2' compressed, trying to use the -bz option.\n");
				bzflag = TRUE;
				break;
			case C_LZO:
				if (!silent) errmsgno(EX_BAD,
					"WARNING: Archive is 'lzop' compressed, trying to use the -lzo option.\n");
				lzoflag = TRUE;
				break;
			case C_7Z:
				if (!silent) errmsgno(EX_BAD,
					"WARNING: Archive is '7z' compressed, trying to use the -7z option.\n");
				p7zflag = TRUE;
				break;
			case C_XZ:
				if (!silent) errmsgno(EX_BAD,
					"WARNING: Archive is 'xz' compressed, trying to use the -xz option.\n");
				xzflag = TRUE;
				break;
			case C_LZIP:
				if (!silent) errmsgno(EX_BAD,
					"WARNING: Archive is 'lzip' compressed, trying to use the -lzip option.\n");
				lzipflag = TRUE;
				break;
			default:
				if (!silent) errmsgno(EX_BAD,
					"WARNING: Unknown compression type %d.\n", cmptype);
				break;
			}
		}
		mtseek((off_t)0, SEEK_SET);
	}
	if (Zflag || zflag || bzflag || lzoflag ||
	    p7zflag || xzflag || lzipflag ||
	    compress_prg) {
		if (isremote)
			comerrno(EX_BAD, "Cannot compress remote archives (yet).\n");
		/*
		 * If both values are zero, this is a device and thus may be a tape.
		 */
		if (tape_dev || tape_ino)
			compressopen();
		else
			comerrno(EX_BAD, "Can only compress files.\n");
	}

#ifdef	timerclear
	if (showtime && starttime.tv_sec == 0 && starttime.tv_nsec == 0 &&
	    getnstimeofday(&starttime) < 0)
		comerr("Cannot get starttime\n");
#endif
}

/*
 * Close the current open tarf/remfd (a remote connection must be open)
 * Called from star.c main() and from changetape().
 */
EXPORT void
closetape()
{
	if (isremote) {
#ifdef	USE_REMOTE
		/*
		 * isremote will always be FALSE if USE_REMOTE is not defined.
		 */
		if (rmtclose(remfd) < 0)
			errmsg("Remote close failed.\n");
#endif
	} else {
		compressclose();
		if (tarf)
			fclose(tarf);
	}
}

/*
 * Low level medium change routine.
 * Called from nextitape()/nextotape() and fifo_owait().
 */
EXPORT void
changetape(donext)
	BOOL	donext;
{
	char	ans[2];
	int	nextindex;

	if (donext) {
		pid_t	opid = pid;

		if (pid == 0)
			pid = 1; /* Make sure the statistics are printed */
		prstats();
		pid = opid;
		if (!cflag &&
		    (gip->tapesize > 0) &&
		    (stats->blocks*stats->nblocks + stats->parts/TBLOCK) !=
							    gip->tapesize) {
			errmsgno(EX_BAD,
			"WARNING: Archive size error.\n");
			errmsgno(EX_BAD,
			"Expected size %llu blocks, actual size %lld blocks.\n",
			gip->tapesize,
			stats->blocks*stats->nblocks + stats->parts/TBLOCK);
		}
		stats->Tblocks += stats->blocks;
		stats->Tparts += stats->parts;
	}
	stats->blocks = 0L;
	stats->parts = 0L;
	closetape();
	/*
	 * XXX Was passiert, wenn wir das 2. Mal bei einem Band vorbeikommen?
	 * XXX Zur Zeit wird gnadenlos ueberschrieben.
	 */
	if (donext) {
		stats->volno++;
		gip->volno = stats->volno;
		nextindex = tarfindex + 1;
		if (nextindex >= ntarfiles)
			nextindex = 0;
	} else {
		nextindex = tarfindex;
	}
	/*
	 * XXX We need to add something like the -l & -o option from
	 * XXX ufsdump.
	 */
	if (newvol_script) {
		char	scrbuf[PATH_MAX];

		fflush(vpr);
		if (!donext) {
			errmsgno(EX_BAD,
			"Mounted volume on '%s' did not match archive",
				tarfiles[nextindex]);
			comerrno(EX_BAD, "Aborting.\n");
		}
		/*
		 * The script is called with the next volume # and volume name
		 * as argument.
		 */
		js_snprintf(scrbuf, sizeof (scrbuf), "%s '%d' '%s'",
				newvol_script,
				stats->volno, tarfiles[nextindex]);
		system(scrbuf);
	} else {
		errmsgno(EX_BAD, "Mount volume #%d on '%s' and hit <RETURN>",
			stats->volno, tarfiles[nextindex]);
		fgetline(tty, ans, sizeof (ans));
		if (feof(tty))
			exit(1);
	}
	tarfindex = nextindex;
	openremote();
	opentape();
}

/*
 * High level input medium change routine.
 * Currently called from buf_rwait().
 * Volume verification in the fifo case is done in the fifo process.
 * For this reason, we only verify the new volume in the non fifo case.
 */
EXPORT void
nextitape()
{
#ifdef	FIFO
	if (use_fifo) {
		fifo_chitape();
	} else
#endif
	{
		int	skip;

		changetape(TRUE);
		readbuf();
		while (bigcnt > 0 && !verifyvol(bigptr, bigcnt, stats->volno, &skip)) {
			changetape(FALSE);
			readbuf();
		}
		if (skip > 0)
			buf_rwake(skip*TBLOCK);
	}
	if (intr)
		exit(2);
}

/*
 * High level output medium change routine.
 * Currently called from write_tcb() and only used if
 * -multivol has not been specified. So this is always called
 * from the tar process and never from the fifo background process.
 */
EXPORT void
nextotape()
{
	weof();
#ifdef	FIFO
	if (use_fifo) {
		fifo_chotape();
	} else
#endif
	changetape(TRUE);
	if (intr)
		exit(2);
}

/*
 * Called from writetape()
 */
EXPORT int
startvol(buf, amount)
	char	*buf;		/* The original buffer address		*/
	int	amount;		/* The related requested transfer count	*/
{
	char	*obuf = bigbuf;
	char	*optr = bigptr;
	long	ocnt = bigcnt;
	long	xcnt = 0;
	BOOL	ofifo = use_fifo;
static	BOOL	active = FALSE;	/* If TRUE: We are already in a media change */
extern	m_head	*mp;

	if (amount <= 0)
		return (amount);
	if (active)
		comerrno(EX_BAD, "Panic: recursive media change requested!\n");
	if (amount > bigsize) {
		comerrno(EX_BAD,
		"Panic: trying to write more than bs (%d > %d)!\n",
		amount, bigsize);
	}
	mp->chreel = TRUE;
#ifdef	FIFO
	if (use_fifo) {
		/*
		 * Make sure the put side of the FIFO is waiting either on
		 * mp->iblocked (because the FIFO is full) or on mp->reelwait
		 * before temporary disabling the FIFO during media change.
		 */
		while (mp->iblocked == FALSE && mp->reelwait == FALSE) {
			usleep(100000);
		}
	}
#endif
	active = TRUE;

	/*
	 * Save the current write data in "bigbase".
	 */
	movebytes(buf, bigbase, amount);

	use_fifo = FALSE;
	bigbuf = &bigbase[bigsize];
	bigptr = bigbuf;
	bigcnt = 0;

	newvolhdr(buf, amount, ofifo);

	if (bigcnt > 0) {	/* We did create a volhdr */
		xcnt = bigsize - bigcnt;
		if (amount < xcnt)
			xcnt = amount;
		if (xcnt > 0) {
			/*
			 * Move data from original buffer past the volhdr to
			 * fill up a complete block size.
			 */
			movebytes(bigbase, bigptr, xcnt);
			bigcnt += xcnt;
		}
		writetape(bigbuf, bigcnt);
	}

	movebytes(bigbase, buf, amount);
	bigbuf = obuf;
	bigptr = optr;
	bigcnt = ocnt;
	use_fifo = ofifo;
	active = FALSE;
	mp->chreel = FALSE;
#ifdef	FIFO
	if (use_fifo) {
		fifo_reelwake();
	}
#endif
	return (xcnt);		/* Return the amount taken from orig. buffer */
}

EXPORT void
newvolhdr(buf, amount, do_fifo)
	char	*buf;		/* The original buffer address		*/
	int	amount;		/* The related requested transfer count	*/
	BOOL	do_fifo;
{
extern	m_head	*mp;
	off_t	scur_size;
	off_t	scur_off;
	off_t	sold_size;
	off_t	sold_off;
#ifdef	FIFO
	off_t	new_size = 0;
	BOOL	nsize_valid = FALSE;
#endif

	fifo_lock_critical();
	scur_size = stats->cur_size;
	scur_off  = stats->cur_off;
	sold_size = stats->old_size;
	sold_off  = stats->old_off;
#ifdef	FIFO
	/*
	 * If needed, find next header position in FIFO bitmap.
	 */
	if (do_fifo && buf != NULL)	/* buf == NULL -> called from put_tcb */
		nsize_valid = fifo_hpos(buf, &new_size);
#endif
	fifo_unlock_critical();

	xbbackup();		/* Save current xheader data */

	gip->blockoff = stats->Tblocks * stats->nblocks +
			stats->Tparts / TBLOCK;

	put_release();		/* Pax 'g' vendor unique */
	put_archtype();		/* Pax 'g' vendor unique */

#ifdef	FIFO
	if (do_fifo && buf != NULL) {	/* buf == NULL -> called from put_tcb */
		off_t	new_off = 0;

		if (!nsize_valid) {
			new_size = scur_size;
			new_off  = scur_off - FIFO_AMOUNT(mp);
		}
		/*
		 * Write a 'g'-header and either a 'V'-header
		 * or a 'M'-header.
		 */
		put_volhdr(gip->label, new_size <= 0);
		if (new_size > 0)
			put_multhdr(new_size, new_off);
	} else
#endif
	/*
	 * Write a 'g'-header and either a 'V'-header
	 * or a 'M'-header.
	 */
	if (!do_fifo && (sold_off < sold_size)) {
		put_volhdr(gip->label, FALSE);
		put_multhdr(sold_size, sold_off);
	} else {
		put_volhdr(gip->label, TRUE);
	}

	xbrestore();		/* Restore current xheader data */
	stats->cur_size = scur_size;
	stats->cur_off  = scur_off;
	stats->old_size = sold_size;
	stats->old_off  = sold_off;
}

#ifdef	FIFO
/*
 * Make the macro a function...
 */
LOCAL void
fbit_ffss(name, startb, stopb, value)
	register bitstr_t *name;
	register int	startb;
	register int	stopb;
	register int	*value;
{
	bit_ffss(name, startb, stopb, value);
}

/*
 * Find next header position in FIFO bitmap.
 */
LOCAL BOOL
fifo_hpos(buf, posp)
	char	*buf;
	off_t	*posp;
{
		int	startb;
		int	stopb;
		int	endb;
		int	bitpos = -1;
	extern	m_head	*mp;

	startb = (buf - mp->base) / TBLOCK;
	stopb  = -1 + (mp->putptr - mp->base) / TBLOCK;
	endb   = -1 + (mp->size) / TBLOCK;

	if (buf < mp->base) {
		stopb = mp->bmlast;
		startb = stopb + 1 - (mp->base - buf) / TBLOCK;

		fbit_ffss(mp->bmap, startb, stopb, &bitpos);
		if (bitpos >= 0) {
			*posp = (bitpos - startb) * TBLOCK;
			return (TRUE);
		}
		startb = 0;
		stopb = -1 + (mp->putptr - mp->base) / TBLOCK;
	}
	if (stopb < startb)
		fbit_ffss(mp->bmap, startb, endb, &bitpos);
	else
		fbit_ffss(mp->bmap, startb, stopb, &bitpos);
	if (bitpos >= 0) {
		*posp = (bitpos - startb) * TBLOCK;
		return (TRUE);
	}
	if (stopb < startb) {
		fbit_ffss(mp->bmap, 0, stopb, &bitpos);
		if (bitpos >= 0) {
			/*
			 * endb+1 - startb == # of bits in rear part
			 */
			*posp = (bitpos + endb+1 - startb) * TBLOCK;
			return (TRUE);
		}
	}
	return (FALSE);
}
#endif

/*
 * Init buffer or fifo.
 * called from star.c main().
 */
EXPORT void
initbuf(nblocks)
	int	nblocks;
{
	BOOL	cvolhdr = cflag && (multivol || tsize > 0);

	pid = getpid();
	bufsize = bigsize = nblocks * TBLOCK;
#ifdef	FIFO
	if (use_fifo) {
		initfifo();
	}
#endif
	/*
	 * As bigbuf is allocated here only in case that we have no FIFO or we
	 * are in create mode, there are no aliasing problems with bigbuf and
	 * the shared memory in the FIFO while trying to detect the archive
	 * format and byte swapping in read/extract modes.
	 * Note that -r and -u currently disable the FIFO.
	 * In case that we enable the FIFO for -r and -u, we need to add
	 * another exception here in order to have space to remember the last
	 * block of significant data that needs to be modified to append.
	 */
	if (!use_fifo || cvolhdr || rflag || uflag) {
		int	pagesize = getpagesize();

		/*
		 * llitos() overshoots by one space (' ') in cpio mode,
		 * add 10 bytes.
		 * If we create multi volume archives that need volume
		 * headers, we need additional space to prepare the
		 * first write to a new medium after a medium change.
		 * "bufsize" may be modified by initfifo() in the FIFO case,
		 * so we use "bigsize" for the extra multivol buffer to
		 * avoid allocating an unneeded huge amount of data here.
		 * In "replace" or "update" mode, we also may need to
		 * save/restore * the buffer for the tape record when doing
		 * EOF detection. As this space is needed at a different
		 * time, it may be shared with the extra multivol buffer.
		 */
		if (cvolhdr || rflag || uflag)
			bigsize *= 2;

		/*
		 * roundup(x, y), x needs to be unsigned or x+y non-genative.
		 */
#undef	roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

		bigptr = bigbuf = ___malloc((size_t)bigsize+10+pagesize,
								"buffer");
		bigptr = bigbuf = (char *)roundup((UIntptr_t)bigptr, pagesize);
		fillbytes(bigbuf, bigsize, '\0');
		fillbytes(&bigbuf[bigsize], 10, 'U');

		if (cvolhdr || rflag || uflag) {
			bigsize /= 2;
			bigbase = bigbuf;
			bigbuf = bigptr = &bigbase[bigsize];
		}
	}
	stats->nblocks = nblocks;
	stats->blocksize = bigbsize = bigsize;
	stats->volno = 1;
	stats->swapflg = -1;
}

/*
 * Mark the EOF position (the position of the first logical TAR EOF block)
 * We need this position for later repositioning when appending to an archive.
 */
EXPORT void
markeof()
{
#ifdef	FIFO
	if (use_fifo) {
		/*
		 * Remember current FIFO status.
		 */
		/* EMPTY */
	}
#endif
	eofptr = bigptr - TBLOCK;

	if (debug) {
		error("Blocks: %lld\n", tblocks());
		error(
		"bigptr - bigbuff: %lld bigbuf: %p bigptr: %p eofptr: %p lastsize: %ld\n",
			(Llong)(bigptr - bigbuf),
			(void *)bigbuf, (void *)bigptr, (void *)eofptr,
			stats->lastsize);
	}
}

EXPORT void
marktcb(addr)
	char	*addr;
{
#ifdef	FIFO
	extern	m_head  *mp;
	register int	bit;
#endif
	if (!multivol || !use_fifo)
		return;
	/*
	 * As long as we don't start supporting -multivol with CPIO archives,
	 * we will never come here when writing unblocked archives.
	 */
#ifdef	FIFO
	bit = addr - mp->base;
	if (bit % TBLOCK)		/* Remove this paranoia test in future. */
		error("TCB offset not mudulo 512.\n");
	bit /= TBLOCK;
	if (bit_test(mp->bmap, bit))	/* Remove this paranoia test in future. */
		error("Bit %d is already set.\n", bit);
	bit_set(mp->bmap, bit);
#endif
}

/*
 * Prepare the buffer/fifo for reversing the direction from reading to writing.
 * Called from star.c main() after skipall() is ready.
 */
EXPORT void
syncbuf()
{
#ifdef	FIFO
	if (use_fifo) {
		/*
		 * Switch FIFO direction.
		 */
		excomerr("Cannot update tape with FIFO.\n");
	}
#endif
	if (eofptr) {
		/*
		 * Only back up to "eofptr" if markeof() has been called,
		 * this is not the case when we did ancounter a hard EOF
		 * at the beginning of the archive (empty file),
		 */
		bigptr = eofptr;
		bigcnt = eofptr - bigbuf;
	}
}

/*
 * Peek into buffer for amount bytes.
 * Return at most TBLOCK (512) bytes.
 *
 * Called from get_tcb() for checking the archive format of the first
 * tape block.
 */
EXPORT int
peekblock(buf, amount)
	register char	*buf;
	register int	amount;
{
	register int	n;

	if ((n = buf_rwait(amount)) == 0)
		return (EOF);
	if (n > amount)
		n = amount;
	if (n >= TBLOCK) {
		n = TBLOCK;
		movetcb((TCB *)bigptr, (TCB *)buf);
	} else {
		movebytes(bigptr, buf, n);
	}
	return (n);
}

/*
 * Read amount bytes.
 * Return at most TBLOCK (512) bytes.
 * Do CPIO buffer wrap handling here.
 *
 * Called from get_tcb() and from the sparse handling functions in hole.c
 */
EXPORT int
readblock(buf, amount)
	register char	*buf;
	register int	amount;
{
	register int	n;

	if ((n = peekblock(buf, amount)) != EOF) {
		buf_rwake(n);
		if (n < amount) {
			if ((amount = readblock(&buf[n], amount-n)) == EOF)
				return (n);
			return (n + amount);
		}
	}
	return (n);
}

/*
 * Low level routine to read a TAPE Block (usually 10k)
 * Called from opentape() to check the compression and from readtape().
 */
LOCAL int
readtblock(buf, amount)
	char	*buf;
	int	amount;
{
	int	cnt;

	stats->reading = TRUE;
	if (isremote) {
#ifdef	USE_REMOTE
		/*
		 * isremote will always be FALSE if USE_REMOTE is not defined.
		 */
		if ((cnt = rmtread(remfd, buf, amount)) < 0)
			excomerr("Error reading '%s'.\n", tarfiles[tarfindex]);
#endif
	} else {
		if ((cnt = _niread(fileno(tarf), buf, amount)) < 0)
			excomerr("Error reading '%s'.\n", tarfiles[tarfindex]);
	}
	return (cnt);
}

/*
 * Refill the buffer if no fifo.
 * Called from buf_rwait()
 */
LOCAL void
readbuf()
{
	bigcnt = readtape(bigbuf, bigsize);
	bigptr = bigbuf;
}

/*
 * Mid level function to read a tape block (usually 10k)
 * Called from the fifo fill code and from readbuf().
 */
EXPORT int
readtape(buf, amount)
	char	*buf;
	int	amount;
{
	int	amt;
	int	cnt;
	char	*bp;
	int	size;
static	BOOL	teof = FALSE;

	if (teof)
		return (0);

	amt = 0;
	bp = buf;
	size = amount;

	do {
		cnt = readtblock(bp, size);

		amt += cnt;
		bp += cnt;
		size -= cnt;
	} while (amt < amount && cnt > 0 && multblk);

	if (amt == 0)
		return (amt);
	if (amt < TBLOCK) {
		errmsgno(EX_BAD, "Error reading '%s' size (%d) too small.\n",
						tarfiles[tarfindex], amt);
		/*
		 * Do not continue after we did read less than 512 bytes.
		 */
		teof = TRUE;
	}
	/*
	 * First block
	 */
	if (stats->swapflg < 0) {
		if ((amt % TBLOCK) != 0)
			comerrno(EX_BAD, "Invalid blocksize %d bytes.\n", amt);
		if (amt < amount) {
			stats->blocksize = bigsize = amt;
			stats->nblocks = bigsize/TBLOCK;
#ifdef	FIFO
			if (use_fifo)
				fifo_ibs_shrink(amt);
#endif
			errmsgno(EX_BAD, "Blocksize = %ld records.\n",
						stats->blocksize/TBLOCK);
		}
	}
	if (stats->swapflg > 0)
		swabbytes(buf, amt);

	if (amt == stats->blocksize)
		stats->blocks++;
	else
		stats->parts += amt;
	stats->lastsize = amt;
#ifdef	DEBUG
	error("readbuf: cnt: %d.\n", amt);
#endif
	return (amt);
}

#define	DO8(a)	a; a; a; a; a; a; a; a;

#ifdef	MY_SWABBYTES

void
swabbytes(bp, cnt)
	register char	*bp;
	register int	cnt;
{
	register char	c;

	cnt /= 2;	/* even count only */
	while ((cnt -= 8) >= 0) {
		DO8(c = *bp++; bp[-1] = *bp; *bp++ = c);
	}
	cnt += 8;

	while (--cnt >= 0) {
		c = *bp++; bp[-1] = *bp; *bp++ = c;
	}
}
#endif

#define	DO16(a)		DO8(a) DO8(a)

EXPORT void
filltcb(ptb)
	register TCB	*ptb;
{
	register int	i;
	register long	*lp = ptb->ldummy;

	for (i = 512/sizeof (long)/16; --i >= 0; ) {
		DO16(*lp++ = 0L)
	}
}

EXPORT void
movetcb(from_ptb, to_ptb)
	register TCB	*from_ptb;
	register TCB	*to_ptb;
{
	register int	i;
	register long	*from = from_ptb->ldummy;
	register long	*to   = to_ptb->ldummy;

	for (i = 512/sizeof (long)/16; --i >= 0; ) {
		DO16(*to++ = *from++)
	}
}

/*
 * Try to allocate 'amount' bytes from the buffer or from the fifo.
 * If it is not possible to get 'amount' bytes in a single chunk, return NULL.
 */
EXPORT void *
get_block(amount)
	int	amount;
{
	if (buf_wait(amount) < amount)
		return ((void *)NULL);
	return ((void *)bigptr);
}

/*
 * Tell the buffer or the fifo that 'amount' bytes in the buffer/fifo space
 * are ready to be written.
 */
EXPORT void
put_block(amount)
	int	amount;
{
	buf_wake(amount);
}

/*
 * Write TBLOCK bytes into the buffer/fifo space and tell the buffer/fifo
 * that TBLOCK bytes are ready to be written.
 */
EXPORT char *
writeblock(buf)
	char	*buf;
{
	char	*obp;

	buf_wait(TBLOCK);
	obp = bigptr;
	movetcb((TCB *)buf, (TCB *)bigptr);
	buf_wake(TBLOCK);

	return (obp);
}

/*
 * Mid level function to write a tape block (usually 10k)
 * Called from the fifo fill output code and from writebuf().
 */
EXPORT int
writetape(buf, amount)
	char	*buf;
	int	amount;
{
	int	cnt;
	int	err = 0;
					/* hartes oder weiches EOF ???  */
					/* d.h. < 0 oder <= 0		*/
	stats->reading = FALSE;
	if (multivol && tsize) {
		Ullong	cursize;

		cursize = stats->blocks * stats->nblocks + stats->parts / TBLOCK;

		if (cursize >= tsize) {	/* tsize= induced change */
			changetape(TRUE);
			cnt = startvol(buf, amount);
			if (cnt > 0)
				return (cnt);
		}
	}
	seterrno(0);
	if (nullout) {
		cnt = amount;
#ifdef	USE_REMOTE
	} else if (isremote) {
		cnt = rmtwrite(remfd, buf, amount);	   /* Handles EINTR */
#endif
	} else {
		cnt = _niwrite(fileno(tarf), buf, amount); /* Handles EINTR */
	}
	if (cnt == 0) {
		err = EFBIG;
	} else if (cnt < 0) {
		err = geterrno();
	}

	if (multivol && (err == EFBIG || err == ENOSPC)) {
		/*
		 * QIC tapes (unblocked) may do partial writes at EOT.
		 * We do the tape change not at the point when we write less
		 * than a tape block (this may happen on pipes too) but after
		 * we got a true EOF condition.
		 */
		return (-2);
	}
	if (multivol && (err == ENXIO)) {
		/*
		 * EOF condition on disk devices
		 */
		return (-2);
	}

	if (cnt == stats->blocksize)
		stats->blocks++;
	else if (cnt >= 0)
		stats->parts += cnt;

	if (cnt <= 0)
		excomerrno(err, "Error writing '%s'.\n", tarfiles[tarfindex]);
	return (cnt);
}

/*
 * Write output the buffer if no fifo.
 * Called from buf_wait()
 */
LOCAL void
writebuf(amount)
	int	amount;
{
	long	cnt;


nextwrite:
	cnt = writetape(bigbuf, amount);

	if (cnt < amount) {
		if (cnt == -2) {		/* EOT induced change */
			changetape(TRUE);
			if ((cnt = startvol(bigbuf, amount)) <= 0)
				goto nextwrite;
		}
		/*
		 * QIC tapes (unblocked) may do partial writes at EOT
		 *
		 * Even if we hit a "planned" tape change, the fact
		 * that we need to write a vol header looks from higher
		 * levels as if there was a partial write.
		 */
		bigptr  = &bigbuf[cnt];
		bigcnt -= cnt;
		movebytes(bigptr, bigbuf, bigcnt);
		bigptr  = &bigbuf[bigcnt];
	} else {
		bigptr = bigbuf;
		bigcnt = 0;
	}
	stats->old_size = stats->cur_size;
	stats->old_off  = stats->cur_off;
}

/*
 * Called only from weof()
 */
LOCAL void
flushbuf()
{
#ifdef	FIFO
	if (!use_fifo)
#endif
	{
		/*
		 * Loop because a tape change and writing a vol header
		 * may look like an incomplete write and need a second
		 * write to really flush the buffer.
		 */
		while (bigcnt > 0)
			writebuf(bigcnt);
	}
}

/*
 * Write an empty TBLOCK
 * Called from weof() and cr_file()
 */
EXPORT void
writeempty()
{
	TCB	tb;

	filltcb(&tb);
	writeblock((char *)&tb);
}

/*
 * Write a logical TAR EOF (2 empty TBLOCK sized blocks)
 * or a CPIO EOF marker.
 */
EXPORT void
weof()
{
	if ((props.pr_flags & PR_CPIO) != 0) {
		cpio_weof();
		buf_sync(TBLOCK);
	} else {
		writeempty();
		writeempty();
	}
	if (!partial)
		buf_sync(0);
	flushbuf();
}

/*
 * If size == 0, fill the buffer up to a TAPE record (usually 10k),
 * if size != 0, fill the buffer up to size.
 */
EXPORT void
buf_sync(size)
	int	size;
{
#ifdef	FIFO
	if (use_fifo) {
		fifo_sync(size);
	} else
#endif
	if (size) {
		int	amt = 0;

		if ((bigcnt % size) != 0)
			amt = size - bigcnt%size;

		fillbytes(bigptr, amt, '\0');
		bigcnt += amt;
		bigptr += amt;
	} else {
		fillbytes(bigptr, bigsize - bigcnt, '\0');
		bigcnt = bigsize;
	}
}

/*
 * Drain the fifo if in fifo mode.
 */
EXPORT void
buf_drain()
{
#ifdef	FIFO
	if (use_fifo) {
		fifo_oflush();
		wait(0);
	}
#endif
}

/*
 * Wait until we may put amount bytes into the buffer/fifo.
 * The returned count may be lower. Callers need to be prepared about this.
 */
EXPORT int
buf_wait(amount)
	int	amount;
{
#ifdef	FIFO
	if (use_fifo) {
		return (fifo_iwait(amount));
	} else
#endif
	{
		if (bigcnt >= bigsize)
			writebuf(bigsize);
		return (bigsize - bigcnt);
	}
}

/*
 * Tell the buffer/fifo management that amount bytes are ready to be written.
 * The space may now be written to the media and the space may be made
 * avbailable for being filled up again.
 */
EXPORT void
buf_wake(amount)
	int	amount;
{
#ifdef	FIFO
	if (use_fifo) {
		fifo_owake(amount);
	} else
#endif
	{
		bigptr += amount;
		bigcnt += amount;
	}
	if (copyflag) {
		/*
		 * In copy mode, there is no blocked read/write from the fifo
		 * Tape process. For this reason, we increment the byte count
		 * at this place.
		 */
		stats->parts += amount;
	}
}

/*
 * Wait until we may read amount bytes from the buffer/fifo.
 * The returned count may be lower. Callers need to be prepared about this.
 */
EXPORT int
buf_rwait(amount)
	int	amount;
{
	int	cnt;

again:
#ifdef	FIFO
	if (use_fifo) {
		cnt = fifo_owait(amount);
	} else
#endif
	{
		if (bigcnt <= 0)
			readbuf();
		cnt = bigcnt;
	}
	if (cnt == 0 && multivol) {
		nextitape();
		goto again;
	}
	return (cnt);
}

/*
 * Tell the buffer/fifo management that amount bytes are no longer needed for
 * read access. The space may now be filled up with new data from the medium.
 */
EXPORT void
buf_rwake(amount)
	int	amount;
{
#ifdef	FIFO
	if (use_fifo) {
		fifo_iwake(amount);
	} else
#endif
	{
		bigptr += amount;
		bigcnt -= amount;
	}
}

/*
 * Resume the fifo if the fifo has been blocked after the first read
 * TAPE block (usually 10 k).
 * If the fifo is active, fifo_resume() triggers a shadow call to
 * setprops() in the fifo background process.
 */
EXPORT void
buf_resume()
{
extern	long	hdrtype;
	stats->swapflg = swapflg;	/* copy over for fifo process */
	stats->hdrtype = hdrtype;	/* copy over for fifo process */
	bigsize = stats->blocksize;	/* copy over for tar process */
#ifdef	FIFO
	if (use_fifo)
		fifo_resume();
#endif
}

/*
 * Backspace tape or medium to prepare it for appending to an archive.
 * Note that this currently only handles TAR archives and that even then it
 * will not work if the TAPE record size is < 2*TBLOCK (1024 bytes).
 */
EXPORT void
backtape()
{
	Llong	ret;
	BOOL	istape = FALSE;

	if (debug) {
		error("Blocks: %lld\n", tblocks());
		error("filepos: %lld seeking to: %lld bigsize: %d\n",
		(Llong)mtseek((off_t)0, SEEK_CUR),
		(Llong)mtseek((off_t)0, SEEK_CUR) - (Llong)stats->lastsize, bigsize);
	}

	if (mtioctl(MTNOP, 0) >= 0) {
		istape = TRUE;
		if (debug)
			error("Is a tape: BSR 1...\n");
		ret = mtioctl(MTBSR, 1);
	} else {
		if (debug)
			error("Is a file: lseek()\n");
		ret = mtseek(-stats->lastsize, SEEK_CUR);
	}
	if (ret == (Llong)-1)
		excomerr("Cannot backspace %s.\n", istape ? "tape":"medium");

	if (stats->lastsize == stats->blocksize)
		stats->blocks--;
	else
		stats->parts -= stats->lastsize;
}

/*
 * Send an MTIOCTOP call to the file descriptor that is use for the medium.
 */
EXPORT int
mtioctl(cmd, count)
	int	cmd;
	int	count;
{
	int	ret;

	if (nullout && !(uflag || rflag)) {
		return (0);
#ifdef	USE_REMOTE
	} else if (isremote) {
		ret = rmtioctl(remfd, cmd, count);
#endif
	} else {
#if	defined(MTIOCTOP) && defined(HAVE_IOCTL)
		struct mtop mtop;

		mtop.mt_op = cmd;
		mtop.mt_count = count;

		ret = ioctl(fdown(tarf), MTIOCTOP, &mtop);
#else
#ifdef	ENOSYS
		seterrno(ENOSYS);
#else
		seterrno(EINVAL);
#endif
		return (-1);
#endif
	}
	if (ret < 0 && debug) {
		errmsg("Error sending mtioctl(%d, %d) to '%s'.\n",
					cmd, count, tarfiles[tarfindex]);
	}
	return (ret);
}

/*
 * Make an lseek() call to the file descriptor that is use for the medium.
 */
EXPORT off_t
mtseek(offset, whence)
	off_t	offset;
	int	whence;
{
	if (nullout && !(uflag || rflag)) {
		return (0L);
#ifdef	USE_REMOTE
	} else if (isremote) {
		return (rmtseek(remfd, offset, whence));
#endif
	} else {
		return (lseek(fileno(tarf), offset, whence));
	}
}

/*
 * Return the current archive block number based on 512 byte (TBLOCK) units.
 */
EXPORT Llong
tblocks()
{
	long	fifo_cnt = 0;
	Llong	ret;

#ifdef	FIFO
	if (use_fifo)
		fifo_cnt = fifo_amount()/TBLOCK;
#endif
	if (stats->reading)
		ret = (-fifo_cnt + stats->blocks * stats->nblocks +
				(stats->parts - (bigcnt+TBLOCK))/TBLOCK);
	else
		ret = (fifo_cnt + stats->blocks * stats->nblocks +
				(stats->parts + bigcnt)/TBLOCK);
	if (debug) {
		error("tblocks: %lld blocks: %lld blocksize: %ld parts: %lld bigcnt: %ld fifo_cnt: %ld\n",
		ret, stats->blocks, stats->blocksize, stats->parts, bigcnt, fifo_cnt);
	}
	curblockno = ret;
	return (ret);
}

EXPORT void
prstats()
{
	Llong	bytes;
	Llong	kbytes;
	int	per;
#ifdef	timerclear
	int	sec;
	int	nsec;
	int	tmsec;
#endif
	char	*p;

#ifdef	FIFO
	if (use_fifo) {
		extern	m_head  *mp;
		p = mp->end;
	} else
#endif
		p = &bigbuf[bigbsize];

	if ((*p != 'U' && *p != ' ') || p[1] != 'U')
		errmsgno(EX_BAD, "The buffer has been overwritten, please contact the author.\n");

	if (no_stats)
		return;
	if (pid == 0)	/* child */
		return;

#ifdef	timerclear
	if (showtime && getnstimeofday(&stoptime) < 0)
		comerr("Cannot get stoptime\n");
#endif
#ifdef	FIFO
	if (use_fifo && do_fifostats)
		fifo_stats();
#endif

	bytes = stats->blocks * (Llong)stats->blocksize + stats->parts;
	kbytes = bytes >> 10;
	per = ((bytes&1023)<<10)/10485;

	if (cpio_stats) {
		bytes = stats->Tblocks * (Llong)stats->blocksize + stats->Tparts;

		error("%lld blocks\n", stats->eofblock + 1 + bytes/512);
		return;
	}

	errmsgno(EX_BAD,
		"%lld blocks + %lld bytes (total of %lld bytes = %lld.%02dk).\n",
		stats->blocks, stats->parts, bytes, kbytes, per);

	if (stats->Tblocks + stats->Tparts) {
		bytes = (stats->blocks + stats->Tblocks) *
						(Llong)stats->blocksize +
						(stats->parts + stats->Tparts);
		kbytes = bytes >> 10;
		per = ((bytes&1023)<<10)/10485;

		errmsgno(EX_BAD,
		"Total %lld blocks + %lld bytes (total of %lld bytes = %lld.%02dk).\n",
		stats->blocks + stats->Tblocks,
		stats->parts + stats->Tparts,
		bytes, kbytes, per);
	}
#ifdef	timerclear
	if (showtime) {
		Llong	kbs;

		sec = stoptime.tv_sec - starttime.tv_sec;
		nsec = stoptime.tv_nsec - starttime.tv_nsec;
		tmsec = sec*1000 + nsec/1000000;
		if (nsec < 0) {
			sec--;
			nsec += 1000000000;
		}
		if (tmsec == 0)
			tmsec++;

		kbs = kbytes*(Llong)1000/tmsec;

		errmsgno(EX_BAD, "Total time %d.%03dsec (%lld kBytes/sec)\n",
				sec, nsec/1000000, kbs);
	}
#endif
}

EXPORT BOOL
checkerrs()
{
	if (xstats.s_staterrs	||
#ifdef	USE_ACL
	    xstats.s_getaclerrs	||
#endif
	    xstats.s_openerrs	||
	    xstats.s_rwerrs	||
	    xstats.s_misslinks	||
	    xstats.s_toolong	||
	    xstats.s_toobig	||
	    xstats.s_isspecial	||
	    xstats.s_sizeerrs	||
	    xstats.s_chdir	||

	    xstats.s_settime	||
	    xstats.s_security	||
	    xstats.s_lsecurity	||
	    xstats.s_samefile	||
#ifdef	USE_ACL
	    xstats.s_badacl	||
	    xstats.s_setacl	||
#endif
#ifdef USE_XATTR
	    xstats.s_getxattr	||
	    xstats.s_setxattr	||
#endif
	    xstats.s_setmodes	||
	    xstats.s_restore) {
		if (nowarn || no_stats || (pid == 0) /* child */)
			return (TRUE);

		errmsgno(EX_BAD, "The following problems occurred during archive processing:\n");
		errmsgno(EX_BAD, "Cannot: stat %d, open %d, read/write %d, chdir %d. Size changed %d.\n",
				xstats.s_staterrs,
				xstats.s_openerrs,
				xstats.s_rwerrs,
				xstats.s_chdir,
				xstats.s_sizeerrs);
		errmsgno(EX_BAD, "Missing links %d, Name too long %d, File too big %d, Not dumped %d.\n",
				xstats.s_misslinks,
				xstats.s_toolong,
				xstats.s_toobig,
				xstats.s_isspecial);
		if (xstats.s_settime || xstats.s_setmodes)
			errmsgno(EX_BAD, "Cannot set: time %d, modes %d.\n",
				xstats.s_settime,
				xstats.s_setmodes);
		if (xstats.s_security || xstats.s_lsecurity)
			errmsgno(EX_BAD, "Skipped for security reason: path name %d, link name %d.\n",
				xstats.s_security, xstats.s_lsecurity);
		if (xstats.s_samefile)
			errmsgno(EX_BAD, "Skipped same file %d.\n",
				xstats.s_samefile);
#ifdef	USE_ACL
		if (xstats.s_getaclerrs || xstats.s_badacl || xstats.s_setacl)
			errmsgno(EX_BAD, "Cannot get ACL: %d set ACL: %d. Bad ACL %d.\n",
				xstats.s_getaclerrs,
				xstats.s_setacl,
				xstats.s_badacl);
#endif
#ifdef USE_XATTR
		if (xstats.s_getxattr || xstats.s_setxattr)
			errmsgno(EX_BAD, "Cannot get xattr: %d set xattr: %d.\n",
				xstats.s_getxattr,
				xstats.s_setxattr);
#endif
		if (xstats.s_restore)
			errmsgno(EX_BAD, "Problems with restore database.\n");
		return (TRUE);
	}
	return (FALSE);
}

EXPORT void
exprstats(ret)
	int	ret;
{
	prstats();
	checkerrs();
	exit(ret);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
excomerrno(int err, char *fmt, ...)
#else
EXPORT void
excomerrno(err, fmt, va_alist)
	int	err;
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	errmsgno(err, "%r", fmt, args);
	va_end(args);
#ifdef	FIFO
	fifo_exit(err);
#endif
	exprstats(err);
	/* NOTREACHED */
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
excomerr(char *fmt, ...)
#else
EXPORT void
excomerr(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	err = geterrno();

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	errmsgno(err, "%r", fmt, args);
	va_end(args);
#ifdef	FIFO
	fifo_exit(err);
#endif
	exprstats(err);
	/* NOTREACHED */
}

EXPORT void
die(err)
	int	err;
{
	excomerrno(err, "Cannot recover from error - exiting.\n");
}

/*
 * Quick hack to implement a -z flag. May be changed soon.
 */
#include <schily/signal.h>
#if	defined(SIGDEFER) || defined(SVR4)
#define	signal	sigset
#endif
LOCAL void
compressopen()
{
#ifdef	HAVE_FORK
	FILE	*pp[2];
	int	mypid;
	char	*zip_prog = "gzip";

	if (compress_prg)
		zip_prog = compress_prg;
	else if (bzflag)
		zip_prog = "bzip2";
	else if (Zflag)
		zip_prog = "compress";
	else if (lzoflag)
		zip_prog = "lzop";
	else if (p7zflag)
		zip_prog = "p7zip";
	else if (xzflag)
		zip_prog = "xz";
	else if (lzipflag)
		zip_prog = "lzip";

	multblk = TRUE;

	if (cflag && (uflag || rflag))
		comerrno(EX_BAD, "Cannot update compressed archives.\n");

#ifdef __DJGPP__
	if (cflag) {
		/*
		 * We try to emulate a command line like:
		 *
		 *	"star -c dir | %s > dir.tar.%s\n",
		 *	zip_prog,
		 *	Zflag?"Z":bzflag?"bz2":compress_prg?compress_prg:"gz");
		 *
		 * If we would use popen() instead, DJGPP will run the program
		 * from popen() first, so there would
		 * be no data from the "tarf" File pointer.
		 * We use the temporary file instead.
		 */
		if ((compress_tmpf = tmpfile()) == NULL)
			comerr("Compress pipe failed\n");
		compress_tarf_save = tarf;
		tarf = compress_tmpf;

	} else {
		int	stdin_save;
		char	zip_cmd[256];

		/*
		 * We try to emulate a command line like:
		 *
		 *	"%s -d < archive.tar.%s | star -x\n",
		 *	zip_prog,
		 *	Zflag?"Z":bzflag?"bz2":compress_prg?compress_prg:"gz");
		 */
		js_snprintf(zip_cmd, sizeof (zip_cmd), "%s.exe -d", zip_prog);

		stdin_save = dup(STDIN_FILENO);
		dup2(fileno(tarf), STDIN_FILENO);
		compress_tarf_save = tarf;
		if ((tarf = popen(zip_cmd, "rb")) == NULL)
			comerr("Compress pipe failed\n");
		dup2(stdin_save, STDIN_FILENO);
		fclose(compress_tarf_save);
	}
#else
	if (fpipe(pp) == 0)
		comerr("Compress pipe failed\n");
	mypid = fork();
	if (mypid < 0)
		comerr("Compress fork failed\n");
	if (mypid == 0) {
		FILE	*null;
		char	*flg = getenv("STAR_COMPRESS_FLAG"); /* Temporary ? */

		signal(SIGQUIT, SIG_IGN);
		if (cflag)
			fclose(pp[1]);
		else
			fclose(pp[0]);

#ifdef	NEED_O_BINARY
		if (cflag)
			setmode(fileno(pp[0]), O_BINARY);
		else
			setmode(fileno(pp[1]), O_BINARY);
#endif

		/* We don't want to see errors */
		null = fileopen("/dev/null", "rw");

		if (cflag)
			fexecl(zip_prog, pp[0], tarf, null, zip_prog, flg, (char *)NULL);
		else
			fexecl(zip_prog, tarf, pp[1], null, zip_prog, "-d", (char *)NULL);
		errmsg("Compress: exec of '%s' failed\n", zip_prog);
		_exit(-1);
	}
	fclose(tarf);
	if (cflag) {
		tarf = pp[1];
		fclose(pp[0]);
	} else {
		tarf = pp[0];
		fclose(pp[1]);
	}
	setmode(fileno(tarf), O_BINARY);
#endif /* !__DJGPP__ */
#else  /* !HAVE_FORK */
	comerrno(EX_BAD, "Inline compression not available.\n");
#endif
}

LOCAL void
compressclose()
{
#ifdef HAVE_FORK
#ifdef __DJGPP__
	if (cflag) {
		if (compress_tmpf) {
			char	zip_cmd[256];
			FILE	*zipf;
			int	cnt = -1;
			char	buf[8192];
			char	*zip_prog = "gzip";

			if (compress_prg)
				zip_prog = compress_prg;
			else if (bzflag)
				zip_prog = "bzip2";
			else if (Zflag)
				zip_prog = "compress";
			else if (lzoflag)
				zip_prog = "lzop";
			else if (p7zflag)
				zip_prog = "p7zip";
			else if (xzflag)
				zip_prog = "xz";
			else if (lzipflag)
				zip_prog = "lzip";

			js_snprintf(zip_cmd, sizeof (zip_cmd), "%s.exe", zip_prog);

			dup2(fileno(compress_tarf_save), STDOUT_FILENO);

			if ((zipf = popen(zip_cmd, "wb")) == NULL)
				comerr("Compress pipe failed\n");

			fseek(compress_tmpf, 0l, SEEK_SET);

			while ((cnt = ffileread(compress_tmpf, buf, sizeof (buf))) > 0)
				ffilewrite(zipf, buf, cnt);

			pclose(zipf);
			fclose(compress_tmpf);
			compress_tmpf = (FILE *)NULL;
		}

	} else {
		pclose(tarf);
	}
#endif
#endif
}
