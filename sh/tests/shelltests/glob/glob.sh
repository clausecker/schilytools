#! /bin/sh
#
# @(#)glob.sh	1.4 16/06/05 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

remove d/*
remove dir/*
rmdir d dir 2>/dev/null

#
# Basic tests to check whether globbing works
#
docommand gl00 "$SHELL -c 'echo gl?b.sh'" 0 "glob.sh\n" ""
docommand gl01 "$SHELL -c 'echo gl*b.sh'" 0 "glob.sh\n" ""
docommand gl02 "$SHELL -c 'echo g*b.sh'" 0 "glob.sh\n" ""
docommand gl03 "$SHELL -c 'echo gl[abco]b.sh'" 0 "glob.sh\n" ""
docommand gl04 "$SHELL -c 'echo gl[a-o]b.sh'" 0 "glob.sh\n" ""
docommand gl05 "$SHELL -c 'echo gl\ob.sh'" 0 "glob.sh\n" ""

#
# Basic tests to check whether globbing with syntax error does not expand
#
docommand gl06 "$SHELL -c 'echo ![*[*'" 0 "![*[*\n" ""

$SRCROOT/conf/mkdir-sh -p '[x'
: > '[x/foo'
docommand gl07 "$SHELL -c 'echo [*'" 0 "[*\n" ""
docommand gl08 "$SHELL -c 'echo *[x'" 0 "*[x\n" ""
docommand gl09 "$SHELL -c 'echo [x/*'" 0 "[x/foo\n" ""
remove '[x/foo'
rmdir '[x'

symlink="ln -s"
MKLINKS_TEST=${MKLINKS_TEST-:}
rm -f xxzzy.123 xxzzy.345
echo test > xxzzy.123
$symlink xxzzy.123 xxzzy.345
test $? = 0 || symlink=cp
test -r xxzzy.345 || symlink=cp
${MKLINKS_TEST} -h xxzzy.345 || symlink=cp
rm -f xxzzy.123 xxzzy.345

if [ "$symlink" != cp ]; then
$SRCROOT/conf/mkdir-sh -p 'dir'
$symlink non-existent dir/abc

docommand gl10 "$SHELL -c 'echo d*/*'" 0 "dir/abc\n" ""
docommand gl11 "$SHELL -c 'echo d*/abc'" 0 "dir/abc\n" ""

remove dir/abc
rmdir dir
fi

docommand gl20 "$SHELL -c 'case foo in f*) echo OK;; *) echo FAIL;; esac'" 0 "OK\n" ""
docommand gl21 "$SHELL -c 'case foo in bla|f*) echo OK;; *) echo FAIL;; esac'" 0 "OK\n" ""
docommand gl22 "$SHELL -c 'case f\\* in f*) echo OK;; *) echo FAIL;; esac'" 0 "OK\n" ""
docommand gl23 "$SHELL -c 'case f\\* in f\*) echo OK;; *) echo FAIL;; esac'" 0 "OK\n" ""
docommand gl24 "$SHELL -c 'case f\\* in f'*') echo OK;; *) echo FAIL;; esac'" 0 "OK\n" ""
docommand gl25 "$SHELL -c 'case f\\* in 'f*') echo OK;; *) echo FAIL;; esac'" 0 "OK\n" ""

#
# The ideas from the following tests have been taken from the "mksh" test suite
# Thanks to Thorsten Glaser
#

$SRCROOT/conf/mkdir-sh -p 'd'
:> "d/.bc" 
:> "d/abc" 
:> "d/bbc" 
:> "d/cbc" 
:> "d/-bc" 
#
# Note that we need to escape ^ as the Bourne Shell implements ^ as
# an alias for |
#
# glob-range-1
# Test range matching
#
cat > x <<"XEOF"
cd d
echo [ab-]*
echo [-ab]*
echo [!-ab]*
echo [!ab]*
echo []ab]*
:>'./!bc'
:>'./^bc'
echo [\^ab]*
echo [!ab]*
XEOF
docommand glob00 "$SHELL ./x" 0 "\
-bc abc bbc
-bc abc bbc
cbc
-bc cbc
abc bbc
^bc abc bbc
!bc -bc ^bc cbc
" ""
remove x
remove "d/.bc" "d/abc" "d/bbc" "d/cbc" "d/-bc" "d/!bc" "d/^bc"
rmdir d

:>abc
#
# glob01 is from mksh but fails to fail as usual gmatch() or 
# fnmatch implementations check for: 
#	(c == sub || (c <= sub && sub <= d))
# so they match if the character from the filename matches the
# first character in the range.
#
#docommand glob01 "$SHELL -c 'echo [a--]*'" 0 "OK\n" ""

#
# Z is after - in ascii, so this is an illegal glob.
#
docommand glob02 "$SHELL -c 'echo [Z--]*'" 0 "[Z--]*\n" ""
remove abc

#
# glob-range-3
# ISO-8859-1 matching
:> "aÂc"
docommand glob03 "$SHELL -c 'echo a[Á-Ú]*'" 0 "aÂc\n" ""
remove "aÂc"

:>.bc
docommand glob04 "$SHELL -c 'echo [a.]*'" 0 "[a.]*\n" ""
remove .bc

$SRCROOT/conf/mkdir-sh -p 'd'
:> "d/abc" 
:> "d/bbc" 
:> "d/cbc" 
:> "d/dbc" 
:> "d/ebc"
:> "d/-bc"
#
# glob-range-5
#	Results unspecified according to POSIX
#	(AT&T gmatch() from libgen treats this like [a-cc-e]*)
#	Our gmatch.c treats this like [a-c] + [-e], so we
#	cannot use this as long as we may use the one or the other
#
cat > x <<"XEOF"
cd d
echo [a-c-e]*
XEOF
#docommand glob05 "$SHELL ./x" 0 "-bc abc bbc cbc ebc\n" ""
remove x
remove "d/abc" "d/bbc" "d/cbc" "d/dbc" "d/ebc" "d/-bc"
rmdir d

#
# glob-trim-1
# Check against a regression from fixing IFS-subst-2
#
cat > x <<"XEOF"
x='#foo'
printf "before='%s'\n" "$x"
x=${x%%#*}
printf "after='%s'\n" "$x"
XEOF
docommand glob06 "$SHELL ./x" 0 "\
before='#foo'
after=''
" ""
remove x

cat > x <<"XEOF"
XEOF
#docommand glob01 "$SHELL ./x" 0 "" ""
remove x

success
