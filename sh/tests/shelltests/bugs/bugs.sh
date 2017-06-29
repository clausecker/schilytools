#! /bin/sh
#
# @(#)bugs.sh	1.1 17/06/28 2017 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check for regression bugs
#

#
# A bug from March 2017 caused the shell to clobber the strings "/dev/null"
# when in -x mode. The reason was: _macro() does not reset staktop and thus
# the next _macro() call will includ ethe previous expansion that was $PS4.
#
cat > x <<"XEOF"
: ${ECHO=/usr/bin/does-not-exist}
$ECHO 'bla' < /dev/null > /dev/null 2> /dev/null || ECHO=/bin/does-not-exist
$ECHO 'bla' < /dev/null > /dev/null 2> /dev/null || ECHO=echo
XEOF

docommand -noremove bug01 "LC_ALL=C $SHELL -x ./x" 0 IGNORE NONEMPTY

#
# Bash adds stray spaces in the output :-(
# We need to check with diff -w
#
cat > expected <<"XEOF"
+ : /usr/bin/does-not-exist 
+ /usr/bin/does-not-exist bla 
+ ECHO=/bin/does-not-exist
+ /bin/does-not-exist bla 
+ ECHO=echo
XEOF
diff -w expected got.stderr
if [ $? != 0 ]; then
	fail "Test $cmd_label failed: wrong error message"
fi
remove x expected
do_remove

#
# A bug from June 2017 was triggered on Haiku because Haiku does not support
# hard links. As a result, the mamagement of the shell /tmp/ files failed.
# Bosh now copies the /tmp/ files in case that link() failes with other
# than EEXIST
#
cat > x <<"XEOF"
echo `(cat | cat) << EOF
1
2
3
EOF
`
XEOF
docommand bug02 "$SHELL ./x" 0 "1 2 3\n" ""

remove x

success
