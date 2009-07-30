/* @(#)xdelay.c	1.16 09/07/11 Copyright 1991-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)xdelay.c	1.16 09/07/11 Copyright 1991-2009 J. Schilling";
#endif
/*
 *	Delay for disks that cannot disconnect
 *
 *	Copyright (c) 1991-2009 J. Schilling
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

#ifdef	sun

#include <schily/stdio.h>
#include <schily/unistd.h>
#include <kvm.h>
#include <schily/fcntl.h>
#include <nlist.h>
#include <schily/param.h>	/* Include various defs needed with some OS */
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/libport.h>

#ifdef	FMT
#include "fmt.h"
#endif

static int initialized;
static kvm_t *kd;

static long avenrun[3];

struct nlist nl[] = {
#ifdef	SVR4
	{ "avenrun"},
#else
	{ "_avenrun"},
#endif
	{ 0 },
};

LOCAL	void	xinit	__PR((void));
EXPORT	void	xdelay	__PR((void));

LOCAL void
xinit()
{
	initialized = 1;

	kd = kvm_open(0, 0, 0, O_RDONLY, 0);
	if (!kd)
		return;

	kvm_nlist(kd, nl);
	if (nl[0].n_type == 0) {
		kvm_close(kd);
		kd = 0;
		return;
	}
/*	printf("avenrun: %X\n", nl[0].n_value);*/
}

EXPORT void
xdelay()
{
	static long oavrun = 0;
	long avrun;
	long x;

	if (!initialized)
		xinit();
	if (!kd) {
		usleep(10000);
		return;
	}
	if (kvm_read(kd, nl[0].n_value, (char *)avenrun, sizeof (avenrun)) !=
		sizeof (avenrun))
		errmsg("kvm_read\n");

/*	printf("\n%f %d\n", (double)avenrun[0]/FSCALE, avenrun[0]*1000/FSCALE);*/
	avrun = avenrun[0]*100/FSCALE;
	x = avrun - 95;
	x *= 4;
	if (x < 20) x = 20;
	printf("\r\t\tload %ld.%02ld wtime: %ld ms  \r",
						avrun/100, avrun%100, x);
	if (x > 2000) x = 2000;
	usleep(x*1000);
	if ((oavrun - avrun) > 10)
		usleep(100000);
	oavrun = avrun;
}

#else	/* sun */

#include <schily/mconfig.h>
#include <schily/unistd.h>
#include <schily/standard.h>

#include "fmt.h"

EXPORT	void	xdelay	__PR((void));

EXPORT void
xdelay()
{
	usleep(10000);
}
#endif	/* sun */
