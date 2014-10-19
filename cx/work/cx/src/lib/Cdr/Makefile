ARLIBS=			libCdr.a libdatatree.a
include			libCdr.a_COMPONENTS.mk
include			libdatatree.a_COMPONENTS.mk

MAKEFILE_PARTS+=	libCdr.a_COMPONENTS.mk libdatatree.a_COMPONENTS.mk

EXPORTSFILES=		$(ARLIBS)
EXPORTSDIR=		lib

SPECIFIC_CFLAGS=	-fPIC

######################################################################
TOPDIR=	        ../..
include		$(TOPDIR)/TopRules.mk
######################################################################
