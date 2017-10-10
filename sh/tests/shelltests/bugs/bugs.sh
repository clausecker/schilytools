#! /bin/sh
#
# @(#)bugs.sh	1.3 17/10/04 2017 J. Schilling
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
if $is_bourne || $is_osh; then
cat > expected <<"XEOF"
+ : /usr/bin/does-not-exist 
+ /usr/bin/does-not-exist bla 
ECHO=/bin/does-not-exist
+ /bin/does-not-exist bla 
ECHO=echo
XEOF
else
cat > expected <<"XEOF"
+ : /usr/bin/does-not-exist 
+ /usr/bin/does-not-exist bla 
+ ECHO=/bin/does-not-exist
+ /bin/does-not-exist bla 
+ ECHO=echo
XEOF
fi
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

#
# The next 8 tests check whether $? is affected by shared memory from vfork()
# that has no workaround. The problem and the tests have been reported by
# Martijn Dekker for MacOS and FreeBSD. The way we test here also hits the
# related problem on Solaris.
#
docommand bug03 "$SHELL -c 'expr a \">\" b >/dev/null; echo \$?'" 0 "1\n" ""
docommand bug04 "$SHELL -c 'command expr a \">\" b >/dev/null; echo \$?'" 0 "1\n" ""

docommand bug05 "$SHELL -c 'PATH=\$PATH:/usr/bin:/bin expr a \">\" b >/dev/null; echo \$?'" 0 "1\n" ""
docommand bug06 "$SHELL -c 'PATH=\$PATH:/usr/bin:/bin command expr a \">\" b >/dev/null; echo \$?'" 0 "1\n" ""

remove b

docommand -noremove bug07 "$SHELL -c 'expr  2>/dev/null; echo \$?'" 0 NONEMPTY ""
expr `cat got.stdout` ">=" 2 > /dev/null
if [ $? -ne 0 ]; then
	fail "Test $cmd_label failed: wrong exit code"
fi
docommand -noremove bug08 "$SHELL -c 'command expr  2>/dev/null; echo \$?'" 0 NONEMPTY ""
expr `cat got.stdout` ">=" 2 > /dev/null
if [ $? -ne 0 ]; then
	fail "Test $cmd_label failed: wrong exit code"
fi

docommand -noremove bug09 "$SHELL -c 'PATH=\$PATH:/usr/bin:/bin expr  2>/dev/null; echo \$?'" 0 NONEMPTY ""
expr `cat got.stdout` ">=" 2 > /dev/null
if [ $? -ne 0 ]; then
	fail "Test $cmd_label failed: wrong exit code"
fi
docommand -noremove bug10 "$SHELL -c 'PATH=\$PATH:/usr/bin:/bin command expr  2>/dev/null; echo \$?'" 0 NONEMPTY ""
expr `cat got.stdout` ">=" 2 > /dev/null
if [ $? -ne 0 ]; then
	fail "Test $cmd_label failed: wrong exit code"
fi


success
