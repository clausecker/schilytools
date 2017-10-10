#! /bin/sh
#
# @(#)alias.sh	1.9 17/10/04 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

if $is_bourne || $is_osh; then
	echo "Bourne Shell does not support aliases, skipping tests..."
	success
	exit
fi

#
# Basic tests to check whether aliases work
#
docommand al00 "$SHELL -c 'alias xxx=echo
xxx test'" 0 "test\n" ""

#
# The ideas from the following tests have been taken from the "mksh" test suite
# Thanks to Thorsten Glaser
#

#
# Check whether recursion is avoided in aliases.
#
cat > x <<"XEOF"
LC_ALL=C
alias fooBar=fooBar
fooBar
exit 0
XEOF
docommand -noremove al01 "$SHELL ./x" 0 "" IGNORE
err=`grep 'fooBar.*not found' got.stderr`
if [ -z "$err" ]; then
	fail "Test $cmd_label failed: wrong error message"
fi
remove x
do_remove

#
# Check whether complex recursion is avoided in aliases.
#
cat > x <<"XEOF"
LC_ALL=C
alias fooBar=barFoo
alias barFoo=fooBar
fooBar
exit 0
XEOF
docommand -noremove al02 "$SHELL ./x" 0 "" IGNORE
err=`grep 'oo.*not found' got.stderr`
if [ -z "$err" ]; then
	fail "Test $cmd_label failed: wrong error message"
fi
remove x
do_remove

#
# Check whether complex recursion is avoided in aliases.
#
cat > x <<"XEOF"
LC_ALL=C
alias fooBar=barFoo
alias barFoo=fooBar
barFoo
exit 0
XEOF
docommand -noremove al03 "$SHELL ./x" 0 "" IGNORE
err=`grep 'oo.*not found' got.stderr`
if [ -z "$err" ]; then
	fail "Test al03 failed: wrong error message"
fi
remove x
do_remove

#
# Check whether complex recursion is avoided in aliases and whether
# a trailing space works.
#
cat > x <<"XEOF"
LC_ALL=C
alias Echo="echo "
alias fooBar=barFoo
alias barFoo=fooBar
Echo fooBar
unalias barFoo
Echo fooBar
XEOF
docommand al04 "$SHELL ./x" 0 "fooBar\nbarFoo\n" ""
remove x

#
# Check whether alias expansion is not done for keywords at keyword position
#
cat > x <<"XEOF"
LC_ALL=C
alias Echo="echo "
alias while=While
while false; do echo hi ; done
Echo while
XEOF
docommand al05 "$SHELL ./x" 0 "While\n" ""
remove x

#
# Check whether alias expansion works with trailing space.
#
cat > x <<"XEOF"
LC_ALL=C
alias Echo="echo "
alias foo="bar stuff "
alias bar="Bar1 Bar2 "
alias stuff="Stuff"
alias blah="Blah"
Echo foo blah
XEOF
docommand al06 "$SHELL ./x" 0 "Bar1 Bar2 Stuff Blah\n" ""
remove x

#
# Check whether alias expansion works with trailing space.
#
cat > x <<"XEOF"
LC_ALL=C
alias Echo="echo "
alias foo="bar bar "
alias bar="Bar "
alias blah="Blah"
Echo foo blah
XEOF
docommand al07 "$SHELL ./x" 0 "Bar Bar Blah\n" ""
remove x

#
# Check whether alias expansion works with trailing space.
#
cat > x <<"XEOF"
LC_ALL=C
alias X="case "
alias Y=Z
X Y in "Y") echo is y ;; Z) echo is z ;; esac
XEOF
docommand al08 "$SHELL ./x" 0 "is z\n" ""
remove x

#
# Check whether newlines in an alias permit more than one command
#
cat > x <<"XEOF"
LC_ALL=C
alias foo="


echo hi



echo there 


"
foo 
XEOF
docommand al09 "$SHELL ./x" 0 "hi\nthere\n" ""
remove x

#
# Check that recursion is avoided and a command of the same name is called
#
cat > x <<"XEOF"
LC_ALL=C
echo "echo bla" > xxx
chmod +x xxx
pwd > /dev/null	# initialize $PWD
PATH=$PWD:$PATH
export PATH
alias xxx=xxx
xxx
echo = now
i=`xxx`
echo $i
echo = out
XEOF
docommand al10 "$SHELL ./x" 0 "bla\n= now\nbla\n= out\n" ""
remove x
remove xxx

#
# Another recursion test
#
cat > x <<"XEOF"
LC_ALL=C
alias foo="echo hello "
alias bar="foo world"
echo $(bar)
XEOF
docommand al11 "$SHELL ./x" 0 "hello world\n" ""
remove x

#
# Check whether aliases work in eval
#
cat > x <<"XEOF"
LC_ALL=C
alias foo="echo"
foo a
eval foo b
XEOF
docommand al11 "$SHELL ./x" 0 "a\nb\n" ""
remove x

cat > x <<"XEOF"
LC_ALL=C
alias foo="echo ok"
foo
foo a
eval foo
eval foo b
XEOF
docommand al12 "$SHELL ./x" 0 "ok\nok a\nok\nok b\n" ""
remove x

#
# Check whether a new alias of the same name may be defined after
# unaliasing the old one.
#
cat > x <<"XEOF"
LC_ALL=C
for i in 1 2 3; do
	alias "foo=bar$i" || echo OOPS
	alias
	unalias foo
done

XEOF
docommand al13 "$SHELL ./x" 0 "foo='bar1'\nfoo='bar2'\nfoo='bar3'\n" ""
remove x

success
