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



# ====

#!!! Should port here "X11_LIBS_DIR"+"_L_MOTIF_LIB" woodoism from Rules.mk
ifeq "1" "0"
MOTIF_LIBS:=    -L/usr/X11R6/lib \
		-lm -lXm -lXpm -lXt -lSM -lICE -lXext -lXmu -lX11
endif

# Is that woodoism still needed?
ifneq "$(wildcard /usr/include/X11)" "/usr/include/X11"
   TOP_INCLUDES+=	-I/usr/X11R6/include
endif
ifneq "$(wildcard /usr/include/Xm)" "/usr/include/Xm"
  ifneq "$(wildcard /usr/X11R6/include/Xm)" "/usr/X11R6/include/Xm"
     TOP_INCLUDES+=	-I/usr/local/include
     _L_MOTIF_LIB=	-L/usr/local/lib
  endif
endif

# Damn, a hack for FC4@x86_64, where libXm.so is found in /usr/X11R6/lib64
ifeq ":$(OS):$(CPU):" ":LINUX:X86_64:"
  X11_LIBS_DIR=/usr/X11R6/lib64
else
  X11_LIBS_DIR=/usr/X11R6/lib
endif
_L_X11_LIB=-L$(X11_LIBS_DIR)

MOTIF_LIBS:=	$(_L_X11_LIB) $(_L_MOTIF_LIB) \
		-lm -lXm -lXpm -lXt -lSM -lICE -lXext -lXmu -lX11

# ==== Directories ===================================================

TOPEXPORTSDIR=	$(TOPDIR)/../exports
TOPINSTALLDIR=	/tmp/cx
# For autoconf-lovers -- they can use "make PREFIX=<path> install"
ifneq "$(PREFIX)" ""
  TOPINSTALLDIR=$(PREFIX)
endif

INSTALLSUBDIRS=	bin sbin configs include lib settings

INSTALL_bin_DIR=	$(TOPINSTALLDIR)/bin
INSTALL_sbin_DIR=	$(TOPINSTALLDIR)/sbin
INSTALL_configs_DIR=	$(TOPINSTALLDIR)/configs
INSTALL_include_DIR=	$(TOPINSTALLDIR)/include
INSTALL_lib_DIR=	$(TOPINSTALLDIR)/lib
INSTALL_settings_DIR=	$(TOPINSTALLDIR)/settings


# ==== Libraries =====================================================

ifeq "$(OUTOFTREE)" ""
  LIB_PREFIX_F=	$(TOPDIR)/lib/$(1)/lib$(2).a
else
  LIB_PREFIX_F=	$(TOPEXPORTSDIR)/lib/lib$(2).a
endif

LIBMISC:=		$(call LIB_PREFIX_F,misc,misc)
LIBUSEFUL:=		$(call LIB_PREFIX_F,useful,useful)
LIBCXSCHEDULER:=	$(call LIB_PREFIX_F,useful,cxscheduler)

LIBXH:=			$(call LIB_PREFIX_F,Xh,Xh)
LIBXH_CXSCHEDULER:=	$(call LIB_PREFIX_F,Xh,Xh_cxscheduler)
LIBAUXMOTIFWIDGETS:=	$(call LIB_PREFIX_F,AuxMotifWidgets,AuxMotifWidgets)

LIBCX_ASYNC:=		$(call LIB_PREFIX_F,cxlib,cx_async)
LIBCX_SYNC:=		$(call LIB_PREFIX_F,cxlib,cx_sync)
LIBCDA:=		$(call LIB_PREFIX_F,cda,cda)
LIBDATATREE:=		$(call LIB_PREFIX_F,Cdr,datatree)
LIBCDR:=		$(call LIB_PREFIX_F,Cdr,Cdr)

LIBKNOBS:=		$(call LIB_PREFIX_F,Knobs,Knobs)
LIBCHL:=		$(call LIB_PREFIX_F,Chl,Chl)
LIBZASTADC_DATA:=	$(call LIB_PREFIX_F,zastadc,zastadc_data)
LIBZASTADC_GUI:=	$(call LIB_PREFIX_F,zastadc,zastadc_gui)
LIBPZFRAME_DRV:=	$(call LIB_PREFIX_F,pzframe,pzframe_drv)
LIBPZFRAME_DATA:=	$(call LIB_PREFIX_F,pzframe,pzframe_data)
LIBPZFRAME_GUI:=	$(call LIB_PREFIX_F,pzframe,pzframe_gui)
LIBVDEV:=		$(call LIB_PREFIX_F,vdev,vdev)
