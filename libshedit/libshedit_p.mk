#ident "@(#)libshedit_p.mk	1.3 20/07/08 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/profiled
SUBINSDIR=	/profiled
INSDIR=		lib
TARGETLIB=	shedit

CPPOPTS +=	-DBSH			# Tell the code that we compile for bsh
CPPOPTS +=	-DUSE_LARGEFILES	# Allow Large Files (> 2 GB)
CPPOPTS +=	-DINTERACTIVE		# Include command line history editor
CPPOPTS +=	-DDO_SUID		# Include code for 'suid' builtin
# ??? CPPOPTS +=	-DJOBCONTROL		# Include Job Control management
CPPOPTS +=	-DVFORK			# Use vfork() if possible
CPPOPTS +=	-DOLD_PWORD		# Use old "word" tokenizer
CPPOPTS +=	-DFAST_MALLOC		# malloc() without freechecking
					# and without bound checks
#CPPOPTS +=	-DNO_USER_MALLOC	# Do not use our own malloc()
CPPOPTS +=	-DTESTMAIL		# Do mail file checking

CPPOPTS	+=	-DBOURNE
CPPOPTS	+=	-DLIB_SHEDIT
CPPOPTS	+=	-DINCL_MYSTDIO
CPPOPTS	+=	-DNO_WEAK_SYMBOLS
CPPOPTS	+=	-DNO_GETLINE_COMPAT
CPPOPTS	+=	-I.
#CPPOPTS +=	-DNO_LOCALE		# Don't use setlocale()
#CPPOPTS +=	-DNO_WCHAR		# Don't use wide chars

#COPTX += -g -xO0
#LDOPTX += -g

COPTS +=	$(COPTGPROF)

#
# Additional defines:
#
#	-DFAST_MALLOC	a malloc() without freechecking and without a check
#			for overrun size bounds.
#	-DNO_USER_MALLOC Do not use our own (user defined) malloc()

include		Targets

LIBS=		-lxtermcap -lschily -lc
#XMK_FILE=	Makefile.man
XMK_FILE=	

inputc.c map.c bsh.h comerr.c:
	@echo "	==> MAKING SYMLINKS in ."; sh ./MKLINKS
$(ALLTARGETS): inputc.c map.c
$(SRCROOT)/$(RULESDIR)/rules.lib: bsh.h

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
count: $(HFILES) $(CFILES) 
	count $r1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.rel
###########################################################################
