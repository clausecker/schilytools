/* @(#)cdda2wav.c	1.87 08/02/17 Copyright 1998-2004 Heiko Eissfeldt, Copyright 2004-2008 J. Schilling */
#ifndef lint
static char	sccsid[] =
"@(#)cdda2wav.c	1.87 08/02/17 Copyright 1998-2004 Heiko Eissfeldt, Copyright 2004-2008 J. Schilling";

#endif
#undef	DEBUG_BUFFER_ADDRESSES
#undef	GPROF
#undef	DEBUG_FORKED
#undef	DEBUG_CLEANUP
#undef	DEBUG_DYN_OVERLAP
#undef	DEBUG_READS

#define	DEBUG_ILLLEADOUT	0	/* 0 disables, 1 enables */
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
 * parts    (C) Peter Widow
 * parts    (C) Thomas Niederreiter
 * parts    (C) RSA Data Security, Inc.
 * parts    (C) J. Schilling
 *
 * last changes:
 *   18.12.93 - first version,	OK
 *   01.01.94 - generalized & clean up HE
 *   10.06.94 - first linux version HE
 *   12.06.94 - wav header alignment problem fixed HE
 *   12.08.94 - open the cdrom device O_RDONLY makes more sense :-)
 *		no more floating point math
 *		change to sector size 2352 which is more common
 *		sub-q-channel information per kernel ioctl requested
 *		doesn't work as well as before
 *		some new options (-max -i)
 *   01.02.95 - async i/o via semaphores and shared memory
 *   03.02.95 - overlapped reading on sectors
 *   03.02.95 - generalized sample rates. all integral divisors are legal
 *   04.02.95 - sun format added
 *              more divisors: all integral halves >= 1 allowed
 *		floating point math needed again
 *   06.02.95 - bugfix for last track and not d0
 *              tested with photo-cd with audio tracks
 *		tested with xa disk
 *   29.01.96 - new options for bulk transfer
 *   01.06.96 - tested with enhanced cd
 *   01.06.96 - tested with cd-plus
 *   02.06.96 - support pipes
 *   02.06.96 - support raw format
 *   04.02.96 - security hole fixed
 *   22.04.97 - large parts rewritten
 *   28.04.97 - make file names DOS compatible
 *   01.09.97 - add speed control
 *   20.10.97 - add find mono option
 *   Jan/Feb 98 - conversion to use Joerg Schillings SCSI library
 *   see ChangeLog
 */

#include "config.h"

#include <schily/unistd.h>
#include <stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <signal.h>
#include <math.h>
#include <schily/fcntl.h>
#include <schily/time.h>
#if defined(HAVE_LIMITS_H) && (HAVE_LIMITS_H == 1)
#include <limits.h>
#endif
#include <schily/ioctl.h>
#include <schily/errno.h>
#include <schily/stat.h>
#include <schily/wait.h>
#if defined(HAVE_SETPRIORITY) && (HAVE_SETPRIORITY == 1)
#include <sys/resource.h>
#endif
#include <schily/varargs.h>

#include <scg/scsitransp.h>

#ifdef	HAVE_AREAS
#include <be/kernel/OS.h>
#endif

#include "mytype.h"
#include "sndconfig.h"

#include "semshm.h"	/* semaphore functions */
#include "sndfile.h"
#include "wav.h"	/* wav file header structures */
#include "sun.h"	/* sun audio file header structures */
#include "raw.h"	/* raw file handling */
#include "aiff.h"	/* aiff file handling */
#include "aifc.h"	/* aifc file handling */
#ifdef	USE_LAME
#include "mp3.h"	/* mp3 file handling */
#endif
#include "interface.h"  /* low level cdrom interfacing */
#include "cdda2wav.h"
#include "cdrecord.h"	/* Only for getnum() */
#include "resample.h"
#include "toc.h"
#include "setuid.h"
#include "ringbuff.h"
#include "global.h"
#include "exitcodes.h"
#ifdef	USE_PARANOIA
#include "cdda_paranoia.h"
#endif
#include "defaults.h"
#include "version.h"

#ifdef	VMS
#include <vms_init.h>
#define	open(n, p, m)	(open)((n), (p), (m), \
				"ctx=bin", "rfm=fix", "mrs=512", \
				"acc", acc_cb, &open_id)
#endif

EXPORT	int	main			__PR((int argc, char **argv));
static	void	RestrictPlaybackRate	__PR((long newrate));
static	void	output_indices		__PR((FILE *fp, index_list *p,
						unsigned trackstart));
static	int	write_info_file		__PR((char *fname_baseval,
						unsigned int track,
						unsigned long SamplesDone,
						int numbered));
static	void	CloseAudio		__PR((int channels_val,
						unsigned long nSamples,
						struct soundfile *audio_out));
static	void	CloseAll		__PR((void));
static	void	OpenAudio		__PR((char *fname, double rate,
						long nBitsPerSample,
						long channels_val,
						unsigned long expected_bytes,
						struct soundfile *audio_out));
static	void	set_offset		__PR((myringbuff *p, int offset));
static	int	get_offset		__PR((myringbuff *p));
static	void	usage			__PR((void));
static	void	init_globals		__PR((void));
static	int	is_fifo			__PR((char * filename));


/*
 * Rules:
 * unique parameterless options first,
 * unique parametrized option names next,
 * ambigious parameterless option names next,
 * ambigious string parametrized option names last
 */
/* BEGIN CSTYLED */
static const char *opts = "paranoia,paraopts&,version,help,h,\
no-write,N,dump-rates,R,bulk,B,alltracks,verbose-scsi+,V+,\
find-extremes,F,find-mono,G,no-infofile,H,\
deemphasize,T,info-only,J,silent-scsi,Q,\
cddbp-server*,cddbp-port*,\
scanbus,device*,dev*,D*,ts&,auxdevice*,A*,interface*,I*,output-format*,O*,\
output-endianess*,E*,cdrom-endianess*,C*,speed#,S#,\
playback-realtime#L,p#L,md5#,M#,set-overlap#,P#,sound-device*,K*,\
cddb#,L#,channels*,c*,bits-per-sample#,b#,rate#,r#,gui,g,\
divider*,a*,track*,t*,index#,i#,duration*,d*,offset#L,o#L,start-sector#L,\
sectors-per-request#,n#,verbose-level&,v&,buffers-in-ring#,l#,\
stereo,s,mono,m,wait,w,echo,e,quiet,q,max,x,out-fd#,audio-fd#\
";
/* END CSTYLED */


#ifdef	NEED_O_BINARY
#include <io.h>		/* for setmode() prototype */
#endif

/*
 * global variables
 */
global_t	global;

/*
 * static variables
 */
static unsigned long	nSamplesDone = 0;

static	int		child_pid = -2;

static unsigned long	*nSamplesToDo;
static unsigned int	current_track;
static int		bulk = 0;

unsigned int get_current_track __PR((void));

unsigned int
get_current_track()
{
	return (current_track);
}

static void
RestrictPlaybackRate(newrate)
	long	newrate;
{
	global.playback_rate = newrate;

	/*
	 * filter out insane values
	 */
	if (global.playback_rate < 25)
		global.playback_rate = 25;
	if (global.playback_rate > 250)
		global.playback_rate = 250;

	if (global.playback_rate < 100)
		global.nsectors = (global.nsectors*global.playback_rate)/100;
}


long
SamplesNeeded(amount, undersampling_val)
	long	amount;
	long	undersampling_val;
{
	long	retval = ((undersampling_val * 2 + Halved) * amount) / 2;

	if (Halved && (*nSamplesToDo & 1))
		retval += 2;
	return (retval);
}

static int	argc2;
static int	argc3;
static char	**argv2;

static void reset_name_iterator __PR((void));
static void
reset_name_iterator()
{
	argv2 -= argc3 - argc2;
	argc2 = argc3;
}

static char *get_next_name __PR((void));
static char
*get_next_name()
{
	if (argc2 > 0) {
		argc2--;
		return (*argv2++);
	} else {
		return (NULL);
	}
}

static char *cut_extension __PR((char * fname));

static char
*cut_extension(fname)
	char	*fname;
{
	char	*pp;

	pp = strrchr(fname, '.');

	if (pp == NULL) {
		pp = fname + strlen(fname);
	}
	*pp = '\0';

	return (pp);
}

#ifdef INFOFILES
static void
output_indices(fp, p, trackstart)
	FILE		*fp;
	index_list	*p;
	unsigned	trackstart;
{
	int	ci;

	fprintf(fp, "Index=\t\t");

	if (p == NULL) {
		fprintf(fp, "0\n");
		return;
	}

	for (ci = 1; p != NULL; ci++, p = p->next) {
		int	frameoff = p->frameoffset;

		if (p->next == NULL)
			fputs("\nIndex0=\t\t", fp);
#if 0
		else if (ci > 8 && (ci % 8) == 1)
			fputs("\nIndex =\t\t", fp);
#endif
		if (frameoff != -1)
			fprintf(fp, "%d ", frameoff - trackstart);
		else
			fprintf(fp, "-1 ");
	}
	fputs("\n", fp);
}

/*
 * write information before the start of the sampling process
 *
 *
 * uglyfied for Joerg Schillings ultra dumb line parser
 */
static int
write_info_file(fname_baseval, track, SamplesDone, numbered)
	char		*fname_baseval;
	unsigned int	track;
	unsigned long	SamplesDone;
	int		numbered;
{
	FILE	*info_fp;
	char	fname[200];
	char	datetime[30];
	time_t	utc_time;
	struct tm *tmptr;

	/*
	 * write info file
	 */
	if (strcmp(fname_baseval, "-") == 0)
		return (0);

	strncpy(fname, fname_baseval, sizeof (fname) -1);
	fname[sizeof (fname) -1] = 0;
	if (numbered)
		sprintf(cut_extension(fname), "_%02u.inf", track);
	else
		strcpy(cut_extension(fname), ".inf");

	info_fp = fopen(fname, "w");
	if (!info_fp)
		return (-1);

#if 0
#ifdef MD5_SIGNATURES
	if (global.md5blocksize)
		MD5Final(global.MD5_result, &global.context);
#endif
#endif

	utc_time = time(NULL);
	tmptr = localtime(&utc_time);
	if (tmptr) {
		strftime(datetime, sizeof (datetime), "%x %X", tmptr);
	} else {
		strncpy(datetime, "unknown", sizeof (datetime));
	}
	fprintf(info_fp, "#created by cdda2wav %s%s %s\n#\n",
								VERSION,
								VERSION_OS,
								datetime);
	fprintf(info_fp, "CDINDEX_DISCID=\t'%s'\n", global.cdindex_id);
	/* BEGIN CSTYLED */
	fprintf(info_fp,
"CDDB_DISCID=\t0x%08lx\n\
MCN=\t\t%s\n\
ISRC=\t\t%15.15s\n\
#\n\
Albumperformer=\t'%s'\n\
Performer=\t'%s'\n\
Albumtitle=\t'%s'\n",
		(unsigned long) global.cddb_id,
		Get_MCN(),
		Get_ISRC(track),
		global.creator != NULL ?
			global.creator : (const unsigned char *)"",
		global.trackcreator[track] != NULL ?
			global.trackcreator[track] :
		(global.creator != NULL ?
			global.creator : (const unsigned char *)""),
		global.disctitle != NULL ?
			global.disctitle : (const unsigned char *)"");
	/* END CSTYLED */
	fprintf(info_fp,  "Tracktitle=\t'%s'\n",
		global.tracktitle[track] ?
			global.tracktitle[track] : (const unsigned char *)"");
	fprintf(info_fp, "Tracknumber=\t%u\n", track);
	fprintf(info_fp, "Trackstart=\t%ld\n", Get_AudioStartSector(track));
	fprintf(info_fp,
	"# track length in sectors (1/75 seconds each), rest samples\nTracklength=\t%ld, %d\n",
		SamplesDone/588L, (int)(SamplesDone%588));
	fprintf(info_fp, "Pre-emphasis=\t%s\n",
		Get_Preemphasis(track) && (global.deemphasize == 0) ?
						"yes" : "no");
	fprintf(info_fp, "Channels=\t%d\n",
		Get_Channels(track) ? 4 : global.channels == 2 ? 2 : 1);

	{  int cr = Get_Copyright(track);

		fputs("Copy_permitted=\t", info_fp);
		switch (cr) {
			case 0:
				fputs("once (copyright protected)\n", info_fp);
			break;
			case 1:
				fputs("no (SCMS first copy)\n", info_fp);
			break;
			case 2:
				fputs("yes (not copyright protected)\n",
					info_fp);
			break;
			default:
				fputs("unknown\n", info_fp);
		}
	}
	fprintf(info_fp, "Endianess=\t%s\n",
		global.need_big_endian ? "big" : "little");
	fprintf(info_fp, "# index list\n");
	output_indices(info_fp, global.trackindexlist[track],
		Get_AudioStartSector(track));
#if 0
	/*
	 * MD5 checksums in info files are currently broken.
	 * for on-the-fly-recording the generation of info files has been
	 * shifted before the recording starts, so there is no checksum at
	 * that point.
	 */
#ifdef MD5_SIGNATURES
	fprintf(info_fp,
		"#(blocksize) checksum\nMD-5=\t\t(%d) %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		global.md5blocksize,
		global.MD5_result[0],
		global.MD5_result[1],
		global.MD5_result[2],
		global.MD5_result[3],
		global.MD5_result[4],
		global.MD5_result[5],
		global.MD5_result[6],
		global.MD5_result[7],
		global.MD5_result[8],
		global.MD5_result[9],
		global.MD5_result[10],
		global.MD5_result[11],
		global.MD5_result[12],
		global.MD5_result[13],
		global.MD5_result[14],
		global.MD5_result[15]);
#endif
#endif
	fclose(info_fp);
	return (0);
}
#endif

static void
CloseAudio(channels_val, nSamples, audio_out)
	int channels_val;
	unsigned long nSamples;
	struct soundfile *audio_out;
{
	/*
	 * define length
	 */
	audio_out->ExitSound(global.audio,
			(nSamples-global.SkippedSamples) *
			global.OutSampleSize*channels_val);

	close(global.audio);
	global.audio = -1;
}

static	unsigned int	track = 1;

/*
 * On terminating:
 * define size-related entries in audio file header, update and close file
 */
static void
CloseAll()
{
	WAIT_T	chld_return_status;
	int	amiparent;

	/*
	 * terminate child process first
	 */
	amiparent = child_pid > 0;

	if (global.iloop > 0) {
		/* set to zero */
		global.iloop = 0;
	}

#if	defined	HAVE_FORK_AND_SHAREDMEM
#ifdef DEBUG_CLEANUP
	fprintf(stderr, "%s terminating, \n", amiparent ?
		"Parent (READER)" : "Child (WRITER)");
#endif
#else
#ifdef DEBUG_CLEANUP
	fprintf(stderr, "Cdda2wav single process terminating, \n");
#endif
#endif

	if (amiparent || child_pid < 0) {
		/* switch to original mode and close device */
		EnableCdda(get_scsi_p(), 0, 0);
	}

	if (!amiparent) {
		/* do general clean up */

		if (global.audio >= 0) {
			if (bulk) {
				/* finish sample file for this track */
				CloseAudio(global.channels,
					global.nSamplesDoneInTrack,
					global.audio_out);
			} else {
				/* finish sample file for this track */
				CloseAudio(global.channels,
					(unsigned int) *nSamplesToDo,
					global.audio_out);
			}
		}

		/* tell minimum and maximum amplitudes, if required */
		if (global.findminmax) {
			fprintf(outfp,
			"Right channel: minimum amplitude :%d/-32768, maximum amplitude :%d/32767\n",
			global.minamp[0], global.maxamp[0]);
			fprintf(outfp,
			"Left  channel: minimum amplitude :%d/-32768, maximum amplitude :%d/32767\n",
			global.minamp[1], global.maxamp[1]);
		}

		/* tell mono or stereo recording, if required */
		if (global.findmono) {
			fprintf(outfp,
				"Audio samples are originally %s.\n",
				global.ismono ? "mono" : "stereo");
		}

		return;	/* end of child or single process */
	}


	if (global.have_forked == 1) {
#ifdef DEBUG_CLEANUP
		fprintf(stderr, "Parent wait for child death, \n");
#endif

		/* wait for child to terminate */
		if (0 > wait(&chld_return_status)) {
			errmsg("Error waiting for child.\n");
		} else {
			if (WIFEXITED(chld_return_status)) {
				if (WEXITSTATUS(chld_return_status)) {
					fprintf(stderr,
					"\nW Child exited with %d\n",
					WEXITSTATUS(chld_return_status));
				}
			}
			if (WIFSIGNALED(chld_return_status)) {
				fprintf(stderr,
				"\nW Child exited due to signal %d\n",
				WTERMSIG(chld_return_status));
			}
			if (WIFSTOPPED(chld_return_status)) {
				fprintf(stderr,
				"\nW Child is stopped due to signal %d\n",
				WSTOPSIG(chld_return_status));
			}
		}

#ifdef DEBUG_CLEANUP
		fprintf(stderr,
			"\nW Parent child death, state:%d\n",
			chld_return_status);
#endif
	}

#ifdef GPROF
	rename("gmon.out", "gmon.child");
#endif
}


/*
 * report a usage error and exit
 */
#ifdef  PROTOTYPES
static void
usage2(const char *szMessage, ...)
#else
static void
usage2(szMessage, va_alist)
	const char *szMessage;
	va_dcl
#endif
{
	va_list	marker;

#ifdef  PROTOTYPES
	va_start(marker, szMessage);
#else
	va_start(marker);
#endif

	errmsgno(EX_BAD, "%r", szMessage, marker);

	va_end(marker);
	fprintf(stderr,
		"\nPlease use -help or consult the man page for help.\n");

	exit(1);
}


/*
 * report a fatal error, clean up and exit
 */
#ifdef  PROTOTYPES
void
FatalError(int err, const char *szMessage, ...)
#else
void
FatalError(err, szMessage, va_alist)
	int		err;
	const char	*szMessage;
	va_dcl
#endif
{
	va_list marker;

#ifdef  PROTOTYPES
	va_start(marker, szMessage);
#else
	va_start(marker);
#endif

	errmsgno(err, "%r", szMessage, marker);

	va_end(marker);

	if (child_pid >= 0) {
		if (child_pid == 0) {
			pid_t	ppid;
			/*
			 * Kill the parent too if we are not orphaned.
			 */
			ppid = getppid();
			if (ppid > 1)
				kill(ppid, SIGINT);
		} else {
			kill(child_pid, SIGINT);
		}
	}
	exit(1);
}


/*
 * open the audio output file and prepare the header.
 * the header will be defined on terminating (when the size
 * is known). So hitting the interrupt key leaves an intact
 * file.
 */
static void
OpenAudio(fname, rate, nBitsPerSample, channels_val, expected_bytes, audio_out)
	char *fname;
	double rate;
	long nBitsPerSample;
	long channels_val;
	unsigned long expected_bytes;
	struct soundfile * audio_out;
{
	if (global.audio == -1) {
#ifdef	VMS
		static int	open_id = 3;
#endif
		global.audio = open(fname,
#ifdef SYNCHRONOUS_WRITE
				O_SYNC |
#endif
				O_CREAT | O_WRONLY | O_TRUNC | O_BINARY,
								(mode_t)0666);
		if (global.audio == -1) {
			if (errno == EAGAIN && is_fifo(fname)) {
				FatalError(errno,
				"Could not open fifo %s. Probably no fifo reader present.\n",
				fname);
			}
			FatalError(errno,
				"Could not open audio sample file %s.\n",
				fname);
		}
	}
	global.SkippedSamples = 0;
	any_signal = 0;
	audio_out->InitSound(global.audio, channels_val, (unsigned long)rate,
					nBitsPerSample, expected_bytes);

#ifdef MD5_SIGNATURES
	if (global.md5blocksize)
		MD5Init(&global.context);
	global.md5count = global.md5blocksize;
#endif
}

#include "scsi_cmds.h"

static int	RealEnd __PR((SCSI *scgp, UINT4 *buff));

static int
RealEnd(scgp, buff)
	SCSI	*scgp;
	UINT4	*buff;
{
	if (scg_cmd_err(scgp) != 0) {
		int c, k, q;

		k = scg_sense_key(scgp);
		c = scg_sense_code(scgp);
		q = scg_sense_qual(scgp);
		if ((k == 0x05	/* ILLEGAL_REQUEST */ &&
		    c  == 0x21	/* lba out of range */ &&
		    q  == 0x00) ||
		    (k == 0x05	/* ILLEGAL_REQUEST */ &&
		    c  == 0x63	/* end of user area encount. on this tr. */ &&
		    q  == 0x00) ||
		    (k == 0x08	/* BLANK_CHECK */ &&
		    c  == 0x64	/* illegal mode for this track */ &&
		    q  == 0x00)) {
			return (1);
		}
	}

	if (scg_getresid(scgp) > 16)
		return (1);

	{
		unsigned char *p;

		/* Look into the subchannel data */
		buff += CD_FRAMESAMPLES;
		p = (unsigned char *)buff;
		if (p[0] == 0x21 && p[1] == 0xaa) {
			return (1);
		}
	}
	return (0);
}

static void
set_offset(p, offset)
	myringbuff	*p;
	int		offset;
{
#ifdef DEBUG_SHM
	fprintf(stderr, "Write offset %d at %p\n", offset, &p->offset);
#endif
	p->offset = offset;
}


static int
get_offset(p)
	myringbuff	*p;
{
#ifdef DEBUG_SHM
	fprintf(stderr, "Read offset %d from %p\n", p->offset, &p->offset);
#endif
	return (p->offset);
}


static void
usage()
{
	/* BEGIN CSTYLED */
	fputs("usage: cdda2wav [OPTIONS ...] [trackfilenames ...]\n\
OPTIONS:\n\
        [-c chans] [-s] [-m] [-b bits] [-r rate] [-a divider] [-S speed] [-x]\n\
        [-t track[+endtrack]] [-i index] [-o offset] [-d duration] [-F] [-G]\n\
        [-q] [-w] [-v vopts] [-R] [-P overlap] [-B] [-T] [-C input-endianess]\n\
        [-e] [-n sectors] [-N] [-J] [-L cddbp-mode] [-H] [-g] [-l buffers] [-D cd-device]\n\
        [-I interface] [-K sound-device] [-O audiotype] [-E output-endianess]\n\
        [-A auxdevice] [-paranoia] [-cddbp-server=name] [-cddbp-port=port] [-version]\n",
		stderr);
	/* END CSTYLED */

	/* BEGIN CSTYLED */
	fputs("\
  (-D) dev=device		set the cdrom or scsi device (as Bus,Id,Lun).\n\
       ts=#			set maximum transfer size for a single SCSI command\n\
  (-A) auxdevice=device		set the aux device (typically /dev/cdrom).\n\
  (-K) sound-device=device	set the sound device to use for -e (typically /dev/dsp).\n\
       out-fd=descriptor	set the file descriptor for general output to descriptor.\n\
       audio-fd=descriptor	set the file descriptor for '-' audio output.\n\
  (-I) interface=interface	specify the interface for cdrom access.\n\
        (generic_scsi or cooked_ioctl).\n\
  (-c) channels=channels	set 1 for mono, 2 or s for stereo (s: channels swapped).\n\
  (-s) -stereo			select stereo recording.\n\
  (-m) -mono			select mono recording.\n\
  (-x) -max			select maximum quality (stereo/16-bit/44.1 KHz).\n\
  (-b) bits=bits		set bits per sample per channel (8, 12 or 16 bits).\n\
  (-r) rate=rate		set rate in samples per second. -R gives all rates\n\
  (-a) divider=divider		set rate to 44100Hz / divider. -R gives all rates\n\
  (-R) -dump-rates		dump a table with all available sample rates\n\
  (-S) speed=speedfactor	set the cdrom drive to a given speed during reading\n\
  (-P) set-overlap=sectors	set amount of overlap sampling (default is 0)\n\
  (-n) sectors-per-request=secs	read 'sectors' sectors per request.\n\
  (-l) buffers-in-ring=buffers	use a ring buffer with 'buffers' elements.\n\
  (-t) track=track[+end track]	select start track (option. end track).\n\
  (-i) index=index		select start index.\n\
  (-o) offset=offset		start at 'offset' sectors behind start track/index.\n\
        one sector equivalents 1/75 second.\n\
       start-sector=sector	set absolute start sector.\n\
  (-O) output-format=audiotype	set to wav, au (sun), cdr (raw), aiff or aifc format.\n\
  (-C) cdrom-endianess=endian	set little, big or guess input sample endianess.\n\
  (-E) output-endianess=endian	set little or big output sample endianess.\n\
  (-d) duration=seconds		set recording time in seconds or 0 for whole track.\n\
  (-w) -wait			wait for audio signal, then start recording.\n\
  (-F) -find-extremes		find extrem amplitudes in samples.\n\
  (-G) -find-mono		find if input samples are mono.\n\
  (-T) -deemphasize		undo pre-emphasis in input samples.\n\
  (-e) -echo			echo audio data to sound device (see -K) SOUND_DEV.\n\
  (-v) verbose-level=optlist	controls verbosity (for a list use -vhelp).\n\
  (-N) -no-write		do not create audio sample files.\n\
  (-J) -info-only		give disc information only.\n\
  (-L) cddb=cddbpmode		do cddbp title lookups.\n\
        resolve multiple entries according to cddbpmode: 0=interactive, 1=first entry\n\
  (-H) -no-infofile		no info file generation.\n\
  (-g) -gui			generate special output suitable for gui frontends.\n\
  (-Q) -silent-scsi		do not print status of erreneous scsi-commands.\n\
       -scanbus			scan the SCSI bus and exit\n\
  (-M) md5=count		calculate MD-5 checksum for blocks of 'count' bytes.\n\
  (-q) -quiet			quiet operation, no screen output.\n\
  (-p) playback-realtime=perc	play (echo) audio pitched at perc percent (50%-200%).\n\
  (-V) -verbose-scsi		each option increases verbosity for SCSI commands.\n\
  (-h) -help			show this help screen.\n\
  (-B) -alltracks, -bulk	record each track into a seperate file.\n\
       -paranoia		use the lib paranoia for reading.\n\
       -paraopts=opts		set options for lib paranoia (see -paraopts=help).\n\
       -cddbp-server=servername	set the cddbp server to use for title lookups.\n\
       -cddbp-port=portnumber	set the cddbp port to use for title lookups.\n\
       -version			print version information.\n\
\n\
Please note: some short options will be phased out soon (disappear)!\n\
\n\
parameters: (optional) one or more file names or - for standard output.\n\
",
	stderr);
	/* END CSTYLED */

	fputs("Version ", stderr);
	fputs(VERSION, stderr);
	fputs(VERSION_OS, stderr);
	/* BEGIN CSTYLED */
	fprintf(stderr, "\n\
defaults	%s, %d bit, %d.%02d Hz, track 1, no offset, one track,\n",
		  CHANNELS-1?"stereo":"mono", BITS_P_S,
		 44100 / UNDERSAMPLING,
		 (4410000 / UNDERSAMPLING) % 100);

	fprintf(stderr, "\
          type %s '%s', don't wait for signal, not quiet,\n",
		AUDIOTYPE, FILENAME);
	fprintf(stderr, "\
          use %s, device %s, aux %s\n",
		DEF_INTERFACE, CD_DEVICE, AUX_DEVICE);
	/* END CSTYLED */
	exit(SYNTAX_ERROR);
}

static void
init_globals()
{
#ifdef	HISTORICAL_JUNK
	global.dev_name = CD_DEVICE;	/* device name */
#endif
	global.aux_name = AUX_DEVICE;	/* auxiliary cdrom device */
	global.out_fp = stderr;		
	strncpy(global.fname_base, FILENAME,
		sizeof (global.fname_base)); /* auxiliary cdrom device */
	global.have_forked = 0;		/* state variable for clean up */
	global.parent_died = 0;		/* state variable for clean up */
	global.audio    = -1;		/* audio file desc */
	global.cooked_fd  = -1;		/* cdrom file desc */
	global.no_file  =  0;		/* flag no_file */
	global.no_infofile  =  0;	/* flag no_infofile */
	global.no_cddbfile  =  0;	/* flag no_cddbfile */
	global.quiet	  =  0;		/* flag quiet */
	global.verbose  =  SHOW_TOC + SHOW_SUMMARY +
				SHOW_STARTPOSITIONS +
				SHOW_TITLES;	/* verbose level */
	global.scsi_silent = 0;
	global.scsi_verbose = 0;	/* SCSI verbose level */
	global.scanbus = 0;
	global.multiname = 0;		/* multiple file names given */
	global.sh_bits  =  0;		/* sh_bits: sample bit shift */
	global.Remainder =  0;		/* remainder */
	global.iloop    =  0;		/* todo counter */
	global.SkippedSamples =  0;	/* skipped samples */
	global.OutSampleSize  =  0;	/* output sample size */
	global.channels = CHANNELS;	/* output sound channels */
	global.nSamplesDoneInTrack = 0;	/* written samples in current track */
	global.buffers = 4;		/* buffers to use */
	global.bufsize = -1L;		/* The SCSI buffer size */
	global.nsectors = NSECTORS;	/* sectors to read in one request */
	global.overlap = 1;		/* amount of overlapping sectors */
	global.useroverlap = -1;	/* amt of overl. sect. user override */
	global.need_hostorder = 0;	/* processing needs samples in host endianess */
	global.in_lendian = -1;		/* input endianess from SetupSCSI() */
	global.outputendianess = NONE;	/* user specified output endianess */
	global.findminmax  =  0;	/* flag find extrem amplitudes */
#ifdef HAVE_LIMITS_H
	global.maxamp[0] = INT_MIN;	/* maximum amplitude */
	global.maxamp[1] = INT_MIN;	/* maximum amplitude */
	global.minamp[0] = INT_MAX;	/* minimum amplitude */
	global.minamp[1] = INT_MAX;	/* minimum amplitude */
#else
	global.maxamp[0] = -32768;	/* maximum amplitude */
	global.maxamp[1] = -32768;	/* maximum amplitude */
	global.minamp[0] = 32767;	/* minimum amplitude */
	global.minamp[1] = 32767;	/* minimum amplitude */
#endif
	global.speed = DEFAULT_SPEED;	/* use default */
	global.userspeed = -1;		/* speed user override */
	global.findmono  =  0;		/* flag find if samples are mono */
	global.ismono  =  1;		/* flag if samples are mono */
	global.swapchannels  =  0;	/* flag if channels shall be swapped */
	global.deemphasize  =  0;	/* flag undo pre-emphasis in samples */
	global.playback_rate = 100;	/* new fancy selectable sound output rate */
	global.gui  =  0;		/* flag plain formatting for guis */
	global.cddb_id = 0;		/* disc identifying id for CDDB database */
	global.cddb_revision = 0;	/* entry revision for CDDB database */
	global.cddb_year = 0;		/* disc identifying year for CDDB database */
	global.cddb_genre[0] = '\0';	/* disc identifying genre for CDDB database */
	global.cddbp = 0;		/* flag if titles shall be looked up from CDDBP */
	global.cddbp_server = 0;	/* user supplied CDDBP server */
	global.cddbp_port = 0;		/* user supplied CDDBP port */
	global.illleadout_cd = 0;	/* flag if illegal leadout is present */
	global.reads_illleadout = 0;	/* flag if cdrom drive reads cds with illegal leadouts */
	global.disctitle = NULL;
	global.creator = NULL;
	global.copyright_message = NULL;
	memset(global.tracktitle, 0, sizeof (global.tracktitle));
	memset(global.trackindexlist, 0, sizeof (global.trackindexlist));

	global.just_the_toc = 0;
#ifdef	USE_PARANOIA
	global.paranoia_parms.disable_paranoia =
	global.paranoia_parms.disable_extra_paranoia =
	global.paranoia_parms.disable_scratch_detect =
	global.paranoia_parms.disable_scratch_repair = 0;
	global.paranoia_parms.retries = 20;
	global.paranoia_parms.overlap = -1;
	global.paranoia_parms.mindynoverlap = -1;
	global.paranoia_parms.maxdynoverlap = -1;
#endif
}

#if !defined(HAVE_STRCASECMP) || (HAVE_STRCASECMP != 1)
#include <ctype.h>
static int strcasecmp __PR((const char *s1, const char *s2));
static int
strcasecmp(s1, s2)
	const char	*s1;
	const char	*s2;
{
	if (s1 && s2) {
		while (*s1 && *s2 && (tolower(*s1) - tolower(*s2) == 0)) {
			s1++;
			s2++;
		}
		if (*s1 == '\0' && *s2 == '\0')
			return (0);
		if (*s1 == '\0')
			return (-1);
		if (*s2 == '\0')
			return (+1);
		return (tolower(*s1) - tolower(*s2));
	}
	return (-1);
}
#endif

static int
is_fifo(filename)
	char	*filename;
{
#if	defined S_ISFIFO
	struct stat	statstruct;

	if (stat(filename, &statstruct)) {
		/*
		 * maybe the output file does not exist.
		 */
		if (errno == ENOENT)
			return (0);
		else
			comerr("Error during stat for output file\n");
	} else {
		if (S_ISFIFO(statstruct.st_mode)) {
			return (1);
		}
	}
	return (0);
#else
	return (0);
#endif
}


#if !defined(HAVE_STRTOUL) || (HAVE_STRTOUL != 1)
static unsigned int strtoul __PR((const char *s1, char **s2, int base));
static unsigned int
strtoul(s1, s2, base)
	const char	*s1;
	char		**s2;
	int		base;
{
	long retval;

	if (base == 10) {
		/* strip zeros in front */
		while (*s1 == '0')
			s1++;
	}
	if (s2 != NULL) {
		*s2 = astol(s1, &retval);
	} else {
		(void) astol(s1, &retval);
	}

	return ((unsigned long) retval);
}
#endif

static unsigned long SectorBurst;
#if (SENTINEL > CD_FRAMESIZE_RAW)
error block size for overlap check has to be < sector size
#endif


static void
switch_to_realtime_priority __PR((void));

#ifdef  HAVE_SYS_PRIOCNTL_H

#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
static void
switch_to_realtime_priority()
{
	pcinfo_t	info;
	pcparms_t	param;
	rtinfo_t	rtinfo;
	rtparms_t	rtparam;
	int		pid;

	pid = getpid();

	/*
	 * get info
	 */
	strcpy(info.pc_clname, "RT");
	if (-1 == priocntl(P_PID, pid, PC_GETCID, (void *)&info)) {
		errmsg("Cannot get priority class id priocntl(PC_GETCID)\n");
		goto prio_done;
	}

	memmove(&rtinfo, info.pc_clinfo, sizeof (rtinfo_t));

	/*
	 * set priority not to the max
	 */
	rtparam.rt_pri = rtinfo.rt_maxpri - 2;
	rtparam.rt_tqsecs = 0;
	rtparam.rt_tqnsecs = RT_TQDEF;
	param.pc_cid = info.pc_cid;
	memmove(param.pc_clparms, &rtparam, sizeof (rtparms_t));
	priv_on();
	needroot(0);
	if (-1 == priocntl(P_PID, pid, PC_SETPARMS, (void *)&param))
		errmsg(
		"Cannot set priority class parameters priocntl(PC_SETPARMS)\n");
prio_done:
	priv_off();
	dontneedroot();
}
#else
#if defined(_POSIX_PRIORITY_SCHEDULING) && _POSIX_PRIORITY_SCHEDULING -0 >= 0
#define	USE_POSIX_PRIORITY_SCHEDULING
#endif
#ifdef	USE_POSIX_PRIORITY_SCHEDULING
#include <sched.h>

static void
switch_to_realtime_priority()
{
#ifdef  _SC_PRIORITY_SCHEDULING
	if (sysconf(_SC_PRIORITY_SCHEDULING) == -1) {
		errmsg("WARNING: RR-scheduler not available, disabling.\n");
	} else
#endif
	{
	int sched_fifo_min, sched_fifo_max;
	struct sched_param sched_parms;

	sched_fifo_min = sched_get_priority_min(SCHED_FIFO);
	sched_fifo_max = sched_get_priority_max(SCHED_FIFO);
	sched_parms.sched_priority = sched_fifo_max - 1;
	priv_on();
	needroot(0);
	if (-1 == sched_setscheduler(getpid(), SCHED_FIFO, &sched_parms) &&
	    global.quiet != 1)
		errmsg("cannot set posix realtime scheduling policy\n");
	priv_off();
	dontneedroot();
	}
}
#else
#if defined(__CYGWIN32__)

/*
 * NOTE: Base.h from Cygwin-B20 has a second typedef for BOOL.
 *	 We define BOOL to make all local code use BOOL
 *	 from Windows.h and use the hidden __SBOOL for
 *	 our global interfaces.
 *
 * NOTE: windows.h from Cygwin-1.x includes a structure field named sample,
 *	 so me may not define our own 'sample' or need to #undef it now.
 *	 With a few nasty exceptions, Microsoft assumes that any global
 *	 defines or identifiers will begin with an Uppercase letter, so
 *	 there may be more of these problems in the future.
 *
 * NOTE: windows.h defines interface as an alias for struct, this
 *	 is used by COM/OLE2, I guess it is class on C++
 *	 We man need to #undef 'interface'
 */
#define	BOOL	WBOOL		/* This is the Win BOOL		*/
#define	format	__format	/* Avoid format parameter hides global ... */
#include <windows.h>
#undef format
#undef interface

static void
switch_to_realtime_priority()
{
	/*
	 * set priority class
	 */
	if (FALSE == SetPriorityClass(GetCurrentProcess(),
						REALTIME_PRIORITY_CLASS)) {
		/*
		 * XXX No errno?
		 */
		errmsgno(EX_BAD, "No realtime priority possible.\n");
		return;
	}

	/*
	 * set thread priority
	 */
	if (FALSE == SetThreadPriority(GetCurrentThread(),
						THREAD_PRIORITY_HIGHEST)) {
		/*
		 * XXX No errno?
		 */
		errmsgno(EX_BAD, "Could not set realtime priority.\n");
	}
}
#else
static void
switch_to_realtime_priority()
{
}
#endif
#endif
#endif

/* SCSI cleanup */
int on_exitscsi __PR((void *status));

int
on_exitscsi(status)
	void *status;
{
	exit((int)status);
	return (0);
}

/* wrapper for signal handler exit needed for Mac-OS-X */
static void exit_wrapper __PR((int status));

static void
exit_wrapper(status)
	int	status;
{
#if defined DEBUG_CLEANUP
	fprintf(stderr, "Exit(%d) for %s\n",
			status, child_pid == 0 ? "Child" : "Parent");
	fflush(stderr);
#endif

	if (child_pid != 0) {
		SCSI *scgp = get_scsi_p();
		if (scgp->running) {
			scgp->cb_fun = on_exitscsi;
			scgp->cb_arg = (void *)status;
		} else {
			on_exitscsi((void *)status);
		}
	} else {
		exit(status);
	}
}

/* signal handler for process communication */
static void set_nonforked __PR((int status));

/* ARGSUSED */
static void
set_nonforked(status)
	int	status;
{
	global.parent_died = 1;
#if defined DEBUG_CLEANUP
	fprintf(stderr, "SIGPIPE received from %s\n.",
			child_pid == 0 ? "Child" : "Parent");
#endif
	if (child_pid == 0) {
		pid_t	ppid;
		/*
		 * Kill the parent too if we are not orphaned.
		 */
		ppid = getppid();
		if (ppid > 1)
			kill(ppid, SIGINT);
	} else {
		kill(child_pid, SIGINT);
	}
	exit(SIGPIPE_ERROR);
}



#ifdef	USE_PARANOIA
static struct paranoia_statistics
{
	long	c_sector;
	long	v_sector;
	int	last_heartbeatstate;
	long	lasttime;
	char	heartbeat;
	int	minoverlap;
	int	curoverlap;
	int	maxoverlap;
	int	slevel;
	int	slastlevel;
	int	stimeout;
	int	rip_smile_level;
	unsigned verifies;
	unsigned reads;
	unsigned fixup_edges;
	unsigned fixup_atoms;
	unsigned readerrs;
	unsigned skips;
	unsigned overlaps;
	unsigned scratchs;
	unsigned drifts;
	unsigned fixup_droppeds;
	unsigned fixup_dupeds;
}	*para_stat;


static void paranoia_reset __PR((void));
static void
paranoia_reset()
{
	para_stat->c_sector = 0;
	para_stat->v_sector = 0;
	para_stat->last_heartbeatstate = 0;
	para_stat->lasttime = 0;
	para_stat->heartbeat = ' ';
	para_stat->minoverlap = 0x7FFFFFFF;
	para_stat->curoverlap = 0;
	para_stat->maxoverlap = 0;
	para_stat->slevel = 0;
	para_stat->slastlevel = 0;
	para_stat->stimeout = 0;
	para_stat->rip_smile_level = 0;
	para_stat->verifies = 0;
	para_stat->reads = 0;
	para_stat->readerrs = 0;
	para_stat->fixup_edges = 0;
	para_stat->fixup_atoms = 0;
	para_stat->fixup_droppeds = 0;
	para_stat->fixup_dupeds = 0;
	para_stat->drifts = 0;
	para_stat->scratchs = 0;
	para_stat->overlaps = 0;
	para_stat->skips = 0;
}

static void paranoia_callback __PR((long inpos, int function));

static void
paranoia_callback(inpos, function)
	long	inpos;
	int	function;
{
	struct timeval thistime;
	long	test;

	switch (function) {
		case	-2:
			para_stat->v_sector = inpos / CD_FRAMEWORDS;
			return;
		case	-1:
			para_stat->last_heartbeatstate = 8;
			para_stat->heartbeat = '*';
			para_stat->slevel = 0;
			para_stat->v_sector = inpos / CD_FRAMEWORDS;
		break;
		case	PARANOIA_CB_VERIFY:
			if (para_stat->stimeout >= 30) {
				if (para_stat->curoverlap > CD_FRAMEWORDS) {
					para_stat->slevel = 2;
				} else {
					para_stat->slevel = 1;
				}
			}
			para_stat->verifies++;
		break;
		case	PARANOIA_CB_READ:
			if (inpos / CD_FRAMEWORDS > para_stat->c_sector) {
				para_stat->c_sector = inpos / CD_FRAMEWORDS;
			}
			para_stat->reads++;
		break;
		case	PARANOIA_CB_FIXUP_EDGE:
			if (para_stat->stimeout >= 5) {
				if (para_stat->curoverlap > CD_FRAMEWORDS) {
					para_stat->slevel = 2;
				} else {
					para_stat->slevel = 1;
				}
			}
			para_stat->fixup_edges++;
		break;
		case	PARANOIA_CB_FIXUP_ATOM:
			if (para_stat->slevel < 3 || para_stat->stimeout > 5) {
				para_stat->slevel = 3;
			}
			para_stat->fixup_atoms++;
		break;
		case	PARANOIA_CB_READERR:
			para_stat->slevel = 6;
			para_stat->readerrs++;
		break;
		case	PARANOIA_CB_SKIP:
			para_stat->slevel = 8;
			para_stat->skips++;
		break;
		case	PARANOIA_CB_OVERLAP:
			para_stat->curoverlap = inpos;
			if (inpos > para_stat->maxoverlap)
				para_stat->maxoverlap = inpos;
			if (inpos < para_stat->minoverlap)
				para_stat->minoverlap = inpos;
			para_stat->overlaps++;
		break;
		case	PARANOIA_CB_SCRATCH:
			para_stat->slevel = 7;
			para_stat->scratchs++;
		break;
		case	PARANOIA_CB_DRIFT:
			if (para_stat->slevel < 4 || para_stat->stimeout > 5) {
				para_stat->slevel = 4;
			}
			para_stat->drifts++;
		break;
		case	PARANOIA_CB_FIXUP_DROPPED:
			para_stat->slevel = 5;
			para_stat->fixup_droppeds++;
		break;
		case	PARANOIA_CB_FIXUP_DUPED:
			para_stat->slevel = 5;
			para_stat->fixup_dupeds++;
		break;
	}

	gettimeofday(&thistime, NULL);
	/* now in tenth of seconds. */
	test = thistime.tv_sec * 10 + thistime.tv_usec / 100000;

	if (para_stat->lasttime != test ||
	    function == -1 ||
	    para_stat->slastlevel != para_stat->slevel) {

		if (function == -1 ||
		    para_stat->slastlevel != para_stat->slevel) {

			static const char hstates[] = " .o0O0o.";

			para_stat->lasttime = test;
			para_stat->stimeout++;

			para_stat->last_heartbeatstate++;
			if (para_stat->last_heartbeatstate > 7) {
				para_stat->last_heartbeatstate = 0;
			}
			para_stat->heartbeat =
				hstates[para_stat->last_heartbeatstate];

			if (function == -1) {
				para_stat->heartbeat = '*';
			}
		}

		if (para_stat->slastlevel != para_stat->slevel) {
			para_stat->stimeout = 0;
		}
		para_stat->slastlevel = para_stat->slevel;
	}

	if (para_stat->slevel < 8) {
		para_stat->rip_smile_level = para_stat->slevel;
	} else {
		para_stat->rip_smile_level = 0;
	}
}
#endif

static long		lSector;
static long		lSector_p2;
static double		rate = 44100.0 / UNDERSAMPLING;
static int		bits = BITS_P_S;
static char		fname[200];
static const char	*audio_type;
static long		BeginAtSample;
static unsigned long	SamplesToWrite;
static unsigned		minover;
static unsigned		maxover;

static unsigned long calc_SectorBurst __PR((void));
static unsigned long
calc_SectorBurst()
{
	unsigned long	SectorBurstVal;

	SectorBurstVal = min(global.nsectors,
			(global.iloop + CD_FRAMESAMPLES-1) / CD_FRAMESAMPLES);
	if (lSector+(int)SectorBurst-1 >= lSector_p2)
		SectorBurstVal = lSector_p2 - lSector;
	return (SectorBurstVal);
}

/*
 * if PERCENTAGE_PER_TRACK is defined, the percentage message will reach
 * 100% every time a track end is reached or the time limit is reached.
 *
 * Otherwise if PERCENTAGE_PER_TRACK is not defined, the percentage message
 * will reach 100% once at the very end of the last track.
 */
#define	PERCENTAGE_PER_TRACK

static int do_read __PR((myringbuff *p, unsigned *total_unsuccessful_retries));
static int
do_read(p, total_unsuccessful_retries)
	myringbuff	*p;
	unsigned	*total_unsuccessful_retries;
{
	unsigned char *newbuf;
	int offset;
	unsigned int added_size;

	/* how many sectors should be read */
	SectorBurst =  calc_SectorBurst();

#ifdef	USE_PARANOIA
	if (global.paranoia_selected) {
		int i;

		for (i = 0; i < SectorBurst; i++) {
			void *dp;

			dp = paranoia_read_limited(global.cdp,
				paranoia_callback,
				global.paranoia_parms.retries);
#ifdef	nonono
			{
				char *err;
				char *msg;
				err = cdda_errors(global.cdp);
				msg = cdda_messages(global.cdp);
				if (err) {
					fputs(err, stderr);
					free(err);
				}
				if (msg) {
					fputs(msg, stderr);
					free(msg);
				}
			}
#endif	/* nonono */
			if (dp != NULL)	{
				memcpy(p->data + i*CD_FRAMESAMPLES, dp,
					CD_FRAMESIZE_RAW);
			} else {
				errmsgno(EX_BAD,
					"E unrecoverable error!\n");
				exit(READ_ERROR);
			}
		}
		newbuf = (unsigned char *)p->data;
		offset = 0;
		set_offset(p, offset);
		added_size = SectorBurst * CD_FRAMESAMPLES;
		global.overlap = 0;
		handle_inputendianess(p->data, added_size);
	} else
#endif
	{
		unsigned int retry_count;
#define	MAX_READRETRY 12

		retry_count = 0;
		do {
			SCSI *scgp = get_scsi_p();
			int retval;
#ifdef DEBUG_READS
			fprintf(stderr,
				"reading from %lu to %lu, overlap %u\n",
				lSector, lSector + SectorBurst -1,
				global.overlap);
#endif

#ifdef DEBUG_BUFFER_ADDRESSES
			fprintf(stderr, "%p %l\n", p->data, global.pagesize);
			if (((unsigned)p->data) & (global.pagesize -1) != 0) {
				fprintf(stderr,
					"Address %p is NOT page aligned!!\n",
					p->data);
			}
#endif

			if (global.reads_illleadout != 0 &&
			    lSector > Get_StartSector(LastTrack())) {
				int	singles = 0;
				UINT4	bufferSub[CD_FRAMESAMPLES + 24];

				/*
				 * we switch to single sector reads,
				 * in order to handle the remaining sectors.
				 */
				scgp->silent++;
				do {
					int retval2 = ReadCdRomSub(scgp,
							bufferSub,
							lSector+singles, 1);
					*eorecording = RealEnd(scgp,
							bufferSub);
					if (*eorecording) {
						break;
					}
					memcpy(p->data+singles*CD_FRAMESAMPLES,
						bufferSub, CD_FRAMESIZE_RAW);
					singles++;
				} while (singles < SectorBurst);
				scgp->silent--;

				if (*eorecording) {
					patch_real_end(lSector+singles);
					SectorBurst = singles;
#if	DEBUG_ILLLEADOUT
				fprintf(stderr,
				"iloop=%11lu, nSamplesToDo=%11lu, end=%lu -->\n",
					global.iloop,
					*nSamplesToDo,
					lSector+singles);
#endif

					*nSamplesToDo -= global.iloop -
							SectorBurst *
							CD_FRAMESAMPLES;
					global.iloop = SectorBurst *
							CD_FRAMESAMPLES;
#if	DEBUG_ILLLEADOUT
				fprintf(stderr,
				"iloop=%11lu, nSamplesToDo=%11lu\n\n",
					global.iloop, *nSamplesToDo);
#endif

				}
			} else {
				retval = ReadCdRom(scgp, p->data, lSector,
								SectorBurst);
			}
			handle_inputendianess(p->data,
						SectorBurst * CD_FRAMESAMPLES);
			if (NULL ==
				(newbuf = synchronize(p->data,
						SectorBurst*CD_FRAMESAMPLES,
						*nSamplesToDo-global.iloop))) {
				/*
				 * could not synchronize!
				 * Try to invalidate the cdrom cache.
				 * Increase overlap setting, if possible.
				 */
#ifdef				nonono
				trash_cache(p->data, lSector, SectorBurst);
#endif
				if (global.overlap < global.nsectors - 1) {
					global.overlap++;
					lSector--;
					SectorBurst = calc_SectorBurst();
#ifdef DEBUG_DYN_OVERLAP
					fprintf(stderr,
					"using increased overlap of %u\n",
						global.overlap);
#endif
				} else {
					lSector += global.overlap - 1;
					global.overlap = 1;
					SectorBurst =  calc_SectorBurst();
				}
			} else
				break;
		} while (++retry_count < MAX_READRETRY);

		if (retry_count == MAX_READRETRY && newbuf == NULL &&
		    global.verbose != 0) {
			(*total_unsuccessful_retries)++;
		}

		if (newbuf) {
			offset = newbuf - ((unsigned char *)p->data);
		} else {
			offset = global.overlap * CD_FRAMESIZE_RAW;
		}
		set_offset(p, offset);

		/* how much has been added? */
		added_size = SectorBurst * CD_FRAMESAMPLES - offset/4;

		if (newbuf && *nSamplesToDo != global.iloop) {
			minover = min(global.overlap, minover);
			maxover = max(global.overlap, maxover);


			/* should we reduce the overlap setting ? */
			if (offset > CD_FRAMESIZE_RAW && global.overlap > 1) {
#ifdef DEBUG_DYN_OVERLAP
				fprintf(stderr,
				"decreasing overlap from %u to %u (jitter %d)\n",
				global.overlap, global.overlap-1,
				offset - (global.overlap)*CD_FRAMESIZE_RAW);
#endif
				global.overlap--;
				SectorBurst =  calc_SectorBurst();
			}
		}
	}
	if (global.iloop >= added_size) {
		global.iloop -= added_size;
	} else {
		global.iloop = 0;
	}

	lSector += SectorBurst - global.overlap;

#if	defined	PERCENTAGE_PER_TRACK && defined HAVE_FORK_AND_SHAREDMEM
	{
		int as;
		while ((as = Get_StartSector(current_track+1)) != -1 &&
							lSector >= as) {
			current_track++;
		}
	}
#endif

	return (offset);
}

static void
print_percentage __PR((unsigned *poper, int c_offset));

static void
print_percentage(poper, c_offset)
	unsigned *poper;
	int	c_offset;
{
	unsigned per;
#ifdef	PERCENTAGE_PER_TRACK
	/* Thomas Niederreiter wants percentage per track */
	unsigned start_in_track = max(BeginAtSample,
		Get_AudioStartSector(current_track)*CD_FRAMESAMPLES);

	per = min(BeginAtSample + (long)*nSamplesToDo,
		Get_StartSector(current_track+1)*CD_FRAMESAMPLES)
		- (long)start_in_track;

	per = (BeginAtSample+nSamplesDone - start_in_track)/(per/100);

#else
	per = global.iloop ? (nSamplesDone)/(*nSamplesToDo/100) : 100;
#endif

	if (global.overlap > 0) {
		fprintf(outfp, "\r%2d/%2d/%2d/%7d %3d%%",
			minover, maxover, global.overlap,
			c_offset - global.overlap*CD_FRAMESIZE_RAW,
			per);
	} else if (*poper != per) {
		fprintf(outfp, "\r%3d%%", per);
	}
	*poper = per;
	fflush(outfp);
}

static unsigned long do_write __PR((myringbuff *p));
static unsigned long
do_write(p)
	myringbuff	*p;
{
	int current_offset;
	unsigned int InSamples;
	static unsigned oper = 200;

	current_offset = get_offset(p);

	/* how many bytes are available? */
	InSamples = global.nsectors*CD_FRAMESAMPLES - current_offset/4;
	/* how many samples are wanted? */
	InSamples = min((*nSamplesToDo-nSamplesDone), InSamples);

	/* when track end is reached, close current file and start a new one */
	while ((nSamplesDone < *nSamplesToDo) && (InSamples != 0)) {
		long unsigned int how_much = InSamples;

		long int left_in_track;
		left_in_track  = Get_StartSector(current_track+1) *
					CD_FRAMESAMPLES
					- (int)(BeginAtSample+nSamplesDone);

		if (*eorecording != 0 && current_track == cdtracks+1 &&
		    (*total_segments_read) == (*total_segments_written)+1) {
			/*
			 * limit, if the actual end of the last track is
			 * not known from the toc.
			 */
			left_in_track = InSamples;
		}

		if (left_in_track < 0) {
			errmsgno(EX_BAD,
			"internal error: negative left_in_track:%ld, current_track=%d\n",
				left_in_track, current_track);
		}

		if (bulk) {
			how_much = min(how_much,
						(unsigned long) left_in_track);
		}

#ifdef MD5_SIGNATURES
		if (global.md5count) {
			MD5Update(&global.context,
				((unsigned char *)p->data) +current_offset,
				min(global.md5count, how_much));
			global.md5count -= min(global.md5count, how_much);
		}
#endif
		if (SaveBuffer(p->data + current_offset/4,
						how_much,
						&nSamplesDone)) {
			if (global.have_forked == 1) {
				pid_t	ppid;
				/*
				 * Kill the parent too if we are not orphaned.
				 */
				ppid = getppid();
				if (ppid > 1)
					kill(ppid, SIGINT);
			}
			exit(WRITE_ERROR);
		}

		global.nSamplesDoneInTrack += how_much;
		SamplesToWrite -= how_much;

		/* move residual samples upto buffer start */
		if (how_much < InSamples) {
			memmove(
				(char *)(p->data) + current_offset,
				(char *)(p->data) + current_offset +
				how_much * 4,
				(InSamples - how_much) * 4);
		}

		if ((unsigned long) left_in_track <= InSamples ||
		    SamplesToWrite == 0) {
			/*
			 * the current portion to be handled is
			 * the end of a track
			 */

			if (bulk) {
				/* finish sample file for this track */
				CloseAudio(global.channels,
					global.nSamplesDoneInTrack,
					global.audio_out);
			} else if (SamplesToWrite == 0) {
				/* finish sample file for this track */
				CloseAudio(global.channels,
					(unsigned int) *nSamplesToDo,
					global.audio_out);
			}

			if (global.verbose) {
#ifdef	USE_PARANOIA
				double	f;
#endif
				print_percentage(&oper, current_offset);
				fputc(' ', outfp);
#ifndef	THOMAS_SCHAU_MAL
				if ((unsigned long)left_in_track > InSamples) {
					fputs(" incomplete", outfp);
				}
#endif
				if (global.tracktitle[current_track] != NULL) {
					fprintf(outfp,
					" track %2u '%s' recorded",
					current_track,
					global.tracktitle[current_track]);
				} else {
					fprintf(outfp,
						" track %2u recorded",
						current_track);
				}
#ifdef	USE_PARANOIA
				oper = para_stat->readerrs + para_stat->skips +
						para_stat->fixup_edges +
						para_stat->fixup_atoms +
						para_stat->fixup_droppeds +
						para_stat->fixup_dupeds +
						para_stat->drifts;
				f = (100.0 * oper) /
				(((double)global.nSamplesDoneInTrack)/588.0);

				if (para_stat->readerrs) {
					fprintf(outfp,
						" with audible hard errors");
				} else if ((para_stat->skips) > 0) {
					fprintf(outfp,
						" with %sretry/skip errors",
						f < 2.0 ? "":"audible ");
				} else if (oper > 0) {
					oper = f;

					fprintf(outfp, " with ");
					if (oper < 2)
						fprintf(outfp, "minor");
					else if (oper < 10)
						fprintf(outfp, "medium");
					else if (oper < 67)
						fprintf(outfp,
							"noticable audible");
					else if (oper < 100)
						fprintf(outfp,
							"major audible");
					else
						fprintf(outfp,
							"extreme audible");
					fprintf(outfp, " problems");
				} else {
					fprintf(outfp, " successfully");
				}
				if (f >= 0.1) {
					fprintf(outfp,
					" (%.1f%% problem sectors)", f);
				}
#else
				fprintf(outfp, " successfully");
#endif

				if (waitforsignal == 1) {
					fprintf(outfp,
					". %d silent samples omitted",
						global.SkippedSamples);
				}
				fputs("\n", outfp);

				if (global.reads_illleadout &&
				    *eorecording == 1) {
					fprintf(outfp,
					"Real lead out at: %ld sectors\n",
					(*nSamplesToDo+BeginAtSample)/CD_FRAMESAMPLES);
				}
#ifdef	USE_PARANOIA
				if (global.paranoia_selected) {
					oper = 200;	/* force new output */
					print_percentage(&oper, current_offset);
					if (para_stat->minoverlap == 0x7FFFFFFF)
						para_stat->minoverlap = 0;
					fprintf(outfp,
						"  %u rderr, %u skip, %u atom, %u edge, %u drop, %u dup, %u drift\n",
						para_stat->readerrs,
						para_stat->skips,
						para_stat->fixup_atoms,
						para_stat->fixup_edges,
						para_stat->fixup_droppeds,
						para_stat->fixup_dupeds,
						para_stat->drifts);
					oper = 200;	/* force new output */
					print_percentage(&oper, current_offset);
					fprintf(outfp,
						"  %u overlap(%.4g .. %.4g)\n",
						para_stat->overlaps,
						(float)para_stat->minoverlap /
						    (2352.0/2.0),
						(float)para_stat->maxoverlap /
						    (2352.0/2.0));
					paranoia_reset();
				}
#endif
			}

			global.nSamplesDoneInTrack = 0;
			if (bulk && SamplesToWrite > 0) {
				if (!global.no_file) {
					char *tmp_fname;

					/* build next filename */
					tmp_fname = get_next_name();
					if (tmp_fname != NULL) {
						strncpy(global.fname_base,
							tmp_fname,
							sizeof (global.fname_base));
						global.fname_base[
							sizeof (global.fname_base)-1] =
							'\0';
					}

					tmp_fname = cut_extension(global.fname_base);
					tmp_fname[0] = '\0';

					if (global.multiname == 0) {
						sprintf(fname, "%s_%02u.%s",
							global.fname_base,
							current_track+1,
							audio_type);
					} else {
						sprintf(fname, "%s.%s",
							global.fname_base,
							audio_type);
					}

					OpenAudio(fname, rate, bits,
						global.channels,
						(Get_AudioStartSector(current_track+1) -
						Get_AudioStartSector(current_track))
						*CD_FRAMESIZE_RAW,
						global.audio_out);
				} /* global.nofile */
			} /* if (bulk && SamplesToWrite > 0) */
			current_track++;

		} /* left_in_track <= InSamples */
		InSamples -= how_much;

	}  /* end while */
	if (!global.quiet && *nSamplesToDo != nSamplesDone) {
		print_percentage(&oper, current_offset);
	}
	return (nSamplesDone);
}

#define	PRINT_OVERLAP_INIT \
	if (global.verbose) { \
		if (global.overlap > 0) \
			fprintf(outfp, "overlap:min/max/cur, jitter, percent_done:\n  /  /  /          0%%"); \
		else \
			fputs("percent_done:\n  0%", outfp); \
	}

#if defined HAVE_FORK_AND_SHAREDMEM
static void forked_read __PR((void));

/*
 * This function does all audio cdrom reads
 * until there is nothing more to do
 */
static void
forked_read()
{
	unsigned	total_unsuccessful_retries = 0;

#if !defined(HAVE_SEMGET) || !defined(USE_SEMAPHORES)
	init_child();
#endif

	minover = global.nsectors;

	PRINT_OVERLAP_INIT
	while (global.iloop) {

		do_read(get_next_buffer(), &total_unsuccessful_retries);

		define_buffer();
	}

	if (total_unsuccessful_retries) {
		fprintf(stderr,
			"%u unsuccessful matches while reading\n",
			total_unsuccessful_retries);
	}
}

static void forked_write __PR((void));

static void
forked_write()
{

	/*
	 * don't need these anymore.  Good security policy says we get rid
	 * of them ASAP
	 */
	priv_off();
	neverneedroot();
	neverneedgroup();

#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
#else
	init_parent();
#endif

	for (; nSamplesDone < *nSamplesToDo; ) {
		if (*eorecording == 1 &&
		    (*total_segments_read) == (*total_segments_written))
			break;

		/*
		 * get oldest buffers
		 */
		nSamplesDone = do_write(get_oldest_buffer());

		drop_buffer();
	}
}
#endif

/*
 * This function implements the read and write calls in one loop (in case
 * there is no fork/thread_create system call).
 * This means reads and writes have to wait for each other to complete.
 */
static void nonforked_loop __PR((void));

static void
nonforked_loop()
{
	unsigned	total_unsuccessful_retries = 0;

	minover = global.nsectors;

	PRINT_OVERLAP_INIT
	while (global.iloop) {

		do_read(get_next_buffer(), &total_unsuccessful_retries);

		do_write(get_oldest_buffer());
	}

	if (total_unsuccessful_retries) {
		fprintf(stderr, "%u unsuccessful matches while reading\n", total_unsuccessful_retries);
	}
}

void verbose_usage __PR((void));

void
verbose_usage()
{
	fputs("\
	help		lists all verbose options.\n\
	disable		disables verbose mode.\n\
	all		enables all verbose options.\n\
	toc		display the table of contents.\n\
	summary		display a summary of track parameters.\n\
	indices		retrieve/display index positions.\n\
	catalog		retrieve/display media catalog number.\n\
	trackid		retrieve/display international standard recording code.\n\
	sectors		display the start sectors of each track.\n\
	titles		display any known track titles.\n\
", stderr);
}

#ifdef	USE_PARANOIA
void paranoia_usage __PR((void));

void
paranoia_usage()
{
	/* BEGIN CSTYLED */
	fputs("\
	help		lists all paranoia options.\n\
	disable		disables paranoia mode. Paranoia is still being used.\n\
	no-verify	switches verify off, and overlap on.\n\
	retries=amount	set the number of maximum retries per sector.\n\
	overlap=amount	set the number of sectors used for statical paranoia overlap.\n\
	minoverlap=amt	set the min. number of sectors used for dynamic paranoia overlap.\n\
	maxoverlap=amt	set the max. number of sectors used for dynamic paranoia overlap.\n\
",
		stderr);
	/* END CSTYLED */
}
#endif

int
handle_verbose_opts __PR((char *optstr, long *flagp));

int
handle_verbose_opts(optstr, flagp)
	char	*optstr;
	long	*flagp;
{
	char	*ep;
	char	*np;
	int	optlen;

	*flagp = 0;
	while (*optstr) {
		if ((ep = strchr(optstr, ',')) != NULL) {
			optlen = ep - optstr;
			np = ep + 1;
		} else {
			optlen = strlen(optstr);
			np = optstr + optlen;
		}
		if (strncmp(optstr, "toc", optlen) == 0) {
			*flagp |= SHOW_TOC;
		} else if (strncmp(optstr, "summary", optlen) == 0) {
			*flagp |= SHOW_SUMMARY;
		} else if (strncmp(optstr, "indices", optlen) == 0) {
			*flagp |= SHOW_INDICES;
		} else if (strncmp(optstr, "catalog", optlen) == 0) {
			*flagp |= SHOW_MCN;
		} else if (strncmp(optstr, "trackid", optlen) == 0) {
			*flagp |= SHOW_ISRC;
		} else if (strncmp(optstr, "sectors", optlen) == 0) {
			*flagp |= SHOW_STARTPOSITIONS;
		} else if (strncmp(optstr, "titles", optlen) == 0) {
			*flagp |= SHOW_TITLES;
		} else if (strncmp(optstr, "all", optlen) == 0) {
			*flagp |= SHOW_MAX;
		} else if (strncmp(optstr, "disable", optlen) == 0) {
			*flagp = 0;
		} else if (strncmp(optstr, "help", optlen) == 0) {
			verbose_usage();
			exit(NO_ERROR);
		} else {
			char	*endptr;
			unsigned arg = strtoul(optstr, &endptr, 10);
			if (optstr != endptr &&
			    arg <= SHOW_MAX) {
				*flagp |= arg;
				errmsgno(EX_BAD,
					"Warning: numerical parameters for -v are no more supported in the next releases!\n");
			} else {
				errmsgno(EX_BAD,
					"Unknown option '%s'.\n", optstr);
				verbose_usage();
				exit(SYNTAX_ERROR);
			}
		}
		optstr = np;
	}
	return (1);
}


int
handle_paranoia_opts __PR((char *optstr, long *flagp));

int
handle_paranoia_opts(optstr, flagp)
	char *optstr;
	long *flagp;
{
#ifdef	USE_PARANOIA
	char	*ep;
	char	*np;
	int	optlen;

	while (*optstr) {
		if ((ep = strchr(optstr, ',')) != NULL) {
			optlen = ep - optstr;
			np = ep + 1;
		} else {
			optlen = strlen(optstr);
			np = optstr + optlen;
		}
		if (strncmp(optstr, "retries=", min(8, optlen)) == 0) {
			char *eqp = strchr(optstr, '=');
			int   rets;

			astoi(eqp+1, &rets);
			if (rets >= 0) {
				global.paranoia_parms.retries = rets;
			}
		} else if (strncmp(optstr, "overlap=", min(8, optlen)) == 0) {
			char *eqp = strchr(optstr, '=');
			int   rets;

			astoi(eqp+1, &rets);
			if (rets >= 0) {
				global.paranoia_parms.overlap = rets;
			}
		} else if (strncmp(optstr, "minoverlap=",
						min(11, optlen)) == 0) {
			char *eqp = strchr(optstr, '=');
			int   rets;

			astoi(eqp+1, &rets);
			if (rets >= 0) {
				global.paranoia_parms.mindynoverlap = rets;
			}
		} else if (strncmp(optstr, "maxoverlap=",
						min(11, optlen)) == 0) {
			char *eqp = strchr(optstr, '=');
			int   rets;

			astoi(eqp+1, &rets);
			if (rets >= 0) {
				global.paranoia_parms.maxdynoverlap = rets;
			}
		} else if (strncmp(optstr, "no-verify", optlen) == 0) {
			global.paranoia_parms.disable_extra_paranoia = 1;
		} else if (strncmp(optstr, "disable", optlen) == 0) {
			global.paranoia_parms.disable_paranoia = 1;
		} else if (strncmp(optstr, "help", optlen) == 0) {
			paranoia_usage();
			exit(NO_ERROR);
		} else {
			errmsgno(EX_BAD, "Unknown option '%s'.\n", optstr);
			paranoia_usage();
			exit(SYNTAX_ERROR);
		}
		optstr = np;
	}
	return (1);
#else
	errmsgno(EX_BAD, "lib paranoia support is not configured!\n");
	return (0);
#endif
}


/*
 * and finally: the MAIN program
 */
int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	long		lSector_p1;
	long		sector_offset = 0;
	long		start_sector = -1;
	unsigned long	endtrack = 1;
	double		rectime = DURATION;
	int		cd_index = -1;
	double		int_part;
	int		littleendian = -1;
	char		*int_name;
static char		*user_sound_device = "";
	char		*env_p;
	int		tracks_included;
	int		moreargs;

	int_name = DEF_INTERFACE;
	audio_type = AUDIOTYPE;
	save_args(argc, argv);

	/*
	 * init global variables
	 */
	init_globals();
	{
		int	am_i_cdda2wav;

		/*
		 * When being invoked as list_audio_tracks, just dump a list of
		 * audio tracks.
		 */
		am_i_cdda2wav = !(strlen(argv[0]) >= sizeof ("list_audio_tracks")-1 &&
				strcmp(argv[0]+strlen(argv[0])+1-sizeof ("list_audio_tracks"), "list_audio_tracks") == 0);
		if (!am_i_cdda2wav)
			global.verbose = SHOW_JUSTAUDIOTRACKS;
	}
	/*
	 * Control those set-id privileges...
	 */
	initsecurity();

	env_p = getenv("CDDA_DEVICE");
	if (env_p != NULL) {
		global.dev_name = env_p;
	}

	env_p = getenv("CDDBP_SERVER");
	if (env_p != NULL) {
		global.cddbp_server = env_p;
	}

	env_p = getenv("CDDBP_PORT");
	if (env_p != NULL) {
		global.cddbp_port = env_p;
	}

{
	int	cac;
	char	*const*cav;

	BOOL	version = FALSE;
	BOOL	help = FALSE;
	char	*channels = NULL;
	int	irate = -1;
	char	*divider = NULL;
	char	*trackspec = NULL;
	char	*duration = NULL;

	char	*oendianess = NULL;
	char	*cendianess = NULL;
	int	cddbp = -1;
	BOOL	stereo = FALSE;
	BOOL	mono = FALSE;
	BOOL	domax = FALSE;
	BOOL	dump_rates = FALSE;
	int	userverbose = -1;
	long	paraopts = 0;
	int	outfd = -1;
	int	audiofd = -1;

	cac = argc;
	cav = argv;
	cac--;
	cav++;
	if (getargs(&cac, &cav, opts,
			&global.paranoia_selected,
			handle_paranoia_opts, &paraopts,
			&version,
			&help, &help,

			&global.no_file, &global.no_file,

			&dump_rates, &dump_rates,
			&bulk, &bulk, &bulk,
			&global.scsi_verbose, &global.scsi_verbose,

			&global.findminmax, &global.findminmax,
			&global.findmono, &global.findmono,
			&global.no_infofile, &global.no_infofile,

			&global.deemphasize, &global.deemphasize,
			&global.just_the_toc, &global.just_the_toc,
			&global.scsi_silent, &global.scsi_silent,

			&global.cddbp_server, &global.cddbp_port,
			&global.scanbus,
			&global.dev_name, &global.dev_name, &global.dev_name,
			getnum, &global.bufsize,
			&global.aux_name, &global.aux_name,
			&int_name, &int_name,
			&audio_type, &audio_type,

			&oendianess, &oendianess,
			&cendianess, &cendianess,
			&global.userspeed, &global.userspeed,

			&global.playback_rate, &global.playback_rate,
			&global.md5blocksize, &global.md5blocksize,
			&global.useroverlap, &global.useroverlap,
			&user_sound_device, &user_sound_device,

			&cddbp, &cddbp,
			&channels, &channels,
			&bits, &bits,
			&irate, &irate,
			&global.gui, &global.gui,

			&divider, &divider,
			&trackspec, &trackspec,
			&cd_index, &cd_index,
			&duration, &duration,
			&sector_offset, &sector_offset, &start_sector,

			&global.nsectors, &global.nsectors,
			handle_verbose_opts, &userverbose,
			handle_verbose_opts, &userverbose,
			&global.buffers, &global.buffers,

			&stereo, &stereo,
			&mono, &mono,
			&waitforsignal, &waitforsignal,
			&global.echo, &global.echo,
			&global.quiet, &global.quiet,
			&domax, &domax, &outfd, &audiofd) < 0) {
		errmsgno(EX_BAD, "Bad Option: %s.\n", cav[0]);
		fputs("use 'cdda2wav -help' to get more information.\n",
				stderr);
		exit(SYNTAX_ERROR);
	}
	if (getfiles(&cac, &cav, opts) == 0)
		/* No more file type arguments */;
	moreargs = cav - argv;
	if (version) {
#ifdef	OLD_VERSION_PRINT
		fputs("cdda2wav version ", outfp);
		fputs(VERSION, outfp);
		fputs(VERSION_OS, outfp);
		fputs("\n", outfp);
#else
		/*
		 * Make the version string similar for all cdrtools programs.
		 */
		printf("cdda2wav %s (%s-%s-%s) Copyright (C) 1993-2004 Heiko Eißfeldt (C) 2004-2008 Jörg Schilling\n",
					VERSION,
					HOST_CPU, HOST_VENDOR, HOST_OS);
#endif
		exit(NO_ERROR);
	}
	if (help) {
		usage();
	}
	if (outfd >= 0) {
#ifdef	F_GETFD
		if (fcntl(outfd, F_GETFD, 0) < 0)
			comerr("Cannot redirect output to fd %d.\n", outfd);
#endif
		global.out_fp = fdopen(outfd, "wa");
		if (global.out_fp == NULL)
			comerr("Cannot open output fd %d.\n", outfd);
	}
	if (!global.scanbus)
		cdr_defaults(&global.dev_name, NULL, NULL, &global.bufsize, NULL);
	if (global.bufsize < 0L)
		global.bufsize = DEF_BUFSIZE;	/* The SCSI buffer size */

	if (dump_rates) {	/* list available rates */
		int	ii;

		/* BEGIN CSTYLED */
		fputs("\
Available rates are:\n\
Rate   Divider      Rate   Divider      Rate   Divider      Rate   Divider\n\
",
			outfp);
		/* END CSTYLED */
		for (ii = 1; ii <= 44100 / 880 / 2; ii++) {
			long i2 = ii;
			fprintf(outfp, "%7.1f  %2ld         %7.1f  %2ld.5       ",
				44100.0/i2, i2, 44100.0/(i2+0.5), i2);
			i2 += 25;
			fprintf(outfp, "%7.1f  %2ld         %7.1f  %2ld.5\n",
				44100.0/i2, i2, 44100.0/(i2+0.5), i2);
			i2 -= 25;
		}
		exit(NO_ERROR);
	}
	if (channels) {
		if (*channels == 's') {
			global.channels = 2;
			global.swapchannels = 1;
		} else {
			global.channels = strtol(channels, NULL, 10);
		}
	}
	if (irate >= 0) {
		rate = irate;
	}
	if (divider) {
		double divider_d;
		divider_d = strtod(divider, NULL);
		if (divider_d > 0.0) {
			rate = 44100.0 / divider_d;
		} else {
			errmsgno(EX_BAD,
			"E option -divider requires a nonzero, positive argument.\nSee -dump-rates.\n");
			exit(SYNTAX_ERROR);
		}
	}
	if (sector_offset != 0 && start_sector != -1) {
		errmsgno(EX_BAD, "-offset and -start-sector are mutual exclusive.\n");
		exit(SYNTAX_ERROR);
	}
	if (trackspec) {
		char * endptr;
		char * endptr2;

		if (start_sector != -1) {
			errmsgno(EX_BAD, "-t and -start-sector are mutual exclusive.\n");
			exit(SYNTAX_ERROR);
		}
		track = strtoul(trackspec, &endptr, 10);
		endtrack = strtoul(endptr, &endptr2, 10);
		if (endptr2 == endptr) {
			endtrack = track;
		} else if (track == endtrack) {
			bulk = -1;
		}
	}
	if (duration) {
		char *end_ptr = NULL;
		rectime = strtod(duration, &end_ptr);
		if (*end_ptr == 'f') {
			rectime = rectime / 75.0;
			/* TODO: add an absolute end of recording. */
#if	0
		} else if (*end_ptr == 'F') {
			rectime = rectime / 75.0;
#endif
		} else if (*end_ptr != '\0') {
			rectime = -1.0;
		}
	}
	if (oendianess) {
		if (strcasecmp(oendianess, "little") == 0) {
			global.outputendianess = LITTLE;
		} else if (strcasecmp(oendianess, "big") == 0) {
			global.outputendianess = BIG;
		} else {
			usage2("Wrong parameter '%s' for option -E", oendianess);
		}
	}
	if (cendianess) {
		if (strcasecmp(cendianess, "little") == 0) {
			littleendian = 1;
		} else if (strcasecmp(cendianess, "big") == 0) {
			littleendian = 0;
		} else if (strcasecmp(cendianess, "guess") == 0) {
			littleendian = -2;
		} else {
			usage2("Wrong parameter '%s' for option -C", cendianess);
		}
	}
	if (cddbp >= 0) {
		global.cddbp = 1 + cddbp;
	}
	if (stereo) {
		global.channels = 2;
	}
	if (mono) {
		global.channels = 1;
		global.need_hostorder = 1;
	}
	if (global.echo) {
#ifdef	ECHO_TO_SOUNDCARD
		if (global.playback_rate != 100) {
			RestrictPlaybackRate(global.playback_rate);
		}
		global.need_hostorder = 1;
#else
		errmsgno(EX_BAD,
			"There is no sound support compiled into %s.\n",
			argv[0]);
		global.echo = 0;
#endif
	}
	if (global.quiet) {
		global.verbose = 0;
	}
	if (domax) {
		global.channels = 2; bits = 16; rate = 44100;
	}
	if (global.findminmax) {
		global.need_hostorder = 1;
	}
	if (global.deemphasize) {
		global.need_hostorder = 1;
	}
	if (global.just_the_toc) {
		global.verbose = SHOW_MAX;
		bulk = 1;
	}
	if (global.gui) {
#ifdef	Thomas_will_es
		global.no_file = 1;
		global.no_infofile = 1;
		global.verbose = SHOW_MAX;
#endif
		global.no_cddbfile = 1;
	}
	if (global.no_file) {
		global.no_infofile = 1;
		global.no_cddbfile = 1;
	}
	if (global.no_infofile) {
		global.no_cddbfile = 1;
	}
	if (global.md5blocksize) {
#ifdef	MD5_SIGNATURES
		errmsgno(EX_BAD,
			"MD5 signatures are currently broken! Sorry\n");
#else
		errmsgno(EX_BAD,
			"The option MD5 signatures is not configured!\n");
#endif
	}
	if (user_sound_device) {
#ifndef	ECHO_TO_SOUNDCARD
		errmsgno(EX_BAD, "There is no sound support configured!\n");
#endif
	}
	if (global.paranoia_selected) {
		global.useroverlap = 0;
	}
	if (userverbose >= 0) {
		global.verbose = userverbose;
	}
	/*
	 * check all parameters
	 */
	if (global.bufsize < CD_FRAMESIZE_RAW) {
		usage2("Incorrect transfer size setting: %d", global.bufsize);
	}

	if (global.buffers < 1) {
		usage2("Incorrect buffer setting: %d", global.buffers);
	}

	if (global.nsectors < 1) {
		usage2("Incorrect nsectors setting: %d", global.nsectors);
	}

	if (global.verbose < 0 || global.verbose > SHOW_MAX) {
		usage2("Incorrect verbose level setting: %d", global.verbose);
	}
	if (global.verbose == 0)
		global.quiet = 1;

	if (rectime < 0.0) {
		usage2("Incorrect recording time setting: %d.%02d",
			(int)rectime, (int)(rectime*100+0.5) % 100);
	}

	if (global.channels != 1 && global.channels != 2) {
		usage2("Incorrect channel setting: %d", global.channels);
	}

	if (bits != 8 && bits != 12 && bits != 16) {
		usage2("Incorrect bits_per_sample setting: %d", bits);
	}

	if (rate < 827.0 || rate > 44100.0) {
		usage2("Incorrect sample rate setting: %d.%02d",
			(int)rate, ((int)rate*100) % 100);
	}

	int_part = (double)(long) (2*44100.0 / rate);

	if (2*44100.0 / rate - int_part >= 0.5) {
		int_part += 1.0;
		fprintf(outfp,
			"Nearest available sample rate is %d.%02d Hertz\n",
			2*44100 / (int)int_part,
			(2*4410000 / (int)int_part) % 100);
	}
	Halved = ((int) int_part) & 1;
	rate = 2*44100.0 / int_part;
	undersampling = (int) int_part / 2.0;
	samples_to_do = undersampling;

	if (strcmp((char *)int_name, "generic_scsi") == 0) {
		interface = GENERIC_SCSI;
	} else if (strcmp((char *)int_name, "cooked_ioctl") == 0) {
		interface = COOKED_IOCTL;
	} else  {
		usage2("Incorrect interface setting: %s", int_name);
	}

	/*
	 * check * init audio file
	 */
	if (strncmp(audio_type, "wav", 3) == 0) {
		global.audio_out = &wavsound;
	} else if (strncmp(audio_type, "sun", 3) == 0 ||
		    strncmp(audio_type, "au", 2) == 0) {
		/*
		 * Enhanced compatibility
		 */
		audio_type = "au";
		global.audio_out = &sunsound;
	} else if (strncmp(audio_type, "cdr", 3) == 0||
		    strncmp(audio_type, "raw", 3) == 0) {
		global.audio_out = &rawsound;
	} else if (strncmp(audio_type, "aiff", 4) == 0) {
		global.audio_out = &aiffsound;
	} else if (strncmp(audio_type, "aifc", 4) == 0) {
		global.audio_out = &aifcsound;
#ifdef USE_LAME
	} else if (strncmp(audio_type, "mp3", 3) == 0) {
		global.audio_out = &mp3sound;
		if (!global.quiet) {
			unsigned char Lame_version[20];

			fetch_lame_version(Lame_version);
			fprintf(outfp,
				"Using LAME version %s.\n", Lame_version);
		}
		if (bits < 9) {
			bits = 16;
			fprintf(outfp,
			"Warning: sample size forced to 16 bit for MP3 format.\n");
		}
#endif /* USE_LAME */
	} else {
		usage2("Incorrect audio type setting: %3s", audio_type);
	}

	if (bulk == -1)
		bulk = 0;

	global.need_big_endian = global.audio_out->need_big_endian;
	if (global.outputendianess != NONE)
		global.need_big_endian = global.outputendianess == BIG;

	if (global.no_file)
		global.fname_base[0] = '\0';

	if (!bulk) {
		strcat(global.fname_base, ".");
		strcat(global.fname_base, audio_type);
	}

	/*
	 * If we need to calculate with samples or write them to a soundcard,
	 * we need a conversion to host byte order.
	 */
	if (global.channels != 2 ||
	    bits != 16 ||
	    rate != 44100) {
		global.need_hostorder = 1;
	}

	/*
	 * Bad hack!!
	 * Remove for release 2.0
	 * this is a bug compatibility feature.
	 */
	if (global.gui && global.verbose == SHOW_TOC)
		global.verbose |= SHOW_STARTPOSITIONS | SHOW_SUMMARY |
					SHOW_TITLES;

	/*
	 * all options processed.
	 * Now a file name per track may follow
	 */
	argc2 = argc3 = argc - moreargs;
	argv2 = argv + moreargs;
	if (moreargs < argc) {
		if (strcmp(argv[moreargs], "-") == 0) {
			if (audiofd >= 0) {
#ifdef	F_GETFD
				if (fcntl(audiofd, F_GETFD, 0) < 0) {
					comerr(
					"Cannot redirect audio data to fd %d.\n",
					audiofd);
				}
#endif
				global.audio = audiofd;
			} else {
				global.audio = dup(fileno(stdout));
			}
#ifdef	NEED_O_BINARY
			setmode(global.audio, O_BINARY);
#endif
			strncpy(global.fname_base, "standard_output",
						sizeof (global.fname_base));
			global.fname_base[sizeof (global.fname_base)-1] = 0;
		} else if (!is_fifo(argv[moreargs])) {
			/*
			 * we do have at least one argument
			 */
			global.multiname = 1;
		}
	}
}	/* End of getargs() local vars */

#define	SETSIGHAND(PROC, SIG, SIGNAME) if (signal(SIG, PROC) == SIG_ERR) \
	{ errmsg("Cannot set signal %s handler.\n", SIGNAME); exit(SETSIG_ERROR); }
	SETSIGHAND(exit_wrapper, SIGINT, "SIGINT")
	SETSIGHAND(exit_wrapper, SIGQUIT, "SIGQUIT")
	SETSIGHAND(exit_wrapper, SIGTERM, "SIGTERM")
	SETSIGHAND(exit_wrapper, SIGHUP, "SIGHUP")

	SETSIGHAND(set_nonforked, SIGPIPE, "SIGPIPE")

	/* setup interface and open cdrom device */
	/*
	 * request sychronization facilities and shared memory
	 */
	SetupInterface();

	/*
	 * use global.useroverlap to set our overlap
	 */
	if (global.useroverlap != -1)
		global.overlap = global.useroverlap;

	/*
	 * check for more valid option combinations
	 */
	if (global.nsectors < 1+global.overlap) {
		errmsgno(EX_BAD,
		"Warning: Setting #nsectors to minimum of %d, due to jitter correction!\n",
			global.overlap+1);
		global.nsectors = global.overlap+1;
	}

	if (global.overlap > 0 && global.buffers < 2) {
		errmsgno(EX_BAD,
		"Warning: Setting #buffers to minimum of 2, due to jitter correction!\n");
		global.buffers = 2;
	}

	/*
	 * Value of 'nsectors' must be defined here
	 */
	global.shmsize = 0;
#ifdef	USE_PARANOIA
	while (global.shmsize < sizeof (struct paranoia_statistics))
		global.shmsize += global.pagesize;
#endif
	global.shmsize += 10*global.pagesize;	/* XXX Der Speicherfehler ist nicht in libparanoia sondern in cdda2wav :-( */
	global.shmsize += HEADER_SIZE + ENTRY_SIZE_PAGE_AL * global.buffers;

#if	defined(HAVE_FORK_AND_SHAREDMEM)
	/*
	 * The (void *) cast is to avoid a GCC warning like:
	 * warning: dereferencing type-punned pointer will break
	 * strict-aliasing rules
	 * which does not apply to this code. (void *) introduces a compatible
	 * intermediate type in the cast list.
	 */
	he_fill_buffer = request_shm_sem(global.shmsize,
				(unsigned char **)(void *)&he_fill_buffer);
	if (he_fill_buffer == NULL) {
		errmsgno(EX_BAD, "No shared memory available!\n");
		exit(SHMMEM_ERROR);
	}
#else /* do not have fork() and shared memory */
	he_fill_buffer = malloc(global.shmsize);
	if (he_fill_buffer == NULL) {
		errmsg("No buffer memory available!\n");
		exit(NOMEM_ERROR);
	}
#endif
#ifdef	USE_PARANOIA
	{
		int	i = 0;

		para_stat = (struct paranoia_statistics *)he_fill_buffer;
		while (i < sizeof (struct paranoia_statistics)) {
			i += global.pagesize;
			he_fill_buffer += global.pagesize;
			global.shmsize -= global.pagesize;
		}
	}
#endif

	if (global.verbose != 0) {
		fprintf(outfp,
		"%u bytes buffer memory requested, transfer size %ld bytes, %d buffers, %d sectors\n",
			global.shmsize, global.bufsize, global.buffers, global.nsectors);
	}

	/*
	 * initialize pointers into shared memory segment
	 */
	last_buffer = he_fill_buffer + 1;
	total_segments_read = (unsigned long *) (last_buffer + 1);
	total_segments_written = total_segments_read + 1;
	child_waits = (int *) (total_segments_written + 1);
	parent_waits = child_waits + 1;
	in_lendian = parent_waits + 1;
	eorecording = in_lendian + 1;
	*total_segments_read = *total_segments_written = 0;
	nSamplesToDo = (unsigned long *)(eorecording + 1);
	*eorecording = 0;
	*in_lendian = global.in_lendian;

	set_total_buffers(global.buffers, sem_id);


#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
	atexit(free_sem);
#endif

	/*
	 * set input endian default
	 */
	if (littleendian != -1)
		*in_lendian = littleendian;

	/*
	 * get table of contents
	 */
	cdtracks = ReadToc();
	if (cdtracks == 0) {
		errmsgno(EX_BAD,
			"No track in table of contents! Aborting...\n");
		exit(MEDIA_ERROR);
	}

	calc_cddb_id();
	calc_cdindex_id();

#if	1
	Check_Toc();
#endif

	if (ReadTocText != NULL && FirstAudioTrack() != -1) {
		ReadTocText(get_scsi_p());
		handle_cdtext();
	}
	if (global.verbose == SHOW_JUSTAUDIOTRACKS) {
		unsigned int z;

		for (z = 0; z < cdtracks; z++) {
			if (Get_Datatrack(z) == 0) {
				printf("%02d\t%06ld\n",
					Get_Tracknumber(z),
					Get_AudioStartSector(z));
			}
		}
		exit(NO_ERROR);
	}

	if (global.verbose != 0) {
		fputs("#Cdda2wav version ", outfp);
		fputs(VERSION, outfp);
		fputs(VERSION_OS, outfp);
#if defined USE_POSIX_PRIORITY_SCHEDULING || defined HAVE_SYS_PRIOCNTL_H
		fputs(", real time sched.", outfp);
#endif
#if defined ECHO_TO_SOUNDCARD
		fputs(", soundcard", outfp);
#endif
#if defined USE_PARANOIA
		fputs(", libparanoia", outfp);
#endif
		fputs(" support\n", outfp);
	}

	FixupTOC(cdtracks + 1);

#if	0
	if (!global.paranoia_selected) {
		error("NICE\n");
		/*
		 * try to get some extra kicks
		 */
		priv_on();
		needroot(0);
#if defined HAVE_SETPRIORITY
		setpriority(PRIO_PROCESS, 0, -20);
#else
#if defined(HAVE_NICE) && (HAVE_NICE == 1)
		nice(-NZERO);
#endif
#endif
		priv_off();
		dontneedroot();
	}
#endif

	/*
	 * switch cdrom to audio mode
	 */
	EnableCdda(get_scsi_p(), 1, CD_FRAMESIZE_RAW);

	atexit(CloseAll);

	DisplayToc();
	if (FirstAudioTrack() == -1) {
		if (no_disguised_audiotracks()) {
			FatalError(EX_BAD,
				"This disk has no audio tracks.\n");
		}
	}

	Read_MCN_ISRC();

	/*
	 * check if start track is in range
	 */
	if (track < 1 || track > cdtracks) {
		usage2("Incorrect start track setting: %d", track);
	}

	/*
	 * check if end track is in range
	 */
	if (endtrack < track || endtrack > cdtracks) {
		usage2("Incorrect end track setting: %ld", endtrack);
	}

	/*
	 * Find track that is related to the absolute sector offset.
	 */
	if (start_sector != -1) {
		int	t;

		for (t = 1; t <= cdtracks; t++) {
			lSector = Get_AudioStartSector(t);
			lSector_p1 = Get_EndSector(t) + 1;
			if (lSector < 0)
				continue;
			if (start_sector >= lSector &&
			    start_sector < lSector_p1) {
				track = t;
				sector_offset = start_sector - lSector;
			}
			if (bulk)
				endtrack = t;
		}
	}

	do {
		lSector = Get_AudioStartSector(track);
		lSector_p1 = Get_EndSector(track) + 1;

		if (lSector < 0) {
			if (bulk == 0) {
				FatalError(EX_BAD,
					"Track %d not found.\n", track);
			} else {
				fprintf(outfp,
					"Skipping data track %d...\n", track);
				if (endtrack == track)
					endtrack++;
				track++;
			}
		}
	} while (bulk != 0 && track <= cdtracks && lSector < 0);

	if ((global.illleadout_cd == 0 || global.reads_illleadout != 0) &&
	    cd_index != -1) {
		if (global.verbose && !global.quiet) {
			global.verbose |= SHOW_INDICES;
		}
		sector_offset += ScanIndices(track, cd_index, bulk);
	} else {
		cd_index = 1;
		if (global.deemphasize || (global.verbose & SHOW_INDICES)) {
			ScanIndices(track, cd_index, bulk);
		}
	}

	lSector += sector_offset;
	/*
	 * check against end sector of track
	 */
	if (lSector >= lSector_p1) {
		fprintf(stderr,
		"W Sector offset %lu exceeds track size (ignored)\n",
			sector_offset);
		lSector -= sector_offset;
	}

	if (lSector < 0L) {
		fputs("Negative start sector! Set to zero.\n", stderr);
		lSector = 0L;
	}

	lSector_p2 = Get_LastSectorOnCd(track);
	if (bulk == 1 && track == endtrack && rectime == 0.0)
		rectime = 99999.0;
	if (rectime == 0.0) {
		/*
		 * set time to track time
		 */
		 *nSamplesToDo = (lSector_p1 - lSector) * CD_FRAMESAMPLES;
		rectime = (lSector_p1 - lSector) / 75.0;
		if (CheckTrackrange(track, endtrack) == 1) {
			lSector_p2 = Get_EndSector(endtrack) + 1;

			if (lSector_p2 >= 0) {
				rectime = (lSector_p2 - lSector) / 75.0;
				*nSamplesToDo = (long)(rectime*44100.0 + 0.5);
			} else {
				fputs(
				"End track is no valid audio track (ignored)\n",
					stderr);
			}
		} else {
			fputs(
			"Track range does not consist of audio tracks only (ignored)\n",
				stderr);
		}
	} else {
		/*
		 * Prepare the maximum recording duration.
		 * It is defined as the biggest amount of
		 * adjacent audio sectors beginning with the
		 * specified track/index/offset.
		 */
		if (rectime > (lSector_p2 - lSector) / 75.0) {
			rectime = (lSector_p2 - lSector) / 75.0;
			lSector_p1 = lSector_p2;
		}

		/* calculate # of samples to read */
		*nSamplesToDo = (long)(rectime*44100.0 + 0.5);
	}

	global.OutSampleSize = (1+bits/12);
	if (*nSamplesToDo/undersampling == 0L) {
		usage2("Time interval is too short. Choose a duration greater than %d.%02d secs!",
			undersampling/44100, (int)(undersampling/44100) % 100);
	}
	if (moreargs < argc) {
		if (strcmp(argv[moreargs], "-") == 0 ||
		    is_fifo(argv[moreargs])) {
			/*
			 * pipe mode
			 */
			if (bulk == 1) {
				fprintf(stderr,
				"W Bulk mode is disabled while outputting to a %spipe\n",
					is_fifo(argv[moreargs]) ?
							"named " : "");
				bulk = 0;
			}
			global.no_cddbfile = 1;
		}
	}
	if (global.no_infofile == 0) {
		global.no_infofile = 1;
		if (global.channels == 1 || bits != 16 || rate != 44100) {
			fprintf(stderr,
			"W Sample conversions disable generation of info files!\n");
		} else if (waitforsignal == 1) {
			fprintf(stderr,
			"W Option -w 'wait for signal' disables generation of info files!\n");
		} else if (sector_offset != 0) {
			fprintf(stderr,
			"W Using an start offset (option -o) disables generation of info files!\n");
		} else if (!bulk &&
			    !((lSector == Get_AudioStartSector(track)) &&
			    ((long)(lSector + rectime*75.0 + 0.5) ==
			    Get_EndSector(track) + 1))) {
			fprintf(stderr,
			"W Duration is not set for complete tracks (option -d), this disables generation\n  of info files!\n");
		} else {
			global.no_infofile = 0;
		}
	}

	SamplesToWrite = *nSamplesToDo*2/(int)int_part;

	{
		int first = FirstAudioTrack();

		tracks_included = Get_Track(
				(unsigned) (lSector +
					    *nSamplesToDo/CD_FRAMESAMPLES -1))
					    - max((int)track, first) +1;
	}

	if (global.multiname != 0 && moreargs + tracks_included > argc) {
		global.multiname = 0;
	}

	if (!waitforsignal) {

#ifdef INFOFILES
		if (!global.no_infofile) {
			int i;

			for (i = track; i < (int)track + tracks_included; i++) {
				unsigned minsec, maxsec;
				char *tmp_fname;

				/*
				 * build next filename
				 */
				tmp_fname = get_next_name();
				if (tmp_fname != NULL)
					strncpy(global.fname_base, tmp_fname,
						sizeof (global.fname_base)-8);
				global.fname_base[sizeof (global.fname_base)-1] = 0;
				minsec = max(lSector, Get_AudioStartSector(i));
				maxsec = min(lSector + rectime*75.0 + 0.5,
							1+Get_EndSector(i));
				if ((int)minsec == Get_AudioStartSector(i) &&
				    (int)maxsec == 1+Get_EndSector(i)) {
					write_info_file(global.fname_base, i,
						(maxsec-minsec)*CD_FRAMESAMPLES,
						bulk && global.multiname == 0);
				} else {
					fprintf(stderr,
					"Partial length copy for track %d, no info file will be generated for this track!\n", i);
				}
				if (!bulk)
					break;
			}
			reset_name_iterator();
		}
#endif
	}

	if (global.just_the_toc)
		exit(NO_ERROR);

#ifdef  ECHO_TO_SOUNDCARD
	if (user_sound_device[0] != '\0') {
		set_snd_device(user_sound_device);
	}
	init_soundcard(rate, bits);
#endif /* ECHO_TO_SOUNDCARD */

	if (global.userspeed > -1)
		global.speed = global.userspeed;

	if (global.speed != 0 && SelectSpeed != NULL) {
		SelectSpeed(get_scsi_p(), global.speed);
	}

	current_track = track;

	if (!global.no_file) {
		{
			char *myfname;

			myfname = get_next_name();

			if (myfname != NULL) {
				strncpy(global.fname_base, myfname,
						sizeof (global.fname_base)-8);
				global.fname_base[sizeof (global.fname_base)-1] = 0;
			}
		}

		/* strip audio_type extension */
		{
			char *cp = global.fname_base;

			cp = strrchr(cp, '.');
			if (cp == NULL) {
				cp = global.fname_base +
						strlen(global.fname_base);
			}
			*cp = '\0';
		}
		if (bulk && global.multiname == 0) {
			sprintf(fname, "%s_%02u.%s",
				global.fname_base, current_track, audio_type);
		} else {
			sprintf(fname, "%s.%s", global.fname_base, audio_type);
		}

		OpenAudio(fname, rate, bits, global.channels,
			(unsigned)(SamplesToWrite*global.OutSampleSize*global.channels),
			global.audio_out);
	}

	global.Remainder = (75 % global.nsectors)+1;

	global.sh_bits = 16 - bits;		/* shift counter */

	global.iloop = *nSamplesToDo;
	if (Halved && (global.iloop&1))
		global.iloop += 2;

	BeginAtSample = lSector * CD_FRAMESAMPLES;

#if	1
	if ((global.verbose & SHOW_SUMMARY) && !global.just_the_toc &&
	    (global.reads_illleadout == 0 ||
	    lSector+*nSamplesToDo/CD_FRAMESAMPLES
	    <= (unsigned) Get_AudioStartSector(cdtracks-1))) {

		fprintf(outfp, "samplefile size will be %lu bytes.\n",
			global.audio_out->GetHdrSize() +
			global.audio_out->InSizeToOutSize(SamplesToWrite*global.OutSampleSize*global.channels));
		fprintf(outfp,
		"recording %d.%04d seconds %s with %d bits @ %5d.%01d Hz",
			(int)rectime, (int)(rectime * 10000) % 10000,
			global.channels == 1 ? "mono":"stereo",
			bits, (int)rate, (int)(rate*10)%10);
		if (!global.no_file && *global.fname_base)
			fprintf(outfp, " ->'%s'...", global.fname_base);
		fputs("\n", outfp);
	}
#endif

#if defined(HAVE_SEMGET) && defined(USE_SEMAPHORES)
#else
	init_pipes();
#endif

#ifdef	USE_PARANOIA
	if (global.paranoia_selected) {
		long paranoia_mode;

		global.cdp = paranoia_init(get_scsi_p(), global.nsectors,
				cdda_read,
				cdda_disc_firstsector, cdda_disc_lastsector,
				cdda_tracks,
				cdda_track_firstsector, cdda_track_lastsector,
				cdda_sector_gettrack, cdda_track_audiop);

		if (global.paranoia_parms.overlap >= 0) {
			int	overlap = global.paranoia_parms.overlap;

			if (overlap > global.nsectors - 1)
				overlap = global.nsectors - 1;
			paranoia_overlapset(global.cdp, overlap);
		}
		/*
		 * Default to a  minimum of dynamic overlapping == 0.5 sectors.
		 * If we don't do this, we get the default from libparanoia
		 * which is approx. 0.1.
		 */
		if (global.paranoia_parms.mindynoverlap < 0)
			paranoia_dynoverlapset(global.cdp, CD_FRAMEWORDS/2, -1);
		paranoia_dynoverlapset(global.cdp,
			global.paranoia_parms.mindynoverlap * CD_FRAMEWORDS,
			global.paranoia_parms.maxdynoverlap * CD_FRAMEWORDS);

		paranoia_mode = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;

		if (global.paranoia_parms.disable_paranoia) {
			paranoia_mode = PARANOIA_MODE_DISABLE;
		}
		if (global.paranoia_parms.disable_extra_paranoia) {
			paranoia_mode |= PARANOIA_MODE_OVERLAP;
			paranoia_mode &= ~PARANOIA_MODE_VERIFY;
		}
		/* not yet implemented */
		if (global.paranoia_parms.disable_scratch_detect) {
			paranoia_mode &= ~(PARANOIA_MODE_SCRATCH|PARANOIA_MODE_REPAIR);
		}
		/* not yet implemented */
		if (global.paranoia_parms.disable_scratch_repair) {
			paranoia_mode &= ~PARANOIA_MODE_REPAIR;
		}

		paranoia_modeset(global.cdp, paranoia_mode);
		if (global.verbose)
			fprintf(outfp, "using lib paranoia for reading.\n");
		paranoia_seek(global.cdp, lSector, SEEK_SET);
		paranoia_reset();
	}
#endif
#if defined(HAVE_FORK_AND_SHAREDMEM)

	/*
	 * Everything is set up. Now fork and let one process read cdda sectors
	 * and let the other one store them in a wav file
	 */

	/* forking */
	child_pid = fork();
	if (child_pid > 0 && global.gui > 0 && global.verbose > 0)
		fprintf(stderr, "child pid is %d\n", child_pid);

	/* ********************** fork ************************************* */
	if (child_pid == 0) {
		/* child WRITER section */

#ifdef	HAVE_AREAS
		/*
		 * Under BeOS a fork() with shared memory does not work as
		 * it does under System V Rel. 4. The mapping of the child
		 * works with copy on write semantics, so changes do not
		 * propagate back and forth. The existing mapping has to be
		 * deleted and replaced by an clone without copy on write
		 * semantics.
		 * This is done with clone_area(...,B_CLONE_ADDRESS,...).
		 * Thanks to file support.c from the postgreSQL project.
		 */
		area_info inf;
		int32 cook = 0;
		/*
		 * iterate over all mappings to find our shared memory mapping.
		 */
		while (get_next_area_info(0, &cook, &inf) == B_OK) {
			/* check the name of the mapping. */
			if (strcmp(inf.name, AREA_NAME) == 0) {
				void *area_address;
				area_id area_parent;

				/* kill the cow mapping. */
				area_address = inf.address;
				if (B_OK != delete_area(inf.area)) {
					errmsgno(EX_BAD,
					"delete_area: no valid area.\n");
					exit(SHMMEM_ERROR);
				}
				/* get the parent mapping. */
				area_parent = find_area(inf.name);
				if (area_parent == B_NAME_NOT_FOUND) {
					errmsgno(EX_BAD,
					"find_area: no such area name.\n");
					exit(SHMMEM_ERROR);
				}
				/* clone the parent mapping without cow. */
				if (B_OK > clone_area("shm_child",
				    &area_address, B_CLONE_ADDRESS,
				    B_READ_AREA | B_WRITE_AREA, area_parent)) {
					errmsgno(EX_BAD,
						"clone_area failed\n");
					exit(SHMMEM_ERROR);
				}
			}
		}
#endif
#ifdef	__EMX__
		if (DosGetSharedMem(he_fill_buffer, 3)) {
			comerr("DosGetSharedMem() failed.\n");
		}
#endif
		global.have_forked = 1;
		forked_write();
#ifdef	__EMX__
		DosFreeMem(he_fill_buffer);
		_exit(NO_ERROR);
		/* NOTREACHED */
#endif
		exit_wrapper(NO_ERROR);
		/* NOTREACHED */
	} else if (child_pid > 0) {
		/* parent READER section */

		global.have_forked = 1;
		switch_to_realtime_priority();

		forked_read();
#ifdef	HAVE_AREAS
		{
			area_id aid;
			aid = find_area(AREA_NAME);
			if (aid < B_OK) {
				comerrno(aid, "find_area() failed.\n");
			}
			delete_area(aid);
		}
#endif
#ifdef	__EMX__
		DosFreeMem(he_fill_buffer);
#endif
		exit_wrapper(NO_ERROR);
		/* NOTREACHED */
	} else {
		errmsg("Cannot fork.\n");
	}

#endif
	/* version without fork */
	{
		global.have_forked = 0;
#if	0
		if (!global.paranoia_selected) {
			error("REAL\n");
			switch_to_realtime_priority();
		}
#endif
		fprintf(stderr, "a nonforking version is running...\n");
		nonforked_loop();
		exit_wrapper(NO_ERROR);
		/* NOTREACHED */
	}
#ifdef	USE_PARANOIA
	if (global.paranoia_selected)
		paranoia_free(global.cdp);
#endif

	return (0);
}
