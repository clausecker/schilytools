/* @(#)pch.h	1.10 15/06/02 2011-2015 J. Schilling */
/*
 *	Copyright (c) 1986, 1987 Larry Wall
 *	Copyright (c) 2011-2015 J. Schilling
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
extern	size_t	pch_line_len __PR((LINENUM line));
extern	char	pch_char __PR((LINENUM line));
extern	char	*pfetch __PR((LINENUM line));
extern	LINENUM	pch_hunk_beg __PR((void));
extern	void	do_ed_script __PR((void));
extern	LINENUM	atolnum __PR((char *s));
extern	int	atoinum __PR((char *s));
