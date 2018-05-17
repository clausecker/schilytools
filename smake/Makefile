#ident @(#)Makefile	1.21 18/05/08 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		smake
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_NSECS		# Enable nanosecond time resolution
CPPOPTS +=	-DNO_COMPAT '-DDEFAULTS_PATH="$(INS_BASE)/share/lib/smake/defaults.smk"'
CPPOPTS +=	-DSCHILY_PRINT

#CPPOPTS +=	-DNO_MYECHO		# Disable built in echo command

CFILES=		make.c archconf.c readfile.c parse.c update.c rules.c \
		job.c memory.c
HFILES=		make.h job.h
#
# $(LIB_INTL) is needed because libschily/mem.c needs it.
#
LIBS=		-lschily $(LIB_INTL)
XMK_FILE=	Makefile.def Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

