MONO_C_FILES=	cx-rdt cx-wrt cx-sub cx-setchan cx-bigc \
		cx-alive cx-console

EXPORTSFILES=	$(MONO_C_FILES)
EXPORTSDIR=	bin

TOPDIR=	        ../..
include		$(TOPDIR)/TopRules.mk

$(MONO_C_FILES):	$(LIBCX_SYNC) $(LIBCXSCHEDULER) $(LIBUSEFUL) $(LIBMISC)
