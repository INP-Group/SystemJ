SUBDIRS=

ARLIBS=			libtsycam.a
ifeq "$(NOX)" ""
  ARLIBS+=		libmiscXutils.a
endif
libtsycam.a_COMPONENTS=		tsycamlib.o
libmiscXutils.a_COMPONENTS=	gray2image.o


EXPORTSFILES=		$(ARLIBS)
EXPORTSDIR=		lib

######################################################################
PRJDIR=		..
include		$(PRJDIR)/PrjRules.mk
######################################################################
