#! /bin/sh
#
# @(#)syntax.sh	1.3 18/10/09 Copyright 2017 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether syntay detection works as expected.
# These tests have been taken from NetBSD.
#
docommand syn00 "$SHELL -c 'true; fi'" "!=0" "" NONEMPTY
docommand syn01 "$SHELL -c 'false; fi'" "!=0" "" NONEMPTY
docommand syn02 "$SHELL -c 'false; then echo wut'" "!=0" "" NONEMPTY
docommand syn03 "$SHELL -c 'true; then echo wut'" "!=0" "" NONEMPTY
docommand syn04 "$SHELL -c 'true; do echo wut'" "!=0" "" NONEMPTY
docommand syn05 "$SHELL -c 'true; then'" "!=0" "" NONEMPTY
docommand syn06 "$SHELL -c 'true; else'" "!=0" "" NONEMPTY
docommand syn07 "$SHELL -c 'true; do'" "!=0" "" NONEMPTY
docommand syn08 "$SHELL -c 'true; done'" "!=0" "" NONEMPTY
docommand syn09 "$SHELL -c ': ; }'" "!=0" "" NONEMPTY
docommand syn10 "$SHELL -c ': ; )'" "!=0" "" NONEMPTY

docommand syn11 "$SHELL -c 'true& fi'" "!=0" "" NONEMPTY
docommand syn12 "$SHELL -c 'false& fi'" "!=0" "" NONEMPTY
docommand syn13 "$SHELL -c 'false& then echo wut'" "!=0" "" NONEMPTY
docommand syn14 "$SHELL -c 'true& then echo wut'" "!=0" "" NONEMPTY
docommand syn15 "$SHELL -c 'true& do echo wut'" "!=0" "" NONEMPTY
docommand syn16 "$SHELL -c 'true& then'" "!=0" "" NONEMPTY
docommand syn17 "$SHELL -c 'true& else'" "!=0" "" NONEMPTY
docommand syn18 "$SHELL -c 'true& do'" "!=0" "" NONEMPTY
docommand syn19 "$SHELL -c 'true& done'" "!=0" "" NONEMPTY
docommand syn20 "$SHELL -c ':&}'" "!=0" "" NONEMPTY
docommand syn21 "$SHELL -c ':&)'" "!=0" "" NONEMPTY

docommand syn30 "$SHELL -c 'case x in <|() ;; esac'" "!=0" "" NONEMPTY
docommand syn31 "$SHELL -c 'case x in ((|)) ;; esac'" "!=0" "" NONEMPTY
docommand syn32 "$SHELL -c 'case x in _|() ;; esac'" "!=0" "" NONEMPTY
docommand syn33 "$SHELL -c 'case x in ()|() ;; esac'" "!=0" "" NONEMPTY
docommand syn34 "$SHELL -c 'case x in -|;) ;; esac'" "!=0" "" NONEMPTY
docommand syn35 "$SHELL -c 'case x in (;|-) ;; esac'" "!=0" "" NONEMPTY
docommand syn36 "$SHELL -c 'case x in ;;|;) ;; esac'" "!=0" "" NONEMPTY
docommand syn37 "$SHELL -c 'case x in (|| | ||) ;; esac'" "!=0" "" NONEMPTY
docommand syn38 "$SHELL -c 'case x in (<<|>>) ;; esac'" "!=0" "" NONEMPTY
docommand syn39 "$SHELL -c 'case x in (&&|&) ;; (|||>&) ;; &) esac'" "!=0" "" NONEMPTY
docommand syn40 "$SHELL -c 'case x in (>||<) ;; esac'" "!=0" "" NONEMPTY
docommand syn41 "$SHELL -c 'case x in( || | || | || | || | || );; esac'" "!=0" "" NONEMPTY
docommand syn42 "$SHELL -c 'case x in (||| ||| ||| ||| ||) ;; esac'" "!=0" "" NONEMPTY
docommand syn43 "$SHELL -c 'case x in <> |    
) ;; esac'" "!=0" "" NONEMPTY

#
# Those that follow are not syntax errors, and should be parsed properly.
#
o_posix=
[ "$is_bosh" = true ] && o_posix="-o posix"

#
# Der historische Bourne Shell parst "{" hier als KTSYM
docommand syn100 "$SHELL -c 'case fi in ({|}) ;; (!) ;; esac'" 0 "" ""
docommand syn101 "$SHELL -c 'case esac in ([|]);; (][);; !!!|!!!|!!!|!!!);; esac'" 0 "" ""
docommand syn102 "$SHELL ${o_posix} -c 'case then in ({[]]}) ;; (^^);; (^|^);; ([!]);; (-);; esac'" 0 "" ""
#
# Der historische Bourne Shell parst " while   )" hier als WHSYM
docommand syn103 "$SHELL -c 'case while in while   );;(if|then|elif|fi);;(do|done);; esac'" 0 "" ""
docommand syn104 "$SHELL -c 'case until in(\$);;(\$\$);;(\$4.50);;(1/2);;0.3333);;esac'" 0 "" ""
#
# Der historische Bourne Shell parst !) hier als NOTSYM
docommand syn105 "$SHELL -c 'case return in !);; !\$);; \$!);; !#);; (@);; esac'" 0 "" ""
docommand syn106 "$SHELL -c 'case break in (/);; (\/);; (/\|/\));; (\\//);; esac'" 0 "" ""

docommand syn110 "$SHELL -c 'r=\"!\"; case \$r in !) echo \$r 1 ;; !\$) echo \$r 2;; \$!) echo \$r 3;; !#) echo \$r 4;; (@) echo \$r 5;; *) echo \$r 6 ;; esac'" 0 "! 1\n" ""
docommand syn111 "$SHELL -c 'r=\"!\$\"; case \$r in !) echo \$r 1 ;; !\$) echo \$r 2;; \$!) echo \$r 3;; !#) echo \$r 4;; (@) echo \$r 5;; *) echo \$r 6 ;; esac'" 0 "!\$ 2\n" ""
docommand syn112 "$SHELL -c 'r=\"\$!\"; case \$r in !) echo \$r 1 ;; !\$) echo \$r 2;; \$!) echo \$r 3;; !#) echo \$r 4;; (@) echo \$r 5;; *) echo \$r 6 ;; esac'" 0 "3\n" ""	# $! expands to non-existing last background
docommand syn113 "$SHELL -c 'r=\"!#\"; case \$r in !) echo \$r 1 ;; !\$) echo \$r 2;; \$!) echo \$r 3;; !#) echo \$r 4;; (@) echo \$r 5;; *) echo \$r 6 ;; esac'" 0 "!# 4\n" ""
docommand syn114 "$SHELL -c 'r=\"@\"; case \$r in !) echo \$r 1 ;; !\$) echo \$r 2;; \$!) echo \$r 3;; !#) echo \$r 4;; (@) echo \$r 5;; *) echo \$r 6 ;; esac'" 0 "@ 5\n" ""
docommand syn115 "$SHELL -c 'r=\"bla\"; case \$r in !) echo \$r 1 ;; !\$) echo \$r 2;; \$!) echo \$r 3;; !#) echo \$r 4;; (@) echo \$r 5;; *) echo \$r 6 ;; esac'" 0 "bla 6\n" ""
docommand syn116 "$SHELL -c 'r=\"(@)\"; case \$r in !) echo \$r 1 ;; !\$) echo \$r 2;; \$!) echo \$r 3;; !#) echo \$r 4;; (@) echo \$r 5;; *) echo \$r 6 ;; esac'" 0 "(@) 6\n" ""

#
# The AT&T hack in _macro() from the 1980s together with the POSIX change in
# case caused this to fail when the shell is in strict POSIX mode:
#
docommand syn140 "$SHELL -c 'x=; case \$x in \"\") echo OK;; *) echo BAD; esac'" 0 "OK\n" ""
docommand syn141 "$SHELL ${o_posix}  -c 'x=; case \$x in \"\") echo OK;; *) echo BAD; esac'" 0 "OK\n" ""

success
