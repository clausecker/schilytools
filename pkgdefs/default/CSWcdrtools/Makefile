#ident %W% %E% %Q%
###########################################################################
# Sample makefile for creating SVr4 packages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

PKG	:sh=	sed -n 's/^PKG=//p' pkginfo
VERSION	:sh=	sed -n 's/^VERSION=//p' pkginfo
NAME	:sh=	sed -n 's/^NAME=//p' pkginfo|awk '{print $1}'
OS_NAME :sh=	uname -s

VENDOR=		-CSW

all:
	pkgmk -o -b `pwd`/$(SRCROOT)/$(PROTODIR) -d $(SRCROOT)/packages/ -a $(P_ARCH) -f prototype
	pkgtrans -s $(SRCROOT)/packages/ $(NAME)-$(VERSION)-$(OS_NAME)$(OSREL)-$(P_ARCH)$(VENDOR).pkg $(PKG)
	bzip2 -9f $(SRCROOT)/packages/$(NAME)-$(VERSION)-$(OS_NAME)$(OSREL)-$(P_ARCH)$(VENDOR).pkg

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.pkg
###########################################################################
