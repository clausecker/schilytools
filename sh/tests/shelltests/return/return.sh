#! /bin/sh
#
# @(#)return.sh	1.3 17/09/06 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

if [ "$is_bosh" = true ]; then
docommand r1 "$SHELL -c 'echo a; return; echo b'" 1 "a\n" IGNORE
else
echo "Skipping test r1 as this shell is $shell and not bosh and the behavior is unspecified"
fi

cat > x <<"XEOF"
echo hi
return
XEOF
docommand r50 "$SHELL -c 'fn() { . ./x; echo one; }; fn; echo two'" 0 "hi\none\ntwo\n" ""
cat > test.dot <<"XEOF"
fn() {
	. ./x
	echo one
}
fn
echo two
XEOF
docommand r51 "$SHELL ./test.dot" 0 "hi\none\ntwo\n" ""

remove x test.dot

success
