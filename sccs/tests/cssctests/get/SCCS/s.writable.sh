h04040
s 00072/00000/00000
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
# writable.sh:  Will get over-write a writable file?

# Import common functions & definitions.
. ../common/test-common

# You cannot run the test suite as root.
. ../common/not-root

remove command.log log log.stdout log.stderr

f=wrtest
gfile=_g.$f
remove s.$f

# Generate empty file.
: > $f

# Create the s. file and make sure it exists.
docommand W1 "$admin -n -i$f s.$f" 0 "" IGNORE

test -r s.$f         || fail admin did not create s.$f
remove $f
echo foo > $f
chmod +w $f

# Try running get when gfile was writable -- it should fail.
docommand W2 "${vg_get} s.$f" 1 IGNORE IGNORE
remove $gfile
test -f $gfile	    && miscarry could not remove _g.$f

# Now run get with the -G option and it should work even
# though the file's usual name is occupied by a writable file.
docommand W3 "${vg_get} -G$gfile s.$f" 0 "1.1\n0 lines\n" IGNORE


# If you specify the "-k" option, the gotten file should be read-write.
# If you don't specify -k or -e, it will be read-only.  -e implies -k.
remove $gfile $f
docommand W4 "${vg_get} s.$f" 0 "1.1\n0 lines\n" IGNORE

# Make sure the file is read only.
echo_nonl "W5..."
if test -w $f 
then
    fail W5: "get s.$f created writable $f"
fi
echo passed



# Now do the same again, using the -k option.
remove $gfile $f
docommand W6 "${vg_get} -k s.$f" 0 "1.1\n0 lines\n" IGNORE

# Make sure the file is read only.
echo_nonl "W7..."
if test -w $f 
then
    true
else
    fail W5: "get -k s.$f created read-only $f.  It should be writable"
fi
echo passed





remove $f s.$f $gfile
remove command.log
success
E 1
