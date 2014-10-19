MONO_C_FILES=	simpleaccess-test findfilein-test

TOPDIR=	        ../..
include		$(TOPDIR)/TopRules.mk

simpleaccess-test:	$(LIBCDR) $(LIBDATATREE) \
			$(LIBCDA) $(LIBCX_ASYNC) \
			$(LIBUSEFUL) $(LIBCXSCHEDULER) $(LIBMISC)
simpleaccess-test:	SPECIFIC_LIBS=$(LIBDL) -lm

findfilein-test:	$(LIBUSEFUL)
