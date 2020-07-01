hV6,sum=24988
s 00027/00004/00070
d D 1.3 2020/06/24 20:52:36+0200 joerg 3 2
S s 46084
c Neue Tests im NewMode aus subdir
e
s 00002/00002/00072
d D 1.2 2020/06/21 18:26:13+0200 joerg 2 1
S s 64926
c "sccs init" -> "sccs init ."
e
s 00074/00000/00000
d D 1.1 2020/06/08 21:44:03+0200 joerg 1 0
S s 64566
c date and time created 20/06/08 21:44:03 by joerg
e
u
U
f e 0
G r 0e46e8b6bb582
G p sccs/tests/sccstests/sccs/sccs-all.sh
t
T
I 1
#! /bin/sh
#
# %Z%%M%	%I% %E% Copyright 2020 J. Schilling
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

D 3
remove command.log log log.stdout log.stderr SCCS
E 3
I 3
remove command.log log log.stdout log.stderr SCCS subdir
E 3

I 3
mkdir subdir

E 3
g=tfile
s=SCCS/s.${g}
S=$s
p=SCCS/p.${g}
x=SCCS/x.${g}
z=SCCS/z.${g}
I 3
tdir=''
E 3
output=got.output	# for ../../common/optv
error=got.error		# for ../../common/optv

remove $s $p $g $x $z .sccs SCCS/s.${g} SCCS

echo "Using the driver program ${sccs}"


SCCS_NMODE=FALSE
export SCCS_NMODE

mkdir SCCS 2>/dev/null

echo "Testing old mode..."
. ./dotests
D 3
remove .sccs
E 3
I 3
[ $? != 0 ] && fail "old mode ..."
remove .sccs $g
E 3


SCCS_NMODE=i
D 2
${vg_sccs} init -s
E 2
I 2
${vg_sccs} init -fs .
E 2

s=${g}
echo "Testing new mode with in-tree history files..."
. ./dotests
D 3
remove .sccs
E 3
I 3
[ $? != 0 ] && fail "in-tree history files..."
remove .sccs $g
E 3


I 3
${vg_sccs} init -fs .

echo "Testing new mode with in-tree history files from subdir..."
(tdir=../;cd subdir; . ../dotests)
[ $? != 0 ] && fail "in-tree history files from subdir..."
remove .sccs $g


E 3
SCCS_NMODE=o
D 2
${vg_sccs} init -s
E 2
I 2
${vg_sccs} init -fs .
E 2

s=${g}
S=.sccs/data/SCCS/s.$s
echo "Testing new mode with off-tree history files..."
. ./dotests
D 3
remove .sccs
E 3
I 3
[ $? != 0 ] && fail "off-tree history files..."
remove .sccs $g
E 3

I 3
${vg_sccs} init -fs .
E 3

I 3
echo "Testing new mode with off-tree history files from subdir..."
(tdir=../;cd subdir; . ../dotests)
[ $? != 0 ] && fail "off-tree history files from subdir..."
remove .sccs $g


remove subdir

E 3
restore		# Restore SCCS directory in current directory.
success
E 1
