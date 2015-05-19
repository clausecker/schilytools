/* @(#)util.h	1.9 15/05/11 2011-2015 J. Schilling */
/*
 *	Copyright (c) 1986 Larry Wall
 *	Copyright (c) 2011-2015 J. Schilling
 *
 *	This program may be copied as long as you don't try to make any
 *	money off of it, or pretend that you wrote it.
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
