#ident @(#)Makefile	1.17 21/08/20 Copyr 1988 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

.SEARCHLIST:	. $(ARCHDIR) dat $(ARCHDIR)
VPATH=		.:dat:$(ARCHDIR)

INSDIR=		bin
TARGET=		sformat
CPPOPTS +=	-I../libscg
CPPOPTS +=	-I.		# Für dat/*.c
CPPOPTS +=	-DFMT
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_NLS		# Include locale support
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DINS_RBASE=\"${INS_RBASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_utils\"

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
LIBS=		-lscg $(LIB_VOLMGT) -lschily $(SCSILIB) $(LIB_KVM) $(LIB_SOCKET) $(LIB_INTL)
XMK_FILE=	Makefile.dat Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count: $(CFILES) $(HFILES)
	count $r1

