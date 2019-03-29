#! /bin/sh
#
# @(#)lpath.sh	1.2 19/03/25 Copyright 2019 J. Schilling
#

# lpath.sh:	Tests to check whether long path names.
#		This is portability in contrast to achive type
#		recognition.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL

#d=`../testutils/realpwd`

s=../../testscripts/tar-test-inputs/long-paths/gnu.tar
docommand LPATH-gnu "${tar} -t f=${s}" 0 "012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/input.txt
" IGNORE

s=../../testscripts/tar-test-inputs/long-paths/pax.tar
docommand LPATH-pax "${tar} -t f=${s}" 0 "012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/input.txt
" IGNORE

s=../../testscripts/tar-test-inputs/long-paths/star.tar
docommand LPATH-star "${tar} -t f=${s}" 0 "012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/input.txt
" IGNORE

s=../../testscripts/tar-test-inputs/long-paths/ustar.tar
docommand LPATH-ustar "${tar} -t f=${s}" 0 "012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/012345678901234567890123456789/input.txt
" IGNORE

s=../../testscripts/longpath.tar.bz2
do_output LPATH-verylong-t "${tar} -t f=${s}" 0 longnames IGNORE
docommand LPATH-verylong-x "${tar} -x -no-fsync f=${s}" 0 "" "\
star: WARNING: Archive is 'bzip2' compressed, trying to use the -bz option.
star: 8 blocks + 9728 bytes (total of 91648 bytes = 89.50k).
"

remove LLLong
success
