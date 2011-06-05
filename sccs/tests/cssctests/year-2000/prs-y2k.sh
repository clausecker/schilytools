#! /bin/sh


# prs-y2k.sh:  Testing for correct operation of prs
#               with regard to date issues.

# Import common functions & definitions.
. ../common/test-common
. ../common/real-thing


s=s.y2k.txt

brief='"-d:I: :D: :T:"'

r1_5="1.5 68/12/31 23:59:59\n" # 2068: the last year we have
r1_4="1.4 00/02/29 00:00:00\n" # Year 2000 is a leap year.
r1_3="1.3 00/01/01 00:00:00\n" # Just after the milennium
r1_2="1.2 99/12/31 23:59:59\n" # Just before the milennium
r1_1="1.1 69/01/01 00:00:00\n" # 1969: the earliest year we have

allrevs="${r1_5}${r1_4}${r1_3}${r1_2}${r1_1}"

# First some easy tests that any y2k-compliant version should pass.
docommand ez2 "${vg_prs} ${brief} -r1.2  $s" 0 "${r1_2}" ""
docommand ez3 "${vg_prs} ${brief} -r1.3  $s" 0 "${r1_3}" ""
docommand ez4 "${vg_prs} ${brief} -r1.4  $s" 0 "${r1_4}" ""


if "$TESTING_CSSC"
then
    expect_fail=false
    OS=`uname`
    #
    # AIX does not support any date before January 1 1970 with the
    # time functions in libc
    #
    if test .$OS = .AIX
    then
	expect_fail=true
    fi
else
    # Many versions of SCCS are y2k-safe, but don't work right out to
    # the boundary dates specified by the POSIX windowing scheme.
    # For example OpenSolaris 2009.06 renders "68/12/31 23:59:59"
    # as "32/11/25 17:31:43".
    expect_fail=true
fi

## And now the harder tests.

## If we just specify -e without -c we should get all the revisions.
## Check that the dates are printed correctly.
docommand A1 "${vg_prs} ${brief} -e $s" 0 "${allrevs}" ""

docommand t1 "${vg_prs} ${brief} -e -c690101000000  $s" 0 \
    "${r1_1}" ""
docommand t2 "${vg_prs} ${brief} -l -c690101000000  $s" 0 \
    "${allrevs}" ""

docommand t3 "${vg_prs} ${brief} -l -c690101000001  $s" 0 \
    "${r1_5}${r1_4}${r1_3}${r1_2}" ""

docommand t4 "${vg_prs} ${brief} -e -c991231235959  $s" 0 \
    "${r1_2}${r1_1}" ""

docommand t5 "${vg_prs} ${brief} -l -c991231235959  $s" 0 \
    "${r1_5}${r1_4}${r1_3}${r1_2}" ""

docommand t6 "${vg_prs} ${brief} -e -c000101000000  $s" 0 \
    "${r1_3}${r1_2}${r1_1}" ""
docommand t7 "${vg_prs} ${brief} -l -c000101000000  $s" 0 \
    "${r1_5}${r1_4}${r1_3}" ""

docommand t8 "${vg_prs} ${brief} -l -c000101000001  $s" 0 \
    "${r1_5}${r1_4}" ""

docommand t9 "${vg_prs} ${brief} -e -c000229000000  $s" 0 \
    "${r1_4}${r1_3}${r1_2}${r1_1}" ""
docommand t10 "${vg_prs} ${brief} -e -c000229000001  $s" 0 \
    "${r1_4}${r1_3}${r1_2}${r1_1}" ""
docommand t11 "${vg_prs} ${brief} -l -c000229000000  $s" 0 \
    "${r1_5}${r1_4}" ""

docommand t12 "${vg_prs} ${brief} -l -c681231235959  $s" 0 \
    "${r1_5}" ""


## Tests involving fields that take default values...

# Just giving the year should be equivalent to explicitly
# specifying the last second of that year.
docommand d1 "${vg_prs} ${brief} -l -c99  $s" 0 \
    "${r1_5}${r1_4}${r1_3}${r1_2}" ""

docommand d2 "${vg_prs} ${brief} -l -c0001  $s" 0 \
    "${r1_5}${r1_4}" ""

docommand d3 "${vg_prs} ${brief} -l -c000228  $s" 0 \
    "${r1_5}${r1_4}" ""
docommand d4 "${vg_prs} ${brief} -e -c68  $s" 0 \
    "${r1_5}${r1_4}${r1_3}${r1_2}${r1_1}" ""

docommand d5 "${vg_prs} ${brief} -l -c68  $s" 0 \
    "${r1_5}" ""




remove command.log
success
