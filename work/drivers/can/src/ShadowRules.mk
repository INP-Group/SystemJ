######################################################################
#                                                                    #
#  drivers/can/common_sources/DirRules.mk                            #
#      Standard rules for local-compiling of CAN stuff               #
#                                                                    #
#  Input parameters:                                                 #
#      CANKOZ_SRCDIR                                                 #
#      CANKOZ_PFX                                                    #
#      CANKOZ_HALS_INCLFILE                                          #
#                                                                    #
#                                                                    #
#                                                                    #
######################################################################

.PHONY:			firsttarget
firsttarget:		all

ifeq "$(CANKOZ_HAL_DESCR)" ""
  CANKOZ_HAL_DESCR=	$(CANKOZ_PFX)
endif

include			$(CANKOZ_SRCDIR)/LISTOFCANDRIVERS.mk

CANKOZ_HALHDR=		$(CANKOZ_PFX)canhal.h
CANKOZ_DRV_HEADERS=	cankoz_numbers.h cankoz_lyr.h
CANKOZ_LYR_HEADERS=	cankoz_strdb.h \
			canhal.h $(CANKOZ_HALHDR) $(CANKOZ_HALS_INCLFILE)
CANKOZ_HEADERS=		$(CANKOZ_DRV_HEADERS) $(CANKOZ_LYR_HEADERS)
CANKOZ_DRV_SRCS=	$(addsuffix _drv.c, $(LISTOFCANDRIVERS))
CANKOZ_LYR_SRCS=	cankoz_lyr_common.c
CANKOZ_UTL_SRCS=	canmon_common.c
CANKOZ_C_SRCS=		$(CANKOZ_DRV_SRCS) $(CANKOZ_LYR_SRCS) $(CANKOZ_UTL_SRCS)
CANKOZ_SYMLINKS=	$(CANKOZ_HEADERS) $(CANKOZ_C_SRCS)
$(CANKOZ_SYMLINKS):	%: $(CANKOZ_SRCDIR)/%

SYMLINKS=		$(CANKOZ_SYMLINKS)
LOCAL_GNTDFILES+=	$(SYMLINKS) $(CANKOZ_PFX)cankoz_lyr.c $(CANKOZ_PFX)canmon.c

$(SYMLINKS):
			$(SCRIPTSDIR)/ln-sf_safe.sh $< $@

$(CANKOZ_LYR_SRCS:.c=.%):	SPECIFIC_DEFINES=-DCANHAL_FILE_H='"$(CANKOZ_HALHDR)"' -DCANLYR_NAME=$(CANKOZ_PFX)cankoz -DCANHAL_DESCR="$(CANKOZ_HAL_DESCR)"

$(CANKOZ_DRV_SRCS:.c=.d):	$(CANKOZ_DRV_HEADERS)
$(CANKOZ_LYR_SRCS:.c=.d):	$(CANKOZ_DRV_HEADERS) $(CANKOZ_LYR_HEADERS)
$(CANKOZ_UTL_SRCS:.c=.d):	$(CANKOZ_DRV_HEADERS) $(CANKOZ_LYR_HEADERS)

$(CANKOZ_UTL_SRCS:.c=.%):	SPECIFIC_DEFINES=-DCANHAL_FILE_H='"$(CANKOZ_HALHDR)"'

$(CANKOZ_PFX)canmon:		$(LIBMISC)

$(CANKOZ_PFX)cankoz_lyr.c $(CANKOZ_PFX)canmon.c:
				touch $@

# #### END OF CommonRules.mk #########################################
