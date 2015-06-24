#!/bin/sh

cstyle.js -l132 -b -K "$@"

#cstyle "$@" 						| \
#	match -hv ': missing blank * comment'		| \
#	match -hv ': line > 80 characters'		| \
#	match -hv ': blank after preprocessor \#'
