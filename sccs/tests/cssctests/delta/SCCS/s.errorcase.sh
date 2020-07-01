hV6,sum=12673
s 00007/00001/00209
d D 1.7 2019/05/12 14:12:21+0200 joerg 7 6
S s 33971
c prs -D:DI: unterscheidet nun fuer non-POSIX Varianten
c Kommentar verbessert
e
s 00001/00001/00209
d D 1.6 2018/04/30 13:06:44+0200 joerg 6 5
S s 09063
c Quoten, demit Username mit SPACE funktioniert
e
s 00002/00002/00208
d D 1.5 2015/06/03 00:06:44+0200 joerg 5 4
S s 08985
c ../common/test-common -> ../../common/test-common
e
s 00003/00003/00207
d D 1.4 2015/06/01 23:55:23+0200 joerg 4 3
S s 08707
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00004/00003/00206
d D 1.3 2011/06/15 23:36:37+0200 joerg 3 2
S s 05050
c Test ob chmod 0 eine Datei unlesbar macht
e
s 00004/00000/00205
d D 1.2 2011/05/31 22:43:36+0200 joerg 2 1
S s 01029
c if [ ".$CYGWIN" = '.' ]; then um Tests mit chmod 0 p.foo / s.foo
e
s 00205/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 56641
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y
G r 0e46e8ec3ef99
G p sccs/tests/cssctests/delta/errorcase.sh
t
T
I 1
#! /bin/sh
# errorcase.sh:  Testing for the various error cases for "delta".

# Import common functions & definitions.
D 5
. ../common/test-common
. ../common/real-thing
E 5
I 5
. ../../common/test-common
. ../../common/real-thing
E 5

I 7
# $TESTING_SCCS_V5	Test SCCSv5 features from SunOS
# $TESTING_CSSC		Relict from CSSC tests, also applies to SCCS
# $TESTING_REAL_CSSC	Test real CSSC 4-digit year extensions
# $TESTING_REAL_SCCS	Test real Schily SCCS 4 digit year extensions
# $TESTING_SCCS_V6	Test SCCSv6 features

E 7
remove command.log log log.stdout log.stderr
mkdir test 2>/dev/null

g=foo
s=s.$g
p=p.$g
z=z.$g
q=q.$g
d=d.$g
x=x.$g
files="$g $s $p $z ${g}_1 ${g}_2 $g $q $d $x $g.saved $g.old"

remove $files
test -d $x.bak && rmdir $x.bak

append() {
   f="$1"
   shift
   echo  "$@" >> "$f" || miscarry "Could not append a line to $1" 
}


createfile () {
    touch "$1" && test -r "$1" || miscarry "could not create file $1"
}

removedirs () {
    for d
    do
      if test -d "$d"
      then
	  rmdir "$d" || miscarry "Failed to remove directory $d"
      else 
	  if test -f "$d"
	  then
	      remove "$d"
          fi
      fi
    done
}


D 4
if wrong_group=`../../testutils/user foreigngroup`
E 4
I 4
if wrong_group=`${SRCROOT}/tests/testutils/user foreigngroup`
E 4
then
    true
else
    miscarry "could not select the name of a group to which you do not belong"
fi
# echo "You do not belong to group number" $wrong_group


# Create the SCCS file - and make sure that delta can be made to work at all.
docommand E1 "${admin} -n $s" 0 IGNORE IGNORE 
docommand E2 "${get} -e $s"   0 IGNORE IGNORE 
append $g "test data"
docommand E3 "${vg_delta} -yNoComment $s"   0 IGNORE IGNORE 

# Now set up the authorised groups list.   
docommand E4 "${admin} -a${wrong_group} $s" 0 IGNORE IGNORE

# cannot do get -e if you are not in the authorised user list.
docommand E5 "${get} -e $s"   1 IGNORE IGNORE 

# Momentarily zap the authorised user list so that "get -e" works.
docommand E6 "${admin} -e${wrong_group} $s" 0 IGNORE IGNORE
docommand E7 "${get} -e $s"   0 IGNORE IGNORE
docommand E8 "${admin} -a${wrong_group} $s" 0 IGNORE IGNORE

append $g "more test data"

# delta should still fail if we are not in the authorised user list
# (in other words the list is checked both by get -e and delta).
docommand E9 "${vg_delta} -yNoComment $s"   1 IGNORE IGNORE

# Remove the authorised group list; check-in should now work
docommand E10 "${admin} -e${wrong_group} $s" 0 IGNORE IGNORE
docommand E11 "${vg_delta} -yNoComment $s"   0 IGNORE IGNORE 


# Now, what if the authorised user list just excludes?
remove $s
D 4
if mygroup=`../../testutils/user group`
E 4
I 4
if mygroup=`${SRCROOT}/tests/testutils/user group`
E 4
then
    true
else
    miscarry "could not determine group-id"
fi

D 4
if myname=`../../testutils/user name`
E 4
I 4
if myname=`${SRCROOT}/tests/testutils/user name`
E 4
then
    true
else
    miscarry "could not determine user name"
fi

# Regular SCCS does not underatand the use of "!username" 
# to specifically exclude users.  Hence for compatibility 
# nor must we.
docommand E12 "${admin} -n $s"              0 IGNORE IGNORE 
docommand E13 "${admin} -a${mygroup} $s"    0 IGNORE IGNORE
D 6
docommand E14 "${admin} -a\!${myname} $s"   0 IGNORE IGNORE
E 6
I 6
docommand E14 "${admin} -a\!'${myname}' $s"   0 IGNORE IGNORE
E 6
docommand E15 "${get} -e $s"                0 IGNORE IGNORE
# this means that the above tests should succeed.

# Check - use of delta when a q-file already exists...
createfile $q
docommand E16 "${vg_delta} -yNoComment $s" 1 IGNORE IGNORE
remove $q

# Unreadable g-file should also cause a failure. 
I 2
D 3
if [ ".$CYGWIN" = '.' ]; then
E 3
E 2
chmod 0 $g
D 3
docommand E17 "${vg_delta} -yNoComment $s" 1 IGNORE IGNORE
E 3
I 3
cat $g > /dev/null 2> /dev/null
if [ $? -ne 0 ]; then
	docommand E17 "${vg_delta} -yNoComment $s" 1 IGNORE IGNORE
E 3
I 2
else
D 3
	echo "Your OS or your the filesystem do not support chmod 0 - some tests skipped"
E 3
I 3
	echo "Your permissions on your OS do not support chmod 0 to remove read permission - test E17 skipped"
E 3
fi
E 2
chmod +r $g
docommand E18 "${vg_delta} -yNoComment $s" 0 IGNORE IGNORE


# Failure to create the d-file should NOT cause a failure. 
docommand E19 "${get} -e $s"                0 IGNORE IGNORE
remove $x
createfile $d
docommand E20 "${vg_delta} -yNoComment $s" 0 IGNORE IGNORE

# This should not leave any other temporary file lying about
# but it should also not delete the s-file
docommand E22 "test -r $x" 1 "" ""
docommand E23 "test -r $q" 1 "" ""

# The d-file would have been deleted (without causing an error) in E20.
# Since there was no error the g-file should no longer be there either.
docommand E24 "test -r $d" 1 "" ""
docommand E25 "test -r $s" 0 "" "" 
docommand E26 "test -w $g" 1 "" "" 

# Since E20 was successful, no need to do the delta again
#remove $d
#docommand E27 "${vg_delta} -yNoComment $s" 0 IGNORE IGNORE

# %A as the last two characters of the file to be checked in 
# should not cause the world to end. 
remove $s
D 7
if ${TESTING_CSSC}
E 7
I 7
if ${TESTING_REAL_CSSC} || ${TESTING_REAL_SCCS}
E 7
then
    docommand E28 "${admin} -b -n $s" 0 IGNORE IGNORE 
    
    docommand E29 "${get} -e $s"                0 IGNORE IGNORE
    echo_nonl "%A" > $g
    cp $g $g.saved || miscarry "could not back up $g"
    docommand E30 "${vg_delta} -yNoComment $s" 0 IGNORE IGNORE
    docommand E31 "${get} -k $s"                0 IGNORE IGNORE
    
    echo_nonl E32...
    if diff $g.saved $g 
    then
        echo passed
    else
        fail "E32: Expcected to get the same contents back, did we did not" 
    fi
    remove $g


    # Now tests for not being able to rename an existing x-file.  This 
    # is not an error - we just overwrite the original x-file as 
    # SCCS does, rather than backing it up. 
    # This test is specific to CSSC because SCCS doesn't rename the x-file...
    mkdir $x.bak
    createfile $x
    docommand E33 "${get} -e $s"                0 IGNORE IGNORE
    docommand E34 "${vg_delta} -yNoComment $s"     0 IGNORE IGNORE
    removedirs $x.bak

else
    echo "(Some tests skipped - we are not sure if ${admin} has binary file support)"
fi 
remove $s


# Test for the case where the p-file lists a SID which is not in the 
# SCCS file. 

# Create deltas 1.1 and 1.2
docommand E35 "${admin} -n $s"     0 IGNORE IGNORE
docommand E36 "${get} -e $s"       0 IGNORE IGNORE
docommand E37 "${vg_delta} -yNoComment $s" 0 IGNORE IGNORE

# Edit delta 1.2 and then remove delta 1.2
docommand E38 "${get} -e $s"       0 IGNORE IGNORE
rename $p saved.$p
docommand E39 "${rmdel} -r1.2 $s" 0 IGNORE IGNORE
rename saved.$p $p

# Try to check in the file - this should fail. 
docommand E40 "${delta} -yNoComment $s" 1 "" IGNORE
remove $p

remove $files


success
E 1
