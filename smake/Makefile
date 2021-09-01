#ident @(#)Makefile	1.23 21/08/20 
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
CPPOPTS +=	-DUSE_NLS		# Include locale support
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DINS_RBASE=\"${INS_RBASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_utils\"

#CPPOPTS +=	-DNO_MYECHO		# Disable built in echo command

CFILES=		make.c archconf.c readfile.c parse.c update.c rules.c \
		job.c memory.c
HFILES=		make.h job.h version.h
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

