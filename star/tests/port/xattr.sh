#! /bin/sh
#
# @(#)xattr.sh	1.2 19/03/25 Copyright 2019 J. Schilling
#

# xattr.sh:	Tests to check whether xattr works.
#		This is portability in contrast to achive type
#		recognition.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL

#d=`../testutils/realpwd`

s=../../testscripts/tar-test-inputs/xattr/acl.tar
docommand XATTR-acl "${tar} -tv f=${s}" 0 "\
      5 -rw-------+ mgorny/mgorny Nov 24 18:59 2018 input.txt
" "\
star: Blocksize = 6 records.
star: 1 blocks + 0 bytes (total of 3072 bytes = 3.00k).
"

s=../../testscripts/tar-test-inputs/xattr/fflags-libarchive.tar
docommand XATTR-fflags-libarchive-defect "${tar} -tv f=${s}" 0 "\
      5 -rw-r--r--  mgorny/mgorny Nov 24 21:14 2018 input.txt
" "\
star: Blocksize = 6 records.
star: 1 blocks + 0 bytes (total of 3072 bytes = 3.00k).
"

s=../../testscripts/tar-test-inputs/xattr/fflags-schily.tar
docommand XATTR-fflags-schily "${tar} -tv f=${s}" 0 "\
Release     star 1.5.3 (x86_64-unknown-linux-gnu)
Archtype    exustar
Dumpdate    1543095975.083524831 (Sat Nov 24 22:46:15 2018)
Volno       1
Blocksize   3 records
      5 -rw-r--r--  mgorny/mgorny Nov 24 21:14 2018 input.txt
" "\
star: Blocksize = 9 records.
star: 1 blocks + 0 bytes (total of 4608 bytes = 4.50k).
"

s=../../testscripts/tar-test-inputs/xattr/xattr-libarchive.tar
docommand XATTR-xattr-libarchive "${tar} -tv f=${s}" 0 "\
      5 -rw-r--r--  mgorny/mgorny Nov 24 22:21 2018 input.txt
" "\
star: Blocksize = 6 records.
star: Unknown extended header keyword 'LIBARCHIVE.xattr.user.mime_type' ignored at -1.
star: 1 blocks + 0 bytes (total of 3072 bytes = 3.00k).
"

s=../../testscripts/tar-test-inputs/xattr/xattr-schily.tar
docommand XATTR-xattr-schily "${tar} -tv f=${s}" 0 "\
      5 -rw-r--r--@ mgorny/mgorny Nov 24 22:21 2018 input.txt
" "\
star: Blocksize = 6 records.
star: 1 blocks + 0 bytes (total of 3072 bytes = 3.00k).
"

success
