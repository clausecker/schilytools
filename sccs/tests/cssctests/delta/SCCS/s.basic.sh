h00718
s 00144/00000/00000
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
# basic.sh:  Testing for the basic operation of "delta".

# Import common functions & definitions.
. ../common/test-common

remove command.log log log.stdout log.stderr
mkdir test 2>/dev/null

# Create the input files.
cat > base <<EOF
This is a test file containing nothing interesting.
EOF
for i in 1 2 3 4 5 6
do 
    cat base                       > test/passwd.$i
    echo "This is file number" $i >> test/passwd.$i
done 

s=test/s.passwd

remove base test/[xz].*
remove test/[spx].passwd
remove passwd

#
# Create an SCCS file with several branches to work on.
# We generally ignore stderr output since we produce "Warning: no id keywords"
# more often than "real" SCCS.
#
docommand B1 "${admin} -itest/passwd.1 $s" 0 \
    ""                                              IGNORE
docommand B2 "${get} -e $s" 0 \
    "1.1\nnew delta 1.2\n2 lines\n"                 IGNORE
cp test/passwd.2 passwd
docommand B3 "${vg_delta} -y\"\" $s" 0 \
    "1.2\n1 inserted\n1 deleted\n1 unchanged\n"     IGNORE
docommand B4 "${get} -e $s"  0 \
    "1.2\nnew delta 1.3\n2 lines\n"                 IGNORE
cp test/passwd.3 passwd
docommand B5 "${vg_delta} -y'' $s" 0 \
    "1.3\n1 inserted\n1 deleted\n1 unchanged\n"     IGNORE
docommand B6 "${get} -e $s" 0 \
    "1.3\nnew delta 1.4\n2 lines\n"                 IGNORE
cp test/passwd.4 passwd
docommand B7 "${vg_delta} -y'' $s" 0 \
    "1.4\n1 inserted\n1 deleted\n1 unchanged\n"     IGNORE
docommand B8 "${get} -e $s" 0 \
    "1.4\nnew delta 1.5\n2 lines\n"                 IGNORE
cp test/passwd.5 passwd
docommand B9 "${vg_delta} -y'' $s" 0 \
    "1.5\n1 inserted\n1 deleted\n1 unchanged\n"     IGNORE
docommand B10 "${get} -e -r1.3 $s" 0 \
    "1.3\nnew delta 1.3.1.1\n2 lines\n"             IGNORE
cp test/passwd.6 passwd
docommand B11 "${vg_delta} -y'' $s" 0 \
    "1.3.1.1\n1 inserted\n1 deleted\n1 unchanged\n" IGNORE

remove passwd command.log $s

###
### Other stuff
### 
g=foo; for n in p z x s q; do eval $n=$n.${g}; done
files="$s $p $z $x $s $q"

remove $files 
cat > $g <<EOF
first line
second line
third line
fourth line
fifth line
sixth line
seventh line
eighth line
ninth line
EOF

docommand C1  "${admin} -i${g} $s" 0 IGNORE IGNORE
remove $g
docommand C2  "${get} -e $s" 0 IGNORE IGNORE

rename ${g} ${g}.old 
sed -e '2,4 d' < ${g}.old > $g || miscarry "sed failed"

docommand C3  "${vg_delta} -y $s" 0 IGNORE IGNORE
docommand C4  "${get} -p $s" 0 "first line
fifth line
sixth line
seventh line
eighth line
ninth line
" IGNORE

docommand C5  "${get} -e $s" 0 IGNORE IGNORE
rename ${g} ${g}.old 
sed -e '2,4 d' < ${g}.old > $g || miscarry "sed failed"
docommand C6  "${vg_delta} -y $s" 0 IGNORE IGNORE
docommand C7  "${get} -p $s" 0 "first line
eighth line
ninth line
" IGNORE

# If we try to do a delta again, it should fail because we have no 
# outstanding edits - that is, there is no p-file.
docommand C8  "${vg_delta} -y $s" 1 IGNORE IGNORE


# Now we try checking in a SID which we do not have checked out.
docommand C9   "${get} -e $s" 0 IGNORE IGNORE
docommand C10  "${vg_delta} -y -r1.1 $s" 1 IGNORE IGNORE
remove $p $g


# ... and checking in a SID which is in the p-file but not the s-file...
docommand C11   "${get} -e -r1.3 $s" 0 IGNORE IGNORE
rename ${p} ${p}.old 
( sed -e 's/1\.3/3.1/' < ${p}.old | sed -e 's/1\.4/3.2/' > $p ) || miscarry "sed failed"
remove ${p}.old
docommand C12  "${vg_delta} -y -r1.1 $s" 1 IGNORE IGNORE
remove $g


# If two edits are outstanding, it is an error not to use the "-r" option
docommand C13   "${get} -e -r1.2 $s" 0 IGNORE IGNORE
docommand C14  "${vg_delta} -y $s" 1 IGNORE IGNORE

# It is also an error to specify a SID which is not being edited.
docommand C15  "${vg_delta} -y -r1.3 $s" 1 IGNORE IGNORE

# It is an error to check in a file containing no SCCS keywords when the 
# "i" flag is set. 
docommand C16  "${admin} -fi $s" 0 "" IGNORE
docommand C17  "${vg_delta} -y -r1.2 $s" 1 "" IGNORE

# ... but if we turn that flag off again, it should work fine.
docommand C18  "${admin} -di $s" 0 "" IGNORE
docommand C19  "${vg_delta} -y -r1.2 $s" 0 IGNORE IGNORE


rm -rf test
remove passwd command.log $files
success
E 1
