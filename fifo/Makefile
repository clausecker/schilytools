#ident @(#)Makefile	1.4 21/08/19 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		fifo
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DFIFO			# Include FIFO code
CPPOPTS +=	-DFIFO_STANDALONE	# FIFO code is used as standalone
CPPOPTS +=	-DUSE_MMAP		# Use mmap() for the FIFO
CPPOPTS +=	-DUSE_NLS		# Include locale support
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DINS_RBASE=\"${INS_RBASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_utils\"

CFILES=		fifo.c fifo_main.c
HFILES=
LIBS=		-lschily $(LIB_INTL)
XMK_FILE=	Makefile.man

fifo.c:
	@echo "	==> MAKING SYMLINKS in ."; sh ./MKLINKS
$(SRCROOT)/$(RULESDIR)/rules.cmd: fifo.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

