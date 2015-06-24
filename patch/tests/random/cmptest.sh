#!/bin/sh
#
# cmptest @(#)cmptest.sh	1.2 15/06/03 Copyright 2015 J. Schilling
#
# Usage: cmptest	---> runs 1000 test loops
#	 cmptest #	---> runs # test loops
#
# A random file test with patch -DXXX usage.
# A file is generated, then modified.
# Then a patch file is created from original and new file.
# A reference patch implementation is called with -DXXX and the output
# is compared to the output from out test patch. Gpatch is buggy, so we
# sometimes need to use our test patch as reference.
#
# Finally cpp is used to check whether original and new file can be
# reconstructed from the #ifdef XXX ... #else ... #endif parts.
#
# The diff type is random, this allows to check all diff types.
#

trap cleanup EXIT INT HUP

cleanup() {
	rm -f saved_orig changed patch_file expected original failure xo xm xof xmf xef
}

rrand() {
	nawk '
	function random(low, range) {
		return int(range * rand()) + low
	}

	BEGIN {
		base = ARGV[1]
		max = ARGV[2]
		seed = ARGV[3]

		srand()		# Initialze with current time
		s = srand()	# get previous seed
		s = s + seed	# Current time + seed
		srand(s)	# Better new seed

		x = random(base, max)
		print x
	}
	' "$@"
}

makefile() {
	nawk '
	BEGIN {
		nflines = ARGV[1]
		for (i = 1; i <= nflines; i++) {
			printf("This is original line %d\n", i);
		}
	}
	' "$@"
}

changefile() {
	nawk '
	function random(n) {
		return int(n * rand())
	}

	BEGIN {
		nflines = ARGV[1]
		chlines = ARGV[2]
		seed = chlines

		ARGV[1] = ""	# Do not use as filename
		ARGV[2] = ""	# Do not use as filename

		srand()		# Initialze with current time
		s = srand()	# get previous seed
		s = s + seed	# Current time + seed
		srand(s)	# Better new seed

		for (i = 1; i <= nflines; i++) {
			change_line[i] = 0
		}

		while (chlines > 0) {
			i = random(nflines)
			if (change_line[i] == 0) {
				chlines--
				change_line[i] = 1
			}
		}
		i = 1;
	}

	{
		if (change_line[i] != 0) {
			what = random(3)
			if (what == 0) {
				print "Modified, was: " $0
			} else if (what == 1) {
				print $0
				print "New line, inserted after: " $0
			}
		} else {
			print $0
		}
		i++
		next
	}
	' "$@"
}

nlines=$$	# seed startup helper, awk would get seed based on time_t
maxlines=5000	# Longest file for our tests
maxch=4		# Max. 25% of all lines are changed

#
# Diff Program to use. Solaris diff -U0 has bugs, so use our fixed Solaris diff
# from the SCCS distribution.
#
diff=/opt/schily/ccs/bin/diff
type $diff > /dev/null 2> /dev/null
[ $? -ne 0 ] && diff=diff	# fallback to probably defective system diff
LC_ALL=C $diff -? 2>&1 | grep -i Option > /dev/null
if [ $? -ne 0 ]; then
	echo "No working diff program found"
	exit 1
fi
echo "Using diff programm: $diff"
#
# Reference patch program, note that gpatch has problems itself
# and fails with -diff -C0
#
rpatch=gpatch
LC_ALL=C $rpatch -? 2>&1 | grep -i Option > /dev/null
if [ $? -ne 0 ]; then
	echo "Reference patch program \"$rpatch\" not working"
	exit 1
fi
echo "Using reference patch programm: $rpatch"
#
# Test patch implementation:
# Add -W+ to permit POSIX + enhancements for "patch -s"
#
tpatch="eval ../../OBJ/"`../../../conf/oarch.sh`"/spatch -W+"
silent=-s
LC_ALL=C $tpatch -? 2>&1 | grep -i Option > /dev/null
if [ $? -ne 0 ]; then
	echo "Test patch program \"$tpatch\" not working"
	exit 1
fi
echo "Using test patch programm: $tpatch"

idx=0
maxidx=${1:-1000}
while [ $idx -le $maxidx ]
do
	nlines=`rrand 10 $maxlines $nlines`

	max_changes=`expr $nlines / $maxch`
	changes=`rrand 1 $max_changes $nlines`

	makefile $nlines > original
	cp original saved_orig

	echo Test $idx:  testing nlines=$nlines changes=$changes

	dtype=`rrand 0 93983 $nlines`	# rrand 0 6 would be of bad quality, so
	dtype=`expr $dtype \% 6`	# use "rrand 0 bigprime % 6" instead

	if [ $dtype -eq 0 ]; then
		dtype="-c"
	elif [ $dtype -eq 1 ]; then
		dtype="-u"
	elif [ $dtype -eq 2 ]; then
		dtype="-C0"
	elif [ $dtype -eq 3 ]; then
		dtype="-U0"
	elif [ $dtype -eq 4 ]; then
		dtype="-e"
	else
		dtype="  "
	fi

	changefile $nlines $changes original > changed
	echo diff $dtype original changed
	echo "Index:original" > patch_file
	$diff $dtype original changed >> patch_file

	cp saved_orig original
	#
	# $rpatch: gpatch is buggy and does not support "diff -C0"
	#
	$rpatch -D XXX original < patch_file || $tpatch $silent -D XXX original < patch_file
	#
	# gpatch never uses "#endif /* XXX */", while our patch
	# depends on wether it is in POSIX mode, where the comment is missing
	#
#	sed -e 's^#endif$^#endif /* XXX */^g' < original > expected
	cp original expected

	if [ "$dtype" = "-e" ]; then
		diff changed original > xef
		ret=$?
		if [ $ret -ne 0 ]; then
			echo Test $idx: Reference Patch $dtype did not restore original
			[ $ret -eq 1 ] && trap 0
			exit 1
		fi
	else
		cpp original | grep This > xo
		cpp -DXXX original | grep This > xm
		diff saved_orig xo > xof
		ret=$?
		if [ $ret -ne 0 ]; then
			echo Test $idx: Reference Patch $dtype did not restore original
			[ $ret -eq 1 ] && trap 0
			exit 1
		fi
		diff changed xm > xmf
		ret=$?
		if [ $ret -ne 0 ]; then
			echo Test $idx: Reference Patch $dtype did not restore changed
			[ $ret -eq 1 ] && trap 0
			exit 1
		fi
	fi
	

	echo Patching file...
	cp saved_orig original
	$tpatch $silent -D XXX original < patch_file
	ret=$?
	if [ $ret -ne 0 ]; then
		echo Test $idx: Patch $dtype returned $ret
		diff original expected > failure
		trap 0
		exit 1
	fi

	diff original expected > failure
	ret=$?
	if [ $ret -ne 0 ]; then
		echo Test $idx: diff $dtype returned $ret
		[ $ret -eq 1 ] && trap 0
		exit 1
	fi
	idx=`expr $idx + 1`
done
idx=`expr $idx - 1`
echo Test succeeded after $idx runs...