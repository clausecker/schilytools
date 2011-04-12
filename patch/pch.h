/* @(#)pch.h	1.7 11/01/31 2011 J. Schilling */
/*
 *	Copyright (c) 1986, 1987 Larry Wall
 *	Copyright (c) 2011 J. Schilling
 *
 *	This program may be copied as long as you don't try to make any
 *	money off of it, or pretend that you wrote it.
 */

extern	void	re_patch __PR((void));
extern	void	open_patch_file __PR((char *filename));
extern	void	set_hunkmax __PR((void));
extern	bool	there_is_another_patch __PR((void));
extern	bool	another_hunk __PR((void));
extern	bool	pch_swap __PR((void));
extern	LINENUM	pch_first __PR((void));
extern	LINENUM	pch_ptrn_lines __PR((void));
extern	LINENUM	pch_newfirst __PR((void));
extern	LINENUM	pch_repl_lines __PR((void));
extern	LINENUM	pch_end __PR((void));
extern	LINENUM	pch_context __PR((void));
extern	short	pch_line_len __PR((LINENUM line));
extern	char	pch_char __PR((LINENUM line));
extern	char	*pfetch __PR((LINENUM line));
extern	LINENUM	pch_hunk_beg __PR((void));
extern	void	do_ed_script __PR((void));
