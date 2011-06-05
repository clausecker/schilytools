#! /bin/sh
# sid-select.sh:  Do we select the correct SIDs?

# Import common functions & definitions.
. ../common/test-common


# Get a test file...
s=s.testfile
remove $s
../../testutils/uu_decode --decode < testfile.uue || 
    miscarry could not extract test file.

get_expect () {
label=$1         ; shift
sid_expected=$1  ; shift
docommand $label "${vg_get} -g $*" 0 "$sid_expected\n" IGNORE
}

# Do various forms of get on the file and make sure we get the right SID.
get_expect X1  1.1        -r1.1      $s
get_expect X2  1.2        -r1.2      $s
get_expect X3  1.3        -r1.3      $s
get_expect X4  1.4        -r1.4      $s
get_expect X5  1.5        -r1.5      $s
get_expect X6  1.5        -r1        $s
get_expect X7  1.3.1.1    -r1.3.1.1  $s
get_expect X8  1.3.1.1    -r1.3.1    $s
get_expect X9  2.1        -r2.1      $s
get_expect X10 2.1        -r2        $s
get_expect X11 2.1        ""         $s
get_expect X12 2.1        -r3        $s
get_expect X13 2.1        -r9000     $s

docommand F1 "${vg_get} -r3.1   s.testfile" 1 "" IGNORE
docommand F2 "${vg_get} -r3.1.1 s.testfile" 1 "" IGNORE

remove $s
success
