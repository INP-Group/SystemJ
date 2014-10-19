######################################################################
######################################################################

CANKOZ_PFX=		mar
CANKOZ_HALS_INCLFILE=	candrv.h
CANKOZ_HAL_DESCR=	Marathon driver for Linux-2.4
CANKOZ_SRCDIR=		../src
include			$(CANKOZ_SRCDIR)/LISTOFCANDRIVERS.mk

SUBDIRS=
DRIVERS=	$(addsuffix _drv.so, $(LISTOFCANDRIVERS))
LAYERS=		marcankoz_lyr.so
MONO_C_FILES=	$(DRIVERS)
SOLIBS=		$(LAYERS)
EXES=		marcanmon
marcankoz_lyr.so_COMPONENTS=	cankoz_lyr_common.o marcankoz_lyr.o
marcanmon_COMPONENTS=		canmon_common.o marcanmon.o

TOPDIR=		../../../4cx/src
include $(TOPDIR)/TopRules.mk
include $(CANKOZ_SRCDIR)/ShadowRules.mk

######################################################################
