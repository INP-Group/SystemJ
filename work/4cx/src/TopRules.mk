######################################################################
#                                                                    #
######################################################################

# ==== Just a sanity check ===========================================

ifeq "$(strip $(TOPDIR))" ""
  DUMMY:=	$(shell echo 'TopRules.mk: *** TOPDIR is unset' >&2)
  error		# Makes `make' stop with a "missing separator" error
endif


# ==== Configuration =================================================

include $(TOPDIR)/GeneralRules.mk

ifeq "$(OUTOFTREE)" ""
  TOP_INCLUDES=	-I$(TOPDIR)/include
else
  TOP_INCLUDES=	-I$(TOPEXPORTSDIR)/include
endif
#!!! Temporarily, while TOPEXPORTSDIR is absent
  TOP_INCLUDES=	-I$(TOPDIR)/include


# ====

#!!! Should port here "X11_LIBS_DIR"+"_L_MOTIF_LIB" woodoism from cx/*Rules.mk
MOTIF_LIBS:=	-L/usr/X11R6/lib \
		-lm -lXm -lXpm -lXt -lSM -lICE -lXext -lXmu -lX11


# ==== Libraries =====================================================

ifeq "$(OUTOFTREE)" ""
  LIB_PREFIX_F= $(TOPDIR)/lib/$(1)/lib$(2).a
else
  LIB_PREFIX_F= $(TOPEXPORTSDIR)/lib/lib$(2).a
endif
#!!! Temporarily, while TOPEXPORTSDIR is absent
  LIB_PREFIX_F= $(TOPDIR)/lib/$(1)/lib$(2).a

LIBMISC:=       $(call LIB_PREFIX_F,misc,misc)
LIBUSEFUL:=     $(call LIB_PREFIX_F,useful,useful)
LIBCXSCHEDULER:=$(call LIB_PREFIX_F,useful,cxscheduler)

LIBXH:=		$(call LIB_PREFIX_F,Xh,Xh)
LIBXH_CXSCHEDULER:=	$(call LIB_PREFIX_F,Xh,Xh_cxscheduler)
LIBAUXMOTIFWIDGETS:=	$(call LIB_PREFIX_F,AuxMotifWidgets,AuxMotifWidgets)

LIBCX_ASYNC:=	$(call LIB_PREFIX_F,cxlib,cx_async)
LIBCX_SYNC:=	$(call LIB_PREFIX_F,cxlib,cx_sync)
LIBCDA:=	$(call LIB_PREFIX_F,cda,cda)

LIBCXSD:=	$(call LIB_PREFIX_F,srv,cxsd)
LIBCXSD_PLUGINS:=	$(call LIB_PREFIX_F,srv,cxsd_plugins)
LIBREMDRVLET:=	$(call LIB_PREFIX_F,rem,remdrvlet)
LIBREMSRV:=	$(call LIB_PREFIX_F,rem,remsrv)

LIBDATATREE:=	$(call LIB_PREFIX_F,datatree,datatree)
LIBCDR:=	$(call LIB_PREFIX_F,Cdr,Cdr)
LIBKNOBSCORE:=	$(call LIB_PREFIX_F,KnobsCore,KnobsCore)
LIBMOTIFKNOBS:=	$(call LIB_PREFIX_F,MotifKnobs,MotifKnobs)
LIBCHL:=	$(call LIB_PREFIX_F,Chl,Chl)
