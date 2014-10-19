ARLIBS=			libuseful.a libcxscheduler.a
include			libuseful.a_COMPONENTS.mk
include			libcxscheduler.a_COMPONENTS.mk

MAKEFILE_PARTS+=	libuseful.a_COMPONENTS.mk libcxscheduler.a_COMPONENTS.mk

EXPORTSFILES=		$(ARLIBS)
EXPORTSDIR=		lib

SPECIFIC_CFLAGS=	-fPIC

######################################################################
TOPDIR=	        ../..
include		$(TOPDIR)/TopRules.mk
######################################################################
