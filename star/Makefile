#ident @(#)Makefile	1.23 08/04/06 
###########################################################################
#include		$(MAKE_M_ARCH).def
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#
# This is star_fat.mk, it creates one "fat" binary for all functionality.
#
# If you like to create non "fat" binaries, remove Makefile
# and copy all.mk to Makefile.
#
INSDIR=		bin
TARGET=		star
#SYMLINKS=	ustar tar
SYMLINKS=	ustar tar gnutar suntar scpio spax
CPPOPTS +=	-D__STAR__
CPPOPTS +=	-DSET_CTIME -DFIFO -DUSE_MMAP -DUSE_REMOTE -DUSE_RCMD_RSH
#CPPOPTS +=	-DSET_CTIME -DFIFO -DUSE_MMAP
#CPPOPTS +=	-DSET_CTIME -DUSE_MMAP
#CPPOPTS +=	-DFIFO -DUSE_MMAP
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_FIND
CPPOPTS +=	-DUSE_ACL
CPPOPTS +=	-DUSE_XATTR
CPPOPTS +=	-DUSE_FFLAGS
CPPOPTS +=	-DCOPY_LINKS_DELAYED
CPPOPTS +=	-DSTAR_FAT
CPPOPTS +=	-DSCHILY_PRINT
CFILES=		star_fat.c header.c cpiohdr.c xheader.c xattr.c \
		list.c extract.c create.c append.c diff.c restore.c \
		remove.c star_unix.c acl_unix.c acltext.c fflags.c \
		buffer.c dirtime.c lhash.c \
		hole.c longnames.c \
		movearch.c table.c props.c \
		unicode.c \
		subst.c volhdr.c \
		chdir.c match.c defaults.c dumpdate.c \
		fifo.c device.c checkerr.c \
		\
		findinfo.c

HFILES=		star.h starsubs.h dirtime.h xtab.h xutimes.h \
		movearch.h table.h props.h fifo.h diff.h restore.h \
		checkerr.h dumpdate.h bitstring.h

#LIBS=		-lunos
#LIBS=		-lschily -lc /usr/local/lib/gcc-gnulib
LIBS=		-ldeflt -lrmt -lfind -lschily $(LIB_ACL) $(LIB_ATTR) $(LIB_SOCKET) $(LIB_INTL)
#
#	Wenn -lfind, dann auch  $(LIB_INTL)
#
XMK_FILE=	Makefile.man starformatman.mk scpioman.mk gnutarman.mk \
		spaxman.mk suntarman.mk Makefile.dfl Makefile.doc

star_fat.c: star.c
	$(RM) $(RM_FORCE) $@; cp star.c $@

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

