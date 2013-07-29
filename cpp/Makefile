#ident "@(#)Makefile	1.17 13/07/14 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

.SEARCHLIST:	. $(ARCHDIR) $(ARCHDIR) $(ARCHDIR)
VPATH=		.:$(ARCHDIR)

INSDIR=		lib
TARGET=		cpp
CPPOPTS +=	-I.
CPPOPTS +=	-Dunix=1		# Compile for UNIX environment
CPPOPTS +=	-DFLEXNAMES		# Allow long identifier names
CPPOPTS +=	-DSUN_SAYLINE		# Add enter/leave note to # line
CPPOPTS +=	-DUSE_LARGEFILES
#CPPOPTS +=	-DSCHILY_PRINT

___CPY.C=	$(_UNIQ)$(YACC)
__CPY.C=	$(___CPY.C:%-y=%)	# Sometimes $(YACC) is bison -y
_CPY.C=		$(__CPY.C:$(_UNIQ)noyacc=noyacc.c)
CPY.C=		$(_CPY.C:$(_UNIQ)%=cpy.c)

CFILES=		cpp.c $(CPY.C)
HFILES=		cpp.h
LIBS=		-lschily
XMK_FILE=	Makefile.man

cpy.c $(ARCHDIR)/cpy.c:	cpy.y
		@echo "	==> YACCING $@"; $(YACC) cpy.y
		@mv y.tab.c $(ARCHDIR)/cpy.c

CLEAN_FILEX +=	$(ARCHDIR)/cpy.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

count:	$(CFILES) $(HFILES)
	count $r1

