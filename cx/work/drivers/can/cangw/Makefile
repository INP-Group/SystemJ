######################################################################
######################################################################

CANKOZ_PFX=		c4l
CANKOZ_HALS_INCLFILE=	can4linux.h
CANKOZ_HAL_DESCR=	CAN4LINUX
CANKOZ_SRCDIR=		../src
include			$(CANKOZ_SRCDIR)/LISTOFCANDRIVERS.mk

SUBDIRS=
EXES=			canserver c4lcanmon
canserver_COMPONENTS=	canserver.o cankoz_lyr_common.o c4lcankoz_lyr.o \
			$(addsuffix _drv.o, $(LISTOFCANDRIVERS))
c4lcanmon_COMPONENTS=	canmon_common.o c4lcanmon.o

TOPDIR=		../../../4cx/src
include $(TOPDIR)/TopRules.mk
include $(CANKOZ_SRCDIR)/ShadowRules.mk

canserver:	$(LIBREMDRVLET) \
		$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)

######################################################################
