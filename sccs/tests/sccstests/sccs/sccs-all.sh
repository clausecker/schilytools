#! /bin/sh
#
# @(#)sccs-all.sh	1.3 20/06/24 Copyright 2020 J. Schilling
#
# sccs-all.sh:  Testing for the basic operation of the BSD wrapper "sccs".
#                   We test each of the subcommands in all three work modes:
#
#	-	Classic mode
#	-	NewMode with in-tree history store
#	-	NewMode with off-trrr history store

# Import common functions & definitions.
. ../../common/test-common
. ../../common/real-thing

# The test suite fails if you run it as root, particularly because
# "test -w foo" returns 0 if you are root, even if foo is a readonly
# file. We try to avoid this by calling the "wtest" function instead
# of just "test".
# Please don't run the test suite as root, because it may spuriously
# fail.
. ../../common/not-root

. ./setup	# Loda save and restore functions

setup		# Save SCCS directory in current directory.

remove command.log log log.stdout log.stderr SCCS subdir

mkdir subdir

g=tfile
s=SCCS/s.${g}
S=$s
p=SCCS/p.${g}
x=SCCS/x.${g}
z=SCCS/z.${g}
tdir=''
output=got.output	# for ../../common/optv
error=got.error		# for ../../common/optv

remove $s $p $g $x $z .sccs SCCS/s.${g} SCCS

echo "Using the driver program ${sccs}"


SCCS_NMODE=FALSE
export SCCS_NMODE

mkdir SCCS 2>/dev/null

echo "Testing old mode..."
. ./dotests
[ $? != 0 ] && fail "old mode ..."
remove .sccs $g


SCCS_NMODE=i
${vg_sccs} init -fs .

s=${g}
echo "Testing new mode with in-tree history files..."
. ./dotests
[ $? != 0 ] && fail "in-tree history files..."
remove .sccs $g


${vg_sccs} init -fs .

echo "Testing new mode with in-tree history files from subdir..."
(tdir=../;cd subdir; . ../dotests)
[ $? != 0 ] && fail "in-tree history files from subdir..."
remove .sccs $g


SCCS_NMODE=o
${vg_sccs} init -fs .

s=${g}
S=.sccs/data/SCCS/s.$s
echo "Testing new mode with off-tree history files..."
. ./dotests
[ $? != 0 ] && fail "off-tree history files..."
remove .sccs $g

${vg_sccs} init -fs .

echo "Testing new mode with off-tree history files from subdir..."
(tdir=../;cd subdir; . ../dotests)
[ $? != 0 ] && fail "off-tree history files from subdir..."
remove .sccs $g


remove subdir

restore		# Restore SCCS directory in current directory.
success
