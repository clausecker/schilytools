#! /bin/sh
#
# @(#)compress.sh	1.4 19/06/14 Copyright 2019 J. Schilling
#

# compress.sh:	Tests to check whether all compression types work
#		and are auto-detected.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL

#d=`../testutils/realpwd`

# File name	COMPR.		Created with:	Uncompress with:
#
# tar.tar.z	1 C_PACK	"pack"		"pack" / "gzip"
# tar.tar.gz	2 C_GZIP	"gzip"		"gzip"
# tar.tar.Z	3 C_LZW		"compress"	"uncompress" / "gzip"
#		4 C_FREEZE	"freeze"	"melt" / "gzip"
#		5 C_LZH		"lzh"		"gzip"
#		6 C_PKZIP	"pkzip"		"gzip"
# tar.tar.bz2	7 C_BZIP2	"bzip2"		"bzip2"
# tar.tar.lzo	8 C_LZO		"lzop"		"lzop"
# tar.tar.7z	9 C_7Z		"p7zip"		"p7zip"
# tar.tar.xz	10 C_XZ		"xz"		"xz"
# tar.tar.lz	11 C_LZIP	"lzip"		"lzip"
# tar.tar.zst	12 C_ZSTD	"zstd"		"zstd"
# tar.tar.lzma	13 C_LZMA	"lzma"		"lzma"
# tar.tar.F2	14 C_FREEZE2	"freeze"	"freeze"

#
# pack	1 C_PACK
#
if type gzip > /dev/null; then		# XXX See gzip related "fi" below

if gzip -d < tar.tar.z > /dev/null; then
s=tar.tar.z
docommand COMP-pack "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is 'pack' compressed, trying to use the -z option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-pack-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.z: pack compressed ustar archive.
" "\
star: Blocksize = 3 records.
"
else
	#
	# This is a bug seen on Ubuntu-16.04 with gzip-1.6
	#
	echo "FAIL: gzip program is unable to decompress packed data: Skipping related test"
fi

#
# gzip	2 C_GZIP
#
s=tar.tar.gz
docommand COMP-gzip "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is 'gzip' compressed, trying to use the -z option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-gzip-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.gz: gzip compressed ustar archive.
" "\
star: Blocksize = 3 records.
"

#
# lzw	3 C_LZW	(compress)
#
if gzip -d < tar.tar.Z > /dev/null; then
s=tar.tar.Z
docommand COMP-lzw-compress "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is 'lzw' compressed, trying to use the -z option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-lzw-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.Z: lzw compressed ustar archive.
" "\
star: Blocksize = 3 records.
"
else
	#
	# This is a bug seen on Cygwin
	#
	echo "FAIL: gzip program is unable to decompress compressed data: Skipping related test"
fi

#
# freeze 4 C_FREEZE
#

#
# lzh	5 C_LZH
#

#
# pkzip	6 C_PKZIP
#
else
	echo "gzip missing: Skipping related compression tests"
fi				# XXX This is the gzip related "fi"

#
# bzip2	7 C_BZIP2
#
if type bzip2 > /dev/null; then
s=tar.tar.bz2
docommand COMP-bzip2 "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is 'bzip2' compressed, trying to use the -bz option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-bzip2-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.bz2: bzip2 compressed ustar archive.
" "\
star: Blocksize = 3 records.
"
else
	echo "bzip2 missing: Skipping related compression tests"
fi

#
# lzo	8 C_LZO
#
if type lzop > /dev/null; then
s=tar.tar.lzo
docommand COMP-lzo "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is 'lzop' compressed, trying to use the -lzo option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-lzo-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.lzo: lzo compressed ustar archive.
" "\
star: Blocksize = 3 records.
"
else
	echo "lzop missing: Skipping related compression tests"
fi

#
# 7z	9 C_7Z
#
if type p7zip > /dev/null; then
s=tar.tar.7z
docommand COMP-7z "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is '7z' compressed, trying to use the -7z option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-7z-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.7z: 7z compressed ustar archive.
" "\
star: Blocksize = 3 records.
"
else
	echo "p7zip missing: Skipping related compression tests"
fi

#
# xz	10 C_XZ
#
if type xz > /dev/null; then
s=tar.tar.xz
docommand COMP-xz "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is 'xz' compressed, trying to use the -xz option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-xz-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.xz: xz compressed ustar archive.
" "\
star: Blocksize = 3 records.
"
else
	echo "xz missing: Skipping related compression tests"
fi

#
# lzip	11 C_LZIP
#
if type lzip > /dev/null; then
s=tar.tar.lz
docommand COMP-lzip "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is 'lzip' compressed, trying to use the -lzip option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-lzip-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.lz: lzip compressed ustar archive.
" "\
star: Blocksize = 3 records.
"
else
	echo "lzip missing: Skipping related compression tests"
fi

#
# zstd	12 C_ZSTD
#
if type zstd > /dev/null; then
s=tar.tar.zst
docommand COMP-zstd "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is 'zstd' compressed, trying to use the -zstd option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-zstd-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.zst: zstd compressed ustar archive.
" "\
star: Blocksize = 3 records.
"
else
	echo "zstd missing: Skipping related compression tests"
fi

#
# lzma	13 C_LZMA
#
if type lzma > /dev/null; then
s=tar.tar.lzma
docommand COMP-lzma "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is 'lzma' compressed, trying to use the -lzma option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-lzma-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.lzma: lzma compressed ustar archive.
" "\
star: Blocksize = 3 records.
"
else
	echo "lzma missing: Skipping related compression tests"
fi

#
# lzma	14 C_FREEZE2
#
if type freeze > /dev/null; then
s=tar.tar.F2
docommand COMP-freeze2 "${tar} -t f=${s}" 0 "file
" "\
star: WARNING: Archive is 'freeze2' compressed, trying to use the -freeze option.
star: Blocksize = 3 records.
star: 1 blocks + 0 bytes (total of 1536 bytes = 1.50k).
"
docommand COMP-freeze2-prtype "${tar} -t -print-artype f=${s}" 0 "\
tar.tar.F2: freeze2 compressed ustar archive.
" "\
star: Blocksize = 3 records.
"
else
	echo "lzma missing: Skipping related compression tests"
fi

success
