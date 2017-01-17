/* @(#)util.h	1.13 16/12/18 2011-2016 J. Schilling */
/*
 *	Copyright (c) 1986 Larry Wall
 *	Copyright (c) 2011-2016 J. Schilling
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
						bool *isnulldate));
extern	int	pspawn	__PR((char *av[]));
