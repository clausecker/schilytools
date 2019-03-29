#! /bin/sh
#
# @(#)id.sh	1.2 19/03/25 Copyright 2019 J. Schilling
#

# id.sh:	Tests to check whether large uid/gid or long uname/gname work.
#		This is portability in contrast to achive type
#		recognition.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL

#d=`../testutils/realpwd`

#
# Warning: large number tests fail on platforms where uid_t/git_t is a short.
#

s=../../testscripts/tar-test-inputs/user-group-largenum/8-digit.tar
docommand BIG-ID-8-byte "${tar} -tv f=${s}" 0 "\
      5 -rw-r--r--  8388608/8388608 Nov 23 18:49 2018 input.txt
" "\
star: Blocksize = 4 records.
star: WARNING: Unterminated octal number at 0.
star: WARNING: Unterminated octal number at 0.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"

s=../../testscripts/tar-test-inputs/user-group-largenum/gnu.tar
docommand BIG-ID-base-256 "${tar} -tv f=${s}" 0 "\
      5 -rw-r--r--  2147483648/2147483648 Nov 23 18:49 2018 input.txt
" "\
star: Blocksize = 4 records.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"

s=../../testscripts/tar-test-inputs/user-group-largenum/pax.tar
docommand BIG-ID-pax "${tar} -tv f=${s}" 0 "\
      5 -rw-r--r--  2147483648/2147483648 Nov 23 18:49 2018 input.txt
" "\
star: Blocksize = 6 records.
star: 1 blocks + 0 bytes (total of 3072 bytes = 3.00k).
"

s=../../testscripts/tar-test-inputs/user-group-name/ustar-32chars.tar
docommand LONG-ID-32-chars "${tar} -tv f=${s}" 0 "\
      5 -rw-r--r--  veryveryveryveryveryveryverylong/veryveryveryveryveryveryverylong Nov 23 19:00 2018 input.txt
" "\
star: Blocksize = 4 records.
star: 1 blocks + 0 bytes (total of 2048 bytes = 2.00k).
"

s=../../testscripts/tar-test-inputs/user-group-name/pax.tar
docommand LONG-ID-pax "${tar} -tv f=${s}" 0 "\
      5 -rw-r--r--  veryveryveryveryveryveryverylongg/veryveryveryveryveryveryverylongg Nov 23 19:00 2018 input.txt
" "\
star: Blocksize = 6 records.
star: 1 blocks + 0 bytes (total of 3072 bytes = 3.00k).
"

success
