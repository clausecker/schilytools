h49899
s 00001/00001/00129
d D 1.5 18/04/30 13:07:11 joerg 5 4
c Pfadnamen quoten, damit SPACE darin sein kann
e
s 00001/00001/00129
d D 1.4 15/06/03 00:06:44 joerg 4 3
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00129
d D 1.3 15/06/01 23:55:23 joerg 3 2
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00003/00003/00127
d D 1.2 11/10/21 23:07:39 joerg 2 1
c prs -d:DI: Tests sind nun POSIX konform
e
s 00130/00000/00000
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

# keywords.sh:  Testing for correct expansion of formats for prs -d.

# Import common functions & definitions.
D 4
. ../common/test-common
E 4
I 4
. ../../common/test-common
E 4


sid=1.1

expands_to () {
    # $1 -- label
    # $2 -- format
    # $3 -- expansion
docommand $1 "${vg_prs} \"-d$2\" -r${sid} s.1" 0 "$3" IGNORE
}

remove s.1 p.1 1 z.1

# Create file
echo "Descriptive Text" > DESC
docommand P1 "${admin} -n -tDESC s.1" 0 "" ""
remove DESC

docommand P2 "${vg_prs} -d':M:\n' s.1" 0 "1
" ""

docommand P3 "${get} -e s.1" 0 "1.1\nnew delta 1.2\n0 lines\n" IGNORE
echo "hello from %M%" >> 1
docommand P4 "${delta} -y s.1" 0 "1.2\n1 inserted\n0 deleted\n0 unchanged\n" ""

D 3
expands_to z1 :PN:      `../../testutils/realpwd`"/s.1\n"
E 3
I 3
D 5
expands_to z1 :PN:      `${SRCROOT}/tests/testutils/realpwd`"/s.1\n"
E 5
I 5
expands_to z1 :PN:      "`${SRCROOT}/tests/testutils/realpwd`/s.1\n"
E 5
E 3


expands_to X1  :I:      "1.1\n"
expands_to X1r :R:      "1\n"
expands_to X1l :L:      "1\n"
expands_to X1b :B:      "\n"
expands_to X1s :S:      "\n"
expands_to X2  :BF:     "no\n"
D 2
expands_to X3  :DI:     "\n"
E 2
I 2
expands_to X3  :DI:     "//\n"
E 2
expands_to X4  :DL:     "00000/00000/00000\n"
expands_to X5  :DT:     "D\n"
expands_to X7  :J:      "no\n"
expands_to X8  :LK:     "none\n"
expands_to X9  :MF:     "no\n"
expands_to X10 :MP:     "none\n"
expands_to X11 :MR:     "\n"
expands_to X12 :Z:      '@(#)\n'
expands_to X13 'x\\ny'  "x\ny\n"
expands_to X14 ':Q:'    '\n'
expands_to X15 'x\ty'   'x\ty\n'
expands_to X16 'x\\ty'   'x\ty\n'
expands_to X17 'x\\ny'   'x\ny\n'
expands_to X18 ':FD:'   'Descriptive Text\n\n'

remove got.stdout expected.stdout
echo_nonl Z1...
${vg_prs}  -d'\\' s.1 > got.stdout 2>got.stderr || fail prs failed.
echo \\            > expected.stdout || miscarry redirection to expected.stdout
diff expected.stdout got.stdout >/dev/null || fail stdout format error.
test -s got.stderr && fail expected empty stderr output
remove got.stderr got.stdout expected.stdout 
echo passed


# Make sure prs accepts an empty "-r" option.
docommand Z2 "${vg_prs} -r -d':M:\n' s.1" 0 "1
" ""

remove s.1
docommand  K0 "cp sample_foo s.1" 0 IGNORE IGNORE
docommand  K1 "${admin} -fqQFLAG s.1" 0 IGNORE IGNORE

expands_to K2 ':Dy:'   '02\n'
expands_to K3 ':Dm:'   '03\n'
expands_to K4 ':Dd:'   '16\n'

expands_to K5 ':Th:'   '21\n'
expands_to K6 ':Tm:'   '39\n'
expands_to K7 ':Ts:'   '36\n'


docommand _1 "${get} -e -x1.1,1.2 -r1.4 s.1" 0 IGNORE IGNORE
echo hello >> 1 || miscarry "could not write to file '1'"
#docommand _2 "${delta} -g1.1 s.1" 0 IGNORE IGNORE
docommand _2 "${delta} -y'You only Live Twice'  s.1" 0 IGNORE IGNORE

docommand _3 "${get} -e -i1.3 -x1.2,1.1 -r1.5 s.1" 0 IGNORE IGNORE
echo foobar >> 1 || miscarry "could not write to file '1'"
#docommand _4 "${delta} -g1.1 s.1" 0 IGNORE IGNORE
docommand _4 "${delta} -y' Roundabout'  s.1" 0 IGNORE IGNORE

docommand _5 "${admin} -fi -ftMODULE_TYPE -fl1 s.1" 0 IGNORE IGNORE
docommand _6 "${admin} -asanta s.1" 0 IGNORE IGNORE


sid=1.5


# Excluded deltas
expands_to K8 ':Dx:'   '2 1\n'

# Ignored deltas
# expands_to K9 ':Dg:'   '2\n'
expands_to K9 ':Dg:'   '\n'

# Authorised user list
expands_to K10 ':UN:'   'santa\n\n'

# Module type (t) flag
expands_to K11 ':Y:'   'MODULE_TYPE\n'

# KF - keyword warning flag
expands_to K12 ':KF:'   'yes\n'

expands_to K13 ':LK:'   '1\n'
expands_to K14 ':Q:'   'QFLAG\n'


D 2
expands_to K15 ':DI:'   '/2 1\n'
E 2
I 2
expands_to K15 ':DI:'   '/2 1/\n'
E 2

sid=1.6

D 2
expands_to K16 ':DI:'   '3/2 1\n'
E 2
I 2
expands_to K16 ':DI:'   '3/2 1/\n'
E 2

## 
## 
remove s.1 p.1 z.1 1 command.log
success
E 1
