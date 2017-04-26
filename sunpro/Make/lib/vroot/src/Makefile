#ident @(#)Makefile	1.5 17/04/22 
###########################################################################
SRCROOT=	../../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	vroot
CPPOPTS +=	-I../../../../include
CPPOPTS +=	-I../../../include
#CPPOPTS +=	-compat=4
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-D_FILE_OFFSET_BITS=64 -DSUN5_0
CPPOPTS +=	-DSUNOS4_AND_AFTER
CPPOPTS +=	-DPMAKE
CPPOPTS +=	-DREDIRECT_ERR

C++FILES=	access.cc args.cc chdir.cc chmod.cc chown.cc chroot.cc creat.cc execve.cc lock.cc lstat.cc mkdir.cc mount.cc open.cc readlink.cc report.cc rmdir.cc setenv.cc stat.cc statfs.cc truncate.cc unlink.cc unmount.cc utimes.cc vroot.cc

LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
