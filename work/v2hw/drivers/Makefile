SUBDIRS=        can cpci vme serial ccd eth camac
MONO_C_FILES=	s2v_drv.so pzframe_gw_drv.so

######################################################################
PRJDIR=		..
SECTION=	programs/drivers/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
######################################################################

s2v_drv.so:		$(LIBPZFRAME_DRV)
pzframe_gw_drv.so:	$(LIBCDA) $(LIBCX_ASYNC) $(zLIBUSEFUL) $(LIBMISC)
pzframe_gw_drv.so:	SPECIFIC_LDFLAGS=-lm
