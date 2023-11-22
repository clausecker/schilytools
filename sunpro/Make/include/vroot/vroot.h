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
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)vroot.h 1.10 06/12/12
 */

#pragma	ident	"@(#)vroot.h	1.10	06/12/12"

/*
 * Copyright 2017 J. Schilling
 * Copyright 2023 the schilytools team
 *
 * @(#)vroot.h	1.5 21/08/16 2017 J. Schilling
 */

#ifndef _VROOT_H_
#define _VROOT_H_

#if defined(SCHILY_BUILD) || defined(SCHILY_INCLUDES)
#include <schily/unistd.h>
#include <schily/stdio.h>
#else
#include <unistd.h>
#include <stdio.h>
#endif

#define VROOT_DEFAULT ((pathpt)-1)

typedef struct {
	char		*path;
	short		length;
} pathcellt, *pathcellpt, patht;
typedef patht		*pathpt;

extern	void		add_dir_to_path(const char *path, pathpt *pointer, int position);
extern	void		flush_path_cache(void);
extern	void		flush_vroot_cache(void);
extern	const char	*get_path_name(void);
extern	char		*get_vroot_path(char **vroot, char **path, char **filename);
extern	const char	*get_vroot_name(void);
extern	int		open_vroot(char *path, int flags, int mode, pathpt vroot_path, pathpt vroot_vroot);
extern	pathpt		parse_path_string(char *string, int remove_slash);
extern	void		scan_path_first(void);
extern	void		scan_vroot_first(void);
extern	void		set_path_style(int style);

extern	int		access_vroot(char *path, int mode, pathpt vroot_path, pathpt vroot_vroot);

extern	int		execve_vroot(char *path, char **argv, char **environ, pathpt vroot_path, pathpt vroot_vroot);

extern	int		lstat_vroot(char *path, struct stat *buffer, pathpt vroot_path, pathpt vroot_vroot);
extern	int		stat_vroot(char *path, struct stat *buffer, pathpt vroot_path, pathpt vroot_vroot);
extern	int		readlink_vroot(char *path, char *buffer, int buffer_size, pathpt vroot_path, pathpt vroot_vroot);


#endif
