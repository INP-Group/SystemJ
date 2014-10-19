SUBDIRS=	camac vme
MONO_C_FILES=	\
                host_bpmd_drv.so \
                bpmd_txzi_drv.so \
                bpmd_cfg_drv.so \
                \
                bpmd_txzi_out_drv.so \



LIBMOVING_STATISTICS=			libmoving_statistics.a
libmoving_statistics.a_COMPONENTS=	moving_statistics.o

LIBCOORD_CALC_BEP=			libcoord_calc_bep.a
libcoord_calc_bep.a_COMPONENTS=		coord_calc_bep.o

LIBCOORD_CALC_V2K=			libcoord_calc_v2k.a
libcoord_calc_v2k.a_COMPONENTS=		coord_calc_v2k.o

LIBCOORD_CALC_V5R=			libcoord_calc_v5r.a
libcoord_calc_v5r.a_COMPONENTS=		coord_calc_v5r.o

LIBCOORD_CALC=				libcoord_calc.a
libcoord_calc.a_COMPONENTS=		coord_calc_bep.o coord_calc_v2k.o coord_calc_v5r.o coord_calc.o

ARLIBS=			$(LIBMOVING_STATISTICS) $(LIBCOORD_CALC_BEP) $(LIBCOORD_CALC_V2K) $(LIBCOORD_CALC_V5R) $(LIBCOORD_CALC)

######################################################################
# Local/ShadowRules
.PHONY:         firsttarget
firsttarget:    all

######################################################################
PRJDIR=		..
SECTION=	programs/drivers/DirRules.mk
include		$(PRJDIR)/PrjRules.mk
######################################################################

LOCAL_INCLUDES += -I./given
LOCAL_LDFLAGS  += -lm

host_bpmd_drv.so:	                    libcoord_calc.a $(LIBPZFRAME_DRV)
bpmd_txzi_drv.so:	moving_statistics.o libcoord_calc.a $(LIBPZFRAME_DRV)

bpmd_cfg_drv.so:	$(LIBVDEV)
