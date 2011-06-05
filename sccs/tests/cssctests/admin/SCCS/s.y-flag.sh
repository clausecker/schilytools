h14691
s 00002/00002/00196
d D 1.2 11/05/30 01:18:22 joerg 2 1
c test -e -> test -r
e
s 00198/00000/00000
d D 1.1 11/04/26 03:04:16 joerg 1 0
c date and time created 11/04/26 03:04:16 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# y-flag.sh:  Testing for the 'y' flag for admin (admin -fy).

# Import common functions & definitions.
. ../common/test-common

# Determine if we are testing CSSC or the real thing.
. ../common/real-thing

g=bar
s=s.${g}
z=z.${g}

remove $s $g $z foo command.log last.command core 
remove expected.stderr got.stderr expected.stdout got.stdout

# Figure out if we should expect the thing to work.
if ${admin} -n -i/dev/null -fyM "${s}" >/dev/null 2>&1 || $TESTING_CSSC
then
D 2
    test -e "${s}" || miscarry "admin program '${admin}' silently did nothing"
E 2
I 2
    test -r "${s}" || miscarry "admin program '${admin}' silently did nothing"
E 2
    echo "We are testing an SCCS implementation that supports the y flag.  Good."
    remove "${s}"
else
    echo "WARNING: some test have been skipped since I think that ${admin} does not support the 'y' flag."
    remove $s $g $z foo command.log last.command core 
    remove expected.stderr got.stderr expected.stdout got.stdout
    success
    exit 0
fi


remove foo
cat > foo <<EOF 
 1 M %M%
 2 R %R%
 3 L %L%
 4 B %B%
 5 S %S%
 6 Y %Y%
 7 F %F%
 8 Q %Q% 
 9 C %C%
10 C %C%
11 Z %Z%
12 W %W%
EOF
test -r foo || miscarry cannot create file foo.
D 2
test -e "${s}" && miscarry initial conditions were incorrectly set up
E 2
I 2
test -r "${s}" && miscarry initial conditions were incorrectly set up
E 2

docommand Y1 "${admin} -ifoo ${s}" 0 "" IGNORE
remove foo


# docommand A2 "${admin} -dy $s" 0 IGNORE IGNORE

# default situation is that everything is expanded.
docommand Y2 "${vg_get} -p -r1.1 ${s}" 0 "\
 1 M bar
 2 R 1
 3 L 1
 4 B 0
 5 S 0
 6 Y 
 7 F s.bar
 8 Q  
 9 C 9
10 C 10
11 Z @(#)
12 W @(#)bar	1.1
" IGNORE


docommand YMa "${vg_admin} -fyM ${s}" 0 "" IGNORE
docommand YMg "${get} -p -r1.1 ${s}" 0 "\
 1 M bar\n 2 R %R%\n 3 L %L%\n 4 B %B%\n 5 S %S%\n 6 Y %Y%
 7 F %F%\n 8 Q %Q% \n 9 C %C%\n10 C %C%\n11 Z %Z%\n12 W %W%
" IGNORE

# docommand Y_a "${admin} -fy_ ${s}" 0 "" IGNORE
# docommand Y_g "${get} -p -r1.1 ${s}" 0 "\
#  1 M %M%\n 2 R %R%\n 3 L %L%\n 4 B %B%\n 5 S %S%\n 6 Y %Y%
#  7 F %F%\n 8 Q %Q% \n 9 C %C%\n10 C %C%\n11 Z %Z%\n12 W %W%
# " IGNORE


docommand YRa "${vg_admin} -fyR ${s}" 0 "" IGNORE
docommand YRg "${get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R 1\n 3 L %L%\n 4 B %B%\n 5 S %S%\n 6 Y %Y%
 7 F %F%\n 8 Q %Q% \n 9 C %C%\n10 C %C%\n11 Z %Z%\n12 W %W%
" IGNORE


docommand YLa "${vg_admin} -fyL ${s}" 0 "" IGNORE
docommand YLg "${vg_get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R %R%\n 3 L 1\n 4 B %B%\n 5 S %S%\n 6 Y %Y%
 7 F %F%\n 8 Q %Q% \n 9 C %C%\n10 C %C%\n11 Z %Z%\n12 W %W%
" IGNORE

docommand YBa "${vg_admin} -fyB ${s}" 0 "" IGNORE
docommand YBg "${vg_get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R %R%\n 3 L %L%\n 4 B 0\n 5 S %S%\n 6 Y %Y%
 7 F %F%\n 8 Q %Q% \n 9 C %C%\n10 C %C%\n11 Z %Z%\n12 W %W%
" IGNORE

docommand YSa "${vg_admin} -fyS ${s}" 0 "" IGNORE
docommand YSg "${vg_get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R %R%\n 3 L %L%\n 4 B %B%\n 5 S 0\n 6 Y %Y%
 7 F %F%\n 8 Q %Q% \n 9 C %C%\n10 C %C%\n11 Z %Z%\n12 W %W%
" IGNORE

docommand YYa "${vg_admin} -fyY ${s}" 0 "" IGNORE
docommand YYg "${vg_get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R %R%\n 3 L %L%\n 4 B %B%\n 5 S %S%\n 6 Y 
 7 F %F%\n 8 Q %Q% \n 9 C %C%\n10 C %C%\n11 Z %Z%\n12 W %W%
" IGNORE


docommand YFa "${vg_admin} -fyF ${s}" 0 "" IGNORE
docommand YFg "${vg_get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R %R%\n 3 L %L%\n 4 B %B%\n 5 S %S%\n 6 Y %Y%
 7 F s.bar\n 8 Q %Q% \n 9 C %C%\n10 C %C%\n11 Z %Z%\n12 W %W%
" IGNORE

docommand YQa "${vg_admin} -fyQ ${s}" 0 "" IGNORE
docommand YQg "${vg_get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R %R%\n 3 L %L%\n 4 B %B%\n 5 S %S%\n 6 Y %Y%
 7 F %F%\n 8 Q  \n 9 C %C%\n10 C %C%\n11 Z %Z%\n12 W %W%
" IGNORE


docommand YCa "${vg_admin} -fyC ${s}" 0 "" IGNORE
docommand YCg "${vg_get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R %R%\n 3 L %L%\n 4 B %B%\n 5 S %S%\n 6 Y %Y%
 7 F %F%\n 8 Q %Q% \n 9 C 9\n10 C 10\n11 Z %Z%\n12 W %W%
" IGNORE


docommand YZa "${vg_admin} -fyZ ${s}" 0 "" IGNORE
docommand YZg "${vg_get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R %R%\n 3 L %L%\n 4 B %B%\n 5 S %S%\n 6 Y %Y%
 7 F %F%\n 8 Q %Q% \n 9 C %C%\n10 C %C%\n11 Z @(#)\n12 W %W%
" IGNORE

docommand YWa "${vg_admin} -fyW ${s}" 0 "" IGNORE
docommand YWg "${get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R %R%\n 3 L %L%\n 4 B %B%\n 5 S %S%\n 6 Y %Y%
 7 F %F%\n 8 Q %Q% \n 9 C %C%\n10 C %C%\n11 Z %Z%\n12 W @(#)bar	1.1
" IGNORE

docommand YCWa "${vg_admin} -fyW,C ${s}" 0 "" IGNORE
docommand YCWg "${get} -p -r1.1 ${s}" 0 "\
 1 M %M%\n 2 R %R%\n 3 L %L%\n 4 B %B%\n 5 S %S%\n 6 Y %Y%
 7 F %F%\n 8 Q %Q% \n 9 C 9\n10 C 10\n11 Z %Z%\n12 W @(#)bar	1.1
" IGNORE


# Now, testing for %A% and %I%
remove ${g} ${s}


remove foo
cat > foo <<EOF 
 1 %Z%%Y% %M% %I%%Z%
 2 %A%
EOF
test -r foo || miscarry cannot create file foo.

docommand YA1 "${admin} -ifoo ${s}" 0 "" IGNORE
remove foo

docommand YA2 "${vg_admin} -dy ${s}" 0 "" IGNORE
docommand YA3 "${get} -p -r1.1 ${s}" 0 "\
 1 @(#) bar 1.1@(#)
 2 @(#) bar 1.1@(#)
" IGNORE

# Disable expansion of %Z% and %I%, and check that it is still expanded in 
# %A%.
docommand YA4 "${vg_admin} -fyA,M ${s}" 0 "" IGNORE
docommand YA5 "${get} -p -r1.1 ${s}" 0 "\
 1 %Z%%Y% bar %I%%Z%
 2 @(#) bar 1.1@(#)
" IGNORE

# Disable M as well and check again.
# Disable expansion of %Z% and %I%, and check that it is still expanded in 
# %A%.
docommand YA6 "${vg_admin} -fyA ${s}" 0 "" IGNORE
docommand YA7 "${get} -p -r1.1 ${s}" 0 "\
 1 %Z%%Y% %M% %I%%Z%
 2 @(#) bar 1.1@(#)
" IGNORE


remove $s $g $z foo command.log last.command core 
remove expected.stderr got.stderr expected.stdout got.stdout
success
E 1
