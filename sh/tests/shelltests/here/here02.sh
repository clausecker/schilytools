#! /bin/sh
#
# @(#)here02.sh	1.7 17/09/11 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common
. ${SRCROOT}/tests/bin/echo_nonl

#
# Basic tests for here documents
# (ideas taken from "mksh" test suite)
# Thanks to Thorsten Glaser
#

#
# heredoc-1
# Check ordering/content of redundent here documents.
#
cat > x <<"XEOF"
cat << EOF1 << EOF2
hi
EOF1
there
EOF2
XEOF
docommand here01 "$SHELL ./x" 0 "there\n" ""
remove x

#
# heredoc-2
# Check quoted here-doc is protected.
#
cat > x <<"XEOF"
a=foo
cat << 'EOF'
hi\
there$a
stuff
EO\
F
EOF
XEOF
docommand here02 "$SHELL ./x" 0 "\
hi\\
there\$a
stuff
EO\\
F
" ""
remove x

#
# heredoc-3
# Check that newline isn't needed after heredoc-delimiter marker.
#
cat > x <<"XEOF"
cat << EOF
hi
there
XEOF
echo_nonl EOF >> x
docommand here03 "$SHELL ./x" 0 "\
hi
there
" ""
remove x

#
# heredoc-4
# Check that an error occurs if the heredoc-delimiter is missing.
# We do not run this as most shells do not error out
#
cat > x <<"XEOF"
cat << EOF
hi
there
XEOF
#docommand here04 "$SHELL ./x" 0 "hi\nthere\n" ""
remove x

#
# heredoc-5
# Check that backslash quotes a $, ` and \ and kills a \newline
#
cat > x <<"XEOF"
a=BAD
b=ok
cat << EOF
h\${a}i
h\\${b}i
th\`echo not-run\`ere
th\\`echo is-run`ere
fol\\ks
more\\
last \
line
EOF
XEOF
docommand here05 "$SHELL ./x" 0 "\
h\${a}i
h\\oki
th\`echo not-run\`ere
th\\is-runere
fol\\ks
more\\
last line
" ""
remove x

#
# heredoc-6
# Check that \newline in initial here-delim word doesn't imply a quoted here-doc
#
cat > x <<"XEOF"
a=i
cat << EO\
F
h$a
there
EOF
XEOF
docommand here06 "$SHELL ./x" 0 "hi\nthere\n" ""
remove x

#
# heredoc-7
# The Bourne Shell man page says: "After parameter and command substitution is
# done on word..."
# POSIX says: only quote removal is applied to the delimiter.
# So this is an incomatible change from POSIX.
#
cat > x <<"XEOF"
a=b
cat << "E$a"
hi
h$a
hb
E$a
echo done
XEOF
#
# Disabled until bosh introduced a way to switch to this
# POSIX non-compliance.
#docommand here07 "$SHELL ./x" 0 "" ""
#exit
remove x

#
# heredoc-8
# Check that double quoted escaped $ expressions in here
# delimiters are not expanded and match the delimiter.
# POSIX says only quote removal is applied to the delimiter
# (\ counts as a quote).
#
cat > x <<"XEOF"
a=b
cat << "E\$a"
hi
h$a
h\$a
hb
h\b
E$a
echo done
XEOF
docommand here08 "$SHELL ./x" 0 "\
hi
h\$a
h\\\$a
hb
h\\\b
done
" ""
remove x

# 9a: here strings -> ksh specific
# 9b: here strings -> ksh specific
# 9c: here strings -> ksh specific
# 9d: here strings -> ksh specific
# 9e: here strings -> ksh specific
# 9f: here strings -> ksh specific
# 10: also contains here strings -> ksh specific (remove <<< ???)
# 11: mksh specific extension

#
# heredoc-12
# Note: shells differ here.
# bash, ksh88, ksh93 and bosh do it the same
# mksh, dash, zsh differ
#
cat > x <<"XEOF"
set -- a b
nl='
'
IFS=" 	$nl"; n=1
cat <<EOF
$n foo $* foo
$n bar "$*" bar
$n baz $@ baz
$n bla "$@" bla
EOF
IFS=":"; n=2
cat <<EOF
$n foo $* foo
$n bar "$*" bar
$n baz $@ baz
$n bla "$@" bla
EOF
IFS=; n=3
cat <<EOF
$n foo $* foo
$n bar "$*" bar
$n baz $@ baz
$n bla "$@" bla
EOF
XEOF
docommand here12 "$SHELL ./x" 0 '1 foo a b foo
1 bar "a b" bar
1 baz a b baz
1 bla "a b" bla
2 foo a b foo
2 bar "a b" bar
2 baz a b baz
2 bla "a b" bla
3 foo a b foo
3 bar "a b" bar
3 baz a b baz
3 bla "a b" bla
' ""
remove x

#
# heredoc-14
# Check that using multiple here documents works
# mksh tests more here using ksh extensions.
# mksh also tests whether a binary syntax tree max be converted
# back into a command line again.
#
cat > x <<"XEOF"
foo() {
	echo "got $(cat) on stdin"
	echo "got $(cat <&4) on fd#4"
	echo "got $(cat <&5) on fd#5"
}
bar() {
	foo 4<<-a <<-b 5<<-c
	four
	a
	zero
	b
	five
	c
}
bar
XEOF
docommand here14 "$SHELL ./x" 0 "\
got zero on stdin
got four on fd#4
got five on fd#5
" ""
remove x

#
# heredoc-comsub-1
# Tests for here documents in COMSUB, taken from Austin ML
# mksh uses: "EOF)" instead of "EOF\n)" but this is not compatible with
# the POSIX standard that requires "EOF" to be exactly on a separate line.
#
cat > x <<"XEOF"
text=$(cat <<EOF
here is the text
EOF
)
echo = $text =
XEOF
docommand here15 "$SHELL ./x" 0 "= here is the text =\n" ""
remove x

#
# heredoc-comsub-2
# Tests for here documents in COMSUB, taken from Austin ML
# mksh uses: "EOF)" instead of "EOF\n)" but this is not compatible with
# the POSIX standard that requires "EOF" to be exactly on a separate line.
#
cat > x <<"XEOF"
unbalanced=$(cat <<EOF
this paren ) is a problem
EOF
)
echo = $unbalanced =
XEOF
docommand here16 "$SHELL ./x" 0 "= this paren ) is a problem =\n" ""
remove x

#
# heredoc-comsub-3
# Tests for here documents in COMSUB, taken from Austin ML
# mksh uses: "EOF)" instead of "EOF\n)" but this is not compatible with
# the POSIX standard that requires "EOF" to be exactly on a separate line.
#
cat > x <<"XEOF"
balanced=$(cat <<EOF
these parens ( ) are not a problem
EOF
)
echo = $balanced =
XEOF
docommand here17 "$SHELL ./x" 0 "= these parens ( ) are not a problem =\n" ""
remove x

#
# heredoc-comsub-4
# Tests for here documents in COMSUB, taken from Austin ML
# mksh uses: "EOF)" instead of "EOF\n)" but this is not compatible with
# the POSIX standard that requires "EOF" to be exactly on a separate line.
#
cat > x <<"XEOF"
balanced=$(cat <<EOF
these parens \( ) are a problem
EOF
)
echo = $balanced =
XEOF
docommand here18 "$SHELL ./x" 0 "= these parens \( ) are a problem =\n" ""
remove x

#
# heredoc-subshell-2
# Tests for here documents in subshells, taken from Austin ML
# The test heredoc-subshell-1 did test the non-POSIX EOF) as mentioned above
#
cat > x <<"XEOF"
(cat <<EOF
some text
EOF
)
echo end 
XEOF
docommand here19 "$SHELL ./x" 0 "some text\nend\n" ""
remove x

#
# heredoc-subshell-3
# Tests for here documents in subshells, taken from Austin ML
#
cat > x <<"XEOF"
(cat <<EOF; )
some text
EOF
echo end
XEOF
docommand here20 "$SHELL ./x" 0 "some text\nend\n" ""
remove x

#
# heredoc-weird-1
# Tests for here documents, taken from Austin ML
# This fails with ksh93
#
cat > x <<"XEOF"
cat <<END
hello
END\
END
END
echo end
XEOF
docommand here21 "$SHELL ./x" 0 "\
hello
ENDEND
end
" ""
remove x

#
# heredoc-weird-2
# Tests for here documents, taken from Austin ML
#
cat > x <<"XEOF"
cat <<'    END    '
hello
    END    
echo end
XEOF
docommand here22 "$SHELL ./x" 0 "hello\nend\n" ""
remove x

#
# heredoc-weird-4
# Tests for here documents, taken from Austin ML
#
cat > x <<"XEOF"
cat <<END
hello\
END
END
echo end 
XEOF
docommand here23 "$SHELL ./x" 0 "helloEND\nend\n" ""
remove x

#
# heredoc-weird-5
# Tests for here documents, taken from Austin ML
#
# We need to work around a POSIX non-compliance with
# bash, ksh93 and mksh. These shells incorrectly expand
# \E to the ESC character with "echo".
# So we replaced END by FIN
#
cat > x <<"XEOF"
cat <<FIN
hello
\FIN
FIN
echo end
XEOF
docommand here24 "$SHELL ./x" 0 "hello\n\FIN\nend\n" ""
remove x

#
# heredoc-tmpfile-1
# Check that heredoc temp files aren't removed too soon or too late.
# Heredoc in simple command.
# Note that mksh checks for left over files, but we cannot do this as
# the Bourne Shell does not honor TMPDIR for /tmp/ files.
# The code still allows one to check whether /tmp files are removed too early.
#
cat > x <<"XEOF"
pwd > /dev/null	# initialize $PWD
TMPDIR=$PWD 
export TMPDIR
eval '
	cat <<- EOF
	hi
	EOF
	for i in a b ; do
		cat <<- EOF
		more
		EOF
	done
' &
sleep 3
#echo Left overs: *
XEOF
docommand here25 "$SHELL ./x" 0 "hi\nmore\nmore\n" ""
remove x

#
# heredoc-tmpfile-2
# Check that heredoc temp files aren't removed too soon or too late.
# Heredoc in function, multiple calls to function.
# Note that mksh checks for left over files, but we cannot do this as
# the Bourne Shell does not honor TMPDIR for /tmp/ files.
# The code still allows one to check whether /tmp files are removed too early.
#
cat > x <<"XEOF"
pwd > /dev/null	# initialize $PWD
TMPDIR=$PWD 
export TMPDIR
eval '
	foo() {
		cat <<- EOF
		hi
		EOF
	}
	foo
	foo
' &
sleep 2
#echo Left overs: *
XEOF
docommand here26 "$SHELL ./x" 0 "hi\nhi\n" ""
remove x

#
# heredoc-tmpfile-3
# Check that heredoc temp files aren't removed too soon or too late.
# Heredoc in function in loop, multiple calls to function.
# Note that mksh checks for left over files, but we cannot do this as
# the Bourne Shell does not honor TMPDIR for /tmp/ files.
# The code still allows one to check whether /tmp files are removed too early.
#
cat > x <<"XEOF"
pwd > /dev/null	# initialize $PWD
TMPDIR=$PWD
export TMPDIR
eval '
	foo() {
		cat <<- EOF
		hi
		EOF
	}
	for i in a b; do
		foo
		foo() {
			cat <<- EOF
			folks $i
			EOF
		}
	done
	foo
' &
sleep 3
#echo Left overs: *
XEOF
docommand here27 "$SHELL ./x" 0 "hi\nfolks b\nfolks b\n" ""
remove x

#
# heredoc-tmpfile-4
# Check that heredoc temp files aren't removed too soon or too late.
# Backgrounded simple command with here doc
# Note that mksh checks for left over files, but we cannot do this as
# the Bourne Shell does not honor TMPDIR for /tmp/ files.
# The code still allows one to check whether /tmp files are removed too early.
#
cat > x <<"XEOF"
pwd > /dev/null	# initialize $PWD
TMPDIR=$PWD
export TMPDIR
eval '
	cat <<- EOF &
	hi
	EOF
' &
sleep 2
#echo Left overs: *
XEOF
docommand here28 "$SHELL ./x" 0 "hi\n" ""
remove x

#
# heredoc-tmpfile-5
# Check that heredoc temp files aren't removed too soon or too late.
# Backgrounded subshell command with here doc
# Note that mksh checks for left over files, but we cannot do this as
# the Bourne Shell does not honor TMPDIR for /tmp/ files.
# The code still allows one to check whether /tmp files are removed too early.
#
cat > x <<"XEOF"
pwd > /dev/null	# initialize $PWD
TMPDIR=$PWD
export TMPDIR
eval '
	(
	sleep 1 # so parent exits
	echo A
	cat <<- EOF
	hi
	EOF
	echo B
	) &
' &
sleep 3
#echo Left overs: *
XEOF
docommand here29 "$SHELL ./x" 0 "A\nhi\nB\n" ""
remove x

#
# heredoc-tmpfile-6
# Check that heredoc temp files aren't removed too soon or too late.
# Heredoc in pipeline.
# Note that mksh checks for left over files, but we cannot do this as
# the Bourne Shell does not honor TMPDIR for /tmp/ files.
# The code still allows one to check whether /tmp files are removed too early.
#
cat > x <<"XEOF"
pwd > /dev/null	# initialize $PWD
TMPDIR=$PWD
export TMPDIR
eval '
	cat <<- EOF | sed "s/hi/HI/"
	hi
	EOF
' &
sleep 2
#echo Left overs: *
XEOF
docommand here30 "$SHELL ./x" 0 "HI\n" ""
remove x

#
# heredoc-tmpfile-7
# Check that heredoc temp files aren't removed too soon or too late.
# Heredoc in backgrounded pipeline.
# Note that mksh checks for left over files, but we cannot do this as
# the Bourne Shell does not honor TMPDIR for /tmp/ files.
# The code still allows one to check whether /tmp files are removed too early.
#
cat > x <<"XEOF"
pwd > /dev/null	# initialize $PWD
TMPDIR=$PWD
export TMPDIR
eval '
	cat <<- EOF | sed 's/hi/HI/' &
	hi
	EOF
' &
sleep 2
#echo Left overs: *
XEOF
docommand here31 "$SHELL ./x" 0 "HI\n" ""
remove x

#
# heredoc-tmpfile-8
# Check that heredoc temp files aren't removed too soon or too late.
# Heredoc in function, backgrounded call to function.
# Note that mksh checks for left over files, but we cannot do this as
# the Bourne Shell does not honor TMPDIR for /tmp/ files.
# The code still allows one to check whether /tmp files are removed too early.
#
cat > x <<"XEOF"
pwd > /dev/null	# initialize $PWD
TMPDIR=$PWD
export TMPDIR
# Background eval so main shell doesn't do parsing
eval '
	foo() {
		cat <<- EOF
		hi
		EOF
	}
	foo
	# sleep so eval can die
	(sleep 1; foo) &
	(sleep 1; foo) &
	foo
' &
sleep 3
#echo Left overs: *
XEOF
docommand here32 "$SHELL ./x" 0 "hi\nhi\nhi\nhi\n" ""
remove x

#
# heredoc-quoting-unsubst
# Check for correct handling of quoted characters in
# here documents without substitution (marker is quoted).
#
cat > x <<"XEOF"
foo=bar
cat <<-'EOF'
	x " \" \ \\ $ \$ `echo baz` \`echo baz\` $foo \$foo x
EOF
XEOF
docommand here33 "$SHELL ./x" 0 'x " \" \ \\\\ $ \$ `echo baz` \`echo baz\` $foo \$foo x\n' ""
remove x

#
# heredoc-quoting-subst
# Check for correct handling of quoted characters in
# here documents with substitution (marker is not quoted).
#
cat > x <<"XEOF"
foo=bar
cat <<-EOF
	x " \" \ \\ $ \$ `echo baz` \`echo baz\` $foo \$foo x
EOF
XEOF
docommand here34 "$SHELL ./x" 0 'x " \" \ \ $ $ baz `echo baz` bar $foo x\n' ""
remove x

#
# Check whether the quoting state is reset while expanding a here document.
#
cat > x <<"XEOF"
var="you"
echo "`cat <<EOF
Hi ${var}
EOF
`"
XEOF
docommand here35 "$SHELL ./x" 0 "Hi you\n" ""
remove x

cat > x <<"XEOF"
var="you"
echo "$(cat <<EOF
Hi ${var}
EOF
)"
XEOF
docommand here36 "$SHELL ./x" 0 "Hi you\n" ""
remove x

cat > x <<"XEOF"
read var <<EOF
$(echo Hi)
EOF
echo "${var-nothing was read}"
XEOF
docommand here37 "$SHELL ./x" 0 "Hi\n" ""
remove x

cat > x <<"XEOF"
for N in 1 4; do echo "$( cat <<- EOF
	Mary had ${N}
	little
	lamb$( [ $N -gt 1 ] && echo s )
	EOF
	)"; done
XEOF
docommand here38 "$SHELL ./x" 0 "Mary had 1
little
lamb
Mary had 4
little
lambs
" ""
remove x

cat > x <<"XEOF"
for N in 1 4; do echo "` cat <<- EOF
	Mary had ${N}
	little
	lamb\` [ $N -gt 1 ] && echo s \`
	EOF
	`"; done
XEOF
docommand here39 "$SHELL ./x" 0 "Mary had 1
little
lamb
Mary had 4
little
lambs
" ""
remove x

cat > x <<"XEOF"
eval 'f() {
cat << EOF
1
2
3
EOF
}'

f
XEOF
docommand here40 "$SHELL ./x" 0 "1\n2\n3\n" ""
remove x

cat > x <<"XEOF"
XEOF
#docommand here01 "$SHELL ./x" 0 "" ""
remove x

success
