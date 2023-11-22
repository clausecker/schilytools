#ifndef _MKSH_MISC_H
#define _MKSH_MISC_H
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
 * @(#)misc.h 1.4 06/12/12
 */

#pragma	ident	"@(#)misc.h	1.4	06/12/12"

/*
 * Copyright 2017 J. Schilling
 * Copyright 2023 the schilytools team
 *
 * @(#)misc.h	1.3 21/08/16 2017 J. Schilling
 */

#include <mksh/defs.h>

extern void	append_char(wchar_t from, String to);
extern Property	append_prop(Name target, Property_id type);
extern void	append_string(wchar_t *from, String to, int length);
extern void	enable_interrupt(void (*handler) (int));
extern char	*errmsg(int errnum);
extern void	fatal_mksh(const char * message, ...);
extern void	fatal_reader_mksh(const char *pattern, ...);
extern char	*get_current_path_mksh(void);
extern Property	get_prop(Property start, Property_id type);
extern char	*getmem(int size);
extern Name	getname_fn(wchar_t *name, int len, Boolean dont_enter, Boolean * foundp = NULL);
extern void	store_name(Name name);
extern void	free_name(Name name);
extern void	handle_interrupt_mksh(int);
extern Property	maybe_append_prop(Name target, Property_id type);
extern void	retmem(wchar_t *p);
extern void	retmem_mb(caddr_t p);
extern void	setup_char_semantics(void);
extern void	setup_interrupt(void (*handler) (int));
extern void	warning_mksh(char * message, ...);

extern void	append_string(char *from, String to, int length);
extern wchar_t	*get_wstring(char * from);


#endif
