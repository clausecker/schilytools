/* @(#)xmodes.c	1.23 06/09/13 Copyright 1991-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)xmodes.c	1.23 06/09/13 Copyright 1991-2004 J. Schilling";
#endif
/*
 *	Routines to handle external mode data (database)
 *
 *	Copyright (c) 1991-2004 J. Schilling
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
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/utypes.h>
#include "fmt.h"

#include <schily/intcvt.h>

#include <scg/scsidefs.h>
#include <scg/scsireg.h>

EXPORT	void	ext_modeselect	__PR((SCSI *scgp, struct disk *));
LOCAL	int	do_page		__PR((SCSI *Scgp, struct disk *));
LOCAL	BOOL	set_page	__PR((SCSI *scgp, struct disk *, int, int, BOOL));
LOCAL	int	set_vars	__PR((u_char *, u_char *, int));
LOCAL	int	do_var		__PR((u_char *, u_char *, int));
LOCAL	int	set_bits	__PR((u_char *, u_char *, int, BOOL, long, int, int));
LOCAL	int	set_byte	__PR((u_char *, u_char *, int, BOOL, long));
LOCAL	int	set_short	__PR((u_char *, u_char *, int, BOOL, long));
LOCAL	int	set_3byte	__PR((u_char *, u_char *, int, BOOL, long));
LOCAL	int	set_long	__PR((u_char *, u_char *, int, BOOL, long));
LOCAL	void	xprint		__PR((char *, long));
LOCAL	void	modewarning	__PR((void));
#ifdef	NONO
LOCAL	int	xset_bits	__PR((u_char *, u_char *, int, long, int, int));
#endif

#ifdef	FMT
extern	int	xdebug;
extern	int	save_mp;
#else
	int	xdebug = 1;
#endif

LOCAL BOOL	modewarn = 1; /*XXX*/

#ifndef	FMT
struct disk ds;

main(ac, av)
	int	ac;
	char	**av;
{
	char	*name;

	if (ac > 1)
		name = av[1];
	else
		name = "hallo";

	if (!opendatfile("mode_pages"))
		return;

	ds.mode_pages = name;
	ext_modeselect(&ds);

	closedatfile();
}
#endif


/*---------------------------------------------------------------------------
|
|	Setzt beliebige Mode Seiten durch
|	externe Steuerung ueber eine Datei.
|
+---------------------------------------------------------------------------*/

EXPORT void
ext_modeselect(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	if (!dp->mode_pages)
		return;
	if (xdebug)
		printf("set_modes: mode_pages: '%s'\n", dp->mode_pages);

	if (rewinddatfile() < 0)
		return;
	if (xdebug)
		printf("ext_modeselect: data file OK\n");
	if (scanfortable("mode_pages", dp->mode_pages) && nextline())
		while (do_page(scgp, dp) == 1)
			;
}


/*---------------------------------------------------------------------------
|
|	Parst einen Page Header Eintrag in der Steuerungsdatei
|
+---------------------------------------------------------------------------*/

LOCAL int
do_page(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	int	type	= 2;	/* Default Werte */
	int	mpsave	= 1;	/* Modeparameter Sichern */
	int	page;
	char	*word;

	if (xdebug) printf("do_page: line: %d curword: '%s'\n",
						getlineno(), curword());

	if ((word = scanforline("Page", NULL)) == NULL)
		return (0);

	if (word == NULL)	/* EOF XXXX????? */
		return (EOF);

	word = nextitem();
	if (*astoi(word, &page) != '\0') {
		datfileerr("Bad page number '%s'", word);
		return (0);
	}

	word = nextitem();
	if (*word) {
		if (streql(word, "current"))
			type = 0;
		else if (streql(word, "changeable"))
			type = 1;
		else if (streql(word, "default"))
			type = 2;
		else if (streql(word, "saved"))
			type = 3;
		else {
			datfileerr("Bad page type '%s'", word);
			return (0);
		}
	}

	word = nextitem();
	if (*word) {
		if (streql(word, "noset"))
			mpsave = -1;
		else if (streql(word, "nosave"))
			mpsave = 0;
		else if (streql(word, "save"))
			mpsave = 1;
		else {
			datfileerr("Bad save type '%s'", word);
			return (0);
		}
	}

	word = nextitem();
	(void) garbage(word);

	if (!nextline()) {
		datfileerr("Premature EOF");
		return (EOF);
	}

	set_page(scgp, dp, page, type, mpsave);
	return (1);
}


/*---------------------------------------------------------------------------
|
|	Setzt eine Mode Seite
|
+---------------------------------------------------------------------------*/

LOCAL BOOL
set_page(scgp, dp, page, type, mpsave)
	SCSI		*scgp;
	struct disk	*dp;
	int	page;
	int	type;
	BOOL	mpsave;
{
	char	name[16];
	u_char	mode[0x100];
	u_char	cmode[0x100];
	u_char	dmode[0x100];
	int	len;
	struct	scsi_mode_page_header *mp;
	struct	scsi_mode_page_header *cmp;
	struct	scsi_mode_page_header *smp;

	fillbytes(mode, sizeof (mode), '\0');
	fillbytes(cmode, sizeof (cmode), '\0');
	fillbytes(dmode, sizeof (dmode), '\0');

	sprintf(name, "Mode Page 0x%02X", page);
	if (xdebug)
		printf("%s (type %d)\n", name, type);
#ifdef	FMT
	if (!get_mode_params(scgp, page, name,
					mode, cmode, dmode, (u_char *)0, &len))
		return (FALSE);
	if (len == 0)
		return (TRUE);
#else
len = 32;
mode[0] = 32;	/* Länge */
dmode[0] = 32;	/* Länge */
mode[3] = 8;	/* Blockdes Länge */
dmode[3] = 8;	/* Blockdes Länge */
mode[10] = 2;	/* Sectorsize */
dmode[10] = 2;	/* Sectorsize */
mode[12] = 20;	/* Pagelänge */
dmode[12] = 20;	/* Pagelänge */
#endif

	cmp = (struct scsi_mode_page_header *)
		(cmode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)cmode)->blockdesc_len);

	switch (type) {

	case 0:	smp = (struct scsi_mode_page_header *)mode;	break;
	case 1:	smp = (struct scsi_mode_page_header *)cmode;	break;
	case 2:	smp = (struct scsi_mode_page_header *)dmode;	break;
	case 3:
	default:
		datfileerr("WARNING:\tSaved values currently not supported -\n\
		using default values");
		smp = (struct scsi_mode_page_header *)dmode;	break;
	}
	mp = (struct scsi_mode_page_header *)
		((u_char *)smp + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)smp)->blockdesc_len);

	set_vars((u_char *)mp, (u_char *)cmp,
					len-((u_char *)mp-(u_char *)smp));
	mp->parsave = 0;

	if (xdebug) {
		prbytes("Mode Sense Data", (u_char *)smp, len);
		printf("mp: %lx mode: %lX mode-mp: %lld\n", (long)mp, (long)smp,
					(Llong)((u_char *)mp-(u_char *)smp));
		printf("mpsave: %d\n", mpsave);
	}

	if (mpsave < 0)
		return (TRUE);

#ifdef	FMT
	return (set_mode_params(scgp, name, (u_char *) smp, len,
						(save_mp && mpsave > 0), dp));
#else
	return (0);
#endif
}


/*---------------------------------------------------------------------------
|
|	Setzt die Page Variablen nach den
|	Eintraegen in der Steuerungsdatei.
|
+---------------------------------------------------------------------------*/

LOCAL int
set_vars(mode, cmode, len)
	u_char	*mode;
	u_char	*cmode;
	int	len;
{
	char	*word;

	for (;;) {
		if ((word = scanforline("Byte", "Page")) == NULL)
			return (0);
		if (streql(word, "Page"))
			return (1);

		if (word == NULL)	/* EOF XXX????? */
			return (EOF);

		do_var(mode, cmode, len);

		if (!nextline())
			return (EOF);
	}
}


/*---------------------------------------------------------------------------
|
|	Parst einen Page Variablen Eintrag in der Steuerungsdatei
|
+---------------------------------------------------------------------------*/

#define	O_SET	1
#define	O_AND	2
#define	O_OR	3
#define	O_PRINT	4

LOCAL int
do_var(mode, cmode, len)
	u_char	*mode;
	u_char	*cmode;
	int	len;
{
	char	*word;
	int	idx;
	int	size;
	int	op;
	BOOL	donot = FALSE;
	long	val;
	int	bitno;
	int	nbits;
	char	prarg[80];

	word = nextitem();

	if (*astoi(word, &idx) != '\0' || idx < 0 || idx >= len) {
		datfileerr("Bad byte offset '%s'", word);
		return (0);
	}
/*XXX*/
/*printf("offset: %d len: %d\n", idx, len);*/

	word = nextitem();
	if (streql(word, "bit")) {
		size = 0;
		word = nextitem();
		word = astoi(word, &bitno);
		if (*word) {
			if (word[0] == '.' && word[1] == '.') {
				if (*astoi(&word[2], &nbits) != '\0') {
					datfileerr("Bad end bit '%s'", word);
					return (0);
				}
				nbits -= bitno-1;
			} else {
				datfileerr("Bad bit range '%s'", word);
				return (0);
			}
		} else
			nbits = 1;
	} else if (streql(word, "byte") || streql(word, "1byte"))
		size = 1;
	else if (streql(word, "short") || streql(word, "2byte"))
		size = 2;
	else if (streql(word, "3byte"))
		size = 3;
	else if (streql(word, "long") || streql(word, "4byte"))
		size = 4;
	else {
		datfileerr("Bad val size '%s'", word);
		return (0);
	}

	word = nextitem();
	if (streql(word, "="))
		op = O_SET;
	else if (streql(word, "&="))
		op = O_AND;
	else if (streql(word, "|="))
		op = O_OR;
	else if (streql(word, "print"))
		op = O_PRINT;
	else if (streql(word, "printall")) {
		prbytes(nextitem(), mode, len);
		return (0);
	} else if (streql(word, "printpages")) {
		u_char	*p = mode;
		u_char	*ep = &mode[len];

		printf("%s", nextitem());
		while (p < ep) {
			printf("0x%x ", *p&0x3F);
			p += p[1]+2;
		}
		printf("\n");
		return (0);
	} else {
		datfileerr("Bad op '%s'", word);
		return (0);
	}

	if (!isval(word = nextitem()))
		return (0);

	if (op == O_PRINT) {
		*movebytes(word, prarg, sizeof (prarg)-1) = 0;
		val = (long)prarg;
	} else {
		if (*word == '~') {
			word++;
			donot = TRUE;
		}
		if (*word == '\0')
			word = nextitem();
		if (*astol(word, &val) != '\0') {
			datfileerr("Bad val '%s'", word);
			return (0);
		}
	}

	word = nextitem();
	(void) garbage(word);

/*printf("idx: %d size: %d op: %d, val: %ld\n", idx, size, op, val);*/
/*XXX*/
#ifndef	FMT
{int i;
for (i = 0; i < 20; i++)
	cmode[i] = 0xFF;
}
#endif
	switch (size) {

	case 0:
		return (set_bits(&mode[idx], &cmode[idx], op, donot, val,
								bitno, nbits));
	case 1:
		return (set_byte(&mode[idx], &cmode[idx], op, donot, val));
	case 2:
		return (set_short(&mode[idx], &cmode[idx], op, donot, val));
	case 3:
		return (set_3byte(&mode[idx], &cmode[idx], op, donot, val));
	case 4:
		return (set_long(&mode[idx], &cmode[idx], op, donot, val));
	default:
		datfileerr("Bad size '%d' paranoia?", size);
		return (0);
	}
}

LOCAL int
set_bits(mode, cmode, op, donot, val, bitno, nbits)
	u_char	*mode;
	u_char	*cmode;
	int	op;
	BOOL	donot;
	long	val;
	int	bitno;
	int	nbits;
{
	long	oval;
	long	lbitmask;
	long	bitmask;

	if (bitno < 0 || bitno > 7) {
		datfileerr("Bad start bit '%d'", bitno);
		return (0);
	}
	if (nbits < 0 || (bitno+nbits-1) > 7) {
		datfileerr("Bad end bit '%d'", bitno+nbits-1);
		return (0);
	}

	for (lbitmask = 0, oval = 0; oval < nbits; oval++)
		lbitmask |= 1 << oval;

	bitmask = lbitmask << bitno;

	if (op != O_PRINT) {
		if (((unsigned long)val) > lbitmask) {
			datfileerr("Bad bit val '%ld'", val);
			return (0);
		}

		if (donot)
			val = ~val & bitmask;
		val <<= bitno;

		if ((*cmode & bitmask) == 0) {
			modewarning();
			return (1);	/* XXX Wir tun mal so als ob es geht */
		}
	}
	oval = *mode & bitmask;

	switch (op) {

	case O_SET:				break;
	case O_AND:	val = oval & val;	break;
	case O_OR:	val = oval | val;	break;
	case O_PRINT:
			xprint((char *)val, oval>>bitno); return (1);
	}

	*mode &= ~bitmask;
	*mode |=  val;
	return (1);
}

LOCAL int
set_byte(mode, cmode, op, donot, val)
	u_char	*mode;
	u_char	*cmode;
	int	op;
	BOOL	donot;
	long	val;
{
	long	oval;

	if (op != O_PRINT) {
		if (((unsigned long)val) > 0xFF) {
			datfileerr("Bad byte val '%ld'", val);
			return (0);
		}

		if (donot)
			val = ~val & 0xFF;

		if (*cmode == 0) {
			modewarning();
			return (1);	/* XXX Wir tun mal so als ob es geht */
		}
	}
	oval = *mode;

	switch (op) {

	case O_SET:				break;
	case O_AND:	val = oval & val;	break;
	case O_OR:	val = oval | val;	break;
	case O_PRINT:	xprint((char *)val, oval); return (1);
	}

	*mode =  val;
	return (1);
}

LOCAL int
set_short(mode, cmode, op, donot, val)
	u_char	*mode;
	u_char	*cmode;
	int	op;
	BOOL	donot;
	long	val;
{
	long	oval;

	if (op != O_PRINT) {
		if (((unsigned long)val) > 0xFFFF) {
			datfileerr("Bad 2byte val '%ld'", val);
			return (0);
		}

		if (donot)
			val = ~val & 0xFFFF;

		if (a_to_u_2_byte(cmode) == 0) {
			modewarning();
			return (1);	/* XXX Wir tun mal so als ob es geht */
		}
	}
	oval = a_to_u_2_byte(mode);

	switch (op) {

	case O_SET:				break;
	case O_AND:	val = oval & val;	break;
	case O_OR:	val = oval | val;	break;
	case O_PRINT:	xprint((char *)val, oval); return (1);
	}

	i_to_2_byte(mode, val);
	return (1);
}

LOCAL int
set_3byte(mode, cmode, op, donot, val)
	u_char	*mode;
	u_char	*cmode;
	int	op;
	BOOL	donot;
	long	val;
{
	long	oval;

	if (op != O_PRINT) {
		if (((unsigned long)val) > 0xFFFFFF) {
			datfileerr("Bad 3byte val '%ld'", val);
			return (0);
		}

		if (donot)
			val = ~val & 0xFFFFFF;

		if (a_to_u_3_byte(cmode) == 0) {
			modewarning();
			return (1);	/* XXX Wir tun mal so als ob es geht */
		}
	}
	oval = a_to_u_3_byte(mode);

	switch (op) {

	case O_SET:				break;
	case O_AND:	val = oval & val;	break;
	case O_OR:	val = oval | val;	break;
	case O_PRINT:	xprint((char *)val, oval); return (1);
	}

	i_to_3_byte(mode, val);
	return (1);
}

LOCAL int
set_long(mode, cmode, op, donot, val)
	u_char	*mode;
	u_char	*cmode;
	int	op;
	BOOL	donot;
	long	val;
{
	long	oval;

	if (op != O_PRINT) {
		if (donot)
			val = ~val;

		if (a_to_4_byte(cmode) == 0) {
			modewarning();
			return (1);	/* XXX Wir tun mal so als ob es geht */
		}
	}
	oval = a_to_4_byte(mode);

	switch (op) {

	case O_SET:				break;
	case O_AND:	val = oval & val;	break;
	case O_OR:	val = oval | val;	break;
	case O_PRINT:	xprint((char *)val, oval); return (1);
	}

	i_to_4_byte(mode, val);
	return (1);
}

LOCAL void
xprint(fmt, val)
	char	*fmt;
	long	val;
{
	register char	c;

	while ((c = *fmt++) != '\0') {
		if (c == '%') {
			c = *fmt++;
			if (c == '\0')
				break;
			if (c == 'd')
				printf("%ld", val);
			else if (c == 'o')
				printf("%lo", val);
			else if (c == 'x')
				printf("%lx", val);
			else
				putchar(c);
		} else if (c == '\\') {
			c = *fmt++;
			if (c == '\0')
				break;
			if (c == 'n')
				c = '\n';
			else if (c == 't')
				c = '\t';
			putchar(c);
		} else {
			putchar(c);
		}
	}
}

LOCAL void
modewarning()
{
	if (modewarn)
		datfileerr("WARNING: Controller does not support field");
}

#ifdef	NONO
LOCAL int
xset_bits(mode, cmode, op, val, bitno, nbits)
	u_char	*mode;
	u_char	*cmode;
	int	op;
	long	val;
	int	bitno;
	int	nbits;
{
	long	oval;
	long	lbitmask;
	long	bitmask;

	if (bitno < 0 || bitno > 31) {
		datfileerr("Bad start bit '%d'", bitno);
		return (0);
	}
	if (nbits < 0 || (bitno+nbits-1) > 31) {
		datfileerr("Bad end bit '%d'", bitno+nbits-1);
		return (0);
	}

	for (lbitmask = 0, oval = 0; oval < nbits; oval++)
		lbitmask |= 1 << oval;

	bitmask = lbitmask << bitno;

	if (((unsigned long)val) > lbitmask) {
		datfileerr("Bad val '%d'", val);
		return (0);
	}
	val <<= bitno;

	if ((a_to_4_byte(cmode) & bitmask) == 0) {
		modewarning();
		return (1);	/* XXX Wir tun mal so als ob es geht */
	}
	oval = a_to_4_byte(mode) & bitmask;

	switch (op) {

	case O_SET:				break;
	case O_AND:	val = oval & val;	break;
	case O_OR:	val = oval | val;	break;
	}

	oval = a_to_4_byte(mode) & ~bitmask;
	val |= oval;
	i_to_4_byte(mode, val);
	return (1);
}
#endif
