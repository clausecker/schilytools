#ident @(#)Makefile	1.16 11/09/14 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		smake
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DNO_COMPAT '-DDEFAULTS_PATH="$(INS_BASE)/lib/defaults.smk"'
CPPOPTS +=	-DSCHILY_PRINT
CFILES=		make.c archconf.c readfile.c parse.c update.c rules.c memory.c
HFILES=		make.h job.h
LIBS=		-lschily
XMK_FILE=	Makefile.def Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

