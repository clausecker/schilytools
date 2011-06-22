h36880
s 00006/00005/00091
d D 1.3 11/06/15 23:36:37 joerg 3 2
c Test ob chmod 0 eine Datei unlesbar macht
e
s 00004/00000/00092
d D 1.2 11/05/31 22:43:36 joerg 2 1
c if [ ".$CYGWIN" = '.' ]; then um Tests mit chmod 0 p.foo / s.foo
e
s 00092/00000/00000
d D 1.1 10/05/03 03:11:28 joerg 1 0
c date and time created 10/05/03 03:11:28 by joerg
e
u
U
f e 0
f y
t
T
I 1
#! /bin/sh

# valbasic.sh:  Basic tests for the "val" command.

# Import common functions & definitions.
. ../common/test-common

files="f s.f"

remove $files

docommand v1 "${admin} -n s.f" 0 IGNORE IGNORE
docommand v2 "${vg_val} s.f" 0 IGNORE IGNORE

docommand v3 "${vg_val} -r1.1 s.f" 0 IGNORE IGNORE
docommand v4 "${vg_val} -s s.f" 0 IGNORE IGNORE

# Having no args is an error.
docommand v5 "${vg_val}" 128 IGNORE IGNORE


# Module flag mismatch
docommand v6 "${vg_val} -mZ s.f" 1 IGNORE IGNORE

# Change the module flag
docommand v7 "${admin} -fmZ s.f" 0 IGNORE IGNORE

# Module flag match
docommand v8 "${vg_val} -mZ s.f" 0 IGNORE IGNORE


# Type flag mismatch
docommand v9 "${vg_val} -yA s.f" 2 IGNORE IGNORE

# Change the type flag
docommand v10 "${admin} -ftA s.f" 0 IGNORE IGNORE

# Module flag match
docommand v11 "${vg_val} -yA s.f" 0 IGNORE IGNORE

# SID not found
docommand v12 "${vg_val} -r1.2 s.f" 4 IGNORE IGNORE

# SID not valid
docommand v13 "${vg_val} -r1.2xyzzy s.f" 8 IGNORE IGNORE

I 2
D 3
if [ ".$CYGWIN" = '.' ]; then
E 3
E 2
chmod 0 s.f || miscarry "Cannot change permissions for file s.f"
D 3
# Cannot read file
docommand v14 "${vg_val} s.f" 16 IGNORE IGNORE
chmod +r s.f || miscarry "Cannot reset permissions for file s.f"
E 3
I 3
cat s.f > /dev/null 2> /dev/null
if [ $? -ne 0 ]; then
	# Cannot read file
	docommand v14 "${vg_val} s.f" 16 IGNORE IGNORE
	chmod +r s.f || miscarry "Cannot reset permissions for file s.f"
E 3
I 2
else
D 3
	echo "Your OS or your the filesystem do not support chmod 0 - some tests skipped"
E 3
I 3
	echo "Your permissions on your OS do not support chmod 0 to remove read permission - test v14 skipped"
E 3
fi
E 2

# Missing file
docommand v15 "${vg_val} -r1.1" 128 IGNORE IGNORE

# Too many -r options
docommand v16 "${vg_val} -r1.1 -r1.2 s.f" 64 IGNORE IGNORE


# A corrupt file
remove s.corrupt
cat valbasic.sh s.f > s.corrupt || miscarry "cannot create file s.corrupt"
docommand v17 "${vg_val} -r1.1 s.corrupt" 32 IGNORE IGNORE
remove s.corrupt


# Too many -r options (a different way)
docommand v18 "${vg_val} -r1.1 -s -r1.1 s.f" 64 IGNORE IGNORE

# Too many -m options
docommand v19 "${vg_val} -mX -mX s.f" 64 IGNORE IGNORE

# Too many -y options
docommand v20 "${vg_val} -yX -yX s.f" 64 IGNORE IGNORE

# Unknown option
docommand v21 "${vg_val} -X s.f" 64 IGNORE IGNORE




# done rc   0 (success)
# done rc   1 (Val_MismatchedM)
# done rc   2 (Val_MismatchedY)
# done rc   4 (Val_NoSuchSID)
# done rc   8 (Val_InvalidSID)
# done rc  16 (Val_CannotOpenOrWrongFormat)
# done rc  32 (Val_CorruptFile)
# done rc  64 (Val_InvalidOption)
# done rc 128 (Val_MissingFile)

remove $files
success
E 1
