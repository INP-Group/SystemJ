######################################################################
######################################################################

CANKOZ_PFX=		skt
CANKOZ_HALS_INCLFILE=	
CANKOZ_HAL_DESCR=	SocketCAN
CANKOZ_SRCDIR=		../src
include			$(CANKOZ_SRCDIR)/LISTOFCANDRIVERS.mk

SUBDIRS=
DRIVERS=	$(addsuffix _drv.so, $(LISTOFCANDRIVERS))
LAYERS=		sktcankoz_lyr.so
MONO_C_FILES=	$(DRIVERS)
SOLIBS=		$(LAYERS)
EXES=		sktcanmon
sktcankoz_lyr.so_COMPONENTS=	cankoz_lyr_common.o sktcankoz_lyr.o
sktcanmon_COMPONENTS=		canmon_common.o sktcanmon.o

TOPDIR=		../../../4cx/src
include $(TOPDIR)/TopRules.mk
include $(CANKOZ_SRCDIR)/ShadowRules.mk

######################################################################
