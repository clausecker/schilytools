#ident "@(#)Makefile	1.22 19/08/05 "
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
CPPOPTS +=	-DEXIT_ON_ERROR		# exit() on #error directive
CPPOPTS +=	-DUSE_LARGEFILES
#CPPOPTS +=	-DSCHILY_PRINT

___CPY.C=	$(_UNIQ)$(YACC)
__CPY.C=	$(___CPY.C:%-y=%)	# Sometimes $(YACC) is bison -y
_CPY.C=		$(__CPY.C:$(_UNIQ)noyacc=noyacc.c)
CPY.C=		$(_CPY.C:$(_UNIQ)%=cpy.c)

CFILES=		cpp.c $(CPY.C)
HFILES=		cpp.h version.h
LIBS=		-lschily
XMK_FILE=	Makefile.man

#
# The next line is a work around for a bug in SunPro Make:
# SunPro Make just cals readdir() for VPATH at startup and never again
# checks whether a command did create more interesing files in VPATH.
#
# This workaround makes smake and SunPro Make call YACC twice and gmake
# complain about "circular dependencies", but all finally compile the
# program. Given that we have many workarounds for gmake problems, this
# looks like a fair deal.
#
cpy.c: $(ARCHDIR)/cpy.c

cpy.c $(ARCHDIR)/cpy.c:	cpy.y
		@echo "	==> YACCING $@"; $(YACC) cpy.y
		@mv y.tab.c $(ARCHDIR)/cpy.c

CLEAN_FILEX +=	$(ARCHDIR)/cpy.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
include		$(SRCROOT)/$(RULESDIR)/rules.tst
###########################################################################

count:	$(CFILES) $(HFILES)
	count $r1

