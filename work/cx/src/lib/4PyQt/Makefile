UNCFILES=	libCdr4PyQt.so

EXPORTSFILES=	$(UNCFILES)
EXPORTSDIR=	lib

######################################################################
TOPDIR=	        ../..
include		$(TOPDIR)/TopRules.mk
######################################################################

LIBS_TO_PACK=	$(LIBCDR) $(LIBDATATREE) $(LIBCDA) $(LIBCX_ASYNC) \
		$(LIBUSEFUL) $(LIBMISC) ../Qcxscheduler/libQcxscheduler.a
libCdr4PyQt.so:	$(LIBS_TO_PACK)
		$(LD)   -o $@ -shared -Wl,-soname,$@ $(ACT_LDFLAGS) \
		-Wl,--whole-archive $(LIBS_TO_PACK) -Wl,--no-whole-archive
