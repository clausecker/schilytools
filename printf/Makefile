#ident @(#)Makefile	1.5 20/07/08 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		printf
OINCSDIR +=	-I.			# Use local defs.h
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CPPOPTS +=	-DDO_SYSPRINTF_FLOAT	# Include float support in "printf"

CFILES=		printf.c misc.c
HFILES=		defs.h
LIBS=		-lschily $(LIB_INTL)
XMK_FILE=	Makefile.man

printf.c:
	@echo "	==> MAKING SYMLINKS in ."; sh ./MKLINKS
$(SRCROOT)/$(RULESDIR)/rules.cmd: printf.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

