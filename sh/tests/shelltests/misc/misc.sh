#! /bin/sh
#
# @(#)misc.sh	1.3 17/06/14 2016-2017 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check miscelaneous things
# (ideas taken from "mksh" test suite)
# Thanks to Thorsten Glaser
#

#
# A comment always ends at a newline
#
cat > x <<"XEOF"
echo hi #there \
echo folks
XEOF
docommand misc00 "$SHELL ./x" 0 'hi\nfolks\n' ""
remove x

#
# \newline is retained inside single quotes
#
cat > x <<"XEOF"
echo 'hi \
there'
echo folks
XEOF
docommand misc01 "$SHELL ./x" 0 'hi \\\nthere\nfolks\n' ""
remove x

#
# \newline is retained inside quoted heredoc
#
cat > x <<"XEOF"
cat << \EOF
hi \
there
EOF
XEOF
docommand misc02 "$SHELL ./x" 0 'hi \\\nthere\n' ""
remove x

#
# Aliases and variables are not expanded in quoted heredocs
#
cat > x <<"XEOF"
a=2
alias x='echo hi
cat << "EOF"
foo\
bar
some'
x
more\
stuff$a
EOF
XEOF
docommand misc03 "$SHELL ./x" 0 'hi\nfoo\\\nbar\nsome\nmore\\\nstuff$a\n' ""
remove x

#
# Backslash at end of input is ignored
#
cat > x <<"XEOF"
echo `echo foo\\`bar
echo hi\
XEOF
docommand misc04 "$SHELL ./x" 0 'foobar\nhi\n' ""
remove x

#
# \newline at normal places should be removed
#
cat > x <<"XEOF"
		\
		 echo hi\
There, \
folks
XEOF
docommand misc05 "$SHELL ./x" 0 'hiThere, folks\n' ""
remove x

#
# \newline in $ sequences must be removed
# ksh93 fails here
#
cat > x <<"XEOF"
a=12
ab=19
echo $\
a
echo $a\
b
echo $\
{a}
echo ${a\
b}
echo ${ab\
}
XEOF
docommand misc06 "$SHELL ./x" 0 '12\n19\n12\n19\n19\n' ""
remove x

#
# \newline in $(...) and `...` sequences must be removed
# ksh93 fails here
#
cat > x <<"XEOF"
echo $\
(echo foobar1)
echo $(\
echo foobar2)
echo $(echo foo\
bar3)
echo $(echo foobar4\
)
echo `
echo stuff1`
echo `echo st\
uff2`
XEOF
docommand misc07 "$SHELL ./x" 0 'foobar1\nfoobar2\nfoobar3\nfoobar4\nstuff1\nstuff2\n' ""
remove x

#
# \newline in $((...)) sequences must be removed
# ksh93 fails here
#
cat > x <<"XEOF"
echo $\
((1+2))
echo $(\
(1+2+3))
echo $((\
1+2+3+4))
echo $((1+\
2+3+4+5))
echo $((1+2+3+4+5+6)\
)
XEOF
docommand misc08 "$SHELL ./x" 0 '3\n6\n10\n15\n21\n' ""
remove x

#
# \newline must be removed in quoted strings
#
cat > x <<"XEOF"
echo "\
hi"
echo "foo\
bar"
echo "folks\
"
XEOF
docommand misc09 "$SHELL ./x" 0 'hi\nfoobar\nfolks\n' ""
remove x

#
# \newline must be removed in here document delimiters
# ksh93 fails in the second part
#
cat > x <<"XEOF"
a=12
cat << EO\
F
a=$a
foo\
bar
EOF
cat << E_O_F
foo
E_O_\
F
echo done 
XEOF
docommand misc10 "$SHELL ./x" 0 'a=12\nfoobar\nfoo\ndone\n' ""
remove x

#
# \newline must be removed in double-quoted here document delimiters
#
cat > x <<"XEOF"
a=12
cat << "EO\
F"
a=$a
foo\
bar
EOF
echo done 
XEOF
docommand misc11 "$SHELL ./x" 0 'a=$a\nfoo\\\nbar\ndone\n' ""
remove x

#
# \newline must be removed in various 2+ character tokens
#
cat > x <<"XEOF"
echo hi &\
& echo there
echo foo |\
| echo bar
cat <\
< EOF
stuff
EOF
cat <\
<\
- EOF
more stuff
EOF
cat <<\
EOF
abcdef
EOF
echo hi >\
> /dev/null
echo $?
i=1
case $i in
(\
x|\
1\
) echo hi;\
;
(*) echo oops
esac
XEOF
docommand misc12 "$SHELL ./x" 0 'hi\nthere\nfoo\nstuff\nmore stuff\nabcdef\n0\nhi\n' ""
remove x

#
# \ at end of an alias must be removed when followed by a newline
# POSIX makes this undefined because of a related bash bug
#
cat > x <<"XEOF"
alias x='echo hi\'
x
echo there
XEOF
docommand misc13 "$SHELL ./x" 0 'hiecho there\n' ""
remove x

#
# \newline inside a keyword must be removed
#
cat > x <<"XEOF"
i\
f true; then\
 echo pass; el\
se echo fail; fi
XEOF
docommand misc14 "$SHELL ./x" 0 'pass\n' ""
remove x

#
# \newline inside case globbing must be removed
#
cat > x <<"XEOF"
xxx=foo
case $xxx in
(f*\
o\
) echo pass ;;
*) echo bad
esac
XEOF
docommand misc15 "$SHELL ./x" 0 'pass\n' ""
remove x

#
# The following two tests are from Robert Elz:
#
cat > x <<"XEOF"
V\
AR=hel\
lo
unset U V1
pri\
ntf '%s' ${\
VAR\
}
p\
r\
i\
n\
t\
f\
 \
'%s' \
$\
{\
V\
A\
R}
printf '%s' ${U\
-\
"$\
{V\
1:\
=$\
{V\
AR+\
${V\
AR}\
}\
}"}
printf '%s' ${V\
1?V1\
 \
FAIL}
XEOF

docommand misc30 "$SHELL ./x" 0 'hellohellohellohello' ""
remove x

cat > x <<"XEOF"
l\
s=7 bi\
n\
=\
3
echo $(\
( ls /bin )\
)
XEOF
docommand misc31 "$SHELL ./x" 0 '2\n' ""
remove x

success
