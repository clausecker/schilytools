#ident @(#)Makefile	1.31 18/09/16 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		ved
SYMLINKS +=	ved-e
SYMLINKS +=	ved-w
#
# It you like to compile ved with large file support, uncomment the next
# line to make CPPOPTS += -DUSE_LARGEFILES active.
#
# It you don"t like to compile ved with large file support, comment the next
# line to make CPPOPTS += -DUSE_LARGEFILES inactive.
#
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DVED_STATS
CPPOPTS +=	-DFASTPOS
#CPPOPTS +=	-DFASTPOS -DCHECKPOS
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		ved.c edit.c binding.c vedtmpops.c cmds.c quitcmds.c \
		execcmds.c numbercmds.c cursorcmds.c delcmds.c \
		searchcmds.c filecmds.c tmpfiles.c takecmds.c \
		markcmds.c screen.c ctab.c movedot.c buffer.c storage.c \
		terminal.c cmdline.c io.c fileio.c filesubs.c \
		take.c message.c search.c \
		ttycmds.c ttymodes.c macro.c coloncmds.c \
		substcmds.c tags.c map.c vedstats.c consdebug.c

HFILES=		buffer.h ved.h func.h map.h movedot.h terminal.h ttys.h \
		version.h
#LIBS=		-lunos
LIBS=		-lxtermcap -lschily
XMK_FILE=	Makefile.man ved-e.mk1 ved-w.mk1 \
		Makefile.help Makefile.doc Makefile.dot Makefile.warn

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

count:		$(CFILES) $(HFILES)
		count $r1

