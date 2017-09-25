#! /bin/sh
#
# @(#)return.sh	1.5 17/09/10 Copyright 2016 J. Schilling
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

#
# Check whether return only returns from function and not from dot script
# as well.
#
cat > x <<"XEOF"
f() {
echo "In f"
return
}
echo "Start"
f
echo "End"
XEOF
docommand r51 "$SHELL ./x" 0 "Start\nIn f\nEnd\n" ""

remove x

#
# Check whether continue in a dot script does not cause
# a return from the dot script.
#
cat > x <<"XEOF"
echo "begin dot"
x=0
while [ "$((x+=1))" -le 3 ]; do
        echo "$x"
        continue
done
echo "end dot"
XEOF
docommand r51 "$SHELL -c '. ./x'" 0 "begin dot\n1\n2\n3\nend dot\n" ""

remove x

success
