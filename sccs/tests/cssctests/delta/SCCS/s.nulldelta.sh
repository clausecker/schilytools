hV6,sum=16146
s 00004/00000/00072
d D 1.5 2019/05/12 14:28:12+0200 joerg 5 4
S s 23412
c Neuer Hinweis: :DI: Bug wurde 1984 surch AT&T eingefuehrt
e
s 00014/00000/00058
d D 1.4 2019/05/12 14:12:22+0200 joerg 4 3
S s 16962
c prs -D:DI: unterscheidet nun fuer non-POSIX Varianten
c Kommentar verbessert
e
s 00001/00001/00057
d D 1.3 2015/06/03 00:06:44+0200 joerg 3 2
S s 46948
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00057
d D 1.2 2011/10/21 23:07:38+0200 joerg 2 1
S s 46809
c prs -d:DI: Tests sind nun POSIX konform
e
s 00058/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 46715
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ec70f71
G p sccs/tests/cssctests/delta/nulldelta.sh
t
T
I 1
#! /bin/sh

# flags.sh:  Testing for null deltas.

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
I 4
. ../../common/real-thing
E 4
E 3

I 4
# $TESTING_SCCS_V5	Test SCCSv5 features from SunOS
# $TESTING_CSSC		Relict from CSSC tests, also applies to SCCS
# $TESTING_REAL_CSSC	Test real CSSC 4-digit year extensions
# $TESTING_REAL_SCCS	Test real Schily SCCS 4 digit year extensions
# $TESTING_SCCS_V6	Test SCCSv6 features

E 4
g=new.txt
s=s.$g
p=p.$g
remove foo $s $g $p [zx].$g

###
### Tests for the 'v' flag; see also init-mrs.sh.
###

# Create SCCS file with a substituted keyword.
echo '%M%' >foo


###
### Tests for the n flag.
### 
# Create an SCCS file with the "n" flag turned on.
docommand n1 "${admin} -ifoo $s" 0 "" ""
docommand n2 "${admin} -fn $s" 0 "" ""

# Skip a release (2)
docommand n3 "${get} -e -r4 $s" 0 "1.1\nnew delta 4.1\n1 lines\n" \
            IGNORE
echo "hello" >> $g
docommand n4 "${vg_delta} -y $s" 0 "4.1\n1 inserted\n0 deleted\n1 unchanged\n" ""



# Check that a null delta was made for release 2, at all.
docommand n5 "${prs} -r2.1 -d:I: $s" 0 "2.1\n" IGNORE

# Check that a null delta was made for release 3, at all.
docommand n6 "${prs} -r3.1 -d:I: $s" 0 "3.1\n" IGNORE

## TODO: also ceck the deltas are in the right order.

# Check some details about that release.  The comment is 
# "AUTO NULL DELTA", no deltas were included or excluded;
# one delta was ignored; the predecessor sequence number must be 1; 
# the sequence number of this delta must be 2, and the type must be 'D',
# that is, a normal delta.
I 4

if $TESTING_REAL_SCCS
then
E 4
docommand n7 "${prs} -r2.1 '-d:C:|:DI:|:DP:|:DS:|:DT:' $s" 0 \
D 2
  'AUTO NULL DELTA\n||1|2|D\n' IGNORE
E 2
I 2
  'AUTO NULL DELTA\n|//|1|2|D\n' IGNORE
I 4
else
I 5
#
# Not POSIX compliant for -d:DI
# This applies to AT&T SCCS past 1984 and to CSSC
#
E 5
docommand n7-non-POSIX "${prs} -r2.1 '-d:C:|:DI:|:DP:|:DS:|:DT:' $s" 0 \
  'AUTO NULL DELTA\n||1|2|D\n' IGNORE
fi
E 4
E 2

###
### Cleanup and exit.
###
rm -rf test 
remove foo $s $g $p [zx].$g command.log

success
E 1
