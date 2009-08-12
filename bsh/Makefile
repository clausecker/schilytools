#ident @(#)Makefile	1.25 09/08/04 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		bsh
SYMLINKS=	pfbsh
CPPOPTS +=	-DBSH			# Tell the code that we compile for bsh
CPPOPTS +=	-DUSE_LARGEFILES	# Allow Large Files (> 2 GB)
CPPOPTS +=	-DINTERACTIVE		# Include command line history editor
CPPOPTS +=	-DDO_SUID		# Include code for 'suid' builtin
CPPOPTS +=	-DDO_PFEXEC		# Include code for "pfbsh" (pfexec)
CPPOPTS +=	-DJOBCONTROL		# Include Job Control management
CPPOPTS +=	-DVFORK			# Use vfork() if possible
CPPOPTS +=	-DOLD_PWORD		# Use old "word" tokenizer
CPPOPTS +=	-DFAST_MALLOC		# malloc() without freechecking
					# and without bound checks
#CPPOPTS +=	-DD_MALLOC		# add support for dbg_malloc()
#CPPOPTS +=	-DNO_USER_MALLOC	# Do not use our own malloc()
CPPOPTS +=	-DTESTMAIL		# Do mail file checking
CPPOPTS +=	-DSCHILY_PRINT		# Use js_*() versions of *print*() funcs
CPPOPTS +=	-DDO_FIND		# Include find(1) builtin
#CPPOPTS +=	-DNO_LOCALE		# Don't use setlocale()
#CPPOPTS +=	-DNO_WCHAR		# Don't use wide chars



#
# Additional defines:
#
#	-DFAST_MALLOC	a malloc() without freechecking and without a check
#			for overrun size bounds.
#	-DNO_USER_MALLOC Do not use our own (user defined) malloc()

CFILES=		bsh.c node.c parse.c hashcmd.c alias.c abbtab.c \
		exec.c sys.c proc.c \
		builtin.c btab.c ctype.c signames.c \
		call.c cond.c dirs.c test.c idops.c input.c limit.c \
		getrusage.c \
		abbrev.c expand.c strsubs.c inputc.c ttymodes.c map.c \
		testmail.c oldhistory.c evops.c str.c version.c \
		alloc.c wait3.c pfexec.c
#
# match.c nur wenn evt. a='test'cc"bbb" implementiert wird
#		alloc.c match.c wait3.c

HFILES=		abbrev.h bsh.h map.h node.h abbtab.h btab.h limit.h \
		str.h \
		strsubs.h ctype.h bshconf.h
#LIBS=		-lunos
#
# $(LIB_ACL_TEST) $(LIB_INTL) are needed for libfind
#
LIBS=		-lxtermcap -lfind -lschily $(LIB_SECDB) $(LIB_CRYPT) $(LIB_ACL_TEST) $(LIB_INTL)
XMK_FILE=	Makefile.man Makefile.dot

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count: $(HFILES) $(CFILES) 
	count $r1


