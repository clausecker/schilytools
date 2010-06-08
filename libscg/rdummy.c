/* @(#)rdummy.c	1.4 10/05/24 Copyright 2000-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char _sccsid[] =
	"@(#)rdummy.c	1.4 10/05/24 Copyright 2000-2010 J. Schilling";
#endif
/*
 *	scg Library
 *	dummy remote ops
 *
 *	Copyright (c) 2000-2010 J. Schilling
 */
/*@@C@@*/

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/schily.h>

#include <scg/scsitransp.h>

EXPORT	scg_ops_t *scg_remote	__PR((void));

EXPORT scg_ops_t *
scg_remote()
{
extern	scg_ops_t scg_remote_ops;

	return (&scg_remote_ops);
}
