#! /bin/sh
#
# @(#)cmdsub.sh	1.7 20/04/22 Copyright 2016-2020 J. Schilling
#

# Read test core functions
. ../../common/test-common

docommand A1 "$SHELL cmdsub-A1" 0 "x\n" ""
docommand A2 "$SHELL cmdsub-A2" 0 "x\n" ""
docommand A3 "$SHELL cmdsub-A3" 0 "x\n" ""
docommand A4 "$SHELL cmdsub-A4" 0 "x\n" ""
docommand A5 "$SHELL cmdsub-A5" 0 "x\n" ""

docommand B "$SHELL cmdsub-B" 0 "quoted )\n" ""

docommand C "$SHELL cmdsub-C" 0 "comment\n" ""

docommand D1 "$SHELL cmdsub-D1" 0 "here-doc with )\n" ""
docommand D2 "$SHELL cmdsub-D2" 2 "" IGNORE
docommand D3 "$SHELL cmdsub-D3" 0 "here-doc with \\()\n" ""

docommand E "$SHELL cmdsub-E" 0 "here-doc terminated with a parenthesis\n" ""

docommand F "$SHELL cmdsub-F" 0 "' # or a single back- or doublequote\n" ""

docommand G "$SHELL cmdsub-G" 0 "\n" ""

docommand H "$SHELL cmdsub-H" 0 "\n" ""

#
# This command substitution is used by Sven Maschek in "whatshell.sh"
#
if [ "$is_bosh" = true ]; then

expect_fail_save=$expect_fail
expect_fail=true
docommand -silent -esilent cmdsub00 \
  "$SHELL -c 'set -o | grep \"posix.*on\" > /dev/null && echo YES || echo NO'" \
  0 "NO\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	echo
	echo "$SHELL is in POSIX mode, skipping test cmdsub01."
	echo
else
	docommand cmdsub01 "$SHELL -c 'case \$( (:^times) 2>&1) in *0m*) echo OK;; *) echo FAIL;; esac'" 0 "OK\n" ""
fi
else
	echo
	echo "Not a Bourne Shell, skipping cmdsub01: ^ pipe test: \"(:^times)\""
	echo
fi

#
# Check whether $() allows to include complex strings that contain newlines
#
cat > x <<"XEOF"
: ${AWK=/usr/bin/nawk}
#$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=/usr/bin/nawk
$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=/usr/bin/gawk
$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=/usr/bin/awk
$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=nawk
$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=gawk
$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=awk

seed=
rand() {
  eval "$(
    $AWK -v range="$1" '
      BEGIN {
        srand('"$seed"')
        print "seed=" int(rand() * 2^31)
        print "echo " int(rand() * range)
      }')"
}

rand 100000000
rand 100000000
XEOF
docommand cmdsub02 "$SHELL ./x" 0 IGNORE ""

#
# Check whether "didnl" in func.c is correctly reset.
#
docommand cmdsub03 "$SHELL -c 'echo \$( printf a; printf b )
: \$(
cat << HERE
HERE
)
echo \$( printf a; printf b ) '" 0 "ab\nab\n" ""

remove x
success
