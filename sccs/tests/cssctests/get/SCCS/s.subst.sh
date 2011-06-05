h18167
s 00095/00000/00000
d D 1.1 10/04/29 02:05:14 joerg 1 0
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# Some substitution tests...


# Import common functions & definitions.
. ../common/test-common


# Get a test file...
f=keywords.txt
s=s.$f
output=get.output


remove $s $f
../../testutils/uu_decode --decode < keywords.uue || miscarry could not extract test file.

# Expand all the keywords from the s.file and save the format in 
# a temporary file.   We then examine this file later.
echo_nonl "Preparing..."
remove ${output}
if ${vg_get} -p $s > ${output} 2>/dev/null
then
    echo passed
else
    fail "$0: preparation step: could not run get -p."
fi

# Ckeck that the format of stderr is correct.
docommand "stderr format" "${vg_get} -p $s" 0 "IGNORE" "1.1\n83 lines\n"

expands_to () {
    # $1 -- format
    # $2 -- expansion
docommand "%${1}%" "egrep \"^_${1}_ \" <${output}" 0 "$2" ""
}

# Examine each of the things formatted into the file and
# check them against our expectations.

expands_to A "_A_ @(#) keywords.txt 1.1@(#)\n" 
expands_to B "_B_ 0\n"                         
expands_to C "_C_ 16\n_C_ 17\n"                
expands_to E "_E_ 97/10/25\n"                  
expands_to F "_F_ s.keywords.txt\n"            
expands_to G "_G_ 10/25/97\n"                  
expands_to I "_I_ 1.1\n" 
expands_to L "_L_ 1\n"                         
expands_to M "_M_ keywords.txt\n"	         
expands_to P "_P_ `../../testutils/realpwd`/s.keywords.txt\n"     
expands_to Q "_Q_ \n"                          
expands_to R "_R_ 1\n"                         
expands_to S "_S_ 0\n"                         
expands_to U "_U_ 13:19:31\n"                  
expands_to W "_W_ @(#)keywords.txt\t1.1\n"     
expands_to Y "_Y_ \n"                          
expands_to Z "_Z_ @(#)\n"                      

# TODO: better tests for Q, D, H, T, Y


# Test the -k flag, which disables keyword substitution.
if percents=`${vg_get} -p -k $s 2>/dev/null | tr -dc % | wc -c`
then
    if [ $percents -eq 68 ]
    then
	echo D1...passed
    else
	fail There should be 68 % signs in the gotten output with -k option.
    fi
else
    fail get without substitution should not fail here.
fi


remove $s $output

# Tests to make sure that the keyword substitution gets the right IDs
# and so forth when working with the -c date cutoff.
s=s.keys.txt
remove $s
../../testutils/uu_decode --decode < keys.uue || miscarry could not extract test file.
docommand K1 "${vg_get} -p -c971025230458 $s" 0 "1.2 1.2\n" "1.2\n1 lines\n"
docommand K2 "${vg_get} -p -c971025230457 $s" 0 "1.1 1.1\n" \
	"IGNORE"

# TODO: We currently say Excluded: blah... if a version is 
# excluded because of the cutoff date.  We should not do that.


# tests are finished.
remove $s
remove command.log
success
E 1
