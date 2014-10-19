######################################################################
#                                                                    #
#  programs/utils/Makefile                                           #
#      Rules for making CX command-line utilities                    #
#                                                                    #
######################################################################

EXES=			drvinfo ppf4td_test \
			cxclient cdaclient das-experiment

drvinfo_COMPONENTS=	drvinfo.o drvinfo_publics.o
ppf4td_test_COMPONENTS=	ppf4td_test.o
cxclient_COMPONENTS=	cxclient.o
cdaclient_COMPONENTS=	cdaclient.o
das-experiment_COMPONENTS=das-experiment.o

SUBDIRS=	
TOPDIR=		../..
include $(TOPDIR)/TopRules.mk

drvinfo:		SPECIFIC_LIBS+=$(LIBDL)
drvinfo:		SPECIFIC_LDFLAGS=-Wl,-export-dynamic
drvinfo:		$(LIBCXSD) \
			$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)

ppf4td_test:		$(LIBUSEFUL) $(LIBMISC)

cxclient:		$(LIBCX_SYNC) \
			$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)

cdaclient:		$(LIBCDA) $(LIBCX_ASYNC) \
			$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)
cdaclient:		SPECIFIC_LIBS=$(LIBDL) -lm

das-experiment:		$(LIBCDA) $(LIBCX_ASYNC) \
			$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)
das-experiment:		SPECIFIC_LIBS=$(LIBDL) -lm

# #### END OF Makefile ###############################################
