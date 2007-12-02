#ident @(#)all.mk	1.1 05/02/16 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#
# This is all.mk, it creates sevral binaries, one for each function.
#
# If you like to create one single "fat" binary, remove Makefile
# and copy star_fat.mk to Makefile.
#

MK_FILES= star.mk pax.mk suntar.mk gnutar.mk cpio.mk 

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mks
###########################################################################
