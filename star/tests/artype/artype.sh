#! /bin/sh

# artype.sh:	Tests to check whether all archive types work
#		and are auto-detected.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C

#d=`../testutils/realpwd`

s=../../testscripts/ustar-all-filetypes.tar
s=v7tar.tar
docommand AT1a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand AT1b "${tar} -t -print-artype f=${s}" 0 "v7tar.tar: tar archive.
" "\
star: Blocksize = 3 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT1c "${tar} -t -print-artype f=a" 0 "a: swapped tar archive.
" "\
star: Blocksize = 3 records.
"
gzip < ${s} > a
docommand AT1d "${tar} -t -print-artype f=a" 0 "a: gzip compressed tar archive.
" "\
star: Blocksize = 3 records.
"

s=utar.tar
docommand AT2a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand AT2b "${tar} -t -print-artype f=${s}" 0 "utar.tar: unknown tar archive.
" "\
star: Blocksize = 3 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT2c "${tar} -t -print-artype f=a" 0 "a: swapped unknown tar archive.
" "\
star: Blocksize = 3 records.
"
gzip < ${s} > a
docommand AT2d "${tar} -t -print-artype f=a" 0 "a: gzip compressed unknown tar archive.
" "\
star: Blocksize = 3 records.
"

s=star.tar
docommand AT3a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand AT3b "${tar} -t -print-artype f=${s}" 0 "star.tar: star archive.
" "\
star: Blocksize = 3 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT3c "${tar} -t -print-artype f=a" 0 "a: swapped star archive.
" "\
star: Blocksize = 3 records.
"
gzip < ${s} > a
docommand AT3d "${tar} -t -print-artype f=a" 0 "a: gzip compressed star archive.
" "\
star: Blocksize = 3 records.
"

s=gtar.tar
docommand AT4a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand AT4b "${tar} -t -print-artype f=${s}" 0 "gtar.tar: gnutar archive.
" "\
star: Blocksize = 3 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT4c "${tar} -t -print-artype f=a" 0 "a: swapped gnutar archive.
" "\
star: Blocksize = 3 records.
"
gzip < ${s} > a
docommand AT4d "${tar} -t -print-artype f=a" 0 "a: gzip compressed gnutar archive.
" "\
star: Blocksize = 3 records.
"

s=ustar.tar
docommand AT5a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand AT5b "${tar} -t -print-artype f=${s}" 0 "ustar.tar: ustar archive.
" "\
star: Blocksize = 3 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT5c "${tar} -t -print-artype f=a" 0 "a: swapped ustar archive.
" "\
star: Blocksize = 3 records.
"
gzip < ${s} > a
docommand AT5d "${tar} -t -print-artype f=a" 0 "a: gzip compressed ustar archive.
" "\
star: Blocksize = 3 records.
"

s=xstar.tar
docommand AT6a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand AT6b "${tar} -t -print-artype f=${s}" 0 "xstar.tar: xstar archive.
" "\
star: Blocksize = 3 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT6c "${tar} -t -print-artype f=a" 0 "a: swapped xstar archive.
" "\
star: Blocksize = 3 records.
"
gzip < ${s} > a
docommand AT6d "${tar} -t -print-artype f=a" 0 "a: gzip compressed xstar archive.
" "\
star: Blocksize = 3 records.
"

s=xustar.tar
docommand AT7a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand AT7b "${tar} -t -print-artype f=${s}" 0 "xustar.tar: xustar archive.
" "\
star: Blocksize = 3 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT7c "${tar} -t -print-artype f=a" 0 "a: swapped xustar archive.
" "\
star: Blocksize = 3 records.
"
gzip < ${s} > a
docommand AT7d "${tar} -t -print-artype f=a" 0 "a: gzip compressed xustar archive.
" "\
star: Blocksize = 3 records.
"

s=exustar.tar
docommand AT8a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 7 records.
star: 1 blocks + 0 bytes (total of 3584 bytes = 3.50k).
"
docommand AT8b "${tar} -t -print-artype f=${s}" 0 "exustar.tar: exustar archive.
" "\
star: Blocksize = 7 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT8c "${tar} -t -print-artype f=a" 0 "a: swapped exustar archive.
" "\
star: Blocksize = 7 records.
"
gzip < ${s} > a
docommand AT8d "${tar} -t -print-artype f=a" 0 "a: gzip compressed exustar archive.
" "\
star: Blocksize = 7 records.
"

s=suntar.tar
docommand AT9a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 5 records.
star: 1 blocks + 0 bytes (total of 2560 bytes = 2.50k).
"
docommand AT9b "${tar} -t -print-artype f=${s}" 0 "suntar.tar: suntar archive.
" "\
star: Blocksize = 5 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT9c "${tar} -t -print-artype f=a" 0 "a: swapped suntar archive.
" "\
star: Blocksize = 5 records.
"
gzip < ${s} > a
docommand AT9d "${tar} -t -print-artype f=a" 0 "a: gzip compressed suntar archive.
" "\
star: Blocksize = 5 records.
"

s=pax.tar
docommand AT10a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 5 records.
star: 1 blocks + 0 bytes (total of 2560 bytes = 2.50k).
"
docommand AT10b "${tar} -t -print-artype f=${s}" 0 "pax.tar: pax archive.
" "\
star: Blocksize = 5 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT10c "${tar} -t -print-artype f=a" 0 "a: swapped pax archive.
" "\
star: Blocksize = 5 records.
"
gzip < ${s} > a
docommand AT10d "${tar} -t -print-artype f=a" 0 "a: gzip compressed pax archive.
" "\
star: Blocksize = 5 records.
"

s=bar.bar
docommand AT11a "${tar} -t f=${s}" "!=0" "" "\
star: Blocksize = 16 records.
star: Can't handle bar archives (yet).
star: 1 blocks + 0 bytes (total of 8192 bytes = 8.00k).
"
docommand AT11b "${tar} -t -print-artype f=${s}" 0 "bar.bar: bar archive.
" "\
star: Blocksize = 16 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT11c "${tar} -t -print-artype f=a" 0 "a: swapped bar archive.
" "\
star: Blocksize = 16 records.
"
gzip < ${s} > a
docommand AT11d "${tar} -t -print-artype f=a" 0 "a: gzip compressed bar archive.
" "\
star: Blocksize = 16 records.
"

s=bin.cpio
docommand AT12a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 1 records.
star: 1 blocks + 0 bytes (total of 512 bytes = 0.50k).
"
docommand AT12b "${tar} -t -print-artype f=${s}" 0 "bin.cpio: bin archive.
" "\
star: Blocksize = 1 records.
"
#
# "file" is a filename with even length.
# "bin" cpio archives may use any byte order for the numbers in the header
# so we cannot use the byte order in the header to detect swapped archives.
# Filenames with odd length result in a null byte inside the filename and
# this allows us to auto-detect byte swapped archives.
#
s=binodd.cpio
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT12c "${tar} -t -print-artype f=a" 0 "a: swapped bin archive.
" "\
star: Blocksize = 1 records.
"
gzip < ${s} > a
docommand AT12d "${tar} -t -print-artype f=a" 0 "a: gzip compressed bin archive.
" "\
star: Blocksize = 1 records.
"

s=cpio.cpio
docommand AT13a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 1 records.
star: 1 blocks + 0 bytes (total of 512 bytes = 0.50k).
"
docommand AT13b "${tar} -t -print-artype f=${s}" 0 "cpio.cpio: cpio archive.
" "\
star: Blocksize = 1 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT13c "${tar} -t -print-artype f=a" 0 "a: swapped cpio archive.
" "\
star: Blocksize = 1 records.
"
gzip < ${s} > a
docommand AT13d "${tar} -t -print-artype f=a" 0 "a: gzip compressed cpio archive.
" "\
star: Blocksize = 1 records.
"

s=odc.cpio
docommand AT14a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 1 records.
star: 1 blocks + 0 bytes (total of 512 bytes = 0.50k).
"
docommand AT14b "${tar} -t -print-artype f=${s}" 0 "odc.cpio: cpio archive.
" "\
star: Blocksize = 1 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT14c "${tar} -t -print-artype f=a" 0 "a: swapped cpio archive.
" "\
star: Blocksize = 1 records.
"
gzip < ${s} > a
docommand AT14d "${tar} -t -print-artype f=a" 0 "a: gzip compressed cpio archive.
" "\
star: Blocksize = 1 records.
"

s=crc.cpio
docommand AT15a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 1 records.
star: 1 blocks + 0 bytes (total of 512 bytes = 0.50k).
"
docommand AT15b "${tar} -t -print-artype f=${s}" 0 "crc.cpio: crc archive.
" "\
star: Blocksize = 1 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT15c "${tar} -t -print-artype f=a" 0 "a: swapped crc archive.
" "\
star: Blocksize = 1 records.
"
gzip < ${s} > a
docommand AT15d "${tar} -t -print-artype f=a" 0 "a: gzip compressed crc archive.
" "\
star: Blocksize = 1 records.
"

s=asc.cpio
docommand AT16a "${tar} -t f=${s}" 0 "file
" "\
star: Blocksize = 1 records.
star: 1 blocks + 0 bytes (total of 512 bytes = 0.50k).
"
docommand AT16b "${tar} -t -print-artype f=${s}" 0 "asc.cpio: asc archive.
" "\
star: Blocksize = 1 records.
"
dd if=${s} conv=swab of=a 2>/dev/null
docommand AT16c "${tar} -t -print-artype f=a" 0 "a: swapped asc archive.
" "\
star: Blocksize = 1 records.
"
gzip < ${s} > a
docommand AT16d "${tar} -t -print-artype f=a" 0 "a: gzip compressed asc archive.
" "\
star: Blocksize = 1 records.
"

remove a
success
