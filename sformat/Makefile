#ident @(#)Makefile	1.14 07/12/02 Copyr 1988 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

.SEARCHLIST:	. $(ARCHDIR) dat $(ARCHDIR)
VPATH=		.:dat:$(ARCHDIR)

INSDIR=		bin
TARGET=		sformat
/*CPPOPTS +=	-DFMT -ISun*/
CPPOPTS +=	-DFMT
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		fmt.c modes.c labelsubs.c makelabel.c io.c \
		defect.c badblock.c repair.c diskfmt.c diskparam.c\
		acb4000.c \
		datio.c xmodes.c xdisk.c xpart.c \
		scsicmds.c \
		maptodisk.c checkmount.c \
		xdelay.c \
		bcrypt.c sinfo.c \
		check_part.c rand_rw.c autopart.c modeselect.c

HFILES=		fmt.h \
		defect.h \
		rand_rw.h map.h \
		sun_dklabel.h sun_dkio.h \
		scsicmds.h dsklabel.h

#LIBS=		-lunos
LIBS=		-lscg $(LIB_VOLMGT) -lschily $(SCSILIB) $(LIB_KVM) $(LIB_SOCKET)
XMK_FILE=	Makefile.dat Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count: $(CFILES) $(HFILES)
	count $r1

