#! /bin/sh
#
# @(#)lpath.sh	1.4 20/04/12 Copyright 2019 J. Schilling
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
#
# We cannot use remove as IRIX is buggy and will not remove long paths
#
#remove LLLong

#
# /bin/sh is frequently dash on Linux and this is crappy software and may tear
# down the whole system if it executes the function below. For this reason, we
# first check whether he have bosh (as it has no path limitations at all) or
# whether we have at leash bash.
#
xsh=/bin/sh
if bosh -c :; then
	xsh=bosh
elif bash -c :; then
	xsh=bash
fi
echo Using $xsh to remove long path names...

$xsh -c '

xrm() { 
	for i in "$@"; do
		if [ -d "$i" ]; then
			( cd "$i" ; xrm * )
		fi
		rm -rf "$i"
	done
} 
 
xrm LLLong
'

success
