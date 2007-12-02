#ident %W% %E% %Q%
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		ved_static
#
# It you like to compile ved with large file support, uncomment the next
# line to make CPPOPTS += -DUSE_LARGEFILES active.
#
#CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DVED_STATS
CPPOPTS +=	-DFASTPOS
#CPPOPTS +=	-DFASTPOS -DCHECKPOS
CPPOPTS +=	-DNO_FLOATINGPOINT
CFILES=		ved.c edit.c binding.c vedtmpops.c cmds.c quitcmds.c \
		execcmds.c numbercmds.c cursorcmds.c delcmds.c \
		searchcmds.c filecmds.c tmpfiles.c takecmds.c \
		markcmds.c screen.c ctab.c movedot.c buffer.c storage.c \
		terminal.c cmdline.c io.c fileio.c filesubs.c \
		take.c message.c search.c \
		ttycmds.c ttymodes.c macro.c coloncmds.c \
		substcmds.c tags.c map.c vedstats.c consdebug.c \
		dldummy.c format.c

HFILES=		buffer.h ved.h func.h map.h movedot.h terminal.h ttys.h
#LIBS=		-lunos
#LDOPTS +=	-Bstatic
LDOPTS +=	-dn
LIBS=		-lxtermcap -lschily

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
