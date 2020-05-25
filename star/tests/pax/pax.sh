#! /bin/sh
#
# @(#)pax.sh	1.1 20/05/24 Copyright 2020 J. Schilling
#

# pax.sh:	Tests to check whether pax specific features work.

# Import common functions & definitions.
. ../common/test-common

pax="${pax-${tar} cli=pax}"

LC_ALL=C export LC_ALL

#d=`../testutils/realpwd`

s=../../testscripts/pax-rename.tar.gz

docommand pax-subst00 "${pax} '*.cfg' <${s}" 0 "dir1/sdi1/cfg/fich.cfg
dir1/sdi2/cfg/fich.cfg
" "IGNORE"

docommand pax-subst01 "${pax} -s ',dir1/\([^/]*\)/\([^/]*\),dir2/\1-\2,' '*.cfg' <${s}" 0 "dir2/sdi1-cfg/fich.cfg
dir2/sdi2-cfg/fich.cfg
" "IGNORE"

success
