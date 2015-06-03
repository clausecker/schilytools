#! /bin/sh

# flags.sh:  Testing for setting /unsetting flags.

# Import common functions & definitions.
. ../../common/test-common

g=new.txt
s=s.$g
p=p.$g
remove foo $s $g $p [zx].$g

# Create SCCS file with a substituted keyword.
echo '%M%' >foo
docommand k1 "${admin} -ifoo $s" 0 "" ""



## Sourceforge bug number 121599: "admin -dla" causes crash.
## Project: CSSC
## Category: SCCS incompatibility
## Status: Open
## Resolution: None
## Bug Group: defect
## Priority: 5
## Summary: "admin -dla" crashs
## 
## Details: if you do:
## 
##         admin -dla /usr/local/sccs/tmp/Xmt
## 
## to unlock all versions, admin will crash.
## 
## "-d" ends up invoking sf-admin.cc : sccs_file::admin, and the loop
## associated with unset_flags. (I believe that the loop associated
## with set_flags also has the same problem). A check is made to
## distingish "-da" from "-d#", and for the case of "-da", the code
## will do:
## 
##               flags.all_locked = 0;
##               flags.locked = NULL;
## 
## Note that flags.locked is a release_list, so this will invoke
## 
##         release_list::release_list(0)
## 
## The constructor in rel_list.cc reads:
## 
##         release_list::release_list(const char *s)
##         {
##           ASSERT(NULL != s);
## 
## I believe that the null case should be treated as a
## 
##         release_list::release_list()
## 
## so I made the following change:
## 
##         release_list::release_list(const char *s)
##         {
##           if (NULL == s) {
##              return;
##           }
## 
## which emulates a class creation with no arguments.
## 

docommand k2 "${vg_admin} -dla $s" 0 IGNORE IGNORE


docommand k3 "${vg_admin} -fla $s" 0 IGNORE IGNORE

# Now, all revisions are locked.   A 'get' must fail.
docommand k4 "${get} -e $s" 1 IGNORE IGNORE

# Remove the locks and try again. (This test is a repeat of 
# test k2, but is required for the next test to work).
docommand k5 "${vg_admin} -dla $s" 0 IGNORE IGNORE
docommand k6 "${get} -e $s" 0 IGNORE IGNORE
remove  $p $g

# Lock just release 2; a get should work, since we are getting release 1.
docommand k7 "${vg_admin} -fl2 $s" 0 IGNORE IGNORE
docommand k8 "${get} -e $s" 0 IGNORE IGNORE

# we may not have "prt".
# docommand k8a "${prt} -f $s | 
#       sed -n -e 's/.*releases//p'" 0 "\t2\n" IGNORE
docommand k8a "${prs} -d:LK: $s" 0 "2\n" IGNORE

remove $p $g

# Lock release 1 as well; a get should fail.
docommand k9 "${vg_admin} -fl1 $s" 0 IGNORE IGNORE

# CSSC and SCCS differ in terms of the order they list the locked
# releases in.
# Locking release 1 should implicitly unlock release 2
# (Solaris 2.6 does this).
docommand k9a "${vg_prs} -d:LK: $s" 0 "1\n" IGNORE

docommand k10 "${get} -e $s" 1 IGNORE IGNORE
remove $p $g


# Remove lock on release 1; things should work now.
docommand k13 "${vg_admin} -dl1 $s" 0 IGNORE IGNORE
docommand k14 "${get} -e $s" 0 IGNORE IGNORE
remove $p $g



docommand k15 "${vg_admin} -fla $s" 0 IGNORE IGNORE
docommand k16 "${prs} -d:LK: $s" 0 "a\n" IGNORE

remove $s $g $p

###
### Cleanup and exit.
###
rm -rf test 
remove foo $s $g $p [zx].$g command.log

success
