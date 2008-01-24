/* @(#)isoinfo.c	1.61 08/01/16 joerg */
#ifndef	lint
static	char sccsid[] =
	"@(#)isoinfo.c	1.61 08/01/16 joerg";
#endif
/*
 * File isodump.c - dump iso9660 directory information.
 *
 *
 * Written by Eric Youngdale (1993).
 *
 * Copyright 1993 Yggdrasil Computing, Incorporated
 * Copyright (c) 1999-2007 J. Schilling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Simple program to dump contents of iso9660 image in more usable format.
 *
 * Usage:
 * To list contents of image (with or without RR):
 *	isoinfo -l [-R] -i imagefile
 * To extract file from image:
 *	isoinfo -i imagefile -x xtractfile > outfile
 * To generate a "find" like list of files:
 *	isoinfo -f -i imagefile
 */

#include <schily/mconfig.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>

#include <stdio.h>
#include <schily/utypes.h>
#include <schily/standard.h>
#include <signal.h>
#include <sys/stat.h>
#include <schily/stat.h>
#include <schily/fcntl.h>
#include <schily/schily.h>

#include "../iso9660.h"
#include "../scsi.h"
#include "../../cdrecord/defaults.h"
#include "../../cdrecord/version.h"

#include <schily/siconv.h>

#if	defined(__CYGWIN32__) || defined(__EMX__) || defined(__DJGPP__)
#include <io.h>					/* for setmode() prototype */
#endif

/*
 * Make sure we have a definition for this.  If not, take a very conservative
 * guess.
 * POSIX requires the max pathname component lenght to be defined in limits.h
 * If variable, it may be undefined. If undefined, there should be
 * a definition for _POSIX_NAME_MAX in limits.h or in unistd.h
 * As _POSIX_NAME_MAX is defined to 14, we cannot use it.
 * XXX Eric's wrong comment:
 * XXX From what I can tell SunOS is the only one with this trouble.
 */
#ifdef	HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef	NAME_MAX
#ifdef	FILENAME_MAX
#define	NAME_MAX	FILENAME_MAX
#else
#define	NAME_MAX	256
#endif
#endif

#ifndef	PATH_MAX
#ifdef	FILENAME_MAX
#define	PATH_MAX	FILENAME_MAX
#else
#define	PATH_MAX	1024
#endif
#endif

/*
 * XXX JS: Some structures have odd lengths!
 * Some compilers (e.g. on Sun3/mc68020) padd the structures to even length.
 * For this reason, we cannot use sizeof (struct iso_path_table) or
 * sizeof (struct iso_directory_record) to compute on disk sizes.
 * Instead, we use offsetof(..., name) and add the name size.
 * See iso9660.h
 */
#ifndef	offsetof
#define	offsetof(TYPE, MEMBER)	((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef	S_ISLNK
#define	S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef	S_ISSOCK
#ifdef	S_IFSOCK
#	define	S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)
#else
#	define	S_ISSOCK(m)	(0)
#endif
#endif

/*
 * Note: always use these macros to avoid problems.
 *
 * ISO_ROUND_UP(X)	may cause an integer overflow and thus give
 *			incorrect results. So avoid it if possible.
 *
 * ISO_BLOCKS(X)	is overflow safe. Prefer this when ever it is possible.
 */
#define	SECTOR_SIZE	(2048)
#define	ISO_ROUND_UP(X)	(((X) + (SECTOR_SIZE - 1)) & ~(SECTOR_SIZE - 1))
#define	ISO_BLOCKS(X)	(((X) / SECTOR_SIZE) + (((X)%SECTOR_SIZE)?1:0))

#define	infile	in_image
FILE	*infile = NULL;
int	use_rock = 0;
int	use_joliet = 0;
int	do_listing = 0;
int	do_find = 0;
int	do_sectors = 0;
int	do_pathtab = 0;
int	do_pvd = 0;
BOOL	debug = FALSE;
char	*xtract = 0;
int	su_version = 0;
int	aa_version = 0;
int	ucs_level = 0;

struct stat	fstat_buf;
int		found_rr;
char		name_buf[256];
char		xname[2048];
unsigned char	date_buf[9];
/*
 * Use sector_offset != 0 (-N #) if we have an image file
 * of a single session and we need to list the directory contents.
 * This is the session block (sector) number of the start
 * of the session when it would be on disk.
 */
unsigned int	sector_offset = 0;

unsigned char	buffer[2048];

siconvt_t	*unls;

#define	PAGE sizeof (buffer)

#define	ISODCL(from, to) (to - from + 1)


LOCAL	int	isonum_721	__PR((char * p));
LOCAL	int	isonum_723	__PR((char * p));
LOCAL	int	isonum_731	__PR((char * p));
LOCAL	int	isonum_732	__PR((char * p));
LOCAL	int	isonum_733	__PR((unsigned char * p));
LOCAL	void	printchars	__PR((char *s, int n, BOOL ucs));
LOCAL	char	*sdate		__PR((char *dp));
LOCAL	void	dump_pathtab	__PR((int block, int size));
LOCAL	int	parse_rr	__PR((unsigned char * pnt, int len, int cont_flag));
LOCAL	void	find_rr		__PR((struct iso_directory_record * idr, Uchar **pntp, int *lenp));
LOCAL	int	dump_rr		__PR((struct iso_directory_record * idr));
LOCAL	void	dump_stat	__PR((struct iso_directory_record * idr, int extent));
LOCAL	void	extract_file	__PR((struct iso_directory_record * idr));
LOCAL	void	parse_dir	__PR((char * rootname, int extent, int len));
LOCAL	void	usage		__PR((int excode));
EXPORT	int	main		__PR((int argc, char *argv[]));
LOCAL	void	list_vd		__PR((struct iso_primary_descriptor *vp, BOOL ucs));
LOCAL	void	list_locales	__PR((void));
LOCAL	void	printf_bootinfo	__PR((FILE *f, int bootcat_offset));
LOCAL	char	*arch_name	__PR((int val));
LOCAL	char	*boot_name	__PR((int val));
LOCAL	char	*bootmedia_name	__PR((int val));


LOCAL int
isonum_721(p)
	char	*p;
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8));
}

LOCAL int
isonum_723(p)
	char * p;
{
#if 0
	if (p[0] != p[3] || p[1] != p[2]) {
		fprintf(stderr, "invalid format 7.2.3 number\n");
		exit(1);
	}
#endif
	return (isonum_721(p));
}

LOCAL int
isonum_731(p)
	char	*p;
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8)
		| ((p[2] & 0xff) << 16)
		| ((p[3] & 0xff) << 24));
}

LOCAL int
isonum_732(p)
	char	*p;
{
	return ((p[3] & 0xff)
		| ((p[2] & 0xff) << 8)
		| ((p[1] & 0xff) << 16)
		| ((p[0] & 0xff) << 24));
}

LOCAL int
isonum_733(p)
	unsigned char	*p;
{
	return (isonum_731((char *)p));
}

LOCAL void
printchars(s, n, ucs)
	char	*s;
	int	n;
	BOOL	ucs;
{
	int	i;
	char	*p;
	int	c;

	for (; n > 0 && (*s || (ucs && s[1])); n--) {
		if (ucs) {
			c = *s++ & 0xFF;
			c *= 256;
			c += *s++ & 0xFF;
			n--;
		} else {
			c = *s++;
		}
		if (c == ' ') {
			int	c2;
			p = s;
			i = n;
			while (--i >= 0) {
				if (ucs) {
					c2 = *p++ & 0xFF;
					c2 *= 256;
					c2 += *p++ & 0xFF;
					i--;
				} else {
					c2 = *p++;
				}
				if (c2 != ' ')
					break;
			}
			if (i <= 0)
				break;
		}
		putchar(c);
	}
}

/*
 * Print date info from PVD
 */
LOCAL char *
sdate(dp)
	char	*dp;
{
	static	char	d[30];

	js_sprintf(d, "%4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s",
			&dp[0],		/* Year */
			&dp[4],		/* Month */
			&dp[6],		/* Monthday */
			&dp[8],		/* Hour */
			&dp[10],	/* Minute */
			&dp[12],	/* Seconds */
			&dp[14]);	/* Hunreds of a Seconds */

	/*
	 * dp[16] contains minute offset from Greenwich
	 * Positive values are to the east of Greenwich.
	 */
	return (d);
}

LOCAL void
dump_pathtab(block, size)
	int	block;
	int	size;
{
	unsigned char	*buf;
	int		offset;
	int		idx;
	int		extent;
	int		pindex;
	int		j;
	int		len;
	int		jlen;
	char		namebuf[255];
	unsigned char	uc;


	printf("Path table starts at block %d, size %d\n", block, size);

	buf = (unsigned char *) malloc(ISO_ROUND_UP(size));

#ifdef	USE_SCG
	readsecs(block - sector_offset, buf, ISO_BLOCKS(size));
#else
	lseek(fileno(infile), ((off_t)(block - sector_offset)) << 11, SEEK_SET);
	read(fileno(infile), buf, size);
#endif

	offset = 0;
	idx = 1;
	while (offset < size) {
		len    = buf[offset];
		extent = isonum_731((char *)buf + offset + 2);
		pindex  = isonum_721((char *)buf + offset + 6);
		switch (ucs_level) {
		case 3:
		case 2:
		case 1:
			jlen = len/2;
			namebuf[0] = '\0';
			for (j = 0; j < jlen; j++) {
				UInt16_t	unichar;

				unichar = (buf[offset + 8 + j*2] & 0xFF) * 256 +
					  (buf[offset + 8 + j*2+1] & 0xFF);

				uc = sic_uni2c(unls, unichar);	/* Get the backconverted char */

				namebuf[j] = uc ? uc : '_';
			}
			printf("%4d: %4d %x %.*s\n",
				idx, pindex, extent, jlen, namebuf);
			break;
		case 0:
			printf("%4d: %4d %x %.*s\n",
				idx, pindex, extent, len, buf + offset + 8);
		}

		idx++;
		offset += 8 + len;
		if (offset & 1)
			offset++;
	}

	free(buf);
}

LOCAL int
parse_rr(pnt, len, cont_flag)
	unsigned char	*pnt;
	int		len;
	int		cont_flag;
{
	int slen;
	int xlen;
	int ncount;
	int extent;
	int cont_extent, cont_offset, cont_size;
	int flag1, flag2;
	unsigned char *pnts;
	char symlinkname[1024];
	int goof;

	symlinkname[0] = 0;

	cont_extent = cont_offset = cont_size = 0;

	ncount = 0;
	flag1 = flag2 = 0;
	while (len >= 4) {
		if (pnt[3] != 1 && pnt[3] != 2) {
			printf("**BAD RRVERSION (%d)\n", pnt[3]);
			return (0);		/* JS ??? Is this right ??? */
		}
		ncount++;
		if (pnt[0] == 'R' && pnt[1] == 'R') flag1 = pnt[4] & 0xff;
		if (strncmp((char *)pnt, "PX", 2) == 0) flag2 |= 1;	/* POSIX attributes */
		if (strncmp((char *)pnt, "PN", 2) == 0) flag2 |= 2;	/* POSIX device number */
		if (strncmp((char *)pnt, "SL", 2) == 0) flag2 |= 4;	/* Symlink */
		if (strncmp((char *)pnt, "NM", 2) == 0) flag2 |= 8;	/* Alternate Name */
		if (strncmp((char *)pnt, "CL", 2) == 0) flag2 |= 16;	/* Child link */
		if (strncmp((char *)pnt, "PL", 2) == 0) flag2 |= 32;	/* Parent link */
		if (strncmp((char *)pnt, "RE", 2) == 0) flag2 |= 64;	/* Relocated Direcotry */
		if (strncmp((char *)pnt, "TF", 2) == 0) flag2 |= 128;	/* Time stamp */
		if (strncmp((char *)pnt, "SP", 2) == 0) {
			flag2 |= 1024;					/* SUSP record */
			su_version = pnt[3] & 0xff;
		}
		if (strncmp((char *)pnt, "AA", 2) == 0) {
			flag2 |= 2048;					/* Apple Signature record */
			aa_version = pnt[3] & 0xff;
		}

		if (strncmp((char *)pnt, "PX", 2) == 0) {		/* POSIX attributes */
			fstat_buf.st_mode = isonum_733(pnt+4);
			fstat_buf.st_nlink = isonum_733(pnt+12);
			fstat_buf.st_uid = isonum_733(pnt+20);
			fstat_buf.st_gid = isonum_733(pnt+28);
			fstat_buf.st_ino = 0;
			if ((pnt[2] & 0xFF)>= 44)
				fstat_buf.st_ino = (UInt32_t)isonum_733(pnt+36);
		}

		if (strncmp((char *)pnt, "NM", 2) == 0) {		/* Alternate Name */
			int	l = strlen(name_buf);

			if (!found_rr)
				l = 0;
			strncpy(&name_buf[l], (char *)(pnt+5), pnt[2] - 5);
			name_buf[l + pnt[2] - 5] = 0;
			found_rr = 1;
		}

		if (strncmp((char *)pnt, "CE", 2) == 0) {		/* Continuation Area */
			cont_extent = isonum_733(pnt+4);
			cont_offset = isonum_733(pnt+12);
			cont_size = isonum_733(pnt+20);
		}

		if (strncmp((char *)pnt, "PL", 2) == 0 || strncmp((char *)pnt, "CL", 2) == 0) {
			extent = isonum_733(pnt+4);
		}

		if (strncmp((char *)pnt, "SL", 2) == 0) {		/* Symlink */
			int	cflag;

			cflag = pnt[4];
			pnts = pnt+5;
			slen = pnt[2] - 5;
			while (slen >= 1) {
				switch (pnts[0] & 0xfe) {
				case 0:
					strncat(symlinkname, (char *)(pnts+2), pnts[1]);
					symlinkname[pnts[1]] = 0;
					break;
				case 2:
					strcat(symlinkname, ".");
					break;
				case 4:
					strcat(symlinkname, "..");
					break;
				case 8:
					strcat(symlinkname, "/");
					break;
				case 16:
					strcat(symlinkname, "/mnt");
					printf("Warning - mount point requested");
					break;
				case 32:
					strcat(symlinkname, "kafka");
					printf("Warning - host_name requested");
					break;
				default:
					printf("Reserved bit setting in symlink");
					goof++;
					break;
				}
				if ((pnts[0] & 0xfe) && pnts[1] != 0) {
					printf("Incorrect length in symlink component");
				}
				if (xname[0] == 0) strcpy(xname, "-> ");
				strcat(xname, symlinkname);
				symlinkname[0] = 0;
				xlen = strlen(xname);
				if ((pnts[0] & 1) == 0 && xname[xlen-1] != '/') strcat(xname, "/");

				slen -= (pnts[1] + 2);
				pnts += (pnts[1] + 2);
			}
			symlinkname[0] = 0;
		}

		len -= pnt[2];
		pnt += pnt[2];
		if (len <= 3 && cont_extent) {
			unsigned char	sector[2048];

#ifdef	USE_SCG
			readsecs(cont_extent - sector_offset, sector, ISO_BLOCKS(sizeof (sector)));
#else
			lseek(fileno(infile), ((off_t)(cont_extent - sector_offset)) << 11, SEEK_SET);
			read(fileno(infile), sector, sizeof (sector));
#endif
			flag2 |= parse_rr(&sector[cont_offset], cont_size, 1);
		}
	}
	/*
	 * for symbolic links, strip out the last '/'
	 */
	if (xname[0] != 0 && xname[strlen(xname)-1] == '/') {
		xname[strlen(xname)-1] = '\0';
	}
	return (flag2);
}

LOCAL void
find_rr(idr, pntp, lenp)
	struct iso_directory_record *idr;
	Uchar	**pntp;
	int	*lenp;
{
	struct iso_xa_dir_record *xadp;
	int len;
	unsigned char * pnt;

	len = idr->length[0] & 0xff;
	len -= offsetof(struct iso_directory_record, name[0]);
	len -= idr->name_len[0];

	pnt = (unsigned char *) idr;
	pnt += offsetof(struct iso_directory_record, name[0]);
	pnt += idr->name_len[0];
	if ((idr->name_len[0] & 1) == 0) {
		pnt++;
		len--;
	}
	if (len >= 14) {
		xadp = (struct iso_xa_dir_record *)pnt;

		if (xadp->signature[0] == 'X' && xadp->signature[1] == 'A' &&
		    xadp->reserved[0] == '\0') {
			len -= 14;
			pnt += 14;
		}
	}
	*pntp = pnt;
	*lenp = len;
}

LOCAL int
dump_rr(idr)
	struct iso_directory_record *idr;
{
	int len;
	unsigned char * pnt;

	find_rr(idr, &pnt, &len);
	return (parse_rr(pnt, len, 0));
}

struct todo
{
	struct todo	*next;
	char		*name;
	int		extent;
	int		length;
};

struct todo	*todo_idr = NULL;

char		*months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
				"Aug", "Sep", "Oct", "Nov", "Dec"};

LOCAL void
dump_stat(idr, extent)
	struct iso_directory_record *idr;
	int	extent;
{
	int	i;
	int	off = 0;
	char	outline[100];

	memset(outline, ' ', sizeof (outline));

	if (fstat_buf.st_ino != 0)
		off = js_sprintf(outline, "%10llu ", (ULlong)fstat_buf.st_ino);
	if (off < 0)
		off = 0;

	if (S_ISREG(fstat_buf.st_mode))
		outline[off] = '-';
	else if (S_ISDIR(fstat_buf.st_mode))
		outline[off] = 'd';
	else if (S_ISLNK(fstat_buf.st_mode))
		outline[off] = 'l';
	else if (S_ISCHR(fstat_buf.st_mode))
		outline[off] = 'c';
	else if (S_ISBLK(fstat_buf.st_mode))
		outline[off] = 'b';
	else if (S_ISFIFO(fstat_buf.st_mode))
		outline[off] = 'f';
	else if (S_ISSOCK(fstat_buf.st_mode))
		outline[off] = 's';
	else
		outline[off] = '?';

	memset(outline+off+1, '-', 9);
	if (fstat_buf.st_mode & S_IRUSR)
		outline[off+1] = 'r';
	if (fstat_buf.st_mode & S_IWUSR)
		outline[off+2] = 'w';
	if (fstat_buf.st_mode & S_IXUSR)
		outline[off+3] = 'x';

	if (fstat_buf.st_mode & S_IRGRP)
		outline[off+4] = 'r';
	if (fstat_buf.st_mode & S_IWGRP)
		outline[off+5] = 'w';
	if (fstat_buf.st_mode & S_IXGRP)
		outline[off+6] = 'x';

	if (fstat_buf.st_mode & S_IROTH)
		outline[off+7] = 'r';
	if (fstat_buf.st_mode & S_IWOTH)
		outline[off+8] = 'w';
	if (fstat_buf.st_mode & S_IXOTH)
		outline[off+9] = 'x';

	off += 10;
	off += js_sprintf(outline+off, " %3ld", (long)fstat_buf.st_nlink);
	off += js_sprintf(outline+off, " %4ld", (unsigned long)fstat_buf.st_uid);
	off += js_sprintf(outline+off, " %4ld", (unsigned long)fstat_buf.st_gid);

	if (do_sectors == 0) {
		off += js_sprintf(outline+off, " %10lld", (Llong)fstat_buf.st_size);
	} else {
		off += js_sprintf(outline+off, " %10lld", (Llong)((fstat_buf.st_size+PAGE-1)/PAGE));
	}

	if (date_buf[1] >= 1 && date_buf[1] <= 12) {
		memcpy(outline+off+1, months[date_buf[1]-1], 3);
		off += 4;
	}

	off += js_sprintf(outline+off, " %2d", date_buf[2]);
	off += js_sprintf(outline+off, " %4d", date_buf[0]+1900);

	off += js_sprintf(outline+off, " [%7d", extent);	/* XXX up to 20 GB */
	off += js_sprintf(outline+off, " %02X]", idr->flags[0] & 0xFF);

	for (i = 0; i < off; i++) {
		if (outline[i] == 0) outline[i] = ' ';
	}
	outline[off] = 0;
	printf("%s %s %s\n", outline, name_buf, xname);
}

LOCAL void
extract_file(idr)
	struct iso_directory_record *idr;
{
	int		extent, len, tlen;
	unsigned char	buff[2048];

#if	defined(__CYGWIN32__) || defined(__CYGWIN__) || defined(__EMX__) || defined(__DJGPP__)
	setmode(fileno(stdout), O_BINARY);
#endif

	extent = isonum_733((unsigned char *)idr->extent);
	len = isonum_733((unsigned char *)idr->size);

	while (len > 0) {
#ifdef	USE_SCG
		readsecs(extent - sector_offset, buff, ISO_BLOCKS(sizeof (buff)));
		tlen = (len > sizeof (buff) ? sizeof (buff) : len);
#else
		lseek(fileno(infile), ((off_t)(extent - sector_offset)) << 11, SEEK_SET);
		tlen = (len > sizeof (buff) ? sizeof (buff) : len);
		read(fileno(infile), buff, tlen);
#endif
		len -= tlen;
		extent++;
		write(STDOUT_FILENO, buff, tlen);
	}
}

LOCAL void
parse_dir(rootname, extent, len)
	char	*rootname;
	int	extent;
	int	len;
{
	char		testname[PATH_MAX+1];
	struct todo 	*td;
	int		i;
	struct iso_directory_record * idr;
	unsigned char	uc;
	unsigned char	flags = 0;
	Llong		size = 0;
	int		sextent = 0;


	if (do_listing)
		printf("\nDirectory listing of %s\n", rootname);

	while (len > 0) {
#ifdef	USE_SCG
		readsecs(extent - sector_offset, buffer, ISO_BLOCKS(sizeof (buffer)));
#else
		lseek(fileno(infile), ((off_t)(extent - sector_offset)) << 11, SEEK_SET);
		read(fileno(infile), buffer, sizeof (buffer));
#endif
		len -= sizeof (buffer);
		extent++;
		i = 0;
		while (1 == 1) {
			idr = (struct iso_directory_record *) &buffer[i];
			if (idr->length[0] == 0) break;
			memset(&fstat_buf, 0, sizeof (fstat_buf));
			found_rr = 0;
			name_buf[0] = xname[0] = 0;
			fstat_buf.st_size = (off_t)(unsigned)isonum_733((unsigned char *)idr->size);
			if (idr->flags[0] & 2)
				fstat_buf.st_mode |= S_IFDIR;
			else
				fstat_buf.st_mode |= S_IFREG;
			if (idr->name_len[0] == 1 && idr->name[0] == 0)
				strcpy(name_buf, ".");
			else if (idr->name_len[0] == 1 && idr->name[0] == 1)
				strcpy(name_buf, "..");
			else {
				switch (ucs_level) {
				case 3:
				case 2:
				case 1:
					/*
					 * Unicode name.  Convert as best we can.
					 */
					{
					int	j;

					name_buf[0] = '\0';
					for (j = 0; j < (int)idr->name_len[0] / 2; j++) {
						UInt16_t	unichar;

						unichar = (idr->name[j*2] & 0xFF) * 256 +
							  (idr->name[j*2+1] & 0xFF);

						/*
						 * Get the backconverted char
						 */
						uc = sic_uni2c(unls, unichar);

						name_buf[j] = uc ? uc : '_';
					}
					name_buf[idr->name_len[0]/2] = '\0';
					}
					break;
				case 0:
					/*
					 * Normal non-Unicode name.
					 */
					strncpy(name_buf, idr->name, idr->name_len[0]);
					name_buf[idr->name_len[0]] = 0;
					break;
				default:
					/*
					 * Don't know how to do these yet.  Maybe they are the same
					 * as one of the above.
					 */
					exit(1);
				}
			}
			memcpy(date_buf, idr->date, 9);
			if (use_rock)
				dump_rr(idr);
			if ((idr->flags[0] & 2) != 0 &&
			    (idr->name_len[0] != 1 ||
			    (idr->name[0] != 0 && idr->name[0] != 1))) {
				/*
				 * Add this directory to the todo list.
				 */
				td = todo_idr;
				if (td != NULL) {
					while (td->next != NULL)
						td = td->next;
					td->next = (struct todo *) malloc(sizeof (*td));
					td = td->next;
				} else {
					todo_idr = td = (struct todo *) malloc(sizeof (*td));
				}
				td->next = NULL;
				td->extent = isonum_733((unsigned char *)idr->extent);
				td->length = isonum_733((unsigned char *)idr->size);
				td->name = (char *) malloc(strlen(rootname)
								+ strlen(name_buf) + 2);
				strcpy(td->name, rootname);
				strcat(td->name, name_buf);
				strcat(td->name, "/");
			} else {
				strcpy(testname, rootname);
				strcat(testname, name_buf);
				if (xtract && strcmp(xtract, testname) == 0) {
					extract_file(idr);
				}
			}
			if (do_find &&
			    (idr->name_len[0] != 1 ||
			    (idr->name[0] != 0 && idr->name[0] != 1))) {
				strcpy(testname, rootname);
				strcat(testname, name_buf);
				printf("%s\n", testname);
			}
			if (do_listing) {
				if ((idr->flags[0] & ISO_MULTIEXTENT) && size == 0)
					sextent = isonum_733((unsigned char *)idr->extent);
				if (debug ||
				    ((idr->flags[0] & ISO_MULTIEXTENT) == 0 && size == 0)) {
					dump_stat(idr, isonum_733((unsigned char *)idr->extent));
				}
				size += fstat_buf.st_size;
				if ((flags & ISO_MULTIEXTENT) &&
				    (idr->flags[0] & ISO_MULTIEXTENT) == 0) {
					fstat_buf.st_size = size;
					if (!debug)
						idr->flags[0] |= ISO_MULTIEXTENT;
					dump_stat(idr, sextent);
					if (!debug)
						idr->flags[0] &= ~ISO_MULTIEXTENT;
				}
				flags = idr->flags[0];
				if ((idr->flags[0] & ISO_MULTIEXTENT) == 0)
					size = 0;
			}
			i += buffer[i];
			if (i > 2048 - offsetof(struct iso_directory_record, name[0])) break;
		}
	}
}

LOCAL void
usage(excode)
	int	excode;
{
	errmsgno(EX_BAD, "Usage: %s [options] -i filename\n", get_progname());

	error("Options:\n");
	error("\t-help,-h	Print this help\n");
	error("\t-version	Print version info and exit\n");
	error("\t-debug		Print additional debug info\n");
	error("\t-d		Print information from the primary volume descriptor\n");
	error("\t-f		Generate output similar to 'find .  -print'\n");
	error("\t-J		Print information from Joliet extensions\n");
	error("\t-j charset	Use charset to display Joliet file names\n");
	error("\t-l		Generate output similar to 'ls -lR'\n");
	error("\t-p		Print Path Table\n");
	error("\t-R		Print information from Rock Ridge extensions\n");
	error("\t-s		Print file size infos in multiples of sector size (%ld bytes).\n", (long)PAGE);
	error("\t-N sector	Sector number where ISO image should start on CD\n");
	error("\t-T sector	Sector number where actual session starts on CD\n");
	error("\t-i filename	Filename to read ISO-9660 image from\n");
	error("\tdev=target	SCSI target to use as CD/DVD-Recorder\n");
	error("\t-x pathname	Extract specified file to stdout\n");
	exit(excode);
}

EXPORT int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	cac;
	char	* const *cav;
	int	c;
	char	* filename = NULL;
	char	* devname = NULL;
	/*
	 * Use toc_offset != 0 (-T #) if we have a complete multi-session
	 * disc that we want/need to play with.
	 * Here we specify the offset where we want to
	 * start searching for the TOC.
	 */
	int	toc_offset = 0;
	int	extent;
	struct todo * td;
	struct iso_primary_descriptor ipd;
	struct iso_primary_descriptor jpd;
	struct eltorito_boot_descriptor bpd;
	struct iso_directory_record * idr;
	char	*charset = NULL;
	char	*opts = "help,h,version,debug,d,p,i*,dev*,J,R,l,x*,f,s,N#l,T#l,j*";
	BOOL	help = FALSE;
	BOOL	prvers = FALSE;
	BOOL	found_eltorito = FALSE;
	int	bootcat_offset = 0;


	save_args(argc, argv);

	cac = argc - 1;
	cav = argv + 1;
	if (getallargs(&cac, &cav, opts,
				&help, &help, &prvers, &debug,
				&do_pvd, &do_pathtab,
				&filename, &devname,
				&use_joliet, &use_rock,
				&do_listing,
				&xtract,
				&do_find, &do_sectors,
				&sector_offset, &toc_offset,
				&charset) < 0) {
		errmsgno(EX_BAD, "Bad Option: '%s'\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prvers) {
		printf("isoinfo %s (%s-%s-%s) Copyright (C) 1993-1999 Eric Youngdale (C) 1999-2007 Jörg Schilling\n",
					VERSION,
					HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}
	cac = argc - 1;
	cav = argv + 1;
	if (getfiles(&cac, &cav, opts) != 0) {
		errmsgno(EX_BAD, "Bad Argument: '%s'\n", cav[0]);
		usage(EX_BAD);
	}

	if (charset == NULL) {
#if	(defined(__CYGWIN32__) || defined(__CYGWIN__) || defined(__DJGPP__)) && !defined(IS_CYGWIN_1)
		unls = sic_open("cp437");
#else
		unls = sic_open("default");
#endif
	} else {
		unls = sic_open(charset);
	}
	if (unls == NULL) {	/* Unknown charset specified */
		fprintf(stderr, "Unknown charset: %s\nKnown charsets are:\n",
							charset);
		sic_list(stdout); /* List all known charset names */

		list_locales();
		exit(EX_BAD);
		exit(1);
	}
	if (use_iconv(unls)) {
		fprintf(stderr, "Iconv not yet supported\n");
		unls = NULL;
	}

	if (filename != NULL && devname != NULL) {
		errmsgno(EX_BAD, "Only one of -i or dev= allowed\n");
		usage(EX_BAD);
	}
#ifdef	USE_SCG
	if (filename == NULL && devname == NULL)
		cdr_defaults(&devname, NULL, NULL, NULL, NULL);
#endif
	if (filename == NULL && devname == NULL) {
		fprintf(stderr, "ISO-9660 image not specified\n");
		usage(EX_BAD);
	}

	if (filename != NULL)
		infile = fopen(filename, "rb");
	else
		filename = devname;

	if (infile != NULL) {
		/* EMPTY */;
#ifdef	USE_SCG
	} else if (scsidev_open(filename) < 0) {
#else
	} else {
#endif
		fprintf(stderr, "Unable to open %s\n", filename);
		exit(1);
	}

	/*
	 * Absolute sector offset, so don't subtract sector_offset here.
	 */
#ifdef	USE_SCG
	readsecs(16 + toc_offset, &ipd, ISO_BLOCKS(sizeof (ipd)));
#else
	lseek(fileno(infile), ((off_t)(16 + toc_offset)) <<11, SEEK_SET);
	read(fileno(infile), &ipd, sizeof (ipd));
#endif
	idr = (struct iso_directory_record *)ipd.root_directory_record;
	if (do_pvd) {
		/*
		 * High sierra:
		 *
		 *	DESC TYPE	== 1 (VD_SFS)	offset 8	len 1
		 *	STR ID		== "CDROM"	offset 9	len 5
		 *	STD_VER		== 1		offset 14	len 1
		 */
		if ((((char *)&ipd)[8] == 1) &&
		    (strncmp(&((char *)&ipd)[9], "CDROM", 5) == 0) &&
		    (((char *)&ipd)[14] == 1)) {
			printf("CD-ROM is in High Sierra format\n");
			exit(0);
		}
		/*
		 * ISO 9660:
		 *
		 *	DESC TYPE	== 1 (VD_PVD)	offset 0	len 1
		 *	STR ID		== "CD001"	offset 1	len 5
		 *	STD_VER		== 1		offset 6	len 1
		 */
		if ((ipd.type[0] != ISO_VD_PRIMARY) ||
		    (strncmp(ipd.id, ISO_STANDARD_ID, sizeof (ipd.id)) != 0) ||
		    (ipd.version[0] != 1)) {
			printf("CD-ROM is NOT in ISO 9660 format\n");
			exit(1);
		}

		printf("CD-ROM is in ISO 9660 format\n");
		if (!use_joliet)
			list_vd(&ipd, FALSE);
		{
			int	block = 16;
			movebytes(&ipd, &jpd, sizeof (ipd));
			while ((Uchar)jpd.type[0] != ISO_VD_END) {

				if (debug && (Uchar) jpd.type[0] == ISO_VD_SUPPLEMENTARY)
					error("Joliet escape sequence 0: '%c' 1: '%c' 2: '%c' 3: '%c'\n",
						jpd.escape_sequences[0],
						jpd.escape_sequences[1],
						jpd.escape_sequences[2],
						jpd.escape_sequences[3]);
					/*
					 * If Joliet UCS escape sequence found, we may be wrong
					 */
					if (jpd.escape_sequences[0] == '%' &&
					    jpd.escape_sequences[1] == '/' &&
					    (jpd.escape_sequences[3] == '\0' ||
					    jpd.escape_sequences[3] == ' ') &&
					    (jpd.escape_sequences[2] == '@' ||
					    jpd.escape_sequences[2] == 'C' ||
					    jpd.escape_sequences[2] == 'E')) {

						if (jpd.version[0] == 1)
							goto nextblock;
				}
				if (jpd.type[0] == 0) {
					movebytes(&jpd, &bpd, sizeof (bpd));
					if (strncmp(bpd.system_id, EL_TORITO_ID, sizeof (EL_TORITO_ID)) == 0) {
						bootcat_offset = (Uchar)bpd.bootcat_ptr[0] +
								(Uchar)bpd.bootcat_ptr[1] * 256 +
								(Uchar)bpd.bootcat_ptr[2] * 65536 +
								(Uchar)bpd.bootcat_ptr[3] * 16777216;
						found_eltorito = TRUE;
						printf("El Torito VD version %d found, boot catalog is in sector %d\n",
							bpd.version[0],
							bootcat_offset);
					}
				}
				if (jpd.version[0] == 2) {
					printf("CD-ROM uses ISO 9660:1999 relaxed format\n");
					break;
				}

			nextblock:
				block++;
#ifdef	USE_SCG
				readsecs(block + toc_offset, &jpd, ISO_BLOCKS(sizeof (jpd)));
#else
				lseek(fileno(infile), ((off_t)(block + toc_offset)) <<11, SEEK_SET);
				read(fileno(infile), &jpd, sizeof (jpd));
#endif
			}
		}
	}
	/*
	 * ISO 9660:
	 *
	 *	DESC TYPE	== 1 (VD_PVD)	offset 0	len 1
	 *	STR ID		== "CD001"	offset 1	len 5
	 *	STD_VER		== 1		offset 6	len 1
	 */
	if ((ipd.type[0] != ISO_VD_PRIMARY) ||
	    (strncmp(ipd.id, ISO_STANDARD_ID, sizeof (ipd.id)) != 0) ||
	    (ipd.version[0] != 1)) {
		printf("CD-ROM is NOT in ISO 9660 format\n");
		exit(1);
	}

	if (use_joliet || do_pvd) {
		int block = 16;
		movebytes(&ipd, &jpd, sizeof (ipd));
		while ((unsigned char) jpd.type[0] != ISO_VD_END) {
			if (debug && (unsigned char) jpd.type[0] == ISO_VD_SUPPLEMENTARY)
				error("Joliet escape sequence 0: '%c' 1: '%c' 2: '%c' 3: '%c'\n",
					jpd.escape_sequences[0],
					jpd.escape_sequences[1],
					jpd.escape_sequences[2],
					jpd.escape_sequences[3]);
			/*
			 * Find the UCS escape sequence.
			 */
			if (jpd.escape_sequences[0] == '%' &&
			    jpd.escape_sequences[1] == '/' &&
			    (jpd.escape_sequences[3] == '\0' ||
			    jpd.escape_sequences[3] == ' ') &&
			    (jpd.escape_sequences[2] == '@' ||
			    jpd.escape_sequences[2] == 'C' ||
			    jpd.escape_sequences[2] == 'E')) {
				break;
			}

			block++;
#ifdef	USE_SCG
			readsecs(block + toc_offset, &jpd, ISO_BLOCKS(sizeof (jpd)));
#else
			lseek(fileno(infile),
				((off_t)(block + toc_offset)) <<11, SEEK_SET);
			read(fileno(infile), &jpd, sizeof (jpd));
#endif
		}

		if (use_joliet && ((unsigned char) jpd.type[0] == ISO_VD_END)) {
			fprintf(stderr, "Unable to find Joliet SVD\n");
			exit(1);
		}

		switch (jpd.escape_sequences[2]) {
		case '@':
			ucs_level = 1;
			break;
		case 'C':
			ucs_level = 2;
			break;
		case 'E':
			ucs_level = 3;
			break;
		}

		if (ucs_level > 3) {
			fprintf(stderr,
				"Don't know what ucs_level == %d means\n",
				ucs_level);
			exit(1);
		}
		if (jpd.escape_sequences[3] == ' ')
			errmsgno(EX_BAD,
			"Warning: Joliet escape sequence uses illegal space at offset 3\n");
	}

	if (do_pvd) {
		if (ucs_level > 0) {
			if (use_joliet) {
				printf("\nJoliet with UCS level %d found,", ucs_level);
				printf(" Joliet volume descriptor:\n");
				list_vd(&jpd, TRUE);
			} else {
				printf("\nJoliet with UCS level %d found.", ucs_level);
			}
		} else {
			printf("NO Joliet present\n");
		}
		extent = isonum_733((unsigned char *)idr->extent);

#ifdef	USE_SCG
		readsecs(extent - sector_offset, buffer, ISO_BLOCKS(sizeof (buffer)));
#else
		lseek(fileno(infile),
			((off_t)(extent - sector_offset)) <<11, SEEK_SET);
		read(fileno(infile), buffer, sizeof (buffer));
#endif
		idr = (struct iso_directory_record *) buffer;
		if ((c = dump_rr(idr)) != 0) {
/*			printf("RR %X %d\n", c, c);*/
			if (c & 1024) {
				printf(
				"\nRock Ridge signatures version %d found\n",
				su_version);
			} else {
				printf(
				"\nBad Rock Ridge signatures found (SU record missing)\n");
			}
			/*
			 * This is currently a no op!
			 * We need to check the first plain file instead of
			 * the '.' entry in the root directory.
			 */
			if (c & 2048) {
				printf("\nApple signatures version %d found\n",
								aa_version);
			}
		} else {
			printf("\nNO Rock Ridge present\n");
		}
		if (found_eltorito)
			printf_bootinfo(infile, bootcat_offset);
		exit(0);
	}

	if (use_joliet)
		idr = (struct iso_directory_record *)jpd.root_directory_record;

	if (do_pathtab) {
		if (use_joliet) {
			dump_pathtab(isonum_731(jpd.type_l_path_table),
			isonum_733((unsigned char *)jpd.path_table_size));
		} else {
			dump_pathtab(isonum_731(ipd.type_l_path_table),
			isonum_733((unsigned char *)ipd.path_table_size));
		}
	}

	parse_dir("/", isonum_733((unsigned char *)idr->extent),
				isonum_733((unsigned char *)idr->size));
	td = todo_idr;
	while (td) {
		parse_dir(td->name, td->extent, td->length);
		td = td->next;
	}

	if (infile != NULL)
		fclose(infile);
	return (0);
}

LOCAL void
list_vd(vp, ucs)
	struct iso_primary_descriptor	*vp;
	BOOL				ucs;
{
	struct iso_directory_record	*idr = (struct iso_directory_record *)
						vp->root_directory_record;

	printf("System id: ");
	printchars(vp->system_id, 32, ucs);
	putchar('\n');
	printf("Volume id: ");
	printchars(vp->volume_id, 32, ucs);
	putchar('\n');

	printf("Volume set id: ");
	printchars(vp->volume_set_id, 128, ucs);
	putchar('\n');
	printf("Publisher id: ");
	printchars(vp->publisher_id, 128, ucs);
	putchar('\n');
	printf("Data preparer id: ");
	printchars(vp->preparer_id, 128, ucs);
	putchar('\n');
	printf("Application id: ");
	printchars(vp->application_id, 128, ucs);
	putchar('\n');

	printf("Copyright File id: ");
	printchars(vp->copyright_file_id, 37, ucs);
	putchar('\n');
	printf("Abstract File id: ");
	printchars(vp->abstract_file_id, 37, ucs);
	putchar('\n');
	printf("Bibliographic File id: ");
	printchars(vp->bibliographic_file_id, 37, ucs);
	putchar('\n');

	printf("Volume set size is: %d\n", isonum_723(vp->volume_set_size));
	printf("Volume set sequence number is: %d\n", isonum_723(vp->volume_sequence_number));
	printf("Logical block size is: %d\n", isonum_723(vp->logical_block_size));
	printf("Volume size is: %d\n", isonum_733((unsigned char *)vp->volume_space_size));
	if (debug) {
		int	dextent;
		int	dlen;

		dextent = isonum_733((unsigned char *)idr->extent);
		dlen = isonum_733((unsigned char *)idr->size);
		printf("Root directory extent:  %d size: %d\n",
			dextent, dlen);
		printf("Path table size is:     %d\n",
			isonum_733((unsigned char *)vp->path_table_size));
		printf("L Path table start:     %d\n",
			isonum_731(vp->type_l_path_table));
		printf("L Path opt table start: %d\n",
			isonum_731(vp->opt_type_l_path_table));
		printf("M Path table start:     %d\n",
			isonum_732(vp->type_m_path_table));
		printf("M Path opt table start: %d\n",
			isonum_732(vp->opt_type_m_path_table));
		printf("Creation Date:     %s\n",
			sdate(vp->creation_date));
		printf("Modification Date: %s\n",
			sdate(vp->modification_date));
		printf("Expiration Date:   %s\n",
			sdate(vp->expiration_date));
		printf("Effective Date:    %s\n",
			sdate(vp->effective_date));
		printf("File structure version: %d\n",
			vp->file_structure_version[0]);
	}
}

LOCAL void
list_locales()
{
	int	n;

	n = sic_list(stdout);
	if (n <= 0) {
		errmsgno(EX_BAD, "'%s/lib/siconv/' %s.\n",
			INS_BASE, n < 0 ? "missing":"incomplete");
		if (n == 0) {
			errmsgno(EX_BAD,
			"Check '%s/lib/siconv/' for missing translation tables.\n",
			INS_BASE);
		}
	}
#ifdef	USE_ICONV
	if (n > 0) {
		errmsgno(EX_BAD,
		"'iconv -l' lists more available names.\n");
	}
#endif
}

#include <schily/intcvt.h>

LOCAL void
printf_bootinfo(f, bootcat_offset)
	FILE	*f;
	int	bootcat_offset;
{
	struct eltorito_validation_entry	*evp;
	struct eltorito_defaultboot_entry	*ebe;

#ifdef	USE_SCG
	readsecs(bootcat_offset, buffer, ISO_BLOCKS(sizeof (buffer)));
#else
	lseek(fileno(f), ((off_t)bootcat_offset) <<11, SEEK_SET);
	read(fileno(f), buffer, sizeof (buffer));
#endif

	evp = (struct eltorito_validation_entry *)buffer;
	ebe = (struct eltorito_defaultboot_entry *)&buffer[32];

	printf("Eltorito validation header:\n");
	printf("    Hid %d\n", (Uchar)evp->headerid[0]);
	printf("    Arch %d (%s)\n", (Uchar)evp->arch[0], arch_name((Uchar)evp->arch[0]));
	printf("    ID '%.23s'\n", evp->id);
	printf("    Key %X %X\n", (Uchar)evp->key1[0], (Uchar)evp->key2[0]);

	printf("    Eltorito defaultboot header:\n");
	printf("        Bootid %X (%s)\n", (Uchar)ebe->boot_id[0], boot_name((Uchar)ebe->boot_id[0]));
	printf("        Boot media %X (%s)\n", (Uchar)ebe->boot_media[0], bootmedia_name((Uchar)ebe->boot_media[0]));
	printf("        Load segment %X\n", la_to_2_byte(ebe->loadseg));
	printf("        Sys type %X\n", (Uchar)ebe->sys_type[0]);
	printf("        Nsect %X\n", la_to_2_byte(ebe->nsect));
	printf("        Bootoff %lX %ld\n", la_to_4_byte(ebe->bootoff), la_to_4_byte(ebe->bootoff));

}

LOCAL char *
arch_name(val)
	int	val;
{
	switch (val) {

	case EL_TORITO_ARCH_x86:
		return ("x86");
	case EL_TORITO_ARCH_PPC:
		return ("PPC");
	case EL_TORITO_ARCH_MAC:
		return ("MAC");
	default:
		return ("Unknown Arch");
	}
}

LOCAL char *
boot_name(val)
	int	val;
{
	switch (val) {

	case EL_TORITO_BOOTABLE:
		return ("bootable");
	case EL_TORITO_NOT_BOOTABLE:
		return ("not bootable");
	default:
		return ("Illegal");
	}
}

LOCAL char *
bootmedia_name(val)
	int	val;
{
	switch (val) {

	case EL_TORITO_MEDIA_NOEMUL:
		return ("No Emulation Boot");
	case EL_TORITO_MEDIA_12FLOP:
		return ("1200 Floppy");
	case EL_TORITO_MEDIA_144FLOP:
		return ("1.44MB Floppy");
	case EL_TORITO_MEDIA_288FLOP:
		return ("2.88MB Floppy");
	case EL_TORITO_MEDIA_HD:
		return ("Hard Disk Emulation");
	default:
		return ("Illegal Bootmedia");
	}
}
