#! /bin/sh
#
# @(#)options.sh	1.2 16/06/16 2015-2016 J. Schilling
#

# options.sh:  Testing good and bad options

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C

#
# Empty patch
#
docommand OP1 "${patch} < /dev/null" 0 "" "Hmm...  I can't seem to find a patch in there anywhere.\n"

#
# Empty patch - silent mode
#
docommand OP2 "${patch} -s < /dev/null" 0 "" "  I can't seem to find a patch in there anywhere.\n"

#
# Illegal option -q - check exit code only
#
docommand OP3 "${patch} -q < /dev/null" 2 "" IGNORE

#
# Illegal option -q - check usage in Wall-plus mode
#
docommand OP4 "${patch} -W+ -q < /dev/null" 2 "" "patch: unrecognized option \`-q'
Usage: patch [-bEflNRsSv] [-c|-e|-n|-u]
	[-z backup-ext] [-B backup-prefix] [-d directory]
	[-D symbol] [-Fmax-fuzz] [-i patchfile] [-o out-file] [-p[strip-count]]
	[-r rej-name] [origfile] [patchfile] [[+] [options] [origfile]...]
	[-W+] [-Wall] [-Wposix] [-W-posix]
"

#
# Illegal option -q - check usage in POSIX mode
#
docommand OP5 "${patch} -Wposix -q < /dev/null" 2 "" "patch: unrecognized option \`-q'
Usage: patch [-blNR] [-c|-e|-n|-u] [-d dir] [-D define] [-i patchfile]
	[-o outfile] [-p num] [-r rejectfile] [file]
"

#
# Option -F - legal in Wall-plus mode
#
docommand OP6 "${patch} -W+ -F5 < /dev/null" 0 "" "Hmm...  I can't seem to find a patch in there anywhere.\n"

#
# Option -F - illegal in POSIX mode
#
docommand OP6 "${patch} -Wposix -F5 < /dev/null" 2 "" "patch: unrecognized option \`-F5'
Usage: patch [-blNR] [-c|-e|-n|-u] [-d dir] [-D define] [-i patchfile]
	[-o outfile] [-p num] [-r rejectfile] [file]
"

#
# Option -py - illegal argument in POSIX mode
#
docommand OP7 "${patch} -Wposix -py < /dev/null" 2 "" "Not a number 'y'.\n"

success
