/* @(#)movearch.c	1.33 09/01/06 Copyright 1993, 1995, 2001-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)movearch.c	1.33 09/01/06 Copyright 1993, 1995, 2001-2009 J. Schilling";
#endif
/*
 *	Handle non-file type data that needs to be moved from/to the archive.
 *
 *	Copyright (c) 1993, 1995, 2001-2009 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/schily.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include "starsubs.h"
#include "movearch.h"

EXPORT	int	move_from_arch	__PR((move_t *move, char *p, int amount));
EXPORT	int	move_to_arch	__PR((move_t *move, char *p, int amount));


/*
 * Move data from archive.
 */
EXPORT int
move_from_arch(move, p, amount)
	move_t	*move;
	char	*p;
	int	amount;
{
	movebytes(p, move->m_data, amount);
	move->m_data += amount;
	/*
	 * If we make sure that the buffer holds at least one character more
	 * than needed, then we may safely add another null byte at the end of
	 * the extracted buffer.
	 * This makes sure, that a buggy tar implementation which wight archive
	 * non null-terminated long filenames with 'L' or 'K' filetype may
	 * not cause us to core dump.
	 * It is needed when extracting extended attribute buffers from
	 * POSIX.1-2001 archives as POSIX.1-2001 makes the buffer '\n' but not
	 * null-terminated.
	 */
	move->m_data[0] = '\0';
	return (amount);
}

/*
 * Move data to archive.
 */
EXPORT int
move_to_arch(move, p, amount)
	move_t	*move;
	char	*p;
	int	amount;
{
	if (amount > move->m_size)
		amount = move->m_size;
	movebytes(move->m_data, p, amount);
	move->m_data += amount;
	move->m_size -= amount;
	if (move->m_flags & MF_ADDSLASH) {
		if (move->m_size == 1) {
			p[amount-1] = '/';
		} else if (move->m_size == 0) {
			if (amount > 1)
				p[amount-2] = '/';
			p[amount-1] = '\0';
		}
	}
	return (amount);
}
