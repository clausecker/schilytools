h54114
s 00002/00002/00270
d D 1.4 15/06/03 00:06:43 joerg 4 3
c ../common/test-common -> ../../common/test-common
e
s 00013/00008/00259
d D 1.3 15/04/25 18:43:53 joerg 3 2
c test -w -> wtest -w ... wtest ist eine Funktion mit ls -l | grep
e
s 00029/00002/00238
d D 1.2 11/05/30 19:30:32 joerg 2 1
c setup()/restore() neu um SCCS nach XSCCS und zurueck zu wandeln
e
s 00240/00000/00000
d D 1.1 11/04/30 19:50:22 joerg 1 0
c date and time created 11/04/30 19:50:22 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh
# driver-basic.sh:  Testing for the basic operation of the BSD wrapper "sccs".
#                   We test each of the subcommands.

# Import common functions & definitions.
D 4
. ../common/test-common
E 4
I 4
. ../../common/test-common
E 4

# The test suite fails if you run it as root, particularly because
# "test -w foo" returns 0 if you are root, even if foo is a readonly
D 3
# file.
# So please don't run the test suite as root, because it will spuriously
E 3
I 3
# file. We try to avoid this by calling the "wtest" function instead
# of just "test".
# Please don't run the test suite as root, because it may spuriously
E 3
# fail.
D 3
true 
E 3
D 4
. ../common/not-root
E 4
I 4
. ../../common/not-root
E 4

I 2
setup() {
	if test -f SCCS/s.driver-basic.sh
	then
		if test -d XSCCS
		then
			miscarry "Cannot rename SCCS to XSCCS, XSCCS exists"
		else
			mv SCCS XSCCS
		fi
	fi
}
E 2

I 2
restore() {
	if test -f XSCCS/s.driver-basic.sh
	then
		if test -d SCCS
		then
			miscarry "Cannot rename XSCCS back to XSCCS, SCCS exists"
		else
			mv XSCCS SCCS
		fi
	fi
}
E 2

I 2

E 2
# If LANG is defined but the system is misconfigured, we will produce
# the error message "Error setting locale: No such file or directory".
# If that happens, the test suite will fail.  For this reason, we
# unset the LANG environment variable.  Of course, things being
# printed out in the wrong language would also mess up the results of
# the test suite.
# We want to prevent setlocale(LC_ALL, "") failing:
unset LANG

# We assume that all the files we want to work on are in the 
# current directory.
unset PROJECTDIR


I 2
setup
E 2
remove command.log log log.stdout log.stderr SCCS
mkdir SCCS 2>/dev/null

g=tfile 
s=SCCS/s.${g} 
p=SCCS/p.${g} 
x=SCCS/x.${g} 
z=SCCS/z.${g}
remove $s $p $g $x $z

echo "Using the driver program ${sccs}"


# Create the input file.
cat > $g <<EOF
%M%: This is a test file containing nothing interesting.
EOF

#
# Creating the s-file. 
#
# Create the s-file the traditional way...
docommand a1 "${vg_sccs} admin -i$g $s" 0 \
    ""                                              IGNORE
docommand a2 "test -f $s" 0 "" ""
remove $s

docommand a3 "${vg_sccs} enter $g" 0 \
    "\n$g:\n"                                        IGNORE
docommand a4 "test -f $s"  0 "" ""

# Check the backup file still exists.
docommand a5 "test -f ,$g" 0 "" ""
remove ,$g

#
# Making deltas.
#

# First the traditional way.
docommand b1 "${vg_sccs} get -e $s" 0 \
    "1.1\nnew delta 1.2\n1 lines\n"                 IGNORE

echo "hello" >>$g
docommand b2 "${vg_sccs} delta -y\"\" $s" 0 \
    "1.2\n1 inserted\n0 deleted\n1 unchanged\n"     IGNORE


# Now with edit and delget.    
docommand b3 "${vg_sccs} edit $s"  0 \
    "1.2\nnew delta 1.3\n2 lines\n"                 IGNORE


echo "there" >>$g
docommand b4 "${vg_sccs} deledit -y'' $s" IGNORE \
 "1.3\n1 inserted\n0 deleted\n2 unchanged\n1.3\nnew delta 1.4\n" \
 IGNORE
# g-file should now exist and be writable.
D 3
docommand b5 "test -w $g" 0 "" ""
E 3
I 3
#docommand b5 "test -w $g" 0 "" ""
docommand b5 "wtest -w $g" 0 "" ""
E 3


echo '%A%' >>$g
docommand b6 "${vg_sccs} delget -y'' $s" 0 \
 "1.4\n1 inserted\n0 deleted\n3 unchanged\n1.4\n4 lines\n" \
 IGNORE
# g-file should now exist but not be writable.
D 3
docommand b7 "test -w $g" 1 "" ""
E 3
I 3
#docommand b7 "test -w $g" 1 "" ""
docommand b7 "wtest -w $g" 1 "" ""
E 3
docommand b8 "test -f $g" 0 "" ""



#
# fix
#
docommand c1 "${vg_sccs} fix -r1.4 $s" 0 \
 "1.4\n4 lines\n1.3\nnew delta 1.4\n" \
 IGNORE

docommand c2 "${vg_sccs} tell" 0 "tfile\n" ""

docommand c3 "${vg_sccs} delget -y'' $s" 0 \
 "1.4\n1 inserted\n0 deleted\n3 unchanged\n1.4\n4 lines\n" \
 IGNORE


#
# rmdel
#
# Make sure rmdel on its own works OK.
docommand d1 "${vg_sccs} rmdel -r1.4 $s" 0 "" ""

# Make sure that revision is not still present.
docommand d2 "${vg_sccs} get -p -r1.4 $s" 1 "" IGNORE

# Make sure that previous revision is still present.
docommand d3 "${vg_sccs} get -p -r1.3 $s" 0 IGNORE "1.3\n3 lines\n"


#
# what
#
docommand e1 "${vg_sccs} what $g" 0 "${g}:\n\t ${g} 1.4@(#)\n" ""


# 
# enter
# 
remove "foo" ",foo" "SCCS/s.foo"
echo "%Z%" >foo
docommand f1 "test -f ,foo" 1 "" ""
docommand f2 "${vg_sccs} enter foo" 0 "\nfoo:\n" ""
docommand f3 "test -f ,foo" 0 "" ""
docommand f4 "test -f SCCS/s.foo" 0 "" ""
remove ",foo"

#
# clean
# 
docommand g1 "${vg_sccs} edit SCCS/s.foo" 0 \
				    "1.1\nnew delta 1.2\n1 lines\n" ""

# Make sure foo and tfile exist but only foo is writable.
docommand g2 "test -f foo"   0 "" ""
docommand g3 "test -f tfile" 0 "" ""
D 3
docommand g4 "test -w foo"   0 "" ""
docommand g5 "test -w tfile" 1 "" ""
E 3
I 3
#docommand g4 "test -w foo"   0 "" ""
#docommand g5 "test -w tfile" 1 "" ""
docommand g4 "wtest -w foo"   0 "" ""
docommand g5 "wtest -w tfile" 1 "" ""
E 3
docommand g6 "${vg_sccs} clean" 0 IGNORE ""
# Make sure tfile is now gone and foo is not.
docommand g7 "test -f tfile" 1 "" ""
docommand g8 "test -f foo"   0 "" ""
D 3
docommand g9 "test -w foo"   0 "" ""
E 3
I 3
#docommand g9 "test -w foo"   0 "" ""
docommand g9 "wtest -w foo"   0 "" ""
E 3

#
# unedit 
#

case `uname -s 2>/dev/null` in
    CYGWIN*)
	echo Skipping test h1 under CYGWIN, see docs/Platforms for explanation
	echo "(we still perform step h1 because of its effects however)"
	docommand h1_cygwin "${vg_sccs} unedit foo" 0 IGNORE ""
	;;
	*)
	docommand h1 "${vg_sccs} unedit foo" 0 \
		"         foo: removed\n1.1\n1 lines\n" ""
		# That's 9 spaces.
	;;
esac


# the g-file should have been removed.
# actually we don't pass this test, see docs/BUGS.
# FIXME TODO
#docommand h2 "test -f foo" 1 IGNORE IGNORE

#
# info
#
docommand i1 "${vg_sccs} info -b" 0 "Nothing being edited (on trunk)\n" ""
docommand i2 "${vg_sccs} info"    0 "Nothing being edited\n" ""
remove SCCS/s.foo foo


#
# check
#
docommand j1 "${vg_sccs} check" 0 "" ""
docommand j2 "${vg_sccs} edit $s" 0 IGNORE IGNORE
docommand j3 "${vg_sccs} check" 1 IGNORE ""
docommand j4 "${vg_sccs} unedit $g" 0 IGNORE IGNORE



remove {expected,got}.std{out,err} last.command 
remove $s $p $g $x $z
I 2
rmdir SCCS
restore
E 2
success

#
# Still need to test:-

# cdc, comb, help, prs, prt, val, sccsdiff, diffs, -diff,
# branch, create

#
# Tests that would need a canned SCCS file:-
#
# print, info

	    
docommand B6 "${vg_get} -e $s" 0 \
    "1.3\nnew delta 1.4\n2 lines\n"                 IGNORE
cp test/passwd.4 passwd
docommand B7 "${vg_delta} -y'' $s" 0 \
    "1.4\n1 inserted\n1 deleted\n1 unchanged\n"     IGNORE
docommand B8 "${vg_get} -e $s" 0 \
    "1.4\nnew delta 1.5\n2 lines\n"                 IGNORE
cp test/passwd.5 passwd
docommand B9 "${vg_delta} -y'' $s" 0 \
    "1.5\n1 inserted\n1 deleted\n1 unchanged\n"     IGNORE
docommand B10 "${vg_get} -e -r1.3 $s" 0 \
    "1.3\nnew delta 1.3.1.1\n2 lines\n"             IGNORE
cp test/passwd.6 passwd
docommand B11 "${vg_delta} -y'' $s" 0 \
    "1.3.1.1\n1 inserted\n1 deleted\n1 unchanged\n" IGNORE

rm -rf test
D 2
remove passwd command.log $s $g $x $z $p SCCS
rm -rf SCCS
E 2
I 2
remove passwd command.log $s $g $x $z $p
rmdir SCCS
restore
E 2
success
E 1
