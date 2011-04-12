/* @(#)inp.h	1.6 11/01/30 2011 J. Schilling */
/*
 *	Copyright (c) 1986 Larry Wall
 *	Copyright (c) 2011 J. Schilling
 *
 *	This program may be copied as long as you don't try to make any
 *	money off of it, or pretend that you wrote it.
 */

EXT LINENUM input_lines;		/* how long is input file in lines */
EXT LINENUM last_frozen_line;		/* how many input lines have been */
					/* irretractibly output */

extern	void	re_input __PR((void));
extern	void	scan_input __PR((char *filename));
extern	bool	plan_a __PR((char *filename));	/* returns false if no memory */
extern	char	*ifetch __PR((register LINENUM line, int whichbuf));
