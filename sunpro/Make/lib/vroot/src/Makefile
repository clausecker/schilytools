#ident @(#)Makefile	1.7 17/04/30 
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
CPPOPTS +=	-DSCHILY_INCLUDES
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSUN5_0
CPPOPTS +=	-DSUNOS4_AND_AFTER
CPPOPTS +=	-DPMAKE
CPPOPTS +=	-DREDIRECT_ERR

C++FILES=	access.cc args.cc execve.cc lock.cc lstat.cc open.cc readlink.cc report.cc stat.cc statfs.cc unmount.cc vroot.cc

LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
