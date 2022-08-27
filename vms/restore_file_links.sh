#!/bin/sh
#                                               22 October 2006.  SMS.
#
#    Restore symbolic links from VMSTAR-extracted link files.  Adjust
# the names of files which were renamed to "*_link".
#
find . -exec grep -l '^*** This file is a link to ' {} \; | \
(
read name

while [ -n "${name}" ] ; do

    link=` echo "${name}" | sed -e 's/_link$//' `
    targ=` sed -e 's/^*** This file is a link to //' ${name} `
    rm "${name}" ; ln -s "${targ}" "${link}"

    read name

done
)
