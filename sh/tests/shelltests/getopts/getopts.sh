#! /bin/sh
#
# @(#)getopts.sh	1.4 16/06/29 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Test error message for illegal option
#
docommand -noremove G1 "$SHELL getopts-1" 0 "" IGNORE
#
# Not all shells use the same error message, but all include "option"
#
err=`grep 'option' got.stderr`
if [ -z "$err" ]; then
	fail "Test $cmd_label failed: wrong error message"
fi
do_remove

#
# Illegal option with swithced off error message
#
docommand G2 "$SHELL getopts-2" 0 "" ""

#
#
#
docommand G3 "$SHELL getopts-3" 0 "\$? '0' \$OPT '?' \$OPTARG 's' \$OPTIND '2'\n" ""
docommand G4 "$SHELL getopts-4" 0 "\$? '0' \$OPT 'a' \$OPTARG '' \$OPTIND '2'\n" ""
docommand G5 "$SHELL getopts-5" 0 "\$? '0' \$OPT ':' \$OPTARG 'f' \$OPTIND '2'\n" ""
docommand G6 "$SHELL getopts-6" 0 "\$? '0' \$OPT 'f' \$OPTARG 'argument' \$OPTIND '3'\n" ""
docommand G7 "$SHELL getopts-7" 0 "\$? '0' \$OPT 'f' \$OPTARG 'ile' \$OPTIND '2'\n" ""

expect_fail_save=$expect_fail
if [ "$is_bosh" = false ]; then
	expect_fail=true
fi
docommand G8 "$SHELL getopts-8" 0 "\$? '0' \$OPT 'f' \$OPTARG 'argument' \$OPTIND '3'\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	echo
	echo "Test $cmd_label is a Solaris getopt() enhancement."
	echo "Skipping G9..G10."
	echo
else
expect_fail_save=$expect_fail
if [ "$is_bosh" = false ]; then
	expect_fail=true
fi
docommand G9 "$SHELL getopts-9" 0 "\$? '0' \$OPT '1000' \$OPTARG 'argument' \$OPTIND '3'\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	echo
	echo "Test $cmd_label is a Schily getopt() enhancement."
	echo "Skipping G10."
	echo
else
docommand G10 "$SHELL getopts-10" 0 "\$? '0' \$OPT '1000' \$OPTARG 'argument' \$OPTIND '3'\n" ""
fi
fi

#
# Check for the SIGSEGV bug in getopt() from Solaris libc.
# If the Solaris libc is unfixed, the shell receives a SIGSEGV and thus does
# not print the second output line with: "$? '1'".
#
# For other shells, this check whether the behavior at the end of the argument
# list is correct. With OPTIND=99, we expect $? to be 1 to flag that getopt()
# returned -1.
#
cat > x <<-"XEOF"
	# Switch off localization for tests to allow to compare the output
	LC_ALL=C
	getopts ":abf:" OPT -ab
	echo '$?' "'"$?"'" '$OPT' "'"$OPT"'" '$OPTARG' "'"$OPTARG"'"
	OPTIND=99
	getopts ":abf:" OPT -ab
	echo '$?' "'"$?"'"
	exit 0
XEOF
docommand G100 "$SHELL ./x" 0 "\
\$? '0' \$OPT 'a' \$OPTARG ''
\$? '1'
" IGNORE
remove x

success
