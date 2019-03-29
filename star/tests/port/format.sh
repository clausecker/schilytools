#! /bin/sh
#
# @(#)format.sh	1.2 19/03/25 Copyright 2019 J. Schilling
#

# format.sh:	Tests to check whether all archive types work.
#		This is portability in contrast to achive type
#		recognition.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL

#d=`../testutils/realpwd`

s=../../testscripts/tar-test-inputs/format-acceptance/v7.tar
docommand FORMAT-v7 "${tar} -t f=${s}" 0 "input.txt
" "\
star: Blocksize = 4 records.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"

s=../../testscripts/tar-test-inputs/format-acceptance/ustar-pre-posix.tar
docommand FORMAT-ustar-gtar "${tar} -t f=${s}" 0 "input.txt
" "\
star: Blocksize = 4 records.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"

s=../../testscripts/tar-test-inputs/format-acceptance/ustar.tar
docommand FORMAT-ustar "${tar} -t f=${s}" 0 "input.txt
" "\
star: Blocksize = 4 records.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"

s=../../testscripts/tar-test-inputs/format-acceptance/pax.tar
docommand FORMAT-pax "${tar} -t f=${s}" 0 "input.txt
" "\
star: Blocksize = 6 records.
star: 1 blocks + 0 bytes (total of 3072 bytes = 3.00k).
"

s=../../testscripts/tar-test-inputs/format-acceptance/star.tar
docommand FORMAT-star "${tar} -t f=${s}" 0 "input.txt
" "\
star: Blocksize = 4 records.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"

s=../../testscripts/tar-test-inputs/format-acceptance/suntar.tar
docommand FORMAT-suntar "${tar} -t f=${s}" 0 "input.txt
" "\
star: Blocksize = 6 records.
star: 1 blocks + 0 bytes (total of 3072 bytes = 3.00k).
"

s=../../testscripts/tar-test-inputs/format-acceptance/gnu.tar
docommand FORMAT-gnu "${tar} -t f=${s}" 0 "input.txt
" "\
star: Blocksize = 4 records.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"

s=../../testscripts/tar-test-inputs/format-acceptance/gnu-g.tar
docommand FORMAT-gnu-g "${tar} -t f=${s}" 0 "input.txt
" "\
star: Blocksize = 4 records.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"

success
