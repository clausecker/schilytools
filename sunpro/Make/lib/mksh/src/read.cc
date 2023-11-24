/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)read.cc 1.11 06/12/12
 */

#pragma	ident	"@(#)read.cc	1.11	06/12/12"

/*
 * Copyright 2017-2018 J. Schilling
 * Copyright 2022 the schilytools team
 *
 * @(#)read.cc	1.6 21/08/15 2017-2018 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)read.cc	1.6 21/08/15 2017-2018 J. Schilling";
#endif

/*
 *	read.c
 *
 *	This file contains the makefile reader.
 */

/*
 * Included files
 */
#include <mksh/misc.h>		/* retmem() */
#include <mksh/read.h>
#include <sys/uio.h>		/* read() */

#define	STRING_LEN_TO_CONVERT	(8*1024)

/*
 *	get_next_block_fn(source)
 *
 *	Will get the next block of text to read either
 *	by popping one source bVSIZEOFlock of the stack of Sources
 *	or by reading some more from the makefile.
 *
 *	Return value:
 *				The new source block to read from
 *
 *	Parameters:
 *		source		The old source block
 *
 *	Global variables used:
 *		file_being_read	The name of the current file, error msg
 */
Boolean		make_state_locked;
Source
get_next_block_fn(Source source)
{
	off_t		to_read;
	int		length;
	size_t		num_wc_chars;
	char			ch_save;
	char			*ptr;

	if (source == NULL) {
		return NULL;
	}
	if ((source->fd < 0) || 
		((source->bytes_left_in_file <= 0) &&
			(source->inp_buf_ptr >= source->inp_buf_end))) {
		/* We can't read from the makefile, so pop the source block */
		if (source->fd > 2) {
			(void) close(source->fd);
			if (make_state_lockfile != NULL) {
				(void) unlink(make_state_lockfile);
				retmem_mb(make_state_lockfile);
				make_state_lockfile = NULL;
				make_state_locked = false;
			}
		}
		if (source->string.free_after_use &&
		    (source->string.buffer.start != NULL)) {
			retmem(source->string.buffer.start);
			source->string.buffer.start = NULL;
		}
		if (source->inp_buf != NULL) {
			retmem_mb(source->inp_buf);
			source->inp_buf = NULL;
		}
		source = source->previous;
		if (source != NULL) {
			source->error_converting = false;
		}
		return source;
	}
	if (source->bytes_left_in_file > 0) {
	/*
	 * Read the whole makefile.
	 * Hopefully the kernel managed to prefetch the stuff.
	 */
		to_read = source->bytes_left_in_file;
	 	source->inp_buf_ptr = source->inp_buf = getmem(to_read + 1);
		source->inp_buf_end = source->inp_buf + to_read;
		length = read(source->fd, source->inp_buf, (unsigned int) to_read);
		if (length != to_read) {
			WCSTOMBS(mbs_buffer, file_being_read);
			if (length == 0) {
				fatal_mksh(gettext("Error reading `%s': Premature EOF"),
				      mbs_buffer);
			} else {
				fatal_mksh(gettext("Error reading `%s': %s"),
				      mbs_buffer,
				      errmsg(errno));
			}
		}
		*source->inp_buf_end = nul_char;
		source->bytes_left_in_file = 0;
	}
	/*
	 * Try to convert the next piece.
	 */
	ptr = source->inp_buf_ptr + STRING_LEN_TO_CONVERT;
	if (ptr > source->inp_buf_end) {
		ptr = source->inp_buf_end;
	}
	for (num_wc_chars = 0; ptr > source->inp_buf_ptr; ptr--) {
		ch_save = *ptr;
		*ptr = nul_char;
		num_wc_chars = mbstowcs(source->string.text.end,
					source->inp_buf_ptr,
					STRING_LEN_TO_CONVERT);
		*ptr = ch_save;
		if (num_wc_chars != (size_t) -1) {
			break;
		}
	}
		
	if (num_wc_chars == (size_t) -1) {
		source->error_converting = true;
		return source;
	}

	source->error_converting = false;
	source->inp_buf_ptr = ptr;
	source->string.text.end += num_wc_chars;
	*source->string.text.end = 0;

	if (source->inp_buf_ptr >= source->inp_buf_end) {
		if (*(source->string.text.end - 1) != (int) newline_char) {
			WCSTOMBS(mbs_buffer, file_being_read);
			warning_mksh(gettext("newline is not last character in file %s"),
					     mbs_buffer);
			*source->string.text.end++ = (int) newline_char;
			*source->string.text.end = (int) nul_char;
		}
		if (source->inp_buf != NULL) {
			retmem_mb(source->inp_buf);
			source->inp_buf = NULL;
		}
	}
	return source;
}


