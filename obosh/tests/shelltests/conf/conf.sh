#! /bin/sh
#
# @(#)conf.sh	1.1 16/07/31 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether our test shell creates the same "configure"
# results as the system shell. We assume that the system shell is at least
# correct enough to work with "configure".
#

remove config.*

#
# First run the system shell
#
docommand conf00 "$SHELL -c '../../../../autoconf/configure'" 0 IGNORE ""

mv xconfig.h	xconfig.0
mv rules.cnf	rules.0

remove config.*

docommand -noremove conf01 "$SHELL -c 'CONFIG_SHELL=$SHELL $SHELL ../../../../autoconf/configure'" 0 IGNORE ""

diff rules.0 rules.cnf || fail "$cmd_label: rules.cnf format error with $SHELL"
diff xconfig.0 xconfig.h || fail "$cmd_label: xconfig.h format error with $SHELL"

remove config.* xconfig.h xconfig.0 rules.cnf rules.0

success
