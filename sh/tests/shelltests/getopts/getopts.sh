#! /bin/sh
#
# @(#)getopts.sh	1.2 16/06/04 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

docommand G1 "$SHELL getopts-1" 0 "" "getopts-1: illegal option -- s\n"
docommand G2 "$SHELL getopts-2" 0 "" ""
docommand G3 "$SHELL getopts-3" 0 "\$? '0' \$OPT '?' \$OPTARG '' \$OPTIND '2'\n" ""
docommand G4 "$SHELL getopts-4" 0 "\$? '0' \$OPT 'a' \$OPTARG '' \$OPTIND '2'\n" ""
docommand G5 "$SHELL getopts-5" 0 "\$? '0' \$OPT ':' \$OPTARG '' \$OPTIND '2'\n" ""
docommand G6 "$SHELL getopts-6" 0 "\$? '0' \$OPT 'f' \$OPTARG 'argument' \$OPTIND '3'\n" ""
docommand G7 "$SHELL getopts-7" 0 "\$? '0' \$OPT 'f' \$OPTARG 'ile' \$OPTIND '2'\n" ""
docommand G8 "$SHELL getopts-8" 0 "\$? '0' \$OPT 'f' \$OPTARG 'argument' \$OPTIND '3'\n" ""
docommand G9 "$SHELL getopts-9" 0 "\$? '0' \$OPT '1000' \$OPTARG 'argument' \$OPTIND '3'\n" ""
docommand G10 "$SHELL getopts-10" 0 "\$? '0' \$OPT '1000' \$OPTARG 'argument' \$OPTIND '3'\n" ""

success
