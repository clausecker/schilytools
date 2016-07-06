#! /bin/sh
#
# @(#)break02.sh	1.3 16/06/29 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether the supported operators work.
# (ideas taken from "mksh" test suite)
# Thanks to Thorsten Glaser
#

#
# Break test for loops
#
cat > x <<"XEOF"
for i in a b c; do echo $i; break; echo bad-$i; done
echo end-1
for i in a b c; do echo $i; break 1; echo bad-$i; done
echo end-2
for i in a b c; do
	for j in x y z; do
		echo $i:$j
		break
		echo bad-$i
	done
	echo end-$i
done
echo end-3
XEOF
docommand break-2-00 "$SHELL ./x" 0 'a\nend-1\na\nend-2\na:x\nend-a\nb:x\nend-b\nc:x\nend-c\nend-3\n' ""
remove x

#
# Break test for nexted loops
#
cat > x <<"XEOF"
for i in a b c; do
	for j in x y z; do
		echo $i:$j
		break 2
		echo bad-$i
	done
	echo end-$i
done
echo end
XEOF
docommand break-2-01 "$SHELL ./x" 0 'a:x\nend\n' ""
remove x

#
# Try to break out of more loops than present
#
cat > x <<"XEOF"
for i in a b c; do echo $i; break 2; echo bad-$i; done
echo end
XEOF
docommand break-2-02 "$SHELL ./x" 0 'a\nend\n' ""
remove x

#
# Check for error when break arg is not a number
#
cat > x <<"XEOF"
LC_ALL=C
for i in a b c; do echo $i; break abc; echo more-$i; done
echo end
XEOF
docommand -noremove break-2-03 "$SHELL ./x" "!=0" 'a\n' IGNORE
err=`grep 'abc:.*bad number' got.stderr`
if [ -z "$err" ]; then
	if [ "$is_bosh" = true ]; then
		fail "Test $cmd_label failed: wrong error message"
	else
		err=`grep 'number' got.stderr`
		if [ -z "$err" ]; then
			fail "Test $cmd_label failed: wrong error message"
		fi
	fi
fi
remove x
do_remove

#
# Check whether continue works
#
cat > x <<"XEOF"
for i in a b c; do echo $i; continue; echo bad-$i ; done
echo end-1
for i in a b c; do echo $i; continue 1; echo bad-$i; done
echo end-2
for i in a b c; do
	for j in x y z; do
		echo $i:$j
		continue
		echo bad-$i-$j
	done
	echo end-$i
done
echo end-3
XEOF
docommand break-2-04 "$SHELL ./x" 0 'a
b
c
end-1
a
b
c
end-2
a:x
a:y
a:z
end-a
b:x
b:y
b:z
end-b
c:x
c:y
c:z
end-c
end-3
' ""
remove x

#
# Check of continue continues nested loops
#
cat > x <<"XEOF"
for i in a b c; do 
	for j in x y z; do
		echo $i:$j
		continue 2
		echo bad-$i-$j
	done
	echo end-$i
done
echo end
XEOF
docommand break-2-05 "$SHELL ./x" 0 'a:x
b:x
c:x
end
' ""
remove x

#
# Try to continue more loops than present
#
cat > x <<"XEOF"
for i in a b c; do echo $i; continue 2; echo bad-$i; done
echo end
XEOF
docommand break-2-06 "$SHELL ./x" 0 'a\nb\nc\nend\n' ""
remove x

#
# Check for error when continue arg is not a number
#
cat > x <<"XEOF"
LC_ALL=C
for i in a b c; do echo $i; continue abc; echo more-$i; done
echo end
XEOF
docommand -noremove break-2-07 "$SHELL ./x" "!=0" 'a\n' IGNORE
err=`grep 'abc:.*bad number' got.stderr`
if [ -z "$err" ]; then
	if [ "$is_bosh" = true ]; then
		fail "Test $cmd_label failed: wrong error message"
	else
		err=`grep 'number' got.stderr`
		if [ -z "$err" ]; then
			fail "Test $cmd_label failed: wrong error message"
		fi
	fi
fi
remove x
do_remove

cat > x <<"XEOF"
XEOF
#docommand break-2-00 "$SHELL ./x" 0 '' ""
remove x

success
