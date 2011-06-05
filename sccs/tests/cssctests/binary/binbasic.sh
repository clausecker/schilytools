#! /bin/sh
# binbasic.sh:  Testing for the basic operation of "delta", for binary files.

# Import common functions & definitions.
. ../common/test-common
. ../common/real-thing
. ../common/config-data

if $binary_support
then
    true
else
    echo "Skipping these tests -- no binary file support."
    exit 0
fi 

g=passwd
p=test/p.$g
s=test/s.$g
x=test/x.$g
z=test/z.$g


adminflags="-b"


cleanup() {
   remove command.log log log.stdout log.stderr
   remove test/passwd.[123456]
   remove $p $x $s $z
   remove passwd command.log last.command
   remove got.stdout expected.stdout got.stderr expected.stderr
   rm -rf test
}

do_delta() {
   n=$1
   shift
   docommand d${n}a "${vg_get} -e $s" 0 IGNORE IGNORE
   cp test/passwd.${n} passwd
   docommand d${n}b "${vg_delta} -y\"\" $s" 0 IGNORE IGNORE
   
   remove gotten
   rev=-r1.`expr $n + 1`
   docommand d${n}c "${get} ${rev} -Ggotten $s" 0 IGNORE IGNORE
   
   # Find any differences between the file we used as the 
   # source for that delta, and the version we just extracted.
   docommand d${n}d "${DIFF} gotten test/passwd.$n" 0 "" IGNORE
   remove gotten
}

cleanup

echo_nonl prepare1...
mkdir test 2>/dev/null

# Create the input files.
cat > base <<EOF
This is a test file containing nothing interesting.
EOF
for i in 1 2 3 4 5 6
do 
    cat base                       > test/$g.$i
    echo "This is file number" $i >> test/$g.$i
done 
remove base $g test/[xzps].passwd
echo passed


## Find a binary file 
BINARY_FILE=../prt/all.expected.Z

# Make sure we have some real binary input!
echo_nonl prepare2...
if test -r ${DIFF}; then cat ${DIFF} >> test/$g.4; fi
if test -r ${BINARY_FILE}; then cat ${BINARY_FILE} >> test/$g.5; fi
for f in /bin/sh*
do
    if test -r $f; then cat $f >> test/$g.6; fi
done
echo passed

#
# Create an SCCS file with several branches to work on.
# We generally ignore stderr output since we produce "Warning: no id keywords"
# more often than "real" SCCS.
#
docommand B1 "${admin} ${adminflags} -itest/passwd.1 $s" 0 "" IGNORE

for n in 1 2 3
do
    do_delta $n
done

# Binary support not fully working yet.
for n in 4 5 6
do
    do_delta $n
done

cleanup
success
