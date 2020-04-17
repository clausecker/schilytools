#! /bin/sh
#
# @(#)mtime.sh	1.4 20/03/30 Copyright 2019-2020 J. Schilling
#

# mtime.sh:	Tests to check whether large and negative mtimes work.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL
#
# IRIX does GMT-1 wrong
#
TZ=MET export TZ

#d=`../testutils/realpwd`

is_64bit=`file $tar | grep 64-`
if [ "$is_64bit" ]; then
	is_64bit=true
else
	is_64bit=false
	echo "Cannot compare large mtime time stamp when in 32-bit mode"
fi
s=../../testscripts/tar-test-inputs/large-mtime/12-digit.tar 
docommand LMTIME-12digit "${tar} -t f=${s}" 0 "input.txt
" "\
star: Blocksize = 4 records.
star: WARNING: Unterminated octal number at 0.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"
if $is_64bit; then
docommand LMTIME-12digit-v "${tar} -tv f=${s}" 0 "      5 -rw-r--r--  mgorny/mgorny Mar 16 13:56 2242 input.txt
" "\
star: Blocksize = 4 records.
star: WARNING: Unterminated octal number at 0.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"
fi

s=../../testscripts/tar-test-inputs/large-mtime/gnu.tar 
docommand LMTIME-base-256 "${tar} -t f=${s}" 0 "input.txt
" "\
star: Blocksize = 4 records.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"
if $is_64bit; then
docommand LMTIME-base-256-v "${tar} -tv f=${s}" 0 "      5 -rw-r--r--  mgorny/mgorny Mar 16 13:56 2242 input.txt
" "\
star: Blocksize = 4 records.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"
fi

#
# On a 32 bit system we get this warning and thus can only check for
# NONEMPTY stderr
#
# star: WARNING: mtime '8589934592' in extended header at -1 exceeds local range.
# star: Bad timespec '8589934592' for 'mtime' in extended header at -1.
# star: WARNING: mtime '8589934592' in extended header at 1 exceeds local range.
# star: Bad timespec '8589934592' for 'mtime' in extended header at 1.
#
s=../../testscripts/tar-test-inputs/large-mtime/pax.tar 
docommand LMTIME-pax "${tar} -t f=${s}" 0 "input.txt
" NONEMPTY
if $is_64bit; then
docommand LMTIME-pax-v "${tar} -tv f=${s}" 0 "      5 -rw-r--r--  mgorny/mgorny Mar 16 13:56 2242 input.txt
" NONEMPTY
fi

s=../../testscripts/tar-test-inputs/negative-mtime/gnu.tar
docommand NMTIME-base-256 "${tar} -tv f=${s}" 0 "      5 -rw-r--r--  mgorny/mgorny Jan  1 00:00 1960 input.txt
" "\
star: 1 blocks + 0 bytes (total of 10240 bytes = 10.00k).
"

s=../../testscripts/tar-test-inputs/negative-mtime/pax.tar
docommand NMTIME-pax "${tar} -tv f=${s}" 0 "      5 -rw-r--r--  mgorny/mgorny Jan  1 00:00 1960 input.txt
" "\
star: 1 blocks + 0 bytes (total of 10240 bytes = 10.00k).
"


success
