/* @(#)check_part.c	1.30 07/05/24 Copyright 1993-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)check_part.c	1.30 07/05/24 Copyright 1993-2004 J. Schilling";
#endif
/*
 *	Check Partition validity
 *
 *	Copyright (c) 1993-2004 J. Schilling
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
#include <stdio.h>
#ifdef	HAVE_SYS_PARAM_H
#include <sys/param.h>	/* Include various defs needed with some OS */
#endif
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/ioctl.h>
#include <schily/fcntl.h>
#include <sys/file.h>
#include "dsklabel.h"
#ifdef	HAVE_VALUES_H
#include <values.h>
#else
#	ifdef	HAVE_FLOAT_H
#	include <float.h>
#	endif
#endif

#include "fmt.h"

#include <scg/scsireg.h>

typedef struct chk_label {
	int	part_chk;
} CHK_LABEL;

#define	ADJACENT_START		0x0001	/* Anstoßender Start */
#define	ADJACENT_END		0x0002	/* Anstoßendes Ende */
#define	SAME_START		0x0004	/* Start gemeinsam mit anderer part */
#define	SAME_END		0x0008	/* Ende gemeinsam mit anderer part */
#define	UNALIGNED_START		0x0010	/* Einsamer Start */
#define	UNALIGNED_END		0x0020	/* Einsames Ende */
#define	SPACE_BEFORE_START	0x0040	/* Ungenutzter Raum vor part */
#define	SPACE_AFTER_END		0x0080	/* Ungenutzter Raum nach part */
#define	PART_C_CONV_ERR		0x0100	/* Part 'c'/2 nicht nach convention */
#define	PART_RANGE_ERR		0x1000	/* Start < 0 */
#define	PART_TO_BIG		0x2000	/* Ende nach lncyl */

#define	WARNING			0x0FF0	/* Maske für nicht fatale Fehler */
#define	DANGER			0xF000	/* Maske für fatale Fehler */

/*
 * A C H T U N G  !!!
 *
 * Mit diesen Makros geht es nur bis 1 TByte groszen Platten gut.
 */
#define	SPCYL			((long)(lp->dkl_nhead * lp->dkl_nsect))
#define	START(x)		(lp->dkl_map[x].dkl_cylno * SPCYL)
#define	END(x)			(lp->dkl_map[x].dkl_cylno * SPCYL + \
						lp->dkl_map[x].dkl_nblk)

extern	struct	dk_label *d_label;

LOCAL	void	check_start	__PR((struct dk_label *, int));
LOCAL	void	check_end	__PR((struct dk_label *, int));
LOCAL	void	check_bounds	__PR((struct dk_label *, int));
LOCAL	void	init_result	__PR((void));
LOCAL	void	set_bounds	__PR((struct dk_label *, long *, long *));
LOCAL	void	analyse_parts	__PR((struct dk_label *));
EXPORT	void	checklabel	__PR((struct disk *, struct dk_label *, int));
LOCAL	void	check_partc	__PR((struct disk *, struct dk_label *, int));
LOCAL	char	get_startc	__PR((int));
LOCAL	char	get_endc	__PR((int));
LOCAL	void	graph		__PR((struct dk_label *, long, long));
LOCAL	void	print_lwarn	__PR((void));

CHK_LABEL	result[8];

LOCAL void
check_start(lp, n)
	register struct dk_label *lp;
	int	n;
{
	int	i;

	/*
	 * Partition beginnt nicht gemeinsam mit einer anderen Partition
	 * Partition beginnt nicht direkt hinter einer anderen Partition
	 * Partition beginnt nicht am Anfang der Platte
	 */
	if (!(result[n].part_chk & ADJACENT_START) && (START(n) > 0)) {
		result[n].part_chk |= UNALIGNED_START;

		for (i = 0; i < 8; i++) {
			if (lp->dkl_map[i].dkl_nblk <= 0) /*XXX <= ??? sd.c */
				continue;

			if (i == n || (result[i].part_chk & DANGER))
				continue;

			if (START(n) > START(i)) {
				result[n].part_chk &= ~SPACE_BEFORE_START;
				break;
			}
			result[n].part_chk |= SPACE_BEFORE_START;
		}
	}
}

LOCAL void
check_end(lp, n)
	register struct dk_label *lp;
	int	n;
{
	int	i;

	/*
	 * Partition endet nicht gemeinsam mit einer anderen Partition
	 * Partition endet nicht direkt vor einer anderen Partition
	 * Partition endet nicht auf dem gültigen Ende
	 */
	if (!(result[n].part_chk & ADJACENT_END) &&
				(END(n) < (long)(lp->dkl_ncyl*SPCYL))) {
		result[n].part_chk |= UNALIGNED_END;

		for (i = 0; i < 8; i++) {
			if (lp->dkl_map[i].dkl_nblk <= 0) /*XXX <= ??? sd.c */
				continue;

			if (i == n || (result[i].part_chk & DANGER))
				continue;

			if (END(n) < END(i)) {
				result[n].part_chk &= ~SPACE_AFTER_END;
				break;
			}
			result[n].part_chk |= SPACE_AFTER_END;
		}
	}
}

LOCAL void
check_bounds(lp, n)
	register struct dk_label	*lp;
			int		n;
{
	int	i;
	long	spcyl = SPCYL;

	/*
	 * Avoid divide by zero!
	 */
	if (spcyl == 0)
		spcyl = 1;

	if (START(n) < 0)
		result[n].part_chk |= PART_RANGE_ERR;
	if ((long)lp->dkl_ncyl < (END(n) / spcyl))
		result[n].part_chk |= PART_TO_BIG;

	for (i = 0; i < 8; i++) {
		if (lp->dkl_map[i].dkl_nblk <= 0) /*XXX <= ??? sd.c */
			continue;

		if (i == n)
			continue;

		if (START(n) == END(i))
			result[n].part_chk |= ADJACENT_START;
		if (END(n) == START(i))
			result[n].part_chk |= ADJACENT_END;
		if (START(n) == START(i))
			result[n].part_chk |= SAME_START;
		if (END(n) == END(i))
			result[n].part_chk |= SAME_END;
	}
}

LOCAL void
init_result()
{
	int	i;

	for (i = 0; i < 8; i++)
		result[i].part_chk = 0;
}

LOCAL void
set_bounds(lp, low, high)
	register struct dk_label *lp;
	long	*low,
		*high;
{
	int	i;

	for (i = 0; i < 8; i++) {
		if (lp->dkl_map[i].dkl_nblk <= 0)	/*XXX <= ??? sd.c */
			continue;

		if (*low > START(i) && START(i) >= 0)
			*low = START(i);
		if (*high < END(i) && END(i) <= ((long)lp->dkl_ncyl*SPCYL))
			*high = END(i);
	}
}

LOCAL void
analyse_parts(lp)
	register struct dk_label *lp;
{
	int	i;

	for (i = 0; i < 8; i++) {
		check_bounds(lp, i);
		check_start(lp, i);
		check_end(lp, i);
	}
}

LOCAL void
check_partc(dp, lp, set)
	struct disk	*dp;
	register struct dk_label *lp;
	int	set;
{
	if (dp->labelread >= 0 &&
			lp->dkl_map[2].dkl_cylno == 0 &&
			(lp->dkl_ncyl * SPCYL == lp->dkl_map[2].dkl_nblk))
		return;
	/*
	 * Label wurde nicht gelesen, part 'c'/2 beginnt nicht auf 0,
	 * oder part 'c'/2 ist nicht dkl_ncyl groß.
	 */

	if (dp->labelread >= 0) {
		/*
		 * Part 'c'/2 convention error.
		 */
		struct dk_label l;

		l = *lp;
		l.dkl_map[2].dkl_cylno = 0;
		l.dkl_map[2].dkl_nblk = lp->dkl_ncyl * SPCYL;

		printf("WARNING! \ncalculated part '%c':\n", PARTOFF+2);
		printpart(&l, 2);
		printf("current part '%c':\n", PARTOFF+2);
		printpart(lp, 2);
		result[2].part_chk |= PART_C_CONV_ERR;
	}
	if (!set)
		return;

	if (dp->labelread < 0 || yes("set to calculated value? ")) {
		lp->dkl_map[2].dkl_cylno = 0;
		lp->dkl_map[2].dkl_nblk = lp->dkl_ncyl * SPCYL;
		lp->dkl_magic = DKL_MAGIC;
		lp->dkl_cksum =	do_cksum(lp);
		result[2].part_chk &= ~PART_C_CONV_ERR;
	}
}

EXPORT void
checklabel(dp, lp, set)
	struct disk	*dp;
	register struct dk_label *lp;
	int	set;
{
	long		low;
	long		high;

	low  = lp->dkl_ncyl*SPCYL;
	high = 0;

	init_result();
	check_partc(dp, lp, set);
	set_bounds(lp, &low, &high);
	analyse_parts(lp);
	graph(lp, low, high);
	print_lwarn();
}

LOCAL char
get_startc(n)
	int	n;
{
	if (result[n].part_chk & PART_RANGE_ERR)
		return ('X');
	if (result[n].part_chk & ADJACENT_START)
		return ('/');
	if (result[n].part_chk & SAME_START)
		return ('+');
	if (result[n].part_chk & UNALIGNED_START)
		return ('<');
	return ('*');
}

LOCAL char
get_endc(n)
	int	n;
{
	if (result[n].part_chk & PART_TO_BIG)
		return ('X');
	if (result[n].part_chk & ADJACENT_END)
		return ('/');
	if (result[n].part_chk & SAME_END)
		return ('+');
	if (result[n].part_chk & UNALIGNED_END)
		return ('>');
	return ('*');
}

LOCAL void
graph(lp, low, high)
	register struct dk_label *lp;
	long	low;
	long	high;
{
#define	LINELEN	75.0
	int		i;
	int		j;
	long		s;
	long		e;
	char		startc;
	char		endc;
	float		blkpc;
	char		line[(int)LINELEN+2+1];

	printf("\n");
	blkpc = (high - low) / LINELEN;
	if (blkpc < 0.1) {
#ifdef	HAVE_VALUES_H
		blkpc = MAXFLOAT;
#else
#ifdef	HAVE_FLOAT_H
		blkpc = FLT_MAX;
#else
		blkpc = ERROR_NO_FLOAT;
#endif
#endif
	}

	for (i = 0; i < 8; i++) {
		startc = endc = 'I';
		line[0] = '\0';
		printf("%c ", PARTOFF + i);

		if (lp->dkl_map[i].dkl_nblk <= 0) {	/*XXX <= ??? sd.c */
			printf("partition not defined\n");
				if (START(i) != 0)
					printf("partition has no size but start != 0\n");
			result[i].part_chk &= PART_C_CONV_ERR;
			continue;
		}

		if (START(i) != 0)
			startc = get_startc(i);
		if (END(i) != (lp->dkl_ncyl * SPCYL))
			endc = get_endc(i);

		e = (END(i) > high) ? high-low : END(i) - low;
		if (e < 0)
			e = 0;
		/* Die Rundung ist Absicht! */
		j = e / blkpc + 1.0;
		line[j+1] = '\0';
		line[j] = endc;

		while (--j >= 0)
			line[j] = '-';

		s = (START(i) < low) ? 0 : START(i) - low;
		if (s > high)
			s = high-low;
		/* Die Rundung ist Absicht! */
		for (j = s / blkpc + 1.0; --j >= 0; )
			line[j] = ' ';

		/* Die Rundung ist Absicht! */
		j = s / blkpc + 0.999999999;
		line[j] = startc;
		printf("%s\n", line);
	}
}

LOCAL void
print_lwarn()
{
	int		i;
	int		printed = 0;

	for (i = 0; i < 8; i++) {
		if (result[i].part_chk & DANGER) {
			printf("DANGER:\n");
			break;
		}
	}

	for (i = 0; i < 8; i++) {
		if ((result[i].part_chk & DANGER) == 0)
			continue;

		printed = 0;
		printf("part '%c'", PARTOFF + i);
		if (result[i].part_chk & PART_RANGE_ERR) {
			if (printed)
				printf(" and");
			printf(" starts before 0");
			printed++;
		}
		if (result[i].part_chk & PART_TO_BIG) {
			if (printed)
				printf(" and");
			printf(" ends behind lncyl");
			printed++;
		}
		printf("!\n");
	}

	for (i = 0; i < 8; i++) {
		if (result[i].part_chk & WARNING) {
			printf("WARNING:\n");
			break;
		}
	}

	for (i = 0; i < 8; i++) {
		if ((result[i].part_chk & WARNING) == 0)
			continue;

		printed = 0;
		printf("part '%c' has", PARTOFF + i);
		if (result[i].part_chk & UNALIGNED_START) {
			if (printed)
				printf(" and");
			printf(" unaligned start");
			printed++;
		}
		if (result[i].part_chk & UNALIGNED_END) {
			if (printed)
				printf(" and");
			printf(" unaligned end");
			printed++;
		}
		if (result[i].part_chk & SPACE_BEFORE_START) {
			if (printed)
				printf(" and");
			printf(" space before start");
			printed++;
		}
		if (result[i].part_chk & SPACE_AFTER_END) {
			if (printed)
				printf(" and");
			printf(" space after end");
			printed++;
		}
		if (result[i].part_chk & PART_C_CONV_ERR) {
			if (printed)
				printf(" and");
			printf(" not full size");
			printed++;
		}
		printf(".\n");
	}
	printf("\n");
}
