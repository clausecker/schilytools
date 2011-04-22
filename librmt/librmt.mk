#ident @(#)librmt.mk	1.4 11/04/15 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	rmt
CPPOPTS +=	-DUSE_REMOTE
CPPOPTS +=	-DUSE_RCMD_RSH
CPPOPTS +=	-DUSE_LARGEFILES
include		Targets
LIBS=		
XMK_FILE=	Makefile.man rmtinit.mk3 rmtdebug.mk3 \
		rmtfilename.mk3 rmthostname.mk3 \
		rmtgetconn.mk3 \
		rmtopen.mk3 rmtclose.mk3 \
		rmtread.mk3 rmtwrite.mk3 rmtseek.mk3 rmtioctl.mk3 \
		rmtstatus.mk3 rmtxstatus.mk3 mtg2rmtg.mk3 rmtg2mtg.mk3

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################

