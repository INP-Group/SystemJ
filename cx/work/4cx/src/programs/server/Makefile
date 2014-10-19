######################################################################
#                                                                    #
#  programs/server/Makefile                                          #
#      Rules for making CX server                                    #
#                                                                    #
######################################################################

EXES=		cxsd rrund
cxsd_COMPONENTS=	cxsd.o cxsd_vars.o cxsd_config.o \
			cxsd_fndmgr.o cxsd_loader.o \
			cxsd_publics.o \
			cxsd_builtins.o
rrund_components=	rrund.o

SUBDIRS=
TOPDIR=		../..
include $(TOPDIR)/TopRules.mk

LOCAL_WORKFILES=	*.log *.pid

cxsd:		SPECIFIC_LDFLAGS=-Wl,-export-dynamic
cxsd:		SPECIFIC_LIBS+=$(LIBDL)
cxsd:		$(LIBCXSD_PLUGINS) $(LIBCXSD) \
		$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)

rrund:		$(LIBREMDRVLET) \
		$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)

# #### END OF Makefile ###############################################
