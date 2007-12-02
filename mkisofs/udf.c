/* @(#)udf.c	1.30 07/07/30 Copyright 2001-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)udf.c	1.30 07/07/30 Copyright 2001-2007 J. Schilling";
#endif
/*
 * udf.c - UDF support for mkisofs
 *
 * Written by Ben Rudiak-Gould (2001).
 *
 * Copyright 2001-2007 J. Schilling.
 */
/*
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
 * Some remaining issues:
 *
 * - Do not forget to edit joliet.c and remove the VIDEO_TS lines after
 *   we did implement a decent own file name handling for UDF.
 *
 * - There's no support for hard links. This could be easily fixed since
 *   the correct harlink support is available.
 *
 * - The file system mirrors the Joliet file system, so files excluded
 *   from Joliet will also be excluded from UDF, the -jcharset option
 *   also applies to UDF, file names too long for Joliet will also be
 *   truncated on UDF, and characters not allowed by Joliet will also
 *   be translated on UDF. (Fortunately, Joliet is pretty lenient.)
 *
 * - convert_to_unicode is always called with in_nls, not hfs_nls. This
 *   may lead to incorrect name conversion sometimes when using a Mac
 *   filesystem. See joliet.c for an example of what's supposed to be
 *   done.
 *
 * - DVD-Video discs are supposed to have Copyright Management Information
 *   in both the ISO and UDF filesystems. This is not implemented in ISO,
 *   and the code to do it in UDF is currently #ifdef'd out. I'm not sure
 *   whether discs without this information are actually DVD-Video
 *   compliant. The Copyright Management Information is described in ECMA
 *   Technical Report TR/71.
 *
 * - Most of the space before sector 256 on the disc (~480K) is wasted,
 *   because UDF Bridge requires a pointer block at sector 256. ISO 9660
 *   structures could be moved below sector 256 if they're small enough, but
 *   this would be ugly to implement since it breaks the output_fragment
 *   abstraction.
 *
 * - Each file must have a File Entry, and each File Entry seems to
 *   require its own 2K sector. As a result, there is an overhead of more
 *   than 2K *per file* when using UDF. I couldn't see any way to avoid
 *   this.
 *
 * - Read performance would probably be improved by placing the File Entry
 *   for each file just before the file itself, instead of at the beginning
 *   of the disc. But this would not work for DVD-Video files, which have
 *   to be stored contiguously. So there would have to be an override
 *   mechanism to handle this case. I don't know if it's worth the trouble.
 */

#ifdef UDF

#include <schily/mconfig.h>
#include "mkisofs.h"
#include <schily/time.h>
#include <schily/schily.h>
#include <schily/errno.h>

#include "udf.h"
#include "udf_fs.h"
#include "bswap.h"

extern	int	use_sparcboot;

extern struct directory *root;
extern time_t		begun;

static unsigned lba_main_seq;
static unsigned lba_main_seq_copy;
static unsigned lba_integ_seq;
static unsigned lba_udf_partition_start;
static unsigned lba_last_file_entry;
static unsigned lba_end_anchor_vol_desc;

static unsigned num_udf_files;
static unsigned num_udf_directories;

static unsigned volume_set_id[2];

#define	UDF_MAIN_SEQ_LENGTH (16)
#define	UDF_INTEG_SEQ_LENGTH (2)

/* only works for granularity a power of 2! */
#define	PAD(val, granularity)	(((val)+(granularity)-1)&~((granularity)-1))

#define	INSERTMACRESFORK 1
/* system bit as defined for es_FileInfo.attr */
#define	FI_ATTR_SYSTEM		0x0004

/**************** SIZE ****************/

LOCAL unsigned long getperms __PR((mode_t mode));
LOCAL unsigned int directory_size __PR((struct directory *dpnt));
LOCAL void	assign_udf_directory_addresses __PR((struct directory *dpnt));
LOCAL void	assign_udf_file_entry_addresses __PR((struct directory *dpnt));
LOCAL int	udf_vol_recognition_area_size __PR((UInt32_t starting_extent));
LOCAL int	udf_main_seq_size __PR((UInt32_t starting_extent));
LOCAL int	udf_main_seq_copy_size __PR((UInt32_t starting_extent));
LOCAL int	udf_integ_seq_size __PR((UInt32_t starting_extent));
LOCAL int	udf_end_anchor_vol_desc_size __PR((UInt32_t starting_extent));
LOCAL int	udf_file_set_desc_size __PR((UInt32_t starting_extent));
LOCAL int	udf_dirtree_size __PR((UInt32_t starting_extent));
LOCAL int	udf_file_entries_size __PR((UInt32_t starting_extent));
LOCAL int	udf_pad_to_sector_32_size __PR((UInt32_t starting_extent));
LOCAL int	udf_pad_to_sector_256_size __PR((UInt32_t starting_extent));
LOCAL int	udf_padend_avdp_size __PR((UInt32_t starting_extent));
LOCAL unsigned int crc_ccitt __PR((unsigned char *buf, unsigned int len));
LOCAL void	set16 __PR((udf_Uint16 *dst, unsigned int src));
LOCAL void	set32 __PR((udf_Uint32 *dst, unsigned int src));
LOCAL void	set64 __PR((udf_Uint64 *dst, ULlong src));
LOCAL int	set_ostaunicode __PR((unsigned char *dst, int dst_size, char *src));
LOCAL void	set_extent __PR((udf_extent_ad *ext, UInt32_t lba, unsigned int length_bytes));
LOCAL void	set_dstring __PR((udf_dstring *dst, char *src, int n));
LOCAL void	set_charspec __PR((udf_charspec *dst));
LOCAL void	set_impl_ident __PR((udf_EntityID *ent));
LOCAL void	set_tag __PR((udf_tag *t, unsigned int tid, UInt32_t lba, int crc_length));
LOCAL void	set_timestamp_from_iso_date __PR((udf_timestamp *ts, const char *iso_date_raw));
LOCAL void	set_timestamp_from_time_t __PR((udf_timestamp *ts, time_t t));
LOCAL void	set_anchor_volume_desc_pointer __PR((unsigned char *buf, UInt32_t lba));
LOCAL void	set_primary_vol_desc __PR((unsigned char *buf, UInt32_t lba));
LOCAL void	set_impl_use_vol_desc __PR((unsigned char *buf, UInt32_t lba));
LOCAL void	set_partition_desc __PR((unsigned char *buf, UInt32_t lba));
LOCAL void	set_domain_ident __PR((udf_EntityID *ent));
LOCAL void	set_logical_vol_desc __PR((unsigned char *buf, UInt32_t lba));
LOCAL void	set_unallocated_space_desc __PR((unsigned char *buf, UInt32_t lba));
LOCAL void	set_terminating_desc __PR((unsigned char *buf, UInt32_t lba));
LOCAL void	set_logical_vol_integrity_desc __PR((unsigned char *buf, UInt32_t lba));
LOCAL void	set_file_set_desc __PR((unsigned char *buf, UInt32_t rba));
LOCAL int	set_file_ident_desc __PR((unsigned char *, UInt32_t, char *, int, UInt32_t, unsigned));
LOCAL void	set_file_entry __PR((unsigned char *buf, UInt32_t rba, UInt32_t file_rba,
			off_t length, const char *iso_date, int is_directory,
			unsigned link_count, unsigned unique_id, hfsdirent *hfs_ent,
			unsigned long res_log_block, mode_t fmode, uid_t fuid, gid_t fgid));
LOCAL void	udf_size_panic	__PR((int n));
LOCAL void	set_macvolume_filed_entry __PR((unsigned char *buf, UInt32_t rba, UInt32_t file_rba,
			unsigned length, const char *iso_date, int is_directory,
			unsigned link_count, unsigned unique_id, hfsdirent *hfs_ent,
			mode_t fmode, uid_t fuid, gid_t fgid));
LOCAL void	set_attr_file_entry __PR((unsigned char *buf, unsigned rba, unsigned file_rba,
			off_t length, const char *iso_date, int is_directory,
			unsigned link_count, unsigned unique_id, hfsdirent *hfs_ent,
			mode_t fmode, uid_t fuid, gid_t	fgid));
LOCAL void	set_filed_entry __PR((unsigned char *buf, unsigned rba, unsigned file_rba,
			unsigned length, const char *iso_date, int is_directory,
			unsigned link_count, unsigned unique_id, hfsdirent *hfs_ent,
			mode_t fmode, uid_t fuid, gid_t fgid));
LOCAL unsigned int directory_link_count __PR((struct directory *dpnt));
LOCAL void	write_one_udf_directory __PR((struct directory *dpnt, FILE *outfile));
LOCAL void	write_udf_directories __PR((struct directory *dpnt, FILE *outfile));
LOCAL void	write_udf_file_entries __PR((struct directory *dpnt, FILE *outfile));
LOCAL int	udf_vol_recognition_area_write __PR((FILE *out));
LOCAL int	udf_main_seq_write __PR((FILE *out));
LOCAL int	udf_integ_seq_write __PR((FILE *out));
LOCAL int	udf_anchor_vol_desc_write __PR((FILE *out));
LOCAL int	udf_file_set_desc_write __PR((FILE *out));
LOCAL int	udf_dirtree_write __PR((FILE *out));
LOCAL int	udf_file_entries_write __PR((FILE *out));
LOCAL int	pad_to __PR((UInt32_t last_extent_to_write, FILE *out));
LOCAL int	udf_pad_to_sector_32_write __PR((FILE *out));
LOCAL int	udf_pad_to_sector_256_write __PR((FILE *out));
LOCAL int	udf_padend_avdp_write __PR((FILE *out));

EXPORT void	udf_set_extattr_freespace __PR((unsigned char *buf, off_t size, unsigned rba));
EXPORT void	udf_set_extattr_macresfork __PR((unsigned char *buf, off_t size, unsigned rba));
EXPORT int	assign_dvd_weights __PR((char *name, struct directory *this_dir, int val));
EXPORT int	udf_get_symlinkcontents __PR((char *filename, char *contents, off_t *size));

/*
 * get file access modes
 * although it seems as if UDF modes are the same as UNIX modes
 * this is just to make sure
 */
LOCAL unsigned long
#ifdef	PROTOTYPES
getperms(mode_t mode)
#else
getperms(mode)
	mode_t	mode;
#endif
{
	long m = 0;

	if (mode & S_IRUSR)
		m |= UDF_FILEENTRY_PERMISSION_UR;

	/* not allowed on DVD read-only media according to TR/71 3.5.4 */
	/*
	 * but is required if image is used e.g. as backup medium
	 * so we implement UDF_FILEENTRY_PERMISSION_UW
	 */
	if (mode & S_IWUSR)
		m |= UDF_FILEENTRY_PERMISSION_UW;

	if (mode & S_IXUSR)
		m |= UDF_FILEENTRY_PERMISSION_UX;

	if (mode & S_IRGRP)
		m |= UDF_FILEENTRY_PERMISSION_GR;

	/* not allowed on DVD read-only media according to TR/71 3.5.4 */
	/*
	 * but is required if image is used e.g. as backup medium
	 * so we implement UDF_FILEENTRY_PERMISSION_GW
	 */
	if (mode & S_IWGRP)
		m |= UDF_FILEENTRY_PERMISSION_GW;

	if (mode & S_IXGRP)
		m |= UDF_FILEENTRY_PERMISSION_GX;

	if (mode & S_IROTH)
		m |= UDF_FILEENTRY_PERMISSION_OR;

	/* not allowed on DVD read-only media according to TR/71 3.5.4 */
	/*
	 * but is required if image is used e.g. as backup medium
	 * so we implement UDF_FILEENTRY_PERMISSION_OW
	 */
	if (mode & S_IWOTH)
		m |= UDF_FILEENTRY_PERMISSION_OW;

	if (mode & S_IXOTH)
		m |= UDF_FILEENTRY_PERMISSION_OX;

	return (m);
}

LOCAL unsigned
directory_size(dpnt)
	struct directory	*dpnt;
{
	unsigned size_in_bytes;
	struct directory_entry *de;
	Uchar dummy_buf[SECTOR_SIZE];

	/* parent directory */
	size_in_bytes = set_file_ident_desc(dummy_buf, 0, 0, 0, 0, 0);

	/* directory contents */
	for (de = dpnt->jcontents; de; de = de->jnext) {
		if (!(de->de_flags & INHIBIT_UDF_ENTRY)) {
			char *name = USE_MAC_NAME(de) ? de->hfs_ent->name : de->name;
			/* skip . and .. */
			if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0)))
				continue;
			size_in_bytes += set_file_ident_desc(dummy_buf, 0, name, 0, 0, 0);
		}
	}
	return (size_in_bytes);
}

LOCAL void
assign_udf_directory_addresses(dpnt)
	struct directory	*dpnt;
{
	if (!(dpnt->dir_flags & INHIBIT_UDF_ENTRY)) {
		dpnt->self->udf_file_entry_sector = last_extent;
		last_extent += 1 + ISO_BLOCKS(directory_size(dpnt));
		++num_udf_directories;
	}
	if (!(dpnt->dir_flags & INHIBIT_UDF_ENTRY) || dpnt == reloc_dir) {
		for (dpnt = dpnt->subdir; dpnt; dpnt = dpnt->next) {
			assign_udf_directory_addresses(dpnt);
		}
	}
}

LOCAL void
assign_udf_file_entry_addresses(dpnt)
	struct directory	*dpnt;
{
	if (!(dpnt->dir_flags & INHIBIT_UDF_ENTRY)) {
		struct directory_entry *de;
		for (de = dpnt->jcontents; de; de = de->jnext) {
			if (!(de->de_flags & RELOCATED_DIRECTORY) &&
			    !(de->isorec.flags[0] & ISO_DIRECTORY)) {
				de->udf_file_entry_sector = last_extent++;
				++num_udf_files;
#ifdef INSERTMACRESFORK
				if (de->assoc) {
					last_extent++;
					num_udf_files--;
				}
#endif
			}
		}
	}
	if (!(dpnt->dir_flags & INHIBIT_UDF_ENTRY) || dpnt == reloc_dir) {
		for (dpnt = dpnt->subdir; dpnt; dpnt = dpnt->next) {
			assign_udf_file_entry_addresses(dpnt);
		}
	}
}

/****************************/

LOCAL int
udf_vol_recognition_area_size(starting_extent)
	UInt32_t	starting_extent;
{
	last_extent = starting_extent+3;
	return (0);
}

LOCAL int
udf_main_seq_size(starting_extent)
	UInt32_t	starting_extent;
{
	lba_main_seq = starting_extent;
	last_extent = starting_extent + UDF_MAIN_SEQ_LENGTH;
	return (0);
}

LOCAL int
udf_main_seq_copy_size(starting_extent)
	UInt32_t	starting_extent;
{
	lba_main_seq_copy = starting_extent;
	last_extent = starting_extent + UDF_MAIN_SEQ_LENGTH;
	return (0);
}

LOCAL int
udf_integ_seq_size(starting_extent)
	UInt32_t	starting_extent;
{
	lba_integ_seq = starting_extent;
	last_extent = starting_extent + UDF_INTEG_SEQ_LENGTH;
	return (0);
}

LOCAL int
udf_end_anchor_vol_desc_size(starting_extent)
	UInt32_t	starting_extent;
{
	lba_end_anchor_vol_desc = starting_extent;
	last_extent = starting_extent+1;
	return (0);
}

LOCAL int
udf_file_set_desc_size(starting_extent)
	UInt32_t	starting_extent;
{
	lba_udf_partition_start = starting_extent;
	last_extent = starting_extent+2;
	return (0);
}

LOCAL int
udf_dirtree_size(starting_extent)
	UInt32_t	starting_extent;
{
	num_udf_directories = 0;
	assign_udf_directory_addresses(root);
	return (0);
}

LOCAL int
udf_file_entries_size(starting_extent)
	UInt32_t	starting_extent;
{
	num_udf_files = 0;
	assign_udf_file_entry_addresses(root);
	lba_last_file_entry = last_extent-1;
	return (0);
}

LOCAL int
udf_pad_to_sector_32_size(starting_extent)
	UInt32_t	starting_extent;
{
	if (last_extent < session_start+32)
		last_extent = session_start+32;
	return (0);
}

LOCAL int
udf_pad_to_sector_256_size(starting_extent)
	UInt32_t	starting_extent;
{
	if (last_extent < session_start+256)
		last_extent = session_start+256;
	return (0);
}

LOCAL int
udf_padend_avdp_size(starting_extent)
	UInt32_t	starting_extent;
{
	lba_end_anchor_vol_desc = starting_extent;

	/* add at least 16 and at most 31 sectors, ending at a mult. of 16 */
	last_extent = (starting_extent+31) & ~15;
	if (!use_sparcboot)
		last_extent = starting_extent + 150;
	return (0);
}

/**************** WRITE ****************/

LOCAL unsigned
crc_ccitt(buf, len)
	unsigned char	*buf;
	unsigned	len;
{
	const unsigned poly = 0x11021;
static	unsigned short lookup[256];
	unsigned int r;
	unsigned int i;

	if (lookup[1] == 0) {
		unsigned int j, k;
		for (j = 0; j < 256; ++j) {
			unsigned int temp = j << 8;
			for (k = 0; k < 8; ++k) {
				unsigned int hibit = temp & 32768;
				temp <<= 1;
				if (hibit)
					temp ^= poly;
			}
			lookup[j] = temp;
		}
	}

	r = 0;
	for (i = 0; i < len; ++i) {
		r = (r << 8) ^ lookup[((r >> 8) ^ buf[i]) & 255];
	}

	return (r & 65535);
}

#define	set8(dst, src)	do { *(dst) = (src); } while (0)

LOCAL void
set16(dst, src)
	udf_Uint16	*dst;
	unsigned int	src;
{
	dst->l = (char)(src);
	dst->h = (char)(src>>8);
}

LOCAL void
set32(dst, src)
	udf_Uint32	*dst;
	unsigned	src;
{
	dst->l  = (char)(src);
	dst->ml = (char)(src>>8);
	dst->mh = (char)(src>>16);
	dst->h  = (char)(src>>24);
}

LOCAL void
set64(dst, src)
	udf_Uint64	*dst;
	ULlong 		src;
{
	ULlong ll1;
	ULlong ll2;

	set32(&dst->l, src);
	/*
	 * src>>32 actually does the wrong thing on x86 with at least
	 * one compiler, because of x86's shift count masking.
	 * The following code should work with all compilers
	 */
	/*set32(&dst->h, src>>32);*/
	ll1 = src >> 24;
	ll2 = ll1 >> 8;
	set32(&dst->h, ll2);
}

LOCAL int
set_ostaunicode(dst, dst_size, src)
	unsigned char	*dst;
	int		dst_size;
	char		*src;
{
	unsigned char buf[1024];
	int i;
	int expanded_length;

	expanded_length = joliet_strlen(src, 1024, in_nls);
	if (expanded_length > 1024)
		expanded_length = 1024;
	if (expanded_length > (dst_size-1)*2)
		expanded_length = (dst_size-1)*2;

	convert_to_unicode(buf, expanded_length, src, in_nls);
	dst[0] = 8;	/* use 8-bit representation by default */
	for (i = 0; i < (expanded_length>>1); ++i) {
		dst[i + 1] = buf[i*2+1];
		if (buf[i*2] != 0) {
			/*
			 * There's a Unicode character with value >=256.
			 * Use 16-bit representation instead.
			 */
			int length_to_copy = (dst_size-1) & ~1;
			if (length_to_copy > expanded_length)
				length_to_copy = expanded_length;
			dst[0] = 16;
			memcpy(dst+1, buf, length_to_copy);
			return (length_to_copy + 1);
		}
	}
	return ((expanded_length>>1) + 1);
}

LOCAL void
set_extent(ext, lba, length_bytes)
	udf_extent_ad	*ext;
	UInt32_t	lba;
	unsigned	length_bytes;
{
	set32(&ext->extent_length, length_bytes);
	set32(&ext->extent_location, lba);
}

LOCAL void
set_dstring(dst, src, n)
	udf_dstring	*dst;
	char		*src;
	int		n;
{
	dst[n-1] = set_ostaunicode((Uchar *)dst, n-1, src);
}

LOCAL void
set_charspec(dst)
	udf_charspec	*dst;
{
	/*set8(&dst->character_set_type, 0);*/
	memcpy(dst->character_set_info, "OSTA Compressed Unicode", 23);
}

LOCAL void
set_impl_ident(ent)
	udf_EntityID	*ent;
{
	strcpy((char *)ent->ident, "*mkisofs");
}

LOCAL void
set_tag(t, tid, lba, crc_length)
	udf_tag	*t;
	unsigned	tid;
	UInt32_t	lba;
	int		crc_length;
{
	unsigned char checksum;
	int i;

	set16(&t->tag_ident, tid);
	set16(&t->desc_version, 2);
	set16(&t->desc_crc, crc_ccitt((unsigned char *)t+16, crc_length-16));
	set16(&t->desc_crc_length, crc_length-16);
	set32(&t->tag_location, lba);
	set8(&t->tag_checksum, 0);
	checksum = 0;
	for (i = 0; i < 16; ++i)
		checksum += ((unsigned char *)t)[i];
	set8(&t->tag_checksum, checksum);
}

LOCAL void
set_timestamp_from_iso_date(ts, iso_date_raw)
	udf_timestamp	*ts;
	const char	*iso_date_raw;
{
	struct {
		unsigned char years_since_1900;
		unsigned char month, day;
		unsigned char hour, minute, second;
		signed char offset_from_gmt;
	} *iso_date = (void *)iso_date_raw;

	set16(&ts->type_and_time_zone,
		4096 + ((iso_date->offset_from_gmt * 15) & 4095));
	set16(&ts->year, 1900 + iso_date->years_since_1900);
	set8(&ts->month, iso_date->month);
	set8(&ts->day, iso_date->day);
	set8(&ts->hour, iso_date->hour);
	set8(&ts->minute, iso_date->minute);
	set8(&ts->second, iso_date->second);
	/*set8(&ts->centiseconds, 0);*/
	/*set8(&ts->hundreds_of_microseconds, 0);*/
	/*set8(&ts->microseconds, 0);*/
}

LOCAL void
set_timestamp_from_time_t(ts, t)
	udf_timestamp	*ts;
	time_t		t;
{
	char iso_date[7];
	iso9660_date(iso_date, t);
	set_timestamp_from_iso_date(ts, iso_date);
}


LOCAL void
set_anchor_volume_desc_pointer(buf, lba)
	unsigned char	*buf;
	UInt32_t	lba;
{
	udf_anchor_volume_desc_ptr *avdp = (udf_anchor_volume_desc_ptr *)buf;
	set_extent(&avdp->main_volume_desc_seq_extent,
		lba_main_seq, SECTOR_SIZE*UDF_MAIN_SEQ_LENGTH);
	set_extent(&avdp->reserve_volume_desc_seq_extent,
		lba_main_seq_copy, SECTOR_SIZE*UDF_MAIN_SEQ_LENGTH);
	set_tag(&avdp->desc_tag, UDF_TAGID_ANCHOR_VOLUME_DESC_PTR, lba, 512);
}

LOCAL void
set_primary_vol_desc(buf, lba)
	unsigned char	*buf;
	UInt32_t	lba;
{
	char temp[17];

	udf_primary_volume_desc *pvd = (udf_primary_volume_desc *)buf;
	/*set32(&pvd->volume_desc_seq_number, 0);*/
	/*set32(&pvd->primary_volume_desc_number, 0);*/
	set_dstring(pvd->volume_ident, volume_id, sizeof (pvd->volume_ident));
	set16(&pvd->volume_seq_number, 1);
	set16(&pvd->maximum_volume_seq_number, 1);
	set16(&pvd->interchange_level, 2);
	set16(&pvd->maximum_interchange_level, 2);
	set32(&pvd->character_set_list, 1);
	set32(&pvd->maximum_character_set_list, 1);
	sprintf(temp, "%08X%08X", volume_set_id[0], volume_set_id[1]);
	set_dstring(pvd->volume_set_ident, temp,
					sizeof (pvd->volume_set_ident));
	set_charspec(&pvd->desc_character_set);
	set_charspec(&pvd->explanatory_character_set);
	/*pvd->volume_abstract;*/
	/*pvd->volume_copyright_notice;*/
	/*pvd->application_ident;*/
	set_timestamp_from_time_t(&pvd->recording_date_and_time, begun);
	set_impl_ident(&pvd->impl_ident);
	set_tag(&pvd->desc_tag, UDF_TAGID_PRIMARY_VOLUME_DESC, lba, 512);
}

LOCAL void
set_impl_use_vol_desc(buf, lba)
	unsigned char	*buf;
	UInt32_t	lba;
{
	udf_impl_use_volume_desc *iuvd = (udf_impl_use_volume_desc *)buf;
	set32(&iuvd->volume_desc_seq_number, 1);
	strcpy((char *)iuvd->impl_ident.ident, "*UDF LV Info");
	iuvd->impl_ident.ident_suffix[0] = 2;
	iuvd->impl_ident.ident_suffix[1] = 1;
	set_charspec(&iuvd->impl_use.lvi_charset);
	set_dstring(iuvd->impl_use.logical_volume_ident, volume_id,
		sizeof (iuvd->impl_use.logical_volume_ident));
	/*set_dstring(iuvd->impl_use.lv_info1, "", sizeof (iuvd->impl_use.lv_info1));*/
	/*set_dstring(iuvd->impl_use.lv_info2, "", sizeof (iuvd->impl_use.lv_info2));*/
	/*set_dstring(iuvd->impl_use.lv_info3, "", sizeof (iuvd->impl_use.lv_info3));*/
	set_impl_ident(&iuvd->impl_use.impl_id);
	set_tag(&iuvd->desc_tag, UDF_TAGID_IMPL_USE_VOLUME_DESC, lba, 512);
}

LOCAL void
set_partition_desc(buf, lba)
	unsigned char	*buf;
	UInt32_t	lba;
{
	udf_partition_desc *pd = (udf_partition_desc *)buf;
	set32(&pd->volume_desc_seq_number, 2);
	set16(&pd->partition_flags, UDF_PARTITION_FLAG_ALLOCATED);
	/*set16(&pd->partition_number, 0);*/
	set8(&pd->partition_contents.flags, UDF_ENTITYID_FLAG_PROTECTED);	/*???*/
	strcpy((char *)pd->partition_contents.ident, "+NSR02");
	set32(&pd->access_type, UDF_ACCESSTYPE_READONLY);
	set32(&pd->partition_starting_location, lba_udf_partition_start);
	set32(&pd->partition_length,
			lba_end_anchor_vol_desc - lba_udf_partition_start);
	set_impl_ident(&pd->impl_ident);
	set_tag(&pd->desc_tag, UDF_TAGID_PARTITION_DESC, lba, 512);
}

LOCAL void
set_domain_ident(ent)
	udf_EntityID	*ent;
{
	strcpy((char *)ent->ident, "*OSTA UDF Compliant");
	memcpy(ent->ident_suffix, "\002\001\003", 3);
}

LOCAL void
set_logical_vol_desc(buf, lba)
	unsigned char	*buf;
	UInt32_t	lba;
{
	udf_logical_volume_desc *lvd = (udf_logical_volume_desc *)buf;
	set32(&lvd->volume_desc_seq_number, 3);
	set_charspec(&lvd->desc_character_set);
	set_dstring(lvd->logical_volume_ident, volume_id,
					sizeof (lvd->logical_volume_ident));
	set32(&lvd->logical_block_size, SECTOR_SIZE);
	set_domain_ident(&lvd->domain_ident);
	set32(&lvd->logical_volume_contents_use.extent_length, 2*SECTOR_SIZE);
	/*set32(&lvd->logical_volume_contents_use.extent_location.logical_block_number, 0);*/
	/*set16(&lvd->logical_volume_contents_use.extent_location.partition_reference_number, 0);*/
	set32(&lvd->map_table_length, 6);
	set32(&lvd->number_of_partition_maps, 1);
	set_impl_ident(&lvd->impl_ident);
	set_extent(&lvd->integrity_seq_extent, lba_integ_seq,
					SECTOR_SIZE*UDF_INTEG_SEQ_LENGTH);
	set8(&lvd->partition_map[0].partition_map_type,
					UDF_PARTITION_MAP_TYPE_1);
	set8(&lvd->partition_map[0].partition_map_length, 6);
	set16(&lvd->partition_map[0].volume_seq_number, 1);
	/*set16(&lvd->partition_map[0].partition_number, 0);*/
	set_tag(&lvd->desc_tag, UDF_TAGID_LOGICAL_VOLUME_DESC, lba, 446);
}

LOCAL void
set_unallocated_space_desc(buf, lba)
	unsigned char	*buf;
	UInt32_t	lba;
{
	udf_unallocated_space_desc *usd = (udf_unallocated_space_desc *)buf;
	set32(&usd->volume_desc_seq_number, 4);
	/*set32(&usd->number_of_allocation_descs, 0);*/
	set_tag(&usd->desc_tag, UDF_TAGID_UNALLOCATED_SPACE_DESC, lba, 24);
}

LOCAL void
set_terminating_desc(buf, lba)
	unsigned char	*buf;
	UInt32_t	lba;
{
	udf_terminating_desc *td = (udf_terminating_desc *)buf;
	set_tag(&td->desc_tag, UDF_TAGID_TERMINATING_DESC, lba, 512);
}

LOCAL void
set_logical_vol_integrity_desc(buf, lba)
	unsigned char	*buf;
	UInt32_t	lba;
{
	udf_logical_volume_integrity_desc *lvid =
				(udf_logical_volume_integrity_desc *)buf;

	set_timestamp_from_time_t(&lvid->recording_date, begun);
	set32(&lvid->integrity_type, UDF_INTEGRITY_TYPE_CLOSE);
	/*lvid->next_integrity_extent;*/
	set64(&lvid->logical_volume_contents_use.unique_id,
						lba_last_file_entry+1);
	set32(&lvid->number_of_partitions, 1);
	set32(&lvid->length_of_impl_use, 46);
	/*set32(&lvid->free_space_table, 0);*/
	set32(&lvid->size_table,
			lba_end_anchor_vol_desc - lba_udf_partition_start);
	set_impl_ident(&lvid->impl_use.impl_id);
	set32(&lvid->impl_use.number_of_files, num_udf_files);
	set32(&lvid->impl_use.number_of_directories, num_udf_directories);
	/* note by HELIOS: the following three need to be 0x200 if extended file entries are used */
	set16(&lvid->impl_use.minimum_udf_read_revision, 0x102);
	set16(&lvid->impl_use.minimum_udf_write_revision, 0x102);
	set16(&lvid->impl_use.maximum_udf_write_revision, 0x102);
	set_tag(&lvid->desc_tag, UDF_TAGID_LOGICAL_VOLUME_INTEGRITY_DESC,
								lba, 88+46);
}

LOCAL void
set_file_set_desc(buf, rba)
	unsigned char	*buf;
	UInt32_t	rba;
{
	udf_file_set_desc *fsd = (udf_file_set_desc *)buf;

	set_timestamp_from_time_t(&fsd->recording_date_and_time, begun);
	set16(&fsd->interchange_level, 3);
	set16(&fsd->maximum_interchange_level, 3);
	set32(&fsd->character_set_list, 1);
	set32(&fsd->maximum_character_set_list, 1);
	/*set32(&fsd->file_set_number, 0);*/
	/*set32(&fsd->file_set_desc_number, 0);*/
	set_charspec(&fsd->logical_volume_ident_character_set);
	set_dstring(fsd->logical_volume_ident, volume_id,
					sizeof (fsd->logical_volume_ident));
	set_charspec(&fsd->file_set_character_set);
	set_dstring(fsd->file_set_ident, volume_id,
					sizeof (fsd->file_set_ident));
	/*fsd->copyright_file_ident;*/
	/*fsd->abstract_file_ident;*/
	set32(&fsd->root_directory_icb.extent_length, SECTOR_SIZE);
	set32(&fsd->root_directory_icb.extent_location.logical_block_number,
		root->self->udf_file_entry_sector - lba_udf_partition_start);
	set_domain_ident(&fsd->domain_ident);
	/*fsd->next_extent;*/
	set_tag(&fsd->desc_tag, UDF_TAGID_FILE_SET_DESC, rba, 512);
}

LOCAL int
set_file_ident_desc(buf, rba, name, is_directory, file_entry_rba, unique_id)
	unsigned char	*buf;
	UInt32_t	rba;
	char		*name;
	int		is_directory;
	UInt32_t	file_entry_rba;
	unsigned	unique_id;
{
	udf_file_ident_desc *fid = (udf_file_ident_desc *)buf;
	int length_of_file_ident, length, padded_length;
	set16(&fid->file_version_number, 1);
	set8(&fid->file_characteristics,
		(is_directory ? UDF_FILE_CHARACTERISTIC_DIRECTORY : 0)
		+ (name == 0) * UDF_FILE_CHARACTERISTIC_PARENT);
	set32(&fid->icb.extent_length, SECTOR_SIZE);
	set32(&fid->icb.extent_location.logical_block_number, file_entry_rba);
	set16(&fid->icb.extent_location.partition_reference_number, 0);
	set32(&fid->icb.impl_use.unique_id, unique_id);
	set16(&fid->length_of_impl_use, 0);
	if (name) {
		length_of_file_ident =
			set_ostaunicode((Uchar *)fid->file_ident, 512, name);
	} else {
		length_of_file_ident = 0;
	}
	set8(&fid->length_of_file_ident, length_of_file_ident);
	length = 38 + length_of_file_ident;
	padded_length = PAD(length, 4);
	while (length < padded_length) {
		buf[length++] = 0;
	}
	set_tag(&fid->desc_tag, UDF_TAGID_FILE_IDENT_DESC, rba, length);
	return (length);
}

LOCAL void
#ifdef PROTOTYPES
set_file_entry(unsigned char *buf,
	UInt32_t rba,
	UInt32_t file_rba,
	off_t length,
	const char *iso_date,
	int is_directory,
	unsigned link_count,
	unsigned unique_id,

	hfsdirent *hfs_ent,
	unsigned long res_log_block,
	mode_t	fmode,
	uid_t	fuid,
	gid_t	fgid)
#else
set_file_entry(buf, rba, file_rba, length, iso_date, is_directory, link_count,
		unique_id, hfs_ent,
		res_log_block, fmode, fuid, fgid)
	unsigned char	*buf;
	UInt32_t	rba;
	UInt32_t	file_rba;
	off_t		length;
	const char	*iso_date;
	int		is_directory;
	unsigned	link_count;
	unsigned	unique_id;

	hfsdirent	*hfs_ent;
	unsigned long	res_log_block;
	mode_t		fmode;
	uid_t		fuid;
	gid_t		fgid;
#endif
{
	udf_short_ad	*allocation_desc;
	unsigned	chunk;
	unsigned short	checksum;
	int		i;
	unsigned char *p;
	unsigned short	flags;
	short	macflags;

	udf_file_entry *fe = (udf_file_entry *)buf;

	/*set32(&fe->icb_tag.prior_recorded_number_of_direct_entries, 0);*/
	set16(&fe->icb_tag.strategy_type, 4);
	/*set16(&fe->icb_tag.strategy_parameter, 0);*/
	set16(&fe->icb_tag.maximum_number_of_entries, 1);
	if (S_ISLNK(fmode)) {
		set8(&fe->icb_tag.file_type, UDF_ICBTAG_FILETYPE_SYMLINK);
	} else
		set8(&fe->icb_tag.file_type, UDF_ICBTAG_FILETYPE_BYTESEQ);
	/*fe->icb_tag.parent_icb_location;*/
	/* UDF_ICBTAG_FLAG_SYSTEM shall be set for MS-DOS, OS/2, Win95 and WinNT as of UDF260 3.3.2.1 */
	flags = UDF_ICBTAG_FLAG_NONRELOCATABLE | UDF_ICBTAG_FLAG_ARCHIVE | UDF_ICBTAG_FLAG_CONTIGUOUS;
	if (hfs_ent) {
		macflags = hfs_ent->fdflags;
		B2N_16(macflags);
		if (macflags & FI_ATTR_SYSTEM) {
			flags |= UDF_ICBTAG_FLAG_SYSTEM;
		}
	}
	set16(&fe->icb_tag.flags, flags);

	set32(&fe->permissions, getperms(fmode));
	if (rationalize_uid)
		set32(&fe->uid, uid_to_use);
	else
		set32(&fe->uid, fuid);
	if (rationalize_gid)
		set32(&fe->gid, gid_to_use);
	else
		set32(&fe->gid, fgid);


	set16(&fe->file_link_count, link_count);
	/*fe->record_format;*/
	/*fe->record_display_attributes;*/
	/*fe->record_length;*/
	set64(&fe->info_length, length);
	set64(&fe->logical_blocks_recorded, ISO_BLOCKS(length));
	if (iso_date) {
		set_timestamp_from_iso_date(&fe->access_time, iso_date);
		fe->modification_time = fe->access_time;
		fe->attribute_time = fe->access_time;
	}
	set32(&fe->checkpoint, 1);

	if (res_log_block) {
		set32(&fe->ext_attribute_icb.extent_length, 2048);
		set32(&fe->ext_attribute_icb.extent_location.logical_block_number, res_log_block);
		/* &p->ext_attribute_icb.extent_location.partition_reference_number */
	}

	set_impl_ident(&fe->impl_ident);
	set64(&fe->unique_id, unique_id);

	/* write mac finderinfos etc. required for directories and files */
	set32(&fe->length_of_ext_attributes, sizeof (udf_ext_attribute_header_desc) +
		sizeof (udf_ext_attribute_free_ea_space) + sizeof (udf_ext_attribute_dvd_cgms_info) +
		sizeof (udf_ext_attribute_file_macfinderinfo));

	set32(&fe->ext_attribute_header.impl_attributes_location, sizeof (udf_ext_attribute_header_desc));
	set32(&fe->ext_attribute_header.application_attributes_location, sizeof (udf_ext_attribute_header_desc) +
		sizeof (udf_ext_attribute_free_ea_space) + sizeof (udf_ext_attribute_dvd_cgms_info) +
		sizeof (udf_ext_attribute_file_macfinderinfo));
	set_tag(&fe->ext_attribute_header.desc_tag, UDF_TAGID_EXT_ATTRIBUTE_HEADER_DESC, rba,
		sizeof (udf_ext_attribute_header_desc));

	set32(&fe->ext_attribute_free_ea_space.attribute_type, SECTOR_SIZE);
	set8(&fe->ext_attribute_free_ea_space.attribute_subtype, 1);
	set32(&fe->ext_attribute_free_ea_space.attribute_length, 52);
	set32(&fe->ext_attribute_free_ea_space.impl_use_length, 4);
	strcpy((char *)fe->ext_attribute_free_ea_space.impl_ident.ident, "*UDF FreeEASpace");
	fe->ext_attribute_free_ea_space.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_free_ea_space.impl_ident.ident_suffix[1] = 1;

	for (i = 0, checksum = 0, p = (unsigned char *)&fe->ext_attribute_free_ea_space; i < 48; i++)
		checksum += *p++;
	set16(&fe->ext_attribute_free_ea_space.header_checksum, checksum);

	set32(&fe->ext_attribute_dvd_cgms_info.attribute_type, SECTOR_SIZE);
	set8(&fe->ext_attribute_dvd_cgms_info.attribute_subtype, 1);
	set32(&fe->ext_attribute_dvd_cgms_info.attribute_length, 56);
	set32(&fe->ext_attribute_dvd_cgms_info.impl_use_length, 8);
	strcpy((char *)fe->ext_attribute_dvd_cgms_info.impl_ident.ident, "*UDF DVD CGMS Info");
	fe->ext_attribute_dvd_cgms_info.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_dvd_cgms_info.impl_ident.ident_suffix[1] = 1;

	for (i = 0, checksum = 0, p = (unsigned char *)&fe->ext_attribute_dvd_cgms_info; i < 48; i++)
		checksum += *p++;
	set16(&fe->ext_attribute_dvd_cgms_info.header_checksum, checksum);

	set32(&fe->ext_attribute_macfinderinfo.attribute_type, EXTATTR_IMP_USE);
	set8(&fe->ext_attribute_macfinderinfo.attribute_subtype, 1);
	set32(&fe->ext_attribute_macfinderinfo.attribute_length, 96);
	set32(&fe->ext_attribute_macfinderinfo.impl_use_length, 48);
	strcpy((char *)fe->ext_attribute_macfinderinfo.impl_ident.ident, "*UDF Mac FinderInfo");
	fe->ext_attribute_macfinderinfo.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_macfinderinfo.impl_ident.ident_suffix[1] = 1;

	for (i = 0, checksum = 0, p = (unsigned char *)&fe->ext_attribute_macfinderinfo; i < 48; i++)
		checksum += *p++;
	set16(&fe->ext_attribute_macfinderinfo.finderinfo.headerchecksum, checksum);

	/* write mac finderinfos etc. required for files */
	if (hfs_ent) {
		fe->ext_attribute_macfinderinfo.finderinfo.fileinfo.fdtype.l = hfs_ent->u.file.type[3];
		fe->ext_attribute_macfinderinfo.finderinfo.fileinfo.fdtype.ml = hfs_ent->u.file.type[2];
		fe->ext_attribute_macfinderinfo.finderinfo.fileinfo.fdtype.mh = hfs_ent->u.file.type[1];
		fe->ext_attribute_macfinderinfo.finderinfo.fileinfo.fdtype.h = hfs_ent->u.file.type[0];

		fe->ext_attribute_macfinderinfo.finderinfo.fileinfo.fdcreator.l = hfs_ent->u.file.creator[3];
		fe->ext_attribute_macfinderinfo.finderinfo.fileinfo.fdcreator.ml = hfs_ent->u.file.creator[2];
		fe->ext_attribute_macfinderinfo.finderinfo.fileinfo.fdcreator.mh = hfs_ent->u.file.creator[1];
		fe->ext_attribute_macfinderinfo.finderinfo.fileinfo.fdcreator.h = hfs_ent->u.file.creator[0];

		fe->ext_attribute_macfinderinfo.finderinfo.fileinfo.fdflags.l = ((char *)&macflags)[1];
		fe->ext_attribute_macfinderinfo.finderinfo.fileinfo.fdflags.h = ((char *)&macflags)[0];

#ifdef INSERTMACRESFORK
		set32(&fe->ext_attribute_macfinderinfo.finderinfo.resourcedatalength, hfs_ent->u.file.rsize);
		set32(&fe->ext_attribute_macfinderinfo.finderinfo.resourcealloclength, hfs_ent->u.file.rsize);
#endif
	}

	/*
	 * Extended attributes that may (?) be required for DVD-Video
	 * compliance
	 */
#if 0
	set32(&fe->length_of_ext_attributes, 24+52+56);
	set32(&fe->ext_attribute_header.impl_attributes_location, 24);
	set32(&fe->ext_attribute_header.application_attributes_location,
								24+52+56);
	set_tag(&fe->ext_attribute_header.desc_tag,
			UDF_TAGID_EXT_ATTRIBUTE_HEADER_DESC, rba, 24 /*???*/);
	set32(&fe->ext_attribute_free_ea_space.attribute_type, SECTOR_SIZE);
	set8(&fe->ext_attribute_free_ea_space.attribute_subtype, 1);
	set32(&fe->ext_attribute_free_ea_space.attribute_length, 52);
	set32(&fe->ext_attribute_free_ea_space.impl_use_length, 4);
	strcpy((char *)fe->ext_attribute_free_ea_space.impl_ident.ident,
							"*UDF FreeAppEASpace");
	set32(&fe->ext_attribute_dvd_cgms_info.attribute_type, SECTOR_SIZE);
	set8(&fe->ext_attribute_dvd_cgms_info.attribute_subtype, 1);
	set32(&fe->ext_attribute_dvd_cgms_info.attribute_length, 56);
	set32(&fe->ext_attribute_dvd_cgms_info.impl_use_length, 8);
	strcpy((char *)fe->ext_attribute_free_ea_space.impl_ident.ident,
							"*UDF DVD CGMS Info");
	fe->ext_attribute_free_ea_space.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_free_ea_space.impl_ident.ident_suffix[1] = 1;
#else
	/*set32(&fe->length_of_ext_attributes, 0);*/
#endif

	allocation_desc = &fe->allocation_desc;
	/*
	 * Only a file size less than 1GB can be expressed by a single
	 * AllocationDescriptor. When the size of a file is larger than 1GB,
	 * 2 or more AllocationDescriptors should be used. We don't know
	 * whether a single 8-byte AllocationDescriptor should be written or no
	 * one should be written if the size of a file is 0 byte. - FIXME.
	 *
	 * XXX We get called with buf[2048]. This allows a max. file size of
	 * XXX 234 GB. With more we would cause a buffer overflow.
	 * XXX We need to check whether UDF would allow files > 234 GB.
	 */
	for (; length > 0; length -= chunk) {
		chunk = (length > 0x3ffff800) ? 0x3ffff800 : length;
		set32(&allocation_desc->extent_length, chunk);
		set32(&allocation_desc->extent_position, file_rba);
		file_rba += chunk >> 11;
		allocation_desc++;
	}
	if (((Uchar *)allocation_desc) > &buf[2048])
		udf_size_panic(allocation_desc - &fe->allocation_desc);

	set32(&fe->length_of_allocation_descs,
				(unsigned char *) allocation_desc -
				(unsigned char *) &fe->allocation_desc);
	set_tag(&fe->desc_tag, UDF_TAGID_FILE_ENTRY, rba,
		(unsigned char *) allocation_desc - buf);
}

LOCAL void
udf_size_panic(n)
	int	n;
{
	comerrno(EX_BAD,
		"Panic: UDF file size error, too many extents (%d).\n", n);
}


LOCAL void
#ifdef PROTOTYPES
set_macvolume_filed_entry(unsigned char *buf,
	UInt32_t rba,
	UInt32_t file_rba,
	unsigned length,
	const char *iso_date,
	int is_directory,
	unsigned link_count,
	unsigned unique_id,
	hfsdirent *hfs_ent,
	mode_t	fmode,
	uid_t	fuid,
	gid_t	fgid
)
#else
set_macvolume_filed_entry(buf, rba, file_rba, length, iso_date, is_directory, link_count, unique_id, hfs_ent, fmode, fuid, fgid)
	unsigned char	*buf;
	UInt32_t	rba;
	UInt32_t	file_rba;
	unsigned	length;
	const char	*iso_date;
	int		is_directory;
	unsigned	link_count;
	unsigned	unique_id;
	hfsdirent	*hfs_ent;
	mode_t		fmode;
	uid_t		fuid;
	gid_t		fgid;
#endif
{
	udf_short_ad	*allocation_desc;
	unsigned	chunk;
	unsigned short	checksum;
	int		i;
	unsigned char	*p;
	unsigned short	flags;
	short		macflags;

	udf_macvolume_filed_entry *fe = (udf_macvolume_filed_entry *)buf;

	/*set32(&fe->icb_tag.prior_recorded_number_of_direct_entries, 0);*/
	set16(&fe->icb_tag.strategy_type, 4);
	/*set16(&fe->icb_tag.strategy_parameter, 0);*/
	set16(&fe->icb_tag.maximum_number_of_entries, 1);
	set8(&fe->icb_tag.file_type, UDF_ICBTAG_FILETYPE_DIRECTORY);
	/*fe->icb_tag.parent_icb_location;*/
	/* UDF_ICBTAG_FLAG_SYSTEM shall be set for MS-DOS, OS/2, Win95 and WinNT as of UDF260 3.3.2.1 */
	flags = UDF_ICBTAG_FLAG_NONRELOCATABLE | UDF_ICBTAG_FLAG_ARCHIVE | UDF_ICBTAG_FLAG_CONTIGUOUS;
	if (hfs_ent) {
		macflags = hfs_ent->fdflags;
		B2N_16(macflags);
		if (macflags & FI_ATTR_SYSTEM) {
			flags |= UDF_ICBTAG_FLAG_SYSTEM;
		}
	}
	set16(&fe->icb_tag.flags, flags);

	set32(&fe->permissions, getperms(fmode));
	if (rationalize_uid)
		set32(&fe->uid, uid_to_use);
	else
		set32(&fe->uid, fuid);
	if (rationalize_gid)
		set32(&fe->gid, gid_to_use);
	else
		set32(&fe->gid, fgid);

	set16(&fe->file_link_count, link_count);
	/*fe->record_format;*/
	/*fe->record_display_attributes;*/
	/*fe->record_length;*/
	set64(&fe->info_length, length);
	set64(&fe->logical_blocks_recorded, ISO_BLOCKS(length));
	if (iso_date) {
		set_timestamp_from_iso_date(&fe->access_time, iso_date);
		fe->modification_time = fe->access_time;
		fe->attribute_time = fe->access_time;
	}
	set32(&fe->checkpoint, 1);
	/*fe->ext_attribute_icb;*/
	set_impl_ident(&fe->impl_ident);
	set64(&fe->unique_id, unique_id);

	/*write mac finderinfos etc. required for directories */
	set32(&fe->length_of_ext_attributes, sizeof (udf_ext_attribute_header_desc) +
		sizeof (udf_ext_attribute_free_ea_space) + sizeof (udf_ext_attribute_dvd_cgms_info) +
		sizeof (udf_ext_attribute_dir_macvolinfo) + sizeof (udf_ext_attribute_dir_macfinderinfo));

	set32(&fe->ext_attribute_header.impl_attributes_location, sizeof (udf_ext_attribute_header_desc));
	set32(&fe->ext_attribute_header.application_attributes_location, sizeof (udf_ext_attribute_header_desc) +
		sizeof (udf_ext_attribute_free_ea_space) + sizeof (udf_ext_attribute_dvd_cgms_info) +
		sizeof (udf_ext_attribute_dir_macvolinfo) + sizeof (udf_ext_attribute_dir_macfinderinfo));
	set_tag(&fe->ext_attribute_header.desc_tag, UDF_TAGID_EXT_ATTRIBUTE_HEADER_DESC, rba,
		sizeof (udf_ext_attribute_header_desc));

	set32(&fe->ext_attribute_free_ea_space.attribute_type, SECTOR_SIZE);
	set8(&fe->ext_attribute_free_ea_space.attribute_subtype, 1);
	set32(&fe->ext_attribute_free_ea_space.attribute_length, 52);
	set32(&fe->ext_attribute_free_ea_space.impl_use_length, 4);
	strcpy((char *)fe->ext_attribute_free_ea_space.impl_ident.ident, "*UDF FreeAppEASpace");
	fe->ext_attribute_free_ea_space.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_free_ea_space.impl_ident.ident_suffix[1] = 1;

	for (i = 0, checksum = 0, p = (unsigned char *)&fe->ext_attribute_free_ea_space; i < 48; i++)
		checksum += *p++;
	set16(&fe->ext_attribute_free_ea_space.header_checksum, checksum);

	set32(&fe->ext_attribute_dvd_cgms_info.attribute_type, SECTOR_SIZE);
	set8(&fe->ext_attribute_dvd_cgms_info.attribute_subtype, 1);
	set32(&fe->ext_attribute_dvd_cgms_info.attribute_length, 56);
	set32(&fe->ext_attribute_dvd_cgms_info.impl_use_length, 8);
	strcpy((char *)fe->ext_attribute_dvd_cgms_info.impl_ident.ident, "*UDF DVD CGMS Info");
	fe->ext_attribute_dvd_cgms_info.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_dvd_cgms_info.impl_ident.ident_suffix[1] = 1;
	for (i = 0, checksum = 0, p = (unsigned char *)&fe->ext_attribute_dvd_cgms_info; i < 48; i++)
		checksum += *p++;
	set16(&fe->ext_attribute_dvd_cgms_info.header_checksum, checksum);

	set32(&fe->ext_attribute_macvolumeinfo.attribute_type, EXTATTR_IMP_USE);
	set8(&fe->ext_attribute_macvolumeinfo.attribute_subtype, 1);
	set32(&fe->ext_attribute_macvolumeinfo.attribute_length, 108);
	set32(&fe->ext_attribute_macvolumeinfo.impl_use_length, 60);
	strcpy((char *)fe->ext_attribute_macvolumeinfo.impl_ident.ident, "*UDF Mac VolumeInfo");
	fe->ext_attribute_macvolumeinfo.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_macvolumeinfo.impl_ident.ident_suffix[1] = 1;
/*
 * This is how tiger's hdiutil mkhybrid -udf writes this, seems to make no difference
 *	fe->ext_attribute_macvolumeinfo.impl_ident.ident_suffix[0] = 5;
 *	fe->ext_attribute_macvolumeinfo.impl_ident.ident_suffix[1] = 1;
 *	fe->ext_attribute_macvolumeinfo.impl_ident.ident_suffix[2] = 3;
 *	fe->ext_attribute_macvolumeinfo.impl_ident.ident_suffix[3] = 1;
 */
	for (i = 0, checksum = 0, p = (unsigned char *)&fe->ext_attribute_macvolumeinfo; i < 48; i++)
		checksum += *p++;
	set16(&fe->ext_attribute_macvolumeinfo.volumeinfo.headerchecksum, checksum);

	/* write mac volumeinfo */
	if (hfs_ent) {
#ifdef INSERTMACRESFORK
/*
 *	todo: don't know what to write in volfinderinfo and unknown
 *	moddate and budate are derived from folder entry's modification_time, thus no need to set here
 */
#endif
	}

	set32(&fe->ext_attribute_macfinderinfo.attribute_type, EXTATTR_IMP_USE);
	set8(&fe->ext_attribute_macfinderinfo.attribute_subtype, 1);
	set32(&fe->ext_attribute_macfinderinfo.attribute_length, 88);
	set32(&fe->ext_attribute_macfinderinfo.impl_use_length, 40);
	strcpy((char *)fe->ext_attribute_macfinderinfo.impl_ident.ident, "*UDF Mac FinderInfo");
	fe->ext_attribute_macfinderinfo.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_macfinderinfo.impl_ident.ident_suffix[1] = 1;

	for (i = 0, checksum = 0, p = (unsigned char *)&fe->ext_attribute_macfinderinfo; i < 48; i++)
		checksum += *p++;
	set16(&fe->ext_attribute_macfinderinfo.finderinfo.headerchecksum, checksum);

	/* write mac finderinfos etc. required for directories */
	if (hfs_ent) {
		fe->ext_attribute_macfinderinfo.finderinfo.dirinfo.frflags.l = ((char *)&macflags)[1];
		fe->ext_attribute_macfinderinfo.finderinfo.dirinfo.frflags.h = ((char *)&macflags)[0];
		/* todo: other dirinfo values, other dirextinfo */
#ifdef INSERTMACRESFORK
		/* todo: insert folder resource if available */
#endif
	}

	allocation_desc = &fe->allocation_desc;
	/*
	 * Only a file size less than 1GB can be expressed by a single
	 * AllocationDescriptor. When the size of a file is larger than 1GB,
	 * 2 or more AllocationDescriptors should be used. We don't know
	 * whether a single 8-byte AllocationDescriptor should be written or no
	 * one should be written if the size of a file is 0 byte. - FIXME.
	 *
	 * XXX We get called with buf[2048]. This allows a max. file size of
	 * XXX 234 GB. With more we would cause a buffer overflow.
	 * XXX We need to check whether UDF would allow files > 234 GB.
	 */

	for (; length > 0; length -= chunk) {
		chunk = (length > 0x3ffff800) ? 0x3ffff800 : length;
		set32(&allocation_desc->extent_length, chunk);
		set32(&allocation_desc->extent_position, file_rba);
		file_rba += chunk >> 11;
		allocation_desc++;
	}
	if (((Uchar *)allocation_desc) > &buf[2048])
		udf_size_panic(allocation_desc - &fe->allocation_desc);

	set32(&fe->length_of_allocation_descs,
				(unsigned char *) allocation_desc -
				(unsigned char *) &fe->allocation_desc);
	set_tag(&fe->desc_tag, UDF_TAGID_FILE_ENTRY, rba,
		(unsigned char *) allocation_desc - buf);
}

LOCAL void
#ifdef	PROTOTYPES
set_attr_file_entry(unsigned char *buf,
	unsigned rba,
	unsigned file_rba,
	off_t length,
	const char *iso_date,
	int is_directory,
	unsigned link_count,
	unsigned unique_id,
	hfsdirent *hfs_ent,
	mode_t	fmode,
	uid_t	fuid,
	gid_t	fgid)
#else
set_attr_file_entry(buf, rba, file_rba, length, iso_date, is_directory, link_count, unique_id, hfs_ent, fmode, fuid, fgid)
	unsigned char *buf;
	unsigned rba;
	unsigned file_rba;
	off_t length;
	const char *iso_date;
	int is_directory;
	unsigned link_count;
	unsigned unique_id;
	hfsdirent *hfs_ent;
	mode_t	fmode;
	uid_t	fuid;
	gid_t	fgid;
#endif
{
	udf_short_ad	*allocation_desc;
	unsigned	chunk;
	unsigned short	flags;
	short	macflags;

	udf_attr_file_entry *fe = (udf_attr_file_entry *)buf;

	/*set32(&fe->icb_tag.prior_recorded_number_of_direct_entries, 0);*/
	set16(&fe->icb_tag.strategy_type, 4);
	/*set16(&fe->icb_tag.strategy_parameter, 0);*/
	set16(&fe->icb_tag.maximum_number_of_entries, 1);
	set8(&fe->icb_tag.file_type, UDF_ICBTAG_FILETYPE_EA);
	/*fe->icb_tag.parent_icb_location;*/
	/* UDF_ICBTAG_FLAG_SYSTEM shall be set for MS-DOS, OS/2, Win95 and WinNT as of UDF260 3.3.2.1 */
	flags = UDF_ICBTAG_FLAG_NONRELOCATABLE | UDF_ICBTAG_FLAG_ARCHIVE | UDF_ICBTAG_FLAG_CONTIGUOUS;
	if (hfs_ent) {
		macflags = hfs_ent->fdflags;
		B2N_16(macflags);
		if (macflags & FI_ATTR_SYSTEM) {
			flags |= UDF_ICBTAG_FLAG_SYSTEM;
		}
	}
	set16(&fe->icb_tag.flags, flags);

	set32(&fe->permissions, getperms(fmode));
	if (rationalize_uid)
		set32(&fe->uid, uid_to_use);
	else
		set32(&fe->uid, fuid);
	if (rationalize_gid)
		set32(&fe->gid, gid_to_use);
	else
		set32(&fe->gid, fgid);


	set16(&fe->file_link_count, link_count);
	/*fe->record_format;*/
	/*fe->record_display_attributes;*/
	/*fe->record_length;*/
	if (length % 4)
		length += 4 - (length % 4);
	if (length % 2048)
		length += 2048 - (length % 2048);
	set64(&fe->info_length, length);
	set64(&fe->logical_blocks_recorded, ISO_BLOCKS(length));
	if (iso_date) {
		set_timestamp_from_iso_date(&fe->access_time, iso_date);
		fe->modification_time = fe->access_time;
		fe->attribute_time = fe->access_time;
	}
	set32(&fe->checkpoint, 1);
	/*fe->ext_attribute_icb;*/
	set_impl_ident(&fe->impl_ident);
	set64(&fe->unique_id, unique_id);


	allocation_desc = &fe->allocation_desc;
	/*
	 * Only a file size less than 1GB can be expressed by a single
	 * AllocationDescriptor. When the size of a file is larger than 1GB,
	 * 2 or more AllocationDescriptors should be used. We don't know
	 * whether a single 8-byte AllocationDescriptor should be written or no
	 * one should be written if the size of a file is 0 byte. - FIXME.
	 *
	 * XXX We get called with buf[2048]. This allows a max. file size of
	 * XXX 234 GB. With more we would cause a buffer overflow.
	 * XXX We need to check whether UDF would allow files > 234 GB.
	 */

	for (; length > 0; length -= chunk) {
		chunk = (length > 0x3ffff800) ? 0x3ffff800 : length;
		set32(&allocation_desc->extent_length, chunk);
		set32(&allocation_desc->extent_position, file_rba);
		file_rba += chunk >> 11;
		allocation_desc++;
	}
	if (((Uchar *)allocation_desc) > &buf[2048])
		udf_size_panic(allocation_desc - &fe->allocation_desc);

	set32(&fe->length_of_allocation_descs,
				(unsigned char *) allocation_desc -
				(unsigned char *) &fe->allocation_desc);
	set_tag(&fe->desc_tag, UDF_TAGID_FILE_ENTRY, rba,
		(unsigned char *) allocation_desc - buf);
}

LOCAL void
#ifdef	PROTOTYPES
set_filed_entry(unsigned char *buf,
	unsigned rba,
	unsigned file_rba,
	unsigned length,
	const char *iso_date,
	int is_directory,
	unsigned link_count,
	unsigned unique_id,
	hfsdirent *hfs_ent,
	mode_t	fmode,
	uid_t	fuid,
	gid_t	fgid)
#else
set_filed_entry(buf, rba, file_rba, length, iso_date, is_directory, link_count, unique_id, hfs_ent, fmode, fuid, fgid)
	unsigned char *buf;
	unsigned rba;
	unsigned file_rba;
	unsigned length;
	const char *iso_date;
	int is_directory;
	unsigned link_count;
	unsigned unique_id;
	hfsdirent *hfs_ent;
	mode_t	fmode;
	uid_t	fuid;
	gid_t	fgid;
#endif
{
	udf_short_ad	*allocation_desc;
	unsigned	chunk;
	unsigned short	checksum;
	int		i;
	unsigned char *p;
	unsigned short	flags;
	short	macflags;

	udf_filed_entry *fe = (udf_filed_entry *)buf;

	/*set32(&fe->icb_tag.prior_recorded_number_of_direct_entries, 0);*/
	set16(&fe->icb_tag.strategy_type, 4);
	/*set16(&fe->icb_tag.strategy_parameter, 0);*/
	set16(&fe->icb_tag.maximum_number_of_entries, 1);
	set8(&fe->icb_tag.file_type, UDF_ICBTAG_FILETYPE_DIRECTORY);
	/*fe->icb_tag.parent_icb_location;*/
	/* UDF_ICBTAG_FLAG_SYSTEM shall be set for MS-DOS, OS/2, Win95 and WinNT as of UDF260 3.3.2.1 */
	flags = UDF_ICBTAG_FLAG_NONRELOCATABLE | UDF_ICBTAG_FLAG_ARCHIVE | UDF_ICBTAG_FLAG_CONTIGUOUS;
	if (hfs_ent) {
		macflags = hfs_ent->fdflags;
		B2N_16(macflags);
		if (macflags & FI_ATTR_SYSTEM) {
			flags |= UDF_ICBTAG_FLAG_SYSTEM;
		}
	}
	set16(&fe->icb_tag.flags, flags);

	set32(&fe->permissions, getperms(fmode));
	if (rationalize_uid)
		set32(&fe->uid, uid_to_use);
	else
		set32(&fe->uid, fuid);
	if (rationalize_gid)
		set32(&fe->gid, gid_to_use);
	else
		set32(&fe->gid, fgid);


	set16(&fe->file_link_count, link_count);
	/*fe->record_format;*/
	/*fe->record_display_attributes;*/
	/*fe->record_length;*/
	set64(&fe->info_length, length);
	set64(&fe->logical_blocks_recorded, ISO_BLOCKS(length));
	if (iso_date) {
		set_timestamp_from_iso_date(&fe->access_time, iso_date);
		fe->modification_time = fe->access_time;
		fe->attribute_time = fe->access_time;
	}
	set32(&fe->checkpoint, 1);
	/*fe->ext_attribute_icb;*/
	set_impl_ident(&fe->impl_ident);
	set64(&fe->unique_id, unique_id);

	/* write mac finderinfos etc. required for directories */
	set32(&fe->length_of_ext_attributes, sizeof (udf_ext_attribute_header_desc) +
		sizeof (udf_ext_attribute_free_ea_space) + sizeof (udf_ext_attribute_dvd_cgms_info) +
		sizeof (udf_ext_attribute_dir_macfinderinfo));

	set32(&fe->ext_attribute_header.impl_attributes_location, sizeof (udf_ext_attribute_header_desc));
	set32(&fe->ext_attribute_header.application_attributes_location,
		sizeof (udf_ext_attribute_header_desc) + sizeof (udf_ext_attribute_free_ea_space) +
		sizeof (udf_ext_attribute_dvd_cgms_info) + sizeof (udf_ext_attribute_dir_macfinderinfo));
	set_tag(&fe->ext_attribute_header.desc_tag, UDF_TAGID_EXT_ATTRIBUTE_HEADER_DESC, rba,
		sizeof (udf_ext_attribute_header_desc));

	set32(&fe->ext_attribute_free_ea_space.attribute_type, SECTOR_SIZE);
	set8(&fe->ext_attribute_free_ea_space.attribute_subtype, 1);
	set32(&fe->ext_attribute_free_ea_space.attribute_length, 52);
	set32(&fe->ext_attribute_free_ea_space.impl_use_length, 4);
	strcpy((char *)fe->ext_attribute_free_ea_space.impl_ident.ident, "*UDF FreeAppEASpace");
	fe->ext_attribute_free_ea_space.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_free_ea_space.impl_ident.ident_suffix[1] = 1;

	for (i = 0, checksum = 0, p = (unsigned char *)&fe->ext_attribute_free_ea_space; i < 48; i++)
		checksum += *p++;
	set16(&fe->ext_attribute_free_ea_space.header_checksum, checksum);

	set32(&fe->ext_attribute_dvd_cgms_info.attribute_type, SECTOR_SIZE);
	set8(&fe->ext_attribute_dvd_cgms_info.attribute_subtype, 1);
	set32(&fe->ext_attribute_dvd_cgms_info.attribute_length, 56);
	set32(&fe->ext_attribute_dvd_cgms_info.impl_use_length, 8);
	strcpy((char *)fe->ext_attribute_dvd_cgms_info.impl_ident.ident, "*UDF DVD CGMS Info");
	fe->ext_attribute_dvd_cgms_info.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_dvd_cgms_info.impl_ident.ident_suffix[1] = 1;

	for (i = 0, checksum = 0, p = (unsigned char *)&fe->ext_attribute_dvd_cgms_info; i < 48; i++)
		checksum += *p++;
	set16(&fe->ext_attribute_dvd_cgms_info.header_checksum, checksum);

	set32(&fe->ext_attribute_macfinderinfo.attribute_type, EXTATTR_IMP_USE);
	set8(&fe->ext_attribute_macfinderinfo.attribute_subtype, 1);
	set32(&fe->ext_attribute_macfinderinfo.attribute_length, 88);
	set32(&fe->ext_attribute_macfinderinfo.impl_use_length, 40);
	strcpy((char *)fe->ext_attribute_macfinderinfo.impl_ident.ident, "*UDF Mac FinderInfo");
	fe->ext_attribute_macfinderinfo.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_macfinderinfo.impl_ident.ident_suffix[1] = 1;

	for (i = 0, checksum = 0, p = (unsigned char *)&fe->ext_attribute_macfinderinfo; i < 48; i++)
		checksum += *p++;
	set16(&fe->ext_attribute_macfinderinfo.finderinfo.headerchecksum, checksum);

	/* write mac finderinfos etc. required for directories */
	if (hfs_ent) {
		fe->ext_attribute_macfinderinfo.finderinfo.dirinfo.frflags.l = ((char *)&macflags)[1];
		fe->ext_attribute_macfinderinfo.finderinfo.dirinfo.frflags.h = ((char *)&macflags)[0];
		/* todo: other dirinfo values, other dirextinfo */
#ifdef INSERTMACRESFORK
		/* todo: insert folder resource if available */
#endif
	}

	/*
	 * Extended attributes that may (?) be required for DVD-Video
	 * compliance
	 */
#if 0
	set32(&fe->length_of_ext_attributes, 24+52+56);
	set32(&fe->ext_attribute_header.impl_attributes_location, 24);
	set32(&fe->ext_attribute_header.application_attributes_location,
								24+52+56);
	set_tag(&fe->ext_attribute_header.desc_tag,
			UDF_TAGID_EXT_ATTRIBUTE_HEADER_DESC, rba, 24 /*???*/);
	set32(&fe->ext_attribute_free_ea_space.attribute_type, SECTOR_SIZE);
	set8(&fe->ext_attribute_free_ea_space.attribute_subtype, 1);
	set32(&fe->ext_attribute_free_ea_space.attribute_length, 52);
	set32(&fe->ext_attribute_free_ea_space.impl_use_length, 4);
	strcpy((char *)fe->ext_attribute_free_ea_space.impl_ident.ident,
							"*UDF FreeAppEASpace");
	set32(&fe->ext_attribute_dvd_cgms_info.attribute_type, SECTOR_SIZE);
	set8(&fe->ext_attribute_dvd_cgms_info.attribute_subtype, 1);
	set32(&fe->ext_attribute_dvd_cgms_info.attribute_length, 56);
	set32(&fe->ext_attribute_dvd_cgms_info.impl_use_length, 8);
	strcpy((char *)fe->ext_attribute_free_ea_space.impl_ident.ident,
							"*UDF DVD CGMS Info");
	fe->ext_attribute_free_ea_space.impl_ident.ident_suffix[0] = 2;
	fe->ext_attribute_free_ea_space.impl_ident.ident_suffix[1] = 1;
#else
	/*set32(&fe->length_of_ext_attributes, 0);*/
#endif

	allocation_desc = &fe->allocation_desc;
	/*
	 * Only a file size less than 1GB can be expressed by a single
	 * AllocationDescriptor. When the size of a file is larger than 1GB,
	 * 2 or more AllocationDescriptors should be used. We don't know
	 * whether a single 8-byte AllocationDescriptor should be written or no
	 * one should be written if the size of a file is 0 byte. - FIXME.
	 *
	 * XXX We get called with buf[2048]. This allows a max. file size of
	 * XXX 234 GB. With more we would cause a buffer overflow.
	 * XXX We need to check whether UDF would allow files > 234 GB.
	 */
	for (; length > 0; length -= chunk) {
		chunk = (length > 0x3ffff800) ? 0x3ffff800 : length;
		set32(&allocation_desc->extent_length, chunk);
		set32(&allocation_desc->extent_position, file_rba);
		file_rba += chunk >> 11;
		allocation_desc++;
	}
	if (((Uchar *)allocation_desc) > &buf[2048])
		udf_size_panic(allocation_desc - &fe->allocation_desc);

	set32(&fe->length_of_allocation_descs,
				(unsigned char *) allocation_desc -
				(unsigned char *) &fe->allocation_desc);
	set_tag(&fe->desc_tag, UDF_TAGID_FILE_ENTRY, rba,
		(unsigned char *) allocation_desc - buf);
}

LOCAL unsigned
directory_link_count(dpnt)
	struct directory	*dpnt;
{
	/*
	 * The link count is equal to 1 (for the parent) plus the
	 * number of subdirectories.
	 */
	unsigned link_count = 1;
	struct directory_entry *de;

	/* count relocated subdirectories */
	for (de = dpnt->jcontents; de; de = de->jnext) {
		if ((de->de_flags &
		    (INHIBIT_UDF_ENTRY | RELOCATED_DIRECTORY)) ==
							RELOCATED_DIRECTORY) {
			link_count++;
		}
	}
	/* count ordinary subdirectories */
	for (dpnt = dpnt->subdir; dpnt; dpnt = dpnt->next) {
		if (!(dpnt->dir_flags & INHIBIT_UDF_ENTRY)) {
			link_count++;
		}
	}
	return (link_count);
}

LOCAL void
write_one_udf_directory(dpnt, outfile)
	struct directory	*dpnt;
	FILE			*outfile;
{
	unsigned size_in_bytes, padded_size_in_bytes;
	struct directory_entry *de;
	unsigned ident_size;
	UInt32_t base_sector;
	struct directory *parent;
	Uchar buf[SECTOR_SIZE];

	memset(buf, 0, SECTOR_SIZE);
#ifdef INSERTMACRESFORK
	if (dpnt == root) {
		set_macvolume_filed_entry(
			buf,
			last_extent_written - lba_udf_partition_start,
			last_extent_written+1 - lba_udf_partition_start,
			directory_size(dpnt),
			dpnt->self->isorec.date,
			1,	/* is_directory */
			directory_link_count(dpnt),
			(dpnt == root) ? 0 : dpnt->self->udf_file_entry_sector,
#ifdef APPLE_HYB
			dpnt->hfs_ent,
#else
			NULL,
#endif
			dpnt->self->mode,
			dpnt->self->uid,
			dpnt->self->gid);
	} else {
#endif
		set_filed_entry(
			buf,
			last_extent_written - lba_udf_partition_start,
			last_extent_written+1 - lba_udf_partition_start,
			directory_size(dpnt),
			dpnt->self->isorec.date,
			1,	/* is_directory */
			directory_link_count(dpnt),
			(dpnt == root) ? 0 : dpnt->self->udf_file_entry_sector,
#ifdef APPLE_HYB
			dpnt->hfs_ent,
#else
			NULL,
#endif
			dpnt->self->mode,
			dpnt->self->uid,
			dpnt->self->gid);
#ifdef INSERTMACRESFORK
	}
#endif
	xfwrite(buf, SECTOR_SIZE, 1, outfile, 0, FALSE);
	last_extent_written++;

	base_sector = last_extent_written - lba_udf_partition_start;

	/* parent directory */
	parent = dpnt->parent;
	if (parent == reloc_dir) {
		parent = dpnt->self->parent_rec->filedir;
	}
	ident_size = set_file_ident_desc(
		buf,
		base_sector,
		0,
		1,
		parent->self->udf_file_entry_sector - lba_udf_partition_start,
		(parent == root) ? 0 : parent->self->udf_file_entry_sector);
	xfwrite(buf, ident_size, 1, outfile, 0, FALSE);
	size_in_bytes = ident_size;

	/* directory contents */
	for (de = dpnt->jcontents; de; de = de->jnext) {
		char *name;
		struct directory_entry *de1;

		if (de->de_flags & INHIBIT_UDF_ENTRY)
			continue;

		name = USE_MAC_NAME(de) ? de->hfs_ent->name : de->name;

		/* skip . and .. */
		if (name[0] == '.' && (name[1] == 0 ||
		    (name[1] == '.' && name[2] == 0)))
			continue;

		/* look in RR_MOVED for relocated directories */
		de1 = de;
		if (de->de_flags & RELOCATED_DIRECTORY) {
			for (de1 = reloc_dir->contents; de1; de1 = de1->next) {
				if (de1->parent_rec == de) {
					break;
				}
			}
			if (!de1) {
				comerrno(EX_BAD,
				"Unable to locate relocated directory\n");
			}
		}

		ident_size = set_file_ident_desc(
			buf,
			base_sector + (size_in_bytes / SECTOR_SIZE),
			name,
			!!(de1->isorec.flags[0] & ISO_DIRECTORY),
			de1->udf_file_entry_sector - lba_udf_partition_start,
			de1->udf_file_entry_sector);
		xfwrite(buf, ident_size, 1, outfile, 0, FALSE);
		size_in_bytes += ident_size;
	}

	padded_size_in_bytes = PAD(size_in_bytes, SECTOR_SIZE);
	if (size_in_bytes < padded_size_in_bytes) {
		memset(buf, 0, padded_size_in_bytes - size_in_bytes);
		xfwrite(buf, padded_size_in_bytes - size_in_bytes, 1, outfile, 0, FALSE);
	}

	last_extent_written += padded_size_in_bytes / SECTOR_SIZE;
}

LOCAL void
write_udf_directories(dpnt, outfile)
	struct directory	*dpnt;
	FILE			*outfile;
{
	if (!(dpnt->dir_flags & INHIBIT_UDF_ENTRY)) {
		write_one_udf_directory(dpnt, outfile);
	}
	if (!(dpnt->dir_flags & INHIBIT_UDF_ENTRY) || dpnt == reloc_dir) {
		for (dpnt = dpnt->subdir; dpnt; dpnt = dpnt->next) {
			write_udf_directories(dpnt, outfile);
		}
	}
}

LOCAL void
write_udf_file_entries(dpnt, outfile)
	struct directory	*dpnt;
	FILE			*outfile;
{
	Uchar buf[SECTOR_SIZE];
	unsigned long	logical_block = 0;
	off_t		attr_size = 0;

	memset(buf, 0, SECTOR_SIZE);

	if (!(dpnt->dir_flags & INHIBIT_UDF_ENTRY)) {
		struct directory_entry *de;
		for (de = dpnt->jcontents; de; de = de->jnext) {
			if (!(de->de_flags & RELOCATED_DIRECTORY) &&
			    !(de->isorec.flags[0] & ISO_DIRECTORY)) {
#ifdef INSERTMACRESFORK
				if (de->assoc) {
					logical_block = last_extent_written - lba_udf_partition_start + 1;
				} else {
					logical_block = 0;
				}
#endif
				memset(buf, 0, SECTOR_SIZE);
				set_file_entry(
					buf,
					(last_extent_written++) - lba_udf_partition_start,
					get_733(de->isorec.extent) - lba_udf_partition_start,
					de->size,
					de->isorec.date,
					0,	/* is_directory */
					1,	/* link_count */
					de->udf_file_entry_sector,
#ifdef APPLE_HYB
					de->hfs_ent,
#else
					NULL,
#endif
#ifdef INSERTMACRESFORK
					logical_block,
#else
					0,
#endif
					de->mode,
					de->uid,
					de->gid);
				xfwrite(buf, SECTOR_SIZE, 1, outfile, 0, FALSE);

#ifdef INSERTMACRESFORK
				if (de->assoc) {

					if (ISO_ROUND_UP(de->assoc->size) <
					    ISO_ROUND_UP(de->assoc->size + sizeof (udf_ext_attribute_common))) {
						attr_size = sizeof (udf_ext_attribute_common);
					}

					memset(buf, 0, SECTOR_SIZE);
					set_attr_file_entry(
						buf,
						(last_extent_written++) - lba_udf_partition_start,
						get_733(de->assoc->isorec.extent) - lba_udf_partition_start,
						de->assoc->size + SECTOR_SIZE + attr_size,
						de->isorec.date,
						0,
						0,
						de->udf_file_entry_sector,
#ifdef APPLE_HYB
						de->hfs_ent,
#else
						NULL,
#endif
						de->mode,
						de->uid,
						de->gid);
					xfwrite(buf, SECTOR_SIZE, 1, outfile, 0, FALSE);
				}
#endif
			}
		}
	}
	if (!(dpnt->dir_flags & INHIBIT_UDF_ENTRY) || dpnt == reloc_dir) {
		for (dpnt = dpnt->subdir; dpnt; dpnt = dpnt->next) {
			write_udf_file_entries(dpnt, outfile);
		}
	}
}

/****************************/

LOCAL int
udf_vol_recognition_area_write(out)
	FILE	*out;
{
static	const char *identifiers[3] = { "BEA01", "NSR02", "TEA01" };
	int i;
	char buf[SECTOR_SIZE];
	udf_volume_recognition_desc *vsd = (udf_volume_recognition_desc *)buf;

	memset(buf, 0, sizeof (buf));
	/*set8(&vsd->structure_type, 0);*/
	set8(&vsd->structure_version, 1);
	for (i = 0; i < 3; ++i) {
		memcpy(vsd->standard_identifier, identifiers[i], 5);
		xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);
	}
	last_extent_written += 3;
	return (0);
}

LOCAL int
udf_main_seq_write(out)
	FILE	*out;
{
	Uchar buf[SECTOR_SIZE];
	int i;

	/*
	 * volume_set_id needs to be set to a (64-bit) "unique" number.
	 * This will have to do for now.
	 */
	volume_set_id[0] = begun;
	volume_set_id[1] = (unsigned)clock();	/* XXX Maybe non-portable */

	memset(buf, 0, sizeof (buf));
	set_primary_vol_desc(buf, last_extent_written++);
	xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);

	memset(buf, 0, sizeof (buf));
	set_impl_use_vol_desc(buf, last_extent_written++);
	xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);

	memset(buf, 0, sizeof (buf));
	set_partition_desc(buf, last_extent_written++);
	xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);

	memset(buf, 0, sizeof (buf));
	set_logical_vol_desc(buf, last_extent_written++);
	xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);

	memset(buf, 0, sizeof (buf));
	set_unallocated_space_desc(buf, last_extent_written++);
	xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);

	memset(buf, 0, sizeof (buf));
	set_terminating_desc(buf, last_extent_written++);
	xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);

	memset(buf, 0, sizeof (buf));
	for (i = 6; i < UDF_MAIN_SEQ_LENGTH; ++i) {
		xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);
		last_extent_written++;
	}

	return (0);
}

LOCAL int
udf_integ_seq_write(out)
	FILE	*out;
{
	Uchar buf[SECTOR_SIZE*UDF_INTEG_SEQ_LENGTH];

	memset(buf, 0, sizeof (buf));

	set_logical_vol_integrity_desc(buf+0*SECTOR_SIZE,
						last_extent_written++);
	set_terminating_desc(buf+1*SECTOR_SIZE, last_extent_written++);

	xfwrite(buf, SECTOR_SIZE, UDF_INTEG_SEQ_LENGTH, out, 0, FALSE);
	return (0);
}

LOCAL int
udf_anchor_vol_desc_write(out)
	FILE	*out;
{
	Uchar buf[SECTOR_SIZE];

	memset(buf, 0, sizeof (buf));
	set_anchor_volume_desc_pointer(buf, last_extent_written++);
	xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);
	return (0);
}

LOCAL int
udf_file_set_desc_write(out)
	FILE	*out;
{
	Uchar buf[SECTOR_SIZE*2];

	memset(buf, 0, sizeof (buf));

	set_file_set_desc(buf+0*SECTOR_SIZE,
			(last_extent_written++) - lba_udf_partition_start);
	set_terminating_desc(buf+1*SECTOR_SIZE,
			(last_extent_written++) - lba_udf_partition_start);

	xfwrite(buf, SECTOR_SIZE, 2, out, 0, FALSE);

	return (0);
}

LOCAL int
udf_dirtree_write(out)
	FILE	*out;
{
	write_udf_directories(root, out);
	return (0);
}

LOCAL int
udf_file_entries_write(out)
	FILE	*out;
{
	write_udf_file_entries(root, out);
	return (0);
}

LOCAL int
pad_to(last_extent_to_write, out)
	UInt32_t	last_extent_to_write;
	FILE		*out;
{
	char buf[SECTOR_SIZE];
	memset(buf, 0, sizeof (buf));
	while (last_extent_written < last_extent_to_write) {
		xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);
		++last_extent_written;
	}
	return (0);
}

LOCAL int
udf_pad_to_sector_32_write(out)
	FILE	*out;
{
	return (pad_to(session_start+32, out));
}

LOCAL int
udf_pad_to_sector_256_write(out)
	FILE	*out;
{
	return (pad_to(session_start+256, out));
}

LOCAL int
udf_padend_avdp_write(out)
	FILE	*out;
{
	Uchar	buf[SECTOR_SIZE];
	UInt32_t last_extent_to_write = (last_extent_written+31) & ~15;

	if (!use_sparcboot)
		last_extent_to_write = last_extent_written + 150;

	memset(buf, 0, sizeof (buf));
	while (last_extent_written < last_extent_to_write) {
		set_anchor_volume_desc_pointer(buf, last_extent_written++);
		xfwrite(buf, SECTOR_SIZE, 1, out, 0, FALSE);
	}
	return (0);
}

EXPORT void
udf_set_extattr_freespace(buf, size, rba)
	unsigned char *buf;
	off_t size;
	unsigned rba;
{
	unsigned short	checksum;
	int	i;
	unsigned char *p;
	unsigned long	ls = (unsigned long)size;
	udf_ext_attribute_free_ea_space *ea;
	udf_ext_attribute_header_desc *eahdc = (udf_ext_attribute_header_desc*)buf;


	if (ISO_ROUND_UP(ls) < ISO_ROUND_UP(ls + sizeof (udf_ext_attribute_common))) {
		ls += sizeof (udf_ext_attribute_common);
	}

	set32(&eahdc->impl_attributes_location, sizeof (udf_ext_attribute_header_desc));
	if (ls % 4)
		ls += 4 - (ls % 4);
	if (ls % SECTOR_SIZE)
		ls += SECTOR_SIZE - (ls % SECTOR_SIZE);

	set32(&eahdc->application_attributes_location, ls + SECTOR_SIZE);
	set_tag(&eahdc->desc_tag, UDF_TAGID_EXT_ATTRIBUTE_HEADER_DESC, last_extent_written - lba_udf_partition_start,
		sizeof (udf_ext_attribute_header_desc));

	p = (unsigned char *)eahdc;
	p += sizeof (udf_ext_attribute_header_desc);
	ea = (udf_ext_attribute_free_ea_space *)p;


	set32(&ea->attribute_type, EXTATTR_IMP_USE);
	set8(&ea->attribute_subtype, 1);
	set32(&ea->attribute_length, 2024);	/* SECTOR_SIZE - sizeof (udf_ext_attribute_header_desc) */
	set32(&ea->impl_use_length, 1976);	/* attribute_length - 48 */
	strcpy((char *)ea->impl_ident.ident, "*UDF FreeEASpace");
	ea->impl_ident.ident_suffix[0] = 2;
	ea->impl_ident.ident_suffix[1] = 1;

	for (i = 0, checksum = 0, p = (unsigned char *)ea; i < 48; i++)
		checksum += *p++;
	set16(&ea->header_checksum, checksum);
}

EXPORT void
udf_set_extattr_macresfork(buf, size, rba)
	unsigned char *buf;
	off_t size;
	unsigned rba;
{
	unsigned short	checksum;
	int	i;
	unsigned char *p;
	unsigned long	ls = (unsigned long)size;
	udf_ext_attribute_common *ea = (udf_ext_attribute_common *)buf;


	if (ISO_ROUND_UP(ls) < ISO_ROUND_UP(ls + sizeof (udf_ext_attribute_common))) {
		ls += sizeof (udf_ext_attribute_common);
	}

	set32(&ea->attribute_type, EXTATTR_IMP_USE);
	set8(&ea->attribute_subtype, 1);
	if (ls % 4)
		ls += 4 - (ls % 4);
	set32(&ea->impl_use_length, ls);
	if (ls % SECTOR_SIZE)
		ls += SECTOR_SIZE - (ls % SECTOR_SIZE);
	set32(&ea->attribute_length, ls);
	strcpy((char *)ea->impl_ident.ident, "*UDF Mac ResourceFork");
	ea->impl_ident.ident_suffix[0] = 2;
	ea->impl_ident.ident_suffix[1] = 1;

	for (i = 0, checksum = 0, p = (unsigned char *)ea; i < 48; i++)
		checksum += *p++;
	set16(&ea->header_checksum, checksum);
}

/* BEGIN CSTYLED */
struct output_fragment udf_vol_recognition_area_frag = { NULL, udf_vol_recognition_area_size, NULL, udf_vol_recognition_area_write, "UDF volume recognition area" };
struct output_fragment udf_main_seq_frag = { NULL, udf_main_seq_size, NULL, udf_main_seq_write, "UDF main seq" };
struct output_fragment udf_main_seq_copy_frag = { NULL, udf_main_seq_copy_size, NULL, udf_main_seq_write, "UDF second seq" };
struct output_fragment udf_integ_seq_frag = { NULL, udf_integ_seq_size, NULL, udf_integ_seq_write, "UDF integ seq" };
struct output_fragment udf_anchor_vol_desc_frag = { NULL, oneblock_size, NULL, udf_anchor_vol_desc_write, "UDF Anchor volume" };
struct output_fragment udf_file_set_desc_frag = { NULL, udf_file_set_desc_size, NULL, udf_file_set_desc_write, "UDF file set" };
struct output_fragment udf_dirtree_frag = { NULL, udf_dirtree_size, NULL, udf_dirtree_write, "UDF directory tree" };
struct output_fragment udf_file_entries_frag = { NULL, udf_file_entries_size, NULL, udf_file_entries_write, "UDF file entries" };
struct output_fragment udf_end_anchor_vol_desc_frag = { NULL, udf_end_anchor_vol_desc_size, NULL, udf_anchor_vol_desc_write, "UDF Anchor end volume" };

struct output_fragment udf_pad_to_sector_32_frag = { NULL, udf_pad_to_sector_32_size, NULL, udf_pad_to_sector_32_write, "UDF pad to sector 32" };
struct output_fragment udf_pad_to_sector_256_frag = { NULL, udf_pad_to_sector_256_size, NULL, udf_pad_to_sector_256_write, "UDF pad to sector 256" };
struct output_fragment udf_padend_avdp_frag = { NULL, udf_padend_avdp_size, NULL, udf_padend_avdp_write, "UDF Pad end" };
/* END CSTYLED */

/*
 * This function assigns weights as follows:
 *
 * /VIDEO_TS/VIDEO_TS.IFO   11199
 * /VIDEO_TS/VIDEO_TS.VOB   11198
 * /VIDEO_TS/VIDEO_TS.BUP   11188
 * /VIDEO_TS/VTS_01_0.IFO   11187
 * /VIDEO_TS/VTS_01_0.VOB   11186
 *            :               :
 * /VIDEO_TS/VTS_01_9.VOB   11177
 * /VIDEO_TS/VTS_01_0.BUP   11176
 *            :               :
 * /VIDEO_TS/VTS_99_0.BUP   10000
 *
 * This ensures that DVD-Video files are laid out properly on the disc.
 * The same thing is done for AUDIO_TS files, except in the 20000 range
 * instead of the 10000 range.
 *
 * Question: what about JACKET_P files?
 *
 * Answer: At least as far as I know :)
 * JACKET_P files are still images (single frame mpeg video .i.e mp2
 * format). The DVD Jacket pictures will be displayed on the TV screen
 * when the player is in a stop/resume mode.
 * The location is not dependent on IFO information and the only must
 * as far as I know is that they are in upper case (both dir and files).
 * This sparce information makes me conclude that they don't need any
 * weight. This obviously needs to be tested.
 */
EXPORT int
assign_dvd_weights(name, this_dir, val)
	char			*name;
	struct directory	*this_dir;
	int			val;
{
	int ts_number;
	int segment;
	int audio;

	if (name[0] != 'A' && name[0] != 'V')
		return (val);

	if (memcmp(name, "VIDEO_TS", 8) == 0) {
		ts_number = 0;
		audio = 0;
	} else if (memcmp(name, "VTS_", 4) == 0) {
		ts_number = 1;
		audio = 0;
	} else if (memcmp(name, "AUDIO_TS", 8) == 0) {
		ts_number = 0;
		audio = 1;
	} else if (memcmp(name, "ATS_", 4) == 0) {
		ts_number = 1;
		audio = 1;
	} else {
		return (val);
	}

	if (this_dir->parent != root ||
	    strcmp(this_dir->de_name, "VIDEO_TS") != 0)
		return (val);

	if (ts_number == 0) {
		segment = 0;
	} else {
		if (name[4] >= '0' && name[4] <= '9' &&
		    name[5] >= '0' && name[5] <= '9' &&
		    name[6] == '_' &&
		    name[7] >= '0' && name[7] <= '9') {
			ts_number = name[4] * 10 + name[5] - ('0' * 11);
			segment = name[7] - '0';
		} else {
			return (val);
		}
	}

	if (strcmp(name+8, audio ? ".AOB" : ".VOB") == 0) {
		return (audio * 10000 - ts_number * 12 - segment + 11198);
	} else if (strcmp(name+8, ".IFO") == 0) {
		return (audio * 10000 - ts_number * 12 + 11199);
	} else if (strcmp(name+8, ".BUP") == 0) {
		return (audio * 10000 - ts_number * 12 + 11188);
	} else {
		return (val);
	}
}

#ifndef	ENAMETOOLONG
#define	ENAMETOOLONG	EINVAL
#endif

EXPORT int
udf_get_symlinkcontents(filename, contents, size)
	char	*filename;
	char	*contents;
	off_t	*size;
{
#ifdef	HAVE_READLINK
	int	nchar = -1;
	char	tgt[8192];
	char	*target;
	char	*cp;

	memset(contents, 0, *size);
	memset(tgt, 0, sizeof (tgt));
	nchar = readlink(filename, tgt, sizeof (tgt) -1);
	if (nchar < 0) {
		*size = 0;
		return (-1);
	}
	if (*size < 4) {
		seterrno(ENAMETOOLONG);
		*size = 0;
		return (-1);
	}
	target = tgt;
	cp = contents;
	if (*target == '/') {
		*cp++ = 2;	/* Type "root"	*/
		*cp++ = 0;	/* len 0	*/
		*cp++ = 0;
		*cp++ = 0;
		while (*target == '/')
			target++;
	}
	while (*target) {
		char	*sp = target;

		while (*sp != '\0' && *sp != '/')
			sp++;
		if ((sp - target) == 2 &&
		    target[0] == '.' && target[1] == '.') {
			if (((contents + *size) - cp) < 4) {
				seterrno(ENAMETOOLONG);
				*size = 0;
				return (-1);
			}
			*cp++ = 3;	/* Type ".."	*/
			*cp++ = 0;	/* len 0	*/
			*cp++ = 0;
			*cp++ = 0;
		} else if ((sp - target) == 1 && *target == '.') {
			if (((contents + *size) - cp) < 4) {
				seterrno(ENAMETOOLONG);
				*size = 0;
				return (-1);
			}
			*cp++ = 4;	/* Type "."	*/
			*cp++ = 0;	/* len 0	*/
			*cp++ = 0;
			*cp++ = 0;
		} else {
			int	len;

			if (((contents + *size) - cp) < 6) {
				seterrno(ENAMETOOLONG);
				*size = 0;
				return (-1);
			}
			*cp++ = 5;	/* Type "path"	*/
			*cp++ = 0;	/* len 0	*/
			*cp++ = 0;
			*cp++ = 0;
			*sp = '\0';
			len = set_ostaunicode((Uchar *)cp,
						((contents + *size) - cp),
						target);
			*sp = '/';

			if (len > 255) {
				seterrno(ENAMETOOLONG);
				*size = 0;
				return (-1);
			}
			if (((contents + *size) - cp) < (4+len)) {
				seterrno(ENAMETOOLONG);
				*size = 0;
				return (-1);
			}
			cp[-3] = len;
			cp += len;
		}
		while (*sp == '/')
			sp++;
		target = sp;
	}
	nchar = cp - contents;
	*size = nchar;
	return (nchar);
#else
	int	nchar = -1;

	memset(contents, 0, *size);
	*size = 0;
	return (nchar);
#endif
}

#endif  /* UDF */
