#ident %Z%%M%	%I% %E% Copyr 1995-2007 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		cdrecord
CPPOPTS +=	-I../libedc
CPPOPTS +=	-DDRV_DVD
CPPOPTS +=	-DDRV_BD
CPPOPTS +=	-DFIFO
CPPOPTS +=	-DAUINFO
CPPOPTS +=	-DHAVE_LIB_EDC_ECC
CPPOPTS +=	-DCLONE_WRITE
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		cdrecord.c \
		cd_misc.c fifo.c isosize.c audiosize.c auinfo.c \
		cue.c \
		diskid.c \
		wm_session.c wm_track.c wm_packet.c \
		sector.c clone.c \
		movesect.c subchan.c cdtext.c \
		defaults.c \
		cdr_drv.c \
		drv_philips.c drv_sony.c drv_jvc.c drv_7501.c drv_mmc.c \
		drv_dvd.c drv_dvdplus.c \
		drv_bd.c \
		drv_simul.c \
		scsi_scan.c scsi_cdr.c scsi_cdr_mmc4.c scsi_mmc.c scsi_mmc4.c \
		scsilog.c \
		modes.c misc.c crc16.c xio.c
HFILES=		cdrecord.h iso9660.h auheader.h cdtext.h \
		crc16.h movesect.h mmcvendor.h scsi_scan.h \
		scsimmc.h \
		scsilog.h adip.h \
		xio.h \
		defaults.h version.h
HFILES_TAG=	../include/scg/scgio.h ../include/scg/scsidefs.h \
		../include/scg/scsireg.h ../include/scg/scsitransp.h
#LIBS=		-lschily $(LIB_SOCKET)
LIBS=		-lrscg -lscg $(LIB_VOLMGT) -ledc_ecc -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	Makefile.man Makefile.dfl Makefile.doc


###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

