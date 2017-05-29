#! /bin/sh
#
# @(#)local.sh	1.2 17/05/28 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether local variables work
#

cat > ./echo1  <<-"XEOF"
	echo $var
XEOF
chmod +x ./echo1

cat > ./echo2 <<-"XEOF"
	#!/bin/sh
	echo $var
XEOF
chmod +x ./echo2

#
# Check for Solaris bug #3000663:
# unexported variables are available to simple shell scripts
#
docommand local00 "$SHELL -c 'var=init; echo \$var; ./echo1'" 0 "init\n\n" ""

#
# Check for the bug that was introduced while fixing Solaris bug #3000663:
# unexported variables are available to simple shell scripts if made readonly
#
docommand local01 "$SHELL -c 'var=init; readonly var; echo \$var; ./echo1'" 0 "init\n\n" ""

#
# Basic "local var" test: original value must reappear after the function ends
#
docommand local10 "$SHELL -c 'var=init; f() { local var; var=neu; echo \$var; } ; f; echo \$var'" 0 "neu\ninit\n" ""

#
# Unset local var: "unset" must only affect the local value inside the function
#
docommand local11 "$SHELL -c 'var=init; f() { local var; var=neu; echo \$var; unset var; echo \$var; } ; f; echo \$var'" 0 "neu\n\ninit\n" ""

#
# Unexported local variables in function must not be visible in scripts
#
docommand local12 "$SHELL -c 'var=init; f() { local var; var=neu; echo \$var; ./echo1; } ; f; echo \$var'" 0 "neu\n\ninit\n" ""

#
# Exported variable is exported in local variant as well
#
docommand local13 "$SHELL -c 'var=init; export var; f() { local var; var=neu; echo \$var; ./echo1; } ; f; echo \$var'" 0 "neu\nneu\ninit\n" ""
docommand local14 "$SHELL -c 'var=init; export var; f() { local var; var=neu; echo \$var; ./echo2; } ; f; echo \$var'" 0 "neu\nneu\ninit\n" ""

#
# Only the local variant is exported
#
docommand local15 "$SHELL -c 'var=init; f() { local var; export var; var=neu; echo \$var; ./echo1; } ; f; echo \$var; ./echo1'" 0 "neu\nneu\ninit\n\n" ""
docommand local16 "$SHELL -c 'var=init; f() { local var; export var; var=neu; echo \$var; ./echo2; } ; f; echo \$var; ./echo2'" 0 "neu\nneu\ninit\n\n" ""

#
# Unset var: local variable in function must be reverted into unset variable
#
docommand local17 "$SHELL -c 'unset var; f() { local var; var=neu; echo \$var; } ; f; echo \$var'" 0 "neu\n\n" ""

#exit
remove	./echo1 ./echo2
success
