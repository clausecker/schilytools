/* @(#)sndconfig.c	1.27 07/06/22 Copyright 1998-2004 Heiko Eissfeldt, Copyright 2004-2007 J. Schilling */
#ifndef lint
static char	sccsid[] =
"@(#)sndconfig.c	1.27 07/06/22 Copyright 1998-2004 Heiko Eissfeldt, Copyright 2004-2007 J. Schilling";
#endif

/*
 * os dependent functions
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

#include "config.h"
#include <stdio.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/fcntl.h>
#include <schily/unistd.h>
#include <schily/ioctl.h>

#if	!defined __CYGWIN32__
# include <schily/time.h>
#endif
#include <schily/schily.h>


/* soundcard setup */
#if defined(HAVE_SOUNDCARD_H) || defined(HAVE_LINUX_SOUNDCARD_H) || \
	defined(HAVE_SYS_SOUNDCARD_H) || defined(HAVE_MACHINE_SOUNDCARD_H)
# if defined(HAVE_SOUNDCARD_H)
#  include <soundcard.h>
# else
#  if defined(HAVE_MACHINE_SOUNDCARD_H)
#   include <machine/soundcard.h>
#  else
#   if defined(HAVE_SYS_SOUNDCARD_H)
#    include <sys/soundcard.h>
#   else
#    if defined(HAVE_LINUX_SOUNDCARD_H)
#	include <linux/soundcard.h>
#    endif
#   endif
#  endif
# endif
#endif

#include "mytype.h"
#include "byteorder.h"
#include "lowlevel.h"
#include "global.h"
#include "sndconfig.h"

#ifdef	ECHO_TO_SOUNDCARD
#   if defined(__CYGWIN32__)
#	include <windows.h>
#	include "mmsystem.h"
#   endif

#   if	defined(__EMX__)
#	define	INCL_DOS
#	define	INCL_OS2MM
#	include	<os2.h>
#	define	PPFN	_PPFN
#	include	<os2me.h>
#	undef	PPFN
static unsigned long	DeviceID;

#	define	FRAGMENTS	2
/* playlist-structure */
typedef struct {
	ULONG	ulCommand;
	ULONG	ulOperand1;
	ULONG	ulOperand2;
	ULONG	ulOperand3;
} PLAYLISTSTRUCTURE;

static PLAYLISTSTRUCTURE PlayList[FRAGMENTS + 1];
static unsigned BufferInd;
#   endif /* defined __EMX__ */

static char snd_device[200] = SOUND_DEV;

int
set_snd_device(devicename)
	const char	*devicename;
{
	strncpy(snd_device, devicename, sizeof (snd_device));
	return (0);
}

#   if	defined __CYGWIN32__
static HWAVEOUT	DeviceID;
#	define	WAVEHDRS	3
static WAVEHDR	wavehdr[WAVEHDRS];
static unsigned lastwav = 0;

static int check_winsound_caps __PR((int bits, double rate, int channels));

static int
check_winsound_caps(bits, rate, channels)
	int	bits;
	double	rate;
	int	channels;
{
	int		result = 0;
	WAVEOUTCAPS	caps;

	/*
	 * get caps
	 */
	if (waveOutGetDevCaps(0, &caps, sizeof (caps))) {
		errmsgno(EX_BAD, "Cannot get soundcard capabilities!\n");
		return (1);
	}

	/*
	 * check caps
	 */
	if ((bits == 8 && !(caps.dwFormats & 0x333)) ||
	    (bits == 16 && !(caps.dwFormats & 0xccc))) {
		errmsgno(EX_BAD, "%d bits sound are not supported.\n", bits);
		result = 2;
	}

	if ((channels == 1 && !(caps.dwFormats & 0x555)) ||
	    (channels == 2 && !(caps.dwFormats & 0xaaa))) {
		errmsgno(EX_BAD,
			"%d sound channels are not supported.\n", channels);
		result = 3;
	}

	if ((rate == 44100.0 && !(caps.dwFormats & 0xf00)) ||
	    (rate == 22050.0 && !(caps.dwFormats & 0xf0)) ||
	    (rate == 11025.0 && !(caps.dwFormats & 0xf))) {
		errmsgno(EX_BAD,
			"%d sample rate is not supported.\n", (int)rate);
		result = 4;
	}

	return (result);
}
#   endif /* defined CYGWIN */
#endif /* defined ECHO_TO_SOUNDCARD */

#ifdef	HAVE_SUN_AUDIOIO_H
# include <sun/audioio.h>
#endif
#ifdef	HAVE_SYS_AUDIOIO_H
# include <sys/audioio.h>
#endif

#ifdef	HAVE_SYS_ASOUNDLIB_H
# include <sys/asoundlib.h>
snd_pcm_t	*pcm_handle;
#endif

#if	defined	HAVE_OSS && defined SNDCTL_DSP_GETOSPACE
audio_buf_info abinfo;
#endif

int
init_soundcard(rate, bits)
	double	rate;
	int	bits;
{
#ifdef	ECHO_TO_SOUNDCARD
	if (global.echo) {
# if	defined(HAVE_OSS) && HAVE_OSS == 1
		if (open_snd_device() != 0) {
			errmsg("Cannot open sound device '%s'.\n", snd_device);
			global.echo = 0;
		} else {
			/*
			 * This the sound device initialisation for 4front Open sound drivers
			 */
			int	dummy;
			int	garbled_rate = rate;
			int	stereo = (global.channels == 2);
			int	myformat = bits == 8 ? AFMT_U8 :
					(MY_LITTLE_ENDIAN ?
					AFMT_S16_LE : AFMT_S16_BE);
			int	mask;

			if (ioctl(global.soundcard_fd,
			    (int)SNDCTL_DSP_GETBLKSIZE, &dummy) == -1) {
				errmsg("Cannot get blocksize for %s.\n",
					snd_device);
				global.echo = 0;
			}
			if (ioctl(global.soundcard_fd,
			    (int)SNDCTL_DSP_SYNC, NULL) == -1) {
				errmsg("Cannot sync for %s.\n",
					snd_device);
				global.echo = 0;
			}

#if	defined SNDCTL_DSP_GETOSPACE
			if (ioctl(global.soundcard_fd,
			    SNDCTL_DSP_GETOSPACE, &abinfo) == -1) {
				errmsg("Cannot get input buffersize for %s.\n",
					snd_device);
				abinfo.fragments  = 0;
			}
#endif

			/*
			 * check, if the sound device can do the
			 * requested format
			 */
			if (ioctl(global.soundcard_fd,
			    (int)SNDCTL_DSP_GETFMTS, &mask) == -1) {
				errmsg("Fatal error in ioctl(SNDCTL_DSP_GETFMTS).\n");
				return (-1);
			}
			if ((mask & myformat) == 0) {
				errmsgno(EX_BAD,
				"Sound format (%d bits signed) is not available.\n",
				bits);
				if ((mask & AFMT_U8) != 0) {
					bits = 8;
					myformat = AFMT_U8;
				}
			}
			if (ioctl(global.soundcard_fd,
			    (int)SNDCTL_DSP_SETFMT, &myformat) == -1) {
				errmsg("Cannot set %d bits/sample for %s.\n",
					bits, snd_device);
			    global.echo = 0;
			}

			/*
			 * limited sound devices may not support stereo
			 */
			if (stereo &&
			    ioctl(global.soundcard_fd,
			    (int)SNDCTL_DSP_STEREO, &stereo) == -1) {
				errmsg("Cannot set stereo mode for %s.\n",
					snd_device);
				stereo = 0;
			}
			if (!stereo &&
			    ioctl(global.soundcard_fd,
			    (int)SNDCTL_DSP_STEREO, &stereo) == -1) {
				errmsg("Cannot set mono mode for %s.\n",
					snd_device);
				global.echo = 0;
			}

			/*
			 * set the sample rate
			 */
			if (ioctl(global.soundcard_fd,
			    (int)SNDCTL_DSP_SPEED, &garbled_rate) == -1) {
				errmsg("Cannot set rate %d.%2d Hz for %s.\n",
					(int)rate, (int)(rate*100)%100,
					snd_device);
				global.echo = 0;
			}
			if (abs((long)rate - garbled_rate) > rate / 20) {
				errmsgno(EX_BAD,
				"Sound device: next best sample rate is %d.\n",
				garbled_rate);
			}
		}

# else /* HAVE_OSS */

#  if defined	HAVE_SYS_AUDIOIO_H || defined HAVE_SUN_AUDIOIO_H
		/*
		 * This is the SunOS / Solaris and
		 * sound initialisation
		 */
		if ((global.soundcard_fd = open(snd_device, O_WRONLY, 0)) ==
		    EOF) {
			errmsg("Cannot open '%s'.\n", snd_device);
			global.echo = 0;
		} else {
			audio_info_t	info;

#   if	defined(AUDIO_INITINFO) && defined(AUDIO_ENCODING_LINEAR)
			AUDIO_INITINFO(&info);
			info.play.sample_rate = rate;
			info.play.channels = global.channels;
			info.play.precision = bits;
			info.play.encoding = AUDIO_ENCODING_LINEAR;
			info.play.pause = 0;
			info.record.pause = 0;
			info.monitor_gain = 0;
			if (ioctl(global.soundcard_fd, AUDIO_SETINFO, &info) <
			    0) {
				errmsg("Cannot init %s (sun).\n",
					snd_device);
				global.echo = 0;
			}
#   else
			errmsgno(EX_BAD,
			"Cannot init sound device with 44.1 KHz sample rate on %s (sun compatible).\n",
			snd_device);
			global.echo = 0;
#   endif
		}
#  else /* SUN audio */
#   if defined(__CYGWIN32__)
		/*
		 * Windows sound info
		 */
		MMRESULT	mmres;
		WAVEFORMATEX	wavform;

		if (waveOutGetNumDevs() < 1) {
			errmsgno(EX_BAD, "No sound devices available!\n");
			global.echo = 0;
			return (1);
		}

		/*
		 * check capabilities
		 */
		if (check_winsound_caps(bits, rate, global.channels) != 0) {
			errmsgno(EX_BAD,
			"Soundcard capabilities are not sufficient!\n");
			global.echo = 0;
			return (1);
		}

		wavform.wFormatTag = WAVE_FORMAT_PCM;
		wavform.nChannels = global.channels;
		wavform.nSamplesPerSec = (int)rate;
		wavform.wBitsPerSample = bits;
		wavform.cbSize = sizeof (wavform);
		wavform.nAvgBytesPerSec = (int)rate * global.channels *
						(wavform.wBitsPerSample / 8);
		wavform.nBlockAlign = global.channels * (wavform.wBitsPerSample / 8);

		DeviceID = 0;
		mmres = waveOutOpen(&DeviceID, WAVE_MAPPER, &wavform,
			(unsigned long)WIN_CallBack, 0, CALLBACK_FUNCTION);
		if (mmres) {
			char	erstr[329];

			waveOutGetErrorText(mmres, erstr, sizeof (erstr));
			errmsgno(EX_BAD,
				"Soundcard open error: %s!\n", erstr);
			global.echo = 0;
			return (1);
		}

		global.soundcard_fd = 0;

		/*
		 * init all wavehdrs
		 */
		{ int	i;

			for (i = 0; i < WAVEHDRS; i++) {
				wavehdr[i].dwBufferLength = (global.channels*(bits/ 8)*(int)rate*
							global.nsectors)/75;
				wavehdr[i].lpData = malloc(wavehdr[i].dwBufferLength);
				if (wavehdr[i].lpData == NULL) {
					errmsg(
					"No memory for sound buffers available.\n");
					waveOutReset(0);
					waveOutClose(DeviceID);
					return (1);
				}
				mmres = waveOutPrepareHeader(DeviceID,
						&wavehdr[i], sizeof (WAVEHDR));
				if (mmres) {
					char	erstr[129];

					waveOutGetErrorText(mmres, erstr,
							sizeof (erstr));
					errmsgno(EX_BAD,
					"soundcard prepare error: %s!\n",
						erstr);
					return (1);
				}

				wavehdr[i].dwLoops = 0;
				wavehdr[i].dwFlags = WHDR_DONE;
				wavehdr[i].dwBufferLength = 0;
			}
		}

#   else
#    if defined(__EMX__)
#	if defined(HAVE_MMPM)
		/*
		 * OS/2 MMPM/2 MCI sound info
		 */
		MCI_OPEN_PARMS	mciOpenParms;
		int		i;

		/*
		 * create playlist
		 */
		for (i = 0; i < FRAGMENTS; i++) {
			PlayList[i].ulCommand = DATA_OPERATION;	/* play data */
			PlayList[i].ulOperand1 = 0;		/* address */
			PlayList[i].ulOperand2 = 0;		/* size */
			PlayList[i].ulOperand3 = 0;		/* offset */
		}
		PlayList[FRAGMENTS].ulCommand = BRANCH_OPERATION; /* jump */
		PlayList[FRAGMENTS].ulOperand1 = 0;
		PlayList[FRAGMENTS].ulOperand2 = 0;		/* destination */
		PlayList[FRAGMENTS].ulOperand3 = 0;

		memset(&mciOpenParms, 0, sizeof (mciOpenParms));
		mciOpenParms.pszDeviceType = (PSZ) (((unsigned long) MCI_DEVTYPE_WAVEFORM_AUDIO << 16) | \
						(unsigned short) DeviceIndex);
		mciOpenParms.pszElementName = (PSZ) & PlayList;

		/*
		 * try to open the sound device
		 */
		if (mciSendCommand(0, MCI_OPEN,
			MCI_WAIT | MCI_OPEN_SHAREABLE | MCIOPEN_Type_ID,
							&mciOpenParms, 0)
				!= MCIERR_SUCCESS) {
			/*
			 * no sound
			 */
			errmsgno(EX_BAD, "No sound devices available!\n");
			global.echo = 0;
			return (1);
		}
		/*
		 * try to set the parameters
		 */
		DeviceID = mciOpenParms.usDeviceID;

		{
			MCI_WAVE_SET_PARMS	mciWaveSetParms;

			memset(&mciWaveSetParms, 0, sizeof (mciWaveSetParms));
			mciWaveSetParms.ulSamplesPerSec = rate;
			mciWaveSetParms.usBitsPerSample = bits;
			mciWaveSetParms.usChannels = global.channels;
			mciWaveSetParms.ulAudio = MCI_SET_AUDIO_ALL;

			/*
			 * set play-parameters
			 */
			if (mciSendCommand(DeviceID, MCI_SET,
					MCI_WAIT | MCI_WAVE_SET_SAMPLESPERSEC |
					MCI_WAVE_SET_BITSPERSAMPLE |
					MCI_WAVE_SET_CHANNELS,
					(PVOID) & mciWaveSetParms, 0)) {
				MCI_GENERIC_PARMS	mciGenericParms;

				errmsgno(EX_BAD,
				"Soundcard capabilities are not sufficient!\n");
				global.echo = 0;
				/*
				 * close
				 */
				mciSendCommand(DeviceID, MCI_CLOSE, MCI_WAIT,
							&mciGenericParms, 0);
				return (1);
			}
		}

#	endif /* EMX MMPM OS2 sound */
#    else
#	if defined(__QNX__)
		int		card = -1;
		int		dev = 0;
		int		rtn;
		snd_pcm_channel_info_t	pi;
		snd_pcm_channel_params_t	pp;

		if (card == -1) {
			rtn = snd_pcm_open_preferred(&pcm_handle,
				&card, &dev, SND_PCM_OPEN_PLAYBACK);
			if (rtn < 0) {
				errmsg("Error opening sound device.\n");
				return (1);
			}
		} else {
			rtn = snd_pcm_open(&pcm_handle,
				card, dev, SND_PCM_OPEN_PLAYBACK);
			if (rtn < 0) {
				errmsg("Error opening sound device.\n");
				return (1);
			}
		}

		memset(&pi, 0, sizeof (pi));
		pi.channel = SND_PCM_CHANNEL_PLAYBACK;
		rtn = snd_pcm_plugin_info(pcm_handle, &pi);
		if (rtn < 0) {
			errmsg("Snd_pcm_plugin_info failed: '%s'.\n",
				snd_strerror(rtn));
			return (1);
		}

		memset(&pp, 0, sizeof (pp));
		pp.mode = SND_PCM_MODE_BLOCK;
		pp.channel = SND_PCM_CHANNEL_PLAYBACK;
		pp.start_mode = SND_PCM_START_FULL;
		pp.stop_mode = SND_PCM_STOP_STOP;

		pp.buf.block.frag_size = pi.max_fragment_size;
		pp.buf.block.frags_max = 1;
		pp.buf.block.frags_min = 1;

		pp.format.interleave = 1;
		pp.format.rate = rate;
		pp.format.voices = global.channels;
		if (bits == 8) {
			pp.format.format = SND_PCM_SFMT_U8;
		} else {
			pp.format.format = SND_PCM_SFMT_S16_LE;
		}

		rtn = snd_pcm_plugin_params(pcm_handle, &pp);
		if (rtn < 0) {
			errmsg("Snd_pcm_plugin_params failed: '%s'.\n",
				snd_strerror(rtn));
			return (1);
		}

		rtn = snd_pcm_plugin_prepare(pcm_handle,
						SND_PCM_CHANNEL_PLAYBACK);
		if (rtn < 0) {
			errmsg("Snd_pcm_plugin_prepare failed: '%s'.\n",
				snd_strerror(rtn));
			return (1);
		}

	global.soundcard_fd = snd_pcm_file_descriptor(pcm_handle,
						SND_PCM_CHANNEL_PLAYBACK);

#	endif /* QNX sound */
#    endif /* EMX OS2 sound */
#   endif /* CYGWIN Windows sound */
#  endif /* else SUN audio */
# endif /* else HAVE_OSS */
	}
#endif /* ifdef ECHO_TO_SOUNDCARD */
	return (0);
}

int
open_snd_device()
{
#if	defined(F_GETFL) && defined(F_SETFL) && defined(O_NONBLOCK)
	int	fl;
#endif

#if	defined ECHO_TO_SOUNDCARD && !defined __CYGWIN32__ && !defined __EMX__
	global.soundcard_fd = open(snd_device,
#ifdef	linux
		/*
		 * Linux BUG: the sound driver open() blocks,
		 * if the device is in use.
		 */
		O_NONBLOCK |
#endif
		O_WRONLY, 0);

#if	defined(F_GETFL) && defined(F_SETFL) && defined(O_NONBLOCK)
	fl = fcntl(global.soundcard_fd, F_GETFL, 0);
	fl &= ~O_NONBLOCK;
	fcntl(global.soundcard_fd, F_SETFL, fl);
#endif

	return (global.soundcard_fd < 0);
#else
	return (0);
#endif
}

int
close_snd_device()
{
#if	!defined ECHO_TO_SOUNDCARD
	return (0);
#else

# if	defined __CYGWIN32__
	waveOutReset(0);
	return (waveOutClose(DeviceID));
# else /* !Cygwin32 */

#  if	defined __EMX__
#   if	defined HAVE_MMPM
	/*
	 * close the sound device
	 */
	MCI_GENERIC_PARMS mciGenericParms;
	mciSendCommand(DeviceID, MCI_CLOSE, MCI_WAIT, &mciGenericParms, 0);
#   else /* HAVE_MMPM */
	return (0);
#   endif /* HAVE_MMPM */
#  else /* !EMX */
#   if	defined	__QNX__
	snd_pcm_plugin_flush(pcm_handle, SND_PCM_CHANNEL_PLAYBACK);
	return (snd_pcm_close(pcm_handle));
#   else /* !QNX */
	return (close(global.soundcard_fd));
#   endif /* !QNX */
#  endif /* !EMX */
# endif /* !Cygwin32 */
#endif /* ifdef ECHO_TO_SOUNDCARD */
}

int
write_snd_device(buffer, todo)
	char		*buffer;
	unsigned	todo;
{
	int	result = 0;
#ifdef	ECHO_TO_SOUNDCARD
#if	defined __CYGWIN32__
	MMRESULT	mmres;

	wavehdr[lastwav].dwBufferLength = todo;
	memcpy(wavehdr[lastwav].lpData, buffer, todo);

	mmres = waveOutWrite(DeviceID, &wavehdr[lastwav], sizeof (WAVEHDR));
	if (mmres) {
		char erstr[129];

		waveOutGetErrorText(mmres, erstr, sizeof (erstr));
		errmsgno(EX_BAD, "Soundcard write error: %s!\n", erstr);
		return (1);
	}
	if (++lastwav >= WAVEHDRS)
		lastwav -= WAVEHDRS;
	result = mmres;
#else
#if	defined __EMX__
	Playlist[BufferInd].ulOperand1 = buffer;
	Playlist[BufferInd].ulOperand2 = todo;
	Playlist[BufferInd].ulOperand3 = 0;
	if (++BufferInd >= FRAGMENTS)
		BufferInd -= FRAGMENTS;

	/*
	 * no MCI_WAIT here, because application program has to continue
	 */
	memset(&mciPlayParms, 0, sizeof (mciPlayParms));
	if (mciSendCommand(DeviceID, MCI_PLAY, MCI_FROM, &mciPlayParms, 0)) {
		errmsgno(EX_BAD, "Soundcard write error: %s!\n", erstr);
		return (1);
	}
	result = 0;
#else
	int retval2;
	int towrite;

#if	defined	HAVE_OSS && defined SNDCTL_DSP_GETOSPACE
	towrite = abinfo.fragments * abinfo.fragsize;
	if (towrite == 0)
#endif
		towrite = todo;
	do {
		fd_set		writefds[1];
		struct timeval	timeout2;
		int		wrote;

		timeout2.tv_sec = 0;
		timeout2.tv_usec = 4*120000;

		FD_ZERO(writefds);
		FD_SET(global.soundcard_fd, writefds);
		retval2 = select(global.soundcard_fd + 1,
				NULL, writefds, NULL, &timeout2);
		switch (retval2) {

		default:
		case -1: errmsg("Select failed.\n");
			/* FALLTHROUGH */
		case 0: /* timeout */
			result = 2;
			goto outside_loop;
		case 1: break;
		}
		if (towrite > todo) {
			towrite = todo;
		}
#if		defined __QNX__ && defined HAVE_SYS_ASOUNDLIB_H
		wrote = snd_pcm_plugin_write(pcm_handle, buffer, towrite);
#else
		wrote = write(global.soundcard_fd, buffer, towrite);
#endif
		if (wrote <= 0) {
			errmsg("Can't write audio.\n");
			result = 1;
			goto outside_loop;
		} else {
			todo -= wrote;
			buffer += wrote;
		}
	} while (todo > 0);
outside_loop:
	;
#endif	/* !defined __EMX__ */
#endif	/* !defined __CYGWIN32__ */
#endif	/* ECHO_TO_SOUNDCARD */
	return (result);
}
