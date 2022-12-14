#ident @(#)Makefile	1.48 19/08/11 
###########################################################################
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; see the file COPYING.  If not, write to the Free Software
# Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		mkisofs
SYMLINKS=	mkhybrid
CPPOPTS +=	-DUSE_FIND
#CPPOPTS +=	-DADD_FILES
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DAPPLE_HFS_HYB
CPPOPTS +=	-DAPPLE_HYB
CPPOPTS +=	-DUDF
CPPOPTS +=	-DDVD_AUD_VID
CPPOPTS +=	-DSORTING
CPPOPTS +=	-DDUPLICATES_ONCE
CPPOPTS +=	-I../libscg
CPPOPTS +=	-I../libscgcmd
CPPOPTS +=	-I../libhfs_iso
CPPOPTS +=	-I../libcdrdeflt
#CPPOPTS +=	-DAPPLE_PEARSON_OLD	# Use old struct attrinfo from apple.h
CPPOPTS	+=	-DUSE_SCG \
		'-DAPPID_DEFAULT="MKISOFS ISO9660/HFS/UDF FILESYSTEM BUILDER & CDRECORD CD/DVD/BluRay CREATOR (C) 1993 E.YOUNGDALE (C) 1997 J.PEARSON/J.SCHILLING"'
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DUSE_ICONV
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_cdrtools\"

CFILES=		mkisofs.c tree.c write.c hash.c rock.c inode.c udf.c multi.c \
		joliet.c match.c name.c eltorito.c boot.c isonum.c \
		scsi.c \
		apple.c volume.c desktop.c mac_label.c stream.c \
		ifo_read.c dvd_file.c dvd_reader.c \
		walk.c
HFILES=		apple.h bootinfo.h defaults.h diskmbr.h \
		iso9660.h mac_label.h mactypes.h match.h \
		mkisofs.h sunlabel.h udf.h udf_fs.h vms.h \
		ifo_read.h dvd_file.h dvd_reader.h bswap.h ifo_types.h

#
# LIB_CAP is needed for Linux capability support in librscg.
#
LIBS=		-lhfs -lfile -lsiconv -lscgcmd -lrscg -lscg $(LIB_VOLMGT) -lcdrdeflt -ldeflt \
		-lfind -lmdigest -lschily \
		$(LIB_ACL_TEST) $(SCSILIB) $(LIB_SOCKET) \
		$(LIB_ICONV) $(LIB_INTL) $(LIB_CAP)

XMK_FILE=	Makefile.man hybridman.mk Makefile.doc

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

xtxt:	$(CFILES) apple_driver.c
	gxgettext -k_ -k__ -kperr $(CFILES) apple_driver.c
