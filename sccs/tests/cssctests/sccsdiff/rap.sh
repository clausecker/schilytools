#! /bin/sh

# rap.sh: Test script for sccsdiff by Richard Polton <richardp@scopic.com>.

# This file is part of GNU CSSC.

# Create an SCCS file with two deltas. sccsdiff the two deltas.

# Import common functions & definitions.
. ../../common/test-common
export get

# invariant label file1 file2 
#
# diff file1 and file2 and fail the test if they
# are different.
invariant () {
if diff $2 $3
then 
    echo passed
else 
    fail "$1: $2 and $3 are not the same"
fi ;
}

g=foo
s=s.$g

remove $s $g command.log
echo one > $g
docommand prep1 "${admin} -i$g $s" 0 IGNORE IGNORE 
remove $g
docommand prep2 "${get} -e $s " 0 IGNORE  IGNORE 
echo two >> $g
docommand prep3 "${delta} -ycomment $s" 0 IGNORE  IGNORE 


## Avoid any current locale setting - because we delete lines
## including the word "Page" below, so we must use the C or POSIX
## locale, or some implementation-defined English locale, in order 
## to see that string. 

unset  LANGUAGE LC_ALL LC_CTYPE LC_COLLATE LANG
# counterexample for Debian GNU/Linux: LANG=de_DE.ISO-8859-1
# export LANGUAGE LC_ALL LC_CTYPE LC_COLLATE LANG

header="
------- ${g} -------
"


echo_nonl "D1..."
remove  $g
${sccsdiff} -r1.1 -r1.2 $s 2>errs >diff.out

cat > D1.diff.expected <<EOF
${header}1a2
> two
EOF

# Expect success
invariant D1 diff.out D1.diff.expected
remove errs

#
# sccsdiff output to pipe through pr
#
echo_nonl "D2..."
remove diff.out

${sccsdiff} -p -r1.1 -r1.2 $s 2>errs >diff.out


remove diff.test1
pr_header="s.foo: 1.1 vs. 1.2"
cat D1.diff.expected | sed -e '1,2 d' | pr -h "${pr_header}" > D2.diff.expected

# Expect success
invariant D2 diff.out D2.diff.expected
remove  diff.out D1.diff.expected D2.diff.expected errs


#
# Try to sccsdiff non-existent deltas
#
# second sid
#
echo_nonl "D3..."
remove diff.out
${sccsdiff} -r1.1 -r1.3 $s 2>errs >/dev/null 
rv=$?
sed '/No id keywords/d' > diff.out < errs
if test $rv -ne 1
then
    fail sccsdiff should return value 1, got $rv.
else
    echo passed
fi
remove diff.out errs


#
# first sid
#
remove out
echo_nonl "D4..."
${sccsdiff} -r1.3 -r1.1 $s 2>errs >/dev/null
rv=$?
sed '/No id keywords/d' > diff.out < errs
if test $rv -ne 1
then
    fail sccsdiff should return value 1, got $rv.
else
    echo passed
fi
remove diff.out errs


# N.B. The output from solaris sccsdiff is a little different. diff follows:

#1c1,3
#< ERROR [s.foo]: nonexistent sid (ge5)
#---
#> 
#> get: s.foo: Requested SID not found.
#> Failed to get second specified version from s.foo


remove $g $s z.$g x.$g diff.out diff.test diff.test1 command.log

success

# Local Variables:
# Mode: shell
# End:
