#ident "@(#)Makefile	1.11 08/07/31 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		sh			# Shell
SYMLINKS=	pfsh			# Profile Bourne Shell
SYMLINKS +=	bosh			# Bo urne Shell
SYMLINKS +=	jsh			# Job control Bourne Shell
#CPPOPTS +=	-DBSH			# Tell the code that we compile for bsh
#CPPOPTS +=	-DUSE_LARGEFILES	# Allow Large Files (> 2 GB)
#CPPOPTS +=	-DINTERACTIVE		# Include command line history editor
#CPPOPTS +=	-DDO_SUID		# Include code for 'suid' builtin
#CPPOPTS +=	-DJOBCONTROL		# Include Job Control management
#CPPOPTS +=	-DVFORK			# Use vfork() if possible
#CPPOPTS +=	-DOLD_PWORD		# Use old "word" tokenizer
#CPPOPTS +=	-DFAST_MALLOC		# malloc() without freechecking
					# and without bound checks
##CPPOPTS +=	-DNO_USER_MALLOC	# Do not use our own malloc()
#CPPOPTS +=	-DTESTMAIL		# Do mail file checking

CPPOPTS +=	-DBOURNE_SHELL		# Tell the code that we compile for sh
CPPOPTS +=	-DUSE_LARGEFILES	# Allow Large Files (> 2 GB)
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DUSE_WCHAR

CPPOPTS +=	-D_iBCS2 				# SCO echo compat
CPPOPTS +=	-DTEXT_DOMAIN='"SUNW_OST_OSCMD"'	# gettext()
CPPOPTS +=	-D_TS_ERRNO 				# ????
CPPOPTS +=	-DACCT					# Shell Accounting

CPPOPTS +=	-DINTERACTIVE

CFILES=		args.c blok.c bltin.c cmd.c ctype.c defs.c echo.c error.c \
		expand.c fault.c func.c hash.c hashserv.c io.c jobs.c macro.c \
		main.c msg.c name.c print.c pwd.c service.c setbrk.c \
		sh_policy.c stak.c string.c test.c ulimit.c word.c xec.c \
		signames.c gmatch.c mapmem.c

HFILES=		brkincr.h ctype.h defs.h dup.h hash.h mac.h mode.h name.h \
		sh_policy.h stak.h sym.h timeout.h

#LIBS=		-lunos
#OOOLIBS=		-lxtermcap -lschily $(LIB_CRYPT)

# -lgen		fuer gmatch()
# -lsecdb	fuer getexecuser() free_execattr()
#
LIBS=		$(LIB_GEN) $(LIB_SECDB) -lshelledit -lxtermcap -lgetopt -lschily
#LIBS=		-lgen -lsecdb 
XMK_FILE=	Makefile.man

signames.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
all: signames.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count: $(HFILES) $(CFILES) 
	count $r1


