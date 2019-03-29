#! /bin/sh
#
# @(#)size.sh	1.2 19/03/25 Copyright 2019 J. Schilling
#

# size.sh:	Tests to check whether long file sizes work.
#		This is portability in contrast to achive type
#		recognition.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL

#d=`../testutils/realpwd`

s=../../testscripts/tar-test-inputs/file-size/12-digit.tar.bz2
docommand LARGE-12-digit "${tar} -t f=${s}" 0 "big-file.bin
small-file.txt
" "\
star: WARNING: Archive is 'bzip2' compressed, trying to use the -bz option.
star: WARNING: Unterminated octal number at 0.
star: 838862 blocks + 0 bytes (total of 8589946880 bytes = 8388620.00k).
"

s=../../testscripts/tar-test-inputs/file-size/gnu.tar.bz2
docommand LARGE-base-256 "${tar} -t f=${s}" 0 "big-file.bin
small-file.txt
" "\
star: WARNING: Archive is 'bzip2' compressed, trying to use the -bz option.
star: 838861 blocks + 512 bytes (total of 8589937152 bytes = 8388610.50k).
"

s=../../testscripts/tar-test-inputs/file-size/pax.tar.bz2
docommand LARGE-pax "${tar} -t f=${s}" 0 "big-file.bin
small-file.txt
" "\
star: WARNING: Archive is 'bzip2' compressed, trying to use the -bz option.
star: 838861 blocks + 2560 bytes (total of 8589939200 bytes = 8388612.50k).
"

success
