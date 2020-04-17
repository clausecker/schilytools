#! /bin/sh
#
# @(#)vlabel.sh	1.3 20/03/30 Copyright 2019-2020 J. Schilling
#

# vlabel.sh:	Tests to check whether volume labels work.
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

s=../../testscripts/tar-test-inputs/volume-label/gnu.tar
docommand VLABEL-gnu "${tar} -tv f=${s}" 0 "\
  0   0 V---------    0/0   Nov 23 10:54 2018 test --Volume Header--
      5 -rw-r--r--  mgorny/mgorny Nov 23 10:53 2018 input.txt
" "\
star: Blocksize = 5 records.
star: Wrong magic at: 0: ''.
star: WARNING: Archive violates POSIX 1003.1 (mode field starts with null byte).
star: 1 blocks + 0 bytes (total of 2560 bytes = 2.50k).
"

s=../../testscripts/tar-test-inputs/volume-label/pax.tar
docommand VLABEL-gnu-pax "${tar} -tv f=${s}" 0 "\
      5 -rw-r--r--  mgorny/mgorny Nov 23 10:53 2018 input.txt
" "\
star: Blocksize = 8 records.
star: Unknown extended header keyword 'GNU.volume.label' ignored at -1.
star: 1 blocks + 0 bytes (total of 4096 bytes = 4.00k).
"

s=../../testscripts/tar-test-inputs/volume-label/star.tar
docommand VLABEL-star "${tar} -tv f=${s}" 0 "\
  0   0 V---------  root/root Nov 24 14:24 2018 test --Volume Header--
      5 -rw-r--r--  mgorny/mgorny Nov 23 10:53 2018 input.txt
" "\
star: Blocksize = 5 records.
star: 1 blocks + 0 bytes (total of 2560 bytes = 2.50k).
"

success
