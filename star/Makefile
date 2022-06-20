#ident @(#)Makefile	1.37 18/07/30 
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
CPPOPTS +=	-DSET_CTIME		# Include timestorm code to set ctime
CPPOPTS +=	-DFIFO			# Include FIFO code
CPPOPTS +=	-DUSE_SSH		# Use "ssh" by default instead of "rsh"
CPPOPTS +=	-DUSE_MMAP		# Use mmap() for the FIFO
CPPOPTS +=	-DUSE_REMOTE		# Add support for the rmt protocol
CPPOPTS +=	-DUSE_RCMD_RSH		# Add fast support for rcmd()
CPPOPTS +=	-DUSE_LARGEFILES	# Support files > 2 GB
CPPOPTS +=	-DUSE_FIND		# Include support for libfind
CPPOPTS +=	-DUSE_ACL		# Support file ACLs
CPPOPTS +=	-DUSE_XATTR		# Support (currently Linux) xattr
CPPOPTS +=	-DUSE_SELINUX		# Support SELinux
CPPOPTS +=	-DUSE_FFLAGS		# Support BSD style file flags
CPPOPTS +=	-DCOPY_LINKS_DELAYED	# Delayed copy of hard/symlinks
CPPOPTS +=	-DSTAR_FAT		# One binary for all CLI's
CPPOPTS +=	-DUSE_ICONV		# Use iconv() for pax filenames
CPPOPTS +=	-DUSE_NLS		# Include locale support
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DINS_RBASE=\"${INS_RBASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_utils\"
CPPOPTS +=	-DSCHILY_PRINT
CFILES=		star_fat.c header.c cpiohdr.c xheader.c xattr.c \
		list.c extract.c create.c append.c diff.c restore.c \
		remove.c star_unix.c lpath_unix.c \
		acl_unix.c acltext.c fflags.c \
		buffer.c dirtime.c lhash.c \
		hole.c longnames.c \
		movearch.c table.c props.c \
		unicode.c \
		subst.c volhdr.c \
		chdir.c match.c defaults.c dumpdate.c \
		fifo.c device.c checkerr.c paxopts.c \
		\
		findinfo.c pathname.c version.c

HFILES=		star.h starsubs.h dirtime.h xtab.h xutimes.h \
		movearch.h table.h props.h fifo.h diff.h restore.h \
		checkerr.h dumpdate.h bitstring.h pathname.h

#LIBS=		-lunos
#LIBS=		-lschily -lc /usr/local/lib/gcc-gnulib
#
# LIB_CAP is needed for Linux capability support in librmt.
# LIB_SELINUX is needed for SELinux capability support.
#
LIBS=		-ldeflt -lrmt -lfind -lschily $(LIB_ACL) $(LIB_ATTR) $(LIB_SOCKET) $(LIB_ICONV) $(LIB_INTL) $(LIB_CAP) $(LIB_SELINUX)
#
#	Wenn -lfind, dann auch  $(LIB_INTL)
#
XMK_FILE=	Makefile.man ustarman.mk starformatman.mk scpioman.mk gnutarman.mk \
		spaxman.mk suntarman.mk Makefile.dfl Makefile.doc

star_fat.c: star.c
	@echo " ==> MAKING SYMLINKS in ."; sh ./MKLINKS

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
include		$(SRCROOT)/$(RULESDIR)/rules.tst
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

