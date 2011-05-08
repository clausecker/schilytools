/* @(#)util.h	1.7 11/05/06 2011 J. Schilling */
/*
 *	Copyright (c) 1986 Larry Wall
 *	Copyright (c) 2011 J. Schilling
 *
 *	This program may be copied as long as you don't try to make any
 *	money off of it, or pretend that you wrote it.
 */

extern	int	move_file __PR((char *from, char *to));
extern	void	removedirs __PR((char *path));
extern	void	copy_file __PR((char *from, char *to));
extern	char	*savestr __PR((register char *s));
extern	void	say	__PR((const char *fmt, ...)) __printflike__(1, 2);
extern	void	fatal	__PR((const char *fmt, ...)) __printflike__(1, 2);
extern	void	ask	__PR((const char *fmt, ...)) __printflike__(1, 2);
extern	void	set_signals __PR((int reset));
extern	void	ignore_signals __PR((void));
extern	void	makedirs __PR((register char *filename, bool striplast));
extern	char	*fetchname __PR((char *at, int strip_leading,
						int assume_exists,
						bool *isnulldate));
