#ident @(#)Makefile	1.3 20/07/08 
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

