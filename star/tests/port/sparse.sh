#! /bin/sh
#
# @(#)sparse.sh	1.3 20/03/30 Copyright 2019-2020 J. Schilling
#

# sparse.sh:	Tests to check whether sparse files work.
#		This is portability in contrast to achive type
#		recognition.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL
#
# IRIX does GMT-1 wrong
#
TZ=MET export TZ

#d=`../testutils/realpwd`

s=../../testscripts/tar-test-inputs/sparse-files/gnu-small.tar
docommand SPARSE-gnu-small "${tar} -tv f=${s}" 0 "\
 524288 Srw-r--r--  mgorny/mgorny Nov 24 11:29 2018 input.bin
" "\
star: Blocksize = 19 records.
star: 1 blocks + 0 bytes (total of 9728 bytes = 9.50k).
"

s=../../testscripts/tar-test-inputs/sparse-files/gnu.tar
docommand SPARSE-gnu "${tar} -tv f=${s}" 0 "\
2097152 Srw-r--r--  mgorny/mgorny Nov 24 11:26 2018 input.bin
" "\
star: 3 blocks + 4096 bytes (total of 34816 bytes = 34.00k).
"

s=../../testscripts/tar-test-inputs/sparse-files/star.tar
docommand SPARSE-star "${tar} -tv f=${s}" 0 "\
2097152 Srw-r--r--  mgorny/mgorny Nov 24 11:26 2018 input.bin
" "\
star: 4 blocks + 0 bytes (total of 40960 bytes = 40.00k).
"

s=../../testscripts/tar-test-inputs/sparse-files/xstar.tar
docommand SPARSE-xstar "${tar} -tv f=${s}" 0 "\
2097152 Srw-r--r--  mgorny/mgorny Nov 24 11:26 2018 input.bin
" "\
star: 4 blocks + 0 bytes (total of 40960 bytes = 40.00k).
"

success
