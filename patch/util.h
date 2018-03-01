/* @(#)util.h	1.15 18/02/28 2011-2018 J. Schilling */
/*
 *	Copyright (c) 1986 Larry Wall
 *	Copyright (c) 2011-2018 J. Schilling
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition is met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this condition and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Definitions for date+time
 */
typedef struct dtime {
	Llong	dt_sec;		/* Seconds since Jan 1 1970 GMT		*/
	int	dt_nsec;	/* Nanoseconds (must be positive)	*/
	int	dt_zone;	/* Timezone (seconds east to GMT)	*/
} dtime_t;

#define	DT_NO_ZONE	1	/* Impossible timezone - no zone found	*/
#define	DT_MIN_ZONE	(-89940) /* Minimum zone (-24:59)		*/
#define	DT_MAX_ZONE	93540	/* Minimum zone (+25:59)		*/

EXT dtime_t	file_times[2];

extern	int	move_file __PR((char *from, char *to));
extern	void	removedirs __PR((char *path));
extern	void	copy_file __PR((char *from, char *to));
extern	char	*savestr __PR((register char *s));
/*PRINTFLIKE1*/
extern	void	say	__PR((const char *fmt, ...)) __printflike__(1, 2);
/*PRINTFLIKE1*/
extern	void	fatal	__PR((const char *fmt, ...)) __printflike__(1, 2);
/*PRINTFLIKE1*/
extern	void	pfatal	__PR((const char *fmt, ...)) __printflike__(1, 2);
/*PRINTFLIKE1*/
extern	void	ask	__PR((const char *fmt, ...)) __printflike__(1, 2);
extern	void	set_signals __PR((int reset));
extern	void	ignore_signals __PR((void));
#ifndef	_SCHILY_SCHILY_H
extern	int	makedirs __PR((register char *filename, mode_t mode,
						bool striplast));
#endif
extern	char	*fetchname __PR((char *at, int strip_leading,
						int assume_exists,
						dtime_t *dtp));
extern	int	settime	__PR((char *name, int idx, int failed));
extern	int	pspawn	__PR((char *av[]));
