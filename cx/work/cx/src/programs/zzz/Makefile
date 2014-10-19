EXES=			cxsd

cxsd_COMPONENTS=	cxsd.o cxsd_config.o cxsd_data.o \
			cxsd_lib.o \
			cxsd_bigc.o cxsd_channels.o \
			cxsd_dbase.o cxsd_loader.o cxsd_drvmgr.o cxsd_driver.o \
			cxsd_fe_cxv2.o

LOCAL_WORKFILES=	*.log *.pid

EXPORTSFILES=	$(EXES)
EXPORTSDIR=	sbin

EXPORTSFILES2=	cxd.conf cxd-onelog.conf
EXPORTSDIR2=	configs


######################################################################
TOPDIR=	        ../..
include		$(TOPDIR)/TopRules.mk
######################################################################

$(EXES):	$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)

cxsd:		SPECIFIC_LDFLAGS=-Wl,-export-dynamic
cxsd:		SPECIFIC_LIBS=$(LIBDL)
