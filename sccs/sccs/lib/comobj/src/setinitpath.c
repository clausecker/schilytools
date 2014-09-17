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
/*
 * @(#)setinitpath.c	1.1 14/08/03 Copyright 2011-2014 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)setinitpath.c 1.1 14/08/03 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)setinitpath.c"
#pragma ident	"@(#)sccs:lib/comobj/setinitpath.c"
#endif
# include	<defines.h>

EXPORT void
set_init_path(pkt, file, dir)
	register struct packet *pkt;
	char	*file;
	char	*dir;
{
	char	line[max(8192, PATH_MAX+1)];

	/*
	 * Initialize the global initial path only when the directory
	 * .sccs exists and the current path name does not contain "..".
	 */
	if (setrhome != NULL &&
	    file[0] != '/' &&
	    !(file[0] == '.' && file[1] == '.' && file[2] == '/') &&
	    !strstr(file, "/../")) {
		char	*p;

		snprintf(line, sizeof (line), "%s%s%s%s%s",
		homedist > 0 ? cwdprefix:"",
			homedist > 0 ? "/": "",
			dir, *dir ? "/":"",
			auxf(file, 'g'));
		p = fmalloc(size(line));
		strcpy(p, line);
		pkt->p_init_path = p;
	}
}
