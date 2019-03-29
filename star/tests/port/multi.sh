#! /bin/sh
#
# @(#)multi.sh	1.2 19/03/25 Copyright 2019 J. Schilling
#

# multi.sh:	Tests to check whether multi volume archives work.
#		This is portability in contrast to achive type
#		recognition.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL

#d=`../testutils/realpwd`

s1=../../testscripts/tar-test-inputs/multi-volume/exustar-1.tar
s2=../../testscripts/tar-test-inputs/multi-volume/exustar-2.tar
docommand MULTV-exustar "${tar} -t -multivol new-volume-script=true f=${s1} f=${s2}" 0 "\
<none> --Volume Header--
input.txt
" "\
star: 1 blocks + 5120 bytes (total of 15360 bytes = 15.00k).
star: 1 blocks + 1536 bytes (total of 11776 bytes = 11.50k).
star: Total 2 blocks + 6656 bytes (total of 27136 bytes = 26.50k).
"

s1=../../testscripts/tar-test-inputs/multi-volume/xstar-1.tar
s2=../../testscripts/tar-test-inputs/multi-volume/xstar-2.tar
docommand MULTV-xstar "${tar} -t -multivol new-volume-script=true f=${s1} f=${s2}" 0 "\
<none> --Volume Header--
input.txt
" "\
star: 1 blocks + 6144 bytes (total of 16384 bytes = 16.00k).
star: 0 blocks + 6656 bytes (total of 6656 bytes = 6.50k).
star: Total 1 blocks + 12800 bytes (total of 23040 bytes = 22.50k).
"

s1=../../testscripts/tar-test-inputs/multi-volume/xustar-1.tar
s2=../../testscripts/tar-test-inputs/multi-volume/xustar-2.tar
docommand MULTV-xustar "${tar} -t -multivol new-volume-script=true f=${s1} f=${s2}" 0 "\
<none> --Volume Header--
input.txt
" "\
star: 1 blocks + 6144 bytes (total of 16384 bytes = 16.00k).
star: 0 blocks + 7680 bytes (total of 7680 bytes = 7.50k).
star: Total 1 blocks + 13824 bytes (total of 24064 bytes = 23.50k).
"

s1=../../testscripts/tar-test-inputs/multi-volume/gnu-1.tar
s2=../../testscripts/tar-test-inputs/multi-volume/gnu-2.tar
docommand MULTV-gnu "${tar} -t -multivol new-volume-script=true f=${s1} f=${s2}" 0 "\
input.txt
" "\
star: 1 blocks + 6144 bytes (total of 16384 bytes = 16.00k).
star: 0 blocks + 6144 bytes (total of 6144 bytes = 6.00k).
star: Total 1 blocks + 12288 bytes (total of 22528 bytes = 22.00k).
"

if false; then
#
# GNU tar uses a standard plain file header and this way makes star
# believe that there is a problem.
#
s1=../../testscripts/tar-test-inputs/multi-volume/gnupax-1.tar
s2=../../testscripts/tar-test-inputs/multi-volume/gnupax-2.tar
docommand MULTV-gnupax "${tar} -t -multivol new-volume-script=true f=${s1} f=${s2}" 0 "\
input.txt
" "\
star: 1 blocks + 6144 bytes (total of 16384 bytes = 16.00k).
star: Unknown extended header keyword 'GNU.volume.filename' ignored at 17.
star: Unknown extended header keyword 'GNU.volume.size' ignored at 17.
star: Unknown extended header keyword 'GNU.volume.offset' ignored at 17.
star: 0 blocks + 6144 bytes (total of 6144 bytes = 6.00k).
star: Total 1 blocks + 12288 bytes (total of 22528 bytes = 22.00k).
"
fi

success
