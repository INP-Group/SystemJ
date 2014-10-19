.PHONY:		firsttarget
firsttarget:	all

ifeq "$(strip $(232TYPE))" ""
  DUMMY:=	$(shell echo 'ShadowRules.mk: *** 232TYPE is unset' >&2)
  error		# Makes `make' stop with a "missing separator" error
endif

######################################################################
DRIVERSOURCES=	$(232TYPE)kshd485_drv.c $(232TYPE)nkshd485_drv.c
UTILSSOURCES=	$(232TYPE)setupkshd485.c $(232TYPE)dumpserial.c

232SYMLINKS=	serialhal.h

$(232SYMLINKS):	%:	$(SRCDIR)/%

HALSYMLINKS=	$(232TYPE)serialhal.h

$(232TYPE)kshd485_drv.c:	$(SRCDIR)/kshd485_drv_common.c
$(232TYPE)nkshd485_drv.c:	$(SRCDIR)/nkshd485_drv_common.c
$(232TYPE)serialhal.h:		$(SRCDIR)/stdserialhal.h
$(232TYPE)setupkshd485.c:	$(SRCDIR)/setupkshd485_common.c
$(232TYPE)dumpserial.c:		$(SRCDIR)/dumpserial_common.c

$(232TYPE)kshd485_drv.dep $(232TYPE)nkshd485_drv.dep \
$(232TYPE)setupkshd485.dep $(232TYPE)dumpserial.dep:	serialhal.h $(232TYPE)serialhal.h

$(232TYPE)%.o $(232TYPE)%.dep:	SPECIFIC_DEFINES=-DSERIALHAL_FILE_H='"$(232TYPE)serialhal.h"'\
				-DSERIALHAL_PFX='"$(232DEVPFX)"' \
				-DSERIALDRV_PFX=$(232TYPE)

# --------------------------------------------------------------------

SHD_SYMLINKS=	$(232SYMLINKS) $(HALSYMLINKS) \
		$(DRIVERSOURCES) $(UTILSSOURCES)
SHD_GNTDFILES=	$(SHD_SYMLINKS)

$(SHD_SYMLINKS):
		$(SCRIPTSDIR)/ln-sf_safe.sh $< $@

# ====================================================================
