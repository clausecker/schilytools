#ident %W% %E% %Q%
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		star_sym
CPPOPTS +=	-I../star
CPPOPTS +=	-DSTAR_SYM
CPPOPTS +=	-D__STAR__ 
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		star_sym.c restore.c dumpdate.c
LIBS=		-lfind -lschily $(LIB_ACL_TEST) $(LIB_INTL)
#XMK_FILE=	Makefile.man
#XMK_FILE=	tartestman.mk

restore.c dumpdate.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
all: restore.c dumpdate.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

IFDEF=	-UEVALVERS
#XRELFILES=	Makefile tartestman.mk tartest.1
XRELFILES=	Makefile MKLINKS star_sym.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.rel
###########################################################################

MAKE_LICENSE=MKCDDL
