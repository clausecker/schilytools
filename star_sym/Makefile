#ident @(#)star_sym.mk	1.11 18/07/14 
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

CFILES=		star_sym.c restore.c dumpdate.c lpath_unix.c
LIBS=		-lfind -lschily $(LIB_ACL_TEST) $(LIB_INTL)
XMK_FILE=	star_sym.mk1

restore.c dumpdate.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): restore.c dumpdate.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

