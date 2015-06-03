h09951
s 00001/00001/00057
d D 1.3 15/06/03 00:06:44 joerg 3 2
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00057
d D 1.2 11/10/21 23:07:38 joerg 2 1
c prs -d:DI: Tests sind nun POSIX konform
e
s 00058/00000/00000
d D 1.1 10/05/11 11:30:00 joerg 1 0
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
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
E 3

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
docommand n7 "${prs} -r2.1 '-d:C:|:DI:|:DP:|:DS:|:DT:' $s" 0 \
D 2
  'AUTO NULL DELTA\n||1|2|D\n' IGNORE
E 2
I 2
  'AUTO NULL DELTA\n|//|1|2|D\n' IGNORE
E 2

###
### Cleanup and exit.
###
rm -rf test 
remove foo $s $g $p [zx].$g command.log

success
E 1
