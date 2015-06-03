#! /bin/sh
# p-option.sh:  Testing for the -p option of "delta"

# Import common functions & definitions.
. ../../common/test-common

g=foo
s=s.$g
files="$s $g p.$g z.$g"
remove $files

# Create an SCCS file.
docommand p1 "${admin} -n $s"    0 "" IGNORE

# Check the file out for editing.
docommand p2 "${get} -e $s"      0 IGNORE IGNORE

# Append a line
echo "hello" >> $g
docommand p3 "cat $g" 0 "hello\n" ""

# Check the file back in with delta, using the -n option.
docommand p4 "${delta} -p -y $s" 0 \
"1.2
0a1
> hello
1 inserted
0 deleted
0 unchanged
" IGNORE


# Check the file out for editing again
docommand p5 "${get} -e $s"      0 IGNORE IGNORE

# Change the line.
echo "test" > $g
docommand p6 "cat $g" 0 "test\n" ""

docommand p7 "${delta} -p -y $s" 0 \
"1.3
1c1
< hello
---
> test
1 inserted
1 deleted
0 unchanged
" IGNORE


# Delete the (only) line
docommand p8 "${get} -e $s"      0 IGNORE IGNORE
true > $g
docommand p9 "cat $g" 0 "" ""

# Check the file back in with delta, using the -n option.
docommand p10 "${delta} -p -y $s" 0 \
"1.4
1d0
< test
0 inserted
1 deleted
0 unchanged
" IGNORE

remove $files
success
