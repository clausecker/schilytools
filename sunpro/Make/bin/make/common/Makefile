#ident @(#)Makefile	1.9 17/04/25 
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	../../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		make
HARDLINKS=	dmake ../lib/svr4.make ../xpg4/bin/make
CPPOPTS +=	-I../../../../include
CPPOPTS +=	-I../../../include
#CPPOPTS +=	-compat=4
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-D_FILE_OFFSET_BITS=64 -DSUN5_0
CPPOPTS +=	-DSUNOS4_AND_AFTER
CPPOPTS +=	-DPMAKE
CPPOPTS +=	-DREDIRECT_ERR
#CFILES=		socket.c
C++FILES=	ar.cc depvar.cc dist.cc dmake.cc doname.cc dosys.cc \
		files.cc globals.cc implicit.cc macro.cc main.cc \
		make.cc misc.cc nse.cc nse_printdep.cc parallel.cc \
		pmake.cc read.cc read2.cc rep.cc state.cc version.cc

#
# -lmksdmsi18n	needed for catgets()
# -lnsl		needed for e.g. gethostname() on older Svr4 platforms
# -lsocket	needed for TEAMWARE_MAKE_CMN with older Svr4 platforms
# -lw		needed for older Svr4 platforms to support wide chars
#
# -lschily	needed if wcsdup(), ... are missing
# $(LIB_PTHREAD) needed for PMAKE to support sem_post()
#
#LIBS=		-lmksh  -lmksdmsi18n  -lbsd  -lvroot -lintl -lnsl  -lsocket  -lw
LIBS=		-lmksh  -lbsd  -lvroot -lnsl -lschily $(LIB_PTHREAD)

XMK_FILE=	Makefile.man svmakeman.mk Makefile.def svmakedef.mk

LDCC=		$(LDCC++)
###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
