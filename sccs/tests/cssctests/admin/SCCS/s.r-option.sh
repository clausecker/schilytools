h18003
s 00004/00003/00068
d D 1.2 11/05/30 01:19:38 joerg 2 1
c Tests ob admin -ifoo s.foo und admin -n s.foo beide mit -r2 funktionieren
e
s 00071/00000/00000
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

# r-option:  Testing for the -r option of admin.

# Import common functions & definitions.
. ../common/test-common

# Import function which tells us if we're testing CSSC, or something else.
. ../common/real-thing

g=new.txt
s=s.$g


# Create an empty SCCS file to work on.
remove $g $s
echo "%M%" > $g
docommand R1 "${admin} -i$g $s" 0 "" ""

# Make sure it really is ID 1.1.
docommand R2 "${prs} -d:I: $s" 0 "1.1\n" ""


# Create an empty SCCS file to work on, with initial SID 2.1.
remove $g $s
echo "%M%" > $g
docommand R3 "${vg_admin} -i$g -r2 $s" 0 "" ""

# Make sure it really is ID 2.1.
docommand R4 "${prs} -d:I: $s" 0 "2.1\n" ""


##
## Some implementations of SCCS don't allow (e.g.) -r1.2,
## so if we're not running agains CSSC, we skip the 
## tests that deal with that kind of thing.
##

if $TESTING_CSSC
then
    # Create an empty SCCS file to work on.
    remove $g $s
    echo "%M%" > $g
    docommand t1 "${vg_admin} -i$g -r1.2 $s" 0 "" IGNORE
    
    # Make sure it really is ID 1.2.
    docommand t2 "${prs} -d:I: $s" 0 "1.2\n" ""
    
    
    # Now try a 4-component SID.
    remove $g $s
    echo "%M%" > $g
    docommand t3 "${vg_admin} -i$g -r1.2.2.1 $s" 0 "" IGNORE
    
    # Make sure it really is ID 1.2.
    docommand t4 "${prs} -d:I: $s" 0 "1.2.2.1\n" ""

    
D 2
    # The -r option must be accompanied by the -i option.
    # Using the -n option just isn't enough.
E 2
I 2
    # The -r option must be accompanied by the -i or -n option.
E 2
    remove $g $s
    echo "%M%" > $g
D 2
    docommand t5 "${admin} -n -r2 $s" 1 "" IGNORE
E 2
I 2
    docommand t5a "${admin} -i$g -r2 $s" 0 "" IGNORE
    remove $s
    docommand t5b "${admin} -n -r2 $s"   0 "" IGNORE
E 2
    
else
    echo Tests t1-t5 have been skipped
fi


remove $s $g
success
E 1
