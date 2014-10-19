######################################################################
#                                                                    #
#  lib/cda/Makefile                                                  #
#      Rules for making Cx Data Access library                       #
#                                                                    #
######################################################################

V2LIBS_DIR=		../v2cx
v2all_COMPONENTS_file=	$(V2LIBS_DIR)/v2all_COMPONENTS.mk
include			$(v2all_COMPONENTS_file)
MAKEFILE_PARTS+=	$(v2all_COMPONENTS_file)

ARLIBS=		libcda.a
libcda.a_COMPONENTS=	cda_core.o cda_err.o \
			cda_plugmgr.o \
			cda_d_cx.o cda_d_local.o \
			cda_d_vcas.o cda_d_epics.o \
			cda_d_v2cx.o v2subsysaccess.o \
			cda_f_fla.o
libcda.a_GIVEN_COMPONENTS=	$(foreach F, $(v2all_COMPONENTS), $(V2LIBS_DIR)/$F)

TOPDIR=         ../..
include $(TOPDIR)/TopRules.mk

cda_d_v2cx.o   v2subsysaccess.o \
cda_d_v2cx.d   v2subsysaccess.d \
cda_d_v2cx.dep v2subsysaccess.dep: SPECIFIC_INCLUDES+=-I$(V2LIBS_DIR)

# #### END OF Makefile ###############################################
